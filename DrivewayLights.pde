// vi:tabstop=2:expandtab
/*
   Control outdoor lights with PWM fading.

   created 2011
   by Kyle Gordon <kyle@lodge.glasgownet.com>

http://lodge.glasgownet.com
 */

/*		     JeeNode / JeeNode USB / JeeSMD
           -------|-----------------------|----|-----------------------|----
           |	 |D3  A1 [Port2]  D5     |    |D3  A0 [port1]  D4     |    |
           |-------|IRQ AIO +3V GND DIO PWR|    |IRQ AIO +3V GND DIO PWR|    |
           | D1|TXD|							 ---- ----     |
           | A5|SCL|						   D12|MISO|+3v |    |
           | A4|SDA|   Atmel Atmega 328			  D13|SCK |MOSI|D11 |
           |   |PWR|   JeeNode / JeeNode USB / JeeSMD	   |RST |GND |    |
           |   |GND|						   D8 |BO  |B1  |D9  |
           | D0|RXD|							 ---- ----     |
           |-------|PWR DIO GND +3V AIO IRQ|    |PWR DIO GND +3V AIO IRQ|    |
           |	 |    D6 [Port3]  A2  D3 |    |    D7 [Port4]  A3  D3 |    |
           -------|-----------------------|----|-----------------------|----
 */


#include <Ports.h>
#include <RF12.h>

// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

boolean DEBUG = 0;
int loopdelay = 0;

// set pin numbers:
int buttonPin = 6;   // choose the input pin (for a pushbutton)

long previousMillis = 0;	  // last update time
long elapsedMillis = 0;	   // elapsed time
long storedMillis = 0;

boolean timestored = 0;
boolean buttonState = 0;	// variable for reading the pushbutton status
boolean dimming = false;

// Initial target power states
int FrontTargetPower = 0;
int MidTargetPower = 0;
int RearTargetPower = 0;

float FrontPower = 0;
float MidPower = 0;
float RearPower = 0;

float FrontIncrement = 1.02;
float MidIncrement = 1.015;
float RearIncrement = 1.01;
float FrontDecrement = 1.015;
float MidDecrement = 1.02;
float RearDecrement = 1.03;

int offtimeout = 10000;    // Number of milliseconds to stay on for after being turned on

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

void easteregg() {

        int count;

        for (count=0; count<=3; count++){
                dimmer.setReg(DimmerPlug::PWM0, 255);
                delay(200);
                dimmer.setReg(DimmerPlug::PWM1, 255);
                delay(200);
                dimmer.setReg(DimmerPlug::PWM0, 0);
                dimmer.setReg(DimmerPlug::PWM2, 255);
                delay(200);                     
                dimmer.setReg(DimmerPlug::PWM1, 0);
                delay(200);                                                     
                dimmer.setReg(DimmerPlug::PWM2, 0);
                delay(200);

                dimmer.setReg(DimmerPlug::PWM2, 255);
                delay(200);
                dimmer.setReg(DimmerPlug::PWM1, 255);
                delay(200);
                dimmer.setReg(DimmerPlug::PWM2, 0);
                dimmer.setReg(DimmerPlug::PWM0, 255);
                delay(200);
                dimmer.setReg(DimmerPlug::PWM1, 0);
                delay(200);
                dimmer.setReg(DimmerPlug::PWM0, 0);
                delay(200);
        }
}

