// Settings > Voice: credentials, speech-to-text, text-to-speech, talk button, chat page audio.
// Voice catalog, copied from Agora's config.rs so the two pickers offer the same choices.
const VOICE_CATALOG = {
  providers: [
    { id: 'groq', label: 'Groq' },
    { id: 'openai', label: 'OpenAI' },
  ],
  stt: {
    groq: ['whisper-large-v3-turbo', 'whisper-large-v3', 'distil-whisper-large-v3-en'],
    openai: ['gpt-4o-mini-transcribe', 'whisper-1'],
  },
  tts: {
    groq: ['canopylabs/orpheus-v1-english', 'canopylabs/orpheus-arabic-saudi'],
    openai: ['gpt-4o-mini-tts', 'tts-1', 'tts-1-hd'],
  },
  voices: {
    openai: [
      ['alloy', 'Alloy — neutral'],
      ['ash', 'Ash — male'],
      ['ballad', 'Ballad — male'],
      ['coral', 'Coral — female'],
      ['echo', 'Echo — male'],
      ['fable', 'Fable — male'],
      ['onyx', 'Onyx — male'],
      ['nova', 'Nova — female'],
      ['sage', 'Sage — neutral'],
      ['shimmer', 'Shimmer — female'],
      ['verse', 'Verse — male'],
    ],
    groq: [
      ['autumn', 'Autumn — female'],
      ['diana', 'Diana — female'],
      ['hannah', 'Hannah — female'],
      ['austin', 'Austin — male'],
      ['daniel', 'Daniel — male'],
      ['troy', 'Troy — male'],
    ],
    groqArabic: [
      ['abdullah', 'Abdullah — male'],
      ['fahad', 'Fahad — male'],
      ['sultan', 'Sultan — male'],
      ['lulwa', 'Lulwa — female'],
      ['noura', 'Noura — female'],
      ['aisha', 'Aisha — female'],
    ],
  },
  accents: [
    ['american', 'American English'],
    ['british', 'British English'],
    ['arabic', 'Arabic (Saudi)'],
  ],
};
function selectField(label, options, value, help) {
  const wrap = el('label', '', label);
  const select = el('select');
  options.forEach((o) => {
    const [id, text] = Array.isArray(o) ? o : [o, o];
    const opt = el('option', '', text);
    opt.value = id;
    select.append(opt);
  });
  if (value && ![...select.options].some((o) => o.value === value)) {
    const opt = el('option', '', value);
    opt.value = value;
    select.append(opt);
  }
  select.value = value || select.options[0]?.value || '';
  wrap.append(select);
  if (help) wrap.append(el('small', '', help));
  return { wrap, select };
}
function advancedModel(wrap) {
  const details = el('details', 'advanced-setting');
  details.append(el('summary', '', 'Model options'), wrap);
  return details;
}
function panel(title, copy) {
  const p = el('div', 'panel');
  const head = el('div', 'panel-heading');
  head.append(el('h3', '', title), el('p', '', copy));
  p.append(head);
  return p;
}
async function renderVoice() {
  const root = $('#voice-form');
  root.replaceChildren(el('p', 'form-empty', 'Loading voice settings…'));
  let v;
  try {
    v = await api('/api/voice');
  } catch (e) {
    root.replaceChildren(
      el('p', 'form-empty', 'Couldn’t load voice settings. ' + e.message),
      button('Try again', 'ghost', renderVoice),
    );
    return;
  }
  voiceBuilt = true;
  root.replaceChildren();
  const stack = el('div', 'voice-stack');
  let keysChanged = () => {};
  const tabs = el('div', 'sub voice-tabs');
  tabs.setAttribute('role', 'tablist');
  tabs.setAttribute('aria-label', 'Voice settings');
  const credentials = el('div', 'voice-stack'),
    speech = el('div', 'voice-stack');
  const panes = { credentials, speech };
  const activate = (name, focus = false) => {
    voiceTab = name;
    Object.entries(panes).forEach(([id, pane]) => {
      pane.hidden = id !== name;
      const tab = tabs.querySelector('[data-voice-tab="' + id + '"]');
      tab.classList.toggle('on', id === name);
      tab.setAttribute('aria-selected', String(id === name));
      tab.tabIndex = id === name ? 0 : -1;
      if (focus && id === name) tab.focus();
    });
  };
  [
    ['credentials', 'Credentials'],
    ['speech', 'Speech & audio'],
  ].forEach(([id, title], index) => {
    const tab = button(title, '', () => activate(id));
    tab.id = 'voice-tab-' + id;
    tab.dataset.voiceTab = id;
    tab.setAttribute('role', 'tab');
    tab.setAttribute('aria-controls', 'voice-panel-' + id);
    tab.addEventListener('keydown', (e) => {
      if (['ArrowLeft', 'ArrowRight', 'Home', 'End'].includes(e.key)) {
        e.preventDefault();
        activate(
          e.key === 'Home' ? 'credentials' : e.key === 'End' ? 'speech' : index ? 'credentials' : 'speech',
          true,
        );
      }
    });
    panes[id].id = 'voice-panel-' + id;
    panes[id].setAttribute('role', 'tabpanel');
    panes[id].setAttribute('aria-labelledby', tab.id);
    panes[id].tabIndex = 0;
    tabs.append(tab);
  });
  // Credentials — write-only keys, one row per provider, like Agora's API keys card.
  const creds = panel(
    'Connect a speech provider',
    'Add an API key for the provider you want to use. One key can power both listening and spoken replies.',
  );
  VOICE_CATALOG.providers.forEach((p) => {
    const row = el('div', 'key-row');
    const head = el('div', 'key-head');
    const state = el('span', 'badge' + (v.keys[p.id] ? ' on' : ''), v.keys[p.id] ? 'Saved' : 'Not set');
    head.append(el('strong', '', p.label + ' API key'), state);
    const line = el('div', 'key-input');
    const input = el('input');
    input.type = 'password';
    input.setAttribute('aria-label', p.label + ' API key');
    input.autocomplete = 'new-password';
    input.spellcheck = false;
    input.maxLength = 500;
    input.placeholder = v.keys[p.id] ? 'Paste a new key to replace it' : p.id === 'groq' ? 'gsk_…' : 'sk-…';
    const save = button('Save key', 'primary sm');
    const test = button('Test key', 'ghost sm');
    const clear = button('Remove', 'ghost sm');
    const status = el('p', 'note');
    status.setAttribute('role', 'status');
    const actions = el('div', 'key-actions');
    actions.append(test, clear);
    const setSaved = (on) => {
      state.textContent = on ? 'Saved' : 'Not set';
      state.classList.toggle('on', on);
      v.keys[p.id] = on;
      actions.hidden = !on;
      input.placeholder = on ? 'Paste a new key to replace it' : p.id === 'groq' ? 'gsk_…' : 'sk-…';
      keysChanged();
    };
    actions.hidden = !v.keys[p.id];
    save.addEventListener('click', async () => {
      const key = input.value.trim();
      if (!key) {
        note(status, 'Paste a key first.', true);
        return;
      }
      save.disabled = true;
      note(status, 'Saving…');
      try {
        await api('/api/voice/keys', { provider: p.id, api_key: key });
        input.value = '';
        setSaved(true);
        note(status, p.label + ' key saved.');
        status.classList.add('good');
        refreshStatus();
      } catch (err) {
        note(status, err.message, true);
      } finally {
        save.disabled = false;
      }
    });
    test.addEventListener('click', async () => {
      test.disabled = true;
      test.textContent = 'Testing…';
      note(status, 'Checking the key with ' + p.label + '…');
      try {
        await api('/api/voice/test', { provider: p.id }, 20000);
        note(status, p.label + ' credentials work.');
        status.classList.add('good');
      } catch (err) {
        note(status, err.message, true);
      } finally {
        test.disabled = false;
        test.textContent = 'Test key';
      }
    });
    clear.addEventListener('click', async () => {
      if (!confirm('Forget the saved ' + p.label + ' key?')) return;
      clear.disabled = true;
      try {
        await api('/api/voice/keys', { provider: p.id, clear: true });
        setSaved(false);
        note(status, p.label + ' key cleared.');
        refreshStatus();
      } catch (err) {
        note(status, err.message, true);
      } finally {
        clear.disabled = false;
      }
    });
    line.append(input, save);
    row.append(head, line, actions, status);
    creds.append(row);
  });
  // Features — providers and models, saved as soon as a picker changes.
  const featureNote = el('p', 'note');
  featureNote.setAttribute('role', 'status');
  const stt = panel(
    'Listening',
    'Turn your voice into a message for your agent. Choose your speech-to-text service.',
  );
  const sttProvider = selectField(
    'Provider',
    VOICE_CATALOG.providers.map((p) => [p.id, p.label]),
    v.stt_provider,
  );
  const sttModel = selectField('Model', VOICE_CATALOG.stt[v.stt_provider], v.stt_models[v.stt_provider]);
  const sttHint = el('p', 'note');
  stt.append(sttProvider.wrap, advancedModel(sttModel.wrap), sttHint);
  const tts = panel(
    'Spoken replies',
    'Choose how your agent sounds. Turn on the speaker in a conversation to hear replies aloud.',
  );
  const ttsProvider = selectField(
    'Provider',
    VOICE_CATALOG.providers.map((p) => [p.id, p.label]),
    v.tts_provider,
  );
  const accent = selectField('Accent', VOICE_CATALOG.accents, v.accent);
  const voice = selectField('Voice', [], '');
  const ttsModel = selectField('Model', [], '');
  const ttsHint = el('p', 'note');
  const pair = el('div', 'field-pair');
  pair.append(accent.wrap, voice.wrap);
  tts.append(ttsProvider.wrap, pair, advancedModel(ttsModel.wrap), ttsHint);
  const talk = panel(
    'Desk talk button',
    'Hold to speak, release to send. Replies play through your desk speaker.',
  );
  const meta = el('div', 'voice-meta');
  [
    ['Microphone', v.mic ? 'Ready' : 'Unavailable', 'For your spoken messages'],
    ['Speaker', v.speaker ? 'Ready' : 'Unavailable', 'For replies aloud'],
  ].forEach(([k, val, sub]) => {
    const d = el('div');
    d.append(el('strong', '', val), document.createTextNode(k + ' · ' + sub));
    meta.append(d);
  });
  const agentSel = selectField(
    'Send to',
    (agents.length ? agents : agentIds.map((id) => ({ id, name: id }))).map((a) => [
      a.id,
      (a.name || a.title || a.id) + (a.ready === false ? ' · setup needed' : ''),
    ]),
    v.agent,
    'The chat page’s mic uses whichever agent is open there.',
  );
  talk.append(meta, agentSel.wrap);
  const fill = (sel, options, value) => {
    sel.replaceChildren();
    options.forEach((o) => {
      const [id, text] = Array.isArray(o) ? o : [o, o];
      const opt = el('option', '', text);
      opt.value = id;
      sel.append(opt);
    });
    if (value && ![...sel.options].some((o) => o.value === value)) {
      const opt = el('option', '', value);
      opt.value = value;
      sel.append(opt);
    }
    sel.value = value || sel.options[0]?.value || '';
  };
  const ttsVoiceList = () => {
    const p = ttsProvider.select.value;
    if (p === 'openai') return VOICE_CATALOG.voices.openai;
    const arabic = accent.select.value === 'arabic' || /arabic/.test(v.tts_models.groq || '');
    return arabic ? VOICE_CATALOG.voices.groqArabic : VOICE_CATALOG.voices.groq;
  };
  const syncTts = () => {
    const p = ttsProvider.select.value;
    fill(ttsModel.select, VOICE_CATALOG.tts[p], v.tts_models[p]);
    const voices = ttsVoiceList();
    // A voice that cannot speak the chosen model falls back to Agora's default for that combination, and that is what gets saved.
    if (!voices.some(([id]) => id === v.tts_voices[p]))
      v.tts_voices[p] =
        p === 'openai'
          ? accent.select.value === 'british'
            ? 'fable'
            : 'alloy'
          : voices === VOICE_CATALOG.voices.groqArabic
            ? 'noura'
            : 'autumn';
    fill(voice.select, voices, v.tts_voices[p]);
    ttsHint.textContent =
      p === 'groq'
        ? accent.select.value === 'arabic'
          ? 'Arabic voices are available for this accent.'
          : 'Groq uses the same English voices for American and British accents.'
        : /^tts-1/.test(ttsModel.select.value)
          ? 'This model uses the voice’s own accent. Choose gpt-4o-mini-tts to apply your accent preference.'
          : 'Replies use your selected voice and accent.';
    ttsHint.classList.toggle('bad', !v.keys[p]);
    if (!v.keys[p])
      ttsHint.textContent =
        'Add the ' +
        (p === 'groq' ? 'Groq' : 'OpenAI') +
        ' key under Credentials before replies can be spoken.';
  };
  const syncStt = () => {
    const p = sttProvider.select.value;
    fill(sttModel.select, VOICE_CATALOG.stt[p], v.stt_models[p]);
    sttHint.textContent = v.keys[p]
      ? ''
      : 'Add the ' +
        (p === 'groq' ? 'Groq' : 'OpenAI') +
        ' key under Credentials before the mic can be used.';
    sttHint.classList.toggle('bad', !v.keys[p]);
  };
  let saveTimer = 0;
  const saveFeatures = () => {
    clearTimeout(saveTimer);
    saveTimer = setTimeout(async () => {
      note(featureNote, 'Saving…');
      try {
        const data = await api('/api/voice', {
          stt_provider: sttProvider.select.value,
          tts_provider: ttsProvider.select.value,
          stt_model_groq: v.stt_models.groq,
          stt_model_openai: v.stt_models.openai,
          tts_model_groq: v.tts_models.groq,
          tts_model_openai: v.tts_models.openai,
          voice_groq: v.tts_voices.groq,
          voice_openai: v.tts_voices.openai,
          accent: accent.select.value,
          agent: agentSel.select.value,
        });
        note(
          featureNote,
          'Voice settings saved.' +
            (data.stt_ready && data.tts_ready ? '' : ' Add the missing key under Credentials to finish.'),
        );
        featureNote.classList.add('good');
        refreshStatus();
      } catch (err) {
        note(featureNote, err.message, true);
      }
    }, 250);
  };
  sttProvider.select.addEventListener('change', () => {
    v.stt_provider = sttProvider.select.value;
    syncStt();
    saveFeatures();
  });
  sttModel.select.addEventListener('change', () => {
    v.stt_models[sttProvider.select.value] = sttModel.select.value;
    saveFeatures();
  });
  ttsProvider.select.addEventListener('change', () => {
    v.tts_provider = ttsProvider.select.value;
    syncTts();
    saveFeatures();
  });
  accent.select.addEventListener('change', () => {
    v.accent = accent.select.value;
    syncTts();
    saveFeatures();
  });
  voice.select.addEventListener('change', () => {
    v.tts_voices[ttsProvider.select.value] = voice.select.value;
    saveFeatures();
  });
  ttsModel.select.addEventListener('change', () => {
    v.tts_models[ttsProvider.select.value] = ttsModel.select.value;
    syncTts();
    saveFeatures();
  });
  agentSel.select.addEventListener('change', saveFeatures);
  // Per-browser choice, so a phone can use its own mic while the desk's hardware stays for the button.
  const page = panel(
    'Microphone & speaker',
    'Choose where to speak and hear replies when using Conversations. This choice is saved for this browser.',
  );
  const modeSel = selectField(
    'Microphone and speaker',
    [
      ['browser', 'This browser'],
      ['desk', 'Desk device'],
    ],
    audioMode,
  );
  const modeHint = el('p', 'note');
  const syncMode = () => {
    const m = modeSel.select.value;
    modeHint.classList.toggle('bad', m === 'browser' && !micSupported);
    modeHint.textContent =
      m === 'browser'
        ? micSupported
          ? 'Use this phone or computer’s microphone and speaker.'
          : micBlockedWhy + ' Spoken replies still play here.'
        : v.mic && v.speaker
          ? 'Use the microphone and speaker on your desk device.'
          : 'Your desk microphone or speaker is unavailable. Check that both are connected.';
  };
  modeSel.select.addEventListener('change', () => {
    setAudioMode(modeSel.select.value);
    syncMode();
  });
  page.append(modeSel.wrap, modeHint);
  syncMode();
  keysChanged = () => {
    syncStt();
    syncTts();
  };
  syncStt();
  syncTts();
  const privacy = el(
    'p',
    'note',
    'Keys are saved on your desk device and hidden after saving. Audio and text are sent to your selected speech provider.',
  );
  credentials.append(
    creds,
    privacy,
    button('Continue to speech & audio →', 'ghost', () => activate('speech', true)),
  );
  const saveBar = el('div', 'voice-save');
  note(featureNote, 'Changes save automatically.');
  saveBar.append(
    featureNote,
    button('Manage credentials', 'text-button', () => activate('credentials', true)),
  );
  speech.append(saveBar, stt, tts, page, talk);
  stack.append(tabs, credentials, speech);
  root.append(stack);
  activate(voiceTab);
}
