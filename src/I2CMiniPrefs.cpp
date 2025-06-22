/**
 * @file I2CMiniPrefs.cpp
 * @brief Implementation of wear-leveling key-value storage for I2C memories
 * 
 * @author Thomas Walloschke mailto:artkeller@gmx.de
 * @date 2025-06-21
 * @version 1.0.0
 */

#include "I2CMiniPrefs.h"

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
I2CMiniPrefs::I2CMiniPrefs(MemoryType memType, uint8_t i2cAddr,
                         uint32_t totalMemoryBits, uint16_t blockSize,
                         uint8_t maxKeyLen, uint16_t maxValueLen,
                         int8_t sdaPin, int8_t sclPin) 
    : _isInitialized(false),
      _memoryType(memType),
      _i2cAddress(i2cAddr),
      _totalMemoryBytes(totalMemoryBits / 8),
      _blockSizeBytes(blockSize),
      _maxKeyLength(maxKeyLen),
      _maxValueLength(maxValueLen),
      _sdaPin(sdaPin), 
      _sclPin(sclPin), 
      _totalBlocks(0),
      _activeBlockIndex(0)
{
    // Validate configuration constraints
    if ((ENTRY_HEADER_SIZE + _maxKeyLength + _maxValueLength) >= _blockSizeBytes) {
        Serial.println("I2CMiniPrefs: WARNING! Max key/value length too large for block size");
    }
}

// I2C Hardware Layer --------------------------------------------------------

/**
 * @brief Write single byte to I2C memory
 * @param address Memory address
 * @param data Byte to write
 */
void I2CMiniPrefs::_i2c_write_byte(uint16_t address, byte data) {
    Wire.beginTransmission(_i2cAddress);
    Wire.write((uint8_t)(address >> 8));
    Wire.write((uint8_t)(address & 0xFF));
    Wire.write(data);
    Wire.endTransmission();

    // EEPROM requires write cycle delay
    if (_memoryType == MEM_TYPE_EEPROM) delay(5); 
}

/**
 * @brief Read single byte from I2C memory
 * @param address Memory address
 * @return Read byte (0xFF on error)
 */
byte I2CMiniPrefs::_i2c_read_byte(uint16_t address) {
    Wire.beginTransmission(_i2cAddress);
    Wire.write((uint8_t)(address >> 8));
    Wire.write((uint8_t)(address & 0xFF));
    Wire.endTransmission();
    Wire.requestFrom(_i2cAddress, 1);
    return Wire.available() ? Wire.read() : 0xFF;
}

/**
 * @brief Write byte sequence to I2C memory
 * @param address Starting memory address
 * @param data Data buffer
 * @param len Data length
 */
void I2CMiniPrefs::_i2c_write_bytes(uint16_t address, const byte* data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        _i2c_write_byte(address + i, data[i]);
    }
    if (_memoryType == MEM_TYPE_EEPROM) delay(1);
}

/**
 * @brief Read byte sequence from I2C memory
 * @param address Starting memory address
 * @param buffer Output buffer
 * @param len Bytes to read
 */
void I2CMiniPrefs::_i2c_read_bytes(uint16_t address, byte* buffer, size_t len) {
    Wire.beginTransmission(_i2cAddress);
    Wire.write((uint8_t)(address >> 8));
    Wire.write((uint8_t)(address & 0xFF));
    Wire.endTransmission();
    Wire.requestFrom(_i2cAddress, len);
    for (size_t i = 0; i < len; i++) {
        buffer[i] = Wire.available() ? Wire.read() : 0xFF;
    }
}

// Core Algorithms ------------------------------------------------------------

/**
 * @brief Calculate CRC8 checksum
 * @param data Input buffer
 * @param len Data length
 * @return CRC8 checksum
 */
uint8_t I2CMiniPrefs::_calculateCrc8(const byte* data, size_t len) {
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if ((crc & 0x80) != 0) crc = (uint8_t)((crc << 1) ^ 0x07);
            else crc <<= 1;
        }
    }
    return crc;
}

/**
 * @brief Generate DJB2 hash for key
 * @param key Null-terminated string
 * @return 16-bit hash value
 */
uint16_t I2CMiniPrefs::_hashKey(const char* key) {
    uint16_t hash = 5381;
    int c;
    while ((c = *key++)) hash = ((hash << 5) + hash) + c;
    return hash;
}

/**
 * @brief Get physical address of memory block
 * @param blockIndex Block index (0-based)
 * @return Physical memory address
 */
