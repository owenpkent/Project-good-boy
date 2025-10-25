import express from 'express';
import cors from 'cors';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const app = express();
const PORT = process.env.PORT || 3000;

// Simple in-memory state to emulate device
const state = {
  deviceName: 'GoodBoy-Sim',
  wifi: {
    connected: true,
    ssid: 'TestNet',
    rssi: -48,
    ip: '192.168.1.123'
  },
  battery: {
    percent: 86,
    voltage: 3.98
  },
  firmware: '0.1.0-sim',
  lastDispense: null,
  logs: [],
  calibration: {
    mode: 'servo', // 'servo' | 'stepper'
    openAngle: 70,
    closeAngle: 10,
    stepsPerDispense: 180
  }
};

function logEvent(type, msg) {
  const entry = {
    ts: Date.now(),
    type,
    msg
  };
  state.logs.unshift(entry);
  if (state.logs.length > 200) state.logs.pop();
}

app.use(cors());
app.use(express.json());
app.use(express.urlencoded({ extended: false }));

// Static front-end
app.use('/', express.static(path.join(__dirname, 'public')));

// Mock API
app.post('/api/dispense', (req, res) => {
  const count = Number(req.body?.count ?? 1);
  if (!Number.isFinite(count) || count <= 0 || count > 10) {
    return res.status(400).json({ ok: false, error: 'invalid_count' });
  }

  // Simulate work and possible jam
  const durationMs = 500 + Math.floor(Math.random() * 800);
  const jamChance = 0.08; // 8% chance
  const jammed = Math.random() < jamChance;

  setTimeout(() => {
    state.lastDispense = {
      at: Date.now(),
      count,
      result: jammed ? 'jam' : 'ok',
      durationMs
    };
    const msg = jammed ? `Jam detected during dispense (${count}).` : `Dispensed ${count}.`;
    logEvent(jammed ? 'error' : 'info', msg);
  }, durationMs);

  res.json({ ok: true, enqueued: true, etaMs: durationMs });
});

app.get('/api/status', (req, res) => {
  // Vary RSSI/battery a bit to feel alive
  state.wifi.rssi += Math.round((Math.random() - 0.5) * 2);
  state.battery.percent = Math.max(0, Math.min(100, state.battery.percent + (Math.random() - 0.5)));

  res.json({
    deviceName: state.deviceName,
    wifi: state.wifi,
    battery: state.battery,
    firmware: state.firmware,
    lastDispense: state.lastDispense,
    time: Date.now()
  });
});

app.post('/api/calibrate', (req, res) => {
  const { mode, openAngle, closeAngle, stepsPerDispense } = req.body || {};
  if (mode && !['servo', 'stepper'].includes(mode)) {
    return res.status(400).json({ ok: false, error: 'invalid_mode' });
  }
  if (mode === 'servo') {
    if (openAngle != null) state.calibration.openAngle = Number(openAngle);
    if (closeAngle != null) state.calibration.closeAngle = Number(closeAngle);
  } else if (mode === 'stepper') {
    if (stepsPerDispense != null) state.calibration.stepsPerDispense = Number(stepsPerDispense);
  }
  if (mode) state.calibration.mode = mode;
  logEvent('info', `Calibration updated: ${JSON.stringify(state.calibration)}`);
  res.json({ ok: true, calibration: state.calibration });
});

app.get('/api/logs', (req, res) => {
  res.json({ ok: true, logs: state.logs });
});

// Preview endpoint to emulate the Arduino's /run query
app.get('/run', (req, res) => {
  const dir = req.query?.dir === 'reverse' ? 'reverse' : 'forward';
  let speedVal = parseInt(req.query?.speed, 10);
  if (!Number.isFinite(speedVal) || speedVal <= 0 || speedVal > 15) speedVal = 15;

  // Simulate a brief motor action duration
  const durationMs = 1000;
  setTimeout(() => {
    res.type('text/plain').send(`Stepper done (1 rev, ${dir}, ${speedVal} RPM)`);
  }, durationMs);
});

app.listen(PORT, () => {
  console.log(`Good Boy Web running at http://localhost:${PORT}`);
});
