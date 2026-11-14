import React from 'react';
import { Alert, Button } from './Layout';
import { apiErrorMessage, isNetworkError } from '../api';

// A non-destructive API warning: the page keeps rendering the last known data,
// and the user can retry without losing the table they are reading.
export function ApiNotice({ error, onRetry, t, updatedAt }) {
  if (!error) return null;

  const network = isNetworkError(error);
  const lastUpdated = updatedAt
    ? ` ${t('lastUpdated')}: ${updatedAt.toLocaleTimeString()}`
    : '';

  return (
    <Alert variant="warning" className="d-flex align-items-start gap-3">
      <div className="flex-grow-1">
        <strong>{network ? t('serverOffline') : t('apiError')}</strong>
        <div className="small">{apiErrorMessage(error, t)}</div>
        {network && lastUpdated && <div className="small text-secondary">{lastUpdated.trim()}</div>}
      </div>
      <Button variant="outline-secondary" size="sm" onClick={onRetry}>{t('retry')}</Button>
    </Alert>
  );
}
