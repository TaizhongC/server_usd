import { initRenderer, updateMesh } from './render.js';

let socket;
let pendingHeader = null;
let gui;

const statusEl = document.getElementById('status');
const layerListEl = document.getElementById('layerList');

function setStatus(text) {
  if (statusEl) statusEl.textContent = text;
}

function setLayers(layers) {
  if (!layerListEl) return;
  layerListEl.innerHTML = '';
  if (!layers || layers.length === 0) {
    const li = document.createElement('li');
    li.textContent = 'No layers reported';
    layerListEl.appendChild(li);
    return;
  }
  layers.forEach((layer) => {
    const li = document.createElement('li');
    li.textContent = layer;
    layerListEl.appendChild(li);
  });
}

function connect() {
  const protocol = window.location.protocol === 'https:' ? 'wss' : 'ws';
  const url = `${protocol}://${window.location.host}/ws`;
  socket = new WebSocket(url);
  socket.binaryType = 'arraybuffer';

  socket.onopen = () => setStatus('Connected');
  socket.onerror = () => setStatus('Connection error');
  socket.onclose = () => setStatus('Disconnected');
  socket.onmessage = handleMessage;
}

function handleMessage(event) {
  if (typeof event.data === 'string') {
    handleText(event.data);
  } else if (event.data instanceof ArrayBuffer) {
    handleBinary(event.data);
  }
}

function handleText(text) {
  let parsed;
  try {
    parsed = JSON.parse(text);
  } catch (err) {
    console.warn('Failed to parse message', err);
    return;
  }

  if (parsed.cmd === 'UI_BUILD') {
    buildGui(parsed.controls || []);
  } else if (parsed.cmd === 'SCENE_UPDATE') {
    pendingHeader = parsed;
  } else if (parsed.cmd === 'UI_ACK') {
    setStatus(`Ack: ${parsed.action}`);
  } else if (parsed.cmd === 'SCENE_LAYERS') {
    setLayers(parsed.layers || []);
  }
}

function handleBinary(buffer) {
  if (!pendingHeader) return;
  const vertices = new Float32Array(buffer);
  updateMesh(vertices);
  pendingHeader = null;
}

function buildGui(controls) {
  if (!window.lil) return;
  if (gui) gui.destroy();
  gui = new window.lil.GUI({ title: 'Controls' });

  controls.forEach((ctrl) => {
    if (ctrl.type === 'button') {
      gui
        .add({ click: () => sendAction(ctrl.action) }, 'click')
        .name(ctrl.label || ctrl.action);
    } else if (ctrl.type === 'slider') {
      const params = { value: ctrl.value ?? 0 };
      gui
        .add(params, 'value', ctrl.min ?? 0, ctrl.max ?? 1, ctrl.step ?? 0.1)
        .name(ctrl.label || ctrl.action)
        .onChange((v) => sendAction(ctrl.action, v));
    }
  });
}

function sendAction(action, value) {
  if (!socket || socket.readyState !== WebSocket.OPEN) return;
  const payload = value === undefined ? { action } : { action, value };
  socket.send(JSON.stringify(payload));
}

initRenderer();
connect();
