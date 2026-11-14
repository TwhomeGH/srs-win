import React from 'react';
import { Alert, Badge, Table } from 'react-bootstrap';
import { apiGet, apiErrorMessage } from '../api';
import { fmtKbps } from '../format';
import { usePolling } from '../usePolling';

export default function Vhosts({ t }) {
  const loader = React.useCallback(async () => {
    const body = await apiGet('/vhosts/');
    return body.vhosts || [];
  }, []);
  const { data, error } = usePolling(loader, 3000);

  if (error && !data) return <Alert variant="danger">{apiErrorMessage(error, t)}</Alert>;
  if (!data) return <Alert variant="info">{t('loading')}</Alert>;
  if (data.length === 0) return <Alert variant="info">{t('noData')}</Alert>;

  return (
    <Table responsive size="sm" striped bordered hover>
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
            <td>{v.enabled ? <Badge bg="success">{t('enabled')}</Badge> : <Badge bg="secondary">{t('disabled')}</Badge>}</td>
            <td>{v.streams ?? 0}</td>
            <td>{v.clients ?? 0}</td>
            <td>{fmtKbps(v.kbps?.recv_30s)}</td>
            <td>{fmtKbps(v.kbps?.send_30s)}</td>
            <td>{v.hls?.enabled ? <Badge bg="success">{t('enabled')}</Badge> : <Badge bg="secondary">{t('disabled')}</Badge>}</td>
          </tr>
        ))}
      </tbody>
    </Table>
  );
}
