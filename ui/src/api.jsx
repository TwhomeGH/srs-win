import axios from 'axios';

const AUTH_KEY = 'srs.console.token';
let currentAuth = null;

const client = axios.create({
  baseURL: '/api/v1',
  timeout: 10000,
  headers: {
    // The console must never reuse a cached API response across SRS restarts
    // or credential changes.
    'Cache-Control': 'no-store',
    Pragma: 'no-cache',
    // Suppress the browser's native Basic-auth dialog; the console uses its own
    // login form and bearer token instead.
    'X-SRS-Console': '1',
  },
});

export function applyAuth(auth) {
  currentAuth = auth;
  if (auth?.token) {
    client.defaults.headers.common.Authorization = `Bearer ${auth.token}`;
  } else {
    delete client.defaults.headers.common.Authorization;
  }
}

export function restoreAuth() {
  try {
    const raw = localStorage.getItem(AUTH_KEY);
    if (!raw) return null;
    const auth = JSON.parse(raw);
    if (!auth?.token || !auth?.expires_at || auth.expires_at * 1000 <= Date.now()) {
      localStorage.removeItem(AUTH_KEY);
      return null;
    }
    applyAuth(auth);
    return auth;
  } catch {
    return null;
  }
}

function persistAuth(auth) {
  try {
    // A bearer token, not the password. It expires after 7 days and is renewed
    // by the server while the console remains in use.
    localStorage.setItem(AUTH_KEY, JSON.stringify(auth));
  } catch {
    // Private mode may reject storage; keep the in-memory token.
  }
  applyAuth(auth);
  return auth;
}

export async function login(username, password) {
  const response = await fetch('/api/v1/console/login', {
    method: 'POST',
    cache: 'no-store',
    headers: { 'Content-Type': 'application/json', 'X-SRS-Console': '1' },
    body: JSON.stringify({ username, password }),
  });
  const body = await response.json().catch(() => ({}));
  if (!response.ok || body.code !== 0 || !body.data?.token) {
    throw new Error('invalid console credentials');
  }
  return persistAuth({
    username: body.data.username,
    token: body.data.token,
    expires_at: body.data.expires_at,
  });
}

export async function refreshAuth() {
  if (!currentAuth?.token) return null;
  const response = await fetch('/api/v1/console/refresh', {
    method: 'POST',
    cache: 'no-store',
    headers: { Authorization: `Bearer ${currentAuth.token}`, 'X-SRS-Console': '1' },
  });
  const body = await response.json().catch(() => ({}));
  if (!response.ok || body.code !== 0 || !body.data?.token) {
    clearAuth();
    throw new Error('console token refresh failed');
  }
  return persistAuth({
    username: currentAuth.username,
    token: body.data.token,
    expires_at: body.data.expires_at,
  });
}

// Keep a long-lived tab alive without prompting for the password again.
export async function refreshTokenIfNeeded() {
  if (!currentAuth?.token) return null;
  const remaining = currentAuth.expires_at * 1000 - Date.now();
  if (remaining > 24 * 60 * 60 * 1000) return currentAuth;
  return refreshAuth();
}

export function clearAuth() {
  currentAuth = null;
  try {
    localStorage.removeItem(AUTH_KEY);
  } catch {
    // Ignore storage failures.
  }
  applyAuth(null);
}

export function getAuth() {
  return currentAuth;
}

export function authHeader() {
  if (!currentAuth?.token) return {};
  return { Authorization: `Bearer ${currentAuth.token}` };
}

// Restore a token before the first API request.
restoreAuth();

// Let the shell reflect connectivity without polling another endpoint. A
// response means the API is reachable; a response-less failure is a network
// outage. HTTP errors still mean the server is up, so they are not marked
// offline here.
client.interceptors.response.use(
  (res) => {
    window.dispatchEvent(new Event('srs:api-reachable'));
    return res;
  },
  (err) => {
    if (!err.response) window.dispatchEvent(new Event('srs:api-unreachable'));
    if (err.response?.status === 401) window.dispatchEvent(new Event('srs:api-unauthorized'));
    return Promise.reject(err);
  },
);

export async function apiGet(path, params) {
  const res = await client.get(path, { params });
  const body = res.data;
  if (!body || body.code !== 0) {
    const error = new Error(body?.code ? `SRS API code ${body.code}` : 'Invalid SRS API response');
    error.code = body?.code;
    error.body = body;
    throw error;
  }
  return body;
}

export async function apiDelete(path) {
  const res = await client.delete(path);
  const body = res.data;
  if (!body || body.code !== 0) {
    const error = new Error(body?.code ? `SRS API code ${body.code}` : 'Invalid SRS API response');
    error.code = body?.code;
    error.body = body;
    throw error;
  }
  return body;
}

export function isNetworkError(error) {
  if (!error || error.body) return false;
  return error.isAxiosError === true || error.name === 'AxiosError'
    || error.code === 'ERR_NETWORK' || error.code === 'ECONNABORTED' || error.code === 'ETIMEDOUT';
}

export function apiErrorMessage(error, t) {
  if (isNetworkError(error)) return t('serverOfflineDetail');
  if (error?.code) return `${t('apiError')}: ${error.code}`;
  if (error?.response?.status) return `${t('apiError')}: HTTP ${error.response.status}`;
  return error?.message || t('apiError');
}
