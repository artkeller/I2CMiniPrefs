# I2CMiniPrefs

[![Arduino Library Manager](https://img.shields.io/static/v1?label=Arduino&message=v2.1.0&logo=arduino&logoColor=white&color=blue)](https://www.ardu-badge.com/Preferences)
[![PlatformIO Registry](https://badges.registry.platformio.org/packages/vshymanskyy/library/Preferences.svg)](https://registry.platformio.org/packages/libraries/vshymanskyy/Preferences) 

A lightweight, robust, and wear-leveling preferences library for ESP32 microcontrollers using I2C FRAM or EEPROM chips.

## 🌟 Motivation

The standard `Preferences.h` library provided for ESP32 is excellent for storing small amounts of data in the internal NVS (Non-Volatile Storage) flash memory. However, NVS, being flash-based, has inherent limitations regarding **write endurance** (the number of times a sector can be rewritten before it degrades). For applications requiring frequent small data writes, such as logging sensor readings, maintaining counters, or continuously updating configuration parameters, using internal flash can significantly reduce the lifespan of the microcontroller.

This `I2CMiniPrefs` library was developed to address this limitation by leveraging external I2C-connected **FRAM (Ferroelectric RAM)** chips. FRAM offers significantly higher write endurance (virtually unlimited for most practical applications, typically $10^{12}$ to $10^{14}$ write cycles), making it ideal for high-frequency data persistence. While it also supports EEPROM (which requires careful handling due to its lower write endurance of typically $10^{9}$ write cycles), FRAM is the primary focus and recommended memory type for this library's design.

The library implements a robust **wear-leveling mechanism** and a **garbage collection (GC)** system, ensuring that data is evenly distributed across the memory and deleted entries are reclaimed efficiently. This extends the effective lifespan of the external memory chip, especially beneficial for EEPROMs and further enhancing FRAM durability.

## ✨ Features

* **External I2C Memory Support**: Designed for I2C FRAM and EEPROM chips.
* **Wear-Leveling**: Distributes write operations evenly across the memory to extend chip lifespan.
* **Garbage Collection**: Efficiently reclaims space from deleted or updated entries.
* **CRC-based Data Integrity**: Uses CRC8 checksums for all headers (Global, Block) to ensure data integrity.
* **Key-Value Store**: Simple API for storing various data types using string keys.
* **Supported Data Types**: `bool`, `char`, `unsigned char`, `short`, `unsigned short`, `int`, `unsigned int`, `long`, `unsigned long`, `long long`, `unsigned long long`, `float`, `double`, `String`, and raw `bytes`.
* **Configurable**: Easy to configure for different memory sizes, block sizes, key lengths, and value lengths.
* **Explicit I2C Pin Assignment**: Allows specifying custom SDA/SCL pins or using board defaults.
* **I2C Clock Speed Control**: Configurable I2C bus speed (tested up to 1 MHz for FRAM).
* **Lightweight**: Designed to be compact and efficient for embedded systems.

## 🚀 Tested Environment

This library has been rigorously tested and validated on:

* **Microcontroller**: ESP32-C3
* **I2C Memory Chip**: Fujitsu MB85RC256V (256 Kbit / 32 KByte FRAM)
* **I2C Clock Speed**: Successfully tested at 1 MHz (Fast-mode Plus)

## 🛠️ Installation

1.  **Download**: Clone this repository or download it as a ZIP file.
2.  **Arduino IDE**:
    * Open your Arduino IDE.
    * Go to `Sketch > Include Library > Add .ZIP Library...` and select the downloaded ZIP file.
    * Alternatively, manually copy the `I2CMiniPrefs` folder into your Arduino `libraries` directory.
3.  **PlatformIO**:
    * Add `lib_deps = [path_to_I2CMiniPrefs]` to your `platformio.ini` or place the `I2CMiniPrefs` folder in your project's `lib` directory.

## 🔌 Wiring

Connect your I2C FRAM/EEPROM module to your ESP32-C3 as follows:

* **FRAM/EEPROM SDA** $\leftrightarrow$ **ESP32-C3 GPIO8** (or your chosen SDA pin)
* **FRAM/EEPROM SCL** $\leftrightarrow$ **ESP32-C3 GPIO9** (or your chosen SCL pin)
* **FRAM/EEPROM VCC** $\leftrightarrow$ **ESP32-C3 3.3V**
* **FRAM/EEPROM GND** $\leftrightarrow$ **ESP32-C3 GND**

**Important**: Ensure proper I2C pull-up resistors are in place on SDA and SCL lines (typically 4.7kΩ or 10kΩ to 3.3V). Most breakout boards for FRAM/EEPROM already have these integrated.

## 📝 Usage

### Include the Library

```cpp
#include "I2CMiniPrefs.h"
