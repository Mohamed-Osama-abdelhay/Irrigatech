## Irrigatech

This project demonstrates communication between multiple microcontrollers using a Master-Slave architecture with Arduinos and an ESP32. It is designed for applications such as sensor networks, automation systems, or distributed control systems where different microcontrollers must cooperate.

## 🛠 Components

- **Arduino Uno/Nano (Master)**
- **Arduino Uno/Nano (Slave)**
- **ESP32 Dev Module**
- **I2C or UART communication (between Arduino boards)**
- **Wi-Fi/Bluetooth (ESP32)**
- Optional: Sensors, actuators, or displays connected to any node

## 📁 Project Structure

- `Arduino (Master).ino`  
  Handles master logic (e.g., sends commands, requests data from slaves).

- `Arduino (Slave).ino`  
  Executes tasks when requested by the master and responds with data.

- `ESP32.ino`  
  Connects to Wi-Fi or Bluetooth to send/receive data from external sources (e.g., web server, cloud, mobile app).

## 🔌 Communication Overview

- **Arduino Master ↔ Arduino Slave**  
  Communication likely over I2C or UART. Master sends commands, slave processes and responds.

- **ESP32 ↔ Arduino Master/Slave**  
  Can interface via UART or separate logic depending on setup. ESP32 may send updates or receive instructions remotely.

## 🚀 Getting Started

### Prerequisites

- Arduino IDE or PlatformIO
- ESP32 board support installed
- USB cables to flash each microcontroller
- Necessary libraries (e.g., `Wire`, `WiFi`, `SoftwareSerial`)

### Uploading Code

1. Open `Arduino (Slave).ino` and upload to the slave Arduino.
2. Open `Arduino (Master).ino` and upload to the master Arduino.
3. Open `ESP32.ino` and upload to the ESP32 board.

> **Note:** Ensure correct COM ports and board types are selected for each upload.

## ⚙️ Configuration

You may need to adjust:

- I2C/UART pins
- Baud rates
- Wi-Fi credentials (in `ESP32.ino`)
- I2C addresses (if applicable)

## 🧪 Testing

1. Power all devices.
2. Open Serial Monitor on the master and ESP32.
3. Observe communication logs (e.g., messages being sent, sensor data relayed).

## 📝 Future Improvements

- Add error handling and retries
- Implement encryption or secure comms
- Integrate with cloud services (e.g., Firebase, MQTT)
- Add GUI (e.g., web or mobile app) for remote control

