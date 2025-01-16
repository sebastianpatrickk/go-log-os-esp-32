#include <WiFi.h>
#include <HTTPClient.h>
#include <SD.h>

const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* serverUrl = "http://10.0.1.108:3000/api/attendance";

const int buttonPin = 15;
const int sdCSPin = 5;

bool lastButtonState = HIGH;
bool currentButtonState = HIGH;

void setup() {
  Serial.begin(115200);
  
  pinMode(buttonPin, INPUT_PULLUP);
  
  if (!SD.begin(sdCSPin)) {
    Serial.println("SD card initialization failed!");
    return;
  }
  Serial.println("SD card initialized.");

  WiFi.begin(ssid, password); 

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
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
    HTTPClient http;
    
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");
    
    String jsonPayload = "{\"data\":{\"userId\":" + String(userId) + ",\"timestamp\":" + String(millis()) + "}}";
    
    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("HTTP Response code: " + String(httpResponseCode));
      Serial.println("Response: " + response);
    } else {
      Serial.println("Error on HTTP request");
      Serial.println("Error code: " + String(httpResponseCode));
    }

    http.end();
  }
}