void setup() {

        if (DEBUG) {	     // If we want to see the pin values for debugging...
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

        //FrontTargetPower = 255;
        //MidTargetPower = 255;
        //RearTargetPower = 255;

        dimmer.setReg(DimmerPlug::MODE1, 0x00);     // normal
        dimmer.setReg(DimmerPlug::MODE2, 0x14);     // inverted, totem-pole
        dimmer.setReg(DimmerPlug::GRPFREQ, 5);	// blink 4x per sec
        dimmer.setReg(DimmerPlug::GRPPWM, 0xFF);    // max brightness
        setall(DimmerPlug::LEDOUT0, ~0, ~0, ~0);    // all LEDs group-dimmable

        easteregg();

}

void loop() {
        unsigned long currentMillis = millis(); 		// Grab the current time
        buttonState = digitalRead(buttonPin);
        // buttonState = 1;
        if (DEBUG) { Serial.print("Button : "); Serial.println(0 + buttonState);}
        if (buttonState == 1) {
                // What do we do when the button is pressed?
                // It doesn't matter what we're doing. The target values should be set back to 255
                dimming = 1;
                storedMillis = 0;
                elapsedMillis = 0;
                // Set the target values
                FrontTargetPower = 255;
                MidTargetPower = 255;
                RearTargetPower = 255;
                // Set the initial values (cos we can't multiply 0 by anything...)
                if (!FrontPower && !MidPower && !RearPower) {
                        FrontPower = 1;
                        MidPower = 1;
                        RearPower = 1;
                }
        }


        // 'dimming' is when we're ramping up or down

        // If all the target values have been reached, start a timer if they're not 0 
        // (in order to turn off eventually), or just sit and do nothing
        if (FrontPower == FrontTargetPower && MidPower == MidTargetPower && RearPower == RearTargetPower) {
                // All lights are at their target values
                // Start the count down
                if (timestored == 0) {
                        timestored = 1;
                        storedMillis = currentMillis;
                        if (DEBUG) { Serial.print("Storing time : "); Serial.println(currentMillis); }
                }
                if (timestored == 1) {
                        elapsedMillis = currentMillis - storedMillis;
                        // When we reach the timeout, start turning off
                        if (elapsedMillis > offtimeout) { 
                                dimming = 1;
                                FrontTargetPower = 0;
                                MidTargetPower = 0;
                                RearTargetPower = 0;
                        }
                        if (DEBUG) { Serial.print("Static elapsed time : "); Serial.println(elapsedMillis); }
                }

        } else {
                // Everything is fine
                timestored = 0;
                storedMillis = 0;
                elapsedMillis = 0;
        }

        // If we're not at our defined levels, do somethign about it.
        if (FrontPower != FrontTargetPower || MidPower != MidTargetPower || RearPower != RearTargetPower) {
                if (timestored == 0) {
                        timestored = 1;
                        storedMillis = currentMillis;
                        if (DEBUG) { Serial.print("Storing time : "); Serial.println(currentMillis); }
                }
                if (timestored == 1) {
                        elapsedMillis = currentMillis - storedMillis;
                        if (DEBUG) { Serial.print("Dimming elapsed time : "); Serial.println(elapsedMillis); }
                }

                dimming = 1;
                if (DEBUG) {
                        Serial.print("Targets are : ");
                        Serial.print(FrontTargetPower);
                        Serial.print(":");
                        Serial.print(MidTargetPower);
                        Serial.print(":");
                        Serial.println(RearTargetPower);
                }

                // FIXME - this will decrement and then increment. Needs a way to skip it
                if (FrontPower > FrontTargetPower) { 
                        FrontPower = FrontPower / FrontDecrement;                                         // Decrement value
                        if (FrontPower < FrontTargetPower || FrontPower < 1) { FrontPower = FrontTargetPower;}   // Fix overshoot
                }
                if (FrontPower < FrontTargetPower) { 
                        FrontPower = FrontPower * FrontIncrement;                                         // Increment value
                        if (FrontPower > FrontTargetPower) { FrontPower = FrontTargetPower;}    // Fix overshoot
                }
                if (MidPower > MidTargetPower) { 
                        MidPower = MidPower / MidDecrement;
                        if (MidPower < MidTargetPower || MidPower < 1) { MidPower = MidTargetPower;}
                }
                if (MidPower < MidTargetPower) { 
                        MidPower = MidPower * MidIncrement;
                        if (MidPower > MidTargetPower) { MidPower = MidTargetPower;}
                }
                if (RearPower > RearTargetPower) { 
                        RearPower = RearPower / RearDecrement;
                        if (RearPower < RearTargetPower || RearPower < 1) { RearPower = RearTargetPower;}
                }
                if (RearPower < RearTargetPower) { 
                        RearPower = RearPower * RearIncrement;
                        if (RearPower > RearTargetPower) { RearPower = RearTargetPower;}
                }

                if (DEBUG) {
                        Serial.print("Post-adjustment values are : ");
                        Serial.print(FrontPower);
                        Serial.print(":");                              
                        Serial.print(MidPower);
                        Serial.print(":");                                                                              
                        Serial.println(RearPower);
                }

                int val = 100;
                rf12_easyPoll();
                rf12_easySend(&val, sizeof val);
                rf12_easyPoll();

        } else {
                // We appear to have reached our targets. we no longer need to ramp
                dimming = false;
                if (DEBUG) {Serial.println("Constant state"); }
        }


        if (DEBUG) { Serial.print("Dimming : "); Serial.println(0 + dimming); }
        /* Serial.print(FrontPower);
           Serial.print(":");
           Serial.print(MidPower);
           Serial.print(":");
           Serial.println(RearPower);
         */ 

        dimmer.setReg(DimmerPlug::PWM0, (int)FrontPower);
        dimmer.setReg(DimmerPlug::PWM1, (int)MidPower);
        dimmer.setReg(DimmerPlug::PWM2, (int)RearPower);

        // treat each group of 4 LEDS as RGBW combinations
        //set4(DimmerPlug::PWM0, w, b, g, r);
        //set4(DimmerPlug::PWM1, w, b, g, r);
        //set4(DimmerPlug::PWM2, w, b, g, r);
        //set4(DimmerPlug::PWM12, w, b, g, r);

        delay(loopdelay);

}

