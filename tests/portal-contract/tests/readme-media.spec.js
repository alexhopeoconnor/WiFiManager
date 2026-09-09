const fs = require('fs');
const path = require('path');
const { test, expect } = require('@playwright/test');

function mediaPath(...parts) {
  const root = process.env.ARTIFACT_DIR || '/artifacts';
  const target = path.join(root, 'readme-media', ...parts);
  fs.mkdirSync(path.dirname(target), { recursive: true, mode: 0o700 });
  return target;
}

async function waitForCompletedScan(request) {
  await expect.poll(async () => {
    try {
      const response = await request.get('/api/wifi/scan-status');
      if (!response.ok()) return false;
      const result = JSON.parse(await response.text());
      return result.state === 'complete' && result.results_valid && result.count > 0;
    } catch {
      return false;
    }
  }, { timeout: 45_000, intervals: [500, 800, 1_000] }).toBe(true);
}

test.describe('WiFiManager README media', () => {
  test.skip(process.env.PORTAL_CAPTURE_README_MEDIA !== '1', 'README capture was not requested.');

  test('records an approved ESP32 portal tour', async ({ browser, request }) => {
    const context = await browser.newContext({
      viewport: { width: 720, height: 900 },
      recordVideo: {
        dir: mediaPath('raw'),
        size: { width: 720, height: 900 },
      },
    });
    const page = await context.newPage();
    const errors = [];
    page.on('pageerror', (error) => errors.push(error.message));
    page.on('console', (message) => {
      if (message.type() === 'error') errors.push(message.text());
    });

    const video = page.video();
    await page.goto('/', { waitUntil: 'networkidle' });
    await expect(page.locator('#wm-reset-portal-timeout')).toBeVisible();
    // These pauses exist only in the README recording. The ordinary contract
    // remains timing-focused; this tour needs readable stable states.
    await page.waitForTimeout(1200);
    await page.screenshot({ path: mediaPath('portal-overview.png'), fullPage: true });

    await page.locator('a[href="#/wifi"]').click();
    await expect(page.locator('#wm-refresh-scan')).toBeVisible();
    await waitForCompletedScan(request);
    await expect(page.locator('#wm-scan-results .wm-scan-row').first()).toBeVisible();
    await page.locator('#wm-f-installation_label').fill('Workshop sensor');
    await page.waitForTimeout(1200);
    await page.screenshot({ path: mediaPath('portal-wifi-settings.png'), fullPage: true });

    await page.locator('#wm-refresh-scan').click();
    await expect(page.locator('#wm-wifi-scan-overlay')).toBeVisible();
    await page.waitForTimeout(1200);
    await context.close();

    const source = await video.path();
    fs.renameSync(source, mediaPath('raw', 'portal-tour.webm'));
    expect(errors).toEqual([]);
  });
});
