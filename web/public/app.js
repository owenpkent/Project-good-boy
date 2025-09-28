const api = {
  async status() {
    const res = await fetch('/api/status');
    return res.json();
  },
  async dispense(count) {
    const res = await fetch('/api/dispense', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ count })
    });
    return res.json();
  },
  async calibrate(payload) {
    const res = await fetch('/api/calibrate', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    });
    return res.json();
  },
  async logs() {
    const res = await fetch('/api/logs');
    return res.json();
  }
};

function fmtBattery(b) { return `${Math.round(b.percent)}% (${b.voltage.toFixed(2)}V)`; }
function fmtWifi(w) { return `${w.connected ? 'Connected' : 'Disconnected'} ${w.ssid ?? ''} RSSI ${w.rssi}`; }
function fmtTime(ts) { return new Date(ts).toLocaleString(); }

async function refreshStatus() {
  const s = await api.status();
  document.getElementById('deviceName').textContent = s.deviceName;
  document.getElementById('wifi').textContent = `Wi‑Fi: ${fmtWifi(s.wifi)}`;
  document.getElementById('battery').textContent = `Battery: ${fmtBattery(s.battery)}`;
  document.getElementById('fw').textContent = `FW: ${s.firmware}`;
}

async function doDispense() {
  const count = Number(document.getElementById('count').value || 1);
  const out = document.getElementById('dispenseResult');
  out.textContent = 'Sending...';
  try {
    const res = await api.dispense(count);
    out.textContent = JSON.stringify(res, null, 2);
    setTimeout(refreshStatus, 800);
  } catch (e) {
    out.textContent = `Error: ${e}`;
  }
}

function onModeChange() {
  const mode = document.getElementById('mode').value;
  document.getElementById('servoRow').style.display = mode === 'servo' ? '' : 'none';
  document.getElementById('stepperRow').style.display = mode === 'stepper' ? '' : 'none';
}

async function saveCal() {
  const mode = document.getElementById('mode').value;
  const payload = { mode };
  if (mode === 'servo') {
    payload.openAngle = Number(document.getElementById('openAngle').value);
    payload.closeAngle = Number(document.getElementById('closeAngle').value);
  } else {
    payload.stepsPerDispense = Number(document.getElementById('stepsPerDispense').value);
  }
  const out = document.getElementById('calResult');
  out.textContent = 'Saving...';
  try {
    const res = await api.calibrate(payload);
    out.textContent = JSON.stringify(res, null, 2);
  } catch (e) {
    out.textContent = `Error: ${e}`;
  }
}

async function loadLogs() {
  const ul = document.getElementById('logs');
  ul.innerHTML = '';
  const res = await api.logs();
  for (const entry of res.logs) {
    const li = document.createElement('li');
    const cls = entry.type === 'error' ? 'err' : entry.type === 'info' ? 'info' : 'ok';
    li.className = cls;
    li.textContent = `[${fmtTime(entry.ts)}] ${entry.type.toUpperCase()}: ${entry.msg}`;
    ul.appendChild(li);
  }
}

function clearLogs() {
  document.getElementById('logs').innerHTML = '';
}

window.addEventListener('DOMContentLoaded', () => {
  document.getElementById('refreshStatus').addEventListener('click', refreshStatus);
  const big = document.getElementById('btnDispenseBig');
  if (big) big.addEventListener('click', doDispense);
  document.getElementById('mode').addEventListener('change', onModeChange);
  document.getElementById('btnSaveCal').addEventListener('click', saveCal);
  document.getElementById('btnLoadLogs').addEventListener('click', loadLogs);
  document.getElementById('btnClearLogs').addEventListener('click', clearLogs);
  refreshStatus();

  // Tabs
  const tabs = document.querySelectorAll('.tabbtn');
  tabs.forEach(btn => btn.addEventListener('click', () => {
    tabs.forEach(b => b.classList.remove('active'));
    btn.classList.add('active');
    const target = btn.getAttribute('data-tab');
    document.querySelectorAll('.tab').forEach(sec => sec.classList.remove('active'));
    document.getElementById(`tab-${target}`).classList.add('active');
  }));
});
