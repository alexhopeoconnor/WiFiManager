/**
 * Fragments.h
 * Reusable DFTE fragment templates for WiFiManager page composition.
 */

#ifndef _WM_FRAGMENTS_TEMPLATE_H_
#define _WM_FRAGMENTS_TEMPLATE_H_

#include <Arduino.h>

const char WM_ACTION_FORM_TEMPLATE[] PROGMEM =
"%ACTION_PREFIX%<form action='%ACTION%' method='%METHOD%'><button%BUTTON_CLASS_ATTR%>%BUTTON_LABEL%</button></form>%ACTION_SUFFIX%";

const char WM_SUBMIT_BUTTON_TEMPLATE[] PROGMEM =
"<br/><br/><button type='submit'>%BUTTON_LABEL%</button>";

const char WM_CENTERED_BUTTON_TEMPLATE[] PROGMEM =
"<br/><div class='c'><button id='%BUTTON_ID%' type='%BUTTON_TYPE%' onclick='%BUTTON_ONCLICK%'%BUTTON_EXTRA_ATTRS%>%BUTTON_LABEL%</button></div>";

const char WM_SECTION_BREAK_TEMPLATE[] PROGMEM =
"<hr><br/>%SECTION_CONTENT%";

const char WM_SCAN_MESSAGE_TEMPLATE[] PROGMEM =
"%SCAN_MESSAGE%<br/><br/>";

const char WM_SCAN_RESULT_ROW_TEMPLATE[] PROGMEM =
"<div><a href='#p' onclick='c(this)' data-ssid='%SSID_ATTR%'>%SSID_TEXT%</a>"
"<div role='img' aria-label='%QUALITY_LABEL%' title='%QUALITY_LABEL%' class='q q-%QUALITY_ICON% %LOCK_CLASS% %ICON_VISIBILITY_CLASS%'></div>"
"<div class='q %VALUE_VISIBILITY_CLASS%'>%QUALITY_VALUE%</div></div>";

const char WM_FIELD_LABEL_BEFORE_TEMPLATE[] PROGMEM =
"<label for='%FIELD_ID%'>%FIELD_LABEL%</label><br/><input id='%FIELD_ID%' name='%FIELD_NAME%' maxlength='%FIELD_MAXLENGTH%' value='%FIELD_VALUE%'%FIELD_EXTRA_ATTRS%>";

const char WM_FIELD_LABEL_AFTER_TEMPLATE[] PROGMEM =
"<br/><input id='%FIELD_ID%' name='%FIELD_NAME%' maxlength='%FIELD_MAXLENGTH%' value='%FIELD_VALUE%'%FIELD_EXTRA_ATTRS%>"
"<label for='%FIELD_ID%'>%FIELD_LABEL%</label>";

const char WM_FIELD_INPUT_ONLY_TEMPLATE[] PROGMEM =
"<br/><input id='%FIELD_ID%' name='%FIELD_NAME%' maxlength='%FIELD_MAXLENGTH%' value='%FIELD_VALUE%'%FIELD_EXTRA_ATTRS%>";

const char WM_INFO_ROW_TEMPLATE[] PROGMEM =
"<dt>%INFO_LABEL%</dt><dd>%INFO_VALUE%</dd>";

const char WM_INFO_SECTION_TEMPLATE[] PROGMEM =
"%SECTION_PREFIX%<h3>%SECTION_TITLE%</h3><hr><dl>%SECTION_ROWS%</dl>%SECTION_SUFFIX%";

const char WM_STATUS_MESSAGE_TEMPLATE[] PROGMEM =
"<div class='msg%STATUS_CLASS_SUFFIX%'><strong>%STATUS_TITLE%</strong>%STATUS_BODY%</div>";

const char WM_PAGE_HEADING_TEMPLATE[] PROGMEM =
"<h1>%HEADER_TITLE%</h1><h3>%HEADER_SUBTITLE%</h3>";

#endif // _WM_FRAGMENTS_TEMPLATE_H_
