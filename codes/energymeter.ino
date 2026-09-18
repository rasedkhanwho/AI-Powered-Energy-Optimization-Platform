#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include "ACS712.h"
#include <ZMPT101B.h>

#include <EEPROM.h>


// =====================================================
// 1. WIFI CONFIGURATION
// =====================================================

const char* ssid     = "robotics";
const char* password = "12345678";


// =====================================================
// 2. BACKEND CONFIGURATION
// =====================================================

const char* userId =
  "5f66a784-2b00-4702-a6d8-0a04323ec961";

const char* telemetryUrl =
  "http://10.27.119.104:8000/api/energy-meter/telemetry";


// =====================================================
// 3. HARDWARE CONFIGURATION
// =====================================================

// ACS712 30A
// OUT -> GPIO 34
// 30A model approximately 66mV/A

ACS712 ACS(
  34,
  3.3,
  4095,
  66
);


// ZMPT101B
// OUT -> GPIO 35

ZMPT101B voltageSensor(
  35,
  50.0
);


// LCD
// SDA -> GPIO21
// SCL -> GPIO22

LiquidCrystal_I2C lcd(
  0x27,
  16,
  2
);


// =====================================================
// 4. CALIBRATION
// Taken from previous working Blynk version
// =====================================================

const float CURRENT_CALIBRATION = 120.0;

const float VOLTAGE_OFFSET = 3.4;

const float ZMPT_SENSITIVITY = 578.0;

const int SENSOR_SAMPLES = 50;


// =====================================================
// 5. ENERGY VARIABLES
// =====================================================

float voltage = 0.0;
float current = 0.0;
float power   = 0.0;


// Internal accumulated energy in Wh
float energy_Wh = 0.0;


// Converted unit
float energy_kWh = 0.0;


// Electricity rate
const float UNIT_RATE = 10.50;


// Total cost
float totalBill = 0.0;


// Timer used for real energy integration
unsigned long lastEnergyUpdate = 0;


// =====================================================
// 6. EEPROM CONFIGURATION
// =====================================================

#define EEPROM_SIZE 512

#define ENERGY_ADDR 0

#define MAGIC_ADDR 10

#define EEPROM_MAGIC 0xA5


const unsigned long EEPROM_SAVE_INTERVAL =
  10000;

unsigned long lastEEPROMTime = 0;


// =====================================================
// 7. SYSTEM TIMERS
// =====================================================

const unsigned long SENSOR_INTERVAL =
  1000;

const unsigned long TELEMETRY_INTERVAL =
  3000;

const unsigned long WIFI_RETRY_INTERVAL =
  5000;


unsigned long lastSensorTime = 0;

unsigned long lastTelemetryTime = 0;

unsigned long lastWiFiRetry = 0;


// =====================================================
// 8. READ CURRENT
// Previous Blynk measurement method
// =====================================================

float readCurrent() {

  float sum = 0.0;


  for (int i = 0; i < SENSOR_SAMPLES; i++) {

    sum += ACS.mA_AC();

    delay(2);
  }


  float avg_mA =
    sum / SENSOR_SAMPLES;


  // Remove calibration offset
  float corrected_mA =
    fabs(avg_mA) - CURRENT_CALIBRATION;


  // Remove tiny sensor noise
  if (corrected_mA < 20.0) {

    corrected_mA = 0.0;
  }


  // mA -> A
  return corrected_mA / 1000.0;
}


// =====================================================
// 9. READ VOLTAGE
// Previous Blynk measurement method
// =====================================================

float readVoltage() {

  float sum = 0.0;


  for (int i = 0; i < SENSOR_SAMPLES; i++) {

    sum += voltageSensor.getRmsVoltage();

    delay(2);
  }


  float avgVoltage =
    sum / SENSOR_SAMPLES;


  // Calibration correction
  avgVoltage -= VOLTAGE_OFFSET;


  // Ignore small/noise readings
  if (avgVoltage < 20.0) {

    avgVoltage = 0.0;
  }


  return avgVoltage;
}


// =====================================================
// 10. UPDATE ENERGY METER
// =====================================================

