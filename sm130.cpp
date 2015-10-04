#include "sm130.h"

#define sm130_PACKBUFFSIZE 9
uint8_t sm130_packetbuffer[sm130_PACKBUFFSIZE];


/**************************************************************************/
/*! 
    @brief  Created an NFCReader object

*/
/**************************************************************************/
NFCReader::NFCReader() {
#if defined(__AVR_ATmega32U4__) || defined(__MK20DX128__)
	_nfc = &Serial1;
#else
    _nfc = &Serial;
#endif
}

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
  _nfc->write(0xFF);

  // Write reserved
  _nfc->write((byte)0x00);

  // Write length
  _nfc->write(len + 1);

  // Write command
  _nfc->write(command);

  // Write each data bit
  for(int i = 0; i < len; i++) {
    checksum += data[i];
    _nfc->write(data[i]);
  }

  // Send up checksum
  _nfc->write(checksum);

  delay(STANDARD_DELAY);
}

/**************************************************************************/
/*! 
    @brief  Function for receiving raw data from sm130 over UART

    @param  data  Buffer to store response from server into
*/
/**************************************************************************/
uint8_t NFCReader::receive(uint8_t *data, int dataLen) {

  // Initialize the checksum
  uint8_t checksum = 0;

  // wait for data.
  while (!_nfc->available()) ;

  // Wait until we get the header byte
  while (_nfc->available()) {
	  if (_nfc->read() == 0xFF)
		  break;
  }

  // If the next byte isn't reserved, something is wrong
  if(_nfc->read() != 0x00) {
    return -1;
  }

  // Read the length byte
  int len = _nfc->read();
  
  // input buffer not large enough.
  if (dataLen < (len-1)) 
	  return -1;
  
  // Add that to the checksum
  checksum += len;

  // Read the command we're responding to
  int command_in = _nfc->read();

  // Add that to the checksum
  checksum += command_in;

  // If it's not for the command we requested, return
  if(command_in != _last_command)
    return -1;

  // Grab all the data bytes
  for(int i = 0; i < len - 1; i++) {
    data[i] = _nfc->read();
    checksum += data[i];
  }

  // Confirm the checksum
  int checksum_in = _nfc->read();
  if(checksum_in != checksum)
    return -1;
  
  return len;
}

/**************************************************************************/
/*! 
    @brief  Begins communicating on a UART channel
*/
/**************************************************************************/
void NFCReader::setSerial(Stream &serial) {

  _nfc = &serial;
}

