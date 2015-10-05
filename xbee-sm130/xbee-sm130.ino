/**
 * Copyright (c) 2015 Ray Walker. All rights reserved.
 *
 * This file is part of SM130.
 *
 * SM130 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SM130 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
 
#include <XBee.h>
#include <sm130.h>
#include <SoftwareSerial.h>

// 1 - NORMAL Mode
#define RFID_TEST_MODE 2
#define XBEE_TEST_MODE 3

#define RUN_MODE 1

//Prototypes
void send_to_xbee(Tx16Request*);
void get_rfid_version();
uint8_t get_rfid_tag(uint8_t *uid);
void flashLed(int pin, int times, int wait);

#if RUN_MODE != RFID_TEST_MODE
SoftwareSerial xbeeSerial(10, 9);
XBee xbee = XBee();
#endif

#if RUN_MODE != XBEE_TEST_MODE
SoftwareSerial rfid(7, 8);
NFCReader nfc = NFCReader();
#endif


int statusLed = 5;
int errorLed = 4;

void setup() {
  Serial.begin(19200);
  delay(100);

#if RUN_MODE == RFID_TEST_MODE
  Serial.print("RFID TEST Mode");
#elif RUN_MODE == XBEE_TEST_MODE
  Serial.print("XBEE TEST Mode");
#else
  Serial.print("Normal Mode");
#endif

  Serial.println();
  
  pinMode(statusLed, OUTPUT);
  pinMode(errorLed, OUTPUT);

#if RUN_MODE != RFID_TEST_MODE
  xbeeSerial.begin(19200);
  xbee.setSerial(xbeeSerial);
  delay(100);
#endif

#if RUN_MODE != XBEE_TEST_MODE
  rfid.begin(19200);
  nfc.setSerial(rfid);    
  delay(100);

  get_rfid_version();
#endif

}

void loop() {
#if RUN_MODE == XBEE_TEST_MODE
    uint8_t payload[] = { 'a', 'b', 'c', 'd' };
    Tx16Request tx = Tx16Request(0x0002, payload, sizeof(payload));
    send_to_xbee(&tx);

    uint8_t payload2[] = { 'e', 'f', 'g', 'h' };
    Tx16Request tx2 = Tx16Request(0x0001, payload2, sizeof(payload));
    send_to_xbee(&tx2);

    delay(1000);
#else
	uint8_t uid[9];
	uint8_t tagResult = get_rfid_tag(uid);
	if (tagResult == 1) {
		Serial.print(uid[0], HEX);
		Serial.print(uid[1], HEX);
		Serial.print(uid[2], HEX);
		Serial.print(uid[3], HEX);
		Serial.println();

#if RUN_MODE != RFID_TEST_MODE
		uint8_t payload[] = { uid[0], uid[1], uid[2], uid[3] };
		Tx16Request tx = Tx16Request(0x0001, payload, sizeof(payload));

		send_to_xbee(&tx);
#endif
	}
	else if (tagResult == 0x4E) {
		//Serial.println("No Tag Present");
	}
	else {
		Serial.print("Failed reading tag: ");
		Serial.println(tagResult);
	}

	delay(100);
#endif
}

void get_rfid_version()
{
	Serial.print("Firmware version: ");
  rfid.listen();
	int len = 10;
	uint8_t firmwareVersion[len];
	nfc.getFirmwareVersion(firmwareVersion, len);
  Serial.println((const char *)firmwareVersion);
}

uint8_t get_rfid_tag(uint8_t *uid)
{
	rfid.listen(); // uno cannot listen to 2 ports at same time.

	uint8_t length;
	return nfc.readTagID(uid, &length);
}

#if RUN_MODE != RFID_TEST_MODE
void send_to_xbee(Tx16Request* tx)
{
	xbeeSerial.listen(); // uno cannot listen to 2 ports at same time.

	xbee.send(*tx);

	// flash TX indicator
	flashLed(statusLed, 1, 200);

	xbee.readPacket();
	XBeeResponse& response = xbee.getResponse();

	// after sending a tx request, we expect a status response
	// wait up to 5 seconds for the status response
	if (xbee.getResponse().isAvailable()) {
		// got a response!

		// should be a znet tx status            	
		if (response.getApiId() == TX_STATUS_RESPONSE) {

			TxStatusResponse txStatus;
			response.getZBTxStatusResponse(txStatus);

			// get the delivery status, the fifth byte
			if (txStatus.getStatus() == SUCCESS) {
				// success.  time to celebrate
				flashLed(statusLed, 5, 50);
			}
			else {
				// the remote XBee did not receive our packet. is it powered on?
				flashLed(errorLed, 3, 500);
			}
		}
	}
	else if (response.isError()) {
		//nss.print("Error reading packet.  Error code: ");  
		//nss.println(xbee.getResponse().getErrorCode());
		// or flash error led
		flashLed(errorLed, 4, 100);
	}
	else {
		flashLed(errorLed, 2, 50);
	}
}
#endif

uint8_t hi_word(uint16_t t)
{
	return (t >> 8) & 0xff;
}

uint8_t lo_word(uint16_t t)
{
	return (t & 0xff);
}

void flashLed(int pin, int times, int wait) {

	for (int i = 0; i < times; i++) {
		digitalWrite(pin, HIGH);
		delay(wait);
		digitalWrite(pin, LOW);

		if (i + 1 < times) {
			delay(wait);
		}
	}
}

