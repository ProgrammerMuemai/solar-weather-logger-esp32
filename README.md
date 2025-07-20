# Solar Weather Logger (ESP32 + BME280 + INA219 + Google Sheet + WiFiManager)

ระบบบันทึกอุณหภูมิ ความชื้น ความกดอากาศ และแรงดัน-กระแสไฟ จากแผงโซล่า ด้วย ESP32, เซ็นเซอร์ BME280 และ INA219 พร้อมส่งข้อมูลขึ้น Google Sheet แบบอัตโนมัติทุก 5 นาที โดยใช้พลังงานจากแผงโซล่าเซลล์ และเข้าสู่โหมด Deep Sleep เพื่อประหยัดพลังงาน

## 📦 อุปกรณ์ที่ใช้

* ESP32 (รุ่นใดก็ได้)
* เซ็นเซอร์ BME280 (I2C)
* โมดูล INA219 (I2C)
* WiFi (บ้าน/มือถือ)
* แผงโซล่าเซลล์ + แบตเตอรี่สำรอง (ในตัวอย่างทดสอบใช้ LiFePO4 5500mAh)
* ตัวต้านทาน Pull-up สำหรับปุ่มรีเซ็ต

## 🔌 การเชื่อมต่อ

| อุปกรณ์ | ESP32 Pin |
| :--- | :--- |
| BME280 VCC | 3.3V |
| BME280 GND | GND |
| BME280 SDA | GPIO21 |
| BME280 SCL | GPIO22 |
| INA219 VCC | 3.3V หรือ 5V |
| INA219 GND | GND |
| INA219 SDA | GPIO21 (ร่วมกับ BME280) |
| INA219 SCL | GPIO22 (ร่วมกับ BME280) |
| ปุ่มรีเซ็ต Wi-Fi | GPIO14 → GND |

> **หมายเหตุ:** VIN+ ของ INA219 ให้ต่อกับขั้วบวกจากแหล่งจ่าย (แผงโซล่า) และ VIN- ต่อไปยังโหลด เช่น โมดูลชาร์จแบต

## 🔁 การทำงานหลัก

* อ่านค่าอุณหภูมิ ความชื้น ความกดอากาศ จาก BME280
* อ่านแรงดันและกระแสจาก INA219
* ส่งข้อมูลขึ้น Google Sheet ผ่าน Web App URL
* เข้าสู่โหมด Deep Sleep 300 วินาที (5 นาที)
* ใช้ WiFiManager เพื่อจัดการการเชื่อมต่อ Wi-Fi
* หากกดปุ่มที่ต่อกับ GPIO14 ค้างไว้ 2 วินาทีตอนเปิดเครื่อง → เข้าสู่โหมดตั้งค่า Wi-Fi ใหม่

## ⚡️ การวิเคราะห์อัตราการใช้พลังงาน (Power Consumption Analysis)

จากการทดสอบบันทึกข้อมูลอย่างต่อเนื่อง พบพฤติกรรมการใช้พลังงานของอุปกรณ์ดังนี้:

### ลักษณะการใช้กระแสไฟฟ้า

* **สถานะปกติ (Normal Operation):** อุปกรณ์ใช้กระแสไฟฟ้าเฉลี่ยประมาณ **`102-110 mA`**
* **สถานะทำงานสูงสุด (Peak Operation):** ในบางช่วงเวลา (คาดว่าเป็นการส่งข้อมูล) อุปกรณ์มีการใช้กระแสไฟฟ้าพุ่งสูงขึ้นไปถึง **`230-290 mA`**

### การประเมินอายุการใช้งานแบตเตอรี่ (Battery Life Estimation)

* **กระแสไฟฟ้าเฉลี่ยรวม (Overall Average Current):** **`~115 mA`**
* เมื่อทดสอบกับแบตเตอรี่ `LiFePO4 5500mAh` จะสามารถทำงานต่อเนื่องได้ประมาณ **47-48 ชั่วโมง** หรือประมาณ **2 วัน** ต่อการชาร์จเต็มหนึ่งครั้ง

```
อายุการใช้งาน (ชั่วโมง) = 5500 mAh / 115 mA ≈ 47.8 ชั่วโมง
```

## 🌐 การตั้งค่า Google Sheet

1.  สร้าง Google Sheet ใหม่
2.  ไปที่ `ส่วนขยาย (Extensions)` > `Apps Script`
3.  เขียน Google Apps Script เพื่อรับข้อมูล POST และบันทึกลง Sheet
4.  Deploy เป็น Web App และนำ URL มาใส่ในตัวแปร `sheetURL` ในโค้ด Arduino
5.  สร้างแถวแรกใน Sheet ให้เป็นหัวตาราง (Header) ดังนี้:
    `Timestamp|Temperature|Humidity|Pressure|Vbat|Current (mA)|Power(mW)|RainChance|Description`
    *(ไม่มีช่องว่าง และตัวพิมพ์เล็กทั้งหมด ตรงกับ key ที่ ESP32 ส่งมา)*

### 🧠 ตัวอย่าง Apps Script ที่ใช้:

