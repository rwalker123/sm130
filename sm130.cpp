#include "sm130.h"


NFCReader::NFCReader(IInterfaceAdapter *adapter) : _adapter(adapter){}

void NFCReader::write_raw(nfc_command_t command, uint8_t *buf, int len) {

  // Save this command so we can reference when we receive the response
  _last_command = command;

  // Send the command to the sm130 using the delegated adaptor class
  _adapter->send(command, buf, len);
}

void NFCReader::reset() {
  // Write the reset command
  write_raw(NFC_RESET, 0, 0);

  // Wait a bit for safety
  delay(STANDARD_DELAY);
}

int NFCReader::get_firmware_version(uint8_t *buf) {

  // Write the command to get firmware
  write_raw(NFC_GET_FIRMWARE, 0, 0);

  // Delay for processing safety
  delay(STANDARD_DELAY);

  // Get the response
  return receive_raw(buf);
}

status_code_t NFCReader::select(Tag *tag) {

  // Write the command to select current tag in field
  write_raw(NFC_SELECT, 0, 0);

  // Delay for processing safety
  delay(STANDARD_DELAY);


  // Get the response
  return receive_tag(tag);

}

status_code_t NFCReader::seek(Tag *tag) {

  // Write the command to select next tag in field
  write_raw(NFC_SEEK, 0, 0);

  // Delay for processing safety
  delay(STANDARD_DELAY);

  // Get the response
  return receive_tag(tag);
}

int NFCReader::receive_raw(uint8_t *buf) {
  // Delegate receiving to implemented adapter class
  return _adapter->receive(_last_command, buf);
}


status_code_t NFCReader::receive_tag(Tag *tag) {

  // Create a buffer for the response
  uint8_t buf[8];

  // Grab the response from the adapter
  int len = receive_raw(buf);

  // If we got a response with a negative length
  // then there was an error
  if(len < 0) {
    return STATUS_INVALID_RESPONSE;
  } 
  // If the length is one
  // Something weird happened
  else if(len == 1) {
    return (status_code_t)buf[0];
  } 

  // Else, parse the tag info 
  else {
    // Tag type is the first byte
    tag->type = (tag_type_t)buf[0];

    // The size is the rest of the packet
    tag->serial_size = len - 1;

    // Copy the UUID into the buffer
    memcpy(tag->serial, buf + 1, tag->serial_size);

    // Return success
    return STATUS_SUCCESS;
  }
}