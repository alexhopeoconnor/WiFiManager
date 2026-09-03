# Basic Portal

This is the smallest useful WiFiManager application. It first tries the credentials the ESP platform already knows. If it cannot connect, it opens an access point named **WiFiManager Basic** with password **example-pass**.

1. Build and flash the selected `esp8266` or `esp32` environment.
2. Connect a phone or computer to **WiFiManager Basic**.
3. Open `http://192.168.4.1/` if your captive-portal helper does not open it automatically.
4. Select a network and save it. The board joins that network and the next reboot reconnects without opening the portal.

The access-point password is only an example. Choose a unique, Wi-Fi-valid password for a real product.

See the shared [example guide](../README.md) and [getting started](../../docs/GETTING_STARTED.md).
