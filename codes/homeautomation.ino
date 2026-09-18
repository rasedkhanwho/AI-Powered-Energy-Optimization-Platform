#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// =====================================================
// 1. WI-FI CREDENTIALS
// =====================================================

const char* ssid     = "robotics";
const char* password = "12345678";


// =====================================================
// 2. API ENDPOINTS
// =====================================================

// Sync endpoint for this user
const char* syncUrl =
  "http://10.27.119.104:8000/api/appliances/esp32-sync/"
  "5f66a784-2b00-4702-a6d8-0a04323ec961";

// Base endpoint used for toggle requests
const char* toggleBaseUrl =
  "http://10.27.119.104:8000/api/appliances/";


// =====================================================
// 3. HARDWARE CONFIGURATION
// =====================================================

const int NUM_CHANNELS = 2;

// ESP32 relay pins
const int RELAY_PINS[NUM_CHANNELS] = {
  25,   // Relay 1
  26    // Relay 2
};

// ESP32 push-button pins
const int BTN_PINS[NUM_CHANNELS] = {
  32,   // Button 1
  33    // Button 2
};


// Most 4-channel relay modules are ACTIVE LOW

const int RELAY_ON  = LOW;
const int RELAY_OFF = HIGH;


// =====================================================
// 4. APPLIANCE INFORMATION FROM SERVER
// =====================================================

String applianceIds[NUM_CHANNELS] = {
  "",
  ""
};

String applianceNames[NUM_CHANNELS] = {
  "Load 1",
  "Load 2"
};


// =====================================================
// 5. BUTTON DEBOUNCE
// =====================================================

// Raw previous readings
bool lastReading[NUM_CHANNELS] = {
  HIGH,
  HIGH
};

// Stable debounced button state
bool buttonState[NUM_CHANNELS] = {
  HIGH,
  HIGH
};

unsigned long lastDebounceTime[NUM_CHANNELS] = {
  0,
  0
};

const unsigned long DEBOUNCE_DELAY = 50;


// =====================================================
// 6. SERVER POLLING
// =====================================================

const unsigned long POLL_INTERVAL = 2000;

unsigned long lastPollTime = 0;


// =====================================================
// 7. SEND TOGGLE REQUEST
// =====================================================

void sendToggleRequest(int channelIndex) {

  if (channelIndex < 0 ||
      channelIndex >= NUM_CHANNELS) {

    return;
  }


  if (WiFi.status() != WL_CONNECTED) {

    Serial.println(
      "[WARN] WiFi disconnected. "
      "Local relay control still works."
    );

    return;
  }


  String targetId =
    applianceIds[channelIndex];


  if (targetId.length() == 0) {

    Serial.printf(
      "[WARN] No appliance ID mapped "
      "for Relay %d yet.\n",
      channelIndex + 1
    );

    return;
  }


  WiFiClient client;
  HTTPClient http;


  String endpoint =
    String(toggleBaseUrl) +
    targetId +
    "/toggle";


  Serial.println();
  Serial.print("[PATCH] ");
  Serial.println(endpoint);


  http.setTimeout(3000);


  if (!http.begin(client, endpoint)) {

    Serial.println(
      "[ERROR] Could not start HTTP connection."
    );

    return;
  }


  http.addHeader(
    "Content-Type",
    "application/json"
  );


  // Empty JSON body
  int httpCode =
    http.PATCH("{}");


  if (httpCode >= 200 &&
      httpCode < 300) {

    Serial.printf(
      "[SERVER] %s toggle successful.\n",
      applianceNames[channelIndex].c_str()
    );

  }

  else {

    Serial.printf(
      "[SERVER ERROR] Toggle failed. "
      "HTTP code: %d\n",
      httpCode
    );

  }


  http.end();
}


// =====================================================
// 8. SERVER → ESP32 STATE SYNC
// =====================================================

void syncFromServer() {

  if (WiFi.status() != WL_CONNECTED) {

    Serial.println(
      "[SYNC] WiFi disconnected."
    );

    return;
  }


  WiFiClient client;
  HTTPClient http;


  http.setTimeout(3000);


  if (!http.begin(client, syncUrl)) {

    Serial.println(
      "[ERROR] Could not connect to sync API."
    );

    return;
  }


  int httpCode =
    http.GET();


  if (httpCode == HTTP_CODE_OK) {

    String response =
      http.getString();


    // Increase if backend JSON becomes larger
    StaticJsonDocument<2048> doc;


    DeserializationError error =
      deserializeJson(doc, response);


    if (error) {

      Serial.print(
        "[JSON ERROR] "
      );

      Serial.println(
        error.c_str()
      );

      http.end();

      return;
    }


    bool success =
      doc["success"] | false;


    if (!success) {

      Serial.println(
        "[SYNC ERROR] Backend returned success=false"
      );

      http.end();

      return;
    }


    JsonArray relays =
      doc["relays"].as<JsonArray>();


    int channel = 0;


    for (JsonObject relay : relays) {

      if (channel >= NUM_CHANNELS)
        break;


      // -------------------------------------
      // Save appliance metadata
      // -------------------------------------

      applianceIds[channel] =
        relay["id"].as<String>();

      applianceNames[channel] =
        relay["name"].as<String>();


      // -------------------------------------
      // Read server state
      // -------------------------------------

      int serverState =
        relay["state"] | 0;


      // -------------------------------------
      // Apply state to physical relay
      // -------------------------------------

      digitalWrite(
        RELAY_PINS[channel],

        serverState == 1
        ? RELAY_ON
        : RELAY_OFF
      );


      channel++;
    }


    Serial.println();
    Serial.println(
      "------ SERVER SYNC ------"
    );


    for (int i = 0;
         i < NUM_CHANNELS;
         i++) {

      Serial.printf(
        "CH%d | %s | %s\n",

        i + 1,

        applianceNames[i].c_str(),

        digitalRead(RELAY_PINS[i])
          == RELAY_ON
          ? "ON"
          : "OFF"
      );
    }


    Serial.println(
      "-------------------------"
    );

  }

  else {

    Serial.printf(
      "[HTTP ERROR] GET failed. Code: %d\n",
      httpCode
    );

  }


  http.end();
}


