/*
 Control outdoor lights with PWM fading.

 created 2011
 by Kyle Gordon <kyle@lodge.glasgownet.com>

 http://lodge.glasgownet.com
*/

/*                 JeeNode / JeeNode USB / JeeSMD
 -------|-----------------------|----|-----------------------|----
 |       |D3  A1 [Port2]  D5     |    |D3  A0 [port1]  D4     |    |
 |-------|IRQ AIO +3V GND DIO PWR|    |IRQ AIO +3V GND DIO PWR|    |
 | D1|TXD|                                           ---- ----     |
 | A5|SCL|                                       D12|MISO|+3v |    |
 | A4|SDA|   Atmel Atmega 328                    D13|SCK |MOSI|D11 |
 |   |PWR|   JeeNode / JeeNode USB / JeeSMD         |RST |GND |    |
 |   |GND|                                       D8 |BO  |B1  |D9  |
 | D0|RXD|                                           ---- ----     |
 |-------|PWR DIO GND +3V AIO IRQ|    |PWR DIO GND +3V AIO IRQ|    |
 |       |    D6 [Port3]  A2  D3 |    |    D7 [Port4]  A3  D3 |    |
  -------|-----------------------|----|-----------------------|----
*/


#include <Ports.h>
#include <RF12.h>

// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

boolean DEBUG = 1;

// set pin numbers:
const byte FrontDriveLight = 14;		// Front drive MOSFET on Port 1 AIO (PC0)
const byte MidDriveLight = 4;		// Mid drive MOSFET on Port 1 DIO (PD4)
const byte RearDriveLight = 15;		// Rear drive MOSFET on Port 2 AIO (PD1)

boolean FrontState = 0;
boolean MidState = 0;
boolean RearState = 0;

void setup() {

  if (DEBUG) {           // If we want to see the pin values for debugging...
    Serial.begin(57600);  // ...set up the serial ouput on 0004 style
    Serial.println("\n[DriveWayLights]");
  }

  // Initialize the RF12 module. Node ID 30, 868MHZ, Network group 5
  // rf12_initialize(30, RF12_868MHZ, 5);

  // This calls rf12_initialize() with settings obtained from EEPROM address 0x20 .. 0x3F.
  // These settings can be filled in by the RF12demo sketch in the RF12 library
  rf12_config();

  // Set up the easy transmission mechanism
  rf12_easyInit(0);

  // Set up the outputs
  pinMode(FrontDriveLight, OUTPUT);
  pinMode(MidDriveLight, OUTPUT);
  pinMode(RearDriveLight, OUTPUT);

}

void loop() {
  digitalWrite(FrontDriveLight, FrontState);
  digitalWrite(MidDriveLight, MidState);
  digitalWrite(RearDriveLight, RearState);

  FrontState = 1;
  MidState = 1;
  RearState = 1;

  delay(1000);
  digitalWrite(FrontDriveLight, FrontState);
  digitalWrite(MidDriveLight, MidState);
  digitalWrite(RearDriveLight, RearState);

  FrontState = 0;
  MidState = 0;
  RearState = 0;

  delay(1000);


}

