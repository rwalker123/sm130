#include <SoftwareSerial.h>
#include <sm130.h>

UARTInterfaceAdapter adapter(7, 8);
NFCReader reader(&adapter);

void setup() {
  Serial.begin(19200);
  adapter.begin(19200);
  delay(500);
  Serial.println("Initialized");
  // Print firmware version
  reader.get_firmware_version();
  Serial.print("Firmware Version :");
  char buf[10];
  delay(1000);
  int len = reader.receive_raw((uint8_t*)buf);
  buf[len] = 0;
  Serial.println(buf);
}

void loop() {
  Tag tag;
  status_code_t stat;
  char *readableStatus[100];

  reader.seek();
  delay(100);
  stat = reader.receive_tag(&tag);
  delay(100);
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



