// Optional LAN handoff exercised by the portal test harness.
const fs = require('fs');
const { test, expect } = require('@playwright/test');

function stationCredentials() {
  const file = process.env.PORTAL_STATION_ENV;
  if (!file) return null;
  const values = {};
  for (const raw of fs.readFileSync(file, 'utf8').split(/\r?\n/)) {
    if (!raw || raw.startsWith('#')) continue;
    const separator = raw.indexOf('=');
    if (separator > 0) values[raw.slice(0, separator)] = raw.slice(separator + 1);
  }
  if (!values.WIFI_SSID || !values.WIFI_PASSWORD) {
    throw new Error('PORTAL_STATION_ENV must define WIFI_SSID and WIFI_PASSWORD.');
  }
  return values;
}

test('optionally hands the fixture off to a real station network', async ({ request }) => {
  const credentials = stationCredentials();
  test.skip(!credentials, 'No station environment was supplied.');

  const metaResponse = await request.get('/api/wifi/meta');
  expect(metaResponse.ok()).toBeTruthy();
  const meta = JSON.parse(await metaResponse.text());
  const form = Array.isArray(meta.profiles) && meta.profiles.length
    ? { s0: credentials.WIFI_SSID, p0: credentials.WIFI_PASSWORD, stationAction: 'connect' }
    : { s: credentials.WIFI_SSID, p: credentials.WIFI_PASSWORD, stationAction: 'connect' };
  const queued = await request.post('/api/wifi/save', { form });
  expect(queued.status()).toBe(202);

  let state;
  await expect.poll(async () => {
    const response = await request.get('/api/wifi/connect-status');
    state = JSON.parse(await response.text());
    return state.state;
  }, { timeout: 45_000, intervals: [500, 700, 1_000] }).toBe('success');
  expect(state.stationIp).toMatch(/^\d+\.\d+\.\d+\.\d+$/);
  expect(state.redirectUrl).toContain(state.stationIp);

  const complete = await request.post('/api/wifi/connect-complete');
  expect(complete.ok()).toBeTruthy();
});
