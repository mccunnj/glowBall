#include "LightControl.h"
#include "WiFiManager.h"
#include <AccelStepper.h>

#define DETECT_PIN 34  // Define GPIO34 as input
#define LED 2 // Onboard LED

// Create instances of LightControl and WiFiManager
LightControl lights;

// Stepper Setup
AccelStepper stepper (AccelStepper::FULL4WIRE, 19, 5, 18, 17);
// Stepper Control Variables
float stepperSpeed = 200.0;
int stepperDirection = 1;

void setup() {
  Serial.begin(115200);
  delay(1000); // Wait 1 second to ensure serial connection is ready
  Serial.println("ESP32 Serial Monitor is active!");

  pinMode(DETECT_PIN, INPUT);  // Set GPIO34 as an input
  pinMode(LED,OUTPUT);
  int state = digitalRead(DETECT_PIN);  // Read pin state

  if (state == HIGH) {
    Serial.println("Pin 34 is HIGH");
    digitalWrite(LED,HIGH);
  } else {
    Serial.println("Pin 34 is LOW");
    digitalWrite(LED,LOW);
  }

  // Set up the Wi-Fi and start the web server (captive portal)
  //WiFiManager.setupWiFi();

  

  lights.begin();  // Initialize NeoPixel strip
  // Create light control task on Core 1
  xTaskCreatePinnedToCore(
    lightControlTask,        // Task function (calls methods of 'lights')
    "LightControlTask",      // Task name
    10000,                   // Stack size
    NULL,                    // Parameters
    1,                       // Task priority
    NULL,                    // Task handle
    0                        // Pin task to Core 0
  );
}

void loop() {

}
// Task function that uses the 'lights' object
void lightControlTask(void *pvParameters) {
    Serial.print("Task1 running on core ");
    Serial.println(xPortGetCoreID());

    while (true) { // Infinite loop to prevent the task from exiting
        Serial.println("RGB Color Wipe");
        lights.colorWipe(lights.strip.Color(255, 0, 0), 25); // Red
        lights.colorWipe(lights.strip.Color(0, 255, 0), 50); // Green
        lights.colorWipe(lights.strip.Color(0, 0, 255), 100); // Blue

        Serial.println("White over rainbow");
        lights.whiteOverRainbow(75, 5);

        Serial.println("Pulse White");
        lights.pulseWhite(5);

        Serial.println("Rainbow Fade to White");
        lights.rainbowFade2White(3, 3, 1);

        vTaskDelay(1000 / portTICK_PERIOD_MS); // Avoid WDT resets
    }
}