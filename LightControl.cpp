#include "LightControl.h"
#include "DebugMacros.h"

LightControl::LightControl() {
  lastUpdate = 0;
}

void LightControl::begin() {
  strip.begin();
  strip.show();
  strip.setBrightness(BRIGHTNESS);
  lastUpdate = millis();
}

void LightControl::off(){
  strip.setBrightness(0);
  strip.show();
}

void LightControl::updateColorsFromHex(const String& hexPrimary, const String& hexSecondary, const String& hexTertiary) {
  uint8_t r, g, b;

  // Primary
  hexToRGB(hexPrimary, r, g, b);
  primaryRGB = strip.Color(r, g, b);
  rgbToHSV(r, g, b, primaryHue, primarySat, primaryVal);

  // Secondary
  hexToRGB(hexSecondary, r, g, b);
  secondaryRGB = strip.Color(r, g, b);
  rgbToHSV(r, g, b, secondaryHue, secondarySat, secondaryVal);

  // Tertiary
  hexToRGB(hexTertiary, r, g, b);
  tertiaryRGB = strip.Color(r, g, b);
  rgbToHSV(r, g, b, tertiaryHue, tertiarySat, tertiaryVal);
}

void LightControl::hexToRGB(const String &hex, uint8_t &r, uint8_t &g, uint8_t &b) {
  if (hex.length() == 7 && hex.charAt(0) == '#') {
    r = strtol(hex.substring(1, 3).c_str(), NULL, 16);
    g = strtol(hex.substring(3, 5).c_str(), NULL, 16);
    b = strtol(hex.substring(5, 7).c_str(), NULL, 16);
  } else {
    r = g = b = 0; // fallback to black if format is invalid
  }
}

void LightControl::rgbToHSV(uint8_t r, uint8_t g, uint8_t b, uint16_t &h, uint8_t &s, uint8_t &v) {
  float fr = r / 255.0, fg = g / 255.0, fb = b / 255.0;
  float max = std::max({fr, fg, fb});
  float min = std::min({fr, fg, fb});
  float delta = max - min;

  h = 0;
  if (delta > 0.0001f) {
    if (max == fr) h = 60 * fmod(((fg - fb) / delta), 6);
    else if (max == fg) h = 60 * (((fb - fr) / delta) + 2);
    else h = 60 * (((fr - fg) / delta) + 4);
    if (h < 0) h += 360;
  }

  s = (max == 0) ? 0 : (delta / max) * 255;
  v = max * 255;
}

uint32_t LightControl::getPrimaryColorRGB() const { return primaryRGB; }
uint32_t LightControl::getSecondaryColorRGB() const { return secondaryRGB; }
uint32_t LightControl::getTertiaryColorRGB() const { return tertiaryRGB; }

void LightControl::getPrimaryColorHSV(uint16_t &h, uint8_t &s, uint8_t &v) const {
  h = primaryHue; s = primarySat; v = primaryVal;
}
void LightControl::getSecondaryColorHSV(uint16_t &h, uint8_t &s, uint8_t &v) const {
  h = secondaryHue; s = secondarySat; v = secondaryVal;
}
void LightControl::getTertiaryColorHSV(uint16_t &h, uint8_t &s, uint8_t &v) const {
  h = tertiaryHue; s = tertiarySat; v = tertiaryVal;
}

void LightControl::ledAdvance(int8_t direction) {
  static uint8_t tempOrder[LED_COUNT]; // Temporary buffer (only created once)

  if (direction > 0) {
    // Rotate forward: shift everything left, wrap first to end
    for (int i = 0; i < LED_COUNT - 1; ++i) {
      tempOrder[i] = ledOrder[i + 1];
    }
    tempOrder[LED_COUNT - 1] = ledOrder[0];
  } else {
    // Rotate backward: shift everything right, wrap last to front
    for (int i = LED_COUNT - 1; i > 0; --i) {
      tempOrder[i] = ledOrder[i - 1];
    }
    tempOrder[0] = ledOrder[LED_COUNT - 1];
  }

  // Copy temp buffer back to main buffer
  memcpy(ledOrder, tempOrder, LED_COUNT * sizeof(uint8_t));
}