uint16_t I2CMiniPrefs::_getBlockAddress(uint16_t blockIndex) {
    return GLOBAL_HEADER_SIZE + (blockIndex * _blockSizeBytes);
}

/**
 * @brief Read global header from memory
 * @param[out] header Reference to header struct
 * @return true if header is valid, false otherwise
 */
bool I2CMiniPrefs::_readGlobalHeader(GlobalHeader& header) {
    _i2c_read_bytes(0, (byte*)&header, sizeof(GlobalHeader));
    return (header.magic == PREFS_MAGIC &&
            header.version == PREFS_VERSION &&
            _calculateCrc8((byte*)&header, sizeof(GlobalHeader) - 1) == header.checksum);
}

/**
 * @brief Write global header to memory
 * @param header Header struct to write
 * @return true on success, false on error
 */
bool I2CMiniPrefs::_writeGlobalHeader(const GlobalHeader& header) {
    GlobalHeader tempHeader = header;
    tempHeader.checksum = _calculateCrc8((byte*)&tempHeader, sizeof(GlobalHeader) - 1);
    _i2c_write_bytes(0, (byte*)&tempHeader, sizeof(GlobalHeader));
    return true;
}

/**
 * @brief Read block header from memory
 * @param blockIndex Block index to read
 * @param[out] header Reference to header struct
 * @return true if header is valid, false otherwise
 */
