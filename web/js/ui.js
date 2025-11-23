const statusEl = document.getElementById('status');
const layerListEl = document.getElementById('layerList');

export function createUi({ onLayerSelect }) {
  let highlightedPath = null;
  let layers = [];

  const setStatus = (text) => {
    if (statusEl) statusEl.textContent = text;
  };

  const setLayers = (rawLayers) => {
    layers = (rawLayers || []).map((label) => {
      const trimmed = label.trim();
      const space = trimmed.indexOf(' ');
      const path = space === -1 ? trimmed : trimmed.substring(0, space);
      return { label, path };
    });
    if (!layerListEl) return;
    layerListEl.innerHTML = '';
    if (layers.length === 0) {
      const li = document.createElement('li');
      li.textContent = 'No layers reported';
      layerListEl.appendChild(li);
      return;
    }
    layers.forEach((layer) => {
      const li = document.createElement('li');
      li.textContent = layer.label;
      li.dataset.layerPath = layer.path;
      li.onclick = () => {
        onLayerSelect?.(layer.path);
        setHighlightedLayer(layer.path);
      };
      layerListEl.appendChild(li);
    });
  };

  const clearHighlight = () => {
    if (!highlightedPath) return;
    const el = document.querySelector(`[data-layer-path="${highlightedPath}"]`);
    if (el) el.classList.remove('selected-layer');
    highlightedPath = null;
  };

  const setHighlightedLayer = (path) => {
    clearHighlight();
    if (!path) return;
    highlightedPath = path;
    const el = document.querySelector(`[data-layer-path="${highlightedPath}"]`);
    if (el) el.classList.add('selected-layer');
  };

  return { setStatus, setLayers, setHighlightedLayer };
}
