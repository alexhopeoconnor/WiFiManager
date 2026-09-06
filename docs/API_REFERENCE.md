# API reference

This is a task-oriented reference for the supported WiFiManager firmware API. Configure an instance during boot, keep it alive for the application's lifetime, and call process() regularly while WiFiManager may have active work.

For complete end-to-end patterns, use [Provisioning lifecycle](PROVISIONING_LIFECYCLE.md), [Portal content](PORTAL_CONTENT.md), and [Station profiles](STATION_PROFILES.md).

## Lifecycle

| API | Timing and meaning |
| --- | --- |
| autoConnect([apName, apPassword]) | Tries legacy saved station credentials. Returns true if it connected during the call; otherwise starts the enabled portal fallback and returns false. |
| startConfigPortal([apName, apPassword]) | Immediately starts a configuration AP and captive portal. |
| stopConfigPortal() | Immediately stops the configuration portal. |
| startWebPortal() / stopWebPortal() | Starts/stops the portal web server without the configuration-AP flow. |
| process() | Cooperatively services active portal, scan, station-profile, and connection state. Call from loop(); do not treat it as a hard real-time operation or rely on a fixed maximum duration. |
| getConfigPortalSSID() | Returns the current configuration AP name. |
| getConfigPortalActive() / getWebPortalActive() | Reports active configuration or web portal state. |
| setHttpPort(port) | Chooses the portal server port before starting it. |

The application owns any existing HTTP service and must release a shared port before WiFiManager starts its portal.

## Station profiles

| API | Timing and meaning |
| --- | --- |
| setStationProfileStore(store) | Supplies an application-owned durable store; WiFiManager does not own it. |
| startStationConnection([apName, apPassword]) | Loads and begins the stored primary/fallback profile flow. |
| startStationCandidate(candidate[, apName, apPassword]) | Attempts an in-memory profile set; when a store is attached, saves it only after a successful usable connection. |
| saveStationProfiles(profiles) | Deliberately stores a complete profile set without a connection verification attempt. |
| clearStationProfiles() | Clears the store and disconnects the station. |
| setStationRecoveryInterval(milliseconds) | Sets delay before profile recovery attempts after a connection loss. |
| isStationProfileMode(), getStationProfiles(), getStationStatus() | Inspects profile-controller state. |

Profile mode has exactly primary slot 0 and optional fallback slot 1. See [Station profiles](STATION_PROFILES.md) for storage rules and portal fields.

## Application settings and callbacks

| API | Timing and meaning |
| --- | --- |
| portalAddParameter(parameter) | Registers an application-owned WiFiManagerParameter; it must outlive the portal. Returns false if registration fails. |
| portalClearParameters() | Removes registered parameter pointers; it does not delete them. |
| getParameters() / getParametersCount() | Inspects registered parameters. |
| setPreSaveParamsCallback(callback) | Invoked before a parameter-only save. |
| setSaveParamsCallback(callback) | Invoked when parameters save; receives WiFiManagerRequestArgs. |
| setPreSaveConfigCallback(callback) | Invoked before a combined Wi-Fi/config save. |
| setSaveConfigCallback(callback) | Invoked after changed Wi-Fi settings connect successfully, or when break-after-config is enabled. |
| setConfigResetCallback(callback) | Invoked when Wi-Fi settings reset. |
| setAPCallback(callback) / setWebServerCallback(callback) | Invoked after the AP/config portal or web server begins. |
| setConfigPortalTimeoutCallback(callback) | Invoked when a configuration portal times out. |
| setPreOtaUpdateCallback(callback) | Invoked immediately before OTA update handling. |

WiFiManagerRequestArgs provides hasArg(), getArg(), getArgAsInt(), getArgAsFloat(), getArgAsBool(), and count(). It is a callback argument, not durable application configuration.

WiFiManagerParameter constructors accept an ID, label, default value, maximum length, optional custom HTML, and optional label placement. IDs are request field names; keep them stable and simple.

## Portal presentation and policy

| API | Timing and meaning |
| --- | --- |
| setPortalConfig(config) | Applies title, identity text, tagline, SVG logo, and semantic theme. Returns false when a portal is active or the theme is invalid. |
| portalSetPageInfoVisible(), portalSetPageUpdateVisible(), portalSetPageSetupVisible() | Controls built-in page visibility. |
| portalSetActionEraseVisible(), portalSetActionRestartVisible(), portalSetActionExitVisible(), portalSetActionCloseCaptiveVisible(), portalSetActionBackVisible() | Controls built-in action visibility. |
| portalSetLayoutParamsLocation(location) | Selects WiFiPage or SetupPage for custom parameters. |
| portalSetBehaviorCaptivePortalEnabled(), portalSetBehaviorConnectOnSave(), portalSetBehaviorExitAllowed() | Sets portal availability, post-save connection, and exit policy. |
| portalSetBehaviorConnectTimeoutSeconds(), portalSetBehaviorPortalTimeoutSeconds() | Sets connection and portal timeout behavior. |
| portalSetBehaviorAutoReconnect(), portalSetBehaviorApClientCheck(), portalSetBehaviorWebClientCheck() | Sets reconnection and activity/timeout behavior. |
| portalSetFieldPasswordPlaceholderMode(mode) | Selects Hidden, Masked, or Actual password placeholder behavior. |
| portalSetFieldStaticIpVisibility(visibility), portalSetFieldStaticDnsVisibility(visibility) | Selects Hidden, Auto, or Always static-network fields. |
| portalAddInfoSection(), portalClearInfoSections() | Adds/clears copied read-only information sections. |
| portalAddHomeCard(), portalClearHomeCards() | Adds/clears copied overview cards. |

