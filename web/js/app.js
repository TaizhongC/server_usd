import { initRenderer, updateMesh, highlightLayer, setPickHandler } from './render.js';
import { createClient } from './net.js';
import { createUi } from './ui.js';

let pendingHeader = null;

const ui = createUi({
  onLayerSelect: (path) => {
    highlightLayer(path);
    ui.setHighlightedLayer(path);
  },
});

setPickHandler((path) => {
  if (path) {
    ui.setHighlightedLayer(path);
  }
});

const client = createClient({
  onOpen: (url) => ui.setStatus(`Connected (${url})`),
  onClose: (url) => ui.setStatus(`Disconnected (last ${url})`),
  onError: (url) => ui.setStatus(`Connection error (${url})`),
  onText: handleText,
  onBinary: handleBinary,
});

function handleText(text) {
  let parsed;
  try {
    parsed = JSON.parse(text);
  } catch (err) {
    console.warn('Failed to parse message', err);
    return;
  }

  if (parsed.cmd === 'UI_BUILD') {
    // UI is static for now.
  } else if (parsed.cmd === 'SCENE_UPDATE') {
    pendingHeader = parsed;
  } else if (parsed.cmd === 'UI_ACK') {
    ui.setStatus(`Ack: ${parsed.action}`);
  } else if (parsed.cmd === 'SCENE_LAYERS') {
    ui.setLayers(parsed.layers || []);
  }
}

function handleBinary(buffer) {
  if (!pendingHeader) return;
  const vertices = new Float32Array(buffer);
  const path = pendingHeader.path || '/World/TestMesh';
  updateMesh(path, vertices);
  highlightLayer(path);
  ui.setHighlightedLayer(path);
  pendingHeader = null;
}

initRenderer();
client.connect();
