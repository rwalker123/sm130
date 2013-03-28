#ifndef sm130_h
#define sm130_h

#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
  #else
  #include "WProgram.h"
  #endif
#include <inttypes.h>
#include <SoftwareSerial.h>

#define STANDARD_DELAY 100

enum nfc_command_t {
  NFC_NONE = 0,
  NFC_RESET = 0x80,
  NFC_GET_FIRMWARE = 0x81,
  NFC_SEEK = 0x82,
  NFC_SELECT = 0x83,
  NFC_AUTHENTICATE = 0x85,
  NFC_READ_BLOCK = 0x86,
  NFC_READ_VALUE = 0x87,
  NFC_WRITE_BLOCK = 0x89,
  NFC_WRITE_VALUE = 0x8A,
  NFC_WRITE_ULTRALIGHT = 0x8B,
  NFC_WRITE_KEY = 0x8C,
  NFC_INCREMENT = 0x8D,
  NFC_DECREMENT = 0x8E,
  NFC_ANTENNA = 0x90,
  NFC_READ_PORT = 0x91,
  NFC_WRITE_PORT = 0x92,
  NFC_HALT = 0x93,
  NFC_SET_BAUD_RATE = 0x94,
  NFC_SLEEP = 0x96,
};

enum target_t {
  PN532_MIFARE_ISO14443A = 0x01,
};

enum status_code_t {
  STATUS_SUCCESS = 0x00,
  STATUS_INVALID_RESPONSE = 0x01,
  STATUS_INVALID_KEY = 0x45,
  STATUS_IN_PROGRESS = 0x4C,
  STATUS_LOGIN_SUCCESSFUL = 0x4C,
  STATUS_HALTED = 0x4C,
  STATUS_NO_TAG = 0x4E,
  STATUS_RF_OFF = 0x55,
  STATUS_LOGIN_FAILED = 0x55,
};

class NFCReader {
private:
  SoftwareSerial _nfc;
  nfc_command_t _last_command;
  void send(nfc_command_t command, uint8_t *data, int len);
  uint8_t receive(uint8_t *data);
  uint8_t receive_tag(uint8_t *uid, uint8_t *length);
public:

  // Create a new NFC Reader
  NFCReader(int inpin, int outpin);

  // Begin communicating over UART (must be called);
  void begin();

  // Check if the adapter is available for commands
  uint8_t available();
  
  // Software reset on the RFID chip
  void reset();

  // Get the version of the firmware (generally a good test to see if UART is working)
  uint32_t getFirmwareVersion();

  // Wait until a tag enters the field and grab the details
  uint8_t readPassiveTargetID(target_t target, uint8_t *uid, uint8_t *length);

  // Print a value in hex with the '0x' appended at the front
  void PrintHex(const byte * data, const uint32_t numBytes);
};

#endif