void LightControl::setFrontPixel(uint8_t index) {
  if (index < LED_COUNT) {
    frontPixelIndex = index;
    for (int i = 0; i < LED_COUNT; i++) {
      ledOrder[i] = (frontPixelIndex + i) % LED_COUNT;
    }
  }
}

void LightControl::solidSparkle(uint16_t speed, uint8_t lightBrightC) {
  // Map speed (motor speed) to a speed factor (higher speed = quicker updates)
  int speedFactor = map(speed, 0, 950, 100, 10); // Speed factor (refresh rate) based on motor speed

  strip.setBrightness(255);
  // Normalize brightness to a range between 0 and 1
  float baseBrightness = lightBrightC / 255.0; // 0-255 scale to 0-1 scale

  // Randomization frequency (avoid excessive twinkles at low speeds)
  unsigned long now = millis();

  // Initialize currentBrightness and targetOffsets if it's the first run
  static bool firstRun = true;
  if (firstRun) {
    // Initialize currentBrightness and targetOffsets with baseBrightness
    for (int i = 0; i < LED_COUNT; i++) {
      currentBrightness[i] = baseBrightness;
      targetOffsets[i] = baseBrightness;
    }
    firstRun = false;  // Set flag to false after first run
  }

  // Update the animation based on the speed
  if (now - lastUpdate > speedFactor) { // Adjust based on motor speed
    lastUpdate = now;

    // Loop over each LED to apply the sparkle effect
    for (int i = 0; i < LED_COUNT; i++) {
      // Check if current brightness has reached the target, if so, generate a new target offset
      if (abs(currentBrightness[i] - targetOffsets[i]) < 0.01) {  // small threshold to avoid floating-point issues
        // Randomly decide the sparkle effect intensity
        float offsetPercent = 0.0;
        if (random(0, 100) < 15) { 
          offsetPercent = random(20, 33) / 100.0; // Deep sparkle: 25%-75% of baseBrightness
          // Randomly flip the offset direction (positive or negative)
          if (random(0, 2) == 0) {
            offsetPercent = -offsetPercent; // Make it negative with 50% chance
          }
        } else {
          // Standard random offset (±25% around baseBrightness)
          offsetPercent = random(-20, 20) / 100.0; // Random offset between -25% and 25%
        }
        // Constrain targetOffsets to 0-1 range (so brightness won't go negative)
        targetOffsets[i] = constrain(baseBrightness + offsetPercent, 0.0, 1.0); // Apply to base brightness
      }

      // Smoothly transition to the new target brightness
      currentBrightness[i] += (targetOffsets[i] - currentBrightness[i]) * 0.1; // Smooth fading
      currentBrightness[i] = constrain(currentBrightness[i], 0.0, 1.0); // Constrain to 0-1 range
      
      // Optionally debug to see brightness values
      // if (i == 0) {
      //   DEBUG_PRINTF("CurrentBrightness: %.2f, TargetBrightness: %.2f\n", currentBrightness[i], targetOffsets[i]);
      // }

      // Adjust the color based on the current brightness
      uint32_t finalColor = strip.ColorHSV(primaryHue * 182, primarySat, currentBrightness[i] * 255);

      // Set the pixel color
      strip.setPixelColor(i, finalColor);
    }

    strip.show(); // Update the LED strip
  }
}
// --- Helper Functions ---
float lerpHue(float a, float b, float t) {
  float delta = b - a;
  if (delta > 180) delta -= 360;
  if (delta < -180) delta += 360;
  float result = a + delta * t;
  if (result < 0) result += 360;
  if (result >= 360) result -= 360;
  return result;
}

