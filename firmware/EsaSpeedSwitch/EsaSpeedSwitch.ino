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

#define LED_BUILTIN 13
#define THRESHOLD_LOW 0x30 // Lever just pushed a little bit
#define THRESHOLD_HIGH 0x80 // Lever pushed about half way    min: 0x2b (43) max: 0xc5 (197)

#define SPEED_HIGH 30 // km/h
#define SPEED_MEDIUM 25
#define SPEED_LOW 20

#define ADDRESS_22 0x22
#define COMMAND_01 0x01
#define ARG_ECO 0x7C
#define ARG_LIGHT 0xF0
#define ARG_LOCK 0x7D
#define ARG_MAXSPEED 0xF2

#define RX_DISABLE UCSR0B &= ~_BV(RXEN0);
#define RX_ENABLE UCSR0B |= _BV(RXEN0);

bool easyDetune = 0;
bool dashButton = 0;
uint8_t throttle = 0;
uint8_t brake = 0;

uint32_t beginEasyDetuningMillis = 0;
uint32_t lastButtonPressMillis = 0;
uint8_t buttonCount = 0;
uint8_t oldDashButton = 0;

uint16_t calculateChecksum(uint8_t *data)
{
  uint8_t len = data[0] + 2;
  uint16_t sum = 0;
  for (int i = 0; i < len; i++)
    sum += data[i];
  sum ^= 0xFFFF;
  return sum;
}

void sendData(uint8_t address, uint8_t command, uint8_t arg, uint8_t value)
{

  uint8_t len = 0x04;
  int iterations = 3;

  uint8_t data[] = {
      0x55, 0xAA, len, address, command, arg,
      0, 0, //value
      0, 0, //checksum
  };

  if (arg == ARG_MAXSPEED)
    *(uint16_t *)&data[6] = value * 252 / 10; 
  else
    *(uint16_t *)&data[6] = value;

  *(uint16_t *)&data[8] = calculateChecksum(data + 2);

  RX_DISABLE;
  for (int i = 0; i < iterations; i++)
  {
    Serial.write(data, sizeof(data) / sizeof(uint8_t));
    delay(50);
  }
  RX_ENABLE;
}

uint8_t readBlocking()
{
  while (!Serial.available())
    delay(1);
  return Serial.read();
}

uint8_t receivePacket(uint8_t *throttle, uint8_t *brake, bool *dashButton)
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
    *dashButton = (buff[10] == 0x01);
    success = 1;
  }
  break;
  }
  return success;
}

void flashLED(int count, int pause)
{
  for (int i = 0; i < count; i++)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(pause);
    digitalWrite(LED_BUILTIN, LOW);

    if (i < (count - 1))
    {
      delay(pause);
    }
  }
}

void blinkEco()
{
  sendData(ADDRESS_22, COMMAND_01 ,ARG_ECO, 0x01);
  delay(250);
  sendData(ADDRESS_22, COMMAND_01 ,ARG_ECO, 0x00);
}

void setLock(bool lock)
{
  sendData(ADDRESS_22, COMMAND_01 ,ARG_LOCK, lock);
}

void detune()
{
  sendData(ADDRESS_22, COMMAND_01 ,ARG_MAXSPEED, SPEED_LOW);
  blinkEco();
}

void setTune()
{
  uint8_t tuneLevel;

  if (throttle >= THRESHOLD_HIGH)
    tuneLevel = SPEED_HIGH;
  
  else if (throttle >= THRESHOLD_LOW)
    tuneLevel = SPEED_MEDIUM;
  
  else
    tuneLevel = SPEED_LOW; //remove?

  sendData(ADDRESS_22,COMMAND_01 ,ARG_MAXSPEED, tuneLevel);
  blinkEco();
}

void runCommand(uint8_t command)
{
  if (command == 3) //remove?
    detune();

  if (command == 4 && brake > THRESHOLD_HIGH)
    setLock(true);

  if (command == 5)
    setTune();

  if (command == 6 && brake >= THRESHOLD_LOW && brake <= THRESHOLD_HIGH)
    setLock(false);
}

void setup()
{
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  flashLED(1, 500);
}

void loop()
{
  uint32_t now = millis();

  receivePacket(&throttle, &brake, &dashButton);

  bool dashButtonHasChanged = dashButton != oldDashButton;

  if (now - lastButtonPressMillis > 1000 && buttonCount > 0)
  {
    runCommand(buttonCount);
    buttonCount = 0;
  }

  if (dashButtonHasChanged && dashButton)
  {
    buttonCount += 1;
    lastButtonPressMillis = now;
  }

  if (throttle > THRESHOLD_HIGH && brake > THRESHOLD_HIGH)
  {
    if (!easyDetune)
    {
      beginEasyDetuningMillis = now;
    }
    easyDetune = 1;
  }
  else
  {
    easyDetune = 0;
    beginEasyDetuningMillis = 0;
  }

  if (easyDetune && now - beginEasyDetuningMillis > 3000)
  {
    detune();
    beginEasyDetuningMillis = 0;
    easyDetune = 0;
  }

  oldDashButton = dashButton;
}
