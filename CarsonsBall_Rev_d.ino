#include "LightControl.h"
#include <AccelStepper.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>

#define DETECT_PIN 16  // Define GPIO34 as input
#define LED 2 // Onboard LED
#define motorPin1  25      // IN1 on the ULN2003 driver
#define motorPin2  26     // IN2 on the ULN2003 driver
#define motorPin3  33     // IN3 on the ULN2003 driver
#define motorPin4  32     // IN4 on the ULN2003 driver

#include "DebugMacros.h"

// Create instances of LightControl and preferences
LightControl lights;
// Non-Volotile Flash memory
Preferences preferences;

// Mutex for protecting shared settings
SemaphoreHandle_t settingsMutex;

// Stepper Setup
AccelStepper stepper (AccelStepper::FULL4WIRE, motorPin1, motorPin3, motorPin2, motorPin4);
// Stepper Control Variables
float stepperSpeed = 200.0;
int stepperDirection = 1;

// Global variables for light settings
int lightBright = 0;
int lightMode = 0;
String primaryColor = "#ff0000";
String secondaryColor = "#00ff00";
String tertiaryColor = "#0000ff";

// Web Server
WebServer server(80);
DNSServer dnsServer;
// Old Settings
// const char* ssid = "Carson's Glow Ball";
// const char* password = "12345678";

// Captive portal HTML
const char responsePortal[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Carson's Glow Ball Config</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
  h1 {
  font-size: clamp(1.5em, 5vw, 2em);
  margin-bottom: 20px;
  } 
  body {
    text-align: center;
  }
  form {
    max-width: 600px;
    margin: auto;
  }
  .centered-label {
    text-align: center;
    display: block;
    margin-bottom: 4px;
  }
  input[type="range"] {
    width: 90%;
    display: block;
    box-sizing: border-box;
    margin: 0 auto 16px auto;
  }
  .centered-checkbox {
    margin: 16px 0;
  }
  .centered-checkbox input[type="checkbox"] {
    transform: scale(1.5);
    margin-left: 8px;
  }
  select, input[type="color"], input[type="submit"] {
    display: block;
    margin: 8px auto;
  }
  label {
    display: block;
    margin-top: 12px;
  }
    @media (max-width: 600px) {
    body {
      padding: 10px;
    }

    form {
      width: 100%;
    }

    input[type="range"],
    select,
    input[type="color"],
    input[type="submit"] {
      width: 100%;
    }
  }
  </style>

  <script>
    // Function to handle form submission
    function submitForm(event) {
      event.preventDefault();
      const form = event.target;
      const formData = new FormData(form);

      fetch('/save', {
        method: 'POST',
        body: formData
      }).then(response => response.json())
        .then(data => {
          const btn = document.getElementById('saveButton');
          btn.classList.add('saved');
          btn.value = 'Saved!';
          setTimeout(() => {
            btn.classList.remove('saved');
            btn.value = 'Apply Settings';
          }, 2000);
        });
    }

    // Real-time update function
    window.addEventListener('DOMContentLoaded', (event) => {
      const speedSlider = document.getElementById('speed');
      const reverseCheckbox = document.querySelector('input[name="reverse"]');
      const modeSelect = document.querySelector('select[name="mode"]');
      const brightnessSlider = document.getElementById('brightness');
      const primaryColorInput = document.querySelector('input[name="primary"]');
      const secondaryColorInput = document.querySelector('input[name="secondary"]');
      const tertiaryColorInput = document.querySelector('input[name="tertiary"]');

      // Function to send real-time updates to the ESP32
      function sendRealTimeUpdate() {
        const formData = new FormData();
        formData.append("speed", speedSlider.value);
        formData.append("reverse", reverseCheckbox.checked ? "1" : "0");
        formData.append("mode", modeSelect.value);
        formData.append("brightness", brightnessSlider.value);
        formData.append("primary", primaryColorInput.value);
        formData.append("secondary", secondaryColorInput.value);
        formData.append("tertiary", tertiaryColorInput.value);

        fetch('/update', {
          method: 'POST',
          body: formData
        }).then(response => response.json())
          .then(data => {
            console.log('Real-time update successful:', data);
          }).catch(error => {
            console.error('Error in real-time update:', error);
          });
      }

      // Add event listeners to detect changes and update in real-time
      speedSlider.addEventListener('input', sendRealTimeUpdate);
      reverseCheckbox.addEventListener('change', sendRealTimeUpdate);
      modeSelect.addEventListener('change', sendRealTimeUpdate);
      brightnessSlider.addEventListener('input', sendRealTimeUpdate);
      primaryColorInput.addEventListener('input', sendRealTimeUpdate);
      secondaryColorInput.addEventListener('input', sendRealTimeUpdate);
      tertiaryColorInput.addEventListener('input', sendRealTimeUpdate);
    });
  </script>
</head>
<body>
  <h1>Carson's Glow Ball Settings</h1>
  <form onsubmit="submitForm(event)">
    <label for="speed" class="centered-label">Stepper Speed: <span id="speedValue">0</span></label>
    <input type="range" id="speed" name="speed" min="0" max="950" value="0"
       oninput="document.getElementById('speedValue').innerText = this.value;">

    <div class="centered-checkbox">
      Reverse Direction <input type="checkbox" name="reverse" %REVERSE%>
    </div>

    <label for="mode">Light Mode:</label>
    <select name="mode">
      <option value="0">Off</option>
      <option value="1">Solid Sparkle</option>
      <option value="2">Fade Chaser</option>
      <option value="3">Pulse</option>
      <option value="4">Fade to White</option>
    </select><br>

    <label for="brightness" class="centered-label">Brightness: <span id="brightnessValue">0</span></label>
    <input type="range" id="brightness" name="brightness" min="0" max="255" value="0"
      oninput="document.getElementById('brightnessValue').innerText = this.value;">
    

    <label>Primary Color:</label><input type="color" name="primary"><br>
    <label>Secondary Color:</label><input type="color" name="secondary"><br>
    <label>Tertiary Color:</label><input type="color" name="tertiary"><br><br>

    <input type="submit" id="saveButton" value="Apply Settings">
  </form>
</body>
</html>
)rawliteral";


