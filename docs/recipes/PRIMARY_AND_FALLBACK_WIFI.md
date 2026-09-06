# Primary and fallback Wi-Fi

Some deployed devices receive a primary Wi-Fi network and an optional fallback from a local provisioning source. The consuming framework uses WiFiManager's fixed two-profile controller so it can verify a candidate connection before replacing durable known-good profiles.

## Flow

1. The application obtains a complete candidate profile set.
2. It gives the candidate to startStationCandidate().
3. WiFiManager attempts the primary profile and then fallback when needed.
4. On a usable station connection, WiFiManager saves the candidate through the application-owned store.
5. If connection or storage fails, the application can report the result without silently replacing known-good data.

~~~cpp
WiFiManagerStationProfiles candidate = makeDeploymentProfiles();

wifi.setStationProfileStore(&profileStore);
wifi.setStationRecoveryInterval(30000);

if (!wifi.startStationCandidate(candidate, "Device Setup", "setup-password")) {
    reportRejectedProfileSet();
}
~~~

A candidate needs a non-empty enabled primary slot. The fallback slot is optional. WiFiManager never exposes profile passwords through the portal metadata API.

## Treat persistence result as part of success

A candidate can connect successfully while the profile store fails to write. Read getStationStatus() when WM_EVENT_STATION_PROFILE_CONNECTED arrives:

~~~cpp
wifi.setEventCallback([](WiFiManager::wm_event_t event) {
    if (event != WiFiManager::WM_EVENT_STATION_PROFILE_CONNECTED) {
        return;
    }

    const auto& status = wifi.getStationStatus();
    if (status.lastConnectionWasCandidate && status.storageSaveFailed) {
        reportConnectedButNotRetained();
    }
});
~~~

This distinction matters in unattended devices: the device works now, but may fail to reconnect after a restart.

## Bootstrap and reconcile are application decisions

WiFiManager verifies and selects station profiles. It does not define whether a product should accept a supplied profile only on first boot, replace values after an explicit revision, or merge settings from another system. Keep that policy in the application, alongside its durable configuration and schema migration.

See [Station profiles](../STATION_PROFILES.md) for store requirements, portal fields, and the complete controller lifecycle.
