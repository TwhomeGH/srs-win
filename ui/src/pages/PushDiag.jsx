import React from 'react';
import { Alert, Badge, Button, Card, Col, Row } from '../components/Layout';
import { ApiNotice } from '../components/ApiNotice';
import { apiGet } from '../api';
import { fmtBytes, fmtKbps } from '../format';
import { usePolling } from '../usePolling';
import { FlvAnalyzer } from '../flvAnalyzer';

const STATUS_KEYS = {
  idle: 'pushIdle',
  connecting: 'pushConnecting',
  connected: 'pushConnected',
  ended: 'pushEnded',
  stopped: 'pushStopped',
  error: 'pushError',
};

const STATUS_VARIANTS = {
  idle: 'secondary',
  connecting: 'warning',
  connected: 'success',
  ended: 'secondary',
  stopped: 'secondary',
  error: 'danger',
};

export default function PushDiag({ t }) {
  const loader = React.useCallback(async () => {
    const body = await apiGet('/streams/');
    return body.streams || [];
  }, []);
  const { data: streams, error: streamsError, reload } = usePolling(loader, 5000);

  const [app, setApp] = React.useState('live');
  const [name, setName] = React.useState('');
  const [url, setUrl] = React.useState('');
  const [urlTouched, setUrlTouched] = React.useState(false);
  const [stats, setStats] = React.useState(null);
  const [status, setStatus] = React.useState('idle');
  const [running, setRunning] = React.useState(false);
  const [analysisError, setAnalysisError] = React.useState(null);
  const [elapsed, setElapsed] = React.useState(0);
  const [httpBase, setHttpBase] = React.useState('');
  const abortRef = React.useRef(null);
  const startedRef = React.useRef(0);
  const mountedRef = React.useRef(true);

  React.useEffect(() => {
    mountedRef.current = true;
    return () => {
      mountedRef.current = false;
      abortRef.current?.abort();
    };
  }, []);

  React.useEffect(() => {
    if (!urlTouched && app && name) {
      const path = `/${encodeURIComponent(app)}/${encodeURIComponent(name)}.flv`;
      setUrl(httpBase ? `${httpBase}${path}` : path);
    }
  }, [app, httpBase, name, urlTouched]);

  // The API and the HTTP-FLV server normally listen on different ports. Use
  // the new read-only config endpoint to build the default FLV URL instead of
  // guessing the API port; the user can still edit it.
  React.useEffect(() => {
    let cancelled = false;
    apiGet('/config')
      .then((body) => {
        if (cancelled) return;
        const port = body.data?.listeners?.http_server?.[0];
        if (port) {
          setHttpBase(`${window.location.protocol}//${window.location.hostname}:${port}`);
        }
      })
      .catch(() => {});
    return () => { cancelled = true; };
  }, []);

  React.useEffect(() => {
    if (!streams || streams.length === 0 || name) return;
    const first = streams.find((s) => s.publish?.active) || streams[0];
    if (first) {
      setApp(first.app);
      setName(first.name);
    }
  }, [streams, name]);

  React.useEffect(() => {
    if (!running) return undefined;
    const timer = setInterval(() => {
      setElapsed(Math.max(0, (Date.now() - startedRef.current) / 1000));
    }, 500);
    return () => clearInterval(timer);
  }, [running]);

  const selectedId = React.useMemo(
    () => (streams || []).find((s) => s.app === app && s.name === name)?.id || '',
    [streams, app, name],
  );

  const elapsedSeconds = elapsed;
  const bitrate = stats && elapsedSeconds > 0 ? stats.bytes * 8 / elapsedSeconds : 0;

  const start = async () => {
    if (running || !url) return;
    setAnalysisError(null);
    setStats(null);
    setStatus('connecting');
    setRunning(true);
    setElapsed(0);
    startedRef.current = Date.now();

    const analyzer = new FlvAnalyzer();
    const controller = new AbortController();
    abortRef.current = controller;

    try {
      const res = await fetch(url, {
        signal: controller.signal,
        cache: 'no-store',
        headers: { Accept: 'video/x-flv' },
      });
      if (!res.ok) throw new Error(`HTTP ${res.status}`);
      if (!res.body) throw new Error(t('pushNoResponseBody'));

      setStatus('connected');
      const reader = res.body.getReader();
      for (;;) {
        const { value, done } = await reader.read();
        if (done) {
          if (mountedRef.current) setStatus('ended');
          break;
        }
        if (!mountedRef.current) break;
        setStats(analyzer.feed(value));
      }
    } catch (e) {
      if (!mountedRef.current) return;
      if (e?.name === 'AbortError') {
        setStatus('stopped');
      } else {
        setStatus('error');
        setAnalysisError(e);
      }
    } finally {
      if (mountedRef.current) setRunning(false);
      abortRef.current = null;
    }
  };

  const stop = () => {
    abortRef.current?.abort();
    setRunning(false);
    setStatus('stopped');
  };

  const pickStream = (id) => {
    const s = (streams || []).find((item) => String(item.id) === id);
    if (!s) return;
    if (running) stop();
    setApp(s.app);
    setName(s.name);
    setUrlTouched(false);
  };

  const gop = stats?.keyGaps?.length ? stats.keyGaps[stats.keyGaps.length - 1] : null;
  const maxGop = stats?.keyGaps?.length ? Math.max(...stats.keyGaps) : null;
  const videoStale = stats && stats.videoTags > 0 && elapsedSeconds > 0 && stats.lastTimestamp - stats.lastVideoTs > 2000;
  const health = [];
  if (stats) {
    if (stats.videoTags > 0) {
      health.push(stats.audioTags > 0 ? t('pushHealthy') : t('pushVideoOnly'));
    } else if (stats.bytes > 0) {
      health.push(t('pushNoVideo'));
    }
    if (stats.videoTags > 0 && stats.keyframes === 0) health.push(t('pushNoKeyframe'));
    if (videoStale) health.push(t('pushStaleVideo'));
  }
  if (status === 'connected' && (!stats || stats.tags === 0)) health.push(t('pushNoData'));

  return (
    <>
      <ApiNotice error={streamsError} onRetry={reload} t={t} />

      <Card title={t('pushDiagTitle')}>
        <div className="card-body">
          <p className="text-secondary small mb-3">{t('pushHint')}</p>
          {!httpBase && <p className="text-warning small mb-3">{t('pushHttpServerHint')}</p>}
          <Row className="g-3">
            <Col md={5}>
              <label className="form-label" htmlFor="push-source">{t('pushSource')}</label>
              <select
                id="push-source"
                className="form-select"
                value={selectedId}
                onChange={(e) => pickStream(e.target.value)}
              >
                <option value="">{t('pushManual')}</option>
                {(streams || []).map((s) => (
                  <option value={s.id} key={s.id}>
                    {s.app}/{s.name} · {s.publish?.active ? t('publishing') : t('idle')} · {s.clients ?? 0} {t('clients')}
                  </option>
                ))}
              </select>
            </Col>
            <Col md={3}>
              <label className="form-label" htmlFor="push-app">{t('pushApp')}</label>
              <input
                id="push-app"
                className="form-control"
                value={app}
                onChange={(e) => { setApp(e.target.value); setUrlTouched(false); }}
              />
            </Col>
            <Col md={4}>
              <label className="form-label" htmlFor="push-name">{t('pushStreamName')}</label>
              <input
                id="push-name"
                className="form-control"
                value={name}
                onChange={(e) => { setName(e.target.value); setUrlTouched(false); }}
              />
            </Col>
          </Row>
          <div className="mt-3">
            <label className="form-label" htmlFor="push-url">{t('pushUrl')}</label>
            <input
              id="push-url"
              className="form-control font-monospace"
              value={url}
              onChange={(e) => { setUrl(e.target.value); setUrlTouched(true); }}
              placeholder="/live/stream.flv"
            />
          </div>
          <div className="d-flex align-items-center gap-2 mt-3">
            <Button variant={running ? 'outline-danger' : 'primary'} onClick={running ? stop : start} disabled={!url}>
              {running ? t('pushStop') : t('pushStart')}
            </Button>
            <Badge variant={STATUS_VARIANTS[status]}>{t(STATUS_KEYS[status])}</Badge>
            {running && <span className="text-secondary small">{t('pushAnalyzing')}</span>}
          </div>
        </div>
      </Card>

      {analysisError && (
        <Alert variant="danger">
          <strong>{t('pushError')}</strong>
          <div className="small">{analysisError.message}</div>
        </Alert>
      )}

      <Row className="g-3 mt-1">
        <Col xs={12} md={6} xl={3}>
          <Card title={t('pushStatus')}>
            <div className="card-body">
              <div className="fs-4"><Badge variant={STATUS_VARIANTS[status]}>{t(STATUS_KEYS[status])}</Badge></div>
              <div className="text-secondary small mt-2">{url || '-'}</div>
            </div>
          </Card>
        </Col>
        <Col xs={12} md={6} xl={3}>
          <Card title={t('pushReceived')}>
            <div className="card-body fs-4">{stats ? fmtBytes(stats.bytes) : '-'}</div>
          </Card>
        </Col>
        <Col xs={12} md={6} xl={3}>
          <Card title={t('pushBitrate')}>
            <div className="card-body fs-4">{stats ? fmtKbps(bitrate) : '-'}</div>
          </Card>
        </Col>
        <Col xs={12} md={6} xl={3}>
          <Card title={t('pushTags')}>
            <div className="card-body fs-4">{stats ? stats.tags : '-'}</div>
          </Card>
        </Col>
      </Row>

      <Row className="g-3 mt-1">
        <Col xs={12} md={6}>
          <Card title={t('pushMedia')}>
            <div className="card-body">
              <table className="table table-sm mb-0">
                <tbody>
                  <tr><td className="text-secondary">{t('pushVideo')}</td><td>{stats?.videoCodec || '-'} · {stats?.videoTags ?? 0} {t('pushPackets')}</td></tr>
                  <tr><td className="text-secondary">{t('pushAudio')}</td><td>{stats?.audioCodec || '-'}{stats?.audioRate ? ` · ${stats.audioRate} Hz` : ''}{stats?.audioChannels ? ` · ${stats.audioChannels}ch` : ''} · {stats?.audioTags ?? 0} {t('pushPackets')}</td></tr>
                  <tr><td className="text-secondary">{t('pushKeyframes')}</td><td>{stats?.keyframes ?? 0}</td></tr>
                  <tr><td className="text-secondary">{t('pushGop')}</td><td>{gop === null ? '-' : `${gop} ms${maxGop !== null && maxGop !== gop ? ` (${t('pushMax')} ${maxGop} ms)` : ''}`}</td></tr>
                  <tr><td className="text-secondary">{t('pushTimestamps')}</td><td>{stats ? `${stats.firstTimestamp ?? '-'} → ${stats.lastTimestamp ?? '-'} ms` : '-'}</td></tr>
                </tbody>
              </table>
            </div>
          </Card>
        </Col>
        <Col xs={12} md={6}>
          <Card title={t('pushHealth')}>
            <div className="card-body">
              {health.length === 0
                ? <span className="text-secondary">{running ? t('pushWaiting') : t('pushNotStarted')}</span>
                : health.map((item) => <div key={item} className="mb-1">{item}</div>)}
            </div>
          </Card>
        </Col>
      </Row>
    </>
  );
}
