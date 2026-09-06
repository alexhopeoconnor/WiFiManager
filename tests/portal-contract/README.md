# Portal contract container

This directory contains the browser/API half of the real-hardware portal test.
Run it through [`../../tools/portal-hardware`](../../tools/portal-hardware), not
directly: the host command alone selects the serial board and attaches the
explicit secondary Wi-Fi adapter. Docker uses the host network only to reach
the already-routed `192.168.4.1` portal; it never manages host Wi-Fi.

The image pins the Playwright package to the matching official browser image.
Artifacts, traces, screenshots, JSON results, and the HTML report are written
to the output directory printed by the host command.