void handleRoot() {
  server.sendHeader("Location", "/portal");
  server.send(302, "text/plain", "Redirect to captive portal");
}

void handlePortal() {
  // Convert the PROGMEM string into a mutable String
  String html = FPSTR(responsePortal);

  // Inject script for dynamic form prefill
  html += "<script>\n";
  html += "window.addEventListener('DOMContentLoaded', (event) => {\n";
  html += "document.getElementById('speed').value = " + String(stepperSpeed, 1) + ";\n";
  html += "document.getElementById('speedValue').innerText = " + String(stepperSpeed, 1) + ";\n";
  html += "document.querySelector('input[name=\"reverse\"]').checked = " + String(stepperDirection == -1 ? "true" : "false") + ";\n";
  html += "document.querySelector('select[name=\"mode\"]').value = '" + String(lightMode) + "';\n";
  html += "document.getElementById('brightness').value = " + String(lightBright) + ";\n";
  html += "document.getElementById('brightnessValue').innerText = " + String(lightBright) + ";\n";
  html += "document.querySelector('input[name=\"primary\"]').value = '" + primaryColor + "';\n";
  html += "document.querySelector('input[name=\"secondary\"]').value = '" + secondaryColor + "';\n";
  html += "document.querySelector('input[name=\"tertiary\"]').value = '" + tertiaryColor + "';\n";
  html += "});\n";
  html += "</script>\n";

  server.send(200, "text/html", html);
}