Use [Portal UI and configuration](PORTAL_UI.md) for configuration examples and [Portal content](PORTAL_CONTENT.md) for ownership/persistence rules.

## Connection and network policy

| API | Timing and meaning |
| --- | --- |
| setConfigPortalTimeout(seconds) | Limits configuration portal lifetime; setTimeout() is deprecated alias. |
| setConnectTimeout(seconds), setConnectRetries(count) | Bounds legacy automatic connection attempts. |
| setSaveConnectTimeout(seconds), setSaveConnect(enabled) | Controls and bounds portal save-and-connect behavior. |
| setBreakAfterConfig(enabled) | Exits after a configuration submission even when it did not connect. |
| setEnableConfigPortal(enabled), setDisableConfigPortal(enabled) | Controls autoConnect() fallback/start-stop behavior. |
| setAPClientCheck(enabled), setWebPortalClientCheck(enabled) | Controls portal timeout interaction with AP/web clients. |
| setWiFiAutoReconnect(enabled), setCleanConnect(enabled) | Controls station reconnect and pre-connect disconnect behavior. |
| setHostname(), getWiFiHostname() | Sets/reads the supported target hostname. |
| setWiFiSSIDPrefix(), setWiFiAPChannel(), setWiFiAPHidden() | Configures the temporary setup AP. |
| setAPStaticIPConfig(), setSTAStaticIPConfig() | Configures AP or station static addressing. |
| setCountry(), setMinimumSignalQuality(), setRemoveDuplicateAPs(), setScanDispPerc() | Configures country and scan presentation/filter behavior. |
| setRestorePersistent(enabled) | Controls restoration of the platform Wi-Fi persistence setting. |

See [Network configuration](NETWORK_CONFIGURATION.md) for deployment constraints.

## Scan, state, and diagnostics

| API | Timing and meaning |
| --- | --- |
| requestAsyncScan(forceRefresh) | Requests a non-blocking scan. |
| getScanSnapshot(), getScanRuntimeState(), getScanState() | Returns scan lifecycle state. |
| isScanRunning(), hasValidScanResults() | Reads scan progress and cached-result validity. |
| getScanResults() | Returns WiFiManager-owned cached visible results; do not retain references after a new scan or portal shutdown. |
| getRSSIasQuality(rssi) | Converts RSSI to WiFiManager quality. |
| getLastConxResult(), getWLStatusString(), getModeString() | Formats connection and mode diagnostics. |
| hasEnteredConfigPortal(), getConfigPortalConnectState() | Reads portal-session history and last submission state. |
| isConfigPortalConnectPending(), didConfigPortalConnectSucceed(), didConfigPortalConnectFail() | Reads concise portal connection status. |
| getConfigPortalConnectStatus(), getConfigPortalConnectMessage() | Returns platform status and message for the last portal attempt. |
| setEventCallback(callback) | Receives lifecycle notifications; use getters for detailed state. |
| setLogEnabled(), setLogPrefix(), setLogOutput(), setLogSink(), getLogSink() | Configures log output and optional application-owned sink. |

See [Observability](OBSERVABILITY.md) for event meanings and product feedback.

## Reset and low-level helpers

| API | Meaning |
| --- | --- |
| disconnect() | Disconnects without erasing saved configuration. |
| resetSettings() | Clears legacy Wi-Fi settings. |
| erase([optional]) | Erases Wi-Fi configuration. |
| reboot() | Reboots the target. |
| getWiFiIsSaved(), getWiFiSSID(), getWiFiPass() | Reads legacy saved/current station values; handle credentials carefully. |
| getDefaultAPName() | Returns the default generated AP name. |
| debugSoftAPConfig(), debugPlatformInfo() | Writes diagnostic information. |
| htmlEntities(text[, whitespace]) | Escapes text for WiFiManager HTML rendering. |
| preloadWiFi(ssid, password) | Intended for fixtures or controlled integrations that deliberately skip normal station configuration. |

getServer() and getDNSServer() are exposed for testing/host integration. They are not a supported way for product firmware to add private portal routes or mutate the built-in server.

## Continue

- [Architecture and boundaries](ARCHITECTURE.md)
- [Portal API](PORTAL_API.md)
- [Examples](../examples/README.md)

Back to [documentation](README.md) · [project overview](../README.md).
