#pragma once
#include <Arduino.h>

// Single-page phone app, served by the gateway. Talks JSON over ws://<ip>/ws.
// Competitions and results are stored in the phone's localStorage; the gateway
// only runs games and keeps the last results until the app has them.
const char INDEX_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="de"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>ButtonSpiel</title>
<style>
:root{--bg:#111418;--card:#1c2128;--fg:#e8eaed;--mut:#8b939c;--acc:#3ea6ff;--ok:#3fb950;--bad:#f85149;--red:#ff5a5a;--blue:#4f8cff;--line:#30363d}
*{box-sizing:border-box}body{margin:0;font:16px system-ui,sans-serif;background:var(--bg);color:var(--fg);padding:12px;max-width:560px;margin:auto}
h1{font-size:20px;margin:4px 0 12px;display:flex;justify-content:space-between;align-items:center}
.dot{width:10px;height:10px;border-radius:50%;background:var(--bad);display:inline-block}.dot.on{background:var(--ok)}
.card{background:var(--card);border-radius:12px;padding:14px;margin-bottom:12px}
h2{font-size:14px;color:var(--mut);margin:0 0 10px;text-transform:uppercase;letter-spacing:.05em;display:flex;justify-content:space-between;align-items:center}
label{display:block;font-size:13px;color:var(--mut);margin:10px 0 4px}
input,select,textarea{width:100%;padding:10px;border-radius:8px;border:1px solid #333a44;background:#0d1014;color:var(--fg);font-size:16px;font-family:inherit}
.row{display:flex;gap:8px;align-items:flex-end}.row>*{flex:1}
button{padding:12px;border:0;border-radius:8px;font-size:16px;font-weight:600;background:var(--acc);color:#fff;width:100%;margin-top:12px;cursor:pointer}
button.sec{background:#30363d}button.bad{background:var(--bad)}button:disabled{opacity:.4}
button.small{width:auto;padding:6px 10px;margin:0;font-size:13px}
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(130px,1fr));gap:8px;margin-top:8px}.grid button{margin:0}
.seg{display:flex;gap:0;margin-top:4px}.seg button{margin:0;border-radius:0;background:#30363d}.seg button:first-child{border-radius:8px 0 0 8px}.seg button:last-child{border-radius:0 8px 8px 0}.seg button.on{background:var(--acc)}
.chips{display:flex;flex-wrap:wrap;gap:6px;margin-top:6px}.chip{background:#0d1014;border-radius:14px;padding:4px 10px;font-size:14px}.chip b{color:var(--mut);margin-left:6px;cursor:pointer;font-weight:400}
.big{font-size:44px;font-weight:700;text-align:center;margin:6px 0;font-variant-numeric:tabular-nums}
.mut{color:var(--mut);font-size:13px}.center{text-align:center}
.bar{height:8px;background:var(--line);border-radius:4px;overflow:hidden;margin:8px 0}.bar>div{height:100%;background:var(--acc);width:0;transition:width .2s}
.times{display:flex;flex-wrap:wrap;gap:6px;margin-top:8px}.times span{background:#0d1014;border-radius:6px;padding:4px 8px;font-size:14px;font-variant-numeric:tabular-nums}.times span.miss{color:var(--bad)}
.duel{display:grid;grid-template-columns:1fr 1fr;gap:8px}.lane{border-radius:10px;padding:10px;background:#0d1014;text-align:center;border-top:4px solid}
.lane.a{border-color:var(--red)}.lane.b{border-color:var(--blue)}.lane .n{font-size:30px;font-weight:700;font-variant-numeric:tabular-nums}
.red{color:var(--red)}.blue{color:var(--blue)}
table{width:100%;border-collapse:collapse;font-variant-numeric:tabular-nums}td,th{padding:6px 4px;text-align:left;border-bottom:1px solid var(--line);font-size:14px}th{color:var(--mut);font-weight:500}
td.r,th.r{text-align:right}tr.me td{color:var(--acc);font-weight:600}
.unit{display:flex;justify-content:space-between;align-items:center;padding:6px 0;border-bottom:1px solid var(--line)}
.err{position:sticky;top:0;background:var(--bad);color:#fff;padding:10px;border-radius:8px;margin-bottom:12px;display:none;z-index:2}
details summary{cursor:pointer;color:var(--mut);font-size:14px;margin-top:10px}
.hide{display:none}
</style></head><body>
<h1>ButtonSpiel <span class="mut"><span id="conn" class="dot"></span> <span id="unitCount"></span></span></h1>
<div id="err" class="err"></div>

<div class="card">
  <h2>Wettkampf <button class="small sec" id="newCompBtn">+ Neu</button></h2>
  <select id="compSel"></select>
  <div id="compInfo" class="mut" style="margin-top:8px"></div>
  <div id="compPlayers" class="chips"></div>
  <div class="row" id="addPlayerRow"><input id="addPlayer" maxlength="16" placeholder="Spieler hinzufügen" list="names"><button class="small" id="addPlayerBtn" style="flex:0">+</button></div>

  <div id="newComp" class="hide">
    <label for="ncName">Name</label><input id="ncName" maxlength="40">
    <label>Modus</label>
    <div class="seg" id="ncMode"><button data-m="solo" class="on">Einzel</button><button data-m="duel">Duell</button></div>
    <label for="ncPlayers">Spieler (einer pro Zeile oder mit Komma)</label><textarea id="ncPlayers" rows="3"></textarea>
    <div class="row">
      <div><label for="ncPresses">Drücker</label><input id="ncPresses" type="number" min="1" max="50" value="10"></div>
      <div><label for="ncTimeout">Timeout</label><select id="ncTimeout"><option value="3000">3 s</option><option value="5000" selected>5 s</option><option value="10000">10 s</option></select></div>
    </div>
    <div class="row"><button class="sec" id="ncCancel">Abbrechen</button><button id="ncSave">Anlegen</button></div>
  </div>
</div>
<datalist id="names"></datalist>

<div class="card hide" id="play">
  <h2>Spielen</h2>
  <div id="playSolo">
    <div class="mut">Spieler antippen, um seinen Lauf zu starten:</div>
    <div class="grid" id="soloPlayers"></div>
  </div>
  <div id="playDuel">
    <div class="row">
      <div><label class="red" for="duelA">Rot</label><select id="duelA"></select></div>
      <div><label class="blue" for="duelB">Blau</label><select id="duelB"></select></div>
    </div>
    <button id="duelStart">Duell starten</button>
    <div class="mut" style="margin-top:8px">Jeder drückt nur Buttons in seiner Farbe. Wer zuerst alle Drücker geschafft hat, gewinnt.</div>
  </div>
</div>

<div class="card hide" id="live">
  <h2 id="liveTitle">Spiel</h2>
  <div id="liveSolo">
    <div class="big" id="liveBig">–</div>
    <div class="center mut" id="liveSub"></div>
    <div class="bar"><div id="progress"></div></div>
    <div class="times" id="times"></div>
  </div>
  <div id="liveDuel">
    <div class="big" id="raceClock">0,0 s</div>
    <div class="duel" id="lanes"></div>
  </div>
  <button id="abort" class="bad">Abbrechen</button>
</div>

<div class="card hide" id="resultCard">
  <h2>Letztes Spiel</h2>
  <div id="resultBody"></div>
</div>

<div class="card hide" id="rankCard">
  <h2>Rangliste</h2>
  <div id="rankBody"></div>
  <details><summary>Alle Spiele dieses Wettkampfs</summary><div id="gamesBody"></div></details>
</div>

<div class="card">
  <h2>Export</h2>
  <label for="mail">E-Mail-Empfänger (optional)</label><input id="mail" type="email" placeholder="name@example.com">
  <label>Umfang</label>
  <div class="seg" id="expScope"><button data-s="comp" class="on">Dieser Wettkampf</button><button data-s="all">Alle Wettkämpfe</button></div>
  <button id="saveCsv">CSV-Datei speichern / teilen</button>
  <div class="mut" style="margin-top:6px">Für einen Mail-Anhang: Datei speichern, dann in der Download-Meldung „Teilen“ → Mail,
    oder in der Mail-App über die Büroklammer anhängen.</div>
  <button id="mailCsv" class="sec">Als Mail-Text (CSV im Text)</button>
  <button id="showCsv" class="sec">CSV anzeigen / kopieren</button>
  <textarea id="csvBox" rows="6" class="hide" readonly style="margin-top:8px;font-size:12px"></textarea>
  <div class="mut" style="margin-top:8px">Mails werden verschickt, sobald das Handy wieder Internet hat.
    Die Daten liegen nur auf diesem Handy.</div>
  <button id="delComp" class="bad">Wettkampf löschen</button>
</div>

<div class="card">
  <h2>Buttons</h2>
  <div id="units"></div>
  <div class="mut" style="margin-top:8px" id="unitsHint">„Zeigen“ lässt einen Button blau blinken.</div>
  <button class="sec" id="testBtn">Testmodus starten</button>
</div>

<script>
const $=id=>document.getElementById(id);
const esc=s=>String(s).replace(/[&<>"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]));
const sec=ms=>(ms/1000).toFixed(3).replace('.',',')+' s';
const pad=n=>String(n).padStart(2,'0');
const fmtDate=iso=>{const d=new Date(iso);return `${pad(d.getDate())}.${pad(d.getMonth()+1)}.${d.getFullYear()}`};
const fmtTime=iso=>{const d=new Date(iso);return `${pad(d.getHours())}:${pad(d.getMinutes())}`};

// ---- storage (localStorage, survives ESP restarts; only on this phone) ----
function load(k,d){try{const v=JSON.parse(localStorage.getItem(k));return v??d}catch(e){return d}}
function save(k,v){try{localStorage.setItem(k,JSON.stringify(v))}catch(e){showErr('Speichern fehlgeschlagen: '+e.message)}}
let comps=load('bs.comps',[]);      // {id,name,mode,players[],presses,timeoutMs,created}
let results=load('bs.results',[]);  // gateway result + at (ISO, phone time)
let activeId=load('bs.active',null);
let names=load('bs.names',[]);
$('mail').value=load('bs.mail','');
const active=()=>comps.find(c=>c.id===activeId);
const compResults=id=>results.filter(r=>r.comp===id);
function rememberNames(list){for(const n of list)if(!names.includes(n))names.unshift(n);names=names.slice(0,40);save('bs.names',names)}

// ---- gateway connection ----
let ws,state=null,stateAt=0,lastShownResult=null;
function connect(){
  ws=new WebSocket(`ws://${location.host}/ws`);
  ws.onopen=()=>$('conn').classList.add('on');
  ws.onclose=()=>{$('conn').classList.remove('on');setTimeout(connect,1000)};
  ws.onmessage=e=>{const m=JSON.parse(e.data);
    if(m.type==='state'){state=m;stateAt=performance.now();render()}
    else if(m.type==='results'){m.list.forEach(storeResult);render()}
    else if(m.type==='result'){storeResult(m.result);lastShownResult=m.result.id;render()}
    else if(m.type==='error')showErr(m.msg)};
}
function send(o){if(ws&&ws.readyState===1){ws.send(JSON.stringify(o));return true}showErr('Keine Verbindung zum Spiel');return false}
function showErr(t){const e=$('err');e.textContent=t;e.style.display='block';clearTimeout(showErr.t);showErr.t=setTimeout(()=>e.style.display='none',4000)}
function storeResult(r){if(results.some(x=>x.id===r.id))return;r.at=new Date().toISOString();results.push(r);save('bs.results',results)}

// ---- scoring ----
function soloRuns(id){return compResults(id).filter(r=>r.mode==='solo').map(r=>({...r.lanes[0],id:r.id,at:r.at})).sort((a,b)=>a.total-b.total)}
function soloRanking(id){const best={};for(const run of soloRuns(id)){const p=best[run.player];if(!p)best[run.player]={...run,runs:1};else p.runs++}return Object.values(best).sort((a,b)=>a.total-b.total)}
function duelRanking(id){const t={};const get=n=>t[n]??=(t[n]={player:n,wins:0,losses:0,draws:0,games:0,best:null});
  for(const r of compResults(id).filter(r=>r.mode==='duel'))r.lanes.forEach((l,i)=>{const p=get(l.player);p.games++;
    if(r.winner===-1)p.draws++;else if(r.winner===i)p.wins++;else p.losses++;if(p.best===null||l.total<p.best)p.best=l.total});
  return Object.values(t).sort((a,b)=>b.wins-a.wins||a.losses-b.losses||(a.best??1e12)-(b.best??1e12))}
function compBest(id){const runs=soloRuns(id);return runs.length?runs[0].total:0}

// ---- rendering ----
function render(){renderComp();renderPlay();renderLive();renderResult();renderRanking();renderUnits()}
function renderComp(){
  // like the duel boxes: only rebuild pickers on a real change, never on the per-second status
  const sel=$('compSel');const sorted=[...comps].sort((a,b)=>b.created.localeCompare(a.created));
  const selKey=activeId+'|'+sorted.map(c=>c.id+':'+c.name).join('|');
  if(sel.dataset.key!==selKey){sel.dataset.key=selKey;
    sel.innerHTML=sorted.length?sorted.map(c=>`<option value="${c.id}" ${c.id===activeId?'selected':''}>${esc(c.name)}</option>`).join(''):'<option>– noch kein Wettkampf –</option>'}
  const c=active();
  $('compInfo').textContent=c?`${c.mode==='duel'?'Duell':'Einzel'} · ${c.presses} Drücker · Timeout ${c.timeoutMs/1000} s · ${fmtDate(c.created)} ${fmtTime(c.created)} · ${compResults(c.id).length} Spiele`:'Lege zuerst einen Wettkampf an.';
  $('compPlayers').innerHTML=c?c.players.map((p,i)=>`<span class="chip">${esc(p)}<b onclick="removePlayer(${i})">×</b></span>`).join(''):'';
  $('addPlayerRow').style.display=c?'flex':'none';
  $('delComp').disabled=!c;
  if(!c&&expScope==='comp')setScope('all');
  const nl=$('names'),nKey=names.join('|');
  if(nl.dataset.key!==nKey){nl.dataset.key=nKey;nl.innerHTML=names.map(n=>`<option value="${esc(n)}">`).join('')}
}
function running(){return state&&['countdown','running'].includes(state.phase)}
function testing(){return state&&state.phase==='test'}
function renderPlay(){
  const c=active();$('play').classList.toggle('hide',!c||running()||testing());if(!c)return;
  $('playSolo').classList.toggle('hide',c.mode!=='solo');$('playDuel').classList.toggle('hide',c.mode!=='duel');
  if(c.mode==='solo'){const runs=soloRuns(c.id);
    setHtml('soloPlayers',c.players.map((p,i)=>{const n=runs.filter(r=>r.player===p).length;
      return `<button onclick="startSolo(${i})">${esc(p)}<br><span style="font-weight:400;font-size:12px">${n} ${n===1?'Lauf':'Läufe'}</span></button>`}).join('')||'<div class="mut">Noch keine Spieler.</div>');}
  else{// render() runs on every gateway message (about once a second); rebuilding the options
    // then would close an open picker or undo a choice, so only rebuild when the players change
    const key=c.id+'|'+c.players.join('|');
    for(const [id,def] of [['duelA',0],['duelB',1]]){const s=$(id);if(s.dataset.key===key)continue;const old=s.value;
      s.innerHTML=c.players.map(p=>`<option>${esc(p)}</option>`).join('');s.value=c.players.includes(old)?old:(c.players[def]??'');s.dataset.key=key}
    if($('duelA').value===$('duelB').value)pickOther('duelB','duelA')}
}
// red and blue must be two different players: move the other box to someone else
function pickOther(id,keep){const c=active(),taken=$(keep).value;const other=c&&c.players.find(p=>p!==taken);if(other!==undefined)$(id).value=other}
$('duelA').onchange=()=>{if($('duelA').value===$('duelB').value)pickOther('duelB','duelA')};
$('duelB').onchange=()=>{if($('duelA').value===$('duelB').value)pickOther('duelA','duelB')};
function renderLive(){
  const on=running()||(state&&state.phase==='result');$('live').classList.toggle('hide',!on);if(!on)return;
  const s=state,duel=s.mode==='duel',lanes=s.lanes||[];
  $('liveSolo').classList.toggle('hide',duel);$('liveDuel').classList.toggle('hide',!duel);$('abort').classList.toggle('hide',!running());
  if(!duel){const l=lanes[0]||{times:[],missed:[],seq:0};$('liveTitle').textContent=l.player||'Spiel';const t=l.times||[];
    if(s.phase==='countdown'){$('liveBig').textContent='Achtung…';$('liveSub').textContent='gleich geht es los'}
    else{$('liveBig').textContent=t.length?sec(t[t.length-1]):'–';
      $('liveSub').textContent=`Drücker ${Math.min(l.seq+1,s.presses)} / ${s.presses}`+(l.falsePresses?` · ${l.falsePresses}× falsch (+${(s.penaltyMs*l.falsePresses/1000).toString().replace('.',',')} s)`:'')}
    $('progress').style.width=(100*l.seq/s.presses)+'%';
    $('times').innerHTML=t.map((ms,i)=>`<span class="${l.missed[i]?'miss':''}">${l.missed[i]?'✗ ':''}${(ms/1000).toFixed(3).replace('.',',')}</span>`).join('');}
  else{$('liveTitle').textContent=lanes.map(l=>l.player).join(' gegen ');
    $('lanes').innerHTML=lanes.map((l,i)=>`<div class="lane ${i?'b':'a'}"><div class="${i?'blue':'red'}">${esc(l.player)}</div>
      <div class="n">${l.seq} / ${s.presses}</div><div class="mut">${l.done?'fertig: '+sec(l.finishMs):(l.times.length?'zuletzt '+sec(l.times[l.times.length-1]):'–')}</div></div>`).join('');
    tickClock();}
}
function tickClock(){if(!state||state.mode!=='duel')return;
  const ms=state.phase==='running'?state.raceMs+(performance.now()-stateAt):(state.phase==='countdown'?0:Math.max(0,...(state.lanes||[]).map(l=>l.finishMs)));
  $('raceClock').textContent=state.phase==='countdown'?'Achtung…':(ms/1000).toFixed(1).replace('.',',')+' s'}
setInterval(()=>{if(running())tickClock()},100);
function resultHtml(r,c){
  if(r.mode==='duel'){const w=r.winner;
    return `<div class="big">${w===-1?'Unentschieden':`<span class="${w?'blue':'red'}">${esc(r.lanes[w].player)}</span> gewinnt`}</div>`+
      r.lanes.map((l,i)=>`<div class="center"><span class="${i?'blue':'red'}">${esc(l.player)}</span>: ${sec(l.total)} · ${l.misses} verpasst</div>`).join('')}
  const l=r.lanes[0],runs=soloRuns(r.comp),rank=runs.findIndex(x=>x.id===r.id)+1,best=rank===1;
  return `<div class="big">${sec(l.total)}</div><div class="center">${esc(l.player)}: Platz ${rank} von ${runs.length} Läufen${best?' 🏆 Bestzeit!':''}</div>
    <div class="center mut">Ø ${Math.round(l.reactionSum/r.presses)} ms · ${l.misses} verpasst · ${l.falsePresses} falsch</div>`}
function renderResult(){const c=active();const list=c?compResults(c.id):[];const r=list[list.length-1];
  $('resultCard').classList.toggle('hide',!r||running());if(r)$('resultBody').innerHTML=resultHtml(r,c)}
function renderRanking(){const c=active();const list=c?compResults(c.id):[];$('rankCard').classList.toggle('hide',!list.length);if(!list.length)return;
  if(c.mode==='solo'){const rk=soloRanking(c.id);
    $('rankBody').innerHTML=`<table><tr><th>#</th><th>Spieler</th><th class="r">Bestzeit</th><th class="r">Ø</th><th class="r">Läufe</th></tr>`+
      rk.map((p,i)=>`<tr class="${p.id===lastShownResult?'me':''}"><td>${i+1}</td><td>${esc(p.player)}</td><td class="r">${sec(p.total)}</td><td class="r">${Math.round(p.reactionSum/c.presses)} ms</td><td class="r">${p.runs}</td></tr>`).join('')+'</table>'}
  else{const rk=duelRanking(c.id);
    $('rankBody').innerHTML=`<table><tr><th>#</th><th>Spieler</th><th class="r">Siege</th><th class="r">Niederl.</th><th class="r">Bestzeit</th></tr>`+
      rk.map((p,i)=>`<tr><td>${i+1}</td><td>${esc(p.player)}</td><td class="r">${p.wins}</td><td class="r">${p.losses}</td><td class="r">${p.best===null?'–':sec(p.best)}</td></tr>`).join('')+'</table>'}
  $('gamesBody').innerHTML='<table>'+[...list].reverse().map(r=>`<tr><td>${fmtTime(r.at)}</td><td>${r.lanes.map((l,i)=>
    `<span class="${r.mode==='duel'?(i?'blue':'red'):''}">${esc(l.player)}${r.winner===i?' 🏆':''}</span>`).join(' : ')}</td><td class="r">${r.lanes.map(l=>sec(l.total)).join(' : ')}</td></tr>`).join('')+'</table>'}
function renderUnits(){if(!state)return;const on=state.units.filter(u=>u.online).length;
  $('unitCount').textContent=on+' Button'+(on===1?'':'s');
  const t=testing();
  setHtml('units',state.units.map(u=>`<div class="unit"><span><span class="dot ${u.online?'on':''}"></span> Button ${u.n}${u.gateway?' (Zentrale)':''} <span class="mut">${u.mac.slice(-5)}</span></span>
    ${t?`<span class="${u.presses?'':'mut'}">${u.presses?'✓ '+u.presses+'× gedrückt':'noch nicht gedrückt'}</span>`
      :`<button class="small sec" onclick="send({cmd:'identify',unit:${u.n}})" ${u.online&&!running()?'':'disabled'}>Zeigen</button>`}</div>`).join(''));
  const b=$('testBtn');b.textContent=t?'Testmodus beenden':'Testmodus starten';b.className=t?'bad':'sec';b.disabled=running();
  $('unitsHint').textContent=t?'Alle Buttons blinken rot, grün, blau, weiß, dann läuft ein Licht durch den Ring. Jeder Druck blitzt weiß und wird hier gezählt.'
    :'„Zeigen“ lässt einen Button blau blinken. Der Testmodus prüft LEDs und Taster aller Buttons.'}
// replace an element's content only if it really changed, so taps and open pickers survive the status updates
function setHtml(id,html){const e=$(id);if(e.dataset.html!==html){e.dataset.html=html;e.innerHTML=html}}

// ---- actions ----
function startSolo(i){const c=active();const p=c.players[i];
  send({cmd:'start',mode:'solo',players:[p],presses:c.presses,timeoutMs:c.timeoutMs,bestMs:compBest(c.id),comp:c.id})}
$('duelStart').onclick=()=>{const c=active(),a=$('duelA').value,b=$('duelB').value;
  if(!a||!b||a===b){showErr('Bitte zwei verschiedene Spieler wählen');return}
  send({cmd:'start',mode:'duel',players:[a,b],presses:c.presses,timeoutMs:c.timeoutMs,comp:c.id})};
$('abort').onclick=()=>send({cmd:'abort'});
$('testBtn').onclick=()=>send({cmd:'test',on:!testing()});
$('compSel').onchange=e=>{activeId=e.target.value;save('bs.active',activeId);render()};
let ncMode='solo';
$('ncMode').onclick=e=>{const m=e.target.dataset.m;if(!m)return;ncMode=m;[...$('ncMode').children].forEach(b=>b.classList.toggle('on',b.dataset.m===m))};
$('newCompBtn').onclick=()=>{const d=new Date();$('ncName').value=`Wettkampf ${pad(d.getHours())}:${pad(d.getMinutes())}`;$('newComp').classList.remove('hide')};
$('ncCancel').onclick=()=>$('newComp').classList.add('hide');
$('ncSave').onclick=()=>{const players=[...new Set($('ncPlayers').value.split(/[\n,;]/).map(s=>s.trim().slice(0,16)).filter(Boolean))];
  const name=$('ncName').value.trim()||'Wettkampf';
  if(ncMode==='duel'&&players.length<2){showErr('Ein Duell braucht mindestens 2 Spieler');return}
  const c={id:Math.random().toString(36).slice(2,10),name,mode:ncMode,players,presses:Math.min(50,Math.max(1,+$('ncPresses').value||10)),timeoutMs:+$('ncTimeout').value,created:new Date().toISOString()};
  comps.push(c);save('bs.comps',comps);activeId=c.id;save('bs.active',activeId);rememberNames(players);
  $('ncPlayers').value='';$('newComp').classList.add('hide');render()};
$('addPlayerBtn').onclick=()=>{const c=active(),n=$('addPlayer').value.trim().slice(0,16);if(!c||!n)return;
  if(!c.players.includes(n))c.players.push(n);save('bs.comps',comps);rememberNames([n]);$('addPlayer').value='';render()};
function removePlayer(i){const c=active();if(!confirm(`${c.players[i]} aus dem Wettkampf entfernen? Bisherige Ergebnisse bleiben.`))return;c.players.splice(i,1);save('bs.comps',comps);render()}
$('delComp').onclick=()=>{const c=active();if(!c||!confirm(`Wettkampf „${c.name}“ mit ${compResults(c.id).length} Spielen löschen?`))return;
  comps=comps.filter(x=>x.id!==c.id);results=results.filter(r=>r.comp!==c.id);save('bs.comps',comps);save('bs.results',results);
  activeId=comps.length?comps[comps.length-1].id:null;save('bs.active',activeId);render()};
$('mail').onchange=()=>save('bs.mail',$('mail').value.trim());

// ---- CSV export: one row per player per game ----
function csv(list){
  const head=['Wettkampf','Datum','Uhrzeit','Modus','Spieler','Farbe','Gegner','Ergebnis_ms','Sieg','Platz','Reaktionssumme_ms','Verpasst','Falsch','Druecker','Zeiten_ms'];
  const q=v=>{v=String(v??'');return /[;"\n]/.test(v)?'"'+v.replace(/"/g,'""')+'"':v};
  const rows=[head];
  for(const r of [...list].sort((a,b)=>a.at.localeCompare(b.at))){
    const c=comps.find(x=>x.id===r.comp);const runs=r.mode==='solo'?soloRuns(r.comp):[];
    r.lanes.forEach((l,i)=>rows.push([c?c.name:r.comp,fmtDate(r.at),fmtTime(r.at),r.mode==='duel'?'Duell':'Einzel',l.player,
      r.mode==='duel'?(i?'blau':'rot'):'',r.mode==='duel'?r.lanes[1-i].player:'',l.total,
      r.mode==='duel'?(r.winner===-1?'unentschieden':r.winner===i?'ja':'nein'):'',r.mode==='solo'?runs.findIndex(x=>x.id===r.id)+1:'',
      l.reactionSum,l.misses,l.falsePresses,r.presses,l.times.join(' ')]))}
  return rows.map(r=>r.map(q).join(';')).join('\r\n')}
let expScope='comp';
function setScope(s){expScope=s;[...$('expScope').children].forEach(b=>b.classList.toggle('on',b.dataset.s===s))}
$('expScope').onclick=e=>{const s=e.target.dataset.s;if(s==='comp'&&!active()){showErr('Kein Wettkampf gewählt');return}if(s)setScope(s)};
// what to export: {title, file name (ASCII only, it goes into an HTTP header), results}
const fileSafe=s=>s.replace(/ä/g,'ae').replace(/ö/g,'oe').replace(/ü/g,'ue').replace(/Ä/g,'Ae').replace(/Ö/g,'Oe').replace(/Ü/g,'Ue').replace(/ß/g,'ss').replace(/[^\w-]+/g,'_');
function exportSel(){const c=active();
  if(expScope==='comp'&&c){const d=new Date(c.created);
    return {title:`ButtonSpiel: ${c.name} (${fmtDate(c.created)})`,
      file:`ButtonSpiel_${fileSafe(c.name)}_${d.getFullYear()}-${pad(d.getMonth()+1)}-${pad(d.getDate())}.csv`,list:compResults(c.id)}}
  const d=new Date();return {title:'ButtonSpiel: alle Wettkämpfe',file:`ButtonSpiel_alle_${d.getFullYear()}-${pad(d.getMonth()+1)}-${pad(d.getDate())}.csv`,list:results}}
function clickDownload(href,name){const a=document.createElement('a');a.href=href;a.download=name;document.body.appendChild(a);a.click();setTimeout(()=>a.remove(),10000)}
// A real attachment needs the share sheet (HTTPS-only in most browsers) or a downloaded file.
// The download goes through the gateway, because only a server response can set the file name
// in every browser (DuckDuckGo names blob downloads after a random id).
$('saveCsv').onclick=async()=>{const x=exportSel();if(!x.list.length){showErr('Noch keine Ergebnisse');return}
  const text='﻿'+csv(x.list); // BOM: Excel shows umlauts correctly
  const file=new File([text],x.file,{type:'text/csv'});
  if(navigator.canShare&&navigator.canShare({files:[file]})){
    try{await navigator.share({files:[file],title:x.title});return}catch(e){if(e.name==='AbortError')return}}
  try{const r=await fetch('/csv',{method:'POST',headers:{'Content-Type':'text/csv','X-File':x.file},body:text});
    if(r.ok){clickDownload('/csv?t='+Date.now(),x.file);return}}catch(e){}
  const url=URL.createObjectURL(file);clickDownload(url,x.file);setTimeout(()=>URL.revokeObjectURL(url),10000)};
$('mailCsv').onclick=()=>{const x=exportSel();if(!x.list.length){showErr('Noch keine Ergebnisse');return}
  const to=$('mail').value.trim();save('bs.mail',to);
  location.href=`mailto:${encodeURIComponent(to)}?subject=${encodeURIComponent(x.title)}&body=${encodeURIComponent(csv(x.list))}`};
$('showCsv').onclick=()=>{const box=$('csvBox');box.value=csv(exportSel().list);box.classList.remove('hide');box.select();
  try{document.execCommand('copy');showErr('CSV kopiert')}catch(e){}};

render();connect();
</script></body></html>)HTML";
