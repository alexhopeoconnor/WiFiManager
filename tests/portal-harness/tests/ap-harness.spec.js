const { test, expect } = require('@playwright/test');

const hasStationCredentials = Boolean(process.env.PORTAL_STATION_ENV);

async function saveDiagnosticScreenshot(page, target) {
  // The optional station hand-off submits real local credentials. Playwright's
  // automatic artifacts are disabled in that mode, and explicit screenshots
  // must honor the same boundary.
  if (!hasStationCredentials) {
    await page.screenshot({ path: target, fullPage: true });
  }
}

const fixtureParameters = [
  { id: 'installation_label', value: 'Harness fixture' },
  { id: 'escaped_value', value: "7(f+4]2y3fsYTQt'Uhxc\"d\\<>&" },
  { id: 'mqtt_host', value: 'broker.example.local' },
  { id: 'mqtt_port', value: '1883' },
  { id: 'device_room', value: 'Workshop' },
  { id: 'sensor_name', value: 'Ambient temperature' },
  { id: 'telemetry_topic', value: 'sensors/ambient/temperature' },
  { id: 'timezone', value: 'Australia/Brisbane' },
  { id: 'latitude', value: '-27.4698' },
  { id: 'longitude', value: '153.0251' },
  { id: 'firmware_channel', value: 'stable' },
  { id: 'owner_name', value: 'Portal test harness' },
  { id: 'notes', value: 'Thirteen-field rendering fixture' },
];
const escapedUpdatedValue = "updated 'quote\" slash\\<>&";

async function json(response) {
  return JSON.parse(await response.text());
}

function expectFixtureParameters(payload, expected = fixtureParameters) {
  expect(payload.params).toHaveLength(expected.length);
  const byId = new Map(payload.params.map((field) => [field.id, field]));
  for (const { id, value } of expected) {
    expect(byId.get(id), `missing fixture field ${id}`).toBeDefined();
    expect(byId.get(id).value, `unexpected value for ${id}`).toBe(value);
  }
}

async function waitForScan(request) {
  let result;
  await expect.poll(async () => {
    try {
      const response = await request.get('/api/wifi/scan-status');
      if (!response.ok()) return true;
      result = await json(response);
      return result.scanning;
    } catch {
      // ESP8266 AP+STA scans briefly leave the AP channel. A client can lose
      // its association while the radio scans, then reconnect before the
      // asynchronous scan completes. Keep polling; the assertions below still
      // require a reachable portal with a complete, valid result.
      return true;
    }
  }, { timeout: 45_000, intervals: [500, 800, 1_000] }).toBe(false);
  return result;
}

