# ESP32 Attendance System

This project implements an attendance tracking system using an ESP32 microcontroller. Since Wokwi simulator doesn't support RFID readers and cards, this implementation uses an SD card to store the user ID and a push button to simulate placing an RFID card on the reader. The system also includes an OLED display for user feedback.

## Hardware Components

- ESP32 DevKit C
- SSD1306 OLED Display (128x64)
- MicroSD Card Reader (simulates RFID card storage)
- Push Button (Green) (simulates RFID card placement)
- Connecting wires

## Software Features

1. **WiFi Connectivity**

   - Connects to WiFi network for sending attendance data
   - Configurable SSID and password

2. **Time Synchronization**

   - Uses NTP server to maintain accurate time
   - Timestamps attendance records

3. **User Interface**

   - OLED display shows system status and messages
   - Push button simulates placing an RFID card
   - Clear text display with automatic word wrapping

4. **Data Management**

   - Reads user ID from SD card (simulating RFID card data)
   - Sends attendance data to server via HTTP POST
   - Handles JSON responses

5. **Error Handling**
   - SD card initialization checks
   - WiFi connection monitoring
   - JSON parsing error detection
   - Server communication error handling

## Usage

1. Store user ID in `user.txt` file on SD card (this simulates the ID stored on an RFID card)
2. Power up the system
3. Wait for "System Ready" message
4. Press button to simulate placing an RFID card and mark attendance
5. System will display confirmation or error message

## API Integration

The system communicates with a server endpoint for attendance tracking. Here are the API details:

### Endpoint

- URL: Configurable via `serverUrl` (default: "http://10.0.1.108:3000/api/attendance")
- Method: POST
- Content-Type: application/json
- Request Body:
  ```typescript
  {
    data: {
      userId: number,    // User ID from SD card
      timestamp: string  // ISO timestamp, converted to UTC
    }
  }
  ```
