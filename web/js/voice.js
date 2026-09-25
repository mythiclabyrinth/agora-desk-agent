// Voice in the conversation: board button mirroring, speaker toggle, browser/desk microphone.
// Spoken exchanges happen on the board, not in this page. Copy each finished one into the chat log once.
// A "typed" source is a normal chat message being read aloud: the page already logged it.
let voiceState = { state: 'idle' },
  voiceCancelledAt = 0;
function trackVoice(v) {
  // A double click on the talk button drops the recording; the composer says so for a few seconds.
  if (v.cancelled && voiceState.state === 'recording') voiceCancelledAt = Date.now();
  voiceState = v;
  voiceBusy = ['recording', 'transcribing', 'waiting', 'speaking'].includes(v.state);
  if (audioMode === 'desk') micRecording = v.state === 'recording' && v.source === 'page';
  syncMic();
  if (!v.job || !(v.state === 'done' || v.state === 'failed')) return;
  const key = v.job + ':' + (v.heard || '').slice(0, 40) + ':' + v.state;
  if (key === voiceSeen) return;
  voiceSeen = key;
  storage.set('localStorage', 'desk-voice-seen', key);
  const id = v.agent || current;
  if (v.source === 'typed') {
    if (v.error) note($('#chat-note'), v.error, true);
    return;
  }
  if (v.heard) remember(id, { role: 'you', text: v.heard, spoken: true });
  if (v.state === 'done')
    remember(id, {
      role: 'agent',
      name: agentLabel(id),
      text: v.reply || '(The agent returned an empty reply.)',
    });
  if (v.error) remember(id, { role: 'system', text: v.error });
  if (!$('#chat').hidden && id === current) renderChat();
}
// Chat audio has two homes. "browser": this page records with its own microphone, the board forwards the clip
// to the speech API and hands back the transcript, and spoken replies come back as WAV for the page to play.
// "desk": the mic button drives the board's INMP441 and replies play on the MAX98357A. The keys never leave the board.
let audioMode = storage.get('localStorage', 'desk-audio') === 'desk' ? 'desk' : 'browser';
const micSupported = !!(
  navigator.mediaDevices &&
  navigator.mediaDevices.getUserMedia &&
  window.MediaRecorder
);
const micBlockedWhy =
  'This browser cannot use its microphone on this page. Choose Desk device in Settings › Voice › Devices to use the desk microphone.';
function setAudioMode(mode) {
  audioMode = mode === 'desk' ? 'desk' : 'browser';
  storage.set('localStorage', 'desk-audio', audioMode);
  if (audioMode === 'desk') stopBrowserRecording(true);
  else micRecording = false;
  stopPlayer();
  syncSpeak();
  syncMic();
}
// Speaker toggle: off by default; when on, replies are read aloud (by the board or by this page, per audioMode).
let speakOn = storage.get('localStorage', 'desk-speak') === '1',
  micRecording = false,
  micBusy = false,
  pageVoice = false;
function syncSpeak() {
  $('#speak').classList.toggle('on', speakOn);
  $('#speak').setAttribute('aria-pressed', String(speakOn));
  $('#speak').title = speakOn
    ? 'Replies are read aloud'
    : 'Read replies aloud' + (audioMode === 'desk' ? ' on the desk speaker' : ' in this browser');
}
$('#speak').addEventListener('click', () => {
  speakOn = !speakOn;
  storage.set('localStorage', 'desk-speak', speakOn ? '1' : '0');
  if (!speakOn) stopPlayer();
  syncSpeak();
  note(
    $('#chat-note'),
    speakOn
      ? audioMode === 'desk'
        ? 'Replies will be read aloud through the desk speaker.'
        : 'Replies will be read aloud in this browser.'
      : 'Replies stay on screen.',
  );
});
syncSpeak();
let player = null;
function stopPlayer() {
  if (player) {
    player.pause();
    player = null;
  }
}
async function sayInBrowser(text) {
  stopPlayer();
  pageVoice = true;
  note($('#chat-note'), 'Reading the reply aloud…');
  const controller = new AbortController();
  const timer = setTimeout(() => controller.abort(), 90000);
  try {
    const res = await fetch('/api/voice/say', {
      method: 'POST',
      cache: 'no-store',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ text }),
      signal: controller.signal,
    });
    if (!res.ok) {
      let msg = 'The board could not fetch the speech.';
      try {
        msg = (await res.json()).error || msg;
      } catch (_) {}
      throw new Error(msg);
    }
    const url = URL.createObjectURL(await res.blob());
    const a = new Audio(url);
    player = a;
    a.addEventListener('ended', () => {
      URL.revokeObjectURL(url);
      if (player === a) player = null;
      syncComposer();
    });
    await a.play();
    syncComposer();
  } catch (e) {
    player = null;
    note(
      $('#chat-note'),
      e.name === 'AbortError'
        ? 'Speech took too long. Replies stay on screen this time.'
        : e.name === 'NotAllowedError'
          ? 'Your browser blocked playback. Tap the speaker icon once, then send again.'
          : e.message,
      true,
    );
  } finally {
    clearTimeout(timer);
    pageVoice = false;
  }
}
// Mic button. Desk mode opens the board's microphone for the open agent; tap again to send.
// Browser mode records here, sends the clip to the board for transcription, then posts the text like a typed message.
let recorder = null,
  recorderChunks = [];
