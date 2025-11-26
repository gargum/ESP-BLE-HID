#ifndef NEOPIXEL_H
#define NEOPIXEL_H

#include <Arduino.h>
#include <driver/rmt_tx.h>

#define NEO_GRB ((1 << 6) | (1 << 4) | (0 << 2) | (2))

#define RGB_AZURE       0x99, 0xF5, 0xFF
#define RGB_BLACK       0x00, 0x00, 0x00
#define RGB_BLUE        0x00, 0x00, 0xFF
#define RGB_CHARTREUSE  0x80, 0xFF, 0x00
#define RGB_CORAL       0xFF, 0x7C, 0x4D
#define RGB_CYAN        0x00, 0xFF, 0xFF
#define RGB_GOLD        0xFF, 0xD9, 0x00
#define RGB_GOLDENROD   0xD9, 0xA5, 0x21
#define RGB_GREEN       0x00, 0xFF, 0x00
#define RGB_MAGENTA     0xFF, 0x00, 0xFF
#define RGB_ORANGE      0xFF, 0x80, 0x00
#define RGB_PINK        0xFF, 0x80, 0xBF
#define RGB_PURPLE      0x7A, 0x00, 0xFF
#define RGB_RED         0xFF, 0x00, 0x00
#define RGB_SPRINGGREEN 0x00, 0xFF, 0x80
#define RGB_TEAL        0x00, 0x80, 0x80
#define RGB_TURQUOISE   0x47, 0x6E, 0x6A
#define RGB_WHITE       0xFF, 0xFF, 0xFF
#define RGB_YELLOW      0xFF, 0xFF, 0x00
#define RGB_BLACK       0x00, 0x00, 0x00
#define RGB_OFF         0x00, 0x00, 0x00

typedef uint16_t neoPixelType;

static const uint8_t PROGMEM _NeoPixelGammaTable[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,
    1,1,1,1,2,2,2,2,2,2,2,2,3,3,3,3,3,3,4,4,4,4,5,5,5,5,5,6,6,6,6,7,
    7,7,8,8,8,9,9,9,10,10,10,11,11,11,12,12,13,13,13,14,14,15,15,16,16,17,17,18,18,19,19,20,
    20,21,21,22,22,23,24,24,25,25,26,27,27,28,29,29,30,31,31,32,33,34,34,35,36,37,38,38,39,40,41,42,
    42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,68,69,70,71,72,73,75,
    76,77,78,80,81,82,84,85,86,88,89,90,92,93,94,96,97,99,100,102,103,105,106,108,109,111,112,114,115,117,119,120,
    122,124,125,127,129,130,132,134,136,137,139,141,143,145,146,148,150,152,154,156,158,160,162,164,166,168,170,172,174,176,178,180,
    182,184,186,188,191,193,195,197,199,202,204,206,209,211,213,215,218,220,223,225,227,230,232,235,237,240,242,245,247,250,252,255
};

class NeoPixel {
public:
  NeoPixel(uint16_t n, int16_t pin = 6, neoPixelType type = NEO_GRB);
  ~NeoPixel();

  bool begin(void);
  void show(void);
  void setPixelColor(uint16_t n, uint32_t c);
  void fill(uint32_t c = 0, uint16_t first = 0, uint16_t count = 0);
  void setBrightness(uint8_t);
  void clear(void);
  
  bool canShow(void) {
    uint32_t now = micros();
    if (endTime > now) endTime = now;
    return (now - endTime) >= 300L;
  }
  
  uint8_t *getPixels(void) const { return pixels; };
  uint8_t getBrightness(void) const { return brightness - 1; };
  int16_t getPin(void) const { return pin; };
  uint16_t numPixels(void) const { return numLEDs; }
  uint32_t getPixelColor(uint16_t n) const;
  
  static uint8_t gamma8(uint8_t x) { return pgm_read_byte(&_NeoPixelGammaTable[x]); }
  static uint32_t Color(uint8_t r, uint8_t g, uint8_t b) { return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b; }
  static uint32_t ColorHSV(uint16_t hue, uint8_t sat = 255, uint8_t val = 255);
  static uint32_t gamma32(uint32_t x);

  void rainbow(uint16_t first_hue = 0, int8_t reps = 1, uint8_t saturation = 255, uint8_t brightness = 255, bool gammify = true);

private:
  void updateType(neoPixelType t);
  void updateLength(uint16_t n);

  bool begun;
  uint16_t numLEDs;
  uint16_t numBytes;
  int16_t pin;
  uint8_t brightness;
  uint8_t *pixels;
  uint8_t rOffset;
  uint8_t gOffset;
  uint8_t bOffset;
  uint8_t wOffset;
  uint32_t endTime;
};

#endif
