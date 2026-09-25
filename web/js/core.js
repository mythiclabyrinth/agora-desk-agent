'use strict';
// Shared helpers: DOM builders, storage, the fetch wrapper, and page-wide state.
const $ = (s) => document.querySelector(s);
const agentIds = ['claude', 'cursor', 'codex'];
const descriptions = {
  claude: 'A thoughtful partner for your next idea.',
  cursor: 'Keep your coding workflow in reach.',
  codex: 'Move from a thought to working code.',
};
const storage = {
  get(kind, key) {
    try {
      return window[kind].getItem(key);
    } catch (_) {
      return null;
    }
  },
  set(kind, key, value) {
    try {
      window[kind].setItem(key, value);
    } catch (_) {}
  },
  remove(kind, key) {
    try {
      window[kind].removeItem(key);
    } catch (_) {}
  },
};
let agents = [],
  current = storage.get('sessionStorage', 'desk-agent') || 'claude',
  agentTab = current;
let waitingJob = 0,
  sending = false,
  boardBusy = false,
  online = false,
  statusBusy = false,
  settingsBuilt = false,
  voiceBuilt = false,
  cardsSignature = '',
  draftAgent = current;
let voiceTab = 'credentials';
let voiceBusy = false,
  voiceSeen = storage.get('localStorage', 'desk-voice-seen') || '',
  wakeState = {};
const drafts = {};
let statusAgents = [];
// The hands-free agent from /api/status; statuses requested before the page's own save of it are ignored.
let targetAgent = '',
  targetAgentAsOf = 0;
function el(tag, className, text) {
  const n = document.createElement(tag);
  if (className) n.className = className;
  if (text !== undefined) n.textContent = text;
  return n;
}
function icon(name) {
  const svg = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
  svg.setAttribute('aria-hidden', 'true');
  const use = document.createElementNS(svg.namespaceURI, 'use');
  use.setAttribute('href', '#i-' + name);
  svg.append(use);
  return svg;
}
function agentIcon(id) {
  const n = el('span', 'agent-icon ' + (agentIds.includes(id) ? id : ''));
  n.setAttribute('aria-hidden', 'true');
  n.append(icon(agentIds.includes(id) ? id : 'chip'));
  return n;
}
function button(text, cls, action) {
  const n = el('button', cls, text);
  n.type = 'button';
  if (action) n.addEventListener('click', action);
  return n;
}
function note(target, text, bad = false) {
  target.textContent = text;
  target.className = 'note' + (bad ? ' bad' : '');
}
function banner(text, bad = false) {
  $('#banner').hidden = !text;
  $('#banner-text').textContent = text;
  $('#banner').classList.toggle('bad', bad);
  $('#retry').hidden = !bad;
}
async function api(path, body, timeout = 12000) {
  const controller = new AbortController();
  const timer = setTimeout(() => controller.abort(), timeout);
  try {
    const res = await fetch(path, {
      cache: 'no-store',
      signal: controller.signal,
      ...(body
        ? { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(body) }
        : {}),
    });
    const data = await res.json();
    if (!res.ok) throw new Error(data.error || 'The board could not complete this request.');
    return data;
  } catch (e) {
    if (e.name === 'AbortError')
      throw new Error('The board took too long to respond. Check your connection.');
    if (e instanceof TypeError) throw new Error('Cannot reach your board. Check your Wi-Fi connection.');
    throw e;
  } finally {
    clearTimeout(timer);
  }
}