test.describe('portal AP test harness', () => {
  test('serves API, retains all thirteen fixture parameters, and completes a real scan', async ({ request }) => {
    const root = await request.get('/');
    expect(root.ok()).toBeTruthy();
    expect(await root.text()).toContain('<html');

    const [bootstrapResponse, concurrentRoot] = await Promise.all([
      request.get('/api/bootstrap'),
      request.get('/'),
    ]);
    expect(concurrentRoot.ok()).toBeTruthy();
    const bootstrap = await json(bootstrapResponse);
    expect(bootstrap.contractVersion).toBe(3);
    expect(bootstrap.context.portalActive).toBe(true);
    expect(bootstrap.layout.paramsLocation).toBe('setup');

    const metaResponse = await request.get('/api/wifi/meta');
    expect(metaResponse.ok()).toBeTruthy();
    const meta = await json(metaResponse);
    // The fixture intentionally uses a separate Settings page. Its custom
    // parameters are therefore served by /api/params rather than duplicated
    // in the Wi-Fi form metadata.
    expect(meta.params).toHaveLength(0);

    const infoResponse = await request.get('/api/info');
    expect(infoResponse.ok()).toBeTruthy();
    expect(JSON.stringify(await json(infoResponse))).toContain('192.168.4.1');

    const statusResponse = await request.get('/api/status');
    expect(statusResponse.ok()).toBeTruthy();

    // Issue #1787 was intermittent and memory-sensitive on ESP8266. Fetch
    // the complete API response repeatedly so ordinary portal runs verify
    // every field and value without opting into the longer browser soak.
    for (let attempt = 0; attempt < 4; attempt += 1) {
      const paramsResponse = await request.get('/api/params');
      expect(paramsResponse.ok(), `parameter fetch ${attempt + 1}`).toBeTruthy();
      expectFixtureParameters(await json(paramsResponse));
    }

    const resetResponse = await request.post('/api/portal/timeout-reset');
    expect(resetResponse.ok()).toBeTruthy();
    expect((await json(resetResponse)).timeoutSecondsRemaining).toBeGreaterThan(800);

    const scanStart = await request.post('/api/wifi/scan');
    expect([200, 202, 409]).toContain(scanStart.status());
    const completed = await waitForScan(request);
    expect(completed.state).toBe('complete');
    expect(completed.results_valid).toBe(true);

    const missing = await request.get('/not-a-portal-route');
    expect(missing.status()).toBe(404);
  });

  test('renders stable desktop and mobile portal views without page errors', async ({ browser }) => {
    test.skip(process.env.PORTAL_BROWSER_MODE === 'skip', 'Browser checks were explicitly skipped.');
    const desktop = await browser.newContext({ viewport: { width: 1440, height: 1080 } });
    const page = await desktop.newPage();
    const errors = [];
    page.on('pageerror', (error) => errors.push(error.message));
    page.on('console', (message) => {
      if (message.type() === 'error') errors.push(message.text());
    });

    await page.goto('/', { waitUntil: 'networkidle' });
    await expect(page.locator('#wm-reset-portal-timeout')).toBeVisible();
    await saveDiagnosticScreenshot(page, `${process.env.ARTIFACT_DIR}/portal-overview-desktop.png`);

    await page.goto('/#/wifi', { waitUntil: 'networkidle' });
    await expect(page.locator('#wm-refresh-scan')).toBeVisible();
    await page.goto('/#/setup', { waitUntil: 'networkidle' });
    await expect(page.locator('#wm-param-form input')).toHaveCount(fixtureParameters.length);
    await expect(page.locator('#wm-f-installation_label')).toBeVisible();
    await expect(page.locator('#wm-f-escaped_value')).toHaveValue(fixtureParameters[1].value);
    await page.locator('#wm-f-installation_label').fill('Browser verified');
    await saveDiagnosticScreenshot(page, `${process.env.ARTIFACT_DIR}/portal-wifi-desktop.png`);

    const mobile = await browser.newContext({ viewport: { width: 390, height: 844 }, isMobile: true });
    const mobilePage = await mobile.newPage();
    mobilePage.on('pageerror', (error) => errors.push(error.message));
    mobilePage.on('console', (message) => {
      if (message.type() === 'error') errors.push(message.text());
    });
    await mobilePage.goto('/#/info', { waitUntil: 'networkidle' });
    await expect(mobilePage.locator('.wm-page-head')).toBeVisible();
    await saveDiagnosticScreenshot(mobilePage, `${process.env.ARTIFACT_DIR}/portal-device-mobile.png`);

    await mobile.close();
    await desktop.close();
    expect(errors).toEqual([]);
  });

  test('round-trips quotes, apostrophes, backslashes, and HTML-sensitive values', async ({ page, request }) => {
    test.skip(process.env.PORTAL_BROWSER_MODE === 'skip', 'Browser checks were explicitly skipped.');

    await page.goto('/#/setup', { waitUntil: 'networkidle' });
    await expect(page.locator('#wm-param-form input')).toHaveCount(fixtureParameters.length);
    const escapedInput = page.locator('#wm-f-escaped_value');
    await expect(escapedInput).toHaveValue(fixtureParameters[1].value);

    await escapedInput.fill(escapedUpdatedValue);
    const saveResponse = page.waitForResponse((response) => (
      response.url().endsWith('/api/params/save')
      && response.request().method() === 'POST'
    ));
    await page.locator('#wm-param-form button[type="submit"]').click();
    expect((await saveResponse).ok()).toBeTruthy();

    const paramsResponse = await request.get('/api/params');
    expect(paramsResponse.ok()).toBeTruthy();
    const expected = fixtureParameters.map((field) => (
      field.id === 'escaped_value' ? { ...field, value: escapedUpdatedValue } : field
    ));
    expectFixtureParameters(await json(paramsResponse), expected);

    await page.reload({ waitUntil: 'networkidle' });
    await expect(page.locator('#wm-f-escaped_value')).toHaveValue(escapedUpdatedValue);
  });

  test('ESP8266 repeatedly renders all thirteen custom parameters', async ({ page, request }) => {
    test.skip(process.env.PORTAL_CUSTOM_PARAMETER_STRESS !== '1',
      'ESP8266 custom-parameter browser stress was not requested.');
    test.skip(process.env.PORTAL_PLATFORM !== 'esp8266',
      'Custom-parameter browser stress is scoped to ESP8266.');

    const expected = fixtureParameters.map((field) => (
      field.id === 'escaped_value' ? { ...field, value: escapedUpdatedValue } : field
    ));
    const resetResponse = await request.post('/api/params/save', {
      form: Object.fromEntries(expected.map(({ id, value }) => [id, value])),
    });
    expect(resetResponse.ok()).toBeTruthy();

    for (let attempt = 0; attempt < 12; attempt += 1) {
      const paramsResponse = await request.get('/api/params');
      expect(paramsResponse.ok(), `API fetch ${attempt + 1}`).toBeTruthy();
      const params = await json(paramsResponse);
      expectFixtureParameters(params, expected);

      await page.goto('/#/setup', { waitUntil: 'networkidle' });
      await expect(page.locator('#wm-param-form input')).toHaveCount(expected.length);
      for (const { id, value } of expected) {
        await expect(page.locator(`#wm-f-${id}`), `${id}, render ${attempt + 1}`).toHaveValue(value);
      }
    }
  });
});
