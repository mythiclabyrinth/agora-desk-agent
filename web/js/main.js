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
