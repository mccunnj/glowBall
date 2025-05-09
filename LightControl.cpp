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

  //Update Color Wheel

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
  float fr = r / 255.0f, fg = g / 255.0f, fb = b / 255.0f;
  float max = std::max({fr, fg, fb});
  float min = std::min({fr, fg, fb});
  float delta = max - min;

  float hue = 0.0f;
  if (delta > 0.0001f) {
    if (max == fr) hue = 60.0f * fmod((fg - fb) / delta, 6.0f);
    else if (max == fg) hue = 60.0f * (((fb - fr) / delta) + 2.0f);
    else hue = 60.0f * (((fr - fg) / delta) + 4.0f);
    if (hue < 0) hue += 360.0f;
  }

  h = static_cast<uint16_t>(round(hue));
  if (h >= 360) h -= 360;

  s = (max == 0) ? 0 : static_cast<uint8_t>(round((delta / max) * 255.0f));
  v = static_cast<uint8_t>(round(max * 255.0f));
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
  // Determine the new position of the lead LED based on direction
  if (direction > 0) {
    // Move forward (increase the index)
    leadLed = (leadLed + 1) % LED_COUNT;  // Wrap around after reaching the last LED
  } else {
    // Move backward (decrease the index)
    leadLed = (leadLed - 1 + LED_COUNT) % LED_COUNT;  // Wrap around after reaching the first LED
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
// --- Helper Functions for fadeChaser ---
float lerpHue(float a, float b, float t) {
    float delta = fmodf(b - a + 540.0f, 360.0f) - 180.0f;
    float result = fmodf(a + delta * t + 360.0f, 360.0f);
    return result;
}

inline uint16_t degreesToHue16(float hueDegrees) {
  // Normalize to 0–360, then shift so red is centered at 32768
  hueDegrees = fmodf(hueDegrees + 360.0f, 360.0f);  // wrap into 0–360
  return (uint16_t)((hueDegrees / 360.0f) * 65535.0f);  // Note: not 65536
}

void LightControl::populateRing(int8_t direction) {
  // 1. Collect active (non-black) colors
  int8_t colorCount = 0;
    
  if (primaryVal != 0) activeColors[colorCount++] = {primaryHue, primarySat, primaryVal};
  if (secondaryVal != 0) activeColors[colorCount++] = {secondaryHue, secondarySat, secondaryVal};
  if (tertiaryVal != 0) activeColors[colorCount++] = {tertiaryHue, tertiarySat, tertiaryVal};

  for (int i = 0; i < LED_COUNT; i++) {
    // Calculate directon of hue progress around the pixel ring
    uint8_t trailPos;
    if (direction < 0) {
      // Moving forward (direction = 1)
      trailPos = (LED_COUNT - i) % LED_COUNT;
    } else {
      // Moving backward (direction = -1)
      trailPos = (LED_COUNT + i) % LED_COUNT;
    }
    float position = (float)trailPos / LED_COUNT;  // [0.0 - 1.0] along strip
    float fadeFactor = 1.0 - position;

    switch (colorCount) {
      case 0:
        ringColors[i].h = 0; ringColors[i].s = 0; ringColors[i].v = 0;
        break;

      case 1:
        ringColors[i].h = activeColors[0].h;
        ringColors[i].s = activeColors[0].s;
        ringColors[i].v = activeColors[0].v;
        break;

      case 2: {
        ringColors[i].h = lerpHue(activeColors[0].h, activeColors[1].h, fadeFactor);
        ringColors[i].s = lerp(activeColors[0].s, activeColors[1].s, fadeFactor);
        ringColors[i].v = lerp(activeColors[0].v, activeColors[1].v, fadeFactor);
        break;
      }

      default: { // 3 colors
        float posThird = fadeFactor * 3.0f;
        // Last Third
        if (posThird < 1.0f) {
          ringColors[i].h = lerpHue(activeColors[0].h, activeColors[2].h, posThird);
          ringColors[i].s = lerp(activeColors[0].s, activeColors[2].s, posThird);
          ringColors[i].v = lerp(activeColors[0].v, activeColors[2].v, posThird);
        } 
        // Middle Third
        else if (posThird < 2.0f) {
          float t = 1 - (posThird - 1.0f);
          ringColors[i].h = lerpHue(activeColors[1].h, activeColors[2].h, t);
          ringColors[i].s = lerp(activeColors[1].s, activeColors[2].s, t);
          ringColors[i].v = lerp(activeColors[1].v, activeColors[2].v, t);
        } 
        // First Third
        else if (posThird <= 3.0f){
          float t = 1 - (posThird - 2.0f);
          ringColors[i].h = lerpHue(activeColors[0].h, activeColors[1].h, t);
          ringColors[i].s = lerp(activeColors[0].s, activeColors[1].s, t);
          ringColors[i].v = lerp(activeColors[0].v, activeColors[1].v, t);
        }
        break;
      }

    }
    // Debugging output
    DEBUG_PRINTF("i: %i, trailPos: %d, position: %.2f\n", i, trailPos, position);
    DEBUG_PRINTF("Hue: %i, Sat: %i, Val: %i\n", ringColors[i].h, ringColors[i].s, ringColors[i].v);

  }
}

void LightControl::fadeChaser(uint16_t speed, uint8_t lightBrightC, int8_t direction){
  // Map speed (motor speed) to a speed factor (higher speed = faster advances)
  unsigned long speedFactor = map(speed, 1, 950, 2500/transitionSteps, 900/transitionSteps); // Speed factor (chaseSpeed) based on motor speed
  // Activate LEDs
  strip.setBrightness(255);
  // Chase Speed
  unsigned long now = millis();
  if(now - lastUpdate > speedFactor){
    
    for (int i = 0; i < LED_COUNT; i++) {

      int index1 = (leadLed + LED_COUNT + i ) % LED_COUNT;
      int index2 = (leadLed + LED_COUNT + i + 1) % LED_COUNT;
      if (direction < 0) {
        std::swap(index1, index2);
      }

      HSVColor& c1 = ringColors[index1];
      HSVColor& c2 = ringColors[index2];

      float h = lerpHue(c1.h, c2.h, transitionProgress);
      float s = lerp(c1.s, c2.s, transitionProgress);
      float v = lerp(c1.v, c2.v, transitionProgress);
      
      // // CORRECT floating-point math for brightness
      // float brightnessFactor = (float)lightBrightC / 255.0f;
      // //uint8_t finalVal = constrain((int)(finalBrightness + 0.5f), 0, 255);
      
      // Dynamic color calculation based on HSV
      uint32_t finalColor = strip.ColorHSV(degreesToHue16(h), s, lightBrightC);
      // uint32_t finalColor = strip.ColorHSV(degreesToHue16(ringColors[i].h), ringColors[i].s, lightBrightC);

      if(i == 0){
        DEBUG_PRINTF("Index %d, to %d\n", index1, index2);
        DEBUG_PRINTF("Indexed Hue: %i to %i\n", c1.h, c2.h);
        DEBUG_PRINTF("Transition Progress: %.2f, Hue: %.2f, Sat: %.2f, Val: %.2f \n", transitionProgress, h, s, v);
        
        //finalColor = strip.ColorHSV(degreesToHue16(h), s, lightBrightC);
      }
      strip.setPixelColor(i, finalColor);  // Set the pixel color

    }
    strip.show();
    lastUpdate = now;
    // Progress the transition
    transitionProgress += transitionStep;
    if (transitionProgress >= 1.0f) {
      transitionProgress = 0.0f;
      // Advance the lead pixel position
      leadLed = (leadLed + direction + LED_COUNT) % LED_COUNT;

      // Debugging output
      DEBUG_PRINTF("Lead pixel: %i\n", leadLed);
    }
  }
}