// Settings > Wi-Fi: scan dialog, manual entry, joining.
let savedSsid = '',
  wifiJoining = false,
  scanGeneration = 0,
  selectedNetwork = null;
const wifiDialog = $('#wifi-dialog');
function signalLabel(rssi) {
  if (rssi >= -60) return 'Strong';
  if (rssi >= -75) return 'Good';
  return 'Weak';
}
function wifiList(focus = true) {
  $('#wifi-form').hidden = true;
  $('#wifi-list-step').hidden = false;
  $('#wifi-title').textContent = 'Choose a network';
  $('#wifi-description').textContent = 'Nearby networks, discovered by your board.';
  $('#wifi-pass').value = '';
  if (focus) $('#wifi-title').focus();
}
function wifiDetails(net) {
  selectedNetwork = net;
  $('#wifi-list-step').hidden = true;
  $('#wifi-form').hidden = false;
  $('#wifi-title').textContent = net ? 'Join network' : 'Add a network';
  $('#wifi-description').textContent = net
    ? 'One more step to connect your desk.'
    : 'Enter the details for a hidden or unlisted network.';
  $('#ssid-field').hidden = !!net;
  $('#ssid').value = net ? net.ssid : '';
  $('#wifi-selected').hidden = !net;
  $('#wifi-selected').textContent = net ? net.ssid : '';
  $('#password-field').hidden = !!net?.open;
  $('#wifi-pass').value = '';
  $('#wifi-pass').type = 'password';
  $('#wifi-show').textContent = 'Show';
  $('#wifi-show').setAttribute('aria-pressed', 'false');
  $('#wifi-show').setAttribute('aria-label', 'Show password');
  $('#wifi-pass').required = !!net && !net.open && net.ssid !== savedSsid;
  $('#password-help').textContent =
    net && net.ssid === savedSsid
      ? 'Leave blank to use the saved password.'
      : net
        ? 'Enter the password for this network.'
        : 'Leave blank for an open network, or to use its saved password.';
  note(
    $('#wifi-join-note'),
    net?.open
      ? 'This is an open network. No password is needed.'
      : 'Your connection details are saved on this board.',
  );
  (net ? (net.open ? $('#wifi-connect') : $('#wifi-pass')) : $('#ssid')).focus();
}
async function scanNetworks() {
  const generation = ++scanGeneration,
    scan = $('#scan'),
    status = $('#scan-note'),
    list = $('#networks');
  scan.disabled = true;
  list.setAttribute('aria-busy', 'true');
  list.replaceChildren();
  note(status, 'Looking for nearby networks…');
  status.classList.add('scan-loading');
  try {
    const data = await api('/api/wifi/scan', null, 20000);
    if (generation !== scanGeneration) return;
    if (data.error) throw new Error(data.error);
    const nets = (data.networks || []).sort((a, b) => b.rssi - a.rssi);
    note(
      status,
      nets.length
        ? nets.length + ' networks found · strongest first'
        : 'No networks found. Scan again or add one manually.',
    );
    nets.forEach((net) => {
      const item = button('', '', () => wifiDetails(net)),
        bars = el('span', 'net-signal');
      bars.setAttribute('aria-hidden', 'true');
      const strength = net.rssi >= -60 ? 4 : net.rssi >= -75 ? 3 : 1;
      for (let i = 0; i < 4; i++) bars.append(el('i', i < strength ? 'lit' : ''));
      const copy = el('span', 'net-copy');
      copy.append(
        el('strong', '', net.ssid),
        el(
          'small',
          '',
          (online && net.ssid === savedSsid ? 'Connected · ' : '') +
            signalLabel(net.rssi) +
            ' signal · ' +
            (net.open ? 'Open' : 'Password required'),
        ),
      );
      item.append(bars, copy, icon(net.open ? 'arrow' : 'lock'));
      list.append(item);
    });
  } catch (err) {
    if (generation === scanGeneration)
      note(status, err.message + ' Try again or add a network manually.', true);
  } finally {
    if (generation === scanGeneration) {
      scan.disabled = false;
      list.setAttribute('aria-busy', 'false');
      status.classList.remove('scan-loading');
    }
  }
}
$('#choose-network').addEventListener('click', () => {
  wifiList(false);
  wifiDialog.showModal();
  $('#wifi-title').focus();
  scanNetworks();
});
$('#scan').addEventListener('click', scanNetworks);
$('#wifi-manual').addEventListener('click', () => wifiDetails(null));
$('#wifi-back').addEventListener('click', () => wifiList());
function closeWifi() {
  if (!wifiJoining) wifiDialog.close();
}
$('#wifi-close').addEventListener('click', closeWifi);
$('#wifi-cancel').addEventListener('click', closeWifi);
wifiDialog.addEventListener('cancel', (e) => {
  if (wifiJoining) e.preventDefault();
});
wifiDialog.addEventListener('close', () => {
  scanGeneration++;
  $('#wifi-pass').value = '';
  $('#choose-network').focus();
});
$('#wifi-show').addEventListener('click', () => {
  const show = $('#wifi-pass').type === 'password';
  $('#wifi-pass').type = show ? 'text' : 'password';
  $('#wifi-show').textContent = show ? 'Hide' : 'Show';
  $('#wifi-show').setAttribute('aria-pressed', String(show));
  $('#wifi-show').setAttribute('aria-label', show ? 'Hide password' : 'Show password');
});
$('#wifi-form').addEventListener('submit', async (e) => {
  e.preventDefault();
  if (wifiJoining) return;
  const ssid = $('#ssid').value,
    password = selectedNetwork?.open ? '' : $('#wifi-pass').value;
  if (!ssid.trim() || new TextEncoder().encode(ssid).length > 32) {
    note($('#wifi-join-note'), 'Enter a network name of 1–32 bytes.', true);
    return;
  }
  wifiJoining = true;
  $('#wifi-form')
    .querySelectorAll('button,input')
    .forEach((n) => (n.disabled = true));
  $('#wifi-close').disabled = true;
  $('#wifi-connect').textContent = 'Connecting…';
  note($('#wifi-join-note'), 'Connecting to ' + ssid + '… This may take a few seconds.');
  try {
    const data = await api(
      '/api/wifi',
      { ssid, password, security: selectedNetwork?.open ? 'open' : 'password' },
      25000,
    );
    $('#wifi-pass').value = '';
    if (data.connected) {
      note($('#wifi-note'), 'Connected to ' + ssid + ' · ' + data.ip);
      $('#wifi-note').classList.add('good');
      wifiDialog.close();
    } else
      note(
        $('#wifi-join-note'),
        'Network saved, but not connected yet. Check the password and signal, then try again.',
        true,
      );
    refreshStatus();
  } catch (err) {
    note(
      $('#wifi-join-note'),
      err.message + ' If the board changed networks, reopen it at esp32-agent.local or its new IP address.',
      true,
    );
  } finally {
    wifiJoining = false;
    $('#wifi-form')
      .querySelectorAll('button,input')
      .forEach((n) => (n.disabled = false));
    $('#wifi-close').disabled = false;
    $('#wifi-connect').textContent = 'Connect';
  }
});
