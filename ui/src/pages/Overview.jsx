import React from 'react';
import { Alert, Button, Card, Col, Row, Table, Badge } from 'react-bootstrap';
import { apiGet, apiErrorMessage } from '../api';
import { fmtBytes, fmtFixed, fmtKbps, fmtPercent, fmtUptime } from '../format';

export default function Overview({ t }) {
  const [env, setEnv] = React.useState(null);
  const [error, setError] = React.useState(null);
  const [rates, setRates] = React.useState(null);
  const previous = React.useRef(null);

  const load = React.useCallback(async () => {
    try {
      const body = await apiGet('/summaries');
      const data = body.data;
      const prev = previous.current;
      if (prev) {
        const dt = Number(data.system?.net_sample_time) - Number(prev.system?.net_sample_time);
        const srsDt = Number(data.system?.srs_sample_time) - Number(prev.system?.srs_sample_time);
        if (dt > 0) {
          setRates({
            inSys: (data.system.net_recv_bytes - prev.system.net_recv_bytes) * 8 / dt,
            outSys: (data.system.net_send_bytes - prev.system.net_send_bytes) * 8 / dt,
            inSrs: srsDt > 0 ? (data.system.srs_recv_bytes - prev.system.srs_recv_bytes) * 8 / srsDt : 0,
            outSrs: srsDt > 0 ? (data.system.srs_send_bytes - prev.system.srs_send_bytes) * 8 / srsDt : 0,
          });
        }
      }
      previous.current = data;
      setEnv(data);
      setError(null);
    } catch (e) {
      setError(e);
    }
  }, []);

  React.useEffect(() => {
    let cancelled = false;
    let timer = null;
    const tick = async () => {
      if (document.visibilityState === 'visible') await load();
      if (!cancelled) timer = setTimeout(tick, 3000);
    };
    tick();
    return () => {
      cancelled = true;
      if (timer) clearTimeout(timer);
    };
  }, [load]);

  if (error && !env) {
    return (
      <Alert variant="danger">
        {apiErrorMessage(error, t)}
        <Button size="sm" variant="outline-secondary" className="ms-2" onClick={load}>{t('retry')}</Button>
      </Alert>
    );
  }

  if (!env) return <Alert variant="info">{t('loading')}</Alert>;

  const self = env.self || {};
  const system = env.system || {};
  const row = (label, value) => (
    <tr>
      <td className="text-secondary">{label}</td>
      <td>{value ?? '-'}</td>
    </tr>
  );

  return (
    <>
      {error && <Alert variant="warning">{apiErrorMessage(error, t)}</Alert>}
      <Row className="g-3">
        <Col xs={12} md={6} xl={3}>
          <Card>
            <Card.Header>SRS {self.version || ''}</Card.Header>
            <Table size="sm" striped hover className="mb-0">
              <tbody>
                {row(t('uptime'), fmtUptime(self.srs_uptime))}
                {row(t('cpu'), fmtPercent(self.cpu_percent))}
                {row(t('memory'), `${fmtPercent(self.mem_percent)} (${fmtBytes(Number(self.mem_kbyte) * 1024)})`)}
                {row(t('connections'), system.conn_srs)}
                {row(t('networkIn'), rates ? fmtKbps(rates.inSrs) : '-')}
                {row(t('networkOut'), rates ? fmtKbps(rates.outSrs) : '-')}
              </tbody>
            </Table>
          </Card>
        </Col>

        <Col xs={12} md={6} xl={3}>
          <Card>
            <Card.Header>{t('system')}</Card.Header>
            <Table size="sm" striped hover className="mb-0">
              <tbody>
                {row(t('uptime'), fmtUptime(system.uptime))}
                {row(t('cpu'), fmtPercent(system.cpu_percent))}
                {row(t('memory'), fmtPercent(system.mem_ram_percent))}
                {row(t('load'), `${fmtFixed(system.load_1m)} / ${fmtFixed(system.load_5m)} / ${fmtFixed(system.load_15m)}`)}
                {row(t('processors'), `${system.cpus ?? '-'} (${system.cpus_online ?? '-'} online)`)}
              </tbody>
            </Table>
          </Card>
        </Col>

        <Col xs={12} md={6} xl={3}>
          <Card>
            <Card.Header>{t('networkIn')} / {t('networkOut')}</Card.Header>
            <Table size="sm" striped hover className="mb-0">
              <tbody>
                {row(`SRS ${t('networkIn')}`, rates ? fmtKbps(rates.inSrs) : '-')}
                {row(`SRS ${t('networkOut')}`, rates ? fmtKbps(rates.outSrs) : '-')}
                {row(`OS ${t('networkIn')}`, rates ? fmtKbps(rates.inSys) : '-')}
                {row(`OS ${t('networkOut')}`, rates ? fmtKbps(rates.outSys) : '-')}
                {row(t('connections'), `${system.conn_srs ?? '-'} / ${system.conn_sys ?? '-'}`)}
              </tbody>
            </Table>
          </Card>
        </Col>

        <Col xs={12} md={6} xl={3}>
          <Card>
            <Card.Header>{t('process')}</Card.Header>
            <Table size="sm" striped hover className="mb-0">
              <tbody>
                {row(t('pid'), self.pid)}
                {row(t('ppid'), self.ppid)}
                {row(t('healthy'), env.ok ? <Badge bg="success">{t('healthy')}</Badge> : <Badge bg="danger">-</Badge>)}
                {row(t('disk'), `${system.disk_read_KBps ?? '-'} / ${system.disk_write_KBps ?? '-'} KB/s`)}
              </tbody>
            </Table>
          </Card>
        </Col>
      </Row>
    </>
  );
}
