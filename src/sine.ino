#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <TOTP.h>

#define NTP_SERVER     "ntp.kku.ac.th", "time.nist.gov"
#define UTC_OFFSET     25200 // Thailand is UTC+7 → 25200 sec
#define UTC_OFFSET_DST 0

uint8_t HMAC_KEY[] = {0x73, 0x65, 0x63, 0x72, 0x65, 0x74, 0x6b, 0x65, 0x79, 0x30};

#define __EMULATED_ENV__ // Comment this in production code

#ifndef __EMULATED_ENV__
  #define FALLBACK_SSID  "kku-wifi"
#else
  #define FALLBACK_SSID  "Wokwi-GUEST"
#endif

LiquidCrystal_I2C LCD = LiquidCrystal_I2C(0x27, 16, 2);

TOTP totp = TOTP(HMAC_KEY, 10);
String totpCode = String("");

time_t lastSync;

void drawMainInterface() {
  LCD.setCursor(0, 0);  
  LCD.print("time // ");
  LCD.setCursor(0, 1);  
  LCD.print("code //// ");
}
    
void spinner() {
  static int8_t counter = 0;
  const char* glyphs = "\xa1\xa5\xdb";
  LCD.setCursor(15, 1);
  LCD.print(glyphs[counter++]);
  if (counter == strlen(glyphs)) {
    counter = 0;
  }
}

int printCurrentTime() {
  static struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
     return 1;
  }
  LCD.setCursor(8, 0);
  LCD.println(&timeinfo, "%H:%M:%S");
  return 0;
}

int printCurrentTOTP() {
  static struct tm timeinfo;
  static time_t now;
  if (!getLocalTime(&timeinfo)) {
     return 1;
  }
  LCD.setCursor(10, 1);
  time(&now);
  String newCode = String(totp.getCode(now));
  if (totpCode != newCode) {
    totpCode = String(newCode);
    LCD.print(totpCode);
  }
  return 0;
}

void setupTime() {
  static struct tm timeinfo;
  configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);
  while (!getLocalTime(&timeinfo)) {
    delay(100);
  }
  time(&lastSync);
}

void setupWiFiGetTime() {
  LCD.clear();
  LCD.setCursor(0, 0);
  LCD.print("Looking for");
  LCD.setCursor(0, 1);
  LCD.print(FALLBACK_SSID);

  WiFi.begin(FALLBACK_SSID, "");

  while (WiFi.status() != WL_CONNECTED) {
    spinner();
    delay(250);
  }

  LCD.clear();
  LCD.setCursor(0, 0);
  LCD.print("Online!");
  LCD.setCursor(0, 1);
  LCD.print("Updating time");

  setupTime();
}

void setup() {
  Serial.begin(115200);

  LCD.init();
  LCD.backlight();

  setupWiFiGetTime();

  LCD.clear();
  delay(250);
  drawMainInterface();
}

void loop() {
  static time_t now;
  time(&now);

  // Resync time every 5 minutes
  if (now - lastSync > 300) {
    setupTime();
  }

  if (printCurrentTime()) {
    setupWiFiGetTime();
    drawMainInterface();
  }
  printCurrentTOTP();
  delay(250);
}