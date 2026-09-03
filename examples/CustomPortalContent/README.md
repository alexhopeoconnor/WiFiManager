# Custom Portal Content

This example keeps WiFiManager’s portal navigation, validation, and captive behaviour, while adding three application-owned pieces of content:

- an editable MQTT broker host field;
- a compact device-information section;
- a callout on the portal overview.

It opens **WiFiManager Content** with password **example-pass** until it has working station credentials. Save the form, then inspect serial output to see the selected broker value.

The parameter object is global because WiFiManager reads it for the lifetime of the portal. In a real application, copy the value into that application’s own validated persistent configuration inside the save callback.

See [Portal UI](../../docs/PORTAL_UI.md) and the shared [example guide](../README.md).
