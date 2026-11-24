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

bool currentMode = 1; // 1 - глобус; 0 - список
bool lastButtonState = HIGH;

float issLat = 0.0;
float issLon = 0.0;

Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);

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

  if (connectToWiFi()) {
        Serial.println("Network ready");
        tft.fillScreen(ILI9341_BLACK);
        tft.setCursor(10, 10);
        tft.println("Globe");
        fetchISSData();
    } else {
        Serial.println("Try again");
    }
}

void handleButton() {
  bool buttonState = digitalRead(TOGGLE_PIN);
  if (buttonState == LOW && lastButtonState == HIGH) {
    delay(50);
    currentMode = !currentMode;
    Serial.print("Mode = "); Serial.println(currentMode);
    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(10, 10);
    if (currentMode == 1) {
      tft.println("Globe");
    } else {
      tft.println("List");
    }
  }
  lastButtonState = buttonState;
}

bool connectToWiFi() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(10, 10);
  tft.print("Connecting to Wi-Fi...");
  Serial.println("Connecting to Wokwi-GUEST");

  WiFi.begin(SSID, PASS);
  WiFi.setTimeout(30000);
  unsigned long startTime = millis();

  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startTime > 30000) {
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

void fetchISSData() {
  HTTPClient http;
  WiFiClient client;

  http.begin(client, "http://api.open-notify.org/iss-now.json");
  http.setReuse(false);
  http.setTimeout(10000);

  int httpCode = http.GET();
  
  if (httpCode > 0) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, http.getString());
    if (error) {
      Serial.print("JSON parse failed: ");
      Serial.println(error.c_str());
      return;
    } else {
      issLat = doc["iss_position"]["latitude"].as<float>();
      issLon = doc["iss_position"]["longitude"].as<float>();

      Serial.print("latitude = ");
      Serial.print(issLat);
      Serial.print(", longitude = ");
      Serial.println(issLon);
    }
  } else {
    Serial.printf("HTTP request failed. Code: %d\n", httpCode);
  }
  http.end();
}

void loop() {
  handleButton();

  if (currentMode == 1 && millis() - lastISSUpdate > ISS_UPDATE_INTERVAL) {
    lastISSUpdate = millis();
    fetchISSData();
  }
  delay(10);
}
