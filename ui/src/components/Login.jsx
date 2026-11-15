import React from 'react';
import { Alert, Button } from './Layout';

// Basic-auth login for SRS http_api.auth. Credentials are kept in
// sessionStorage and sent only to the same origin.
export default function Login({ onLogin, t }) {
  const [username, setUsername] = React.useState('');
  const [password, setPassword] = React.useState('');
  const [error, setError] = React.useState('');
  const [busy, setBusy] = React.useState(false);

  const submit = async (event) => {
    event.preventDefault();
    setError('');
    setBusy(true);
    try {
      await onLogin(username.trim(), password);
    } catch (e) {
      setError(e?.message || t('loginFailed'));
    } finally {
      setBusy(false);
    }
  };

  return (
    <div className="min-vh-100 d-flex align-items-center justify-content-center bg-body-tertiary">
      <form onSubmit={submit} className="card shadow-sm p-4" style={{ width: '22rem' }}>
        <h1 className="h4 mb-1">SRS Console</h1>
        <p className="text-secondary small mb-3">{t('loginHint')}</p>
        {error && <Alert variant="warning">{error}</Alert>}
        <div className="mb-3">
          <label className="form-label" htmlFor="srs-username">{t('loginUsername')}</label>
          <input
            id="srs-username"
            className="form-control"
            autoComplete="username"
            value={username}
            onChange={(e) => setUsername(e.target.value)}
            required
          />
        </div>
        <div className="mb-3">
          <label className="form-label" htmlFor="srs-password">{t('loginPassword')}</label>
          <input
            id="srs-password"
            type="password"
            className="form-control"
            autoComplete="current-password"
            value={password}
            onChange={(e) => setPassword(e.target.value)}
            required
          />
        </div>
        <Button type="submit" disabled={busy}>
          {busy ? t('loginBusy') : t('loginSubmit')}
        </Button>
      </form>
    </div>
  );
}
