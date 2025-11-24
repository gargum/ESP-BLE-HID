#include "NeoPixel.h"

static SemaphoreHandle_t show_mutex = NULL;

void espShow(uint8_t pin, uint8_t *pixels, uint32_t numBytes) {
  static rmt_data_t *led_data = NULL;
  static uint32_t led_data_size = 0;
  static int rmtPin = -1;

  if (!show_mutex || xSemaphoreTake(show_mutex, 50 / portTICK_PERIOD_MS) != pdTRUE) return;

  uint32_t requiredSize = numBytes * 8;
  if (requiredSize > led_data_size) {
    free(led_data);
    led_data = (rmt_data_t *)malloc(requiredSize * sizeof(rmt_data_t));
    led_data_size = led_data ? requiredSize : 0;
  }

  if (led_data && requiredSize <= led_data_size) {
    if (pin != rmtPin) {
      if (rmtPin >= 0) rmtDeinit(rmtPin);
      if (rmtInit(pin, RMT_TX_MODE, RMT_MEM_NUM_BLOCKS_1, 10000000)) {
        rmtPin = pin;
      }
    }

    if (rmtPin >= 0) {
      int i = 0;
      for (int b = 0; b < numBytes; b++) {
        uint8_t byteVal = pixels[b];
        for (int bit = 7; bit >= 0; bit--) {
          if (byteVal & (1 << bit)) {
            led_data[i].level0 = 1;
            led_data[i].duration0 = 8;
            led_data[i].level1 = 0;
            led_data[i].duration1 = 4;
          } else {
            led_data[i].level0 = 1;
            led_data[i].duration0 = 4;
            led_data[i].level1 = 0;
            led_data[i].duration1 = 8;
          }
          i++;
        }
      }
      rmtWrite(pin, led_data, numBytes * 8, RMT_WAIT_FOR_EVER);
    }
  }
  xSemaphoreGive(show_mutex);
}

void espInit() {
  if (!show_mutex) show_mutex = xSemaphoreCreateMutex();
}

NeoPixel::NeoPixel(uint16_t n, int16_t p, neoPixelType t)
  : begun(false), brightness(0), pixels(NULL), endTime(0) {
  updateType(t);
  updateLength(n);
  pin = p;
  espInit();
}

NeoPixel::~NeoPixel() {
  if (pixels) {
    clear();
    show();
    free(pixels);
  }
  if (pin >= 0) pinMode(pin, INPUT);
}

bool NeoPixel::begin(void) {
  if (pin < 0) return false;
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  begun = true;
  return true;
}

void NeoPixel::updateLength(uint16_t n) {
  free(pixels);
  numBytes = n * ((wOffset == rOffset) ? 3 : 4);
  pixels = (uint8_t *)malloc(numBytes);
  if (pixels) {
    memset(pixels, 0, numBytes);
    numLEDs = n;
  } else {
    numLEDs = numBytes = 0;
  }
}

void NeoPixel::updateType(neoPixelType t) {
  wOffset = (t >> 6) & 0b11;
  rOffset = (t >> 4) & 0b11;
  gOffset = (t >> 2) & 0b11;
  bOffset = t & 0b11;
}

void NeoPixel::show(void) {
  if (!pixels) return;
  while (!canShow());
  espShow(pin, pixels, numBytes);
  endTime = micros();
}

void NeoPixel::setPixelColor(uint16_t n, uint32_t c) {
  if (n >= numLEDs) return;
  
  uint8_t r = (uint8_t)(c >> 16), g = (uint8_t)(c >> 8), b = (uint8_t)c;
  if (brightness) {
    r = (r * brightness) >> 8;
    g = (g * brightness) >> 8;
    b = (b * brightness) >> 8;
  }
  
  uint8_t *p = &pixels[n * ((wOffset == rOffset) ? 3 : 4)];
  if (wOffset != rOffset) {
    uint8_t w = (uint8_t)(c >> 24);
    p[wOffset] = brightness ? ((w * brightness) >> 8) : w;
  }
  p[rOffset] = r;
  p[gOffset] = g;
  p[bOffset] = b;
}