void updateEnergyMeter() {

  // --------------------------------------
  // Voltage
  // --------------------------------------

  voltage =
    readVoltage();


  // --------------------------------------
  // Current
  // --------------------------------------

  current =
    readCurrent();


  // --------------------------------------
  // Power
  // P = V x I
  // --------------------------------------

  power =
    voltage * current;


  // --------------------------------------
  // Energy
  //
  // Wh = Watts x Hours
  // --------------------------------------

  unsigned long now =
    millis();


  if (lastEnergyUpdate == 0) {

    lastEnergyUpdate = now;
  }


  float elapsedHours =

    (now - lastEnergyUpdate)

    / 3600000.0;


  lastEnergyUpdate =
    now;


  if (power > 0.0) {

    energy_Wh +=
      power * elapsedHours;
  }


  // Wh -> kWh

  energy_kWh =
    energy_Wh / 1000.0;


  // Cost

  totalBill =
    energy_kWh * UNIT_RATE;


  // ===================================================
  // SERIAL MONITOR
  // ===================================================

  Serial.println();
  Serial.println(
    "======= SMART ENERGY METER ======="
  );


  Serial.print("Voltage : ");
  Serial.print(voltage, 1);
  Serial.println(" V");


  Serial.print("Current : ");
  Serial.print(current, 3);
  Serial.println(" A");


  Serial.print("Power   : ");
  Serial.print(power, 1);
  Serial.println(" W");


  Serial.print("Energy  : ");
  Serial.print(energy_kWh, 6);
  Serial.println(" kWh");


  Serial.print("Bill    : ");
  Serial.print(totalBill, 2);
  Serial.println(" BDT");


  Serial.println(
    "=================================="
  );


  // ===================================================
  // LCD
  // ===================================================

  // Row 1
  lcd.setCursor(0, 0);
  lcd.print("                ");

  lcd.setCursor(0, 0);

  lcd.print("V:");
  lcd.print(voltage, 0);

  lcd.print(" I:");
  lcd.print(current, 2);


  // Row 2
  lcd.setCursor(0, 1);
  lcd.print("                ");

  lcd.setCursor(0, 1);

  lcd.print("P:");
  lcd.print(power, 0);

  lcd.print(" U:");
  lcd.print(energy_kWh, 3);
}


// =====================================================
// 11. SEND TELEMETRY
// =====================================================

void sendTelemetry() {

  if (WiFi.status() != WL_CONNECTED) {

    Serial.println(
      "[TELEMETRY ERROR] WiFi not connected"
    );

    return;
  }


  Serial.println();
  Serial.println(
    "======= SENDING TELEMETRY ======="
  );


  Serial.print(
    "ESP32 IP: "
  );

  Serial.println(
    WiFi.localIP()
  );


  Serial.print(
    "Server: "
  );

  Serial.println(
    telemetryUrl
  );


  // ===================================================
  // JSON PAYLOAD
  // ===================================================

  StaticJsonDocument<512> doc;


  doc["userId"] =
    userId;


  doc["voltage"] =
    voltage;


  doc["current"] =
    current;


  doc["power"] =
    power;


  // Keep same backend property
  doc["botUnit"] =
    energy_kWh;


  doc["amount"] =
    UNIT_RATE;


  doc["totalBill"] =
    totalBill;


  String jsonPayload;


  serializeJson(
    doc,
    jsonPayload
  );


  Serial.print(
    "JSON: "
  );

  Serial.println(
    jsonPayload
  );


  // ===================================================
  // HTTP REQUEST
  // ===================================================

  WiFiClient client;

  HTTPClient http;


  http.setTimeout(
    5000
  );


  // Handle 301 / 302 / 307 redirects
  http.setFollowRedirects(
    HTTPC_STRICT_FOLLOW_REDIRECTS
  );


  if (
    !http.begin(
      client,
      telemetryUrl
    )
  ) {

    Serial.println(
      "[HTTP ERROR] http.begin() failed"
    );

    return;
  }


  http.addHeader(
    "Content-Type",
    "application/json"
  );


  http.addHeader(
    "Accept",
    "application/json"
  );


  int httpCode =
    http.POST(
      jsonPayload
    );


  // ===================================================
  // DEBUG RESPONSE
  // ===================================================

  Serial.print(
    "HTTP CODE: "
  );

  Serial.println(
    httpCode
  );


  if (httpCode > 0) {

    String response =
      http.getString();


    Serial.print(
      "SERVER RESPONSE: "
    );

    Serial.println(
      response
    );


    if (
      httpCode >= 200 &&
      httpCode < 300
    ) {

      Serial.println(
        "[TELEMETRY] SUCCESS"
      );


      Serial.printf(
        "V: %.1f V | I: %.3f A | P: %.1f W\n",
        voltage,
        current,
        power
      );


      Serial.printf(
        "Energy: %.6f kWh | Bill: %.2f BDT\n",
        energy_kWh,
        totalBill
      );

    }

    else {

      Serial.println(
        "[TELEMETRY ERROR] Backend rejected request"
      );
    }

  }

  else {

    Serial.print(
      "[CONNECTION ERROR] "
    );


    Serial.println(
      http.errorToString(httpCode)
    );
  }


  Serial.println(
    "================================="
  );


  http.end();
}


// =====================================================
// 12. EEPROM SAVE
// =====================================================

void saveEnergy() {

  EEPROM.writeFloat(
    ENERGY_ADDR,
    energy_Wh
  );


  EEPROM.commit();


  Serial.print(
    "[EEPROM] Saved: "
  );

  Serial.print(
    energy_Wh,
    4
  );

  Serial.println(
    " Wh"
  );
}


// =====================================================
// 13. CONNECT WIFI
// =====================================================

