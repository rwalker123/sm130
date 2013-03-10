#include <SoftwareSerial.h>
#include <sm130.h>

UARTInterfaceAdapter adapter(7, 8);
NFCReader reader(&adapter);

void setup() {
  Serial.begin(19200);
  adapter.begin(19200);
  
  Serial.println("Initialized");

  // Print firmware version
  Serial.print("Firmware Version :");
  char buf[10];
  int len = reader.get_firmware_version((uint8_t*)buf);
  buf[len] = 0;
  Serial.println(buf);
}

void loop() {
  Tag tag;
  status_code_t stat;

  stat = reader.seek(&tag);

  if(stat == STATUS_SUCCESS) {
    Serial.print("Detected tag with type: ");
    Serial.print(tag.type, HEX);
    Serial.print(", SIZE: ");
    Serial.print(tag.serial_size, HEX);
    Serial.print(", ID: ");

    for(int i = 0; i < tag.serial_size; i++)
      Serial.print(tag.serial[i], HEX);
    Serial.println();
  }
  delay(250);
}