void NeoPixel::fill(uint32_t c, uint16_t first, uint16_t count) {
  if (first >= numLEDs) return;
  uint16_t end = (count == 0) ? numLEDs : first + count;
  if (end > numLEDs) end = numLEDs;
  for (uint16_t i = first; i < end; i++) setPixelColor(i, c);
}

uint32_t NeoPixel::ColorHSV(uint16_t hue, uint8_t sat, uint8_t val) {
  hue = (hue * 1530L + 32768) / 65536;
  uint8_t r, g, b;
  
  if (hue < 510) { // Red to Green
    b = 0;
    r = (hue < 255) ? 255 : 510 - hue;
    g = (hue < 255) ? hue : 255;
  } else if (hue < 1020) { // Green to Blue
    r = 0;
    g = (hue < 765) ? 255 : 1020 - hue;
    b = (hue < 765) ? hue - 510 : 255;
  } else { // Blue to Red
    g = 0;
    r = (hue < 1275) ? hue - 1020 : 255;
    b = (hue < 1275) ? 255 : 1530 - hue;
  }

  uint32_t v1 = 1 + val;
  uint16_t s1 = 1 + sat;
  uint8_t s2 = 255 - sat;
  return ((((((r * s1) >> 8) + s2) * v1) & 0xff00) << 8) |
         (((((g * s1) >> 8) + s2) * v1) & 0xff00) |
         (((((b * s1) >> 8) + s2) * v1) >> 8);
}

uint32_t NeoPixel::getPixelColor(uint16_t n) const {
  if (n >= numLEDs) return 0;
  uint8_t *p = &pixels[n * ((wOffset == rOffset) ? 3 : 4)];
  
  if (wOffset == rOffset) {
    if (brightness) {
      return (((uint32_t)(p[rOffset] << 8) / brightness) << 16) |
             (((uint32_t)(p[gOffset] << 8) / brightness) << 8) |
             ((uint32_t)(p[bOffset] << 8) / brightness);
    }
    return ((uint32_t)p[rOffset] << 16) | ((uint32_t)p[gOffset] << 8) | p[bOffset];
  } else {
    if (brightness) {
      return (((uint32_t)(p[wOffset] << 8) / brightness) << 24) |
             (((uint32_t)(p[rOffset] << 8) / brightness) << 16) |
             (((uint32_t)(p[gOffset] << 8) / brightness) << 8) |
             ((uint32_t)(p[bOffset] << 8) / brightness);
    }
    return ((uint32_t)p[wOffset] << 24) | ((uint32_t)p[rOffset] << 16) | ((uint32_t)p[gOffset] << 8) | p[bOffset];
  }
}

void NeoPixel::setBrightness(uint8_t b) {
  uint8_t newBrightness = b + 1;
  if (newBrightness == brightness) return;
  
  uint16_t scale;
  if (brightness == 1) {
    scale = 0;
  } else if (b == 255) {
    scale = 65535 / (brightness - 1);
  } else {
    scale = (((uint16_t)newBrightness << 8) - 1) / (brightness - 1);
  }
  
  for (uint16_t i = 0; i < numBytes; i++) {
    pixels[i] = (pixels[i] * scale) >> 8;
  }
  brightness = newBrightness;
}

void NeoPixel::clear(void) {
  if (pixels) memset(pixels, 0, numBytes);
}

uint32_t NeoPixel::gamma32(uint32_t x) {
  uint8_t *y = (uint8_t *)&x;
  for (uint8_t i = 0; i < 4; i++) y[i] = gamma8(y[i]);
  return x;
}

void NeoPixel::rainbow(uint16_t first_hue, int8_t reps, uint8_t saturation, uint8_t bright, bool gammify) {
  for (uint16_t i = 0; i < numLEDs; i++) {
    uint16_t hue = first_hue + (i * (uint32_t)reps * 65536) / numLEDs;
    uint32_t color = ColorHSV(hue, saturation, bright);
    setPixelColor(i, gammify ? gamma32(color) : color);
  }
}
