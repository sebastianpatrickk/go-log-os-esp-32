#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* serverUrl = "http://10.0.1.108:3000/api/attendance";

const int userId = 1;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password); 

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void loop() {
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

  delay(20000);
}
