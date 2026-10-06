#include "DHT.h"

#define DHT_PIN  4
#define DHT_TYPE DHT11

DHT dht(DHT_PIN, DHT_TYPE);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Bat dau doc DHT11...");
  dht.begin();
}

void loop() {
  delay(2000);

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("LOI: khong doc duoc DHT11. Kiem tra lai day va chan GPIO.");
    return;
  }

  Serial.printf("Nhiet do: %.1f C | Do am: %.1f %%\n", t, h);
}
