# FirmUp

A small OTA updater for ESP32 that checks a remote server for new firmware, verifies it with SHA-256, and applies the update using the ESP32 Update API. It keeps state through `Preferences` and uses simple JSON metadata from the server.

## Install

Add this to your `platformio.ini`:

```ini
lib_deps =
    rootcypher/FirmUp@^1.0.0
```

## Basic Use

```c++
#include <FirmUp.h>

FirmUp ota(
    "MyWiFi",
    "MyPass",
    "http://server-ip:5000"
);

void setup() {
    ota.begin("v0");
}

void loop() {
    ota.loop(30000);
}
    ota.loop(30000);
}
```
## Server Layout

/latest

Returns JSON with version and SHA-256.

Example:
```
{
    "version": "v1",
    "sha256": "c2f1a1e9b5..."
}
```
/firmware/firmware-vX.bin
Binary image that matches the hash in the metadata.
Notes
1. Versions are stored in Preferences.
2. The library validates the firmware hash before flashing.
3. Designed for the ESP32 using the Arduino framework.
4. Firmware is written in chunks and verified before activation.
