import React from 'react';
import { Alert, Button } from '../components/Layout';
import { apiErrorMessage, apiGet } from '../api';
import { buildMediaUrl } from '../media';

// Console media player: WHEP for WebRTC, hls.js for HLS, flv.js for HTTP-FLV.
// The media libraries are loaded on demand so the console's initial bundle does
// not pay for players the user never opens.
export default function Player({ t }) {
  const videoRef = React.useRef(null);
  const cleanupRef = React.useRef(null);
  const [streams, setStreams] = React.useState([]);
  const [httpPort, setHttpPort] = React.useState('');
  const [app, setApp] = React.useState('live');
  const [stream, setStream] = React.useState('');
  const [protocol, setProtocol] = React.useState('whep');
  const [status, setStatus] = React.useState('idle');
  const [error, setError] = React.useState('');

  React.useEffect(() => {
    let cancelled = false;
    Promise.all([apiGet('/streams/'), apiGet('/config')])
      .then(([streamBody, configBody]) => {
        if (cancelled) return;
        setStreams(streamBody.streams || []);
        const port = configBody.data?.listeners?.http_server?.[0] || '';
        setHttpPort(String(port || ''));
        if (streamBody.streams?.length) {
          const first = streamBody.streams[0];
          setApp(first.app || 'live');
          setStream(first.name || '');
        }
      })
      .catch((e) => { if (!cancelled) setError(apiErrorMessage(e, t)); });
    return () => { cancelled = true; };
  }, [t]);

  const stop = React.useCallback(() => {
    if (cleanupRef.current) {
      cleanupRef.current();
      cleanupRef.current = null;
    }
    if (videoRef.current) {
      videoRef.current.pause();
      videoRef.current.srcObject = null;
      videoRef.current.removeAttribute('src');
      videoRef.current.load();
    }
    setStatus('idle');
  }, []);

  React.useEffect(() => () => stop(), [stop]);

  const play = async () => {
    setError('');
    stop();
    if (!stream) {
      setError(t('playerNoStream'));
      return;
    }

    const url = buildMediaUrl({
      protocol,
      app,
      stream,
      apiOrigin: window.location.origin,
      host: window.location.hostname || '127.0.0.1',
      httpPort,
    });

    setStatus('connecting');
    try {
      if (protocol === 'whep') {
        if (typeof RTCPeerConnection === 'undefined') {
          throw new Error(t('playerRtcUnavailable'));
        }

        // A video-only stream (for example a WHIP canvas publisher) must not
        // offer an audio m-line: SRS answers with only the tracks it has, and
        // the browser rejects an answer whose m-line order differs.
        const selected = streams.find((item) => item.app === app && item.name === stream);
        const wantAudio = selected ? !!selected.audio : true;

        let pc = null;
        const startWhep = async (withAudio) => {
          pc = new RTCPeerConnection();
          const media = new MediaStream();
          pc.ontrack = (event) => {
            const tracks = event.streams[0]?.getTracks() || [event.track];
            tracks.forEach((track) => {
              if (!media.getTracks().some((existing) => existing.id === track.id)) {
                media.addTrack(track);
              }
            });
            if (videoRef.current) videoRef.current.srcObject = media;
          };
          if (withAudio) pc.addTransceiver('audio', { direction: 'recvonly' });
          pc.addTransceiver('video', { direction: 'recvonly' });
          await pc.setLocalDescription(await pc.createOffer());
          const response = await fetch(url, {
            method: 'POST',
            headers: { 'Content-Type': 'application/sdp' },
            body: pc.localDescription.sdp,
          });
          if (!response.ok) throw new Error(`WHEP HTTP ${response.status}`);
          await pc.setRemoteDescription({ type: 'answer', sdp: await response.text() });
        };

        try {
          await startWhep(wantAudio);
        } catch (e) {
          if (pc) pc.close();
          // Some SRS builds answer a video-only stream even when the client
          // offered audio. Retry once with a video-only offer.
          if (wantAudio && /m-line/.test(e?.message || '')) {
            await startWhep(false);
          } else {
            throw e;
          }
        }
        cleanupRef.current = () => pc.close();
      } else if (protocol === 'flv') {
        const { default: flvjs } = await import('flv.js');
        if (!flvjs.isSupported()) throw new Error(t('playerFlvUnsupported'));
        const player = flvjs.createPlayer({ type: 'flv', url, isLive: true });
        player.attachMediaElement(videoRef.current);
        player.load();
        await player.play();
        cleanupRef.current = () => {
          player.pause();
          player.unload();
          player.detachMediaElement();
          player.destroy();
        };
      } else {
        const { default: Hls } = await import('hls.js');
        if (Hls.isSupported()) {
          const hls = new Hls({ lowLatencyMode: true });
          hls.loadSource(url);
          hls.attachMedia(videoRef.current);
          hls.on(Hls.Events.ERROR, (_, data) => {
            if (data.fatal) setError(`${t('playerError')}: ${data.details || data.type}`);
          });
          cleanupRef.current = () => hls.destroy();
        } else if (videoRef.current.canPlayType('application/vnd.apple.mpegurl')) {
          videoRef.current.src = url;
        } else {
          throw new Error(t('playerHlsUnsupported'));
        }
      }
      setStatus('playing');
    } catch (e) {
      stop();
      setError(e?.message || t('playerError'));
      setStatus('error');
    }
  };

  return (
    <>
      {error && <Alert variant="warning">{error}</Alert>}
      <div className="row g-3">
        <div className="col-lg-8">
          <div className="ratio ratio-16x9 bg-black rounded">
            <video ref={videoRef} controls playsInline autoPlay className="w-100 h-100" />
          </div>
        </div>
        <div className="col-lg-4">
          <div className="mb-3">
            <label className="form-label">{t('playerProtocol')}</label>
            <select className="form-select" value={protocol} onChange={(e) => setProtocol(e.target.value)}>
              <option value="whep">WebRTC (WHEP)</option>
              <option value="flv">HTTP-FLV</option>
              <option value="hls">HLS</option>
            </select>
          </div>
          <div className="mb-3">
            <label className="form-label">{t('playerStream')}</label>
            <select
              className="form-select"
              value={stream ? `${app}/${stream}` : ''}
              onChange={(e) => {
                const [nextApp, nextStream] = e.target.value.split('/');
                setApp(nextApp || 'live');
                setStream(nextStream || '');
              }}
            >
              <option value="">{t('playerNoStream')}</option>
              {streams.map((item) => (
                <option key={`${item.app}/${item.name}`} value={`${item.app}/${item.name}`}>
                  {item.app}/{item.name}
                </option>
              ))}
            </select>
          </div>
          <div className="mb-3">
            <label className="form-label">{t('playerApp')}</label>
            <input className="form-control" value={app} onChange={(e) => setApp(e.target.value)} />
          </div>
          <div className="mb-3">
            <label className="form-label">{t('playerStreamName')}</label>
            <input className="form-control" value={stream} onChange={(e) => setStream(e.target.value)} />
          </div>
          <div className="d-flex gap-2">
            <Button onClick={play} disabled={status === 'connecting'}>
              {status === 'connecting' ? t('playerConnecting') : t('playerPlay')}
            </Button>
            <Button variant="outline-secondary" onClick={stop}>{t('playerStop')}</Button>
          </div>
          <div className="small text-secondary mt-3">
            {t('playerStatus')}: {t(`playerStatus_${status}`)}
          </div>
        </div>
      </div>
    </>
  );
}
