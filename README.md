# Smart-Cycle-Sharing-IoT
An IoT-based cycle rental system built with an ESP32 microcontroller, featuring RFID access control, live GPS tracking, SOS emergency alerts, air quality monitoring, and a Firebase cloud dashboard.

# Overview
This project presents an automated, IoT-based smart cycle sharing system designed to make cycle rentals secure, safe, and user-friendly. Powered by an ESP32 microcontroller, the system uses RFID authentication to ensure only registered users can unlock and ride a cycle. Once a ride begins, it continuously monitors environmental air quality (AQI), provides real-time GPS tracking, and features an SOS button for rider safety. All ride metrics—including start/end times, duration, cost, and location—are automatically synced to a Firebase Firestore database and can be managed via a Firebase-hosted web dashboard.

# Key Features
* **RFID Authentication:** Secure access control that unlocks the cycle's servo motor only for registered users
* **Air Quality Monitoring:** Evaluates environmental conditions at the start of a ride and displays simple AQI status (e.g., "Good" or "Unhealthy")
* **GPS Tracking & SOS Alert:** Tracks live location and allows riders to send emergency SOS alerts with their GPS coordinates
* **Vehicle Indicators:** Push-button controlled left and right LED indicators that mimic real vehicle turn signals for added road safety
* **Automated Billing:** Automatically calculates ride duration and costs upon ride completion
* **Cloud Dashboard:** A web application hosted on Firebase for administrators to add users and review detailed rental histories

# Hardware Components
* ESP32 Microcontroller (Main controller & WiFi communication)
* MFRC522 RFID Module (User authentication)
* NEO-6M GPS Module (Location tracking & SOS)
* MQ135 Gas Sensor (Air quality monitoring)
* 16x2 LCD Display (Real-time user feedback)
* Servo Motor (Cycle locking mechanism)
* Push Buttons & LEDs (SOS trigger and turn indicators)

# Software & Cloud Services
* Arduino IDE (System programming)
* Firebase Firestore (Cloud database for real-time data storage)
* Firebase Hosting (Web dashboard hosting)

# System Workflow
1. **Startup:** ESP32 connects to WiFi, syncs time, and initializes the RFID, GPS, AQI, and LCD modules.
2. **Authentication:** User scans an RFID card. The ESP32 verifies the ID via Firebase. If valid, the servo unlocks the cycle.
3. **Ride Initialization:** The system records the start time and location, checks the AQI, and displays the air quality status on the LCD.
4. **In-Ride:** The system tracks the GPS location. The user can operate LED turn indicators or press the SOS button to send an emergency location alert.
5. **Completion:** The user scans the RFID card again to end the ride. The system locks the cycle, calculates the duration and fee, and uploads all final data to the Firebase database.
