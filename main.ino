#include <WiFi.h>
#include <HTTPClient.h>
#include <SD.h>
#include <time.h>
#include <LiquidCrystal_I2C.h>

const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* serverUrl = "http://10.0.1.108:3000/api/attendance";
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 3600;

const int buttonPin = 15;
const int sdCSPin = 5;

// Initialize LCD (0x27 is the default I2C address for LCD1602)
LiquidCrystal_I2C lcd(0x27, 16, 2);

bool lastButtonState = HIGH;
bool currentButtonState = HIGH;

void setup() {
  Serial.begin(115200);
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Initializing...");
  
  pinMode(buttonPin, INPUT_PULLUP);
  
  if (!SD.begin(sdCSPin)) {
    Serial.println("SD card initialization failed!");
    lcd.clear();
    lcd.print("SD Card Failed!");
    return;
  }
  Serial.println("SD card initialized.");

  lcd.clear();
  lcd.print("Connecting WiFi..");
  WiFi.begin(ssid, password); 

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  lcd.clear();
  lcd.print("Ready!");
  lcd.setCursor(0, 1);
  lcd.print("Press button...");
}

void loop() {
  currentButtonState = digitalRead(buttonPin);

  if (lastButtonState == HIGH && currentButtonState == LOW) {
    int userId = readUserIdFromSD();
    if (userId > 0) {
      sendAttendanceRequest(userId);
    }
  }
  lastButtonState = currentButtonState;
  delay(50);
}

int readUserIdFromSD() {
  File userFile = SD.open("/user.txt");
  if (!userFile) {
    Serial.println("Failed to open user.txt");
    return -1;
  }
  
  String userId = userFile.readStringUntil('\n');
  userFile.close();
  
  return userId.toInt();
}

void sendAttendanceRequest(int userId) {
  if (WiFi.status() == WL_CONNECTED) {
    lcd.clear();
    lcd.print("Sending...");
    lcd.setCursor(0, 1);
    lcd.print("UserID: " + String(userId));
    
    HTTPClient http;
    
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");
    
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)) {
      Serial.println("Failed to obtain time");
      lcd.clear();
      lcd.print("Time Error!");
      return;
    }
    
    char timeStringBuff[30];
    strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
    
    String jsonPayload = "{\"data\":{\"userId\":" + String(userId) + ",\"timestamp\":\"" + String(timeStringBuff) + "\"}}";
    
    int httpResponseCode = http.POST(jsonPayload);

    lcd.clear();
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("HTTP Response code: " + String(httpResponseCode));
      Serial.println("Response: " + response);
      
      lcd.print("Success!");
      lcd.setCursor(0, 1);
      lcd.print("Code: " + String(httpResponseCode));
    } else {
      Serial.println("Error on HTTP request");
      Serial.println("Error code: " + String(httpResponseCode));
      
      lcd.print("Error!");
      lcd.setCursor(0, 1);
      lcd.print("Code: " + String(httpResponseCode));
    }

    http.end();
    
    // Reset display after 2 seconds
    delay(2000);
    lcd.clear();
    lcd.print("Ready!");
    lcd.setCursor(0, 1);
    lcd.print("Press button...");
  }
}
