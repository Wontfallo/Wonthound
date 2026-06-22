#include "FT6336.h"

FT6336::FT6336(uint8_t sda, uint8_t scl, uint8_t intPin, uint8_t rstPin,
               uint16_t width, uint16_t height)
  : pinSda(sda), pinScl(scl), pinInt(intPin), pinRst(rstPin),
    width(width), height(height) {
}

void FT6336::begin(uint8_t addr) {
  address = addr;
  Wire.begin(pinSda, pinScl);
  if (reset()) {
    Serial.println("[TOUCH] FT6336 probe failed");
  }
}

uint8_t FT6336::reset() {
  uint8_t tmp[2] = {0};

  pinMode(pinInt, INPUT);
  pinMode(pinRst, OUTPUT);
  digitalWrite(pinRst, HIGH);
  delay(20);
  digitalWrite(pinRst, LOW);
  delay(20);
  digitalWrite(pinRst, HIGH);
  delay(300);

  readBlockData(&tmp[0], FT6336_ID_G_FOCALTECH_ID, 1);
  if (tmp[0] != 0x11) return 1;

  readBlockData(tmp, FT6336_ID_G_CIPHER_MID, 2);
  if (tmp[0] != 0x26) return 1;
  if (tmp[1] > 0x02) return 1;

  readBlockData(&tmp[0], FT6336_ID_G_CIPHER_HIGH, 1);
  if (tmp[0] != 0x64) return 1;

  return 0;
}

void FT6336::setRotation(uint8_t rot) {
  rotation = rot;
}

void FT6336::read(void) {
  uint8_t data[4];
  uint8_t pointInfo = readByteData(FT6336_TD_STATUS) & 0x0F;
  touches = pointInfo;
  isTouched = (touches > 0 && touches < 3);

  if (!isTouched) return;

  for (uint8_t i = 0; i < touches; i++) {
    readBlockData(data, FT6336_TOUCH_1 + i * 6, sizeof(data));
    points[i] = readPoint(data);
  }
}

TP_Point FT6336::readPoint(uint8_t *data) {
  uint16_t temp;
  uint8_t id = data[2] >> 4;
  uint16_t x = (uint16_t)((data[0] & 0x0F) << 8) + data[1];
  uint16_t y = (uint16_t)((data[2] & 0x0F) << 8) + data[3];

  switch (rotation) {
    case ROTATION_LEFT:
      temp = x;
      x = height - y;
      y = temp;
      break;
    case ROTATION_INVERTED:
      x = width - x;
      y = height - y;
      break;
    case ROTATION_RIGHT:
      temp = x;
      x = y;
      y = width - temp;
      break;
    case ROTATION_NORMAL:
    default:
      break;
  }

  return TP_Point(id, x, y, 0);
}

uint8_t FT6336::readByteData(uint8_t reg) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(address, (uint8_t)1);
  return Wire.available() ? Wire.read() : 0;
}

void FT6336::readBlockData(uint8_t *buf, uint8_t reg, uint8_t size) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(address, size);
  for (uint8_t i = 0; i < size; i++) {
    buf[i] = Wire.available() ? Wire.read() : 0;
  }
}

TP_Point::TP_Point(void) : id(0), x(0), y(0), size(0) {
}

TP_Point::TP_Point(uint8_t id, uint16_t x, uint16_t y, uint16_t size)
  : id(id), x(x), y(y), size(size) {
}

bool TP_Point::operator==(TP_Point point) {
  return point.x == x && point.y == y && point.size == size;
}

bool TP_Point::operator!=(TP_Point point) {
  return point.x != x || point.y != y || point.size != size;
}
