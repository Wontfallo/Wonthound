# Wonthound

**Wonthound** my take Halehound? I wont hound you anymore for a simple 5 line pine reassignment on a closed source project largely build off an open source project. Thanks for the source code. 
This is simple addition that applies the largest impact. These are the cheapest parts from Amazon that get to your door in 2 day. The CYD is 15$ How often does anyone need or use the other RF modules. Not me. All I wanted was simple. Give me access to the pin to use the exposed SPI port for the 2.4GHz module. Look at this clean build. 

Also I was sick of needing classes to read the tiny font on the screen. Also can we calm down on the thousands of skulls? 💀
This project contains firmware and resources for an ESP32-based display and wireless module system. Simple solder one botch wire as seen in the photo and then it's plug and play. 

---

## Hardware Overview

**Main Board and System:**

![Main Board - Large Photo](https://github.com/Wontfallo/Wonthound/blob/main/586819740-e195ca04-e444-4971-b953-b4f7f04b68ee.png)

**ESP32 Display Module:**

![ESP32 Display](https://github.com/Wontfallo/Wonthound/blob/main/ESP32%20Display.png)

---

## Key Documentation & Downloads

- ▶️ **[2.8" ESP32-32E Display Schematic (PDF)](https://github.com/Wontfallo/Wonthound/blob/main/2.8inch_ESP32-32E_Display_Schematic.pdf)**
- [$15 bucks to your doorstep in 2 days AMAZON](https://www.amazon.com/dp/B0D92C9MMH?sp_csd=d2lkZ2V0TmFtZT1zcF92c2VfUlZQX2RldGFpbA)
- ▶️ **[E01-ML01DP5 Module User Manual (PDF)](https://github.com/Wontfallo/Wonthound/blob/main/E01-ML01DP5_Usermanual_EN_V1.7.pdf)**
- [$3 bucks to your doorstep in 2 days AMAZON](https://www.amazon.com/EBYTE-Wireless-E01C-2G4M27SX-Antenna-nRF24L01/dp/B0BV9CC8LW/ref=sr_1_37?rid=ABDZOQ054TTL&dib=eyJ2IjoiMSJ9.26MGA9uR3lPdQrhsTqEeQPazC85xo0q9kLzU_a5qkr4HMHVSxZn1ncQkRa36IaezVdrRUwEGR5y5qD8NFWA9AHDHNVFa7wDHgXMUGAQG23DKXE9gPE89AcmKk0-ern4HGntmOzBjBgW1NHrmWyJMjd8v6iTn7H5XyLRPMHQIfZhRAZczCUcMxlK7lnZvpS6ys7Xt9ehWa29-7fdMECJihZvspEF0prwyvPZv-uSrnwuB8I4pxqB1GQtyiIVhk-f69TRA_3JvPorSXiloJ419QMM_U5wVFXrEeU4rbMqCISY.g7fxEp5c6Fe5V6iU4XbFRDnuiatDISWWk2uvDSEv980&dib_tag=se&keywords=2.4+ghz+module&qid=1779931085&s=electronics&sprefix=2.4+ghz+module+%2Celectronics%2C162&xpid=ycpEPPumBLBg4)
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

| Step | Description / Link |
|------|--------------------|
| 1 | **Review hardware schematics**<br>[2.8" ESP32-32E Display Schematic (PDF)](https://github.com/Wontfallo/Wonthound/blob/main/2.8inch_ESP32-32E_Display_Schematic.pdf) |
| 2 | **Consult module documentation**<br>[E01-ML01DP5 User Manual (PDF)](https://github.com/Wontfallo/Wonthound/blob/main/E01-ML01DP5_Usermanual_EN_V1.7.pdf) |
| 3 | **Flash the firmware**<br>Download: [WontHound-Freenove-E32R28T-P3-NRF24-FULL.bin](https://github.com/Wontfallo/Wonthound/blob/main/WontHound-Freenove-E32R28T-P3-NRF24-FULL.bin)<br>Use your favorite ESP32 tool to upload. |

---

## License

This repository is provided without an explicit open-source license. Please contact the repository owner with questions regarding usage or redistribution.

---

**Feel free to update this README with additional build steps or documentation as the project evolves!**
