import { describe, expect, it } from 'vitest';
import { FlvAnalyzer } from '../src/flvAnalyzer.js';

function u24(n) {
  return [(n >> 16) & 0xff, (n >> 8) & 0xff, n & 0xff];
}

function u32(n) {
  return [(n >>> 24) & 0xff, (n >>> 16) & 0xff, (n >>> 8) & 0xff, n & 0xff];
}

function tag(type, timestamp, data) {
  const header = new Uint8Array(11);
  header[0] = type;
  header.set(u24(data.length), 1);
  header.set(u24(timestamp), 4);
  header[7] = (timestamp >>> 24) & 0xff;
  return new Uint8Array([
    ...header,
    ...data,
    ...u32(11 + data.length),
  ]);
}

function flv(tags) {
  return new Uint8Array([
    ...new TextEncoder().encode('FLV'),
    1,
    0x05,
    ...u32(9),
    ...u32(0),
    ...tags.flatMap((t) => [...t]),
  ]);
}

describe('FlvAnalyzer', () => {
  it('parses a split FLV stream and counts media without counting AVC config as a keyframe', () => {
    const analyzer = new FlvAnalyzer();
    const stream = flv([
      tag(18, 0, [2, 0, 10]),
      tag(9, 0, [0x17, 0x00, 1, 2]), // AVC sequence header
      tag(9, 0, [0x17, 0x01, 0, 0, 0, 0, 1, 0x65]), // keyframe
      tag(8, 0, [0xaf, 0x00, 0x12, 0x10]), // AAC AudioSpecificConfig: 44.1kHz stereo
      tag(9, 1000, [0x27, 0x01, 0, 0, 0, 0, 1, 0x41]), // inter frame
      tag(9, 2000, [0x17, 0x01, 0, 0, 0, 0, 1, 0x65]), // keyframe
    ]);

    // Split across arbitrary network reads to exercise buffer reassembly.
    for (let i = 0; i < stream.length; i += 7) {
      analyzer.feed(stream.slice(i, i + 7));
    }

    const s = analyzer.snapshot();
    expect(s.tags).toBe(6);
    expect(s.scriptTags).toBe(1);
    expect(s.videoTags).toBe(4);
    expect(s.audioTags).toBe(1);
    expect(s.videoCodec).toBe('H.264');
    expect(s.audioCodec).toBe('AAC');
    expect(s.audioRate).toBe(44100);
    expect(s.audioChannels).toBe(2);
    expect(s.keyframes).toBe(2);
    expect(s.keyGaps).toEqual([2000]);
    expect(s.firstTimestamp).toBe(0);
    expect(s.lastTimestamp).toBe(2000);
  });

  it('keeps partial tags buffered until the complete tag arrives', () => {
    const analyzer = new FlvAnalyzer();
    const one = tag(9, 0, [0x17, 0x01, 0x00]);
    const stream = flv([one]);
    const header = stream.slice(0, 9 + 4);
    analyzer.feed(header);
    analyzer.feed(stream.slice(header.length, header.length + 4));
    expect(analyzer.snapshot().tags).toBe(0);
    analyzer.feed(stream.slice(header.length + 4));
    expect(analyzer.snapshot().tags).toBe(1);
    expect(analyzer.snapshot().keyframes).toBe(1);
  });

  it('rejects a non-FLV response instead of silently counting bytes', () => {
    const analyzer = new FlvAnalyzer();
    expect(() => analyzer.feed(new TextEncoder().encode('<html>not a stream'))).toThrow('Not an FLV stream');
  });
});
