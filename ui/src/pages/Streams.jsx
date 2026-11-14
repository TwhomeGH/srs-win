import React from 'react';
import { Alert, Badge, Button, Table } from 'react-bootstrap';
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
  const { data, error, reload } = usePolling(loader, 3000);

  const kick = async (cid) => {
    if (!cid || !window.confirm(t('confirmKick'))) return;
    try {
      await apiDelete(`/clients/${cid}`);
      await reload();
    } catch (e) {
      window.alert(apiErrorMessage(e, t));
    }
  };

  if (error && !data) return <Alert variant="danger">{apiErrorMessage(error, t)}</Alert>;
  if (!data) return <Alert variant="info">{t('loading')}</Alert>;
  if (data.length === 0) return <Alert variant="info">{t('noStreams')}</Alert>;

  return (
    <Table responsive size="sm" striped bordered hover>
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
            <td>{s.publish?.active ? <Badge bg="success">{t('publishing')}</Badge> : <Badge bg="secondary">{t('idle')}</Badge>}</td>
            <td>{s.clients ?? 0}</td>
            <td>{fmtKbps(s.kbps?.recv_30s)}</td>
            <td>{fmtKbps(s.kbps?.send_30s)}</td>
            <td className="small">{fmtVideo(s.video)}</td>
            <td className="small">{fmtAudio(s.audio)}</td>
            <td>
              <Button
                size="sm"
                variant="outline-danger"
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
  );
}
