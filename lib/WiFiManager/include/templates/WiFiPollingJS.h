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
let isPolling = false;

function refreshScan() {
  // Trigger scan via refresh parameter
  fetch('/wifi?refresh=1', {method: 'GET'}).then(() => {
    startPolling();
  });
}

function startPolling() {
  if(isPolling) return;
  isPolling = true;
  document.getElementById('refresh-btn').disabled = true;
  document.getElementById('refresh-btn').textContent = 'Scanning...';
  updateScanStatus();
  scanPollInterval = setInterval(updateScanStatus, 1000);
}

function stopPolling() {
  if(scanPollInterval) {
    clearInterval(scanPollInterval);
    scanPollInterval = null;
  }
  isPolling = false;
  document.getElementById('refresh-btn').disabled = false;
  document.getElementById('refresh-btn').textContent = 'Refresh';
}

function updateScanStatus() {
  fetch('/wifistatus')
    .then(response => response.json())
    .then(data => {
      if(data.scanning) {
        // Still scanning, show status
        document.getElementById('scan-results').innerHTML = 'Scanning for networks...<br/><br/>';
      } else {
        // Scan complete, update network list
        stopPolling();
        updateNetworkList(data);
      }
    })
    .catch(error => {
      console.error('Error polling scan status:', error);
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
      // Map quality 0-100 to icon level 1-4
      let qualityIcon = Math.round((network.quality / 100) * 3) + 1;
      if(qualityIcon < 1) qualityIcon = 1;
      if(qualityIcon > 4) qualityIcon = 4;
      // Encryption class: 'l' if encrypted, empty if open
      let encClass = network.enc_type !== 0 ? 'l' : '';
      let ssidEscaped = escapeHtml(network.ssid);
      // Match original template structure: icon div with quality + encryption class, then percentage div
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

function getQualityIcon(quality) {
  if(quality >= 75) return '▂▄▆█';
  if(quality >= 50) return '▂▄▆▁';
  if(quality >= 25) return '▂▄▁▁';
  return '▂▁▁▁';
}

// Check if scan is in progress on page load
window.addEventListener('load', function() {
  fetch('/wifistatus')
    .then(response => response.json())
    .then(data => {
      if(data.scanning) {
        startPolling();
      }
    });
});
</script>
)rawliteral";

#endif // _WIFI_POLLING_JS_H_

