#include "LightControl.h"

LightControl::LightControl() {}

void LightControl::begin() {
  strip.begin();
  strip.show();
  strip.setBrightness(BRIGHTNESS);
}


void LightControl::colorWipe(uint32_t color, int wait) {
  for(int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, color);
    strip.show();
    delay(wait);
  }
}

void LightControl::whiteOverRainbow(int whiteSpeed, int whiteLength) {
    if(whiteLength >= strip.numPixels()) whiteLength = strip.numPixels() - 1;
    
    int head = whiteLength - 1;
    int tail = 0;
    int loops = 3;
    int loopNum = 0;
    uint32_t lastTime = millis();
    uint32_t firstPixelHue = 0;
    
    for(;;) {
        for(int i = 0; i < strip.numPixels(); i++) {
            if(((i >= tail) && (i <= head)) || ((tail > head) && ((i >= tail) || (i <= head)))) {
                strip.setPixelColor(i, strip.Color(255, 255, 255));
            } else {
                int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
                strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
            }
        }
        
        strip.show();
        firstPixelHue += 40;
        
        if((millis() - lastTime) > whiteSpeed) {
            if(++head >= strip.numPixels()) {
                head = 0;
                if(++loopNum >= loops) return;
            }
            if(++tail >= strip.numPixels()) {
                tail = 0;
            }
            lastTime = millis();
        }
    }
}

void LightControl::pulseWhite(uint8_t wait) {
    for(int j = 0; j < 256; j++) {
        strip.fill(strip.Color(strip.gamma8(j), strip.gamma8(j), strip.gamma8(j)));
        strip.show();
        delay(wait);
    }
    for(int j = 255; j >= 0; j--) {
        strip.fill(strip.Color(strip.gamma8(j), strip.gamma8(j), strip.gamma8(j)));
        strip.show();
        delay(wait);
    }
}

void LightControl::rainbowFade2White(int wait, int rainbowLoops, int whiteLoops) {
  int fadeVal=0, fadeMax=100;

  // Hue of first pixel runs 'rainbowLoops' complete loops through the color
  // wheel. Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to rainbowLoops*65536, using steps of 256 so we
  // advance around the wheel at a decent clip.
  for(uint32_t firstPixelHue = 0; firstPixelHue < rainbowLoops*65536;
    firstPixelHue += 256) {

    for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...

      // Offset pixel hue by an amount to make one full revolution of the
      // color wheel (range of 65536) along the length of the strip
      // (strip.numPixels() steps):
      uint32_t pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());

      // strip.ColorHSV() can take 1 or 3 arguments: a hue (0 to 65535) or
      // optionally add saturation and value (brightness) (each 0 to 255).
      // Here we're using just the three-argument variant, though the
      // second value (saturation) is a constant 255.
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue, 255,
        255 * fadeVal / fadeMax)));
    }

    strip.show();
    delay(wait);

    if(firstPixelHue < 65536) {                              // First loop,
      if(fadeVal < fadeMax) fadeVal++;                       // fade in
    } else if(firstPixelHue >= ((rainbowLoops-1) * 65536)) { // Last loop,
      if(fadeVal > 0) fadeVal--;                             // fade out
    } else {
      fadeVal = fadeMax; // Interim loop, make sure fade is at max
    }
  }

  for(int k=0; k<whiteLoops; k++) {
    for(int j=0; j<256; j++) { // Ramp up 0 to 255
      // Fill entire strip with white at gamma-corrected brightness level 'j':
      strip.fill(strip.Color(strip.gamma8(j), strip.gamma8(j), strip.gamma8(j)));
      strip.show();
    }
    delay(1000); // Pause 1 second
    for(int j=255; j>=0; j--) { // Ramp down 255 to 0
      strip.fill(strip.Color(strip.gamma8(j), strip.gamma8(j), strip.gamma8(j)));
      strip.show();
    }
  }

  delay(500); // Pause 1/2 second
}
