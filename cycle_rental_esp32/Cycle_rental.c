#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ================= WIFI =================
const char* ssid = "TT";
const char* password = "deeraj@303";

// ================= FIREBASE =================
String projectID = "smartcyclesharing";

String usersURL =
"https://firestore.googleapis.com/v1/projects/" +
projectID + "/databases/(default)/documents/users";

String historyURL =
"https://firestore.googleapis.com/v1/projects/" +
projectID + "/databases/(default)/documents/history";

String sosURL =
"https://firestore.googleapis.com/v1/projects/" +
projectID + "/databases/(default)/documents/sos";

// ================= RFID =================
#define SS_PIN   5
#define RST_PIN  22
#define SCK_PIN  18
#define MISO_PIN 19
#define MOSI_PIN 23

// ================= PINS =================
#define SERVO_PIN 14
#define LED_PIN   12
#define SOS_PIN   4

// 🚦 INDICATORS
#define LEFT_BTN   32
#define RIGHT_BTN  33
#define LEFT_LED   25
#define RIGHT_LED  26

#define AQI_PIN 34

LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================= GPS =================
TinyGPSPlus gps;
HardwareSerial gpsSerial(1);

// ================= OBJECTS =================
MFRC522 rfid(SS_PIN, RST_PIN);
Servo lockServo;

// ================= VARIABLES =================
bool rentalActive = false;
unsigned long startMillis = 0;

String currentUser = "";
String currentUID = "";

String startLocation = "";
String endLocation = "";
String startTimeStr = "";
String endTimeStr = "";

bool leftActive = false;
bool rightActive = false;

unsigned long leftStart = 0;
unsigned long rightStart = 0;

unsigned long lastBlink = 0;
bool blinkState = false;

const unsigned long INDICATOR_TIMEOUT = 15000;

// ================= SETUP =================
void setup() {

  Serial.begin(115200);

  connectWiFi();
  setupTime();

  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  rfid.PCD_Init();
  lcd.init();
  lcd.backlight();
  gpsSerial.begin(9600, SERIAL_8N1, 16, 17);

  pinMode(LED_PIN, OUTPUT);
  pinMode(SOS_PIN, INPUT_PULLUP);

  // 🚦 Indicator pins
  pinMode(LEFT_BTN, INPUT_PULLUP);
  pinMode(RIGHT_BTN, INPUT_PULLUP);
  pinMode(LEFT_LED, OUTPUT);
  pinMode(RIGHT_LED, OUTPUT);

  pinMode(AQI_PIN, INPUT);

  lockServo.attach(SERVO_PIN);
  lockServo.write(10);

  Serial.println("Smart Cycle Ready");
  displayText("Smart Cycle Ready", "");
}

// ================= LOOP =================
void loop() {

  handleIndicators(); // 🚦 NEW

  if (WiFi.status() != WL_CONNECTED)
    reconnectWiFi();

  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  if (digitalRead(SOS_PIN) == LOW) {
    handleSOS();
    delay(2000);
  }

  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  String uid = getUID();

  Serial.print("Card UID: ");
  Serial.println(uid);

  if (!rentalActive)
    verifyUserAndStart(uid);
  else
    endRental();

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  delay(1500);
}

int readAQI() {
  int value = analogRead(AQI_PIN);
  return value;
}