function syncMic() {
  const m = $('#mic');
  const a = agents.find((a) => a.id === current);
  m.classList.toggle('rec', micRecording);
  m.setAttribute('aria-pressed', String(micRecording));
  m.querySelector('use').setAttribute('href', micRecording ? '#i-stop' : '#i-mic');
  m.title = micRecording
    ? 'Tap to send'
    : audioMode === 'desk'
      ? 'Talk through the desk microphone'
      : micSupported
        ? 'Talk through this browser’s microphone'
        : 'Microphone needs a secure page';
  m.setAttribute('aria-label', m.title);
  m.disabled =
    micBusy ||
    !online ||
    !a?.ready ||
    (!micRecording && (voiceBusy || !!waitingJob || sending || boardBusy || pageVoice));
}
function stopBrowserRecording(discard) {
  const r = recorder;
  if (!r) return;
  recorder = null;
  if (discard) recorderChunks = [];
  try {
    if (r.state !== 'inactive') r.stop();
  } catch (_) {}
  r.stream.getTracks().forEach((t) => t.stop());
}
function pickMime() {
  return (
    ['audio/webm;codecs=opus', 'audio/webm', 'audio/mp4', 'audio/ogg;codecs=opus'].find((t) =>
      MediaRecorder.isTypeSupported(t),
    ) || ''
  );
}
async function startBrowserRecording() {
  const stream = await navigator.mediaDevices.getUserMedia({
    audio: { channelCount: 1, echoCancellation: true, noiseSuppression: true },
  });
  const mime = pickMime();
  const r = new MediaRecorder(stream, mime ? { mimeType: mime } : undefined);
  recorderChunks = [];
  r.addEventListener('dataavailable', (e) => {
    if (e.data && e.data.size) recorderChunks.push(e.data);
  });
  r.addEventListener('stop', () => {
    stream.getTracks().forEach((t) => t.stop());
    if (recorder === r) recorder = null;
    const chunks = recorderChunks;
    recorderChunks = [];
    if (chunks.length) finishBrowserRecording(new Blob(chunks, { type: r.mimeType || mime || 'audio/webm' }));
  });
  recorder = r;
  r.start(250);
  // The board caps its own recordings at 15 s; a minute is plenty here and keeps the upload small.
  setTimeout(() => {
    if (recorder === r && micRecording) {
      micRecording = false;
      stopBrowserRecording(false);
      syncMic();
    }
  }, 60000);
}
async function finishBrowserRecording(blob) {
  const id = current;
  const ext = /mp4/.test(blob.type)
    ? 'mp4'
    : /ogg/.test(blob.type)
      ? 'ogg'
      : /wav/.test(blob.type)
        ? 'wav'
        : 'webm';
  if (blob.size < 2000) {
    note($('#chat-note'), 'That was too short. Hold on a moment longer before tapping again.', true);
    return;
  }
  pageVoice = true;
  micBusy = true;
  syncMic();
  note($('#chat-note'), 'Writing down what you said…');
  const form = new FormData();
  form.append('file', blob, 'clip.' + ext);
  const controller = new AbortController();
  const timer = setTimeout(() => controller.abort(), 60000);
  try {
    const res = await fetch('/api/voice/transcribe', {
      method: 'POST',
      cache: 'no-store',
      body: form,
      signal: controller.signal,
    });
    let data = {};
    try {
      data = await res.json();
    } catch (_) {}
    if (!res.ok) throw new Error(data.error || 'The board could not transcribe that.');
    const text = (data.text || '').trim();
    if (!text) {
      note($('#chat-note'), 'I didn’t catch anything. Try again a little closer to the microphone.', true);
      return;
    }
    pageVoice = false;
    micBusy = false;
    await sendMessage(id, text, true);
  } catch (e) {
    note(
      $('#chat-note'),
      e.name === 'AbortError' ? 'Transcription took too long. Try a shorter clip.' : e.message,
      true,
    );
  } finally {
    clearTimeout(timer);
    pageVoice = false;
    micBusy = false;
    syncMic();
  }
}
$('#mic').addEventListener('click', async () => {
  if (micBusy) return;
  micBusy = true;
  const id = current;
  try {
    if (audioMode === 'browser') {
      if (micRecording) {
        micRecording = false;
        stopBrowserRecording(false);
        note($('#chat-note'), 'Sending what you said…');
      } else {
        if (!micSupported) throw new Error(micBlockedWhy);
        await startBrowserRecording();
        micRecording = true;
        note($('#chat-note'), 'Listening… tap the mic again to send.');
      }
    } else if (micRecording) {
      await api('/api/voice/talk', { action: 'stop' });
      micRecording = false;
      note($('#chat-note'), 'Sending what you said…');
      pollVoice();
    } else {
      await api('/api/voice/talk', { action: 'start', agent: id });
      micRecording = true;
      voiceBusy = true;
      note($('#chat-note'), 'Listening… tap the mic again to send.');
      pollVoice();
    }
    syncMic();
  } catch (err) {
    micRecording = false;
    stopBrowserRecording(true);
    note(
      $('#chat-note'),
      err.name === 'NotAllowedError'
        ? 'Microphone access was denied. Allow it in the browser’s site settings and try again.'
        : err.name === 'NotFoundError'
          ? 'No microphone was found on this device.'
          : err.message,
      true,
    );
  } finally {
    micBusy = false;
    syncMic();
  }
});
let voicePolling = false;
async function pollVoice() {
  if (voicePolling) return;
  voicePolling = true;
  try {
    do {
      await new Promise((r) => setTimeout(r, 1000));
      await refreshStatus();
    } while (voiceBusy && !document.hidden);
  } finally {
    voicePolling = false;
  }
}
