const RETRY_MS = 2000;

function buildCandidates() {
  const params = new URLSearchParams(window.location.search);
  const override = params.get('ws');
  if (override) return [override];

  const proto = window.location.protocol === 'https:' ? 'wss' : 'ws';
  const host = window.location.host;
  const urls = [];
  if (host) urls.push(`${proto}://${host}/ws`);
  urls.push(`ws://localhost:8000/ws`);
  urls.push(`ws://127.0.0.1:8000/ws`);
  return urls;
}

export function createClient(handlers) {
  let socket = null;
  let reconnectTimer = null;
  const urls = buildCandidates();
  let urlIndex = 0;

  const getNextUrl = () => urls[urlIndex++ % urls.length];

  const connect = () => {
    if (socket && (socket.readyState === WebSocket.OPEN || socket.readyState === WebSocket.CONNECTING)) {
      return;
    }
    const url = getNextUrl();
    socket = new WebSocket(url);
    socket.binaryType = 'arraybuffer';

    socket.onopen = () => {
      handlers.onOpen?.(url);
      if (reconnectTimer) {
        clearTimeout(reconnectTimer);
        reconnectTimer = null;
      }
    };
    socket.onerror = (err) => {
      console.error('WebSocket error', err);
      handlers.onError?.(url, err);
    };
    socket.onclose = () => {
      handlers.onClose?.(url);
      if (!reconnectTimer) {
        reconnectTimer = setTimeout(() => {
          reconnectTimer = null;
          connect();
        }, RETRY_MS);
      }
    };
    socket.onmessage = (event) => {
      if (typeof event.data === 'string') {
        handlers.onText?.(event.data);
      } else if (event.data instanceof ArrayBuffer) {
        handlers.onBinary?.(event.data);
      }
    };
  };

  const sendJson = (obj) => {
    if (!socket || socket.readyState !== WebSocket.OPEN) return;
    socket.send(JSON.stringify(obj));
  };

  return { connect, sendJson };
}
