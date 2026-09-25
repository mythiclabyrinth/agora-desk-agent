// Theme storage is optional: the toggle still works when browser storage is unavailable.
function updateThemeToggle() {
  const dark = document.documentElement.dataset.theme === 'dark';
  $('#theme-toggle').setAttribute('aria-pressed', String(dark));
  $('#theme-toggle').title = dark ? 'Switch to light mode' : 'Switch to dark mode';
  $('meta[name="theme-color"]').content = dark ? '#07090f' : '#f6f5f0';
}
$('#theme-toggle').addEventListener('click', () => {
  const theme = document.documentElement.dataset.theme === 'dark' ? 'light' : 'dark';
  document.documentElement.dataset.theme = theme;
  storage.set('localStorage', 'agora-desk-theme', theme);
  updateThemeToggle();
});
updateThemeToggle();

// Boot: restore a pending job, first status fetch, polling.
async function start() {
  let pending;
  try {
    pending = JSON.parse(storage.get('sessionStorage', 'desk-pending') || 'null');
  } catch (_) {}
  if (pending?.job) {
    waitingJob = pending.job;
    current = pending.agent || current;
    draftAgent = current;
  }
  showTab(pending?.job ? 'chat' : location.hash.slice(1) || 'home');
  await refreshStatus();
  try {
    await loadAgents();
  } catch (e) {
    banner(e.message, true);
  }
  if (pending?.job) waitFor(pending.job, current);
}
start();
setInterval(() => {
  if (!document.hidden) {
    refreshStatus();
    if (!agents.length) loadAgents().catch(() => {});
  }
}, 5000);
window.addEventListener('online', () => refreshStatus());
