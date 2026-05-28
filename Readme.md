# 🐺 Wonthound

> **My take on Halehound** — I won't hound you anymore for a simple 5-line pin reassignment on a closed-source project largely built off an open-source project. Thanks for the source code. 🙏

[![Platform](https://img.shields.io/badge/platform-ESP32-blue.svg)](https://www.espressif.com/)
[![Display](https://img.shields.io/badge/display-2.8%22%20CYD-orange.svg)](https://github.com/Wontfallo/Wonthound)
[![ESP32-S3 Touch](https://img.shields.io/badge/ESP32--S3-Capacitive%20Touch%20Screen-blueviolet?style=for-the-badge&logo=espressif&logoColor=white)](https://www.amazon.com/Capacitive-Supporting-XiaoZhiAI-Dual-core-Microcontroller/dp/B0FSQF6FKN)
[![Radio](https://img.shields.io/badge/radio-nRF24L01-green.svg)](https://github.com/Wontfallo/Wonthound)
[![Status](https://img.shields.io/badge/status-plug%20%26%20play-brightgreen.svg)](https://github.com/Wontfallo/Wonthound)
[![Solder](https://img.shields.io/badge/solder-1%20botch%20wire-red.svg)](https://github.com/Wontfallo/Wonthound)

---

##  Why Wonthound?

This is a **simple addition that delivers the largest impact**. Built from the cheapest parts on Amazon, delivered in 2 days:

- The **2.4 GHz $3 bucks NO EXTRA CAP NEEDED! Why? Because this is the correct way.** — how often does anyone actually use the other RF modules? Not me.
- All I wanted was simple: **access to the pin** to use the exposed SPI port for the 2.4GHz module.
- Also, I was sick of needing reading glasses to see the tiny font on the screen 🔎.
- And can we **💀💀calm down on the thousands of skulls?💀💀**

  ### NEW   [ESP32-S3 support added](https://www.amazon.com/dp/B0FSQLPQ6M/?ref_=cm_wl_huc_item)
  **I also added a build for the S3 capacitive touch screen with voice commmands.**


This project contains firmware and resources for an ESP32-based display and wireless module system. **Solder one botch wire** as seen in the photo — then it's plug and play.

---

## 🛠️ Hardware Overview

### Main Board & System
![Main Board](https://github.com/Wontfallo/Wonthound/blob/main/586819740-e195ca04-e444-4971-b953-b4f7f04b68ee.png)

### ESP32 Display Module
![ESP32 Display](https://github.com/Wontfallo/Wonthound/blob/main/ESP32%20Display.png)

---

## 📚 Key Documentation, Downloads & Parts

| # | Resource | Description | Link |
|---|----------|-------------|------|
| 📄 | **ESP32-32E Display Schematic** | 2.8" CYD pinout & wiring reference | [Download PDF](https://github.com/Wontfallo/Wonthound/blob/main/2.8inch_ESP32-32E_Display_Schematic.pdf) |
| 🛒 | **2.8" CYD Display ESP32-E SPI exposed** | $15 — to your doorstep in 2 days | [Buy on Amazon](https://www.amazon.com/dp/B0D92C9MMH) |
| 📄 | **E01-ML01DP5 User Manual** | nRF24 2.4GHz module documentation | [Download PDF](https://github.com/Wontfallo/Wonthound/blob/main/E01-ML01DP5_Usermanual_EN_V1.7.pdf) |
| 🛒 | **E01-ML01DP5 Module** | $3 — to your doorstep in 2 days | [Buy on Amazon](https://www.amazon.com/EBYTE-Wireless-E01C-2G4M27SX-Antenna-nRF24L01/dp/B0BV9CC8LW) |
| 💾 | **Firmware Binary** | `WontHound-Freenove-E32R28T-P3-NRF24-FULL.bin` | [Download BIN](https://github.com/Wontfallo/Wonthound/blob/main/WontHound-Freenove-E32R28T-P3-NRF24-FULL.bin) |

> 💡 **Total build cost: ~$18 shipped.** That's cheaper than lunch.

---

## ⚙️ Firmware Description

The provided firmware is designed for the custom ESP32-based hardware shown above. Its main functions include:

### 🚀 Espressif ESP32 Bootloader & Application
- 🖥️ Initializes the 2.8" display hardware (via SPI or parallel interface)
- 📡 Communicates with the nRF24-based **E01-ML01DP5** wireless module for low-power 2.4GHz radio
- 🎛️ Manages UI display, button input, and low-power features
- 🔄 Handles device communication, configuration, and OTA updates (when supported)
- 🐛 Provides debugging and serial log output for development

The firmware can be flashed onto the target device using any standard ESP32 upload tool — `esptool`, Arduino IDE, PlatformIO, etc.

---

## 🚦 Getting Started

### 1️⃣ Review the Hardware Schematics
Grab the schematic from the table above and study the pinout.

### 2️⃣ Consult the Module Documentation
Read the E01-ML01DP5 user manual to understand the radio module.

### 3️⃣ Solder the Botch Wire
One wire. That's it. See the photo above. 🔧

### 4️⃣ Flash the Firmware
Grab the binary from the table above, then flash:

```bash
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 921600 write_flash 0x0 WontHound-Freenove-E32R28T-P3-NRF24-FULL.bin
```

### 5️⃣ Power it up and enjoy 🎉

---

## 🧰 Tech Stack

| Component | Spec |
|-----------|------|
| 🧠 MCU | ESP32 (Freenove E32R28T-P3) |
| 🖼️ Display | 2.8" CYD (Cheap Yellow Display) |
| 📻 Radio | E01-ML01DP5 (nRF24L01+ based, 2.4GHz) |
| 🔌 Interface | SPI |
| ⚡ Power | USB 5V |

---

## 📜 License

This repository is provided **without an explicit open-source license**. Please contact the repository owner with questions regarding usage or redistribution.
Who are you kidding?  I've spent 15 years bit banging low level hardware long before the ESP32 even exsisted PIC32. I vibe coded in 5 minutes! 

---

## 🙌 Contributing

Got improvements? Build steps? Better docs? **PRs welcome.**
Feel free to fork, it from hack, and smash it to your liking like I did and make it your own!

---

<div align="center">

**Built with ☕, frustration, and one botch wire.**

⭐ *If this saved you from the skull spam, drop a star.* ⭐

</div>
