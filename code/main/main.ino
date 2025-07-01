#include <WiFiManager.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <Adafruit_INA219.h>  // ✅ เปิดใช้ INA219

#define SDA_PIN 21
#define SCL_PIN 22
#define RESET_BUTTON 0  // ใช้ปุ่ม BOOT (GPIO0)

const char* sheetURL = "https://script.google.com/macros/s/AKfycbyUCfaF-JZhcWBzUe7eB_CQjkXM8wAd_b2z6c30qwO3CbH8MV4xzTkzetrzT_apRcyz/exec";

WiFiManager wm;
Adafruit_BME280 bme;
Adafruit_INA219 ina219;  // ✅ สร้างอ็อบเจกต์ INA219

void setup() {
  Serial.begin(115200);
  pinMode(RESET_BUTTON, INPUT_PULLUP);

  wm.setConfigPortalBlocking(true);
  wm.setShowInfoErase(true);
  wm.setBreakAfterConfig(true);
  wm.setConfigPortalTimeout(300);

  WiFiManagerParameter custom_text("<p>กดปุ่ม 'Erase WiFi' เพื่อล้างค่าเก่า</p>");
  wm.addParameter(&custom_text);

  bool configPortal = false;
  if (digitalRead(RESET_BUTTON) == LOW) {
    unsigned long pressedTime = millis();
    while (digitalRead(RESET_BUTTON) == LOW) {
      if (millis() - pressedTime > 2000) {
        configPortal = true;
        break;
      }
    }
  }

  if (configPortal) {
    Serial.println("🛠 เข้าสู่โหมดตั้งค่า WiFi ด้วยปุ่ม RESET");
    if (!wm.startConfigPortal("Meng-Weather-Setup")) {
      Serial.println("❌ เข้าสู่โหมดตั้งค่า WiFi ไม่สำเร็จ รีสตาร์ท...");
      ESP.restart();
    }
  } else {
    if (!wm.autoConnect("Meng-Weather-Setup")) {
      Serial.println("เชื่อม WiFi ไม่สำเร็จ รีสตาร์ท...");
      ESP.restart();
    }
  }

  Serial.println("✅ WiFi Connected!");

  Wire.begin(SDA_PIN, SCL_PIN);
  if (!bme.begin(0x76)) {
    Serial.println("ไม่พบ BME280");
  }

  if (!ina219.begin()) {
    Serial.println("ไม่พบ INA219");
  }

  float temp = bme.readTemperature();
  float hum = bme.readHumidity();
  float pres = bme.readPressure() / 100.0F;
  float vbat = ina219.getBusVoltage_V();
  float current_mA = ina219.getCurrent_mA();
  float power_mW = ina219.getPower_mW();

  Serial.printf("Temp: %.2f C | Hum: %.2f %% | Press: %.2f hPa\n", temp, hum, pres);
  Serial.printf("VBat: %.2f V | Current: %.2f mA | Power: %.2f mW\n", vbat, current_mA, power_mW);

  HTTPClient http;
  http.begin(sheetURL);
  http.addHeader("Content-Type", "application/json");

  String payload = "{";
  payload += "\"temp\":" + String(temp) + ",";
  payload += "\"hum\":" + String(hum) + ",";
  payload += "\"pres\":" + String(pres) + ",";
  payload += "\"vbat\":" + String(vbat) + ",";
  payload += "\"current\":" + String(current_mA) + ",";
  payload += "\"power\":" + String(power_mW);
  payload += "}";

  int code = http.POST(payload);
  Serial.println("HTTP Response: " + String(code));
  http.end();

  int sleepSec = 300;
  if (vbat < 3.5) sleepSec = 900;
  if (vbat < 3.3) sleepSec = 1800;

  Serial.printf("เข้าสู่ Deep Sleep %d วินาที\n", sleepSec);
  esp_sleep_enable_timer_wakeup((uint64_t)sleepSec * 1000000ULL);
  esp_deep_sleep_start();
}

void loop() {}
