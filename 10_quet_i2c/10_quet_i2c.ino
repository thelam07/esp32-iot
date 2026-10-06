#include <Wire.h>

void setup() {
  Serial.begin(115200);
  delay(1000);
  Wire.begin();
  Serial.println();
  Serial.println("=== Bai 10: Quet I2C ===");
}

void loop() {
  int soThietBi = 0;
  Serial.println("Dang quet tu 0x08 den 0x77 ...");

  for (byte diaChi = 0x08; diaChi <= 0x77; diaChi++) {
    Wire.beginTransmission(diaChi);
    byte loi = Wire.endTransmission();

    if (loi == 0) {
      Serial.print("  -> Tim thay module tai dia chi 0x");
      if (diaChi < 16) Serial.print("0");
      Serial.print(diaChi, HEX);
      Serial.print("   ");
      Serial.println(tenModule(diaChi));
      soThietBi++;
    }
  }

  if (soThietBi == 0) {
    Serial.println("  Khong thay module nao. Kiem tra: SDA->21, SCL->22, VCC, GND.");
  } else {
    Serial.print("  Tong cong: ");
    Serial.print(soThietBi);
    Serial.println(" module.");
  }
  Serial.println();
  delay(5000);
}

const char* tenModule(byte diaChi) {
  switch (diaChi) {
    case 0x3C: case 0x3D: return "(OLED SSD1306)";
    case 0x38:            return "(AHT20 nhiet do + do am)";
    case 0x76: case 0x77: return "(BMP280 ap suat)";
    case 0x23: case 0x5C: return "(BH1750 anh sang)";
    default:              return "(chua biet)";
  }
}
