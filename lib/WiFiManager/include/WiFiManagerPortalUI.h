/**
 * WiFiManagerPortalUI.h
 *
 * Deliberate, portal-specific presentation configuration. This header does
 * not depend on DeviceFramework: WiFiManager remains useful on its own.
 */

#ifndef WIFI_MANAGER_PORTAL_UI_H
#define WIFI_MANAGER_PORTAL_UI_H

#include <Arduino.h>

enum class WiFiManagerPortalStorage : uint8_t {
  Ram,
  Progmem,
};

/** Non-owning static text. The caller retains the data for the firmware lifetime. */
struct WiFiManagerPortalText {
  const char* data = nullptr;
  WiFiManagerPortalStorage storage = WiFiManagerPortalStorage::Ram;

  static constexpr WiFiManagerPortalText ram(const char* value) {
    return {value, WiFiManagerPortalStorage::Ram};
  }
  static constexpr WiFiManagerPortalText progmem(const char* value) {
    return {value, WiFiManagerPortalStorage::Progmem};
  }

  bool empty() const { return data == nullptr || length() == 0; }
  size_t length() const {
    return data == nullptr ? 0 : (storage == WiFiManagerPortalStorage::Progmem ? strlen_P(data) : strlen(data));
  }
  char at(size_t index) const {
    return storage == WiFiManagerPortalStorage::Progmem
      ? static_cast<char>(pgm_read_byte(data + index))
      : data[index];
  }
};

/** Optional inline SVG branding asset. It is not a general HTML extension point. */
struct WiFiManagerPortalAsset {
  WiFiManagerPortalText svg;

  static constexpr WiFiManagerPortalAsset svgFromRam(const char* value) {
    return {WiFiManagerPortalText::ram(value)};
  }
  static constexpr WiFiManagerPortalAsset svgFromProgmem(const char* value) {
    return {WiFiManagerPortalText::progmem(value)};
  }

  bool empty() const { return svg.empty(); }
};

/**
 * Semantic colour and shape values for WiFiManager's built-in portal.
 *
 * Leave a value empty to retain the built-in stylesheet value. Values are
 * validated before they are emitted into the portal stylesheet; they are not
 * a general CSS injection mechanism.
 */
struct WiFiManagerPortalTheme {
  WiFiManagerPortalText pageBackground;
  WiFiManagerPortalText surface;
  WiFiManagerPortalText text;
  WiFiManagerPortalText mutedText;
  WiFiManagerPortalText border;
  WiFiManagerPortalText accent;
  WiFiManagerPortalText accentHover;
  WiFiManagerPortalText accentText;
  WiFiManagerPortalText danger;
  WiFiManagerPortalText dangerHover;
  WiFiManagerPortalText success;
  uint8_t cornerRadiusPx = 0;
  uint8_t smallCornerRadiusPx = 0;
};

/**
 * Complete setup-time presentation configuration for WiFiManager's portal.
 *
 * Text and SVG assets are non-owning static data in RAM or PROGMEM. Apply this
 * before starting a portal; asynchronous portal responses use immutable state.
 */
struct WiFiManagerPortalConfig {
  WiFiManagerPortalText title;
  WiFiManagerPortalText identityText;
  WiFiManagerPortalText homeIntro;
  WiFiManagerPortalAsset logo;
  WiFiManagerPortalText logoAltText;
  WiFiManagerPortalTheme theme;
};

#endif  // WIFI_MANAGER_PORTAL_UI_H
