const API = 'http://localhost:8080';
let me = null;          // { username, role }
let graph = null;       // { nodes, edges }
let hlPath = [];        // highlighted Dijkstra path
let dfsOrd = [];        // DFS traversal order

/* ─── TOAST ─────────────────────────────────────────────── */
function toast(msg, type='info') {
  const t = document.getElementById('toast');
  t.textContent = msg;
  t.style.borderColor = type==='err' ? 'var(--red)'
                       : type==='ok' ? 'var(--green)' : 'var(--blue)';
  t.classList.add('show');
  setTimeout(() => t.classList.remove('show'), 3000);
}

/* ─── API CALLS ──────────────────────────────────────────── */
async function get(path) {
  try {
    const r = await fetch(API + path);
    return await r.json();
  } catch { toast('Server unreachable. Is it running?', 'err'); return null; }
}

async function post(path, data) {
  const body = new URLSearchParams(data).toString();
  try {
    const r = await fetch(API + path, {
      method: 'POST',
      headers: {'Content-Type': 'application/x-www-form-urlencoded'},
      body
    });
    return await r.json();
  } catch { toast('Server unreachable.', 'err'); return null; }
}

/* ─── AUTH TAB SWITCH ────────────────────────────────────── */
function tab(name) {
  document.getElementById('tab-login').style.display    = name==='login' ? '' : 'none';
  document.getElementById('tab-register').style.display = name==='register' ? '' : 'none';
  document.querySelectorAll('.auth-tab').forEach((b, i) =>
    b.classList.toggle('active', (i===0 && name==='login') || (i===1 && name==='register'))
  );
}

/* ─── LOGIN ──────────────────────────────────────────────── */
async function doLogin() {
  const u = document.getElementById('lu').value.trim();
  const p = document.getElementById('lp').value;
  const el = document.getElementById('login-msg');
  if (!u || !p) { showMsg(el,'err','Fill all fields'); return; }

  const res = await post('/api/login', {username:u, password:p});
  if (!res) return;
  if (res.ok) {
    me = {username: res.username, role: res.role};
    initApp();
  } else {
    showMsg(el, 'err', res.error || 'Login failed');
  }
}

/* ─── REGISTER ───────────────────────────────────────────── */
async function doRegister() {
  const data = {
    username: document.getElementById('ru').value.trim(),
    password: document.getElementById('rp').value,
    role:     document.getElementById('rr').value,
    fullName: document.getElementById('rn').value.trim(),
    email:    document.getElementById('re').value.trim(),
  };
  const el = document.getElementById('reg-msg');
  if (!data.username || !data.password || !data.fullName) {
    showMsg(el, 'err', 'Fill all fields'); return;
  }
  const res = await post('/api/register', data);
  if (!res) return;
  showMsg(el, res.ok ? 'ok' : 'err', res.message || res.error);
}

function showMsg(el, type, text) {
  el.className = 'msg ' + type;
  el.textContent = text;
}

/* ─── LOGOUT ─────────────────────────────────────────────── */
function doLogout() {
  me = null;
  document.getElementById('app').style.display = 'none';
  document.getElementById('auth-screen').style.display = 'flex';
  document.getElementById('lp').value = '';
  document.getElementById('login-msg').className = 'msg';
}

/* ─── INIT APP ───────────────────────────────────────────── */
async function initApp() {
  document.getElementById('auth-screen').style.display = 'none';
  document.getElementById('app').style.display = 'flex';

  document.getElementById('u-name').textContent = me.username;
  const rb = document.getElementById('u-role');
  rb.textContent = me.role;
  rb.className = 'badge ' + (me.role==='admin' ? 'b-yellow'
                             : me.role==='faculty' ? 'b-purple' : 'b-blue');

  document.getElementById('nav-admin').style.display =
    me.role==='admin' ? '' : 'none';
  document.getElementById('nav-free').style.display =
    (me.role==='faculty' || me.role==='admin') ? '' : 'none';

  showPanel('dashboard');
  await loadGraph();
  loadRoomsTable();
  loadTT();
  if (me.role === 'admin') { loadPending(); loadUsers(); }
}

