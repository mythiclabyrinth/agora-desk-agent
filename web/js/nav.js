// Tabs, settings sub-tabs, and the static navigation wiring.
function showTab(name, focus = false) {
  if (!['home', 'chat', 'settings'].includes(name)) name = 'home';
  ['home', 'chat', 'settings'].forEach((t) => {
    $('#' + t).hidden = t !== name;
    const b = $('[data-tab="' + t + '"]');
    b.classList.toggle('on', t === name);
    if (t === name) b.setAttribute('aria-current', 'page');
    else b.removeAttribute('aria-current');
  });
  $('#page-title').textContent = { home: 'Overview', chat: 'Conversations', settings: 'Settings' }[name];
  if (name === 'chat') renderChat();
  if (name === 'settings' && !settingsBuilt) renderAgents();
  if (location.hash !== '#' + name) history.replaceState(null, '', '#' + name);
  if (focus) $('#main').focus({ preventScroll: true });
}
function showSub(name) {
  document.querySelectorAll('[data-sub]').forEach((b) => {
    const on = b.dataset.sub === name;
    b.classList.toggle('on', on);
    b.setAttribute('aria-pressed', String(on));
  });
  $('#wifi').hidden = name !== 'wifi';
  $('#agents').hidden = name !== 'agents';
  $('#voice').hidden = name !== 'voice';
  $('#settings-intro-title').textContent = {
    wifi: 'Connect your desk.',
    agents: 'Bring your team along.',
    voice: 'Say it out loud.',
  }[name];
  $('#settings-intro').textContent = {
    wifi: 'Connect to the same network as your computer.',
    agents: 'Connect the agents you use on your computer. You’ll find these details in your Agora setup.',
    voice: 'Connect a speech provider, then choose how your agent listens and speaks.',
  }[name];
  if (name === 'agents' && !settingsBuilt) renderAgents();
  if (name === 'voice' && !voiceBuilt) renderVoice();
}
function openSettings(id) {
  if (id && id !== agentTab) {
    agentTab = id;
    activateAgentForm();
  }
  showTab('settings');
  showSub('agents');
}
document
  .querySelectorAll('[data-tab]')
  .forEach((b) => b.addEventListener('click', () => showTab(b.dataset.tab)));
document
  .querySelectorAll('[data-sub]')
  .forEach((b) => b.addEventListener('click', () => showSub(b.dataset.sub)));
$('.brand').addEventListener('click', (e) => {
  e.preventDefault();
  showTab('home');
});
window.addEventListener('hashchange', () => showTab(location.hash.slice(1)));
$('#manage-agents').addEventListener('click', () => openSettings());
$('#chat-settings').addEventListener('click', () => openSettings(current));
$('#start-chat').addEventListener('click', () => showTab('chat'));
$('#retry').addEventListener('click', () => {
  refreshStatus();
  loadAgents().catch((e) => banner(e.message, true));
});
