// Include required libraries
#include <WiFi.h>          // For WiFi connectivity
#include <HTTPClient.h>     // For making HTTP requests
#include <SD.h>            // For SD card operations
#include <time.h>          // For time operations
#include <Wire.h>          // For I2C communication
#include <Adafruit_GFX.h>  // Graphics library for OLED
#include <Adafruit_SSD1306.h> // OLED display driver
#include <ArduinoJson.h>   // For JSON parsing and creation

// Display configuration constants
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64  
#define OLED_RESET -1  
#define SCREEN_ADDRESS 0x3C // I2C address for OLED display

// Initialize the OLED display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Network and server configuration
const char* ssid = "Wokwi-GUEST";     // WiFi network name
const char* password = "";             // WiFi password
const char* serverUrl = "http://10.0.1.108:3000/api/attendance"; // API endpoint url
const char* ntpServer = "pool.ntp.org"; // NTP server for time sync
const long gmtOffset_sec = 0;          // GMT offset (in seconds)
const int daylightOffset_sec = 3600;   // Daylight savings offset (in seconds)

// Pin definitions
const int buttonPin = 15;  // Button input pin
const int sdCSPin = 5;     // SD card chip select pin

// Button state tracking variables
bool lastButtonState = HIGH;
bool currentButtonState = HIGH;

// Function to display messages on the OLED screen with word wrapping
void showMessage(const String& message) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.cp437(true);  // Use full 256 char 'Code Page 437' font

  // Display configuration
  const unsigned int charPerLine = 16;  // Maximum characters per line
  const int lineHeight = 10;            // Height of each line in pixels
  const int maxLines = 6;               // Maximum number of lines that can be displayed
  int currentLine = 0;
  String msg = message;

  // Convert \n escape sequences to actual newlines
  msg.replace("\\n", "\n");
  
  // Process message line by line
  while (msg.length() > 0 && currentLine < maxLines) {
    String line;
    int newlineIndex = msg.indexOf('\n');
    
    // Handle explicit line breaks
    if (newlineIndex >= 0) {
      line = msg.substring(0, newlineIndex);
      msg = msg.substring(newlineIndex + 1);
    } else {
      line = msg;
      msg = "";
    }
    
    // Word wrap processing
    while (line.length() > 0 && currentLine < maxLines) {
      unsigned int endIndex = (line.length() < charPerLine) ? line.length() : charPerLine;
      
      // Find word boundary for wrapping
      if (endIndex < line.length() && line.charAt(endIndex) != ' ') {
        int lastSpace = line.substring(0, endIndex).lastIndexOf(' ');
        if (lastSpace > 0) {
          endIndex = (unsigned int)lastSpace;
        }
      }

      // Display the line
      display.setCursor(0, currentLine * lineHeight);
      display.println(line.substring(0, endIndex));
      
      line = line.substring(endIndex);
      line.trim();
      currentLine++;
    }
  }
  
  display.display();  // Update display with new content
}

// Initial setup function
void setup() {
  Serial.begin(115200);  // Initialize serial communication

  // Initialize OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    return;
  }
  display.clearDisplay();
  
  pinMode(buttonPin, INPUT_PULLUP);  // Configure button pin with internal pull-up
  
  showMessage("Initializing...");
  
  // Initialize SD card
  if (!SD.begin(sdCSPin)) {
    Serial.println("SD card initialization failed!");
    showMessage("SD Card Failed!");
    return;
  }
  Serial.println("SD card initialized.");

  // Connect to WiFi
  showMessage("Connecting to WiFi...");
  WiFi.begin(ssid, password); 

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  
  // Configure time synchronization
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  showMessage("System Ready!\n\nPress button to\nmark attendance");
}

// Main program loop
void loop() {
  currentButtonState = digitalRead(buttonPin);

  // Detect button press (falling edge)
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    int userId = readUserIdFromSD();
    if (userId > 0) {
      sendAttendanceRequest(userId);
    }
  }
  lastButtonState = currentButtonState;
  delay(50);  // Debounce delay
}

// Read user ID from SD card
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

// Send attendance data to server
void sendAttendanceRequest(int userId) {
  if (WiFi.status() == WL_CONNECTED) {
    showMessage("Sending...\nUserID: " + String(userId));
    
    HTTPClient http;
    
    // Configure HTTP request
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");
    
    // Get current time
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)) {
      Serial.println("Failed to obtain time");
      showMessage("Time Error!");
      return;
    }
    
    // Format timestamp
    char timeStringBuff[30];
    strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
    
    // Create JSON payload
    String jsonPayload = "{\"data\":{\"userId\":" + String(userId) + ",\"timestamp\":\"" + String(timeStringBuff) + "\"}}";
    
    // Send POST request
    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("HTTP Response code: " + String(httpResponseCode));
      Serial.println("Response: " + response);
      
      // Parse JSON response
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
    
    // Wait and show ready message
    delay(3000);
    showMessage("System Ready!\n\nPress button to mark attendance");
  }
}
