// ButtonSpiel simulator: replaces the WebSocket to the gateway with a fake
// gateway that runs in the browser. It mirrors src/gateway.cpp (phases, targets,
// timeouts, false presses, result JSON) closely enough to test the phone app.
//
// URL parameters:  ?units=4   number of simulated buttons (1..10)
//                  ?bot=1     buttons press themselves after 180-650 ms
(() => {
  const P = new URLSearchParams(location.search);
  const UNITS = Math.min(10, Math.max(1, +(P.get('units') || 4)));
  let bot = P.get('bot') === '1';

  // values from src/config.h
  const COUNTDOWN_MS = 3000, PAUSE_MIN_MS = 0, PAUSE_MAX_MS = 0, DUEL_PAUSE_MS = 150;
  const RESULT_SHOW_MS = 4000, IDENTIFY_MS = 2000, PENALTY_MS = 500, MAX_RESULTS = 10;
  const MIN_TIMEOUT_MS = 1000, MAX_TIMEOUT_MS = 30000, MAX_PRESSES = 50;

  const rnd = (a, b) => a + Math.floor(Math.random() * (b - a + 1));
  const now = () => Math.round(performance.now());
  const clamp = (v, a, b) => Math.min(b, Math.max(a, v));
  const mac = i => 'SI:MU:LA:TE:00:' + String(i + 1).padStart(2, '0');
  const bootId = Math.floor(Math.random() * 0xffffffff).toString(16).padStart(8, '0');

  const units = Array.from({ length: UNITS }, (_, i) => ({ online: true, i, presses: 0, flashUntil: 0 }));
  const g = { phase: 'idle', mode: 'solo', comp: '', presses: 10, timeoutMs: 5000, bestMs: 0, lanes: [],
              stray: 0, raceStart: 0, phaseUntil: 0, gameId: 0, lastResult: '', results: [], identify: -1 };
  const sockets = new Set();

  // ---------- gateway logic ----------
  function stateJson() {
    const s = { type: 'state', phase: g.phase, mode: g.mode, comp: g.comp, presses: g.presses, timeoutMs: g.timeoutMs,
                penaltyMs: PENALTY_MS, stray: g.stray, raceMs: g.phase === 'running' ? now() - g.raceStart : 0,
                lastResult: g.lastResult };
    if (g.phase !== 'idle' && g.phase !== 'test')
      s.lanes = g.lanes.map(l => ({ player: l.player, seq: l.seq, target: l.target + 1, done: l.done,
        finishMs: l.finishMs, falsePresses: l.falsePresses, times: l.times.slice(), missed: l.missed.slice() }));
    s.units = units.map((u, i) => Object.assign({ n: i + 1, mac: mac(i), online: u.online, gateway: i === 0 },
      g.phase === 'test' ? { presses: u.presses } : {}));
    return s;
  }
  const push = o => sockets.forEach(ws => ws._recv(o));
  const changed = () => { push(stateJson()); drawPanel(); };
  const setPhase = (p, ms) => { g.phase = p; g.phaseUntil = now() + ms; changed(); };
  const laneTotal = l => g.mode === 'duel' ? l.finishMs
    : l.falsePresses * PENALTY_MS + l.times.reduce((a, b) => a + b, 0);

  function pickTarget(li) {
    const lane = g.lanes[li];
    let c = units.map((u, i) => i).filter(i => units[i].online && !g.lanes.some((o, oi) => oi !== li && o.target === i));
    if (c.length > 1) c = c.filter(i => i !== lane.lastTarget);
    return c.length ? c[rnd(0, c.length - 1)] : -1;
  }
  function lightNext(li) {
    const lane = g.lanes[li], t = pickTarget(li);
    if (t < 0) return;
    lane.target = lane.lastTarget = t;
    lane.litSince = now();
    if (bot) lane.botAt = now() + rnd(180, 650);
    changed();
  }
  function finishGame() {
    let winner = 0;
    if (g.mode === 'duel') {
      const [a, b] = g.lanes.map(laneTotal);
      winner = a < b ? 0 : b < a ? 1 : -1;
    }
    const id = `${bootId}-${g.gameId}`;
    g.lastResult = id;
    const r = { id, comp: g.comp, mode: g.mode, presses: g.presses, timeoutMs: g.timeoutMs, winner, stray: g.stray,
      lanes: g.lanes.map(l => ({ player: l.player, total: laneTotal(l), finishMs: l.finishMs, times: l.times.slice(),
        reactionSum: l.times.reduce((a, b) => a + b, 0), misses: l.missed.filter(Boolean).length,
        falsePresses: l.falsePresses })) };
    g.results.push(r);
    if (g.results.length > MAX_RESULTS) g.results.shift();
    g.lanes.forEach(l => l.target = -1);
    setPhase('result', RESULT_SHOW_MS);
    push({ type: 'result', result: r });
  }
  function recordTurn(li, ms, miss) {
    const lane = g.lanes[li];
    lane.times.push(Math.min(ms, 0xffff));
    lane.missed.push(miss ? 1 : 0);
    lane.seq++;
    lane.target = -1;
    if (lane.seq >= g.presses) { lane.done = true; lane.finishMs = now() - g.raceStart; }
    else lane.nextAt = now() + (g.mode === 'duel' ? DUEL_PAUSE_MS : rnd(PAUSE_MIN_MS, PAUSE_MAX_MS));
    if (g.lanes.every(l => l.done)) finishGame(); else changed();
  }
  function press(u) {
    if (units[u].online && g.phase === 'test') {
      units[u].presses++;
      units[u].flashUntil = now() + 300;
      return changed();
    }
    if (!units[u].online || g.phase !== 'running') return;
    const li = g.lanes.findIndex(l => l.target === u);
    if (li >= 0) {
      const ms = now() - g.lanes[li].litSince;
      recordTurn(li, Math.min(ms, g.timeoutMs), ms >= g.timeoutMs);
    } else {
      if (g.mode === 'solo') g.lanes[0].falsePresses++; else g.stray++;
      changed();
    }
  }
  function command(c, ws) {
    const err = msg => ws._recv({ type: 'error', msg });
    if (c.cmd === 'test') {
      if (!c.on) { if (g.phase === 'test') setPhase('idle', 0); return; }
      if (g.phase === 'test') return;
      if (!['idle', 'result', 'identify'].includes(g.phase)) return err('Erst das laufende Spiel beenden');
      units.forEach(u => u.presses = 0);
      g.identify = -1;
      return setPhase('test', 0);
    }
    if (c.cmd === 'start') {
      if (g.phase === 'test') return err('Erst den Testmodus beenden');
      if (!['idle', 'result'].includes(g.phase)) return err('Es läuft schon ein Spiel');
      const duel = c.mode === 'duel';
      if (duel && units.filter(u => u.online).length < 2) return err('Für ein Duell müssen mindestens 2 Buttons online sein');
      Object.assign(g, { mode: duel ? 'duel' : 'solo', presses: clamp(c.presses || 10, 1, MAX_PRESSES),
        timeoutMs: clamp(c.timeoutMs || 5000, MIN_TIMEOUT_MS, MAX_TIMEOUT_MS), bestMs: c.bestMs || 0,
        comp: c.comp || '', stray: 0 });
      g.gameId++;
      g.lanes = Array.from({ length: duel ? 2 : 1 }, (_, i) => ({ player: (c.players || [])[i] || '?', seq: 0,
        target: -1, lastTarget: -1, done: false, finishMs: 0, falsePresses: 0, times: [], missed: [], nextAt: 0 }));
      setPhase('countdown', COUNTDOWN_MS);
    } else if (c.cmd === 'abort') {
      if (g.phase === 'idle') return;
      g.lanes.forEach(l => l.target = -1);
      g.identify = -1;
      setPhase('idle', 0);
    } else if (c.cmd === 'identify') {
      const i = (c.unit | 0) - 1;
      if (i < 0 || i >= UNITS || g.phase !== 'idle') return err('Gerade nicht möglich');
      g.identify = i;
      setPhase('identify', IDENTIFY_MS);
    }
  }

  let lastPush = 0;
  setInterval(() => {
    const t = now(), due = t >= g.phaseUntil;
    if (g.phase === 'countdown' && due) {
      g.raceStart = t;
      g.lanes.forEach(l => l.nextAt = t + (g.mode === 'solo' ? rnd(PAUSE_MIN_MS, PAUSE_MAX_MS) : 0));
      setPhase('running', 0);
    } else if (g.phase === 'running') {
      g.lanes.forEach((l, li) => {
        if (l.done || g.phase !== 'running') return;
        if (l.target < 0) { if (t >= l.nextAt) lightNext(li); }
        else if (bot && l.botAt && t >= l.botAt) { l.botAt = 0; press(l.target); }
        else if (t - l.litSince > g.timeoutMs) recordTurn(li, g.timeoutMs, true);
      });
    } else if ((g.phase === 'result' || g.phase === 'identify') && due) {
      g.identify = -1;
      setPhase('idle', 0);
    }
    if (g.phase === 'test') drawPanel(); // colour cycle of the virtual buttons
    if (t - lastPush >= 1000) { lastPush = t; changed(); }
  }, 20);

  // ---------- fake WebSocket ----------
  class SimSocket {
    constructor() {
      this.readyState = 0;
      setTimeout(() => {
        this.readyState = 1;
        sockets.add(this);
        this.onopen && this.onopen();
        this._recv(stateJson());
        this._recv({ type: 'results', list: g.results });
      }, 150);
    }
    _recv(o) { if (this.readyState === 1 && this.onmessage) this.onmessage({ data: JSON.stringify(o) }); }
    send(text) { command(JSON.parse(text), this); }
    close() { this.readyState = 3; sockets.delete(this); this.onclose && this.onclose(); }
  }
  window.WebSocket = SimSocket;

  // ---------- virtual buttons ----------
  const css = `#sim{position:fixed;right:8px;bottom:8px;z-index:99;background:#000c;border:1px solid #555;border-radius:10px;
    padding:8px;font:13px system-ui;color:#ddd;max-width:calc(100vw - 16px)}#sim b{display:block;margin-bottom:6px}
    #sim .row{display:flex;flex-wrap:wrap;gap:6px}#sim button.u{width:52px;height:52px;border-radius:50%;border:2px solid #666;
    background:#223;color:#fff;font-weight:bold;cursor:pointer}#sim button.u.red{background:#e33;box-shadow:0 0 12px #f55}
    #sim button.u.blue{background:#36f;box-shadow:0 0 12px #58f}#sim button.u.white{background:#fff;color:#000}#sim button.u.u-green{background:#2c2;box-shadow:0 0 12px #5f5}#sim button.u.id{background:#09c}#sim button.u.off{opacity:.3}
    #sim label{display:flex;align-items:center;gap:6px;margin:6px 0 0;color:#ddd;font-size:13px}
    #sim input{width:auto;margin:0}`;
  document.head.insertAdjacentHTML('beforeend', `<style>${css}</style>`);
  addEventListener('DOMContentLoaded', () => {
    document.body.insertAdjacentHTML('beforeend', `<div id="sim"><b>Simulator – Buttons</b><div class="row" id="simUnits"></div>
      <label><input type="checkbox" id="simBot" ${bot ? 'checked' : ''}> automatisch drücken</label>
      <label><input type="checkbox" id="simOff"> Button ${UNITS} offline</label></div>`);
    document.getElementById('simBot').onchange = e => bot = e.target.checked;
    document.getElementById('simOff').onchange = e => { units[UNITS - 1].online = !e.target.checked; changed(); };
    drawPanel();
  });
  // same 400 ms steps as leds::TEST, roughly: red, green, blue, white with dark steps in between
  const testCls = () => [ 'red', '', 'u-green', '', 'blue', '', 'white', '' ][Math.floor(now() / 400) % 8];
  function drawPanel() {
    const box = document.getElementById('simUnits');
    if (!box) return;
    const html = units.map((u, i) => {
      const li = g.phase === 'running' ? g.lanes.findIndex(l => l.target === i) : -1;
      const cls = g.phase === 'test' ? (u.flashUntil > now() ? 'white' : testCls())
        : li === 0 ? 'red' : li === 1 ? 'blue' : g.identify === i ? 'id' : '';
      return `<button class="u ${cls} ${u.online ? '' : 'off'}" data-u="${i}" title="Button ${i + 1}">${i + 1}</button>`;
    }).join('');
    if (box.dataset.html === html) return;
    box.dataset.html = html;
    box.innerHTML = html;
    box.querySelectorAll('button').forEach(b => b.onpointerdown = () => press(+b.dataset.u));
  }
})();
