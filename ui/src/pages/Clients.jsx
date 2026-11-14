import React from 'react';
import { Alert, Button, Table } from '../components/Layout';
import { ApiNotice } from '../components/ApiNotice';
import { apiDelete, apiErrorMessage, apiGet } from '../api';
import { fmtKbps, fmtUptime } from '../format';
import { usePolling } from '../usePolling';

export default function Clients({ t }) {
  const loader = React.useCallback(async () => {
    const body = await apiGet('/clients/');
    return body.clients || [];
  }, []);
  const { data, error, updatedAt, reload } = usePolling(loader, 3000);

  const kick = async (id) => {
    if (!window.confirm(t('confirmKick'))) return;
    try {
      await apiDelete(`/clients/${id}`);
      await reload();
    } catch (e) {
      window.alert(apiErrorMessage(e, t));
    }
  };

  return (
    <>
      <ApiNotice error={error} onRetry={reload} t={t} updatedAt={updatedAt} />
      {!data
        ? <Alert variant="info">{t('loading')}</Alert>
        : data.length === 0
          ? <Alert variant="info">{t('noClients')}</Alert>
          : (
            <Table responsive>
              <thead>
                <tr>
                  <th>{t('id')}</th>
                  <th>{t('type')}</th>
                  <th>{t('ip')}</th>
                  <th>{t('vhosts')}</th>
                  <th>{t('stream')}</th>
                  <th>{t('alive')}</th>
                  <th>{t('networkIn')}</th>
                  <th>{t('networkOut')}</th>
                  <th>{t('manage')}</th>
                </tr>
              </thead>
              <tbody>
                {data.map((c) => (
                  <tr key={c.id}>
                    <td><code>{c.id}</code></td>
                    <td>{c.type}</td>
                    <td>{c.ip}</td>
                    <td>{c.vhost}</td>
                    <td>{c.stream || '-'}</td>
                    <td>{fmtUptime(c.alive)}</td>
                    <td>{fmtKbps(c.kbps?.recv_30s)}</td>
                    <td>{fmtKbps(c.kbps?.send_30s)}</td>
                    <td>
                      <Button variant="outline-danger" size="sm" onClick={() => kick(c.id)}>{t('kick')}</Button>
                    </td>
                  </tr>
                ))}
              </tbody>
            </Table>
          )}
    </>
  );
}
