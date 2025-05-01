#ifndef LIGHTCONTROL_H
#define LIGHTCONTROL_H

#include <Adafruit_NeoPixel.h>

#define LED_PIN     13
#define LED_COUNT   20
#define BRIGHTNESS  50

class LightControl {
public:
  LightControl();  // Constructor

  void begin();  // Initialize NeoPixel strip
  void colorWipe(uint32_t color, int wait);
  void whiteOverRainbow(int whiteSpeed, int whiteLength);
  void pulseWhite(uint8_t wait);
  void rainbowFade2White(int wait, int rainbowLoops, int whiteLoops);
  /*TODO: 
  Flickering wipe
  Solid
  Solid Sparkle
  2-tone
  */
  Adafruit_NeoPixel strip = Adafruit_NeoPixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

private:
  
};

#endif
