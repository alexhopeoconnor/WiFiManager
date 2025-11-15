/**
 * JS.h
 * JavaScript code for WiFiManager web interface
 * Client-side functionality for WiFi configuration
 */

#ifndef _JS_TEMPLATES_H_
#define _JS_TEMPLATES_H_

#include <Arduino.h>

const char JS_SCRIPT[] PROGMEM = "<script>function c(l){"
"document.getElementById('s').value=l.getAttribute('data-ssid')||l.innerText||l.textContent;"
"var parent = l.parentElement;"
"var isEncrypted = parent.querySelector('.l') !== null || (l.nextElementSibling && l.nextElementSibling.classList.contains('l'));"
"var pField = document.getElementById('p');"
"if(isEncrypted){pField.removeAttribute('disabled');pField.focus();}else{pField.setAttribute('disabled','');};"
"};"
"function f() {var x = document.getElementById('p');x.type==='password'?x.type='text':x.type='password';}"
"window.addEventListener('load',function(){var pField = document.getElementById('p');pField.removeAttribute('disabled');});"
"</script>"; // @todo add button states, disable on click , show ack , spinner etc

#endif // _JS_TEMPLATES_H_

