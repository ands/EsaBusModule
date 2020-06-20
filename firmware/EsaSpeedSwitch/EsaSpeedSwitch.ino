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
#define LED_BUILTIN 13
#define THRESHOLD 0x80 // min: 0x2b max: 0xc5

#define RX_DISABLE UCSR0B &= ~_BV(RXEN0);
#define RX_ENABLE  UCSR0B |=  _BV(RXEN0);

uint16_t calculateChecksum(uint8_t *data)
{
	uint8_t len = data[0] + 2;
	uint16_t sum = 0;
	for(int i = 0; i < len; i++)
		sum += data[i];
	sum ^= 0xFFFF;
	return sum;
}

void setMaxSpeed(uint8_t speed)
{
	uint8_t data[] = {
		0x55, 0xAA, 0x04, 0x22, 0x01, 0xF2,
		0, 0, //rpm
		0, 0, //checksum
	};

	*(uint16_t *)&data[6] = (speed * 252) / 10;
	*(uint16_t *)&data[8] = calculateChecksum(data + 2);

	RX_DISABLE;
	Serial.write(data, sizeof(data) / sizeof(uint8_t));
	RX_ENABLE;
}

uint8_t readBlocking()
{
	while(!Serial.available()) delay(1);
	return Serial.read();
}

uint8_t receivePacket(uint8_t* throttle, uint8_t* brake)
{
	if(readBlocking() != 0x55)
		return 0;
	if(readBlocking() != 0xAA)
		return 0;

	uint8_t buff[256];

	uint8_t len = readBlocking();
	buff[0] = len;
	if(len >= sizeof(buff) - 4)
		return 0;

	uint8_t addr = readBlocking();
	buff[1] = addr;

	for(int i = 0; i < len; i++)
	{
		buff[i + 2] = readBlocking();
	}

	uint16_t actualChecksum = (uint16_t)readBlocking() | ((uint16_t)readBlocking() << 8);
	uint16_t expectedChecksum = calculateChecksum(buff);
	if(actualChecksum != expectedChecksum)
		return 0;

	uint8_t success = 0;
	switch(addr)
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
	}

	return success;
}

void setup()
{
	Serial.begin(115200);
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, HIGH);
	delay(500);
	digitalWrite(LED_BUILTIN, LOW);
}

void loop()
{
	uint8_t throttle = 0;
	uint8_t brake = 0;
	if (receivePacket(&throttle, &brake))
	{
		uint32_t now = millis();

		// Switch tuning states:
		if (!tuned)
		{
			static uint32_t startTune = UINT32_MAX;
			if (throttle > THRESHOLD &&
			       brake > THRESHOLD)
			{
				if (startTune > now)
					startTune = now;
				if ((now - startTune) > 5000)
				{
					digitalWrite(LED_BUILTIN, HIGH);

					setMaxSpeed(30);
					delay(50);
					setMaxSpeed(30);
					delay(50);
					setMaxSpeed(30);

					startTune = UINT32_MAX;
					tuned = true;

					digitalWrite(LED_BUILTIN, LOW);
				}
			}
			else
			{
				startTune = UINT32_MAX;
			}
		}
		else
		{
			static uint32_t startUntune = UINT32_MAX;
			if (throttle < THRESHOLD &&
			       brake > THRESHOLD)
			{
				if (startUntune > now)
					startUntune = now;
				if ((now - startUntune) > 1000)
				{
					digitalWrite(LED_BUILTIN, HIGH);

					setMaxSpeed(20);
					delay(50);
					setMaxSpeed(20);
					delay(50);
					setMaxSpeed(20);

					startUntune = UINT32_MAX;
					tuned = false;

					digitalWrite(LED_BUILTIN, LOW);
				}
			}
			else
			{
				startUntune = UINT32_MAX;
			}
		}
	}
}
