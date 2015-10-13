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
 
// 1 - NORMAL Mode
#define RFID_TEST_MODE 2
#define XBEE_TEST_MODE 3

#define RUN_MODE 1

#define ARDUINO_AVR_MINI

#ifndef ARDUINO_AVR_MINI
#define HAS_SERIAL
#endif

#include <XBee.h>
#include <Wire.h>
#include <sm130i2c.h>

//Prototypes
void send_to_xbee(Tx16Request*);
void get_rfid_version();
uint8_t get_rfid_tag(uint8_t *uid);
void flashLed(int pin, int times, int wait);

#if RUN_MODE != RFID_TEST_MODE
#ifdef HAS_SERIAL
#include <SoftwareSerial.h>
SoftwareSerial xbeeSerial(10, 9);
#else
#define xbeeSerial Serial
#endif
XBee xbee = XBee();
#endif

#if RUN_MODE != XBEE_TEST_MODE
SM130 nfc;
#endif

#ifdef ARDUINO_AVR_MINI
int statusLed = 10;
int errorLed = 11;
#else
int statusLed = 5;
int errorLed = 4;
#endif

void debugPrint(const char *str) {
#ifdef HAS_SERIAL
  Serial.print(str);
#endif
}

void debugPrintln(const char *str = "\0") {
#ifdef HAS_SERIAL
  Serial.print(str);
  Serial.println();
#endif
}

void setup() {
  Wire.begin();

#ifdef HAS_SERIAL
  Serial.begin(115200);
#endif

#if RUN_MODE == RFID_TEST_MODE
  debugPrintln("RFID TEST Mode");
#elif RUN_MODE == XBEE_TEST_MODE
  debugPrintln("XBEE TEST Mode");
#else
  debugPrintln("Normal Mode");
#endif

  pinMode(statusLed, OUTPUT);
  pinMode(errorLed, OUTPUT);

#if RUN_MODE != RFID_TEST_MODE
  xbeeSerial.begin(19200);
  xbee.setSerial(xbeeSerial);
  delay(100);
#endif

#if RUN_MODE != RFID_TEST_MODE
    uint8_t payload[] = { 'i', 'n', 'i', 't' };
    Tx16Request tx = Tx16Request(0x0001, payload, sizeof(payload));
    send_to_xbee(&tx);
#endif

#if RUN_MODE != XBEE_TEST_MODE

  nfc.pinRESET = 0xFF;
  nfc.pinDREADY = 0xFF;
  //nfc.debug = true;
  nfc.reset();

  get_rfid_version();
  
  nfc.seekTag();
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
  //nfc.selectTag();
	if (nfc.available()) {
    if (nfc.getTagType() != 0) {
      debugPrint(nfc.getTagName());
      debugPrint(": ");
      debugPrintln(nfc.getTagString());      
#if RUN_MODE != RFID_TEST_MODE
      Tx16Request tx = Tx16Request(0x0001, nfc.getTagNumber(), nfc.getTagLength());
      send_to_xbee(&tx);
#endif
    
    }

    nfc.seekTag();

	}
#endif

	//delay(100);
}

void get_rfid_version()
{
  const char *firmwareVersion = nfc.getFirmwareVersion();
	debugPrintln(firmwareVersion);
#if RUN_MODE != RFID_TEST_MODE
  Tx16Request tx = Tx16Request(0x0001, (uint8_t*)firmwareVersion, strlen(firmwareVersion));
  send_to_xbee(&tx);
#endif
}

#if RUN_MODE != RFID_TEST_MODE
void send_to_xbee(Tx16Request* tx)
{
	//xbeeSerial.listen(); // uno cannot listen to 2 ports at same time.

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