```javascript
function doPost(e) {
  var sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();
  var data = JSON.parse(e.postData.contents);

  var temp = data.temp;
  var hum = data.hum;
  var pres = data.pres;
  var vbat = data.vbat || "";
  var current = data.current || "";
  var power = data.power || "";

  // คำนวณโอกาสฝนตก (RainChance %)
  var rainChance = 0;
  if (hum > 80) rainChance += 20;
  if (hum > 90) rainChance += 10;
  if (pres < 1010) rainChance += 20;
  if (pres < 1000) rainChance += 10;
  if (temp >= 28) rainChance += 10;
  if (temp >= 30) rainChance += 5;

  // แปลงผลโอกาสฝนตกเป็นคำอธิบาย
  var description = "";
  if (rainChance >= 70) {
    description = "🌧 ฝนมีโอกาสตกสูงมาก";
  } else if (rainChance >= 40) {
    description = "🌦 มีโอกาสฝนตก";
  } else if (rainChance >= 20) {
    description = "⛅ อาจมีฝนเล็กน้อย";
  } else {
    description = "☀️ โอกาสฝนตกต่ำ";
  }

  // เพิ่มข้อมูลลงในชีต
  sheet.appendRow([
    new Date(),
    temp,
    hum,
    pres,
    vbat,
    current,
    power,
    rainChance,
    description
  ]);

  return ContentService.createTextOutput("Success");
}

```

## 📚 ติดตั้งไลบรารีใน Arduino IDE

ให้ติดตั้งไลบรารีเพิ่มเติมจาก Library Manager ดังนี้:

* WiFiManager โดย tzapu
* Adafruit BME280 Library
* Adafruit INA219
* Adafruit Unified Sensor
* ArduinoJson (สำหรับจัดการ JSON)

## 📲 ขั้นตอนการเชื่อมต่อ Wi-Fi ครั้งแรก (ละเอียด)

1.  เปิดบอร์ด ESP32 (ไฟสถานะจะติด และเริ่มบูตระบบ)
2.  หากยังไม่เคยตั้งค่า Wi-Fi มาก่อน ESP32 จะสร้าง Wi-Fi ชื่อ `Meng-Weather-Setup`
3.  บนมือถือหรือคอมพิวเตอร์:
    * เปิดการเชื่อมต่อ Wi-Fi แล้วเลือกเชื่อมต่อกับ `Meng-Weather-Setup`
    * เมื่อเชื่อมต่อแล้ว ให้เปิดเบราว์เซอร์เข้าไปที่ `http://192.168.4.1` (หากเบราว์เซอร์ไม่ขึ้นอัตโนมัติ)
4.  ในหน้า WiFiManager Portal:
    * เลือกเครือข่าย Wi-Fi ที่ต้องการเชื่อมต่อ
    * กรอกรหัสผ่าน Wi-Fi ให้ถูกต้อง
    * กดปุ่ม "Save"
5.  ESP32 จะทำการบันทึกค่าและรีสตาร์ทอัตโนมัติ
6.  หากการเชื่อมต่อสำเร็จ บอร์ดจะเริ่มส่งข้อมูลไปยัง Google Sheet และเข้าสู่โหมด Deep Sleep

## 🔁 วิธีรีเซ็ตการตั้งค่า Wi-Fi (ละเอียด)

> ใช้เมื่อคุณต้องการเปลี่ยน Wi-Fi หรือกรณีเชื่อมต่อ Wi-Fi ไม่สำเร็จ

1.  **กดปุ่ม (GPIO14)** ค้างไว้ **อย่างน้อย 2 วินาที** ก่อนหรือขณะเปิดเครื่อง ESP32 (เช่น กดค้างไว้แล้วกด EN หรือเสียบสาย USB)
2.  ปล่อยปุ่มเมื่อครบเวลา ระบบจะเข้าสู่โหมดตั้งค่า Wi-Fi อัตโนมัติ
3.  ดูจาก Serial Monitor จะเห็นข้อความ: `🛠 เข้าสู่โหมดตั้งค่า WiFi ด้วยปุ่ม RESET`
4.  ESP32 จะปล่อย Wi-Fi ชื่อ `Meng-Weather-Setup`
5.  ทำตามขั้นตอนการเชื่อมต่อ Wi-Fi ใหม่อีกครั้งตามหัวข้อด้านบน

## 🔧 ฟีเจอร์เด่นในโค้ดนี้

* ⚙️ `WiFiManager` ใช้ Portal และรองรับการรีเซ็ตค่า Wi-Fi
* 💾 ส่งข้อมูลในรูปแบบ JSON ไปยัง Google Sheet
* 🔋 ประหยัดพลังงานด้วย Deep Sleep
* 📶 กดปุ่ม (GPIO14) ค้างไว้ 2 วิ → เข้าสู่โหมดตั้งค่า Wi-Fi แบบ Portal
* 📏 อ่านค่าแรงดัน กระแส และพลังงานด้วย INA219
* 🔐 มี Timeout Portal 5 นาที ป้องกันค้าง

## 📎 อัปเกรดได้ในอนาคต

* เพิ่มการวัดระดับแบตเตอรี่ เพื่อปรับรอบส่งข้อมูลตามพลังงานคงเหลือ
* เพิ่มบันทึกข้อมูลลง SD card
* เพิ่มจอ OLED แสดงข้อมูลสั้น ๆ ก่อนเข้าสู่ Deep Sleep

## 👨‍💻 ผู้พัฒนา

[Meng DIY](https://github.com/ProgrammerMuemai)

หากคุณชอบโปรเจกต์นี้ ฝาก ⭐ บน GitHub และกดติดตาม TikTok @MengDIY เพื่อดูโปรเจกต์สนุก ๆ เพิ่มเติม!
