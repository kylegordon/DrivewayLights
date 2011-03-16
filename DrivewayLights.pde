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
const byte FrontDriveLight = 5;	// Front drive MOSFET on Port 1 AIO
const byte MidDriveLight = 6;		// Mid drive MOSFET on Port 2 AIO
const byte RearDriveLight = 3;		// Rear drive MOSFET on Port 3 AIO

long previousMillis = 0;        // last update time
long elapsedMillis = 0;         // elapsed time
long storedMillis = 0;

boolean timestored = 0;
boolean buttonState = 0;      // variable for reading the pushbutton status
boolean dimming = false;
boolean ramping = false;

int buttonPin = 5;   // choose the input pin (for a pushbutton)

boolean FrontState = 0;
boolean MidState = 0;
boolean RearState = 0;

// Target power states
int FrontTargetPower = 0;
int MidTargetPower = 0;
int RearTargetPower = 0;

int FrontPower = 0;
int MidPower = 0;
int RearPower = 0;

int ontimeout = 1000;
int offtimeout = 1000;

PortI2C myBus (1);
DimmerPlug dimmer (myBus, 0x40);

static void setall(byte reg, byte a1, byte a2, byte a3) {
				dimmer.send();
				dimmer.write(0xE0 | reg);
				dimmer.write(a1);
				dimmer.write(a2);
				dimmer.write(a3);
				dimmer.stop();
}

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

				pinMode(buttonPin, INPUT);    // declare pushbutton as input


				FrontTargetPower = 255;
				MidTargetPower = 255;
				RearTargetPower = 255;

				dimmer.setReg(DimmerPlug::MODE1, 0x00);     // normal
				dimmer.setReg(DimmerPlug::MODE2, 0x14);     // inverted, totem-pole
				dimmer.setReg(DimmerPlug::GRPFREQ, 5);      // blink 4x per sec
				dimmer.setReg(DimmerPlug::GRPPWM, 0xFF);    // max brightness
				setall(DimmerPlug::LEDOUT0, ~0, ~0, ~0);  // all LEDs group-dimmable

}

void loop() {
				unsigned long currentMillis = millis(); 		// Grab the current time
				buttonState = digitalRead(buttonPin);

				// 'fullpower' is when we're at full power
				// 'dimming' is when we're ramping up or down

				if (dimming == false) {
								// If all the target values have been reached, start a timer if they're not 0 (in order to turn off eventually), or just sit and do nothing
								if (FrontPower == FrontTargetPower && MidPower == MidTargetPower && RearPower == RearTargetPower) {
												// The lights are on, we should be counting
												if (timestored == 0) {
																timestored = 1;
																storedMillis = currentMillis;
																if (DEBUG) { Serial.print("Storing time : "); Serial.println(currentMillis); }
												}
												if (timestored == 1) {
																elapsedMillis = currentMillis - storedMillis;
																// When we reach the timeout, start turning off
																if (elapsedMillis > ontimeout) { 
																	dimming = 1;
													        FrontTargetPower = 0;
													        MidTargetPower = 0;
													        RearTargetPower = 0;
																}
																if (DEBUG) { Serial.print("Static elapsed time : "); Serial.println(elapsedMillis); }
												}

								} else {
												// Everything is off
												timestored = 0;
												storedMillis = 0;
								}
								}

								if (dimming == true) {
												// We're dimming, so we have to complete the ramping process
												if (FrontPower != FrontTargetPower && MidPower != MidTargetPower && RearPower != RearTargetPower) {
																dimming = 1;
																if (FrontPower < FrontTargetPower) { ++FrontPower;  }
																if (FrontPower > FrontTargetPower) { --FrontPower;  }
																if (MidPower < MidTargetPower) { ++MidPower;  }
																if (MidPower > MidTargetPower) { --MidPower;  }
																if (RearPower < RearTargetPower) { ++RearPower;  }
																if (RearPower > RearTargetPower) { --RearPower;  }

																int val = 100;
																rf12_easyPoll();
																rf12_easySend(&val, sizeof val);
																rf12_easyPoll();

												} else {
													// We appear to have reached our targets. we no longer need to ramp
													dimming = false;
													if (DEBUG) {Serial.println("Power off"); }
												}

								}

								if (buttonState == 1) {
												// What do we do when the button is pressed?
												// It doesn't matter what we're doing. The target values should be set back to 255
												dimming = 1;
												FrontTargetPower = 255;
												MidTargetPower = 255;
												RearTargetPower = 255;
								}

								if (DEBUG) { Serial.print("Dimming : "); Serial.println(0 + dimming); }
								Serial.print(FrontPower);
								Serial.print(":");
								Serial.print(MidPower);
								Serial.print(":");
								Serial.println(RearPower);

								dimmer.setReg(DimmerPlug::PWM0, FrontPower);
								dimmer.setReg(DimmerPlug::PWM1, MidPower);
								dimmer.setReg(DimmerPlug::PWM2, RearPower);

								// treat each group of 4 LEDS as RGBW combinations
								//set4(DimmerPlug::PWM0, w, b, g, r);
								//set4(DimmerPlug::PWM1, w, b, g, r);
								//set4(DimmerPlug::PWM2, w, b, g, r);
								//set4(DimmerPlug::PWM12, w, b, g, r);

								delay(50);

				}