String getAQIStatus(int aqi) {

  if (aqi < 3000)
    return "Good";

  else if (aqi < 4000)
    return "Moderate";

  else if (aqi < 5000)
    return "Unhealthy";

  else
    return "Hazardous";
}
// ================= INDICATORS =================
void handleIndicators() {

  // LEFT BUTTON
  if (digitalRead(LEFT_BTN) == LOW && !leftActive) {
    leftActive = true;
    rightActive = false;
    leftStart = millis();

    Serial.println("Left Indicator ON");
    displayText("Left Indicator", "ON");
  }

  // RIGHT BUTTON
  if (digitalRead(RIGHT_BTN) == LOW && !rightActive) {
    rightActive = true;
    leftActive = false;
    rightStart = millis();

    Serial.println("Right Indicator ON");
    displayText("Right Indicator", "ON");
  }

  // BLINKING
  if ((leftActive || rightActive) && millis() - lastBlink > 500) {

    blinkState = !blinkState;
    lastBlink = millis();

    if (leftActive)
      digitalWrite(LEFT_LED, blinkState);

    if (rightActive)
      digitalWrite(RIGHT_LED, blinkState);
  }

  // AUTO OFF LEFT
  if (leftActive && millis() - leftStart > INDICATOR_TIMEOUT) {
    leftActive = false;
    digitalWrite(LEFT_LED, LOW);

    Serial.println("Left Indicator OFF");
    displayText("Left Indicator", "OFF");
  }

  // AUTO OFF RIGHT
  if (rightActive && millis() - rightStart > INDICATOR_TIMEOUT) {
    rightActive = false;
    digitalWrite(RIGHT_LED, LOW);

    Serial.println("Right Indicator OFF");
    displayText("Right Indicator", "OFF");
  }
}

// ================= REST OF YOUR CODE =================
// (UNCHANGED BELOW — SAME AS YOUR VERSION)

String getUID() {

  String cardID = "";

  for (byte i = 0; i < rfid.uid.size; i++)
    cardID += String(rfid.uid.uidByte[i], HEX);

  cardID.toUpperCase();
  return cardID;
}

void displayText(String line1, String line2) {
  lcd.clear();              // Clear screen
  lcd.setCursor(0, 0);      // First row
  lcd.print(line1);

  lcd.setCursor(0, 1);      // Second row
  lcd.print(line2);
}

// ================= VERIFY USER =================
void verifyUserAndStart(String scannedUID) {

  HTTPClient http;
  http.begin(usersURL);

  int code = http.GET();

  if (code <= 0) {
    Serial.println("Firebase Error");
    displayText("Firebase Error","");
    return;
  }

  String payload = http.getString();
  http.end();

  DynamicJsonDocument doc(25000);
  deserializeJson(doc, payload);

  JsonArray docs = doc["documents"];

  bool found = false;

  for (JsonObject user : docs) {

    String dbUID =
      user["fields"]["uid"]["stringValue"].as<String>();

    dbUID.toUpperCase();

    if (dbUID == scannedUID) {

      currentUser =
        user["fields"]["name"]["stringValue"].as<String>();

      currentUID = scannedUID;

      found = true;
      break;
    }
  }

  if (found) {
    Serial.println("Valid User: " + currentUser);
    displayText("Valid User: ",currentUser);
    delay(750);
    startRental();
  } else {
    Serial.println("Invalid User");
    displayText("Invalid User","");
    warningBlink();
  }
}

// ================= START RENTAL =================
void startRental() {

  if (WiFi.status() != WL_CONNECTED) {
    warningBlink();
    return;
  }

  rentalActive = true;
  startMillis = millis();

  startLocation = WiFi.SSID();
  startTimeStr = getTimeStamp();

  lockServo.write(120);
  blinkLED(2, 200);

  Serial.println("Rental Started");
  displayText("Rental Started","");

  // ===== AQI CHECK =====
  int aqiValue = readAQI();
  String airQuality = getAQIStatus(aqiValue);

  Serial.print("AQI Value: ");
  Serial.println(aqiValue);

  Serial.print("Air Quality: ");
  Serial.println(airQuality);

  // Show on LCD
  displayText("Air: " + airQuality, "AQI:" + String(aqiValue));
  delay(2000);
}

// ================= END RENTAL =================
void endRental() {

  if (WiFi.status() != WL_CONNECTED) {
    warningBlink();
    return;
  }

  rentalActive = false;
  lockServo.write(10);

  unsigned long minutes =
    (millis() - startMillis) / 60000;

  float fee = minutes * 5;

  uploadHistory(String(minutes), fee);

  blinkLED(1, 800);

  Serial.println("Rental Ended");
  displayText("Rental Ended","");
  delay(750);
  displayText("Duration: ","");
  delay(750);
  lcd.print(minutes);
  delay(750);
  displayText("Fee: ","");
  delay(750);
  lcd.print(fee);
  delay(750);
}