void LightControl::fadeChaser(uint16_t speed, uint8_t lightBrightC, int8_t direction){
  // Map speed (motor speed) to a speed factor (higher speed = faster advances)
  unsigned long speedFactor = map(speed, 1, 950, 10000, 507); // Speed factor (chaseSpeed) based on motor speed
  // Activate LEDs
  strip.setBrightness(255);
  // Chase Speed
  unsigned long now = millis();
  if(now - lastUpdate > speedFactor){
    // 1. Collect active (non-black) colors
    struct HSVColor {
      uint16_t h;
      uint8_t s, v;
    };
    
    HSVColor activeColors[3];
    int8_t colorCount = 0;
    
    if (primaryVal != 0) activeColors[colorCount++] = {primaryHue, primarySat, primaryVal};
    if (secondaryVal != 0) activeColors[colorCount++] = {secondaryHue, secondarySat, secondaryVal};
    if (tertiaryVal != 0) activeColors[colorCount++] = {tertiaryHue, tertiarySat, tertiaryVal};
    
    // Advance the lead pixel position, negative for a better match.
    ledAdvance(-direction);
    DEBUG_PRINT("ledOrder:");
    for (int i = 0; i < LED_COUNT; i++) {
      DEBUG_PRINT(ledOrder[i]);
      DEBUG_PRINT(" ");
    }
    DEBUG_PRINTLN("");

    for (int i = 0; i < LED_COUNT; i++) {
      //DEBUG_PRINTLN(i);
      // Calculate position of the trailing pixel relative to the lead pixel
      int trailPos = (LED_COUNT + ledOrder[0] - i) % LED_COUNT; // how far behind the lead pixel this one is
      float position = (float)trailPos / LED_COUNT;  // [0.0 - 1.0] along strip
      float fadeFactor = 1.0 - position;

      uint16_t hue;
      uint8_t sat;
      uint8_t val;

      switch (colorCount) {
        case 0:
          hue = 0; sat = 0; val = 0;
          break;

        case 1:
          hue = activeColors[0].h;
          sat = activeColors[0].s;
          val = activeColors[0].v;
          break;

        case 2: {
          hue = lerpHue(activeColors[0].h, activeColors[1].h, fadeFactor);
          sat = lerp(activeColors[0].s, activeColors[1].s, fadeFactor);
          val = lerp(activeColors[0].v, activeColors[1].v, fadeFactor);
          break;
        }

        default: { // 3 colors
          float posThird = fadeFactor * 3.0f;
          if (posThird < 1.0f) {
            hue = lerpHue(activeColors[0].h, activeColors[1].h, posThird);
            sat = lerp(activeColors[0].s, activeColors[1].s, posThird);
            val = lerp(activeColors[0].v, activeColors[1].v, posThird);
          } else if (posThird < 2.0f) {
            float t = posThird - 1.0f;
            hue = lerpHue(activeColors[1].h, activeColors[2].h, t);
            sat = lerp(activeColors[1].s, activeColors[2].s, t);
            val = lerp(activeColors[1].v, activeColors[2].v, t);
          } else {
            hue = activeColors[2].h;
            sat = activeColors[2].s;
            val = activeColors[2].v;
          }
          break;
        }
      }

      // CORRECT floating-point math for brightness
      float brightnessFactor = (float)lightBrightC / 255.0f;
      float finalBrightness = val * brightnessFactor * position; 
      uint8_t finalVal = constrain((int)(finalBrightness + 0.5f), 0, 255);
      
      // Dynamic color calculation based on HSV
      uint32_t finalColor = strip.ColorHSV(hue * 182, sat, finalVal);

      DEBUG_PRINTF("i: %i, Lead pixel: %i, trailPos: %d, position: %.2f\n", i, ledOrder[0], trailPos, position);
      DEBUG_PRINTF("Pixel: %i, Hue: %i, Sat: %i, Val: %i\n", ledOrder[i], hue, sat, finalVal);
      DEBUG_PRINTF("val: %i, BrightnessFactor: %.2f, FadeFactor: %.2f, finalBrightness: %.2f, finalVal: %i\n\n", val, brightnessFactor, fadeFactor, finalBrightness, finalVal);

      strip.setPixelColor(ledOrder[i], finalColor);  // Set the pixel color
    }
    strip.show();
    lastUpdate = now;
  }
}