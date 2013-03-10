#include <SoftwareSerial.h>
#include <sm130.h>

// Create an adapter using pins 7 & 8
// (7 & 8 required for sm130 shield by SparkFun)
UARTInterfaceAdapter adapter(7, 8);

// Create an NFC reader using that UART adapter
NFCReader reader(&adapter);

// Funtion Prototypes
void printHex(Tag tag);
void reportTagToNodeServer(Tag tag);

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

  // If there is no buffer, it couldn't get the firmware version
  if (!buf) {
    Serial.println("Did not find sm130 board");

    // Loop forever
    while (1);
  }
  
  // Set the end of the response with a null bit to denote
  // the end of a string
  buf[len] = 0;
  
  // Print out the version
  Serial.println(buf);

  Serial.println("Ready to read cards...");
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
    reportTagToNodeServer(tag);
  }
  
  // Sleep for several seconds so we don't keep picking up the same card
  delay(3000);
}

void reportTagToNodeServer(Tag tag) {
  Serial.println("Found an ISO14443A card");
  Serial.print("  UID Length: ");Serial.print(tag.serial_size, DEC);Serial.println(" bytes");
  Serial.print("  UID Value: ");
  printHex(tag);
  Serial.println(""); 
}

void printHex(Tag tag)
{
  const uint8_t *data = tag.serial;
  const uint32_t numBytes = tag.serial_size;
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