/* ─── PANEL SWITCH ───────────────────────────────────────── */
function showPanel(name) {
  document.querySelectorAll('.panel').forEach(p => p.classList.remove('active'));
  document.querySelectorAll('nav button').forEach(b => b.classList.remove('active'));
  document.getElementById('panel-' + name).classList.add('active');
  const idx = ['dashboard','navigation','classrooms','timetable','freeroom','admin'].indexOf(name);
  const btns = document.querySelectorAll('nav button');
  if (btns[idx]) btns[idx].classList.add('active');
}

/* ─── GRAPH & MAP ────────────────────────────────────────── */
async function loadGraph() {
  graph = await get('/api/graph');
  if (!graph) return;
  fillSelects();
  drawMap();
}

function fillSelects() {
  const nodes = graph.nodes.map(n => n.id).sort();
  ['n-from','n-to','dfs-from','dfs-path-from','dfs-path-to','d-from','d-to','r-from'].forEach(id => {
    const s = document.getElementById(id);
    if (!s) return;
    s.innerHTML = nodes.map(n => `<option>${n}</option>`).join('');
  });
  document.getElementById('n-to').value = 'Library';
  document.getElementById('d-to').value  = 'Library';
}

function xy(nodeId) {
  const n = graph.nodes.find(n => n.id === nodeId);
  if (!n) return {x:400, y:230};
  return {x: n.x/100*780+10, y: n.y/100*440+10};
}

function drawMap() {
  const eg = document.getElementById('edges-g');
  const ng = document.getElementById('nodes-g');
  eg.innerHTML = ''; ng.innerHTML = '';

  // Draw edges
  graph.edges.forEach(e => {
    const a = xy(e.from), b = xy(e.to);
    const onDijk = isEdgeOnPath(e.from, e.to, hlPath);
    const onDFS  = isEdgeOnDFS(e.from, e.to, dfsOrd);
    const cls = onDijk ? 'edge path-on' : onDFS ? 'edge dfs-on' : 'edge';

    const line = svgEl('line');
    line.setAttribute('x1',a.x); line.setAttribute('y1',a.y);
    line.setAttribute('x2',b.x); line.setAttribute('y2',b.y);
    line.setAttribute('class', cls);
    eg.appendChild(line);

    // weight label
    const wt = svgEl('text');
    wt.setAttribute('x', (a.x+b.x)/2);
    wt.setAttribute('y', (a.y+b.y)/2 - 4);
    wt.setAttribute('class','w-label');
    wt.textContent = e.weight+'m';
    eg.appendChild(wt);
  });

  // Draw nodes
  graph.nodes.forEach(n => {
    const {x, y} = xy(n.id);
    const isSrc  = hlPath[0]===n.id || dfsOrd[0]===n.id;
    const isDst  = hlPath[hlPath.length-1]===n.id && hlPath.length>1;
    const onPath = hlPath.includes(n.id);
    const onDFS  = dfsOrd.includes(n.id);

    let cls = 'node-circle';
    if (isSrc && hlPath.length) cls += ' is-src';
    else if (isDst) cls += ' is-dst';
    else if (onPath) cls += ' on-path';
    else if (onDFS)  cls += ' dfs-vis';

    const c = svgEl('circle');
    c.setAttribute('cx',x); c.setAttribute('cy',y); c.setAttribute('r',22);
    c.setAttribute('class',cls);
    c.addEventListener('click', () => toast('📍 '+n.id));
    ng.appendChild(c);

    const lbl = svgEl('text');
    lbl.setAttribute('x',x); lbl.setAttribute('y',y+4);
    lbl.setAttribute('class','node-lbl');
    lbl.textContent = n.id.length>8 ? n.id.slice(0,7)+'…' : n.id;
    ng.appendChild(lbl);

    // DFS visit number
    const idx = dfsOrd.indexOf(n.id);
    if (idx !== -1) {
      const num = svgEl('text');
      num.setAttribute('x',x+18); num.setAttribute('y',y-14);
      num.setAttribute('class','dfs-num');
      num.textContent = idx+1;
      ng.appendChild(num);
    }
  });
}

