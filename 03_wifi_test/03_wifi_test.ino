#include <WiFi.h>
#include "bi_mat.h"

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.printf("Dang ket noi toi: %s\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("DA KET NOI!");
  Serial.print("Dia chi IP cua ESP32: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  delay(5000);
  Serial.printf("Trang thai: %s | RSSI: %d dBm\n",
                WiFi.status() == WL_CONNECTED ? "OK" : "MAT KET NOI",
                WiFi.RSSI());
}
