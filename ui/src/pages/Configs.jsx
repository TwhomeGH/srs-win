import React from 'react';
import { Alert, Badge, Card, Col, Row, Table } from '../components/Layout';
import { ApiNotice } from '../components/ApiNotice';
import { apiGet } from '../api';
import { usePolling } from '../usePolling';

export default function Configs({ t }) {
  const loader = React.useCallback(async () => {
    const [configBody, vhostBody, featureBody] = await Promise.all([
      apiGet('/config'),
      apiGet('/vhosts/').catch(() => ({ vhosts: [] })),
      apiGet('/features').catch(() => ({ data: { features: {} } })),
    ]);
    return {
      config: configBody.data || {},
      vhosts: vhostBody.vhosts || [],
      features: featureBody.data?.features || {},
    };
  }, []);
  const { data, error, updatedAt, reload } = usePolling(loader, 5000);

  const config = data?.config;
  const raw = config?.raw_api;

  return (
    <>
      <ApiNotice error={error} onRetry={reload} t={t} updatedAt={updatedAt} />

      {!data ? (
        <Alert variant="info">{t('loading')}</Alert>
      ) : (
        <>
          <Row className="g-3">
            <Col xs={12} md={6} xl={3}>
              <Card title={t('configServer')}>
                <div className="card-body">
                  <div className="text-secondary small">{t('version')}</div>
                  <div className="fs-5 mb-2">{config.version || '-'}</div>
                  <div className="text-secondary small">{t('configBuild')}</div>
                  <div>{config.build || '-'}{config.build_ts && config.build_ts !== '0' ? ` (${config.build_ts})` : ''}</div>
                </div>
              </Card>
            </Col>
            <Col xs={12} md={6} xl={3}>
              <Card title={t('configLimits')}>
                <div className="card-body">
                  <div className="text-secondary small">{t('configMaxConnections')}</div>
                  <div className="fs-5">{config.max_connections ?? '-'}</div>
                </div>
              </Card>
            </Col>
            <Col xs={12} md={6} xl={3}>
              <Card title={t('configRawApi')}>
                <div className="card-body">
                  <Badge variant={raw?.enabled ? 'warning' : 'secondary'}>
                    {raw?.enabled ? t('configRawEnabled') : t('configRawDisabled')}
                  </Badge>
                  <div className="text-secondary small mt-2">{t('configRawNote')}</div>
                </div>
              </Card>
            </Col>
            <Col xs={12} md={6} xl={3}>
              <Card title={t('configHttpServer')}>
                <div className="card-body">
                  <div className="small font-monospace text-break">{config.http_server_dir || '-'}</div>
                  {config.rtc_candidates && <div className="text-secondary small mt-2">RTC: {config.rtc_candidates}</div>}
                </div>
              </Card>
            </Col>
          </Row>

          <Row className="g-3 mt-1">
            <Col xs={12} xl={6}>
              <Card title={t('configListeners')}>
                <Table>
                  <thead>
                    <tr><th>{t('configListener')}</th><th>{t('status')}</th><th>{t('configAddress')}</th></tr>
                  </thead>
                  <tbody>
                    {Object.entries(config.listeners || {}).map(([name, addresses]) => (
                      <tr key={name}>
                        <td><code>{name}</code></td>
                        <td>{listenerEnabled(config, name) ? <Badge variant="success">{t('enabled')}</Badge> : <Badge variant="secondary">{t('disabled')}</Badge>}</td>
                        <td className="font-monospace small">{(addresses || []).join(', ') || '-'}</td>
                      </tr>
                    ))}
                  </tbody>
                </Table>
              </Card>
            </Col>
            <Col xs={12} xl={6}>
              <Card title={t('configFeatures')}>
                <div className="card-body d-flex flex-wrap gap-2">
                  {Object.entries(data.features).map(([name, yes]) => (
                    <Badge key={name} variant={yes ? 'success' : 'secondary'}>{name}</Badge>
                  ))}
                  <Badge variant={config.enabled?.srt ? 'success' : 'secondary'}>srt</Badge>
                  <Badge variant={config.enabled?.rtc ? 'success' : 'secondary'}>rtc</Badge>
                </div>
              </Card>
            </Col>
          </Row>

          <Card title={t('configVhosts')} className="mt-3">
            {data.vhosts.length === 0 ? (
              <div className="card-body text-secondary">{t('configNoVhosts')}</div>
            ) : (
              <Table responsive>
                <thead>
                  <tr>
                    <th>{t('name')}</th>
                    <th>{t('status')}</th>
                    <th>{t('streams')}</th>
                    <th>{t('clients')}</th>
                    <th>HLS</th>
                    <th>HTTP-FLV</th>
                  </tr>
                </thead>
                <tbody>
                  {data.vhosts.map((v) => (
                    <tr key={v.id}>
                      <td>{v.name}</td>
                      <td>{v.enabled ? <Badge variant="success">{t('enabled')}</Badge> : <Badge variant="secondary">{t('disabled')}</Badge>}</td>
                      <td>{v.streams ?? 0}</td>
                      <td>{v.clients ?? 0}</td>
                      <td>{v.hls?.enabled ? <Badge variant="success">{t('enabled')}</Badge> : <Badge variant="secondary">{t('disabled')}</Badge>}</td>
                      <td>{v.http_remux?.enabled ? <Badge variant="success">{t('enabled')}</Badge> : <span className="text-secondary">-</span>}</td>
                    </tr>
                  ))}
                </tbody>
              </Table>
            )}
          </Card>
        </>
      )}
    </>
  );
}

function listenerEnabled(config, name) {
  if (name === 'rtmp') return true;
  if (name === 'http_api') return !!config.enabled?.http_api;
  if (name === 'http_server') return !!config.enabled?.http_server;
  if (name === 'srt') return !!config.enabled?.srt;
  if (name === 'rtc' || name === 'rtc_tcp') return !!config.enabled?.rtc;
  return true;
}
