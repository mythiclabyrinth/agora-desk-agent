// Chat log store, home cards, and the periodic /api/status refresh that drives everything.
function loadLog(id) {
  try {
    const rows = JSON.parse(storage.get('localStorage', 'desk-log-' + id) || '[]');
    return Array.isArray(rows) ? rows.filter((r) => r && typeof r.text === 'string').slice(-40) : [];
  } catch (_) {
    return [];
  }
}
const volatileLogs = {};
function logs(id) {
  return volatileLogs[id] || loadLog(id);
}
function remember(id, row) {
  const rows = [...logs(id), row].slice(-40);
  volatileLogs[id] = rows;
  storage.set('localStorage', 'desk-log-' + id, JSON.stringify(rows));
}
function renderCards(list) {
  const sig = JSON.stringify(list);
  if (sig === cardsSignature) return;
  cardsSignature = sig;
  $('#home-cards').replaceChildren();
  $('#agent-count').textContent = list.filter((a) => a.ready).length + ' / ' + list.length + ' configured';
  list.forEach((a) => {
    const card = el('article', 'card');
    const top = el('div', 'card-top');
    const state = el('span', 'badge' + (a.ready ? ' on' : ''), a.ready ? 'Configured' : 'Not connected');
    if (a.ready) state.prepend(el('span', 'dot on'));
    top.append(agentIcon(a.id), state);
    const action = button(
      a.ready ? 'Open conversation' : 'Set up agent',
      a.ready ? 'primary' : 'ghost',
      () => {
        if (a.ready) {
          selectAgent(a.id);
          showTab('chat');
        } else openSettings(a.id);
      },
    );
    action.append(icon('arrow'));
    card.append(
      top,
      el('h3', '', a.name || a.id),
      el('p', '', descriptions[a.id] || 'Your connection to this agent.'),
      action,
    );
    $('#home-cards').append(card);
  });
}
async function refreshStatus() {
  if (statusBusy) return;
  statusBusy = true;
  try {
    const s = await api('/api/status');
    online = !!s.wifi;
    statusAgents = s.agents || [];
    const v = s.voice || {};
    trackVoice(v);
    if (v.wake) trackWake(v.wake);
    boardBusy = !!s.listening || voiceBusy;
    $('#link').textContent = online ? 'Desk online' : 'Wi-Fi not connected';
    $('#link-dot').classList.toggle('on', online);
    $('#device-dot').classList.toggle('on', online);
    $('#device-status').textContent = online ? s.ssid || 'Connected to Wi-Fi' : 'Join a network in Settings';
    $('#device-ip').textContent = online ? s.ip : 'Setup · ' + (s.ap_ip || '192.168.4.1');
    savedSsid = s.ssid || '';
    $('#wifi-current').textContent = savedSsid || 'No network selected';
    $('#wifi-state').textContent = online
      ? 'Connected · ' + s.ip
      : savedSsid
        ? 'Saved · Not connected'
        : 'Ready to connect';
    renderCards(statusAgents);
    $('#where').textContent = online
      ? 'Local device · ' + s.ip + (s.host ? ' · ' + s.host : '')
      : 'Setup network · ' + (s.ap_ip || '192.168.4.1');
    if (!waitingJob && !sending)
      banner(
        voiceBusy
          ? voiceBanner(v)
          : boardBusy
            ? 'The board is listening for a reply in another conversation.'
            : !online
              ? 'Your board is available. Connect it to Wi-Fi in Settings to send messages.'
              : '',
      );
    syncComposer();
  } catch (e) {
    // While the board is uploading a clip or fetching speech it cannot answer; that is not an outage.
    if (voiceBusy || pageVoice) {
      return;
    }
    online = false;
    $('#link').textContent = 'Desk unreachable';
    $('#link-dot').classList.remove('on');
    $('#device-dot').classList.remove('on');
    $('#device-status').textContent = 'Waiting for your board';
    if (!waitingJob) banner(e.message, true);
    if (!cardsSignature) {
      $('#home-cards').textContent = 'Your agents will appear when the board reconnects.';
    }
    syncComposer();
  } finally {
    statusBusy = false;
  }
}
async function loadAgents() {
  const first = !agents.length;
  const data = await api('/api/agents');
  agents = data.agents || [];
  if (!agents.some((a) => a.id === current)) current = agents[0]?.id || 'claude';
  if (!settingsBuilt) renderAgents();
  if (first && voiceBuilt) renderVoice();
  if (!$('#chat').hidden) renderChat();
}
function agentLabel(id) {
  const a = agents.find((a) => a.id === id) || statusAgents.find((a) => a.id === id);
  return a?.name || a?.title || id || 'your agent';
}
// Wake word state from /api/status: the meter in Settings › Voice follows the detector's score.
function trackWake(w) {
  wakeState = w;
  const m = $('#wake-meter');
  if (!m) return;
  const pct = Math.round(Math.max(0, Math.min(1, +w.score || 0)) * 100);
  m.firstChild.style.width = pct + '%';
  m.setAttribute('aria-valuenow', String(pct));
  m.classList.toggle('idle', !w.armed);
}
function voiceBanner(v) {
  return (
    {
      recording:
        v.source === 'wake'
          ? 'Heard “' +
            (v.wake?.phrase || wakeState.phrase || 'the wake word') +
            '”… go ahead, I’ll send when you pause.'
          : v.source === 'page'
            ? 'Listening to you… tap the mic again to send.'
            : 'Listening to you… let go of the button to send.',
      transcribing: 'Writing down what you said…',
      waiting: 'Waiting for ' + agentLabel(v.agent) + ' to reply…',
      speaking: 'Reading the reply aloud…',
    }[v.state] || ''
  );
}
