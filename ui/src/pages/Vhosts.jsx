import React from 'react';
import { Alert, Badge, Table } from '../components/Layout';
import { ApiNotice } from '../components/ApiNotice';
import { apiGet } from '../api';
import { fmtKbps } from '../format';
import { usePolling } from '../usePolling';

export default function Vhosts({ t }) {
  const loader = React.useCallback(async () => {
    const body = await apiGet('/vhosts/');
    return body.vhosts || [];
  }, []);
  const { data, error, updatedAt, reload } = usePolling(loader, 3000);

  return (
    <>
      <ApiNotice error={error} onRetry={reload} t={t} updatedAt={updatedAt} />
      {!data
        ? <Alert variant="info">{t('loading')}</Alert>
        : data.length === 0
          ? <Alert variant="info">{t('noData')}</Alert>
          : (
            <Table responsive>
              <thead>
                <tr>
                  <th>{t('id')}</th>
                  <th>{t('name')}</th>
                  <th>{t('status')}</th>
                  <th>{t('streams')}</th>
                  <th>{t('clients')}</th>
                  <th>{t('networkIn')}</th>
                  <th>{t('networkOut')}</th>
                  <th>HLS</th>
                </tr>
              </thead>
              <tbody>
                {data.map((v) => (
                  <tr key={v.id}>
                    <td><code>{v.id}</code></td>
                    <td>{v.name}</td>
                    <td>{v.enabled ? <Badge variant="success">{t('enabled')}</Badge> : <Badge variant="secondary">{t('disabled')}</Badge>}</td>
                    <td>{v.streams ?? 0}</td>
                    <td>{v.clients ?? 0}</td>
                    <td>{fmtKbps(v.kbps?.recv_30s)}</td>
                    <td>{fmtKbps(v.kbps?.send_30s)}</td>
                    <td>{v.hls?.enabled ? <Badge variant="success">{t('enabled')}</Badge> : <Badge variant="secondary">{t('disabled')}</Badge>}</td>
                  </tr>
                ))}
              </tbody>
            </Table>
          )}
    </>
  );
}
