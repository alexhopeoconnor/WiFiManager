# Station profiles

WiFiManager adds an opt-in station-profile controller for applications that need a primary Wi-Fi network and one fallback. It is independent of the legacy autoConnect() flow: existing WiFiManager consumers do not need to change.

Profile mode is enabled only when the application supplies a WiFiManagerStationProfileStore. The store may be an in-memory implementation for a temporary session, but durable deployments should provide persistent storage.

## Lifecycle

A profile set has exactly two fixed slots:

- **Primary** (slot 0) is required whenever the controller is enabled.
- **Fallback** (slot 1) is optional.
- The last successful slot is tried first on the next connection cycle, then the remaining enabled slot.

The controller never treats the ESP SDK's saved single network as an additional source of truth. It begins one bounded connection attempt at a time, moves to the fallback after failure, and retries both profiles after a temporary loss of a previously working connection. If a new device has no valid profiles, it opens the normal configuration portal.

A candidate submitted by the portal or another application subsystem is only committed after it connects and receives a usable IP address. A failed candidate leaves the last saved profile set intact.

## Direct WiFiManager use

Implement a small store appropriate to the application. The manager neither allocates nor owns it. This complete in-memory version makes the ownership and return contract visible; use the buildable EEPROM example when the profiles must survive a restart:

```cpp
class MemoryProfileStore final : public WiFiManagerStationProfileStore {
public:
    bool load(WiFiManagerStationProfiles& profiles) override {
        if (!hasProfiles_) return false;  // No saved primary profile: open the portal.
        profiles = profiles_;
        return true;
    }

    bool save(const WiFiManagerStationProfiles& profiles) override {
        profiles_ = profiles;
        hasProfiles_ = true;
        return true;  // A durable store must return false when its write fails.
    }

    bool clear() override {
        profiles_ = {};
        hasProfiles_ = false;
        return true;
    }

private:
    WiFiManagerStationProfiles profiles_{};
    bool hasProfiles_ = false;
};

WiFiManager wifi;
MemoryProfileStore profiles;  // Must outlive WiFiManager's asynchronous connection work.

void setup() {
    wifi.setStationProfileStore(&profiles);
    wifi.setStationRecoveryInterval(30000);
    wifi.startStationConnection("Example Setup", "setup-password");
}

void loop() {
    wifi.process();  // Advances profile retries and serves the fallback portal.
}
```

This memory-only store intentionally loses profiles on restart. The store must return a complete `WiFiManagerStationProfiles` value. Each enabled profile has a NUL-terminated SSID of at most 32 characters and an optional NUL-terminated password of at most 64 characters. Keep slot 0 enabled; set `hasPassword = false` for an open network.

When load() returns false, WiFiManager treats the profile set as unavailable and opens the normal configuration portal. When save() or clear() returns false, getStationStatus().storageSaveFailed is set and the status message explains the failure.

## Verified candidate flow

Use startStationCandidate(candidate) when another application subsystem supplies a complete primary/fallback proposal:

~~~cpp
wifi.setStationProfileStore(&profiles);

if (!wifi.startStationCandidate(candidate, "Example Setup", "setup-password")) {
    reportInvalidProfileCandidate();
}
~~~

WiFiManager tries the candidate in memory, then calls the attached store only after the station connects and has a usable IP address. Inspect getStationStatus() after WM_EVENT_STATION_PROFILE_CONNECTED: lastConnectionWasCandidate tells the application that this was a candidate, and storageSaveFailed distinguishes a usable but non-durable connection.

Use saveStationProfiles(profiles) only when deliberately saving without a connection check. clearStationProfiles() asks the supplied store to clear profiles and disconnects the station only after that clear succeeds.

The [Primary and fallback Wi-Fi recipe](recipes/PRIMARY_AND_FALLBACK_WIFI.md) shows why candidate verification protects known-good data.

## Portal contract

In profile mode, the existing Wi-Fi page becomes a two-profile form. It remains driven by the same local portal endpoints:

- GET /api/wifi/meta returns profiles, activeSlot, and controller state without passwords.
- POST /api/wifi/save accepts s0/p0 for primary and s1/p1 for fallback. A blank submitted password preserves an existing password; send clear0 or clear1 for an intentional open network.
- A normal save verifies the candidate by connecting. The built-in portal polls GET /api/wifi/connect-status for its result.
- stationAction=save stores the submitted profiles for a later connection attempt.

The portal requires a non-empty primary SSID. Its local protocol never returns a password. See [Portal API](PORTAL_API.md) for the shared connection-status response.

## Recovery and troubleshooting

| Symptom | Inspect | Meaning |
| --- | --- | --- |
| Portal starts immediately | getStationStatus().configuredProfiles and message | Store had no valid primary profile, so WiFiManager did not fall back to SDK-owned credentials. |
| Repeated network recovery | state, attemptedSlot, activeSlot, and WM_EVENT_STATION_BACKOFF | The controller is trying configured profiles with the recovery interval. |
| Candidate is connected but lost after reboot | lastConnectionWasCandidate and storageSaveFailed | The candidate connected, but the application store did not retain it. |
| Portal form rejects a save | Local API response/message | Primary profile was missing or SSID/password bounds were invalid. |
| Profiles do not clear | storageSaveFailed and message | The application store rejected clear(); fix its storage error before assuming Wi-Fi was removed. |

The buildable [Station Profiles](../examples/StationProfiles/) example contains a compact EEPROM-backed store for both supported ESP targets.

Back to [documentation](README.md) · [project overview](../README.md).
