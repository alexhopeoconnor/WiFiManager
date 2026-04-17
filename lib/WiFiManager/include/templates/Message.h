/**
 * Message.h
 * Streamed message page content template for WiFiManager.
 */

#ifndef _WM_MESSAGE_TEMPLATE_H_
#define _WM_MESSAGE_TEMPLATE_H_

#include <Arduino.h>

const char WM_MESSAGE_CONTENT_TEMPLATE[] PROGMEM =
"%MESSAGE_BODY%"
"%MESSAGE_ACTIONS%";

#endif // _WM_MESSAGE_TEMPLATE_H_
