#include <Arduino.h>
#include <Wire.h>

#define TOUCH_CS_PIN (16)

#define TOUCH_ADDR (0x38)

bool reg_read(uint8_t addr, uint8_t *data, size_t len) {
  Wire1.beginTransmission(TOUCH_ADDR);
  Wire1.write(addr);
  Wire1.endTransmission(false);

  int n = Wire1.requestFrom(TOUCH_ADDR, len);
  if (n != len) {
    Serial.printf("read fail code %d\n", n);
    return false;
  }
  Wire1.readBytes(data, len);

  return true;
}
void setup() {
  pinMode(TOUCH_CS_PIN, OUTPUT);
  digitalWrite(TOUCH_CS_PIN, HIGH);

  Serial.begin(115200);

  Wire1.begin(23, 19, 100E3);
}

void loop() {
  uint8_t touch_point = 0;
  if (reg_read(0x02, &touch_point, 1)) {
    if (touch_point > 0) {
      struct {
        uint8_t XH;
        uint8_t XL;
        uint8_t YH;
        uint8_t YL;
      } P1;
      if (reg_read(0x03, (uint8_t*) &P1, sizeof(P1))) {
        uint16_t x = ((uint16_t)(P1.XH & 0x0F) << 8) | P1.XL;
        uint16_t y = ((uint16_t)(P1.YH & 0x0F) << 8) | P1.YL;
        Serial.printf("Touch %d, %d\n", x, y);
      } else {
        Serial.println("read point 1 error");
      }
    }
  } else {
    Serial.println("read point count error");
  }
  delay(10);  // wait 5 seconds for next scan
}
