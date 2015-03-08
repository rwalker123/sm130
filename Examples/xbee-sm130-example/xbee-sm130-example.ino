/**
 * Copyright (c) 2009 Andrew Rapp. All rights reserved.
 *
 * This file is part of XBee-Arduino.
 *
 * XBee-Arduino is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * XBee-Arduino is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XBee-Arduino.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <XBee.h>
#include <sm130.h>
#include <SoftwareSerial.h>

//Prototypes
void send_to_xbee(Tx16Request*);

/*
This example is for Series 1 XBee
Sends a TX16 or TX64 request with the value of analogRead(pin5) and checks the status response for success
Note: In my testing it took about 15 seconds for the XBee to start reporting success, so I've added a startup delay
*/

SoftwareSerial xbeeSerial(10, 9);
SoftwareSerial rfid(7, 8);

XBee xbee = XBee();
NFCReader nfc = NFCReader();

unsigned long start = millis();

int statusLed = 5;
int errorLed = 4;

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

void setup() {
  pinMode(statusLed, OUTPUT);
  pinMode(errorLed, OUTPUT);

  xbeeSerial.begin(19200);
  xbee.setSerial(xbeeSerial);
  delay(1000);

  rfid.begin(19200);
  nfc.setSerial(rfid);    
  delay(1000);

  Serial.begin(19200);
}

void loop() {

	rfid.listen(); // uno cannot listen to 2 ports at same time.

	uint8_t uid[9];
	uint8_t length;
	uint8_t tagResult = nfc.readTagID(uid, &length);
	if (tagResult == 1) {
		Serial.print(uid[0], HEX);
		Serial.print(uid[1], HEX);
		Serial.print(uid[2], HEX);
		Serial.print(uid[3], HEX);
		Serial.println();

		xbeeSerial.listen(); // uno cannot listen to 2 ports at same time.
		uint8_t payload[] = { uid[0], uid[1], uid[2], uid[3] };
		Tx16Request tx = Tx16Request(0x0001, payload, sizeof(payload));

		send_to_xbee(&tx);
	}
	else if (tagResult == 0x4E) {
		//Serial.println("No Tag Present");
	}
	else {
		Serial.print("Failed reading tag: ");
		Serial.println(tagResult);
	}

	delay(100);
}

void send_to_xbee(Tx16Request* tx)
{
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

uint8_t hi_word(uint16_t t)
{
	return (t >> 8) & 0xff;
}

uint8_t lo_word(uint16_t t)
{
	return (t & 0xff);
}

/*
void print_serial()
{
  if(flag == 1){
    //print to serial port
    Serial.print(Str1[8], HEX);
    Serial.print(Str1[7], HEX);
    Serial.print(Str1[6], HEX);
    Serial.print(Str1[5], HEX);
    Serial.println();
    delay(100);

    uint8_t payload[] = { hi_word(Str1[8]), lo_word(Str1[8]),
                          hi_word(Str1[7]), lo_word(Str1[7]), 
                          hi_word(Str1[6]), lo_word(Str1[6]), 
                          hi_word(Str1[5]), lo_word(Str1[5]) };
    Tx16Request tx = Tx16Request(0x0001, payload, sizeof(payload));

    send_to_xbee(&tx);
    
    //check_for_notag();
  }
}
*/