// ================= UPLOAD HISTORY =================
void uploadHistory(String duration, float fee) {

  HTTPClient http;

  http.begin(historyURL);
  http.addHeader("Content-Type", "application/json");

  endLocation = WiFi.SSID();
  endTimeStr = getTimeStamp();

  String json =
  "{ \"fields\": {"
  "\"uid\":{\"stringValue\":\"" + currentUID + "\"},"
  "\"name\":{\"stringValue\":\"" + currentUser + "\"},"
  "\"startLocation\":{\"stringValue\":\"" + startLocation + "\"},"
  "\"endLocation\":{\"stringValue\":\"" + endLocation + "\"},"
  "\"startTime\":{\"stringValue\":\"" + startTimeStr + "\"},"
  "\"endTime\":{\"stringValue\":\"" + endTimeStr + "\"},"
  "\"duration\":{\"stringValue\":\"" + duration + "\"},"
  "\"fee\":{\"doubleValue\":" + String(fee) + "}"
  "} }";

  int res = http.POST(json);

  Serial.print("History Upload: ");
  Serial.println(res);
  displayText("History Uploaded","");

  http.end();
}

// ================= SOS =================
void handleSOS() {

  Serial.println("SOS Triggered");
  displayText("SOS Triggered","");
  if (!gps.location.isValid()) {
    Serial.println("GPS Not Ready");
    displayText("GPS Not Ready","");
    warningBlink();
    return;
  }

  float lat = gps.location.lat();
  float lng = gps.location.lng();

  sendSOS(lat, lng);

  blinkLED(3, 200);
}

// ================= SEND SOS =================
void sendSOS(float lat, float lng) {

  HTTPClient http;

  http.begin(sosURL);
  http.addHeader("Content-Type", "application/json");

  String timeNow = getTimeStamp();

  String json =
  "{ \"fields\": {"
  "\"uid\":{\"stringValue\":\"" + currentUID + "\"},"
  "\"name\":{\"stringValue\":\"" + currentUser + "\"},"
  "\"latitude\":{\"doubleValue\":" + String(lat, 6) + "},"
  "\"longitude\":{\"doubleValue\":" + String(lng, 6) + "},"
  "\"time\":{\"stringValue\":\"" + timeNow + "\"}"
  "} }";

  int res = http.POST(json);

  Serial.print("SOS Upload: ");
  Serial.println(res);

  http.end();
}

// ================= WIFI =================
void connectWiFi() {

  Serial.print("Connecting WiFi");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected");
}

void reconnectWiFi() {
  WiFi.disconnect();
  WiFi.begin(ssid, password);
  delay(3000);
}

// ================= TIME =================

String getTimeStamp() {

  struct tm timeinfo;

  if (!getLocalTime(&timeinfo))
    return "NA";

  char buffer[30];

  strftime(buffer, sizeof(buffer),
           "%Y-%m-%d %H:%M:%S",
           &timeinfo);

  return String(buffer);
}

// ================= LED =================
void blinkLED(int t, int d) {
  for (int i = 0; i < t; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(d);
    digitalWrite(LED_PIN, LOW);
    delay(200);
  }
}

void warningBlink() {
  for (int i = 0; i < 5; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
  }
}
// ================= TIME =================
void setupTime() {

  configTime(19800, 0, "pool.ntp.org");

  struct tm timeinfo;
  int retry = 0;

  displayText("Syncing Time", "");

  while (!getLocalTime(&timeinfo) && retry < 20) {
    lcd.setCursor(retry % 16, 1);
    lcd.print(".");
    delay(500);
    retry++;
  }

  if (retry >= 20) {
    Serial.println("Time Failed");
    displayText("Time Failed", "");
  } else {
    Serial.println("Time Synced");
    displayText("Time Synced", "");
  }

  delay(1500);
}