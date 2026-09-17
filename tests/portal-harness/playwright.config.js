// Configuration for the browser half of the portal test harness.
const path = require('path');
const { defineConfig } = require('@playwright/test');

const artifactDir = process.env.ARTIFACT_DIR || path.join(__dirname, 'artifacts');
// The optional station-handoff test submits real local Wi-Fi credentials. The
// runner mounts only its generated two-key file, but Playwright traces can
// retain request bodies, so leave no screenshots, video, or trace behind for
// that one opt-in credential-bearing mode.
const hasStationCredentials = Boolean(process.env.PORTAL_STATION_ENV);

module.exports = defineConfig({
  testDir: './tests',
  timeout: 45_000,
  expect: { timeout: 10_000 },
  forbidOnly: !!process.env.CI,
  fullyParallel: false,
  workers: 1,
  outputDir: path.join(artifactDir, 'test-results'),
  reporter: [
    ['list'],
    ['json', { outputFile: path.join(artifactDir, 'report.json') }],
    ['html', { outputFolder: path.join(artifactDir, 'html-report'), open: 'never' }],
  ],
  use: {
    baseURL: process.env.PORTAL_URL || 'http://192.168.4.1',
    screenshot: hasStationCredentials ? 'off' : 'only-on-failure',
    trace: hasStationCredentials ? 'off' : 'retain-on-failure',
    video: hasStationCredentials ? 'off' : 'retain-on-failure',
  },
});
