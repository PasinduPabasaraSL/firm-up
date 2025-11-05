#include "FirmUp.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>
#include "mbedtls/sha256.h"

FirmUp::FirmUp(const char *ssid,
               const char *pass,
               const char *server,
               const char *ns)
    : wifiSsid(ssid),
      wifiPass(pass),
      otaServer(server),
      prefsNS(ns)
{
}

void FirmUp::begin(const String &initialVersion)
{
    prefs.begin(prefsNS, false);

    String stored = prefs.getString("labota_ver", "");
    pendingVersion = prefs.getString("pending_ver", "");

    if (pendingVersion.length() > 0) {
        currentVersion = pendingVersion;
        prefs.putString("labota_ver", pendingVersion);
        prefs.remove("pending_ver");
    } else if (stored.length() > 0) {
        currentVersion = stored;
    } else {
        currentVersion = initialVersion;
        prefs.putString("labota_ver", initialVersion);
    }

    connectWifi();
}

void FirmUp::loop(unsigned long intervalMs)
{
    unsigned long now = millis();
    if (now - lastCheck >= intervalMs) {
        lastCheck = now;
        checkForUpdate();
    }
}

void FirmUp::connectWifi()
{
    if (WiFi.status() == WL_CONNECTED) return;

    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSsid, wifiPass);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED &&
           millis() - start < 20000) {
        delay(300);
    }
}

String FirmUp::toHex(const uint8_t *buf, size_t len)
{
    const char hex[] = "0123456789abcdef";
    String s;
    s.reserve(len * 2);

    for (size_t i = 0; i < len; i++) {
        s += hex[(buf[i] >> 4) & 0xF];
        s += hex[buf[i] & 0xF];
    }
    return s;
}

void FirmUp::checkForUpdate()
{
    if (WiFi.status() != WL_CONNECTED) {
        connectWifi();
        if (WiFi.status() != WL_CONNECTED) return;
    }

    HTTPClient http;
    String url = String(otaServer) + "/latest";
    http.begin(url);
    http.setTimeout(8000);

    int code = http.GET();
    if (code != 200) {
        http.end();
        return;
    }

    String body = http.getString();
    http.end();

    StaticJsonDocument<256> doc;
    if (deserializeJson(doc, body)) return;

    String ver = doc["version"].as<String>();
    String sha = doc["sha256"].as<String>();

    if (ver.length() < 1 || sha.length() < 1) return;
    if (ver == currentVersion) return;

    String fwUrl = String(otaServer) +
                   "/firmware/firmware-" +
                   ver + ".bin";

    downloadAndUpdate(fwUrl, sha, ver);
}

bool FirmUp::downloadAndUpdate(const String &url,
                               const String &expectedSha,
                               const String &ver)
{
    HTTPClient http;
    http.begin(url);
    http.setTimeout(8000);

    int code = http.GET();
    if (code != 200) {
        http.end();
        return false;
    }

    int len = http.getSize();
    if (len <= 0) {
        http.end();
        return false;
    }

    WiFiClient *stream = http.getStreamPtr();

    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts_ret(&ctx, 0);

    if (!Update.begin(len)) {
        mbedtls_sha256_free(&ctx);
        http.end();
        return false;
    }

    uint8_t buff[1024];
    int remaining = len;

    while (remaining > 0 && http.connected()) {
        if (WiFi.status() != WL_CONNECTED) {
            Update.abort();
            mbedtls_sha256_free(&ctx);
            return false;
        }

        int chunk = remaining > 1024 ? 1024 : remaining;
        int r = stream->read(buff, chunk);

        if (r <= 0) {
            delay(10);
            continue;
        }

        mbedtls_sha256_update_ret(&ctx, buff, r);

        if (Update.write(buff, r) != r) {
            Update.abort();
            mbedtls_sha256_free(&ctx);
            http.end();
            return false;
        }

        remaining -= r;
    }

    unsigned char shaOut[32];
    mbedtls_sha256_finish_ret(&ctx, shaOut);
    mbedtls_sha256_free(&ctx);
    http.end();

    String gotSHA = toHex(shaOut, 32);
    if (gotSHA != expectedSha) {
        Update.abort();
        return false;
    }

    if (!Update.end() || !Update.isFinished()) {
        return false;
    }

    prefs.putString("pending_ver", ver);

    delay(500);
    ESP.restart();
    return true;
}
