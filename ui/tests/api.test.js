import { describe, expect, it } from 'vitest';
import { apiErrorMessage, isNetworkError } from '../src/api.jsx';

describe('API error classification', () => {
  it('treats a response-less Axios failure as offline', () => {
    const error = Object.assign(new Error('Network Error'), { isAxiosError: true, code: 'ERR_NETWORK' });
    expect(isNetworkError(error)).toBe(true);
    expect(apiErrorMessage(error, (key) => key)).toBe('serverOfflineDetail');
  });

  it('does not treat an SRS business error as offline', () => {
    const error = Object.assign(new Error('SRS API code 1061'), { code: 1061, body: { code: 1061 } });
    expect(isNetworkError(error)).toBe(false);
    expect(apiErrorMessage(error, (key) => key)).toBe('apiError: 1061');
  });

  it('reports HTTP status separately from connectivity', () => {
    const error = Object.assign(new Error('HTTP 500'), { response: { status: 500 } });
    expect(isNetworkError(error)).toBe(false);
    expect(apiErrorMessage(error, (key) => key)).toBe('apiError: HTTP 500');
  });
});
