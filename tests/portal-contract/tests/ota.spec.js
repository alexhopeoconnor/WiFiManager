const { test, expect } = require('@playwright/test');

const firmware = process.env.PORTAL_OTA_FIRMWARE;
const initialMarker = process.env.PORTAL_OTA_INITIAL_MARKER || 'A';
const expectedMarker = process.env.PORTAL_OTA_EXPECTED_MARKER || 'B';

const sleep = (milliseconds) => new Promise((resolve) => setTimeout(resolve, milliseconds));

async function getMarker(request) {
  const response = await request.get('/api/test/firmware-marker', { timeout: 4_000 });
  if (!response.ok()) {
    throw new Error(`marker endpoint returned HTTP ${response.status()}`);
  }
  return response.json();
}

async function waitForMarker(request, expected, timeout = 75_000) {
  let observed;
  await expect.poll(async () => {
    try {
      observed = await getMarker(request);
      return observed.marker;
    } catch {
      return undefined;
    }
  }, {
    timeout,
    intervals: [250, 500, 1_000, 1_000],
  }).toBe(expected);
  return observed;
}

async function requireRestartOutage(request) {
  const deadline = Date.now() + 25_000;
  while (Date.now() < deadline) {
    try {
      const response = await request.get('/api/test/firmware-marker', { timeout: 1_000 });
      if (!response.ok()) {
        return;
      }
    } catch {
      return;
    }
    await sleep(150);
  }
  throw new Error('The portal never became unavailable after a successful OTA response.');
}

function waitForOtaResponse(page) {
  // `waitForResponse()` alone waits until the enclosing test timeout when an
  // embedded server resets the upload connection. Treat that as an immediate
  // transport failure so a hardware artifact names the real fault instead of
  // implying that the rendered form never submitted.
  return new Promise((resolve, reject) => {
    const timeout = setTimeout(() => {
      cleanup();
      reject(new Error('Timed out waiting for the portal OTA POST /u response.'));
    }, 90_000);

    const isOtaRequest = (request) => {
      const requestPath = new URL(request.url()).pathname;
      return requestPath === '/u' && request.method() === 'POST';
    };
    const cleanup = () => {
      clearTimeout(timeout);
      page.off('response', onResponse);
      page.off('requestfailed', onRequestFailed);
    };
    const onResponse = (response) => {
      if (!isOtaRequest(response.request())) {
        return;
      }
      cleanup();
      resolve(response);
    };
    const onRequestFailed = (request) => {
      if (!isOtaRequest(request)) {
        return;
      }
      cleanup();
      const failure = request.failure();
      reject(new Error(`Portal OTA POST /u failed before a response: ${failure ? failure.errorText : 'unknown error'}`));
    };

    page.on('response', onResponse);
    page.on('requestfailed', onRequestFailed);
  });
}

test.describe('portal HTTP OTA contract', () => {
  test('uploads B through the rendered portal form, requires automatic reboot, and observes B twice', async ({ page, request }) => {
    test.skip(!firmware, 'OTA firmware is mounted only for portal-hardware ota.');
    // Initial portal availability, an observed outage, and two fresh B
    // responses each have their own bounded waits. Keep the overall budget
    // larger than their sum so a valid slow reassociation is not killed by
    // Playwright before the fixture contract has concluded.
    test.setTimeout(300_000);

    const initial = await waitForMarker(request, initialMarker);
    expect(initial.freeSketchSpace).toEqual(expect.any(Number));
    expect(initial.freeSketchSpace).toBeGreaterThan(0);

    // This deliberately uses the rendered UI and its multipart XHR instead of
    // posting directly to /u. It therefore covers the real file control,
    // submit handling, success JSON, and restart presentation together.
    await page.goto('/#/update', { waitUntil: 'networkidle' });
    const input = page.locator('#wm-ota-file');
    await expect(input).toBeVisible();
    await input.setInputFiles(firmware);

    const updateResponse = waitForOtaResponse(page);
    await page.locator('#wm-ota-form button[type="submit"]').click();

    const response = await updateResponse;
    expect(response.status()).toBe(200);
    await expect(response.json()).resolves.toMatchObject({ ok: true });

    // A manual reset is never issued here. Observing the outage is what proves
    // WiFiManager's Update.end(true) path restarted the board on its own.
    await requireRestartOutage(request);
    const first = await waitForMarker(request, expectedMarker);
    expect(first.freeSketchSpace).toEqual(expect.any(Number));
    await sleep(1_000);
    const second = await waitForMarker(request, expectedMarker);
    expect(second.freeSketchSpace).toEqual(expect.any(Number));
  });
});
