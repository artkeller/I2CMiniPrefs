/**
 * @file I2CMiniPrefs.h
 * @brief Lightweight key-value storage for I2C FRAM/EEPROM with wear-leveling
 */

#pragma once
#include <Arduino.h>
#include <Wire.h>

/**
 * @def PREFS_MAGIC
 * @brief Magic number identifying valid storage header
 */
#define PREFS_MAGIC         0xA5

/**
 * @def PREFS_VERSION
 * @brief Version of the storage format
 */
#define PREFS_VERSION       0x01

/// Block status definitions
#define BLOCK_STATUS_EMPTY      0x00 ///< Block is empty and available
#define BLOCK_STATUS_ACTIVE     0x01 ///< Currently active write block
#define BLOCK_STATUS_VALID      0x02 ///< Block contains valid data
#define BLOCK_STATUS_INVALID    0x03 ///< Block contains invalid data

/**
 * @enum PrefDataType
 * @brief Supported data types for key-value storage
 */
enum PrefDataType : uint8_t {
    TYPE_NONE = 0,           ///< Invalid type
    TYPE_BOOL,               ///< Boolean value
    TYPE_CHAR,               ///< Single character
    TYPE_UCHAR,              ///< Unsigned character
    TYPE_SHORT,              ///< Short integer
    TYPE_USHORT,             ///< Unsigned short integer
    TYPE_INT,                ///< Integer
    TYPE_UINT,               ///< Unsigned integer
    TYPE_LONG,               ///< Long integer
    TYPE_ULONG,              ///< Unsigned long integer
    TYPE_LONG64,             ///< 64-bit integer
    TYPE_ULONG64,            ///< Unsigned 64-bit integer
    TYPE_FLOAT,              ///< Floating point value
    TYPE_DOUBLE,             ///< Double precision float
    TYPE_STRING,             ///< Null-terminated string
    TYPE_BYTES               ///< Raw binary data
};

/**
 * @enum MemoryType
 * @brief Supported I2C memory types
 */
enum MemoryType {
    MEM_TYPE_EEPROM,         ///< EEPROM (requires write delays)
    MEM_TYPE_FRAM            ///< FRAM (no write delays)
};

/**
 * @struct GlobalHeader
 * @brief Header structure at memory start
 */
struct GlobalHeader {
    uint8_t  magic;          ///< Must equal PREFS_MAGIC
    uint8_t  version;        ///< Must equal PREFS_VERSION
    uint16_t totalBlocks;    ///< Total number of blocks
    uint16_t activeBlockIndex; ///< Current active block index
    uint8_t  checksum;       ///< CRC8 checksum of header
};
#define GLOBAL_HEADER_SIZE sizeof(GlobalHeader)

/**
 * @struct BlockHeader
 * @brief Header structure for each memory block
 */
struct BlockHeader {
    uint8_t  status;         ///< Block status flag
    uint16_t currentOffset;  ///< Current write offset in block
    uint8_t  checksum;       ///< CRC8 checksum of header
};
#define BLOCK_HEADER_SIZE sizeof(BlockHeader)

/**
 * @struct EntryHeader
 * @brief Header structure for key-value entries
 */
struct EntryHeader {
    uint8_t  status;         ///< 0x01=valid, 0x00=deleted
    uint8_t  dataType;       ///< PrefDataType value
    uint16_t keyHash;        ///< DJB2 hash of key
    uint8_t  keyLength;      ///< Key string length
    uint16_t valueLength;    ///< Value data length in bytes
};
#define ENTRY_HEADER_SIZE sizeof(EntryHeader)

/**
 * @class I2CMiniPrefs
 * @brief Key-value storage with wear-leveling for I2C memories
 * 
 * Implements a robust storage system with:
 * - Automatic wear-leveling
 * - Garbage collection
 * - CRC data validation
 * - Configurable memory layout
 */
class I2CMiniPrefs {
public:
    /**
     * @brief Construct a new I2CMiniPrefs object
     * @param memType Memory type (FRAM/EEPROM)
     * @param i2cAddr I2C device address (typically 0x50)
     * @param totalMemoryBits Total memory size in bits
     * @param blockSize Block size in bytes
     * @param maxKeyLen Maximum key length
     * @param maxValueLen Maximum value length
     * @param sdaPin Custom SDA pin (-1 for default)
     * @param sclPin Custom SCL pin (-1 for default)
     */
    I2CMiniPrefs(MemoryType memType = MEM_TYPE_EEPROM, uint8_t i2cAddr = 0x50,
                 uint32_t totalMemoryBits = 32 * 1024,
                 uint16_t blockSize = 256,
                 uint8_t maxKeyLen = 16, uint16_t maxValueLen = 240,
                 int8_t sdaPin = -1, int8_t sclPin = -1);

    /// @name Core Management
    ///@{
    /**
     * @brief Initialize storage system
     * @return true if successful, false on error
     */
    bool begin();
    