/**************************************************************************/
/*! 
    @brief  Returns whether or not the UART connection is available
*/
/**************************************************************************/
uint8_t NFCReader::available() {
  return _nfc->available();
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
  uint8_t NFCReader::getFirmwareVersion(uint8_t *versionString, int dataLen) {

  uint32_t response;

  // Write the command to get firmware
  send(NFC_GET_FIRMWARE, 0, 0);

  // Delay for processing safety
  delay(STANDARD_DELAY);

  memset(versionString, '\0', dataLen);
  return receive(versionString, dataLen);
}

/**************************************************************************/
/*! 
    @brief   Authenticates the specified block with the specified Key type and Key
			 sequence. If Authentication fails then the Select Tag operation should be repeated to
			 authenticate again. 
*/
/**************************************************************************/
uint8_t NFCReader::authenticate(uint8_t blockNumber, uint8_t keyType, uint8_t* key) {
  uint8_t authData[8];
  authData[0] = blockNumber;
  authData[1] = keyType;
  // keys are 6 bytes
  for(int i = 0; i < 6; i++) {
	  authData[i + 2] = key[i];
  }
	  
  send(NFC_AUTHENTICATE, authData, sizeof(authData));
  
  delay(STANDARD_DELAY);
  
  int responseLen = 1;
  uint8_t response[responseLen];
  memset(response, '\0', responseLen);
  int len = receive(response, responseLen);
  // length includes command byte.
  if (len == 2) {
	  return response[0];
  }
  
  return 0xFF;
}
  
/**************************************************************************/
/*! 
    @brief  reads 16 bytes from the specified block. Before executing this command,
			the particular block should be authenticated. If not authenticated, this command will
			fail. 
*/
/**************************************************************************/  
uint8_t NFCReader::readBlock(uint8_t blockNumber, uint8_t *blockData) {

  int responseLen = 17; // max of 17 bytes, if error will response will be 1 byte.
  memset(blockData, '\0', responseLen);
  
  send(NFC_READ_BLOCK, &blockNumber, 1);
  delay(STANDARD_DELAY);
  
  // response is blockNumber (1 byte) + blockData (16 bytes)
  int len = receive(blockData, responseLen);
  // length includes command byte.
  if (len == 2) { // 2 bytes is an error (command byte + error code)
	  return blockData[0]; 
  }
  
  return 0x01;
}

/**************************************************************************/
/*! 
    @brief  reads a value block. Value is a 4byte signed integer. Before executing this
			command, the block should be authenticated. If the block is not authenticated, this
			command will fail. Also, this command will fail if the block is not in valid Value format.  
*/
/**************************************************************************/  
uint8_t NFCReader::readValueBlock(uint8_t blockNumber, int32_t *valueData) {
  
  send(NFC_READ_VALUE, &blockNumber, 1);
  delay(STANDARD_DELAY);

  int responseLen = 5; // max of 5 bytes (blockNumber + 4 byte value), if error will response will be 1 byte.  
  uint8_t response[responseLen];
  memset(response, '\0', responseLen);
  
  // response is blockNumber (1 byte) + blockData (16 bytes)
  int len = receive(response, responseLen);
  // length includes command byte.
  if (len == 2) { // 2 bytes is an error (command byte + error code)
	  return response[0]; 
  }
  else {
	// return bytes are LSB -> MSB. Index 0, is LSB. Loop will add MSB, shift right, etc.
    *valueData = 0;
  
	for(int i = 3; i >= 0; --i) {
	  *valueData <<= 8;
	  *valueData |= (response[i] & 0xFF);
	}
  }
  
  return 0x01;	
}

/*
void halt()
{
 //Halt tag
  rfid.write((uint8_t)255);
  rfid.write((uint8_t)0);
  rfid.write((uint8_t)1);
  rfid.write((uint8_t)147);
  rfid.write((uint8_t)148);
}
*/

/**************************************************************************/
/*! 
    @brief  Waits until we have a tag to report then returns the uuid

    @param  uid      Pointer a buffer that will have the uuid stored into it
    @param  length   Length in bytes
*/
/**************************************************************************/
uint8_t NFCReader::waitForTagID(uint8_t *uid, uint8_t *length) {

  // Write the command to select next tag in field
  send(NFC_SEEK, 0, 0);

  // Delay for processing safety
  delay(STANDARD_DELAY);

  // Get the response
  uint8_t retVal = receive_tag(uid, length);
  if (retVal == 0x4C) { // STATUS_NO_TAG
	delay(STANDARD_DELAY);
	// wait for the tag present.
	retVal = receive_tag(uid, length);
  }
  
  return retVal;
}

/**************************************************************************/
/*! 
    @brief  Reads tag, if not present returns error code

    @param  uid      Pointer a buffer that will have the uuid stored into it
    @param  length   Length in bytes
*/
/**************************************************************************/
uint8_t NFCReader::readTagID(uint8_t *uid, uint8_t *length) {

  // Write the command to select next tag in field
  send(NFC_SELECT, 0, 0);

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
  uint8_t len = receive(sm130_packetbuffer, sm130_PACKBUFFSIZE);

  //Serial.print("Length is: ");
  //Serial.println(len);
  //PrintHex(sm130_packetbuffer, sm130_PACKBUFFSIZE);
  
  // There seems to be a bug where only several tags will
  // be sent back unless we reset everytime. 
  //reset();  

  // If we got a response with a negative length
  // then there was an error
  if(len <= 0) {
    return 2;
  } 
  // If the length is one
  // Something weird happened
  else if(len == 1 || len == 0xFF) {
    return 3;
  } 

  // Else, parse the tag info 
  else {

    // // The size is the rest of the packet
    *length = len - 1;

	// error code
	if (*length == 1) {
		return sm130_packetbuffer[0];
	}

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


