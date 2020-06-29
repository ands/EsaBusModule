// EsaSpeedSwitch
//
// This is a demo application for the EsaBusModule, an Arduino Nano compatible development board.
// Its purpose is to switch the maximum speed of ESA 1919/5000 scooters between 20km/h and 30km/h.
// Copyright (C) 2020 https://github.com/ands
//
// This project is based off of the work done in https://github.com/M4GNV5/DocGreenESA5000-Dashboard.

// LICENSE
//
// This project is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This project is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this project. If not, see <https://www.gnu.org/licenses/>.

#include <Arduino.h>
#include <stdint.h>

static bool tuned = false;
static bool locked = false;

uint32_t startMillis = UINT32_MAX;
uint32_t buttonTimer = UINT32_MAX;
int buttonCount = 0;

#define LED_BUILTIN 13
#define THRESHOLD 0x80 //(128) min: 0x2b (43) max: 0xc5 (197)
#define THRESHOLD_LOW 0x40

#define SPEED_HIGH 25 // km/h
#define SPEED_LOW 20

#define COMMAND_LOCK 0x7D
#define COMMAND_SPEED 0xF2

#define RX_DISABLE UCSR0B &= ~_BV(RXEN0);
#define RX_ENABLE  UCSR0B |=  _BV(RXEN0);

uint16_t calculateChecksum(uint8_t *data)
{
	uint8_t len = data[0] + 2;
	uint16_t sum = 0;
	for (int i = 0; i < len; i++)
		sum += data[i];
	sum ^= 0xFFFF;
	return sum;
}

void sendCommand(uint8_t command, uint8_t value, uint8_t address = 0x22, int iterations = 3)
{
	uint8_t data[] = {
	  0x55, 0xAA, 0x04, address, 0x01, command,
	  0, 0, //value
	  0, 0, //checksum
	};

	if (command == COMMAND_SPEED)
		*(uint16_t *)&data[6] = (value * 252) / 10;
	else
		*(uint16_t *)&data[6] = value;

	*(uint16_t *)&data[8] = calculateChecksum(data + 2);

	RX_DISABLE;
	for (int i = 0; i < iterations; i++) {
		Serial.write(data, sizeof(data) / sizeof(uint8_t));
		delay(50);
	}
	RX_ENABLE;
}

uint8_t readBlocking()
{
	while (!Serial.available()) delay(1);
	return Serial.read();
}

uint8_t receivePacket(uint8_t* throttle, uint8_t* brake, uint8_t* dashButton)
{
	if (readBlocking() != 0x55)
		return 0;
	if (readBlocking() != 0xAA)
		return 0;

	uint8_t buff[256];

	uint8_t len = readBlocking();
	buff[0] = len;

	if (len >= sizeof(buff) - 4)
		return 0;

	uint8_t addr = readBlocking();
	buff[1] = addr;

	for (int i = 0; i < len; i++)
		buff[i + 2] = readBlocking();

	uint16_t actualChecksum = (uint16_t)readBlocking() | ((uint16_t)readBlocking() << 8);
	uint16_t expectedChecksum = calculateChecksum(buff);
	if (actualChecksum != expectedChecksum)
		return 0;

	uint8_t success = 0;

	switch (addr)
	{
	case 0x25:
	{
		switch (buff[2])
		{
		case 0x60:
		{
			*throttle = buff[5];
			*brake = buff[6];
			success = 1;
		}
		break;
		case 0x64:
		{
			*throttle = buff[6];
			*brake = buff[7];
			success = 1;
		}
		break;
		}
	}
	break;
	case 0x27:
	{
		*throttle = buff[5];
		*brake = buff[6];
		success = 1;
	}
	break;
	case 0x28:
	{
		*dashButton = buff[10];
		success = 1;
	}
	break;
	}
	return success;
}

void flashLED(int count, int duration)
{
	for (int i = 0; i < count; i++) {
		digitalWrite(LED_BUILTIN, HIGH);
		delay(duration);
		digitalWrite(LED_BUILTIN, LOW);
		delay(100);
	}
}

void setup()
{
	Serial.begin(115200);
	pinMode(LED_BUILTIN, OUTPUT);
	flashLED(1, 500);
}

void loop()
{
	uint8_t throttle = 0;
	uint8_t brake = 0;
	uint8_t dashButton = 0;

	if (receivePacket(&throttle, &brake, &dashButton)) {
		uint32_t now = millis();

		// set speed option by pushing the dashboard button several times

		if (dashButton == 0x01) {
			if (buttonTimer > now && buttonCount == 0) {
				buttonTimer = now;
			}

			buttonCount += 1;

			if (buttonCount >= 6 && 
				now - buttonTimer <= 3000) {
				if (!tuned){
					sendCommand(COMMAND_SPEED, 25);
					tuned = true;
				}
				else {
					sendCommand(COMMAND_SPEED, 20);
					tuned = false;
				}
				
				flashLED(3, 100);
				buttonTimer = UINT32_MAX;
				buttonCount = 0;
			}
		}
		else {
			if (now - buttonTimer > 3000) {
				buttonTimer = UINT32_MAX;
				buttonCount = 0;
			}
		}

		// Switch locking state:
		// Press and hold full brake & light throttle for 7 sec to lock or 10 sec to unlock the scooter
		// Switch tuning state:
		// fully press and hold both brake and throttle lever for 3 sec to set SPEED_HIGH or 1.5 sec for SPEED_LOW

		if (throttle > THRESHOLD_LOW &&
			throttle < THRESHOLD &&
			brake > THRESHOLD) {
			if (startMillis > now)
				startMillis = now;

			if (!locked) {
				if (now - startMillis > 7000) {
					sendCommand(COMMAND_LOCK, 1);
					flashLED(5, 100);
					locked = true;

					startMillis = UINT32_MAX;
				}
			}
			else {
				if (now - startMillis > 10000) {
					sendCommand(COMMAND_LOCK, 0);
					flashLED(5, 300);
					locked = false;

					startMillis = UINT32_MAX;
				}
			}
		}

		else if (throttle > THRESHOLD &&
			brake > THRESHOLD) {
			if (startMillis > now)
				startMillis = now;

			if (!tuned) {
				if (now - startMillis > 3000) {
					sendCommand(COMMAND_SPEED, SPEED_HIGH);
					flashLED(3, 100);
					tuned = true;

					startMillis = UINT32_MAX;
				}
			}
			else {
				if (now - startMillis > 1500) {
					sendCommand(COMMAND_SPEED, SPEED_LOW);
					flashLED(3, 300);
					tuned = false;

					startMillis = UINT32_MAX;
				}
			}
		}

		else {
			startMillis = UINT32_MAX;
		}
	}
}
