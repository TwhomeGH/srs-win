// RTC media E2E: publish a synthetic WHIP stream, play it through the console
// WHEP player, and verify the browser actually decoded video.
//
// Run with a full SRS instance (RTC enabled) serving /console/:
//   SRS_BASE=http://127.0.0.1:11985 node e2e/rtc-roundtrip.mjs
import { chromium } from 'playwright-core';

const base = (process.env.SRS_BASE || 'http://127.0.0.1:11985').replace(/\/$/, '');
const streamName = `rtc-e2e-${Date.now()}`;
const app = 'live';

function fail(message) {
  throw new Error(message);
}

async function launchBrowser() {
  const args = [
    '--autoplay-policy=no-user-gesture-required',
    '--use-fake-ui-for-media-stream',
    '--allow-loopback-in-peer-connection',
    '--disable-features=WebRtcHideLocalIpsWithMdns',
    '--force-webrtc-ip-handling-policy=default',
    '--disable-background-timer-throttling',
    '--disable-backgrounding-occluded-windows',
    '--disable-renderer-backgrounding',
  ];
  const channels = [process.env.PLAYWRIGHT_CHANNEL, 'chrome', 'msedge'].filter(Boolean);
  const errors = [];
  for (const channel of channels) {
    try {
      return await chromium.launch({ channel, headless: true, args });
    } catch (err) {
      errors.push(`${channel}: ${err.message}`);
    }
  }
  fail(`unable to launch Chrome/Edge via playwright-core\n${errors.join('\n')}`);
}

const browser = await launchBrowser();
const context = await browser.newContext();
let publisher;
try {
  publisher = await context.newPage();
  await publisher.goto(`${base}/console/e2e/rtc-whip.html?app=${app}&stream=${streamName}`, {
    waitUntil: 'domcontentloaded',
  });
  await publisher.getByRole('button', { name: 'Publish' }).click();
  await publisher.waitForFunction(() => window.__rtcPublished === true, null, { timeout: 30000 });
  await publisher.waitForFunction(() => window.__rtcPublisher?.connectionState === 'connected', null, { timeout: 30000 });
  const publishState = await publisher.evaluate(() => window.__rtcPublisher?.connectionState);
  if (publishState !== 'connected') fail(`WHIP connection state is ${publishState}`);

  const consolePage = await context.newPage();
  await consolePage.goto(`${base}/console/`, { waitUntil: 'domcontentloaded' });
  await consolePage.getByRole('button', { name: '播放器', exact: true }).click();

  // Wait for the stream selector to receive the just-published stream.
  await consolePage.waitForFunction(
    (label) => [...document.querySelectorAll('select option')].some((o) => o.textContent === label),
    `${app}/${streamName}`,
    { timeout: 30000 },
  );
  await consolePage.locator('select').nth(1).selectOption({ label: `${app}/${streamName}` });
  await consolePage.getByRole('button', { name: '播放', exact: true }).click();

  await consolePage.waitForFunction(() => {
    const video = document.querySelector('video');
    return !!video && video.readyState >= 2 && video.videoWidth > 0 && video.videoHeight > 0;
  }, null, { timeout: 30000 });

  const video = await consolePage.evaluate(() => {
    const element = document.querySelector('video');
    return {
      readyState: element.readyState,
      paused: element.paused,
      width: element.videoWidth,
      height: element.videoHeight,
      tracks: element.srcObject?.getTracks?.().map((track) => track.kind) || [],
    };
  });

  // The original bug starved the ST scheduler when a playback consumer had no
  // packets, which made the HTTP API stop responding. Check it still does.
  const apiResponse = await consolePage.request.get(`${base}/api/v1/versions`, { timeout: 5000 });
  if (!apiResponse.ok()) fail(`API returned HTTP ${apiResponse.status()} during RTC playback`);

  console.log(JSON.stringify({ ok: true, stream: `${app}/${streamName}`, video }, null, 2));
} finally {
  if (publisher) await publisher.close().catch(() => {});
  await context.close().catch(() => {});
  await browser.close().catch(() => {});
}
