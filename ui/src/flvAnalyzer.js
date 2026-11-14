// Minimal incremental FLV parser for the push-diagnostics page. It only
// needs the tag stream, not media decoding: bytes, tag counts, codecs,
// keyframes, and timestamp continuity are enough to tell whether a publisher
// is delivering data.
const TAG_AUDIO = 8;
const TAG_VIDEO = 9;
const TAG_SCRIPT = 18;

function readU24(bytes, offset) {
  return (bytes[offset] << 16) | (bytes[offset + 1] << 8) | bytes[offset + 2];
}

function readU32(bytes, offset) {
  return ((bytes[offset] << 24) | (bytes[offset + 1] << 16) | (bytes[offset + 2] << 8) | bytes[offset + 3]) >>> 0;
}

function concat(a, b) {
  if (a.length === 0) return b;
  const out = new Uint8Array(a.length + b.length);
  out.set(a, 0);
  out.set(b, a.length);
  return out;
}

const VIDEO_CODECS = {
  2: 'Sorenson H.263',
  3: 'Screen video',
  4: 'VP6',
  5: 'VP6 alpha',
  6: 'Screen video 2',
  7: 'H.264',
  12: 'HEVC',
};

const AUDIO_CODECS = {
  0: 'PCM',
  1: 'ADPCM',
  2: 'MP3',
  3: 'PCM LE',
  4: 'Nellymoser 16k',
  5: 'Nellymoser 8k',
  6: 'Nellymoser',
  7: 'G.711 A-law',
  8: 'G.711 mu-law',
  10: 'AAC',
  11: 'Speex',
  14: 'MP3 8k',
};

const AUDIO_RATES = [5500, 11025, 22050, 44100];
const AAC_RATES = [96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350];

export class FlvAnalyzer {
  constructor() {
    this.buffer = new Uint8Array(0);
    this.headerDone = false;
    this.stats = {
      bytes: 0,
      tags: 0,
      videoTags: 0,
      audioTags: 0,
      scriptTags: 0,
      keyframes: 0,
      keyGaps: [],
      lastKeyTs: null,
      firstTimestamp: null,
      lastTimestamp: null,
      videoCodec: null,
      audioCodec: null,
      audioRate: null,
      audioChannels: null,
      lastVideoTs: null,
      lastAudioTs: null,
      metadata: null,
    };
  }

  feed(chunk) {
    const bytes = chunk instanceof Uint8Array ? chunk : new Uint8Array(chunk);
    this.stats.bytes += bytes.length;
    this.buffer = concat(this.buffer, bytes);

    if (!this.headerDone) {
      if (this.buffer.length < 9) return this.snapshot();
      if (String.fromCharCode(this.buffer[0], this.buffer[1], this.buffer[2]) !== 'FLV') {
        throw new Error('Not an FLV stream');
      }
      const dataOffset = readU32(this.buffer, 5);
      if (this.buffer.length < dataOffset + 4) return this.snapshot();
      // Skip the header and PreviousTagSize0.
      this.buffer = this.buffer.slice(dataOffset + 4);
      this.headerDone = true;
    }

    while (this.buffer.length >= 11) {
      const dataSize = readU24(this.buffer, 1);
      const total = 11 + dataSize + 4;
      if (this.buffer.length < total) break;

      const type = this.buffer[0];
      const timestamp = readU24(this.buffer, 4) | (this.buffer[7] << 24);
      const data = this.buffer.slice(11, 11 + dataSize);
      this.onTag(type, timestamp >>> 0, data);
      this.buffer = this.buffer.slice(total);
    }

    return this.snapshot();
  }

  onTag(type, timestamp, data) {
    const s = this.stats;
    s.tags += 1;
    if (s.firstTimestamp === null) s.firstTimestamp = timestamp;
    s.lastTimestamp = timestamp;

    if (type === TAG_VIDEO) {
      s.videoTags += 1;
      s.lastVideoTs = timestamp;
      if (data.length >= 2) {
        const frameType = data[0] >> 4;
        const codec = data[0] & 0x0f;
        s.videoCodec = VIDEO_CODECS[codec] || `codec ${codec}`;
        // AVC/HEVC sequence headers are configuration, not display frames;
        // count only a keyframe NALU so the GOP interval is meaningful.
        const packetType = data[1] & 0x0f;
        if ((codec === 7 || codec === 12) && frameType === 1 && packetType === 1) {
          s.keyframes += 1;
          if (s.lastKeyTs !== null) {
            const gap = timestamp - s.lastKeyTs;
            if (gap >= 0) {
              s.keyGaps.push(gap);
              if (s.keyGaps.length > 10) s.keyGaps.shift();
            }
          }
          s.lastKeyTs = timestamp;
        }
      }
    } else if (type === TAG_AUDIO) {
      s.audioTags += 1;
      s.lastAudioTs = timestamp;
      if (data.length >= 1) {
        const codec = data[0] >> 4;
        s.audioCodec = AUDIO_CODECS[codec] || `codec ${codec}`;
        if (codec === 10 && data.length >= 2) {
          // AAC sample rate lives in the AudioSpecificConfig (packet type 0),
          // not in the FLV sound-rate bits; parse the 5/4/4-bit fields.
          const packetType = data[1] & 0xff;
          if (packetType === 0 && data.length >= 4) {
            const frequencyIndex = ((data[2] & 0x07) << 1) | (data[3] >> 7);
            s.audioRate = AAC_RATES[frequencyIndex] || null;
            s.audioChannels = (data[3] >> 3) & 0x0f;
          }
        }
      }
    } else if (type === TAG_SCRIPT) {
      s.scriptTags += 1;
    }
  }

  snapshot() {
    return {
      ...this.stats,
      keyGaps: this.stats.keyGaps.slice(),
    };
  }
}
