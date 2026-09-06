const { test, expect } = require('@playwright/test');

async function json(response) {
  return JSON.parse(await response.text());
}

async function waitForScan(request) {
  let result;
  await expect.poll(async () => {
    const response = await request.get('/api/wifi/scan-status');
    expect(response.ok()).toBeTruthy();
    result = await json(response);
    return result.scanning;
  }, { timeout: 45_000, intervals: [500, 800, 1_000] }).toBe(false);
  return result;
}

test.describe('portal AP contract', () => {
  test('serves API, persists fixture parameters, and completes a real scan', async ({ request }) => {
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

    const metaResponse = await request.get('/api/wifi/meta');
    expect(metaResponse.ok()).toBeTruthy();
    const meta = await json(metaResponse);
    expect(JSON.stringify(meta)).toContain('installation_label');

    const infoResponse = await request.get('/api/info');
    expect(infoResponse.ok()).toBeTruthy();
    expect(JSON.stringify(await json(infoResponse))).toContain('192.168.4.1');

    const statusResponse = await request.get('/api/status');
    expect(statusResponse.ok()).toBeTruthy();

    const saveResponse = await request.post('/api/params/save', {
      form: { installation_label: 'Contract verified' },
    });
    expect(saveResponse.ok()).toBeTruthy();
    expect(JSON.stringify(await json(saveResponse))).toContain('saved');

    const paramsResponse = await request.get('/api/params');
    expect(paramsResponse.ok()).toBeTruthy();
    expect(JSON.stringify(await json(paramsResponse))).toContain('Contract verified');

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
    await page.screenshot({ path: `${process.env.ARTIFACT_DIR}/portal-overview-desktop.png`, fullPage: true });

    await page.goto('/#/wifi', { waitUntil: 'networkidle' });
    await expect(page.locator('#wm-refresh-scan')).toBeVisible();
    await expect(page.locator('#wm-f-installation_label')).toBeVisible();
    await page.locator('#wm-f-installation_label').fill('Browser verified');
    await page.screenshot({ path: `${process.env.ARTIFACT_DIR}/portal-wifi-desktop.png`, fullPage: true });

    const mobile = await browser.newContext({ viewport: { width: 390, height: 844 }, isMobile: true });
    const mobilePage = await mobile.newPage();
    mobilePage.on('pageerror', (error) => errors.push(error.message));
    mobilePage.on('console', (message) => {
      if (message.type() === 'error') errors.push(message.text());
    });
    await mobilePage.goto('/#/info', { waitUntil: 'networkidle' });
    await expect(mobilePage.locator('.wm-page-head')).toBeVisible();
    await mobilePage.screenshot({ path: `${process.env.ARTIFACT_DIR}/portal-device-mobile.png`, fullPage: true });

    await mobile.close();
    await desktop.close();
    expect(errors).toEqual([]);
  });
});
