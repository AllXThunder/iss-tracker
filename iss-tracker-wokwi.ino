#include <Adafruit_ILI9341.h>
#include <Adafruit_GFX.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define TOGGLE_PIN 4
#define TFT_CS 5
#define TFT_RST 6
#define TFT_DC 1
#define TFT_MOSI 19
#define TFT_SCK 18

const char* SSID = "Wokwi-GUEST";
const char* PASS = "";

unsigned long lastISSUpdate = 0;
const unsigned long ISS_UPDATE_INTERVAL = 60000;

unsigned long lastReconnectAttempt = 0;
const unsigned long RECONNECT_INTERVAL = 60000;

bool currentMode = 1; // 1 - глобус; 0 - список
bool lastButtonState = HIGH;

float issLat = 0.0;
float issLon = 0.0;
bool issDataValid = false;

String astronauts[15];
int astronautCount = 0;
bool astronautDataValid = false;
unsigned long lastAstroUpdate = 0;
const unsigned long ASTRO_UPDATE_INTERVAL = 300000;

Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);

bool connectToWiFi(unsigned long timeoutMs = 30000) {
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(10, 10);
  tft.print("Connecting to Wi-Fi...");
  Serial.println("Connecting to Wokwi-GUEST");

  WiFi.disconnect(true);
  delay(100);
  WiFi.begin(SSID, PASS);
  unsigned long startTime = millis();

  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startTime > timeoutMs) {
      delay(1000);
      tft.println();
      tft.print("Wi-Fi FAIL.");
      Serial.println("Wi-Fi FAIL.");
      delay(1500);
      return false;
    }
  delay(500);
  tft.print(".");
  Serial.print("Status: ");
  Serial.println(WiFi.status());
  delay(500);
  }
  tft.println();
  tft.print("Wi-Fi OK.");
  Serial.print("Wi-Fi OK. IP: ");
  Serial.println(WiFi.localIP());
  delay(1500);
  return true;
}

void fetchAstronautData() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("fetchAstronautData: No WiFi.");
    return;
  }

  HTTPClient http;
  WiFiClient client;
  http.begin(client, "http://api.open-notify.org/astros.json");
  http.setReuse(false);
  http.setTimeout(10000);
  int httpCode = http.GET();

  if (httpCode == 200) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, http.getString());
    if (error) {
      Serial.print("JSON parse failed: "); Serial.println(error.c_str());
      drawErrorScreen("DATA ERROR");
      return;
    }

    astronautCount = 0;
    JsonArray people = doc["people"];
    for (JsonObject person : people) {
      if (astronautCount < 15) {
        astronauts[astronautCount] = person["name"].as<String>();
        astronautCount++;
      }
    }
    astronautDataValid = true;
    Serial.print("Astronauts loaded: ");
    Serial.println(astronautCount);
  } else {
    Serial.printf("Astronaut HTTP failed. Code: %d\n", httpCode);
    astronautDataValid = false;
  }
  http.end();
}

void setup() {
  Serial.begin(115200);

  pinMode(TOGGLE_PIN, INPUT_PULLUP);
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(10, 10);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.println("Booting...");
  delay(500);

  bool connected = false;
  for (int attempt = 1; attempt <= 3; attempt++) {
    Serial.print("Connection attempt ");
    Serial.println(attempt);
    if (connectToWiFi()) {
      connected = true;
      break;
    }
    Serial.println("Attempt failed. Retrying in 2 seconds...");
    delay(2000);
  }

    if (connected) {
    Serial.println("Network ready");
    drawGlobeMode();
  } else {
    Serial.println("Failed to connect after 3 attempts. Working offline.");
    drawGlobeMode();
  }
}

