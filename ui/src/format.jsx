export function fmtBytes(n) {
  const value = Number(n);
  if (!Number.isFinite(value) || value < 0) return '-';
  if (value < 1024) return `${value} B`;
  const units = ['KB', 'MB', 'GB', 'TB'];
  let v = value / 1024;
  let i = 0;
  while (v >= 1024 && i < units.length - 1) {
    v /= 1024;
    i += 1;
  }
  return `${v.toFixed(1)} ${units[i]}`;
}

export function fmtKbps(bitsPerSecond) {
  const value = Number(bitsPerSecond);
  if (!Number.isFinite(value) || value <= 0) return '-';
  if (value < 1000) return `${value.toFixed(0)} bps`;
  if (value < 1000 * 1000) return `${(value / 1000).toFixed(1)} kbps`;
  return `${(value / 1000 / 1000).toFixed(2)} Mbps`;
}

export function fmtPercent(n) {
  const value = Number(n);
  if (!Number.isFinite(value)) return '-';
  return `${value.toFixed(2)}%`;
}

export function fmtFixed(n, digits = 2) {
  const value = Number(n);
  if (!Number.isFinite(value)) return '-';
  return value.toFixed(digits);
}

export function fmtUptime(seconds) {
  const value = Number(seconds);
  if (!Number.isFinite(value) || value < 0) return '-';
  const s = Math.floor(value % 60);
  const m = Math.floor((value / 60) % 60);
  const h = Math.floor((value / 3600) % 24);
  const d = Math.floor(value / 86400);
  if (d > 0) return `${d}d ${h}h ${m}m`;
  if (h > 0) return `${h}h ${m}m ${s}s`;
  if (m > 0) return `${m}m ${s}s`;
  return `${s}s`;
}

export function fmtVideo(video) {
  if (!video) return '-';
  const parts = [video.codec, video.profile, video.level];
  if (video.width && video.height) parts.push(`${video.width}x${video.height}`);
  return parts.filter(Boolean).join(' ') || '-';
}

export function fmtAudio(audio) {
  if (!audio) return '-';
  const channels = audio.channels !== undefined ? audio.channels : audio.channel;
  const ch = channels === 2 ? 'stereo' : channels === 1 ? 'mono' : channels ? `${channels}ch` : '';
  return [audio.codec, audio.sample_rate, ch, audio.profile].filter(Boolean).join(' ') || '-';
}

export function buildStreamUrl(vhost, stream) {
  const app = stream?.app || '';
  const name = stream?.name || '';
  if (!app || !name) return '-';
  if (!vhost || vhost === '__defaultVhost__') return `${app}/${name}`;
  return `${vhost}/${app}/${name}`;
}