function svgEl(tag) {
  return document.createElementNS('http://www.w3.org/2000/svg', tag);
}
function isEdgeOnPath(a, b, path) {
  for (let i=0;i<path.length-1;i++)
    if ((path[i]===a&&path[i+1]===b)||(path[i]===b&&path[i+1]===a)) return true;
  return false;
}
function isEdgeOnDFS(a, b, order) {
  for (let i=0;i<order.length-1;i++)
    if ((order[i]===a&&order[i+1]===b)||(order[i]===b&&order[i+1]===a)) return true;
  return false;
}

/* ─── DIJKSTRA ───────────────────────────────────────────── */
async function runDijkstra() {
  const from = document.getElementById('n-from').value;
  const to   = document.getElementById('n-to').value;
  const res  = await get(`/api/shortest?from=${from}&to=${to}`);
  const box  = document.getElementById('n-result');
  if (!res || res.error) {
    box.innerHTML = `<span style="color:var(--red)">${res?.error||'Error'}</span>`;
  } else {
    hlPath = res.path; dfsOrd = []; drawMap();
    box.innerHTML = `<b style="color:var(--green)">Distance: ${res.distance}m</b>
      <div class="path">${res.path.map((n,i)=>
        `<span class="pnode">${n}</span>${i<res.path.length-1?'<span class="parr">→</span>':''}`
      ).join('')}</div>`;
  }
  box.classList.add('show');
}
/* ─── DFS ────────────────────────────────────────────────── */
async function runDFS() {
  const from = document.getElementById('dfs-from').value;
  const res  = await get(`/api/dfs?from=${from}`);
  const box  = document.getElementById('dfs-result');
  if (!res || res.error) {
    box.innerHTML = `<span style="color:var(--red)">${res?.error||'Error'}</span>`;
  } else {
    dfsOrd = res.order; hlPath = []; drawMap();
    box.innerHTML = `<b style="color:var(--purple)">DFS Traversal (${res.order.length} nodes):</b>
      <div class="path" style="margin-top:8px">${
        res.order.map((n,i)=>`<span class="pnode dfs">${i+1}. ${n}</span>`).join(' ')
      }</div>`;
  }
  box.classList.add('show');
}

/* ─── ALL PATHS (DFS BACKTRACKING) ────────────────────────── */
async function runAllPaths() {
  const from = document.getElementById('dfs-path-from').value;
  const to   = document.getElementById('dfs-path-to').value;
  const res  = await get(`/api/allpaths?from=${from}&to=${to}`);
  const box  = document.getElementById('dfs-result');
  
  if (!res || res.error || !res.paths || res.paths.length === 0) {
    box.innerHTML = `<span style="color:var(--red)">No paths found.</span>`;
  } else {
    let html = `<b style="color:var(--purple)">Found ${res.paths.length} distinct paths:</b>`;
    res.paths.forEach((path, i) => {
      html += `
        <div style="margin-top:10px; padding:8px; border:1px solid var(--border); border-radius:6px; background:rgba(0,0,0,0.1)">
          <small style="color:var(--muted)">Route #${i+1}</small>
          <div class="path">${path.map(n => `<span class="pnode dfs">${n}</span>`).join('→')}</div>
          <button class="btn btn-sm btn-outline" style="margin-top:6px; font-size:10px" 
                  onclick="showSpecificDFS(['${path.join("','")}'])">Show on Map</button>
        </div>`;
    });
    box.innerHTML = html;
  }
  box.classList.add('show');
}

