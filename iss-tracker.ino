#include <Adafruit_ILI9341.h>
#include <Adafruit_GFX.h>
#include <WiFi.h>

#define TOGGLE_PIN 4
#define TFT_CS 5
#define TFT_RST 6
#define TFT_DC 1
#define TFT_MOSI 19
#define TFT_SCK 18

const char* SSID = "Wokwi-GUEST";
const char* PASS = "";

bool currentMode = 1; // 1 - globe; 0 - list
bool lastButtonState = HIGH;

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
    } else {
        Serial.println("Working offline");
    }

  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(10, 10);
  tft.println("Globe");
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
  unsigned long startTime = millis();

  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startTime > 15000) {
      tft.println();
      tft.print("Wi-Fi FAIL.");
      Serial.println("Wi-Fi FAIL.");
      delay(1500);
      return false;
    }
  delay(500);
  tft.print(".");
  Serial.print(".");
  }
  tft.println();
  tft.print("Wi-Fi OK.");
  Serial.print("Wi-Fi OK. IP: ");
  Serial.println(WiFi.localIP());
  delay(1500);
  return true;
}

void loop() {
  handleButton();
  delay(10);
}
