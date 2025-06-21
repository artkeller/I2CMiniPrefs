/**
 * @file BasicTest.inop
 * @brief Example sketch demonstrating the usage of the I2CMiniPrefs library.
 *
 * This sketch provides a comprehensive test of the I2CMiniPrefs library's
 * functionality, including initialization, saving, reading, updating,
 * removing, and clearing configuration values on an I2C FRAM chip.
 *
 * It has been specifically tested and validated on an ESP32-C3 microcontroller
 * with a Fujitsu MB85RC256V FRAM chip (256 Kbit / 32 KByte) connected via I2C.
 *
 * @author Thomas Walloschke mailto:artkeller@gmx.de
 * @date 2025-06-21
 * @version 1.0.0
 */

#include "I2CMiniPrefs.h" // Include the I2CMiniPrefs library header

// =========================================================================
// I2C Memory Chip Configuration Selection:
// Choose and uncomment the appropriate configuration for your setup.
// =========================================================================

/**
 * @brief I2CMiniPrefs instance configured for ESP32-C3 with default I2C pins (GPIO8/9).
 *
 * - MEM_TYPE_FRAM: Specifies the memory type as FRAM (no write delays needed).
 * - 0x50: The I2C address of the FRAM chip (standard for many FRAM modules).
 * - 256 * 1024: Total memory size in bits (256 Kbit = 32 KByte for MB85RC256V).
 * - 128: Block size in bytes. This defines the segment size for wear-leveling and garbage collection.
 * - 8: Maximum key string length (excluding null terminator).
 * - 120: Maximum value data length in bytes.
 * - -1, -1: Use the board's default I2C SDA/SCL pins. For ESP32-C3, these are typically GPIO8 (SDA) and GPIO9 (SCL).
 *
 * Note: While GPIO8/9 are default I2C pins on the C3, watch out for any
 * debug messages related to USB_DM/DP, which might indicate pin conflicts
 * if USB JTAG/Serial is active. Ensure these pins are exclusively used for I2C.
 */
I2CMiniPrefs myPrefs(MEM_TYPE_FRAM, 0x50, 256 * 1024, 128, 8, 120, 8, 9); 

/**
 * @brief Alternative I2CMiniPrefs instance using custom I2C pins (e.g., GPIO4 and GPIO5) for ESP32-C3.
 * Uncomment this line and comment out the above if you want to use different pins.
 */
//I2CMiniPrefs myPrefs(MEM_TYPE_FRAM, 0x50, 256 * 1024, 128, 8, 120, 4, 5);

/**
 * @brief Example for a classic ESP32 (e.g., ESP32-DevKitC) with standard I2C pins (GPIO21/22).
 * This example uses MEM_TYPE_EEPROM, which requires small delays after write operations.
 * Uncomment if you are targeting a standard ESP32 and an EEPROM chip.
 */
// I2CMiniPrefs myPrefs(MEM_TYPE_EEPROM, 0x50, 32 * 1024, 64, 16, 200, 21, 22);

/**
 * @brief Example for using board default pins without explicit specification.
 * This will use `Wire.begin()` without parameters. Note that this might not
 * resolve USB-related pin conflicts if they arise from other board configurations.
 * Uncomment if you prefer to rely on board defaults.
 */
// I2CMiniPrefs myPrefs(MEM_TYPE_EEPROM, 0x50, 32 * 1024, 64, 16, 200);


/**
 * @brief Arduino setup function.
 * Initializes serial communication and the I2CMiniPrefs library, then
 * demonstrates various read/write operations.
 */
void setup() {
  Serial.begin(115200); // Initialize serial communication at 115200 baud
  delay(2000); // Small delay to allow serial monitor to connect
  unsigned long start = millis();
  while (!Serial && (millis() - start) < 5000) {} // Wait for serial connection

  Serial.println("Initialisiere I2CMiniPrefs..."); // Inform about library initialization
  /**
   * @brief Call the begin method to initialize the I2CMiniPrefs library.
   * This step performs I2C bus setup, checks for the FRAM chip,
   * and initializes/formats the memory if necessary.
   */
  if (myPrefs.begin()) {
    Serial.println("I2CMiniPrefs erfolgreich initialisiert."); // Success message
  } else {
    Serial.println("Fehler bei der Initialisierung von I2CMiniPrefs!"); // Error message
    Serial.println("Bitte ueberpruefe die I2C-Verkabelung, die I2C-Adresse und die im Konstruktor gewaehlten Groessen.");
    Serial.println("Stelle sicher, dass die SDA/SCL-Pins korrekt sind und nicht anderweitig verwendet werden.");
    while (true); // Halt execution if initialization fails
  }

  // --- Example: Saving Configuration Values ---
  Serial.println("\n--- Speichere Konfiguration ---");
  myPrefs.putInt("sensorID", 42); // Store an integer value
  myPrefs.putFloat("tempOff", 1.5f); // Store a float value
  myPrefs.putString("devName", "ESP32C3"); // Store a string value
  myPrefs.putBool("debug", true); // Store a boolean value
  myPrefs.putLong64("uptime", 1234567890LL); // Store a long long value

  // --- Example: Reading Configuration Values ---
  Serial.println("\n--- Lese Konfiguration ---");
  int id = myPrefs.getInt("sensorID", -1); // Read integer, provide default if key not found
  float offset = myPrefs.getFloat("tempOff", 0.0f); // Read float
  String name = myPrefs.getString("devName", "Unknown"); // Read String
  bool debug = myPrefs.getBool("debug", false); // Read boolean
  long long uptime = myPrefs.getLong64("uptime", 0LL); // Read long long

  Serial.print("Sensor ID: "); Serial.println(id);
  Serial.print("Temp Offset: "); Serial.println(offset, 2); // Print float with 2 decimal places
  Serial.print("Device Name: "); Serial.println(name);
  Serial.print("Debug Mode: "); Serial.println(debug ? "True" : "False");
  Serial.print("Uptime (ms): "); Serial.println((long)uptime); // Cast to long for printing large numbers

  // --- Example: Updating a Value ---
  Serial.println("\n--- Aktualisiere Sensor ID ---");
  myPrefs.putInt("sensorID", 43); // Update an existing integer value
  Serial.print("Neue Sensor ID: "); Serial.println(myPrefs.getInt("sensorID", -1)); // Verify update

  // --- Example: Removing a Value ---
  Serial.println("\n--- Entferne 'tempOff' ---");
  myPrefs.remove("tempOff"); // Remove a key-value pair
  Serial.print("Temp Offset nach Entfernen (sollte Default sein): "); Serial.println(myPrefs.getFloat("tempOff", 99.9f)); // Confirm removal by reading default value

  // --- Example: Clearing All Settings ---
  Serial.println("\n--- Loeche alle Einstellungen ---");
  myPrefs.clear(); // Clear all stored preferences
  Serial.print("Sensor ID nach Clear (sollte Default sein): "); Serial.println(myPrefs.getInt("sensorID", -1)); // Confirm default
  Serial.print("Device Name nach Clear (sollte Default sein): "); Serial.println(myPrefs.getString("devName", "NoName")); // Confirm default

  myPrefs.end(); // Optional: Release I2C resources. Not strictly necessary if other libs use I2C.
}

/**
 * @brief Arduino loop function.
 * This example sketch does not perform any actions in the main loop.
 */
void loop() {
  // Nothing in the loop for this example
}