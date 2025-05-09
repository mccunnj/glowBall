#ifndef LIGHTCONTROL_H
#define LIGHTCONTROL_H

#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

#define LED_PIN     13
#define LED_COUNT   20
#define BRIGHTNESS  255

class LightControl {
public:
  LightControl();  // Constructor
  void setFrontPixel(uint8_t index);
  void begin();  // Initialize NeoPixel strip
  void off();

  void solidSparkle(uint16_t speed, uint8_t lightBrightC);
  void fadeChaser(uint16_t speed, uint8_t lightBrightC, int8_t direction);

  void updateColorsFromHex(const String& hexPrimary, const String& hexSecondary, const String& hexTertiary);

  uint32_t getPrimaryColorRGB() const;
  uint32_t getSecondaryColorRGB() const;
  uint32_t getTertiaryColorRGB() const;

  void getPrimaryColorHSV(uint16_t &h, uint8_t &s, uint8_t &v) const;
  void getSecondaryColorHSV(uint16_t &h, uint8_t &s, uint8_t &v) const;
  void getTertiaryColorHSV(uint16_t &h, uint8_t &s, uint8_t &v) const;

  void populateRing(int8_t direction);

  Adafruit_NeoPixel strip = Adafruit_NeoPixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

private:
  // Cached 32-bit packed RGB values
  uint32_t primaryRGB, secondaryRGB, tertiaryRGB;

  // HSV values
  uint16_t primaryHue, secondaryHue, tertiaryHue;
  uint8_t primarySat, secondarySat, tertiarySat;
  uint8_t primaryVal, secondaryVal, tertiaryVal;

  void hexToRGB(const String &hex, uint8_t &r, uint8_t &g, uint8_t &b);
  void rgbToHSV(uint8_t r, uint8_t g, uint8_t b, uint16_t &h, uint8_t &s, uint8_t &v);
  
  //Animation timer
  unsigned long lastUpdate;
  //uint updateTime;
  void ledAdvance(int8_t direction);
  
  //SolidSparkle
  float targetOffsets[50];  // Brightness offsets for each LED
  float currentBrightness[50];  // Current brightness values for each LED

  //FadeChaser
  uint8_t frontPixelIndex = 0;
  uint8_t leadLed;
  
  struct HSVColor {
    uint16_t h;
    uint8_t s, v;

    HSVColor() : h(0), s(0), v(0) {}
    HSVColor(uint16_t hue, uint8_t sat, uint8_t val) : h(hue), s(sat), v(val) {}
  };
  HSVColor activeColors[3];
  HSVColor ringColors[LED_COUNT];
  float transitionProgress = 0.0f;
  int transitionSteps = 64;
  float transitionStep = 1.0f / transitionSteps; // 30 steps between transitions (adjust as needed)

};

#endif
