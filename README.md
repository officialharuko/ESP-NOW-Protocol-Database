Project: ESP-NOW Multi-Node Data Management System
This project consists of two ESP32 devices (ESP-A and ESP-B) that communicate using a hybrid of WiFi (HTTP Web Server) and ESP-NOW. It allows for local data logging on one device and wireless synchronization to another, complete with a web-based UI for both.

🛰️ System Overview
The system is split into two distinct roles:

ESP-A: The Data Entry & Controller
Role: Acts as the primary input node.

Web Interface: Provides a form to input names, occupations, grades, room numbers, and specific actions (e.g., "Open Door").

Storage: Saves entries locally to SPIFFS (Flash memory) in a data.json file.

Transmission: Uses ESP-NOW to "push" the stored database to ESP-B wirelessly without needing a router for the inter-device transfer.

Time Sync: Connects to WiFi to fetch real-time timestamps via NTP.

ESP-B: The Remote Viewer & Receiver
Role: Acts as the monitoring or "Guard" station.

ESP-NOW: Listens for incoming data packets and appends them to its own local people.json file.

Web Interface: Provides a searchable dashboard to view all received records and perform administrative tasks like deleting specific entries or clearing the database.

🛠️ Features
Wireless Sync: Transfer data between ESP32s using ESP-NOW (no internet required for the transfer itself).

Persistent Storage: Data survives power cycles thanks to SPIFFS.

Searchable UI: Both nodes feature a web-based search bar to filter entries by Name or Room number.

JSON Powered: Uses ArduinoJson for structured, reliable data handling.

CRUD Operations: Create, Read, and Delete entries directly from your web browser.

📋 Requirements
Hardware
2x ESP32 Development Boards.
A 2.4GHz WiFi Network (for the Web Server and NTP time).
Libraries
ArduinoJson (v6 or v7)
WiFi
WebServer
SPIFFS
esp_now

🚀 Setup Instructions
1. Configure WiFi & MAC Address
In the ESP-A code:
(A) Update ssid and password to match your router.
(B) Find the MAC Address of your ESP-B (the receiver).
Tip: Run the ESP-B code first; it prints its MAC address to the Serial Monitor.
Update the receiverAddress[] array in the ESP-A code with ESP-B's MAC address.

2. Flashing
(A) Upload the first block of code to the device you want to use as ESP-A.
(B) Upload the second block of code to the device you want to use as ESP-B.

3. Usage
(A) Open the Serial Monitor for both devices to find their IP Addresses.
(B) Navigate to ESP-A's IP in your browser to add new people/actions.
(C) On ESP-A, click "Send to ESP-B" to trigger the wireless transfer.
(D) Navigate to ESP-B's IP to see the records arrive in real-time.

📁 File Structure (SPIFFS)
/data.json or /people.json: Stores the record entries in Newline Delimited JSON format.
/id_counter.txt: (ESP-A only) Keeps track of the last used ID to ensure unique entry identification.

⚠️ Important Notes
JSON Capacity: The code uses a StaticJsonDocument<512>. If you add significantly more fields to your data, you may need to increase this value.
ESP-NOW & WiFi: Both devices are set to WIFI_STA mode. Since ESP-NOW and WiFi share the same radio channel, ensure both devices are connected to the same WiFi SSID to prevent channel-switching conflicts.