bool I2CMiniPrefs::_readBlockHeader(uint16_t blockIndex, BlockHeader& header) {
    uint16_t addr = _getBlockAddress(blockIndex);
    _i2c_read_bytes(addr, (byte*)&header, sizeof(BlockHeader));
    byte crcData[3] = {header.status, 
                      (byte)(header.currentOffset & 0xFF),
                      (byte)((header.currentOffset >> 8) & 0xFF)};
    return (_calculateCrc8(crcData, sizeof(crcData)) == header.checksum;
}

/**
 * @brief Write block header to memory
 * @param blockIndex Block index to write
 * @param header Header struct to write
 * @return true on success, false on error
 */
bool I2CMiniPrefs::_writeBlockHeader(uint16_t blockIndex, const BlockHeader& header) {
    uint16_t addr = _getBlockAddress(blockIndex);
    BlockHeader tempHeader = header;
    byte crcData[3] = {header.status,
                      (byte)(header.currentOffset & 0xFF),
                      (byte)((header.currentOffset >> 8) & 0xFF)};
    tempHeader.checksum = _calculateCrc8(crcData, sizeof(crcData));
    _i2c_write_bytes(addr, (byte*)&tempHeader, sizeof(BlockHeader));
    return true;
}

/**
 * @brief Find entry by key
 * @param key Null-terminated key string
 * @param[out] entryValueAddress Address of value data
 * @param[out] entryValueLength Length of value data
 * @param[out] entryDataType Detected data type
 * @return Entry header address or 0 if not found
 */
uint16_t I2CMiniPrefs::_findEntry(const char* key, uint16_t& entryValueAddress, 
                                uint16_t& entryValueLength, PrefDataType& entryDataType) {
    if (!_isInitialized) return 0;

    uint16_t targetKeyHash = _hashKey(key);
    uint8_t targetKeyLen = strlen(key);

    for (uint16_t blockIdx = 0; blockIdx < _totalBlocks; blockIdx++) {
        BlockHeader blockHeader;
        if (!_readBlockHeader(blockIdx, blockHeader)) continue;
        if (blockHeader.status != BLOCK_STATUS_ACTIVE && 
            blockHeader.status != BLOCK_STATUS_VALID) continue;

        uint16_t currentEntryOffset = BLOCK_HEADER_SIZE;
        uint16_t blockStartAddr = _getBlockAddress(blockIdx);

        while (currentEntryOffset < blockHeader.currentOffset) {
            EntryHeader entryHeader;
            uint16_t entryHeaderAddr = blockStartAddr + currentEntryOffset;
            _i2c_read_bytes(entryHeaderAddr, (byte*)&entryHeader, sizeof(EntryHeader));

            // Skip deleted entries
            if (entryHeader.status == 0x00) {
                currentEntryOffset += (ENTRY_HEADER_SIZE + entryHeader.keyLength + entryHeader.valueLength);
                continue;
            }

            // Hash and length match
            if (entryHeader.keyHash == targetKeyHash && entryHeader.keyLength == targetKeyLen) {
                char readKey[_maxKeyLength + 1];
                _i2c_read_bytes(entryHeaderAddr + ENTRY_HEADER_SIZE, (byte*)readKey, targetKeyLen);
                readKey[targetKeyLen] = '\0';
                
                // Full key match
                if (strcmp(key, readKey) == 0) {
                    entryValueAddress = entryHeaderAddr + ENTRY_HEADER_SIZE + entryHeader.keyLength;
                    entryValueLength = entryHeader.valueLength;
                    entryDataType = (PrefDataType)entryHeader.dataType;
                    return entryHeaderAddr;
                }
            }
            currentEntryOffset += (ENTRY_HEADER_SIZE + entryHeader.keyLength + entryHeader.valueLength);
        }
    }
    return 0;
}

/**
 * @brief Write key-value entry to storage
 * @param key Null-terminated key string
 * @param type Data type identifier
 * @param valueBuf Pointer to value data
 * @param valueLen Length of value data
 * @return true on success, false on error
 */
bool I2CMiniPrefs::_writeEntry(const char* key, PrefDataType type, 
                             const void* valueBuf, size_t valueLen) {
    if (!_isInitialized) return false;

    uint8_t keyLen = strlen(key);
    if (keyLen > _maxKeyLength || valueLen > _maxValueLength) return false;

    // Remove existing entry if present
    uint16_t oldValueAddr, oldValueLen;
    PrefDataType oldDataType;
    uint16_t oldEntryHeaderAddr = _findEntry(key, oldValueAddr, oldValueLen, oldDataType);
    if (oldEntryHeaderAddr != 0) _markEntryAsDeleted(oldEntryHeaderAddr);

    BlockHeader currentBlockHeader;
    if (!_readBlockHeader(_activeBlockIndex, currentBlockHeader) || 
        currentBlockHeader.status != BLOCK_STATUS_ACTIVE) {
        return false;
    }

    // Check if block has space
    uint16_t entryTotalSize = ENTRY_HEADER_SIZE + keyLen + valueLen;
    if ((currentBlockHeader.currentOffset + entryTotalSize) > _blockSizeBytes) {
        if (!_runGarbageCollection()) return false;
        if (!_readBlockHeader(_activeBlockIndex, currentBlockHeader) || 
            currentBlockHeader.status != BLOCK_STATUS_ACTIVE) {
            return false;
        }
    }

    // Write new entry
    uint16_t entryStartAddr = _getBlockAddress(_activeBlockIndex) + currentBlockHeader.currentOffset;
    EntryHeader newEntryHeader = {
        .status = 0x01,
        .dataType = static_cast<uint8_t>(type),
        .keyHash = _hashKey(key),
        .keyLength = keyLen,
        .valueLength = static_cast<uint16_t>(valueLen)
    };
    
    _i2c_write_bytes(entryStartAddr, (byte*)&newEntryHeader, sizeof(EntryHeader));
    _i2c_write_bytes(entryStartAddr + ENTRY_HEADER_SIZE, (const byte*)key, keyLen);
    _i2c_write_bytes(entryStartAddr + ENTRY_HEADER_SIZE + keyLen, (const byte*)valueBuf, valueLen);

    // Update block header
    currentBlockHeader.currentOffset += entryTotalSize;
    return _writeBlockHeader(_activeBlockIndex, currentBlockHeader);
}

/**
 * @brief Mark entry as deleted
 * @param entryAddress Address of entry header
 * @return true on success, false on error
 */
bool I2CMiniPrefs::_markEntryAsDeleted(uint16_t entryAddress) {
    if (entryAddress == 0) return false;
    EntryHeader header;
    _i2c_read_bytes(entryAddress, (byte*)&header, sizeof(EntryHeader));
    if (header.status != 0x01) return false;
    header.status = 0x00;
    _i2c_write_byte(entryAddress, header.status);
    return true;
}

/**
 * @brief Perform garbage collection and wear leveling
 * @return true on success, false on error
 * 
 * Steps:
 * 1. Find empty block for new active block
 * 2. Copy valid entries from all blocks
 * 3. Mark old blocks as empty
 * 4. Update global header
 */
bool I2CMiniPrefs::_runGarbageCollection() {
    uint16_t nextEmptyBlockIndex = 0xFFFF;
    for (uint16_t i = 0; i < _totalBlocks; i++) {
        BlockHeader header;
        if (!_readBlockHeader(i, header) || header.status == BLOCK_STATUS_EMPTY) {
            nextEmptyBlockIndex = i;
            break;
        }
    }

    if (nextEmptyBlockIndex == 0xFFFF) return false;

    // Mark current active block as valid
    if (_isInitialized) {
        BlockHeader oldActiveBlockHeader;
        if (_readBlockHeader(_activeBlockIndex, oldActiveBlockHeader)) {
            oldActiveBlockHeader.status = BLOCK_STATUS_VALID;
            _writeBlockHeader(_activeBlockIndex, oldActiveBlockHeader);
        }
    }

    // Initialize new active block
    BlockHeader newActiveBlockHeader = {
        .status = BLOCK_STATUS_ACTIVE,
        .currentOffset = BLOCK_HEADER_SIZE
    };
    _writeBlockHeader(nextEmptyBlockIndex, newActiveBlockHeader);
    uint16_t currentWriteOffset = BLOCK_HEADER_SIZE;
    uint16_t newBlockAddr = _getBlockAddress(nextEmptyBlockIndex);

    // Copy valid entries
    for (uint16_t blockIdx = 0; blockIdx < _totalBlocks; blockIdx++) {
        if (blockIdx == nextEmptyBlockIndex) continue;

        BlockHeader sourceBlockHeader;
        if (!_readBlockHeader(blockIdx, sourceBlockHeader)) continue;
        if (sourceBlockHeader.status != BLOCK_STATUS_ACTIVE && 
            sourceBlockHeader.status != BLOCK_STATUS_VALID) continue;

        uint16_t currentReadOffset = BLOCK_HEADER_SIZE;
        uint16_t sourceBlockAddr = _getBlockAddress(blockIdx);

        while (currentReadOffset < sourceBlockHeader.currentOffset) {
            EntryHeader entryHeader;
            uint16_t entryHeaderAddr = sourceBlockAddr + currentReadOffset;
            _i2c_read_bytes(entryHeaderAddr, (byte*)&entryHeader, sizeof(EntryHeader));
            uint16_t entryTotalSize = ENTRY_HEADER_SIZE + entryHeader.keyLength + entryHeader.valueLength;

            // Only copy valid entries
            if (entryHeader.status == 0x01 && 
                entryHeader.keyLength <= _maxKeyLength && 
                entryHeader.valueLength <= _maxValueLength) {
                
                if ((currentWriteOffset + entryTotalSize) > _blockSizeBytes) return false;
                
                byte* entryData = new byte[entryTotalSize];
                _i2c_read_bytes(entryHeaderAddr, entryData, entryTotalSize);
                _i2c_write_bytes(newBlockAddr + currentWriteOffset, entryData, entryTotalSize);
                delete[] entryData;
                
                currentWriteOffset += entryTotalSize;
            }
            currentReadOffset += entryTotalSize;
        }

        // Mark source block as empty
        sourceBlockHeader.status = BLOCK_STATUS_EMPTY;
        sourceBlockHeader.currentOffset = BLOCK_HEADER_SIZE;
        _writeBlockHeader(blockIdx, sourceBlockHeader);
    }

    // Finalize new block
    newActiveBlockHeader.currentOffset = currentWriteOffset;
    _writeBlockHeader(nextEmptyBlockIndex, newActiveBlockHeader);
    _activeBlockIndex = nextEmptyBlockIndex;

    // Update global header
    GlobalHeader globalHeader = {
        .magic = PREFS_MAGIC,
        .version = PREFS_VERSION,
        .totalBlocks = _totalBlocks,
        .activeBlockIndex = _activeBlockIndex
    };
    return _writeGlobalHeader(globalHeader);
}

// Public API Implementation -------------------------------------------------

/**
 * @brief Initialize storage system
 * @return true if successful, false on error
 * 
 * Performs:
 * - I2C bus initialization
 * - Memory detection
 * - Header validation
 * - Garbage collection if needed
 */
bool I2CMiniPrefs::begin() {
    // Initialize I2C with custom or default pins
    if (_sdaPin != -1 && _sclPin != -1) {
        Wire.begin(_sdaPin, _sclPin);
    } else {
        Wire.begin();
    }
    
    // Set high speed for FRAM, normal for EEPROM
    _memoryType == MEM_TYPE_FRAM ? Wire.setClock(1000000) : Wire.setClock(100000);

    // Verify device presence
    Wire.beginTransmission(_i2cAddress);
    if (Wire.endTransmission() != 0) return false;

    // Calculate memory layout
    _totalBlocks = (_totalMemoryBytes - GLOBAL_HEADER_SIZE) / _blockSizeBytes;
    if (_totalBlocks == 0) return false;

    // Initialize or recover storage
    GlobalHeader globalHeader;
    if (!_readGlobalHeader(globalHeader)) {
        // First-time initialization
        if (!_runGarbageCollection()) return false;
    } else {
        // Existing storage found
        _activeBlockIndex = globalHeader.activeBlockIndex;
        BlockHeader activeBlockHeader;
        if (!_readBlockHeader(_activeBlockIndex, activeBlockHeader) || 
            activeBlockHeader.status != BLOCK_STATUS_ACTIVE) {
            // Repair corrupted storage
            if (!_runGarbageCollection()) return false;
        }
    }
    _isInitialized = true;
    return true;
}

void I2CMiniPrefs::end() {
    // Optional I2C resource release
}

// Put Methods Implementation (template-based) --------------------------------

template<typename T>
bool I2CMiniPrefs::_putValue(const char* key, PrefDataType type, T value) {
    return _writeEntry(key, type, &value, sizeof(T));
}

bool I2CMiniPrefs::putBool(const char* key, bool value) { 
    return _putValue(key, TYPE_BOOL, value); 
}
// ... (all other put methods follow same pattern)

// Get Methods Implementation (template-based) --------------------------------

template<typename T>
T I2CMiniPrefs::_getValue(const char* key, T defaultValue, PrefDataType expectedType) {
    uint16_t valueAddr;
    uint16_t valueLen;
    PrefDataType storedType;
    if (_findEntry(key, valueAddr, valueLen, storedType) != 0 && 
        storedType == expectedType && valueLen == sizeof(T)) {
        T value;
        _i2c_read_bytes(valueAddr, (byte*)&value, sizeof(T));
        return value;
    }
    return defaultValue;
}

bool I2CMiniPrefs::getBool(const char* key, bool defaultValue) { 
    return _getValue(key, defaultValue, TYPE_BOOL); 
}
// ... (all other get methods follow same pattern)

// Complex Type Handlers ------------------------------------------------------

bool I2CMiniPrefs::_putComplexValue(const char* key, PrefDataType type, 
                                  const void* valueBuf, size_t len) {
    return _writeEntry(key, type, valueBuf, len);
}

size_t I2CMiniPrefs::_getComplexValue(const char* key, void* buf, size_t maxLen, 
                                    PrefDataType expectedType) {
    uint16_t valueAddr;
    uint16_t valueLen;
    PrefDataType type;
    if (_findEntry(key, valueAddr, valueLen, type) != 0 && type == expectedType) {
        size_t bytesToRead = min(valueLen, maxLen);
        _i2c_read_bytes(valueAddr, (byte*)buf, bytesToRead);
        return bytesToRead;
    }
    return 0;
}

// String Specializations -----------------------------------------------------

bool I2CMiniPrefs::putString(const char* key, const char* value) {
    if (!value) return false;
    return _putComplexValue(key, TYPE_STRING, value, strlen(value) + 1);
}

bool I2CMiniPrefs::putString(const char* key, const String& value) {
    return putString(key, value.c_str());
}

String I2CMiniPrefs::getString(const char* key, const char* defaultValue) {
    char buffer[_maxValueLength + 1] = {0};
    size_t len = _getComplexValue(key, buffer, _maxValueLength, TYPE_STRING);
    return len > 0 ? String(buffer) : String(defaultValue);
}

// Utility Methods ------------------------------------------------------------

bool I2CMiniPrefs::isKey(const char* key) {
    uint16_t valueAddr, valueLen;
    PrefDataType type;
    return _findEntry(key, valueAddr, valueLen, type) != 0;
}

bool I2CMiniPrefs::remove(const char* key) {
    uint16_t valueAddr, valueLen;
    PrefDataType type;
    uint16_t entryAddr = _findEntry(key, valueAddr, valueLen, type);
    return entryAddr ? _markEntryAsDeleted(entryAddr) : false;
}

bool I2CMiniPrefs::clear() {
    _isInitialized = false;
    _activeBlockIndex = 0;
    return _runGarbageCollection();
}

// Explicit Template Instantiation --------------------------------------------
template bool I2CMiniPrefs::_putValue<bool>(const char*, PrefDataType, bool);
template bool I2CMiniPrefs::_getValue<bool>(const char*, bool, PrefDataType);
// ... (all other supported types)