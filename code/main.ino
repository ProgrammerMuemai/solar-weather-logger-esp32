#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_BME280.h>

#define SDA_PIN 21
#define SCL_PIN 22

const char* ssid = "mshome ais_2.4G";
const char* password = "212224236";
const char* scriptURL = "https://script.google.com/macros/s/AKfycbyUCfaF-JZhcWBzUe7eB_CQjkXM8wAd_b2z6c30qwO3CbH8MV4xzTkzetrzT_apRcyz/exec"; // ใส่ URL ของคุณ

Adafruit_BME280 bme;  // ใช้ I2C

void setup() {
  Serial.begin(115200);
  delay(100);

  Wire.begin(SDA_PIN, SCL_PIN);
  bool status = bme.begin(0x76); // หรือ 0x77 แล้วแต่ขา SDO
  if (!status) {
    Serial.println("ไม่พบ BME280 ตรวจสาย/Address");
    while (1);
  }

  WiFi.begin(ssid, password);
  Serial.print("กำลังเชื่อมต่อ WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("เชื่อมต่อ WiFi แล้ว!");
}

void loop() {
  float temp = bme.readTemperature();
  float hum = bme.readHumidity();
  float pres = bme.readPressure() / 100.0F;

  Serial.println("=== ข้อมูลวัดได้ ===");
  Serial.print("Temp: "); Serial.println(temp);
  Serial.print("Humidity: "); Serial.println(hum);
  Serial.print("Pressure: "); Serial.println(pres);

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(scriptURL);
    http.addHeader("Content-Type", "application/json");

    String json = "{\"temp\": " + String(temp) +
                  ", \"hum\": " + String(hum) +
                  ", \"pres\": " + String(pres) + "}";

    int httpCode = http.POST(json);
    String payload = http.getString();

    Serial.print("ส่งข้อมูลแล้ว! รหัส: ");
    Serial.println(httpCode);
    Serial.println("ตอบกลับ: " + payload);

    http.end();
  }

  delay(300000); // รอ 5 นาที (300,000 ms)
}