void handleSave() {
  preferences.begin("settings", false);
  if (xSemaphoreTake(settingsMutex, portMAX_DELAY)) {
    preferences.putFloat("stepperSpeed", stepperSpeed);
    preferences.putInt("direction", stepperDirection);
    preferences.putInt("brightnessNV", lightBright);
    preferences.putInt("mode", lightMode);
    preferences.putString("primary", primaryColor);
    preferences.putString("secondary", secondaryColor);
    preferences.putString("tertiary", tertiaryColor);
    xSemaphoreGive(settingsMutex);
  }
  DEBUG_PRINTLN("Saved settings:");
  DEBUG_PRINTF("Speed: %.2f, Direction: %d, Mode: %d, Brightness: %d\n",
                preferences.getFloat("stepperSpeed", -1),
                preferences.getInt("direction", 0),
                preferences.getInt("mode", -1),
                preferences.getInt("brightnessNV", -1));
  preferences.end();

  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handleUpdate() {
  if (xSemaphoreTake(settingsMutex, portMAX_DELAY)) {
    if (server.hasArg("speed")) {
      stepperSpeed = server.arg("speed").toFloat();
      DEBUG_PRINTF("Updated stepperSpeed: %.2f\n", stepperSpeed);
    } else {
      DEBUG_PRINTLN("Missing speed parameter");
    }

    if (server.hasArg("reverse")) {
      stepperDirection = server.arg("reverse") == "1" ? -1 : 1;
      DEBUG_PRINTF("Updated stepperDirection: %d\n", stepperDirection);
    } else {
      DEBUG_PRINTLN("Missing reverse parameter");
    }

    if (server.hasArg("mode")) {
      lightMode = server.arg("mode").toInt();
      DEBUG_PRINTF("Updated mode: %d\n", lightMode);
    } else {
      DEBUG_PRINTLN("Missing mode parameter");
    }

    if (server.hasArg("brightness")) {
      lightBright = server.arg("brightness").toInt();
      DEBUG_PRINTF("Updated Brightness: %d\n", lightBright);
    } else {
      DEBUG_PRINTLN("Missing brightness parameter");
    }

    if (server.hasArg("primary")) {
      primaryColor = server.arg("primary");
      DEBUG_PRINTF("Updated primary color: %s\n", primaryColor.c_str());
    } else {
      DEBUG_PRINTLN("Missing primary color parameter");
    }

    if (server.hasArg("secondary")) {
      secondaryColor = server.arg("secondary");
      DEBUG_PRINTF("Updated secondary color: %s\n", secondaryColor.c_str());
    } else {
      DEBUG_PRINTLN("Missing secondary color parameter");
    }

    if (server.hasArg("tertiary")) {
      tertiaryColor = server.arg("tertiary");
      DEBUG_PRINTF("Updated tertiary color: %s\n", tertiaryColor.c_str());
    } else {
      DEBUG_PRINTLN("Missing tertiary color parameter");
    }

    lights.updateColorsFromHex(primaryColor, secondaryColor, tertiaryColor);
    xSemaphoreGive(settingsMutex);
    
  }
  // Respond back to the browser with a success message
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void loadSettings() {
  preferences.begin("settings", true);
  stepperSpeed = preferences.getFloat("stepperSpeed", -1.0);
  stepperDirection = preferences.getInt("direction", 0);
  // Assign light settings to global variables
  lightMode = preferences.getInt("mode", -1);
  lightBright = preferences.getInt("brightnessNV", -1);
  primaryColor = preferences.getString("primary", "#ff0000");
  secondaryColor = preferences.getString("secondary", "#00ff00");
  tertiaryColor = preferences.getString("tertiary", "#0000ff");

  // Debugging output
  DEBUG_PRINTF("Loaded stepperSpeed: %.2f\n", stepperSpeed);
  DEBUG_PRINTF("Loaded stepperDirection: %d\n", stepperDirection);
  DEBUG_PRINTF("Loaded lightMode: %d\n", lightMode);
  DEBUG_PRINTF("Loaded lightBright: %d\n", lightBright);
  DEBUG_PRINTF("Primary Color: %s, Secondary Color: %s, Tertiary Color: %s\n",
    primaryColor.c_str(),
    secondaryColor.c_str(),
    tertiaryColor.c_str());
  
  preferences.end();
}

void setupWiFi() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("Carson's Glow Ball");

  dnsServer.start(53, "*", WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/portal", handlePortal);
  server.on("/update", HTTP_POST, handleUpdate);
  server.on("/save", HTTP_POST, handleSave);
  server.onNotFound(handleRoot);
  server.begin();
}

void printColorData() {
  DEBUG_PRINTLN("=== Color Debug Info ===");

  uint16_t h;
  uint8_t s, v;

  // For Primary Color (HSV and RGB)
  lights.getPrimaryColorHSV(h, s, v);
  DEBUG_PRINT("Primary HSV: ");
  DEBUG_PRINT("H: "); DEBUG_PRINT(h);
  DEBUG_PRINT(" S: "); DEBUG_PRINT(s);
  DEBUG_PRINT(" V: "); DEBUG_PRINTLN(v);

  uint32_t primaryRGB = lights.getPrimaryColorRGB();
  DEBUG_PRINT("Primary RGB: ");
  DEBUG_PRINT("R: "); DEBUG_PRINT((primaryRGB >> 16) & 0xFF);
  DEBUG_PRINT(" G: "); DEBUG_PRINT((primaryRGB >> 8) & 0xFF);
  DEBUG_PRINT(" B: "); DEBUG_PRINTLN(primaryRGB & 0xFF);

  // For Secondary Color (HSV and RGB)
  lights.getSecondaryColorHSV(h, s, v);
  DEBUG_PRINT("Secondary HSV: ");
  DEBUG_PRINT("H: "); DEBUG_PRINT(h);
  DEBUG_PRINT(" S: "); DEBUG_PRINT(s);
  DEBUG_PRINT(" V: "); DEBUG_PRINTLN(v);

  uint32_t secondaryRGB = lights.getSecondaryColorRGB();
  DEBUG_PRINT("Secondary RGB: ");
  DEBUG_PRINT("R: "); DEBUG_PRINT((secondaryRGB >> 16) & 0xFF);
  DEBUG_PRINT(" G: "); DEBUG_PRINT((secondaryRGB >> 8) & 0xFF);
  DEBUG_PRINT(" B: "); DEBUG_PRINTLN(secondaryRGB & 0xFF);

  // For Tertiary Color (HSV and RGB)
  lights.getTertiaryColorHSV(h, s, v);
  DEBUG_PRINT("Tertiary HSV: ");
  DEBUG_PRINT("H: "); DEBUG_PRINT(h);
  DEBUG_PRINT(" S: "); DEBUG_PRINT(s);
  DEBUG_PRINT(" V: "); DEBUG_PRINTLN(v);

  uint32_t tertiaryRGB = lights.getTertiaryColorRGB();
  DEBUG_PRINT("Tertiary RGB: ");
  DEBUG_PRINT("R: "); DEBUG_PRINT((tertiaryRGB >> 16) & 0xFF);
  DEBUG_PRINT(" G: "); DEBUG_PRINT((tertiaryRGB >> 8) & 0xFF);
  DEBUG_PRINT(" B: "); DEBUG_PRINTLN(tertiaryRGB & 0xFF);
}

void setup() {
  #if DEBUG_MODE
    Serial.begin(115200);
    delay(1000); // Give time for the serial monitor to connect
  #endif
  settingsMutex = xSemaphoreCreateMutex();
  if (settingsMutex == NULL) {
    DEBUG_PRINTLN("Failed to create mutex!");
  }

  pinMode(DETECT_PIN, INPUT);  // Set GPIO16(RX2) as an input
  pinMode(LED, OUTPUT); // Switch active
  
  // Set the maximum steps per second:
  stepper.setMaxSpeed(950);

  loadSettings();
  setupWiFi();

  lights.begin();  // Initialize NeoPixel strip
  //lights.setFrontPixel(5);  // Pixel 5 becomes the new "lead" at startup

  // Simulated hex inputs from Web UI
  lights.updateColorsFromHex(primaryColor, secondaryColor, tertiaryColor);
  printColorData();

  // Create light control task on Core -xx
  xTaskCreatePinnedToCore(
    lightControlTask,        // Task function (calls methods of 'lights')
    "LightControlTask",      // Task name
    10000,                   // Stack size
    NULL,                    // Parameters
    1,                       // Task priority
    NULL,                    // Task handle
    1                        // Pin task to Core 0
  );
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();

  int state = digitalRead(DETECT_PIN);
  if (state == HIGH) {
    digitalWrite(LED, HIGH);
    stepper.setSpeed(stepperSpeed * stepperDirection);
    stepper.runSpeed();
  } else {
    digitalWrite(LED, LOW);
  }
}

// Task function that uses the 'lights' object
void lightControlTask(void *pvParameters) {
  DEBUG_PRINT("Task1 running on core ");
  DEBUG_PRINTLN(xPortGetCoreID());

  int brightnessCopy, modeCopy, speedCopy, directionCopy;
  String primaryCopy, secondaryCopy, tertiaryCopy;

  while (true) { // Infinite loop to prevent the task from exiting
    if (xSemaphoreTake(settingsMutex, portMAX_DELAY)) {
      brightnessCopy = lightBright;
      modeCopy = lightMode;
      speedCopy = stepperSpeed;
      directionCopy = stepperDirection;
      primaryCopy = primaryColor;
      secondaryCopy = secondaryColor;
      tertiaryCopy = tertiaryColor;
      xSemaphoreGive(settingsMutex);
    }

    // Handle the current mode
    switch (modeCopy) {
      case 0:
        lights.off();
        break;
      case 1:
        lights.solidSparkle(speedCopy, brightnessCopy);
        break;
      case 2:
        lights.fadeChaser(speedCopy, brightnessCopy, directionCopy);
        break;
      case 3:
        //lights.rainbowFade2WhiteStep(speedCopy, brightnessCopy);
        break;
      default:
        break;
    }

    // //DEBUG_PRINTLN("RGB Color Wipe");
    // lights.colorWipe(lights.strip.Color(255, 0, 0), 25); // Red
    // lights.colorWipe(lights.strip.Color(0, 255, 0), 50); // Green
    // lights.colorWipe(lights.strip.Color(0, 0, 255), 100); // Blue

    // //DEBUG_PRINTLN("White over rainbow");
    // lights.whiteOverRainbow(75, 5);

    // //DEBUG_PRINTLN("Pulse White");
    // lights.pulseWhite(5);

    // //DEBUG_PRINTLN("Rainbow Fade to White");
    // lights.rainbowFade2White(3, 3, 1);

    vTaskDelay(5 / portTICK_PERIOD_MS); // Avoid WDT resets
  }
}