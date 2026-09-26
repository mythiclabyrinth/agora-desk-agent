// The conversation view: agent picker, composer, message log, sending and waiting for replies.
function selectAgent(id) {
  if (waitingJob || sending) return;
  drafts[draftAgent] = $('#box').value;
  current = id;
  draftAgent = id;
  $('#box').value = drafts[id] || '';
  storage.set('sessionStorage', 'desk-agent', id);
  renderChat();
  resizeComposer();
}
function syncComposer() {
  const a = agents.find((a) => a.id === current);
  const bytes = new TextEncoder().encode($('#box').value.trim()).length;
  const busy = !!waitingJob || sending || boardBusy;
  $('#send').disabled = !online || busy || !a?.ready || !$('#box').value.trim() || bytes > 2000;
  $('#box').disabled = !a?.ready;
  $('#send').firstChild.textContent = busy ? 'Waiting…' : 'Send message';
  syncMic();
  $('#char-count').textContent = bytes.toLocaleString() + ' / 2,000 bytes';
  $('#char-count').classList.toggle('char-limit', bytes > 2000);
  if (bytes > 2000) note($('#chat-note'), 'Message is too long. Shorten it to 2,000 bytes.', true);
  else if (!waitingJob && !sending && !pageVoice)
    note(
      $('#chat-note'),
      !online
        ? 'Connect your board to Wi-Fi to send.'
        : micRecording
          ? 'Listening… tap the mic again to send.'
          : player
            ? 'Reading the reply aloud…'
            : voiceBusy
              ? voiceBanner(voiceState)
              : boardBusy
                ? 'The board is busy listening.'
                : Date.now() - voiceCancelledAt < 5000
                  ? 'Cancelled at the desk. Nothing was sent.'
                  : a?.ready
                    ? 'Ready for your next message.'
                    : 'Set up this agent to start a conversation.',
    );
}
function resizeComposer() {
  const box = $('#box');
  box.style.height = 'auto';
  box.style.height = Math.min(box.scrollHeight, 150) + 'px';
  syncComposer();
}
$('#box').addEventListener('input', () => {
  drafts[current] = $('#box').value;
  resizeComposer();
});
$('#box').addEventListener('keydown', (e) => {
  if (e.key === 'Enter' && (e.metaKey || e.ctrlKey) && !e.isComposing) {
    e.preventDefault();
    if (!$('#send').disabled) $('#composer').requestSubmit();
  }
});
function renderChat() {
  const picker = $('#picker');
  picker.replaceChildren();
  agents.forEach((a) => {
    const b = button('', '', () => selectAgent(a.id));
    b.classList.toggle('on', a.id === current);
    b.setAttribute('aria-pressed', String(a.id === current));
    b.disabled = !!waitingJob || sending;
    const text = el('span');
    text.append(
      el('strong', '', a.name || a.title || a.id),
      el('small', '', a.ready ? 'Configured' : 'Setup needed'),
    );
    b.append(agentIcon(a.id), text);
    picker.append(b);
  });
  const a = agents.find((a) => a.id === current);
  $('#chat-name').textContent = a?.name || a?.title || 'Your conversation';
  $('#chat-detail').textContent = a?.ready
    ? 'Type a message or use the microphone'
    : 'Connect this agent to start chatting';
  $('#chat-state').textContent =
    waitingJob || sending ? 'Listening' : a?.ready ? 'Configured' : 'Setup needed';
  $('#chat-state').classList.toggle('on', !!a?.ready);
  const log = $('#log');
  log.replaceChildren();
  const rows = logs(current);
  if (!rows.length) {
    const empty = el('div', 'empty');
    empty.append(
      agentIcon(current),
      el('h2', '', a?.ready ? 'What would you like to work on?' : 'Let’s make a connection.'),
      el(
        'p',
        '',
        a?.ready
          ? 'Ask a question, share an idea, or pick up where your work left off.'
          : agents.length
            ? 'Add this agent’s connection details in Settings to start a conversation.'
            : 'Waiting for your board. Your agents will appear here once connected.',
      ),
    );
    if (a?.ready) {
      const suggestions = el('div', 'suggestions');
      ['What are you working on?', 'Help me plan my next step'].forEach((text) =>
        suggestions.append(
          button(text, '', () => {
            $('#box').value = text;
            resizeComposer();
            $('#box').focus();
          }),
        ),
      );
      empty.append(suggestions);
    } else if (a) empty.append(button('Set up ' + (a.name || a.title), 'ghost', () => openSettings(a.id)));
    log.append(empty);
  } else
    rows.forEach((row) => {
      const role = ['you', 'agent', 'system'].includes(row.role) ? row.role : 'system';
      const bubble = el('div', 'bubble ' + role);
      bubble.append(
        el(
          'span',
          'who',
          role === 'you'
            ? row.spoken
              ? 'You · spoken'
              : 'You'
            : role === 'agent'
              ? row.name || a?.name || 'Agent'
              : 'Connection update',
        ),
        document.createTextNode(row.text),
      );
      log.append(bubble);
    });
  if (waitingJob || sending) {
    const typing = el('div', 'typing');
    typing.append(
      el('i'),
      el('i'),
      el('i'),
      el('span', '', sending ? 'Sending your message…' : 'Giving your agent a moment…'),
    );
    log.append(typing);
  }
  log.scrollTop = rows.length || waitingJob || sending ? log.scrollHeight : 0;
  syncComposer();
}
async function waitFor(job, id) {
  waitingJob = job;
  boardBusy = true;
  renderChat();
  let failures = 0;
  const started = Date.now();
  banner('Listening for your agent’s reply…');
  note($('#chat-note'), 'Listening… You can leave this tab open.');
  try {
    while (true) {
      await new Promise((r) => setTimeout(r, 1200));
      let s;
      try {
        s = await api('/api/listen');
        failures = 0;
      } catch (e) {
        if (++failures >= 5) throw e;
        banner('Connection interrupted. Trying to pick up your reply…');
        continue;
      }
      if (s.job !== job) {
        remember(id, {
          role: 'system',
          text: 'The board restarted or a newer conversation replaced this request.',
        });
        break;
      }
      if (s.state === 'done') {
        remember(id, {
          role: 'agent',
          name: s.from,
          text: s.reply || '(The agent returned an empty reply.)',
        });
        if (speakOn && audioMode === 'browser' && s.reply) sayInBrowser(s.reply);
        break;
      }
      if (s.state === 'failed' || s.state === 'idle') {
        remember(id, {
          role: 'system',
          text: s.error || 'The board stopped listening. You can try another message.',
        });
        break;
      }
      if (Date.now() - started > 240000)
        throw new Error(
          'The reply is taking longer than expected. Check your agent bridge before sending again.',
        );
      note(
        $('#chat-note'),
        'Listening' +
          (s.from ? ' for ' + s.from : '') +
          ' · ' +
          Math.floor((s.waited_ms || Date.now() - started) / 1000) +
          's',
      );
    }
  } catch (e) {
    remember(id, { role: 'system', text: e.message + ' Check Agora for a reply before resending.' });
  } finally {
    waitingJob = 0;
    boardBusy = false;
    storage.remove('sessionStorage', 'desk-pending');
    banner('');
    renderChat();
    refreshStatus();
  }
}
// Typed and browser-spoken messages share this path; only the board speaker asks the board to read the reply.
async function sendMessage(id, text, spoken = false) {
  const a = agents.find((x) => x.id === id);
  if (!a?.ready || !text || waitingJob || sending) return;
  sending = true;
  remember(id, { role: 'you', text, spoken });
  renderChat();
  note($('#chat-note'), 'Sending…');
  try {
    const data = await api('/api/chat', { agent: id, text, speak: speakOn && audioMode === 'desk' });
    sending = false;
    if (data.speak_note) remember(id, { role: 'system', text: data.speak_note });
    storage.set('sessionStorage', 'desk-pending', JSON.stringify({ job: data.job, agent: id }));
    await waitFor(data.job, id);
  } catch (err) {
    sending = false;
    waitingJob = 0;
    remember(id, {
      role: 'system',
      text: err.message + ' Your message is back in the composer. Check Agora before retrying.',
    });
    if (!$('#box').value) {
      $('#box').value = text;
      drafts[id] = text;
    }
    renderChat();
    resizeComposer();
    note($('#chat-note'), err.message, true);
    refreshStatus();
  }
}
$('#composer').addEventListener('submit', async (e) => {
  e.preventDefault();
  if ($('#send').disabled) return;
  const text = $('#box').value.trim();
  if (!text) return;
  $('#box').value = '';
  drafts[current] = '';
  resizeComposer();
  await sendMessage(current, text);
});