function showSpecificDFS(path) {
  hlPath = path;
  dfsOrd = [];
  drawMap();
  toast("Path highlighted on map");
}

/* ─── DASHBOARD QUICK NAV ────────────────────────────────── */
async function quickNav() {
  const from = document.getElementById('d-from').value;
  const to   = document.getElementById('d-to').value;
  const res  = await get(`/api/shortest?from=${from}&to=${to}`);
  const box  = document.getElementById('d-nav-result');
  if (!res || res.error) {
    box.innerHTML = `<span style="color:var(--red)">${res?.error||'Error'}</span>`;
  } else {
    box.innerHTML = `<b style="color:var(--green)">Distance: ${res.distance}m</b>
      <div class="path" style="margin-top:6px">${
        res.path.map((n,i)=>`<span class="pnode">${n}</span>${i<res.path.length-1?'→':''}`).join(' ')
      }</div>`;
  }
  box.classList.add('show');
}

/* ─── DASHBOARD QUICK ROOM ───────────────────────────────── */
async function quickRoom() {
  const id  = document.getElementById('d-room').value.trim().toUpperCase();
  const res = await get(`/api/room?id=${encodeURIComponent(id)}`);
  const box = document.getElementById('d-room-result');
  if (!res || res.error) {
    box.innerHTML = `<span style="color:var(--red)">Room not found</span>`;
  } else {
    box.innerHTML = `<b style="color:var(--blue)">${res.room}</b>
      <span class="badge b-green" style="margin-left:6px">${res.type}</span><br>
      Building: <span class="badge b-blue">${res.building}</span>
      Floor: <span class="badge b-yellow">${res.floor}</span>`;
  }
  box.classList.add('show');
}

/* ─── ROOM TABLE & SEARCH ────────────────────────────────── */
async function loadRoomsTable() {
  const res = await get('/api/rooms');
  const div = document.getElementById('rooms-table');
  if (!res || res.error) { div.innerHTML='Error loading rooms'; return; }
  
  // Group by building
  const bMap = {};
  res.forEach(r => {
    if (!bMap[r.building]) bMap[r.building] = [];
    bMap[r.building].push(r);
  });

  let html = '';
  for (const b in bMap) {
    html += `<div style="margin-bottom:15px">
      <div style="font-weight:700; color:var(--blue); margin-bottom:8px">${b}</div>
      <div class="grid3">
        ${bMap[b].map(r => `<div class="tt-cell" style="cursor:pointer" onclick="clickRoom('${r.id}')">
          <div style="font-weight:600">${r.id}</div>
          <div style="font-size:10px; color:var(--muted)">${r.type}</div>
        </div>`).join('')}
      </div>
    </div>`;
  }
  div.innerHTML = html;
}

function clickRoom(id) {
  document.getElementById('r-id').value = id;
  showPanel('classrooms');
  searchRoom();
}

async function searchRoom() {
  const id = document.getElementById('r-id').value.trim();
  const from = document.getElementById('r-from').value;
  const box = document.getElementById('r-result');
  if (!id) return;

  const res = await get(`/api/room?id=${encodeURIComponent(id)}&from=${from}`);
  if (!res || res.error) { box.innerHTML='Room not found'; }
  else {
    let h = `<b style="color:var(--green)">${res.room}</b> (${res.type})<br>
             Building: ${res.building} | Floor: ${res.floor}`;
    if (res.navPath) {
      h += `<div style="margin-top:8px; padding-top:8px; border-top:1px solid var(--border)">
              <small style="color:var(--muted)">Directions from ${from} (${res.navDist}m):</small>
              <div class="path">${res.navPath.map(n => `<span class="pnode">${n}</span>`).join('→')}</div>
            </div>`;
      hlPath = res.navPath; dfsOrd = []; drawMap();
    }
    box.innerHTML = h;
  }
  box.classList.add('show');
}

