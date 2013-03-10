#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
  #else
  #include "WProgram.h"
  #endif
#include "sm130.h"


UARTInterfaceAdapter::UARTInterfaceAdapter(int inpin, int outpin) : _nfc(inpin, outpin) {}

void UARTInterfaceAdapter::begin(int rate) {
  _nfc.begin(rate);

  // Delay so we can communicate on the new channel
  delay(500);
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
void UARTInterfaceAdapter::send(nfc_command_t command, uint8_t *data, int len) {
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

uint8_t UARTInterfaceAdapter::available() {
  return _nfc.available();
}


int UARTInterfaceAdapter::receive(nfc_command_t command, uint8_t *data) {

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
  if(command_in != command)
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