// =====================================================
// 9. BUTTON CONTROL
// =====================================================

void handleButtons() {

  for (int i = 0;
       i < NUM_CHANNELS;
       i++) {


    bool reading =
      digitalRead(BTN_PINS[i]);


    // Detect change
    if (reading != lastReading[i]) {

      lastDebounceTime[i] =
        millis();

    }


    // Wait until input is stable
    if (
      (millis() - lastDebounceTime[i])
      > DEBOUNCE_DELAY
    ) {


      if (reading != buttonState[i]) {

        buttonState[i] =
          reading;


        // Button has just been pressed
        if (buttonState[i] == LOW) {


          // Current relay state
          bool currentlyOn =
            digitalRead(
              RELAY_PINS[i]
            ) == RELAY_ON;


          // Toggle relay immediately
          digitalWrite(

            RELAY_PINS[i],

            currentlyOn
            ? RELAY_OFF
            : RELAY_ON

          );


          bool newState =
            !currentlyOn;


          Serial.println();

          Serial.printf(
            "[BUTTON %d] %s -> %s\n",

            i + 1,

            applianceNames[i].c_str(),

            newState
              ? "ON"
              : "OFF"
          );


          // Tell web/backend
          sendToggleRequest(i);

        }

      }

    }


    lastReading[i] =
      reading;

  }

}


// =====================================================
// 10. WIFI CONNECTION
// =====================================================

void connectWiFi() {

  Serial.println();

  Serial.print(
    "Connecting to WiFi: "
  );

  Serial.println(
    ssid
  );


  WiFi.mode(
    WIFI_STA
  );


  WiFi.begin(
    ssid,
    password
  );


  int attempts = 0;


  while (
    WiFi.status()
      != WL_CONNECTED
    &&
    attempts < 40
  ) {

    delay(500);

    Serial.print(".");

    attempts++;

  }


  Serial.println();


  if (
    WiFi.status()
      == WL_CONNECTED
  ) {

    Serial.println(
      "[WIFI] Connected!"
    );

    Serial.print(
      "[WIFI] ESP32 IP: "
    );

    Serial.println(
      WiFi.localIP()
    );

  }

  else {

    Serial.println(
      "[WIFI] Connection failed."
    );

    Serial.println(
      "[INFO] Physical buttons will "
      "still operate the relays."
    );

  }

}


// =====================================================
// 11. SETUP
// =====================================================

void setup() {

  Serial.begin(
    115200
  );


  delay(
    500
  );


  Serial.println();
  Serial.println(
    "================================"
  );

  Serial.println(
    " SMART ENERGY MANAGEMENT SYSTEM"
  );

  Serial.println(
    " ESP32 WEB + BUTTON CONTROL"
  );

  Serial.println(
    "================================"
  );


  // -------------------------------------
  // Relay configuration
  // -------------------------------------

  for (
    int i = 0;
    i < NUM_CHANNELS;
    i++
  ) {

    pinMode(
      RELAY_PINS[i],
      OUTPUT
    );


    // All loads OFF at startup
    digitalWrite(
      RELAY_PINS[i],
      RELAY_OFF
    );

  }


  // -------------------------------------
  // Button configuration
  // -------------------------------------

  for (
    int i = 0;
    i < NUM_CHANNELS;
    i++
  ) {

    pinMode(
      BTN_PINS[i],
      INPUT_PULLUP
    );

  }


  // Connect WiFi
  connectWiFi();


  // First immediate server sync
  if (
    WiFi.status()
      == WL_CONNECTED
  ) {

    syncFromServer();

  }

}


// =====================================================
// 12. MAIN LOOP
// =====================================================

void loop() {

  // -------------------------------------
  // Physical push buttons
  // -------------------------------------

  handleButtons();


  // -------------------------------------
  // Periodic web/backend synchronization
  // -------------------------------------

  if (
    millis() - lastPollTime
    >= POLL_INTERVAL
  ) {

    lastPollTime =
      millis();


    if (
      WiFi.status()
        == WL_CONNECTED
    ) {

      syncFromServer();

    }

    else {

      Serial.println(
        "[WIFI] Trying to reconnect..."
      );


      WiFi.reconnect();

    }

  }

}