#include <SoftwareSerial.h>
#include <sm130.h>

// Create an adapter using pins 7 & 8
// (7 & 8 required for sm130 shield by SparkFun)
UARTInterfaceAdapter adapter(7, 8);

// Create an NFC reader using that UART adapter
NFCReader reader(&adapter);

void setup() {
  
  // Start the UART serial ports
  Serial.begin(19200);
  adapter.begin(19200);
  
  Serial.println("Initialized");

  // Print firmware version
  Serial.print("Firmware Version :");
  
  // Create a response buffer
  char buf[10];
  
  // Grab the firmware version
  int len = reader.get_firmware_version((uint8_t*)buf);
  
  // Set the end of the response with a null bit to denote
  // the end of a string
  buf[len] = 0;
  
  // Print out the version
  Serial.println(buf);
}

void loop() {
  // Create a tag for the response
  Tag tag;

  // Create a status code for the response
  status_code_t stat;
  
  // Wait until we get a tag
  stat = reader.seek(&tag);

  // If was a successful read
  if(stat == STATUS_SUCCESS) {
    
    // Print out the details
    Serial.print("Detected tag with type: ");
    Serial.print(tag.type, HEX);
    Serial.print(", SIZE: ");
    Serial.print(tag.serial_size, HEX);
    Serial.print(", ID: ");

    // Print out the UUID
    for(int i = 0; i < tag.serial_size; i++)
      Serial.print(tag.serial[i], HEX);
    Serial.println();
  }
  
  // Sleep for several seconds so we don't keep picking up the same card
  delay(3000);
}