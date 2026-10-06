#include <WiFi.h>
#include <time.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "DHT.h"
#include "bi_mat.h"

#define RONG 128
#define CAO  64
#define DIA_CHI_OLED 0x3C

#define DHT_PIN  4
#define DHT_TYPE DHT11

Adafruit_SSD1306 man(RONG, CAO, &Wire, -1);
DHT dht(DHT_PIN, DHT_TYPE);

float nhietDo = NAN, doAm = NAN;
unsigned long lanDocCuoi = 0;

void veManHinh() {
  man.clearDisplay();
  man.setTextColor(SSD1306_WHITE);

  man.setTextSize(1);
  man.setCursor(0, 0);
  man.print("Phong ngu");
  man.setCursor(98, 0);
  struct tm gio;
  if (getLocalTime(&gio, 0)) {
    man.printf("%02d:%02d", gio.tm_hour, gio.tm_min);
  } else {
    man.print("--:--");
  }
  man.drawLine(0, 11, RONG - 1, 11, SSD1306_WHITE);

  if (isnan(nhietDo) || isnan(doAm)) {
    man.setTextSize(2);
    man.setCursor(0, 28);
    man.print("Dang do...");
  } else {
    man.setTextSize(3);
    man.setCursor(0, 20);
    man.print(nhietDo, 1);
    int x = man.getCursorX();
    man.drawCircle(x + 3, 22, 2, SSD1306_WHITE);
    man.setTextSize(2);
    man.setCursor(x + 8, 27);
    man.print("C");

    man.setTextSize(2);
    man.setCursor(0, 49);
    man.print(doAm, 0);
    man.print(" %");
    man.setTextSize(1);
    man.setCursor(64, 54);
    man.print(doAm < 40 ? "kho" : doAm > 80 ? "am" : "de chiu");
  }
  man.display();
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  Wire.begin();
  if (!man.begin(SSD1306_SWITCHCAPVCC, DIA_CHI_OLED)) {
    Serial.println("KHONG khoi dong duoc OLED");
    while (true) delay(1000);
  }
  Serial.println("OLED OK");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  configTime(7 * 3600, 0, "pool.ntp.org", "time.google.com");
  veManHinh();
}

void loop() {
  if (millis() - lanDocCuoi >= 2500) {
    lanDocCuoi = millis();
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (!isnan(h) && !isnan(t)) { doAm = h; nhietDo = t; }
    struct tm gio;
    if (getLocalTime(&gio, 0)) Serial.printf("%02d:%02d  ", gio.tm_hour, gio.tm_min);
    Serial.printf("%.1f C | %.0f %%\n", nhietDo, doAm);
  }
  veManHinh();
  delay(500);
}
