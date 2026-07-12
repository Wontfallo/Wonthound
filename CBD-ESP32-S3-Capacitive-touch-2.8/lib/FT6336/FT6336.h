#ifndef FT6336_H
#define FT6336_H

#include <Arduino.h>
#include <Wire.h>

#define FT6336_ADDR  (uint8_t)0x38

#define ROTATION_LEFT       (uint8_t)3
#define ROTATION_INVERTED   (uint8_t)2
#define ROTATION_RIGHT      (uint8_t)1
#define ROTATION_NORMAL     (uint8_t)0

#define FT6336_TD_STATUS          (uint8_t)0x02
#define FT6336_TOUCH_1            (uint8_t)0x03
#define FT6336_ID_G_CIPHER_MID    (uint8_t)0x9F
#define FT6336_ID_G_CIPHER_HIGH   (uint8_t)0xA3
#define FT6336_ID_G_FOCALTECH_ID  (uint8_t)0xA8

class TP_Point {
  public:
    TP_Point(void);
    TP_Point(uint8_t id, uint16_t x, uint16_t y, uint16_t size);

    bool operator==(TP_Point);
    bool operator!=(TP_Point);

    uint8_t id;
    uint16_t x;
    uint16_t y;
    uint8_t size;
};

class FT6336 {
  public:
    FT6336(uint8_t sda, uint8_t scl, uint8_t intPin, uint8_t rstPin,
           uint16_t width, uint16_t height);
    void begin(uint8_t addr = FT6336_ADDR);
    uint8_t reset();
    void setRotation(uint8_t rot);
    void read(void);

    uint8_t touches = 0;
    bool isTouched = false;
    TP_Point points[2];

  private:
    TP_Point readPoint(uint8_t *data);
    uint8_t readByteData(uint8_t reg);
    void readBlockData(uint8_t *buf, uint8_t reg, uint8_t size);

    uint8_t rotation = ROTATION_NORMAL;
    uint8_t address = FT6336_ADDR;
    uint8_t pinSda;
    uint8_t pinScl;
    uint8_t pinInt;
    uint8_t pinRst;
    uint16_t width;
    uint16_t height;
};

#endif
