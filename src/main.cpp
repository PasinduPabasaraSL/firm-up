#include <FirmUp.h>

FirmUp ota(
    "MyWiFi",
    "MyPass",
    "http://my-server-ip:5000"
);

void setup() {
    ota.begin("v0");
}

void loop() {
    ota.loop(30000);
}
