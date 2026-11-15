// Authentication E2E for the console token flow.
// Requires SRS http_api.auth enabled and SRS_AUTH_USERNAME/SRS_AUTH_PASSWORD set.
import { chromium } from 'playwright-core';

const base = (process.env.SRS_BASE || 'http://127.0.0.1:11985').replace(/\/$/, '');
const username = process.env.SRS_AUTH_USERNAME || 'admin';
const password = process.env.SRS_AUTH_PASSWORD || '';
const browser = await chromium.launch({
  channel: process.env.PLAYWRIGHT_CHANNEL || 'chrome',
  headless: true,
  args: ['--disable-background-timer-throttling'],
});
const context = await browser.newContext();
const page = await context.newPage();

try {
  await page.goto(`${base}/console/`, { waitUntil: 'domcontentloaded' });
  await page.getByRole('textbox', { name: '使用者名稱' }).fill(username);
  await page.getByRole('textbox', { name: '密碼' }).fill(password);
  await page.getByRole('button', { name: '登入' }).click();
  await page.getByRole('button', { name: '登出' }).waitFor({ timeout: 15000 });
  if (!(await page.evaluate(() => !!localStorage.getItem('srs.console.token')))) {
    throw new Error('login did not persist a bearer token');
  }

  // Reload must reuse the token, not show the login form.
  await page.reload({ waitUntil: 'domcontentloaded' });
  await page.getByRole('button', { name: '登出' }).waitFor({ timeout: 15000 });

  // Refresh must rotate the token without asking for the password.
  const refreshed = await page.evaluate(async () => {
    const auth = JSON.parse(localStorage.getItem('srs.console.token'));
    const response = await fetch('/api/v1/console/refresh', {
      method: 'POST',
      cache: 'no-store',
      headers: { Authorization: `Bearer ${auth.token}`, 'X-SRS-Console': '1' },
    });
    const body = await response.json();
    return {
      status: response.status,
      code: body.code,
      oldExpires: auth.expires_at,
      newExpires: body.data?.expires_at,
      changed: body.data?.token !== auth.token,
    };
  });
  if (refreshed.status !== 200 || refreshed.code !== 0 || refreshed.newExpires < refreshed.oldExpires) {
    throw new Error(`token refresh failed: ${JSON.stringify(refreshed)}`);
  }

  await page.getByRole('button', { name: '登出' }).click();
  await page.getByRole('textbox', { name: '使用者名稱' }).waitFor({ timeout: 10000 });

  console.log(JSON.stringify({ ok: true, username, refreshed }, null, 2));
} finally {
  await context.close().catch(() => {});
  await browser.close().catch(() => {});
}
