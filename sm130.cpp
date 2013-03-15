#include "sm130.h"

#define sm130_PACKBUFFSIZE 36
uint32_t sm130_packetbuffer[sm130_PACKBUFFSIZE];


/**************************************************************************/
/*! 
    @brief  Created an NFCReader object

    @param  inpin   Pin for receiving data from sm130
    @param  outpin  Pin for sending data to sm130
*/
/**************************************************************************/
NFCReader::NFCReader(int inpin, int outpin): _nfc(inpin, outpin) {}

/* Packet Configuration 

HEADER (ALWAYS 0xFF)
---------------------------------
RESERVED (ALWAYS 0x00)
---------------------------------
LENGTH (data length + command)
---------------------------------
COMMAND
---------------------------------
DATA
---------------------------------
CHECKSUM (All bytes except header)

*/

/**************************************************************************/
/*! 
    @brief  Function for sending raw data from sm130 over UART

    @param  command  Specific command being requested of sm130
    @param  data     Buffer to store response from server into
    @param  length   Number of bytes of data
*/
/**************************************************************************/
void NFCReader::send(nfc_command_t command, uint8_t *data, int len) {

  // Save this command
  _last_command = command;

  // Init checksum (length + command )
  uint8_t checksum = len + 1 + command;

  // Write header
  _nfc.write(0xFF);

  // Write reserved
  _nfc.write((byte)0x00);

  // Write length
  _nfc.write(len + 1);

  // Write command
  _nfc.write(command);

  // Write each data bit
  for(int i = 0; i < len; i++) {
    checksum += data[i];
    _nfc.write(data[i]);
  }

  // Send up checksum
  _nfc.write(checksum);

  delay(100);
}

/**************************************************************************/
/*! 
    @brief  Function for receiving raw data from sm130 over UART

    @param  data  Buffer to store response from server into
*/
/**************************************************************************/
uint8_t NFCReader::receive(uint32_t *data) {

  // Initialize the checksum
  uint8_t checksum = 0;

  // Wait until we get the header byte
  while(_nfc.read() != 0xFF);

  // If the next byte isn't reserved, something is wrong
  if(_nfc.read() != 0x00) {
    return -1;
  }

  // Read the length byte
  int len = _nfc.read();

  // Add that to the checksum
  checksum += len;

  // Read the command we're responding to
  int command_in = _nfc.read();

  // Add that to the checksum
  checksum += command_in;

  // If it's not for the command we requested, return
  if(command_in != _last_command)
    return -1;

  // Grab all the data bytes
  for(int i = 0; i < len - 1; i++) {
    data[i] = _nfc.read();
    checksum += data[i];
  }

  // Confirm the checksum
  int checksum_in = _nfc.read();
  if(checksum_in != checksum)
    return -1;

  return len - 1;
}

/**************************************************************************/
/*! 
    @brief  Begins communicating on a UART channel
*/
/**************************************************************************/
void NFCReader::begin() {

  // Using a constant rate to match the API
  // of Lady Adafruit's library which uses I2C
   _nfc.begin(19200);
}

/**************************************************************************/
/*! 
    @brief  Returns whether or not the UART connection is available
*/
/**************************************************************************/
uint8_t NFCReader::available() {
  return _nfc.available();
}

/**************************************************************************/
/*! 
    @brief  Performs a software reset on the device
*/
/**************************************************************************/
void NFCReader::reset() {
  // Write the reset command
  send(NFC_RESET, 0, 0);

  // Wait a bit for safety
  delay(STANDARD_DELAY);
}

/**************************************************************************/
/*! 
    @brief  Returns the components of the firmware in an int32_t
*/
/**************************************************************************/
uint32_t NFCReader::getFirmwareVersion() {

  uint32_t response;

  // Write the command to get firmware
  send(NFC_GET_FIRMWARE, 0, 0);

  // Delay for processing safety
  delay(STANDARD_DELAY);

  memset(sm130_packetbuffer, '\0',sm130_PACKBUFFSIZE);

  if (!receive(sm130_packetbuffer)) {
    return 0;
  }

  response = sm130_packetbuffer[3];
  response <<= 8;
  response |= sm130_packetbuffer[4];
  response <<= 8;
  response |= sm130_packetbuffer[5];
  response <<= 8;
  response |= sm130_packetbuffer[6];

  return response;
  
}

/**************************************************************************/
/*! 
    @brief  Waits until we have a tag to report then returns the uuid

    @param  target   The baud rate of the target. Not currently used
    @param  uid      Pointer a buffer that will have the uuid stored into it
    @param  length   Length in bytes
*/
/**************************************************************************/
uint8_t NFCReader::readPassiveTargetID(target_t target, uint8_t *uid, uint8_t *length) {

  // Write the command to select next tag in field
  send(NFC_SEEK, 0, 0);

  // Delay for processing safety
  delay(STANDARD_DELAY);

  // Get the response
  return receive_tag(uid, length);
}

/**************************************************************************/
/*! 
    @brief  Helper function to accepts a tag and returns the UUID and 
            length of the UUID

    @param  uid      Pointer a buffer that will have the uuid stored into it
    @param  length   Length in bytes
*/
/**************************************************************************/
uint8_t NFCReader::receive_tag(uint8_t *uid, uint8_t *length) {

  // Clear out the packet buffer for safety
  memset(sm130_packetbuffer, '\0', sm130_PACKBUFFSIZE);

  // Grab the response from the adapter
  uint8_t len = receive(sm130_packetbuffer);

  // There seems to be a bug where only several tags will
  // be sent back unless we reset everytime. 
  reset();  

  // If we got a response with a negative length
  // then there was an error
  if(len <= 0) {
    return 0;
  } 
  // If the length is one
  // Something weird happened
  else if(len == 1 || len == 0xFF) {
    return 0;
  } 

  // Else, parse the tag info 
  else {

    // // The size is the rest of the packet
    *length = len - 1;

    // Copy the UUID into the buffer
    for (int i = 0; i < *length; i++) {

      uid[i] = sm130_packetbuffer[i + 1];
    }

    // Return success
    return 1;
  }

  return 1;
}

/**************************************************************************/
/*! 
    @brief  Prints a hexadecimal value in plain characters

    @param  data      Pointer to the byte data
    @param  numBytes  Data length in bytes
*/
/**************************************************************************/
void NFCReader::PrintHex(const byte * data, const uint32_t numBytes)
{
  uint32_t szPos;
  for (szPos=0; szPos < numBytes; szPos++) 
  {
    Serial.print("0x");
    // Append leading 0 for small values
    if (data[szPos] <= 0xF)
      Serial.print("0");
    Serial.print(data[szPos]&0xff, HEX);
    if ((numBytes > 1) && (szPos != numBytes - 1))
    {
      Serial.print(" ");
    }
  }
  Serial.println("");
}