void handleButton() {
  bool buttonState = digitalRead(TOGGLE_PIN);
  if (buttonState == LOW && lastButtonState == HIGH) {
    delay(50);
    tft.fillScreen(ILI9341_BLACK);
    currentMode = !currentMode;
    Serial.print("Mode = "); Serial.println(currentMode);
    
    if (currentMode == 1) {
      fetchISSData();
      drawGlobeMode();
    } else {
      fetchAstronautData();
      drawListMode();
    }
  }
  lastButtonState = buttonState;
}

void fetchISSData() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("fetchISSData: No WiFi.");
    return;
  }

  HTTPClient http;
  WiFiClient client;
  http.begin(client, "http://api.open-notify.org/iss-now.json");
  http.setReuse(false);
  http.setTimeout(10000);
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, http.getString());
    if (error) {
      Serial.print("JSON parse failed: "); Serial.println(error.c_str());
      drawErrorScreen("DATA ERROR");
      return;
    } else {
      issLat = doc["iss_position"]["latitude"].as<float>();
      issLon = doc["iss_position"]["longitude"].as<float>();
      issDataValid = true;
      Serial.println("Data OK");
    }
  } else {
    Serial.printf("HTTP failed. Code: %d\n", httpCode);
    drawErrorScreen("API/Wi-Fi OFFLINE"); 
  }
  http.end();
}

void drawListMode() {
  tft.fillScreen(ILI9341_BLACK);

  tft.setCursor(10, 10);
  tft.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  tft.setTextSize(2);
  tft.print("Crew on ISS: ");

  if (astronautDataValid) {
    tft.println(astronautCount);

    tft.setTextSize(1);
    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    int y = 40;

    for (int i = 0; i < astronautCount; i++) {
      tft.setCursor(10, y);
      tft.print(i + 1);
      tft.print(". ");
      tft.println(astronauts[i]);
      y += 15;
    }
  } else {
    tft.println("");
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
    tft.println("Waiting for data...");
  }
}

void drawErrorScreen(String errorMsg) {
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(10, 130);
  tft.setTextColor(ILI9341_WHITE, ILI9341_RED);
  tft.setTextSize(2);
  tft.print(errorMsg);
}

void drawGlobeMode() {
  tft.fillScreen(ILI9341_BLACK);
  
  tft.setCursor(10, 10);
  tft.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  tft.setTextSize(3);
  tft.println("ISS Tracker");

  tft.fillRect(0, 50, 320, 100, ILI9341_BLACK);
  tft.setCursor(10, 60);
  
  if (issDataValid) {
    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft.setTextSize(2);
    
    tft.print("Lat: ");
    tft.println(issLat, 4);
    
    tft.print("Lon: ");
    tft.println(issLon, 4);
  } else {
    tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
    tft.setTextSize(2);
    tft.println("Waiting for data...");
  }
}

void loop() {
  handleButton();

  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - lastReconnectAttempt > RECONNECT_INTERVAL) {
      lastReconnectAttempt = millis();
      Serial.println("WiFi lost. Attempting to reconnect...");
      tft.fillScreen(ILI9341_BLACK);
      tft.setCursor(10, 10);
      tft.println("Reconnecting...");
      connectToWiFi(10000);
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Reconnected!");
        lastISSUpdate = 0;
      } else {
        Serial.println("Reconnect failed.");
        tft.fillScreen(ILI9341_BLACK);
        tft.setCursor(10, 10);
        tft.println("Offline");
      }
    }
  }

  if (currentMode == 1 && WiFi.status() == WL_CONNECTED && 
      millis() - lastISSUpdate > ISS_UPDATE_INTERVAL) {
    lastISSUpdate = millis();
    fetchISSData();
    if (currentMode == 1) {
      drawGlobeMode();
    }
  }

  if (currentMode == 0 && WiFi.status() == WL_CONNECTED && 
      millis() - lastAstroUpdate > ASTRO_UPDATE_INTERVAL) {
    lastAstroUpdate = millis();
    fetchAstronautData();
    if (currentMode == 0) {
      drawListMode();
    }
  }

  delay(10);
}
