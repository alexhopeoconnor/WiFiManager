# Station profiles

WiFiManager 3.1.0 adds an opt-in station-profile controller for applications that need a primary Wi-Fi network and one fallback. It is independent of the legacy `autoConnect()` flow: existing WiFiManager consumers do not need to change.

DeviceFramework enables this controller and supplies its CRC-protected storage. A direct WiFiManager consumer supplies its own durable store, or can use the controller only for the current process.

## Lifecycle

A profile set has exactly two fixed slots:

- **Primary** (slot 0) is required whenever the controller is enabled.
- **Fallback** (slot 1) is optional.
- The last successful slot is tried first on the next connection cycle, then the remaining enabled slot.

The controller never treats the ESP SDK's saved single network as an additional source of truth. It begins one bounded connection attempt at a time, moves to the fallback after failure, and retries both profiles after a temporary loss of a previously working connection. If a new device has no valid profiles, it opens the normal configuration portal.

A candidate submitted by the portal is only committed after it connects and receives a usable IP address. A failed candidate leaves the last saved profile set intact.

## Direct WiFiManager use

Implement a small store appropriate to the application. The manager neither allocates nor owns it:

```cpp
class MyProfileStore final : public WiFiManagerStationProfileStore {
public:
    bool load(WiFiManagerStationProfiles& profiles) override;
    bool save(const WiFiManagerStationProfiles& profiles) override;
    bool clear() override;
};

WiFiManager wifi;
MyProfileStore profiles;

void setup() {
    wifi.setStationProfileStore(&profiles);
    wifi.setStationRecoveryInterval(30000);
    wifi.startStationConnection("Example Setup", "setup-password");
}

void loop() {
    wifi.process();
}
```

The store must return a complete `WiFiManagerStationProfiles` value. Each enabled profile has a NUL-terminated SSID of at most 32 characters and an optional NUL-terminated password of at most 64 characters. Keep slot 0 enabled; set `hasPassword = false` for an open network.

To stage profiles supplied by another subsystem, use `startStationCandidate(candidate)`. It connects the primary/fallback set first and calls the store only after success. Use `saveStationProfiles(profiles)` only when deliberately saving without a connection check. `clearStationProfiles()` clears the supplied store and disconnects the station.

## Portal contract

In profile mode, the existing Wi-Fi page becomes a two-profile form. It remains driven by the same JSON endpoints:

- `GET /api/wifi/meta` returns `profiles`, `activeSlot`, and controller `state`.
- `POST /api/wifi/save` accepts `s0`/ `p0` for primary and `s1`/ `p1` for fallback. A blank submitted password preserves an existing password; send `clear0` or `clear1` for an intentional open network.
- A normal save verifies the candidate by connecting. Poll `GET /api/wifi/connect-status` for its result.
- Set `stationAction=save` only to store the submitted profiles for a later connection attempt.

The portal requires a non-empty primary SSID. Its API never returns a password; it only reports whether one is set.

See [Portal API](PORTAL_API.md) for the shared connection-status response and [DeviceFramework configuration](https://github.com/alexhopeoconnor/DeviceFramework/blob/main/docs/CONFIGURATION.md) for the local-profile JSON that supplies these slots.

Back to [documentation](README.md) · [project overview](../README.md).