void connectWiFi() {

  WiFi.mode(
    WIFI_STA
  );


  WiFi.begin(
    ssid,
    password
  );


  Serial.println();

  Serial.print(
    "Connecting to WiFi"
  );


  lcd.setCursor(
    0,
    0
  );

  lcd.print(
    "Connecting WiFi "
  );


  int attempts = 0;


  while (
    WiFi.status() != WL_CONNECTED &&
    attempts < 30
  ) {

    delay(
      500
    );

    Serial.print(
      "."
    );

    attempts++;
  }


  Serial.println();


  if (
    WiFi.status() == WL_CONNECTED
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


    Serial.print(
      "[WIFI] Gateway: "
    );

    Serial.println(
      WiFi.gatewayIP()
    );


    Serial.print(
      "[WIFI] Signal: "
    );

    Serial.print(
      WiFi.RSSI()
    );

    Serial.println(
      " dBm"
    );


    lcd.clear();

    lcd.setCursor(
      0,
      0
    );

    lcd.print(
      "WiFi Connected"
    );


    delay(
      1000
    );

  }

  else {

    Serial.println(
      "[WIFI] Connection failed"
    );


    Serial.println(
      "[INFO] Meter continues offline"
    );


    lcd.clear();

    lcd.setCursor(
      0,
      0
    );

    lcd.print(
      "Offline Mode"
    );


    delay(
      1000
    );
  }
}


// =====================================================
// 14. SETUP
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
    "================================="
  );

  Serial.println(
    " SMART ENERGY METER"
  );

  Serial.println(
    " ESP32 WEB TELEMETRY"
  );

  Serial.println(
    "================================="
  );


  // ===================================================
  // LCD
  // ===================================================

  Wire.begin(
    21,
    22
  );


  lcd.init();

  lcd.backlight();


  lcd.setCursor(
    0,
    0
  );

  lcd.print(
    "Smart Energy"
  );


  lcd.setCursor(
    0,
    1
  );

  lcd.print(
    "Meter Starting"
  );


  delay(
    1200
  );


  // ===================================================
  // EEPROM
  // ===================================================

  EEPROM.begin(
    EEPROM_SIZE
  );


  byte magic =
    EEPROM.read(
      MAGIC_ADDR
    );


  if (
    magic != EEPROM_MAGIC
  ) {

    energy_Wh =
      0.0;


    EEPROM.writeFloat(
      ENERGY_ADDR,
      energy_Wh
    );


    EEPROM.write(
      MAGIC_ADDR,
      EEPROM_MAGIC
    );


    EEPROM.commit();


    Serial.println(
      "[EEPROM] First boot -> 0 Wh"
    );

  }

  else {

    energy_Wh =
      EEPROM.readFloat(
        ENERGY_ADDR
      );


    if (
      isnan(energy_Wh) ||
      energy_Wh < 0.0
    ) {

      energy_Wh =
        0.0;
    }


    energy_kWh =
      energy_Wh /
      1000.0;


    totalBill =
      energy_kWh *
      UNIT_RATE;


    Serial.print(
      "[EEPROM] Energy loaded: "
    );

    Serial.print(
      energy_kWh,
      6
    );

    Serial.println(
      " kWh"
    );
  }


  // ===================================================
  // ACS712
  // ===================================================

  Serial.println(
    "[ACS712] Calibrating..."
  );


  ACS.autoMidPoint();


  Serial.println(
    "[ACS712] Calibration complete"
  );


  // ===================================================
  // ZMPT101B
  // ===================================================

  voltageSensor.setSensitivity(
    ZMPT_SENSITIVITY
  );


  Serial.print(
    "[ZMPT] Sensitivity = "
  );

  Serial.println(
    ZMPT_SENSITIVITY
  );


  // ===================================================
  // WIFI
  // ===================================================

  connectWiFi();


  // Start timing after startup

  lastEnergyUpdate =
    millis();


  lastSensorTime =
    millis();


  lastTelemetryTime =
    millis();
}


// =====================================================
// 15. MAIN LOOP
// =====================================================

void loop() {

  unsigned long now =
    millis();


  // ===================================================
  // SENSOR UPDATE
  // ===================================================

  if (
    now - lastSensorTime
    >= SENSOR_INTERVAL
  ) {

    lastSensorTime =
      now;


    updateEnergyMeter();
  }


  // ===================================================
  // TELEMETRY PUSH
  // ===================================================

  if (
    now - lastTelemetryTime
    >= TELEMETRY_INTERVAL
  ) {

    lastTelemetryTime =
      now;


    if (
      WiFi.status() == WL_CONNECTED
    ) {

      sendTelemetry();

    }

    else {

      Serial.println(
        "[WIFI] Disconnected"
      );
    }
  }


  // ===================================================
  // WIFI RECONNECT
  // ===================================================

  if (
    WiFi.status() != WL_CONNECTED &&
    now - lastWiFiRetry >= WIFI_RETRY_INTERVAL
  ) {

    lastWiFiRetry =
      now;


    Serial.println(
      "[WIFI] Attempting reconnect..."
    );


    WiFi.disconnect();

    WiFi.begin(
      ssid,
      password
    );
  }


  // ===================================================
  // EEPROM SAVE
  // ===================================================

  if (
    now - lastEEPROMTime
    >= EEPROM_SAVE_INTERVAL
  ) {

    lastEEPROMTime =
      now;


    saveEnergy();
  }
}