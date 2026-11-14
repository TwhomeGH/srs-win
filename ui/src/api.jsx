import axios from 'axios';

const client = axios.create({
  baseURL: '/api/v1',
  timeout: 10000,
});

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
