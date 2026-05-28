# Wonthound

Welcome to the **Wonthound** hardware project repository!  
This project contains firmware and resources for an ESP32-based display and wireless module system.

---

## Hardware Overview

**Main Board and System:**
![Main Board - Large Photo](https://github.com/Wontfallo/Wonthound/blob/main/586819740-e195ca04-e444-4971-b953-b4f7f04b68ee.png)

**ESP32 Display Module:**
![ESP32 Display](https://github.com/Wontfallo/Wonthound/blob/main/ESP32%20Display.png)

---

## Key Documentation & Downloads

- ▶️ **[2.8" ESP32-32E Display Schematic (PDF)](https://github.com/Wontfallo/Wonthound/blob/main/2.8inch_ESP32-32E_Display_Schematic.pdf)**
- ▶️ **[E01-ML01DP5 Module User Manual (PDF)](https://github.com/Wontfallo/Wonthound/blob/main/E01-ML01DP5_Usermanual_EN_V1.7.pdf)**
- 💾 **[Firmware Binary - WontHound-Freenove-E32R28T-P3-NRF24-FULL.bin](https://github.com/Wontfallo/Wonthound/blob/main/WontHound-Freenove-E32R28T-P3-NRF24-FULL.bin)**

---

## Firmware Description

The provided firmware is designed for the custom ESP32-based hardware shown above. Its main functions include:

- **Espressif ESP32 Bootloader & Application**
  - Initializes the 2.8" display hardware (via SPI or parallel interface).
  - Communicates with the nRF24-based E01-ML01DP5 wireless module for low-power 2.4GHz radio communication.
  - Manages user interface display, button input, and low-power features.
  - Handles device communication, configuration, and potential over-the-air (OTA) updates (if supported in firmware builds).
  - Provides debugging and serial log output for development.

The firmware can be installed onto the target device using standard ESP32 upload tools (esptool, Arduino, PlatformIO, etc.), with the provided binary file.

---

## Getting Started

1. **Review hardware schematics** to familiarize yourself with the connections  
   [2.8" ESP32-32E Display Schematic (PDF)](https://github.com/Wontfallo/Wonthound/blob/main/2.8inch_ESP32-32E_Display_Schematic.pdf)

2. **Consult module documentation**  
   [E01-ML01DP5 User Manual (PDF)](https://github.com/Wontfallo/Wonthound/blob/main/E01-ML01DP5_Usermanual_EN_V1.7.pdf)

3. **Flash the firmware**  
   Download the binary: [WontHound-Freenove-E32R28T-P3-NRF24-FULL.bin](https://github.com/Wontfallo/Wonthound/blob/main/WontHound-Freenove-E32R28T-P3-NRF24-FULL.bin)  
   Use your favorite ESP32 tool to upload.

---

## License

This repository is provided without an explicit open-source license. Please contact the repository owner with questions regarding usage or redistribution.

---

**Feel free to update this README with additional build steps or documentation as the project evolves!**
