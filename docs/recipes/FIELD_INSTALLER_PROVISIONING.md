# Field installer provisioning

A field-installed controller or sensor often needs more time for setup than a desk-bound demo, but should not host an unattended configuration portal forever. Existing consuming device sketches solve this by choosing a deliberate 15-minute configuration window and exposing clear local feedback while setup is active.

## Flow

1. Boot with the product's normal configuration.
2. Try the stored primary/fallback profile set.
3. If Wi-Fi is unavailable, start the local setup AP and portal.
4. Keep the portal available for the installation window.
5. On a successful connection, let the application start services or reboot according to its own policy.
6. On timeout, return to the product's recovery policy rather than assuming the installer finished.

~~~cpp
constexpr unsigned long kInstallerWindowSeconds = 15 * 60;
MyProfileStore profileStore;

void setup() {
    configureProductPortal();
    wifi.setStationProfileStore(&profileStore);
    wifi.setStationRecoveryInterval(30000);
    wifi.setConfigPortalTimeout(kInstallerWindowSeconds);

    wifi.setAPCallback([](WiFiManager*) {
        setIndicator(IndicatorState::Setup);
        showInstallerInstructions();
    });

    wifi.setConfigPortalTimeoutCallback([] {
        setIndicator(IndicatorState::Offline);
        recordSetupTimeout();
    });

    wifi.startStationConnection(deviceSetupName(), deviceSetupPassword());
}

void loop() {
    wifi.process();
    // Do not start or recreate an application server while the portal owns its port.
}
~~~

The timeout is a product decision. A short interval is appropriate for a user-facing appliance; a longer maintenance window may be justified for a device installed in a cabinet or plant room. Set 0 only when an always-open setup portal is an explicit operational choice.

## What to show an installer

Keep feedback independent of a browser redirect:

- the setup SSID and any required password;
- the device's portal address and non-default port, if configured;
- a distinct setup indicator while the portal is active;
- a distinct failure/offline indication after timeout;
- confirmation only after Wi-Fi has actually connected.

Use getConfigPortalActive(), didConfigPortalConnectSucceed(), and getConfigPortalConnectMessage() for this state. See [Observability](../OBSERVABILITY.md).

## Product boundary

WiFiManager supplies the local portal and timer. The product decides what happens after timeout: keep retrying profiles, sleep, wait for a physical action, or operate in an offline mode. If startStationConnection() returns false because no usable stored profile exists, the portal may be the intended next state rather than a reason to reboot immediately.

For a product that deliberately uses one platform-saved network rather than primary/fallback profiles, the smaller legacy equivalent is wifi.autoConnect(deviceSetupName(), deviceSetupPassword()). It has the same local portal fallback, but no application-owned profile store, candidate verification, or fallback network.

Continue with [Provisioning lifecycle](../PROVISIONING_LIFECYCLE.md) or [Provisioning state feedback](PROVISIONING_STATE_FEEDBACK.md).
