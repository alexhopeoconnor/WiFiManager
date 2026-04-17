/**
 * WiFiPollingJS.h
 * JavaScript code for WiFi network scanning with AJAX polling
 * This is embedded in the WiFi configuration page for async WiFi scanning
 */

#ifndef _WIFI_POLLING_JS_H_
#define _WIFI_POLLING_JS_H_

#include <Arduino.h>

const char PROGMEM WIFI_POLLING_JS[] = R"rawliteral(
<script>
let scanPollInterval = null;
let scanBusy = false;

function setRefreshButtonState(disabled, label) {
  const btn = document.getElementById('refresh-btn');
  if (!btn) return;
  btn.disabled = disabled;
  btn.textContent = label;
}

function showScanMessage(message) {
  const scanResults = document.getElementById('scan-results');
  if (!scanResults) return;
  scanResults.innerHTML = message + '<br/><br/>';
}

async function refreshScan() {
  if (scanBusy) return false;
  scanBusy = true;
  setRefreshButtonState(true, 'Scanning...');

  try {
    const response = await fetch('/wifi/scan', { method: 'POST' });
    if (!response.ok) throw new Error('scan request failed');
    startPolling();
  } catch (error) {
    scanBusy = false;
    setRefreshButtonState(false, 'Refresh');
    showScanMessage('Could not start scan. Try again.');
  }

  return false;
}

function startPolling() {
  if (scanPollInterval) return;
  updateScanStatus();
  scanPollInterval = setInterval(updateScanStatus, 1000);
}

function stopPolling() {
  if (scanPollInterval) {
    clearInterval(scanPollInterval);
    scanPollInterval = null;
  }
  scanBusy = false;
  setRefreshButtonState(false, 'Refresh');
}

function updateScanStatus() {
  fetch('/wifistatus')
    .then(response => response.json())
    .then(data => {
      if (data.scanning) {
        showScanMessage('Scanning for networks...');
        return;
      }

      if (data.error === 'timeout') {
        showScanMessage('Scan timed out. Refresh to try again.');
        stopPolling();
        return;
      }

      if (data.error === 'failed') {
        showScanMessage('Scan failed. Refresh to try again.');
        stopPolling();
        return;
      }

      stopPolling();
      updateNetworkList(data);
    })
    .catch(error => {
      console.error('Error polling scan status:', error);
      showScanMessage('Scan status unavailable.');
      stopPolling();
    });
}

function updateNetworkList(data) {
  let html = '';
  if(data.count === 0) {
    html = 'No networks found. Refresh to scan again.<br/><br/>';
  } else if(data.networks && data.networks.length > 0) {
    data.networks.forEach(function(network) {
      let qualityPercent = network.quality + '%';
      let qualityIcon = Math.round((network.quality / 100) * 3) + 1;
      if(qualityIcon < 1) qualityIcon = 1;
      if(qualityIcon > 4) qualityIcon = 4;
      let encClass = network.encrypted ? 'l' : '';
      let ssidEscaped = escapeHtml(network.ssid);
      html += '<div><a href="#p" onclick="c(this)" data-ssid="' + ssidEscaped + '">' + ssidEscaped + '</a>';
      html += '<div role="img" aria-label="' + qualityPercent + '" title="' + qualityPercent + '" class="q q-' + qualityIcon + ' ' + encClass + '"></div>';
      html += '<div class="q">' + qualityPercent + '</div>';
      html += '</div>';
    });
  }
  document.getElementById('scan-results').innerHTML = html;
}

function escapeHtml(text) {
  var map = {};
  map['&'] = '&amp;';
  map['<'] = '&lt;';
  map['>'] = '&gt;';
  map['"'] = '&quot;';
  map["'"] = '&#039;';
  return text.replace(/[&<>"']/g, function(m) { return map[m]; });
}

window.addEventListener('load', function() {
  const refreshButton = document.getElementById('refresh-btn');
  const skipInitialScan = refreshButton && refreshButton.dataset.skipInitialScan === 'true';
  const pField = document.getElementById('p');
  if (pField) {
    pField.removeAttribute('disabled');
  }

  if (skipInitialScan) {
    return;
  }

  fetch('/wifistatus')
    .then(response => response.json())
    .then(data => {
      if (data.scanning) {
        scanBusy = true;
        setRefreshButtonState(true, 'Scanning...');
        startPolling();
        return;
      }

      if (data.error === 'timeout') {
        showScanMessage('Scan timed out. Refresh to try again.');
        return;
      }

      if (data.error === 'failed') {
        showScanMessage('Scan failed. Refresh to try again.');
        return;
      }

      updateNetworkList(data);
    })
    .catch(error => {
      console.error('Error loading initial scan status:', error);
      showScanMessage('Scan status unavailable.');
    });
});
</script>
)rawliteral";

#endif // _WIFI_POLLING_JS_H_

