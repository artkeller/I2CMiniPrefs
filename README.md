# I2CMiniPrefs

[![Arduino Library Manager](https://img.shields.io/static/v1?label=Arduino&message=v2.1.0&logo=arduino&logoColor=white&color=blue)]()
[![PlatformIO Registry](https://badges.registry.platformio.org/packages/vshymanskyy/library/Preferences.svg)]() 
[![Release](https://img.shields.io/static/v1?label=Release&message=v1.0.0&logo=arduino&logoColor=white&color=blue)]()

A lightweight, robust, and wear-leveling preferences library for ESP32 microcontrollers using I2C FRAM or EEPROM chips.

## 🌟 Motivation

The standard `Preferences.h` library provided for ESP32 is excellent for storing small amounts of data in the internal non-volatile (NVS) flash memory. However, NVS, being flash-based, has inherent limitations regarding **write endurance** (the number of times a sector can be rewritten before it degrades, typically $10^{5}$ to $10^{6}$ write cycle). Using internal flash can therefore significantly reduce the lifespan of the microcontroller for applications requiring frequent small data writes, such as logging sensor readings, maintaining counters, or continuously updating configuration parameters.

The `I2CMiniPrefs` library was developed to overcome this limitation by using external, I2C-connected **FRAM (ferroelectric random-access memory)** chips. FRAM offers significantly higher write endurance, making it ideal for high-frequency data persistence. For most practical applications, it has virtually unlimited endurance, with a typical range of $10^{12}$ to $10^{14}$ write cycles. Although it also supports EEPROM, which requires more careful handling due to its lower write endurance of typically $10^{9}$ write cycles, FRAM is the primary focus and recommended memory type for this library's design.

The library implements a robust **wear-leveling mechanism** and a **garbage collection (GC)** system, ensuring that data is evenly distributed across the memory and deleted entries are reclaimed efficiently. This extends the effective lifespan of the external memory chip, especially beneficial for EEPROMs and further enhancing FRAM durability.

Note: see https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/nvs_flash.html#


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
```

### Constructor

Initialize the `I2CMiniPrefs` object:

```cpp
I2CMiniPrefs myPrefs(MemoryType memType, uint8_t i2cAddr,
                     uint32_t totalMemoryBits, uint16_t blockSize,
                     uint8_t maxKeyLen, uint16_t maxValueLen,
                     int8_t sdaPin = -1, int8_t sclPin = -1);
```


#### Parameters:

|Parameter	|Type	Default Value	|Example (MB85RC256V)	|Description|
| :-------- | :----------------- |:--------------------- | :-------- |
|memType	MemoryType	|MEM_TYPE_EEPROM	|MEM_TYPE_FRAM	|Memory technology (FRAM or EEPROM)|
|i2cAddr	|uint8_t	0x50	|0x51	|I2C device address (usually 0x50 ... 0x57 for FRAM)|
|totalMemoryBits	|uint32_t	32 * 1024	|256 * 1024	|Total memory size in bits (256 Kbit = 32 KB)|
|blockSize	|uint16_t	256	|_128_ see 1)	|Wear-leveling block size in bytes (typ. 64-512)|
|maxKeyLen	|uint8_t	16	|8	|Maximum key string length (excl. null terminator)|
|maxValueLen	|uint16_t	240	|120	|Maximum value size in bytes|
|sdaPin	|int8_t	-1	|4	|Custom SDA GPIO (use -1 for board default)|
|sclPin	|int8_t	-1	|5	|Custom SCL GPIO (use -1 for board default)|

#### 1) Configuration Constraints:

* **`(ENTRY_HEADER_SIZE + maxKeyLen + maxValueLen) < blockSize`**
* For MB85RC256V: `(12 + 8 + 120) = 140 < 128` → _Warning triggered_
* **Solution:** Increase block size to 256 for this configuration

### Example for ESP32-C3 with MB85RC256V (256Kbit FRAM):

```cpp
// FRAM chip with custom pins (GPIO4/5)
I2CMiniPrefs myPrefs(
    MEM_TYPE_FRAM,   // Memory type
    0x51,            // I2C address
    256 * 1024,      // 256 Kbit memory
    256,             // Block size (adjusted from 128 to avoid warning)
    8,               // Max key length (8 chars)
    120,             // Max value size (120 bytes)
    4,               // SDA pin (GPIO4)
    5                // SCL pin (GPIO5)
);
```

#### Important Configuration Notes:

1. **Memory Size:**

   - Always specify bits (256 Kbit = 256 * 1024)

2. **Block Size:**

   - Must be larger than ENTRY_HEADER_SIZE + maxKeyLen + maxValueLen
   - Typical values: 128, 256, or 512 bytes
   - Larger blocks = faster writes but less efficient wear-leveling

3. **Key/Value Limits:**

   - Actual usable block space: blockSize - BLOCK_HEADER_SIZE
   - Max entries per block: (blockSize - BLOCK_HEADER_SIZE) / (ENTRY_HEADER_SIZE + avgKeyLen + avgValueLen)

4. **I2C Pins:**

   - ESP32-C3 defaults: SDA=GPIO8, SCL=GPIO9
   - Custom pins override internal pull-ups - ensure external 4.7kΩ resistors

***

#### begin()

Initializes the library and memory. Call this in `setup()`.

```cpp
bool begin();
```

Returns `true` on success, `false` otherwise. It will format the memory if the global header is invalid or missing.

#### end()

Optional: Releases I2C resources. Not strictly necessary if other libraries use I2C.

```cpp
void end();
```

#### put...() Methods (Saving Values)

The `put` methods store key-value pairs. If the key already exists, its value is updated.

```cpp
bool putBool(const char* key, bool value)
bool putChar(const char* key, char value)
bool putUChar(const char* key, unsigned char value)
bool putShort(const char* key, short value)
bool putUShort(const char* key, unsigned short value)
bool putInt(const char* key, int value)
bool putUInt(const char* key, unsigned int value)
bool putLong(const char* key, long value)
bool putULong(const char* key, unsigned long value)
bool putLong64(const char* key, long long value)
bool putULong64(const char* key, unsigned long long value)
bool putFloat(const char* key, float value)
bool putDouble(const char* key, double value)
bool putString(const char* key, const char* value)
bool putString(const char* key, const String& value)
bool putBytes(const char* key, const void* buf, size_t len)
```

Returns `true` if the value was successfully written, `false` otherwise.

#### get...() Methods (Reading Values)

The `get` methods retrieve values. If the key is not found, the `defaultValue` is returned.

```cpp
bool getBool(const char* key, bool defaultValue = false)
char getChar(const char* key, char defaultValue = 0)
unsigned char getUChar(const char* key, unsigned char defaultValue = 0)
short getShort(const char* key, short defaultValue = 0)
unsigned short getUShort(const char* key, unsigned short defaultValue = 0)
int getInt(const char* key, int defaultValue = 0)
unsigned int getUInt(const char* key, unsigned int defaultValue = 0)
long getLong(const char* key, long defaultValue = 0)
unsigned long getULong(const char* key, unsigned long defaultValue = 0)
long long getLong64(const char* key, long long defaultValue = 0)
unsigned long long getULong64(const char* key, unsigned long long defaultValue = 0)
float getFloat(const char* key, float defaultValue = 0.0f)
double getDouble(const char* key, double defaultValue = 0.0)
String getString(const char* key, const char* defaultValue = "")
size_t getBytes(const char* key, void* buf, size_t maxLen)
```

For `getBytes`, `maxLen` is the maximum number of bytes to read into `buf`. It returns the number of bytes read, or 0 if the key is not found.

#### Other Methods

* **`bool isKey(const char* key)`:** Returns true if the key exists, false otherwise.
* **`bool remove(const char* key)`:** Marks an entry as deleted. Its space will be reclaimed during the next garbage collection. Returns true on success.
* **`bool clear()`:** Clears all stored preferences. This effectively formats the memory by triggering a full garbage collection and resetting the global header.

## 🎯 I2CMiniPrefs vs. Preferences.h

While both libraries provide key-value storage, they target different use cases and hardware limitations. Below is a detailed comparison:

| Feature |	Preferences.h (ESP32 NVS)|	I2CMiniPrefs (External I2C)|	Notes |
| :------ | :----------------------- | :------------------------- | :----- |
| Storage Location	|Internal flash (NVS partition)	|External I2C FRAM/EEPROM chip	|I2CMiniPrefs avoids wearing down internal flash|
|Write Endurance	|Limited (10k-100k cycles)	|FRAM: ~10¹⁴ cycles, EEPROM: 100k-1M cycles	|I2CMiniPrefs + FRAM is ideal for high-write scenarios|
|Wear Leveling	|Basic (sector-based)	|Advanced (block-based + GC)	|I2CMiniPrefs distributes writes more evenly|
|Garbage Collection	|Automatic (background)	|On-demand (during writes when block full)	|I2CMiniPrefs GC is explicit and configurable|
|Max Key Length	|15 characters	|Configurable (up to 255 bytes)	|I2CMiniPrefs offers more flexibility|
|Max Value Size	|1984 bytes (single entry)	|Configurable (limited by block size)	|I2CMiniPrefs block size is user-defined|
|Data Types	|Basic types + blobs	|Extended types (inc. 64-bit ints)	|I2CMiniPrefs supports more native types|
|I2C Support	|❌ No	|✅ Yes (core feature)	|I2CMiniPrefs requires external memory|
|Write Speed	|Fast (~ms)	|FRAM: Fast, EEPROM: Slow (5ms/byte)	|FRAM matches internal flash speed|
|Memory Size	|Limited (typ. 1.5MB max)	|Scalable (up to MBs with larger chips)	|I2CMiniPrefs supports larger memories|
|Power Safety	|Good (journaled writes)	|FRAM: Excellent, EEPROM: Good	|FRAM has instant writes without delays|
|Custom Hardware	|❌ No	|✅ Yes (pin selection, I2C params)	|I2CMiniPrefs is hardware-flexible|
|Library Size	|~10KB (core)	|~3KB (lightweight)	|I2CMiniPrefs has smaller footprint|

#### When to Use Which

**Use `Preferences.h` when:**

* Storing infrequently changed configuration
* Working with small datasets (<100KB)
* No external hardware is available
* Maximum write cycles < 100k

**Use `I2CMiniPrefs` when:**

* High write frequency is required (sensor logging, counters)
* Using FRAM for "unlimited" write endurance
* Need larger storage than internal flash provides
* Projects require hardware flexibility (custom I2C pins)
* Preserving internal flash lifespan is critical

#### Key Architectural Differences

**1. Storage Model:**

* **`Preferences.h`:** Uses internal flash with journaling
* **`I2CMiniPrefs`:** Uses external memory with block-based wear leveling

**2. Data Structure:**

* **`Preferences.h`:** Linear storage with namespace separation
* **`I2CMiniPrefs`:** Block-chained structure with hash-based keys

**3. Wear Management:**

* **`Preferences.h`:** Relies on flash sector rotation
* **`I2CMiniPrefs`:** Implements active block rotation + GC

**4. CRC Protection:**

* **`Preferences.h`:** Basic checksum per entry
* **`I2CMiniPrefs`:** Per-header CRC8 with hash verification

## 📄 License

This project is licensed under the MIT License. See the LICENSE file for details.

## 🤝 Contributing

Contributions are welcome! Please open an issue or submit a pull request for any improvements or bug fixes.



