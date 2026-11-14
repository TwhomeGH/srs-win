import axios from 'axios';

const client = axios.create({
  baseURL: '/api/v1',
  timeout: 10000,
});

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

export function apiErrorMessage(error, t) {
  if (error?.code) return `${t('apiError')}: ${error.code}`;
  if (error?.response?.status) return `${t('apiError')}: HTTP ${error.response.status}`;
  return error?.message || t('apiError');
}