/* ─── TIMETABLE ──────────────────────────────────────────── */
async function loadTT() {
  const c = document.getElementById('tt-course').value;
  const res = await get(`/api/timetable?course=${c}`);
  const div = document.getElementById('tt-display');
  if (!res) return;

  const days = ['Monday','Tuesday','Wednesday','Thursday','Friday'];
  const slots = ['09:00-10:00','10:00-11:00','11:00-12:00','12:00-13:00','14:00-15:00','15:00-16:00'];

  let h = `<div class="tt-wrap"><div class="tt-grid">
    <div class="tt-cell tt-head">Time \ Day</div>
    ${days.map(d => `<div class="tt-cell tt-head">${d}</div>`).join('')}`;

  slots.forEach(s => {
    h += `<div class="tt-cell tt-time">${s}</div>`;
    days.forEach(d => {
      const entry = res.find(e => e.day===d && e.time===s);
      if (entry) {
        h += `<div class="tt-cell tt-entry" onclick="clickRoom('${entry.room}')">
                <div class="tt-subj">${entry.subject}</div>
                <div class="tt-fac">${entry.faculty}</div>
                <div class="tt-room">${entry.room}</div>
              </div>`;
      } else {
        h += `<div class="tt-cell tt-empty">Free</div>`;
      }
    });
  });
  h += `</div></div>`;
  div.innerHTML = h;
}

/* ─── FREE ROOMS ─────────────────────────────────────────── */
async function findFree() {
  const day = document.getElementById('fr-day').value;
  const time = document.getElementById('fr-time').value;
  const res = await get(`/api/freerooms?day=${day}&time=${time}`);
  const div = document.getElementById('fr-result');
  if (!res || res.error) { div.innerHTML='Error'; return; }

  div.innerHTML = `<div class="room-cards">
    ${res.map(r => `<div class="room-card" style="cursor:pointer" onclick="clickRoom('${r.room}')">
      <div class="rid">${r.room}</div>
      <div class="rmeta">${r.building}<br>${r.floor}</div>
    </div>`).join('')}
  </div>`;
}

/* ─── ADMIN ──────────────────────────────────────────────── */
async function loadPending() {
  const res = await get('/api/pending');
  const div = document.getElementById('pending-list');
  if (!res || res.length===0) { div.innerHTML='<p style="color:var(--muted)">No pending requests.</p>'; return; }

  div.innerHTML = `<table>
    <tr><th>User</th><th>Role</th><th>Name</th><th>Email</th><th>Action</th></tr>
    ${res.map(p => `<tr>
      <td><b>${p.username}</b></td>
      <td><span class="badge b-purple">${p.role}</span></td>
      <td>${p.fullName}</td>
      <td>${p.email}</td>
      <td>
        <button class="btn btn-sm btn-green" onclick="approve('${p.username}','approve')">Approve</button>
        <button class="btn btn-sm btn-red" onclick="approve('${p.username}','reject')">Reject</button>
      </td>
    </tr>`).join('')}
  </table>`;
}

async function approve(u, act) {
  const res = await post('/api/approve', {username:u, action:act});
  if (res && res.ok) {
    toast(`User ${u} ${act}d!`, 'ok');
    loadPending();
    loadUsers();
  }
}

async function loadUsers() {
  const res = await get('/api/users');
  const div = document.getElementById('users-list');
  if (!res) return;
  div.innerHTML = `<table>
    <tr><th>Username</th><th>Role</th></tr>
    ${res.map(u => `<tr>
      <td><b>${u.username}</b></td>
      <td><span class="badge b-blue">${u.role}</span></td>
    </tr>`).join('')}
  </table>`;
}

// Global click-to-close for results
window.onclick = (e) => {
  if (!e.target.closest('.card')) {
    // Optionally close boxes, but better to keep them open for UX
  }
}