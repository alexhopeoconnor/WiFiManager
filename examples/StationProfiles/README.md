# Station Profiles

This example gives WiFiManager a small application-owned EEPROM store. The portal accepts a required primary Wi-Fi network and an optional fallback, verifies a submitted network before saving it, then remembers the last successful choice across restarts.

On a blank board, connect to **WiFiManager Profiles** with password **example-pass** and open `http://192.168.4.1/`. Enter a primary network and, if useful, one fallback. After a successful connection, restart the board to confirm that it tries the saved profiles before reopening the portal.

`StoredProfiles` is intentionally simple so the ownership boundary is visible. A production application should add its own record versioning and integrity protection around the application’s complete configuration; WiFiManager only owns network-selection policy.

See [Station profiles](../../docs/STATION_PROFILES.md) and the shared [example guide](../README.md).
