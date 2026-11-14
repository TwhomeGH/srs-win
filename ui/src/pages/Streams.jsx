import React from 'react';
import { Alert, Badge, Button, Table } from '../components/Layout';
import { ApiNotice } from '../components/ApiNotice';
import { apiDelete, apiErrorMessage, apiGet } from '../api';
import { buildStreamUrl, fmtAudio, fmtKbps, fmtVideo } from '../format';
import { usePolling } from '../usePolling';

export default function Streams({ t }) {
  const loader = React.useCallback(async () => {
    const [streams, vhosts] = await Promise.all([
      apiGet('/streams/'),
      apiGet('/vhosts/').catch(() => ({ vhosts: [] })),
    ]);
    const owners = new Map((vhosts.vhosts || []).map((v) => [v.id, v.name]));
    return (streams.streams || []).map((s) => ({ ...s, ownerName: owners.get(s.vhost) || s.vhost }));
  }, []);
  const { data, error, updatedAt, reload } = usePolling(loader, 3000);

  const kick = async (cid) => {
    if (!cid || !window.confirm(t('confirmKick'))) return;
    try {
      await apiDelete(`/clients/${cid}`);
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
          ? <Alert variant="info">{t('noStreams')}</Alert>
          : (
            <Table responsive>
              <thead>
                <tr>
                  <th>{t('id')}</th>
                  <th>{t('name')}</th>
                  <th>{t('app')}</th>
                  <th>URL</th>
                  <th>{t('status')}</th>
                  <th>{t('clients')}</th>
                  <th>{t('networkIn')}</th>
                  <th>{t('networkOut')}</th>
                  <th>{t('video')}</th>
                  <th>{t('audio')}</th>
                  <th>{t('manage')}</th>
                </tr>
              </thead>
              <tbody>
                {data.map((s) => (
                  <tr key={s.id}>
                    <td><code>{s.id}</code></td>
                    <td>{s.name}</td>
                    <td>{s.app}</td>
                    <td className="text-break"><code>{buildStreamUrl(s.ownerName, s)}</code></td>
                    <td>{s.publish?.active ? <Badge variant="success">{t('publishing')}</Badge> : <Badge variant="secondary">{t('idle')}</Badge>}</td>
                    <td>{s.clients ?? 0}</td>
                    <td>{fmtKbps(s.kbps?.recv_30s)}</td>
                    <td>{fmtKbps(s.kbps?.send_30s)}</td>
                    <td className="small">{fmtVideo(s.video)}</td>
                    <td className="small">{fmtAudio(s.audio)}</td>
                    <td>
                      <Button
                        variant="outline-danger"
                        size="sm"
                        disabled={!s.publish?.active || !s.publish?.cid}
                        onClick={() => kick(s.publish?.cid)}
                      >
                        {t('kick')}
                      </Button>
                    </td>
                  </tr>
                ))}
              </tbody>
            </Table>
          )}
    </>
  );
}
