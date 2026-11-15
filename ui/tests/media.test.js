import { describe, expect, it } from 'vitest';
import { buildMediaUrl, httpServerBase } from '../src/media.js';

describe('media URL builder', () => {
  it('builds a WHEP URL on the API origin', () => {
    expect(buildMediaUrl({
      protocol: 'whep',
      app: 'live',
      stream: 'demo',
      apiOrigin: 'http://127.0.0.1:11985',
    })).toBe('http://127.0.0.1:11985/rtc/v1/whep/?app=live&stream=demo');
  });

  it('builds HTTP-FLV and HLS URLs on the HTTP server port', () => {
    expect(buildMediaUrl({
      protocol: 'flv',
      app: 'live',
      stream: 'demo',
      host: '127.0.0.1',
      httpPort: 18088,
    })).toBe('http://127.0.0.1:18088/live/demo.flv');

    expect(buildMediaUrl({
      protocol: 'hls',
      app: 'live',
      stream: 'demo',
      host: '127.0.0.1',
      httpPort: 18088,
    })).toBe('http://127.0.0.1:18088/live/demo.m3u8');
  });

  it('encodes app and stream names and rejects an empty stream', () => {
    expect(buildMediaUrl({
      protocol: 'flv',
      app: 'live app',
      stream: 'a b',
      host: 'localhost',
      httpPort: 8088,
    })).toBe('http://localhost:8088/live%20app/a%20b.flv');

    expect(buildMediaUrl({ protocol: 'flv', app: 'live', stream: '' })).toBe('');
  });

  it('omits the port when it is not known', () => {
    expect(httpServerBase('localhost', '')).toBe('http://localhost');
  });
});
