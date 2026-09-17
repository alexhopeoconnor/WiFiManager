# Portal test-harness container

This directory contains the browser/API half of the real-hardware portal test.
Run it through [`../../tools/portal-hardware`](../../tools/portal-hardware), not
directly: the host command alone selects the serial board and attaches the
explicit secondary Wi-Fi adapter. Docker uses the host network only to reach
the already-routed `192.168.4.1` portal; it never manages host Wi-Fi.

The image pins the Playwright package to the matching official browser image.
Artifacts, traces, screenshots, JSON results, and the HTML report are written
to the output directory printed by the host command.

`compose.ota.yaml` is an overlay used only by `portal-hardware ota`. It mounts
the already-built B firmware read-only and enables the A/B browser test
harness. The ordinary portal test harness never receives a firmware artifact.

`compose.station.yaml` is used only by the opt-in station-handoff command. The
host runner stages only `WIFI_SSID` and `WIFI_PASSWORD` in a mode-600 temporary
file instead of mounting the developer's complete environment file. That mode
disables Playwright screenshots, video, and tracing because request bodies can
contain the local password.
