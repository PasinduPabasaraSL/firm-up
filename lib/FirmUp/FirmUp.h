#ifndef FIRMUP_H
#define FIRMUP_H

#include <Arduino.h>
#include <Preferences.h>

class FirmUp
{
public:
    FirmUp(const char *ssid,
           const char *pass,
           const char *server,
           const char *ns = "firmup");

    void begin(const String &initialVersion = "v0");
    void loop(unsigned long intervalMs);

private:
    String currentVersion;
    String pendingVersion;

    const char *wifiSsid;
    const char *wifiPass;
    const char *otaServer;
    const char *prefsNS;

    Preferences prefs;
    unsigned long lastCheck = 0;

    void connectWifi();
    void checkForUpdate();
    bool downloadAndUpdate(const String &url,
                           const String &expectedSha,
                           const String &ver);

    String toHex(const uint8_t *buf, size_t len);
};

#endif
