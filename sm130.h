#ifndef sm130_h
#define sm130_h

#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
  #else
  #include "WProgram.h"
  #endif
#include <inttypes.h>

#define STANDARD_DELAY 100

// Format of send message:
// Header   Reserved    Length   Command    Data       CSUM
// 1 byte    1 byte     1 byte   1 byte     N bytes    1 byte
//
// Format of receive message:
// Header   Reserved    Length   Command    Response   CSUM
// 1 byte    1 byte     1 byte   1 byte     N bytes    1 byte
//
// CSUM: This is the checksum byte. This byte is used on the host as well as the module to
//       check the validity of the packet and to trap any data corruption. This is calculated by
//       adding all the bytes in the packet except the Header byte 

enum nfc_protocol_t {
  NFC_UART,
  NFC_I2C
};

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
  Stream* _nfc;
  nfc_command_t _last_command;
  const nfc_protocol_t _protocol;
  
  void send(nfc_command_t command, uint8_t *data, int len);
  uint8_t receive(uint8_t *data, int dataLen);
  uint8_t receive_tag(uint8_t *uid, uint8_t *length);
  
public:

  // Create a new NFC Reader
  NFCReader(protocol_t protocol);

  // Begin communicating over UART (must be called);
  void setSerial(Stream &serial);

  // Check if the adapter is available for commands
  uint8_t available();
  
  // Software reset on the RFID chip
  void reset();

  // Get the version of the firmware (generally a good test to see if UART is working)
  // firmware version is returned in versionString, length is the size of versionString
  uint8_t getFirmwareVersion(uint8_t *versionString, int dataLen);

  // Read a tag, first method waits for tag, second does not.
  uint8_t waitForTagID(uint8_t *uid, uint8_t *length);
  uint8_t readTagID(uint8_t *uid, uint8_t *length);

  // 
  //Key Type
  // 1 Byte – Option byte that instructs the module which type of key to be
  //          used for authentication
  //          0xAA: Authenticate with Key type A
  //          0xBB: Authenticate with Key type B
  //          0xFF: Authenticate with Key type A and transport key FF FF FF FF FF FF
  //          0x10 to 0x1F: Authenticate with Key type A using the key stored in the
  //          SM13X module’s E2PROM (0 to 15)
  //          0x20 to 0x2F: Authenticate with Key type B using the key stored in the
  //          SM13X module’s E2PROM (0 to 15)
  //          Key 6 Bytes – Key to be used for authentication. 
  // Returns:
  // 0x4C ‘L’ – Login Successful
  // 0x4E ‘N’ – No Tag present or Login Failed
  // 0x55 ‘U’ – Login Failed
  // 0x45 ‘E’ – Invalid key format in E2PROM 
  uint8_t authenticate(uint8_t blockNumber, uint8_t keyType, uint8_t* key);
  
  // reads 16 bytes from the specified block. Before executing this command,
  // the particular block should be authenticated. If not authenticated, this command will
  // fail. 
  // Note: When reading a Mifare UL tag, the first 4 bytes are from the block number specified.
  //       The next 12 bytes are from the consecutive blocks.
  // Returns:
  //  returns 0x01 on success.
  //  0x4E ‘N’ – No Tag present
  //  0x46 ‘F’ – Read Failed 
  uint8_t readBlock(uint8_t blockNumber, uint8_t *blockData);
  
  //  reads a value block. Value is a 4byte signed integer. Before executing this
  //  command, the block should be authenticated. If the block is not authenticated, this
  //  command will fail. Also, this command will fail if the block is not in valid Value format.
  // Returns:
  //  returns 0x01 on success.
  //  0x4E ‘N’ – No Tag present
  //  0x49 ‘I’ – Invalid Value Block 
  //  0x46 ‘F’ – Read Failed 
  uint8_t readValueBlock(uint8_t blockNumber, int32_t *valueData);
  
  // Print a value in hex with the '0x' appended at the front
  void PrintHex(const byte * data, const uint32_t numBytes);
};

#endif