import React from 'react';
import { Alert, Button } from 'react-bootstrap';
import { apiErrorMessage, apiGet } from '../api';

export default function Configs({ t }) {
  const [data, setData] = React.useState(null);
  const [error, setError] = React.useState(null);

  const load = React.useCallback(async () => {
    try {
      const body = await apiGet('/raw');
      setData(body);
      setError(null);
    } catch (e) {
      setData(null);
      setError(e);
    }
  }, []);

  React.useEffect(() => { load(); }, [load]);

  if (error?.code === 1061) {
    return (
      <Alert variant="warning">
        {t('configDisabled')}
        <Button size="sm" variant="outline-secondary" className="ms-2" onClick={load}>{t('retry')}</Button>
      </Alert>
    );
  }
  if (error) {
    return (
      <Alert variant="danger">
        {apiErrorMessage(error, t)}
        <Button size="sm" variant="outline-secondary" className="ms-2" onClick={load}>{t('retry')}</Button>
      </Alert>
    );
  }
  if (!data) return <Alert variant="info">{t('loading')}</Alert>;

  return <pre className="bg-light border rounded p-3 small">{JSON.stringify(data, null, 2)}</pre>;
}