    /**
     * @brief Release I2C resources
     * @note Optional, only needed if I2C bus needs to be reconfigured
     */
    void end();
    ///@}
    
    /// @name Data Write Operations
    ///@{
    bool putBool(const char* key, bool value);
    bool putChar(const char* key, char value);
    bool putUChar(const char* key, unsigned char value);
    bool putShort(const char* key, short value);
    bool putUShort(const char* key, unsigned short value);
    bool putInt(const char* key, int value);
    bool putUInt(const char* key, unsigned int value);
    bool putLong(const char* key, long value);
    bool putULong(const char* key, unsigned long value);
    bool putLong64(const char* key, long long value);
    bool putULong64(const char* key, unsigned long long value);
    bool putFloat(const char* key, float value);
    bool putDouble(const char* key, double value);
    bool putString(const char* key, const char* value);
    bool putString(const char* key, const String& value);
    bool putBytes(const char* key, const void* buf, size_t len);
    ///@}
    
    /// @name Data Read Operations
    ///@{
    bool getBool(const char* key, bool defaultValue = false);
    char getChar(const char* key, char defaultValue = 0);
    unsigned char getUChar(const char* key, unsigned char defaultValue = 0);
    short getShort(const char* key, short defaultValue = 0);
    unsigned short getUShort(const char* key, unsigned short defaultValue = 0);
    int getInt(const char* key, int defaultValue = 0);
    unsigned int getUInt(const char* key, unsigned int defaultValue = 0);
    long getLong(const char* key, long defaultValue = 0);
    unsigned long getULong(const char* key, unsigned long defaultValue = 0);
    long long getLong64(const char* key, long long defaultValue = 0);
    unsigned long long getULong64(const char* key, unsigned long long defaultValue = 0);
    float getFloat(const char* key, float defaultValue = 0.0f);
    double getDouble(const char* key, double defaultValue = 0.0);
    String getString(const char* key, const char* defaultValue = "");
    size_t getBytes(const char* key, void* buf, size_t maxLen);
    ///@}
    
    /// @name Utility Operations
    ///@{
    /**
     * @brief Check if key exists
     * @param key Null-terminated key string
     * @return true if key exists, false otherwise
     */
    bool isKey(const char* key);
    
    /**
     * @brief Mark key-value pair as deleted
     * @param key Null-terminated key string
     * @return true if key was found and marked, false otherwise
     */
    bool remove(const char* key);
    
    /**
     * @brief Clear all stored preferences
     * @return true if successful, false on error
     * @note Performs full garbage collection and reformats storage
     */
    bool clear();
    ///@}

private:
    // Configuration state
    bool _isInitialized;     ///< Initialization status
    MemoryType _memoryType;  ///< Memory chip type
    uint8_t _i2cAddress;     ///< I2C device address
    uint32_t _totalMemoryBytes; ///< Total memory in bytes
    uint16_t _blockSizeBytes; ///< Block size in bytes
    uint8_t _maxKeyLength;   ///< Maximum key length
    uint16_t _maxValueLength; ///< Maximum value length
    int8_t _sdaPin;          ///< Custom SDA pin
    int8_t _sclPin;          ///< Custom SCL pin
    
    // Runtime state
    uint16_t _totalBlocks;   ///< Calculated total blocks
    uint16_t _activeBlockIndex; ///< Current active block index

    // I2C Hardware Abstraction
    void _i2c_write_byte(uint16_t address, byte data);
    byte _i2c_read_byte(uint16_t address);
    void _i2c_write_bytes(uint16_t address, const byte* data, size_t len);
    void _i2c_read_bytes(uint16_t address, byte* buffer, size_t len);

    // Core Algorithms
    uint8_t _calculateCrc8(const byte* data, size_t len);
    uint16_t _hashKey(const char* key);
    uint16_t _getBlockAddress(uint16_t blockIndex);
    bool _readGlobalHeader(GlobalHeader& header);
    bool _writeGlobalHeader(const GlobalHeader& header);
    bool _readBlockHeader(uint16_t blockIndex, BlockHeader& header);
    bool _writeBlockHeader(uint16_t blockIndex, const BlockHeader& header);
    uint16_t _findEntry(const char* key, uint16_t& entryValueAddress, 
                        uint16_t& entryValueLength, PrefDataType& entryDataType);
    bool _writeEntry(const char* key, PrefDataType type, 
                    const void* valueBuf, size_t valueLen);
    bool _markEntryAsDeleted(uint16_t entryAddress);
    bool _runGarbageCollection();

    // Template Helpers
    template<typename T>
    bool _putValue(const char* key, PrefDataType type, T value);
    
    template<typename T>
    T _getValue(const char* key, T defaultValue, PrefDataType expectedType);
    
    bool _putComplexValue(const char* key, PrefDataType type, 
                         const void* valueBuf, size_t len);
    size_t _getComplexValue(const char* key, void* buf, size_t maxLen, 
                           PrefDataType expectedType);
};