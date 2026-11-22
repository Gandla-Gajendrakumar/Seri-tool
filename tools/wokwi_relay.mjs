// Local relay so wokwi-cli works behind an HTTPS proxy.
// wokwi-cli's websocket ignores HTTPS_PROXY, but it honours WOKWI_CLI_SERVER. Point that at
// ws://127.0.0.1:<port>; this relay opens the real wss://wokwi.com connection through the proxy
// and passes messages both ways, forwarding the Authorization and User-Agent headers.
//
// Usage: node tools/wokwi_relay.mjs [port]      (default 18765; prints READY when listening)
import { WebSocketServer, WebSocket } from 'ws';
import { HttpsProxyAgent } from 'https-proxy-agent';

const port = Number(process.argv[2] ?? 18765);
const upstreamUrl = process.env.WOKWI_UPSTREAM ?? 'wss://wokwi.com/api/ws/beta';
const proxy = process.env.HTTPS_PROXY ?? process.env.https_proxy;
const agent = proxy ? new HttpsProxyAgent(proxy) : undefined;

const wss = new WebSocketServer({ host: '127.0.0.1', port });
wss.on('listening', () => console.log(`READY ws://127.0.0.1:${port} -> ${upstreamUrl}${proxy ? ' via proxy' : ''}`));
wss.on('connection', (client, req) => {
  const headers = {};
  for (const h of ['authorization', 'user-agent']) if (req.headers[h]) headers[h] = req.headers[h];
  const up = new WebSocket(upstreamUrl, { agent, headers });
  const pending = [];
  client.on('message', (data, isBinary) => {
    if (up.readyState === WebSocket.OPEN) up.send(data, { binary: isBinary });
    else pending.push([data, isBinary]);
  });
  up.on('open', () => { for (const [d, b] of pending.splice(0)) up.send(d, { binary: b }); });
  up.on('message', (data, isBinary) => client.readyState === WebSocket.OPEN && client.send(data, { binary: isBinary }));
  up.on('unexpected-response', (_req, res) => {
    console.error(`upstream refused: HTTP ${res.statusCode}`);
    client.close(1011, `upstream HTTP ${res.statusCode}`);
  });
  up.on('error', (e) => { console.error('upstream error:', e.message); client.close(1011, 'upstream error'); });
  up.on('close', (code, reason) => client.readyState === WebSocket.OPEN && client.close(code === 1005 ? 1000 : code, reason));
  client.on('close', () => up.close());
});
