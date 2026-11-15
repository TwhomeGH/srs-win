// Build playback URLs for the console. Keep this pure so it can be tested
// without a browser or a running SRS instance.
export function httpServerBase(host, port) {
  const value = port ? `:${port}` : '';
  return `http://${host || '127.0.0.1'}${value}`;
}

export function buildMediaUrl({ protocol, app, stream, apiOrigin, host, httpPort }) {
  const cleanApp = encodeURIComponent(app || 'live');
  const cleanStream = encodeURIComponent(stream || '');
  if (!cleanStream) return '';

  if (protocol === 'whep') {
    const base = apiOrigin || window.location.origin;
    return `${base.replace(/\/$/, '')}/rtc/v1/whep/?app=${cleanApp}&stream=${cleanStream}`;
  }

  const base = httpServerBase(host, httpPort);
  const suffix = protocol === 'hls' ? 'm3u8' : 'flv';
  return `${base}/${cleanApp}/${cleanStream}.${suffix}`;
}
