const path = require('path');
const { defineConfig } = require('@playwright/test');

const artifactDir = process.env.ARTIFACT_DIR || path.join(__dirname, 'artifacts');

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
    screenshot: 'only-on-failure',
    trace: 'retain-on-failure',
    video: 'retain-on-failure',
  },
});
