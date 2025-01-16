#include <WiFi.h>
#include <HTTPClient.h>
#include <SD.h>
#include <time.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* serverUrl = "http://10.0.1.108:3000/api/attendance";
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 3600;

const int buttonPin = 15;
const int sdCSPin = 5;

bool lastButtonState = HIGH;
bool currentButtonState = HIGH;

void showMessage(const String& message) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.cp437(true);

  const unsigned int charPerLine = 16;
  const int lineHeight = 10;
  const int maxLines = 6;
  int currentLine = 0;
  String msg = message;

  msg.replace("\\n", "\n");
  
  while (msg.length() > 0 && currentLine < maxLines) {
    String line;
    int newlineIndex = msg.indexOf('\n');
    
    if (newlineIndex >= 0) {
      line = msg.substring(0, newlineIndex);
      msg = msg.substring(newlineIndex + 1);
    } else {
      line = msg;
      msg = "";
    }
    
    while (line.length() > 0 && currentLine < maxLines) {
      unsigned int endIndex = (line.length() < charPerLine) ? line.length() : charPerLine;
      
      if (endIndex < line.length() && line.charAt(endIndex) != ' ') {
        int lastSpace = line.substring(0, endIndex).lastIndexOf(' ');
        if (lastSpace > 0) {
          endIndex = (unsigned int)lastSpace;
        }
      }

      display.setCursor(0, currentLine * lineHeight);
      display.println(line.substring(0, endIndex));
      
      line = line.substring(endIndex);
      line.trim();
      currentLine++;
    }
  }
  
  display.display();
}

void setup() {
  Serial.begin(115200);

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    return;
  }
  display.clearDisplay();
  
  pinMode(buttonPin, INPUT_PULLUP);
  
  showMessage("Initializing...");
  
  if (!SD.begin(sdCSPin)) {
    Serial.println("SD card initialization failed!");
    showMessage("SD Card Failed!");
    return;
  }
  Serial.println("SD card initialized.");

  showMessage("Connecting to WiFi...");
  WiFi.begin(ssid, password); 

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  showMessage("System Ready!\n\nPress button to\nmark attendance");
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
    showMessage("Sending...\nUserID: " + String(userId));
    
    HTTPClient http;
    
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");
    
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)) {
      Serial.println("Failed to obtain time");
      showMessage("Time Error!");
      return;
    }
    
    char timeStringBuff[30];
    strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
    
    String jsonPayload = "{\"data\":{\"userId\":" + String(userId) + ",\"timestamp\":\"" + String(timeStringBuff) + "\"}}";
    
    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("HTTP Response code: " + String(httpResponseCode));
      Serial.println("Response: " + response);
      
      StaticJsonDocument<200> doc;
      DeserializationError error = deserializeJson(doc, response);
      
      if (error) {
        showMessage("JSON Error\n" + String(error.c_str()));
      } else {
        const char* message = doc["data"]["message"];
        showMessage(message);
      }
    } else {
      Serial.println("Error on HTTP request");
      Serial.println("Error code: " + String(httpResponseCode));
      
      showMessage("Connection Error!\n\nCheck network");
    }

    http.end();
    
    delay(3000);
    showMessage("System Ready!\n\nPress button to mark attendance");
  }
}
