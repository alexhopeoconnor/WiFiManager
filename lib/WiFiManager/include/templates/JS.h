/**
 * JS.h
 *
 * @author alexhopeoconnor
 * @license MIT
 *
 * JavaScript code for WiFiManager web interface.
 * Client-side functionality for WiFi configuration.
 */

#ifndef _JS_TEMPLATES_H_
#define _JS_TEMPLATES_H_

#include <Arduino.h>

const char JS_SCRIPT[] PROGMEM = R"rawliteral(
<script>
function c(l){
  var ssidField = document.getElementById('s');
  if (!ssidField) return;
  ssidField.value = l.getAttribute('data-ssid') || l.innerText || l.textContent;

  var parent = l.parentElement;
  var isEncrypted = parent.querySelector('.l') !== null || (l.nextElementSibling && l.nextElementSibling.classList.contains('l'));
  var pField = document.getElementById('p');
  if (!pField) return;

  if (isEncrypted) {
    pField.removeAttribute('disabled');
    pField.focus();
  } else {
    pField.setAttribute('disabled', '');
  }
}

function f() {
  var x = document.getElementById('p');
  if (!x) return;
  x.type = x.type === 'password' ? 'text' : 'password';
}

window.addEventListener('load', function() {
  var pField = document.getElementById('p');
  if (pField) {
    pField.removeAttribute('disabled');
  }
});
</script>
)rawliteral";

#endif // _JS_TEMPLATES_H_

