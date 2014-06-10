/***************************************************
  MQTT Controller for the driveway lights
  
 ****************************************************/

#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <cc3000_PubSubClient.h>
#include <string.h>

#define SERIALDEBUG = 1;

// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   2  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11

Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT, SPI_CLOCK_DIVIDER);

#define WLAN_SSID       "XXXXXXXXXX"        // cannot be longer than 32 characters!
#define WLAN_PASS       "XXXXXXXXXX"

// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

Adafruit_CC3000_Client client;

// set pin numbers:
int FrontButtonPin = 7;   // Digital input pin 6
int RearButtonPin = 4; // Digital input pin 7

int FrontPWMOutPin = 9;
int MidPWMOutPin = 10;
int RearPWMOutPin = 11;
int AuxPWMOutPin = 3;

long previousMillis = 0;      // last update time
long elapsedMillis = 0;    // elapsed time
long storedMillis = 5;

boolean timestored = 0;
boolean FrontButtonState = 0;     // Front button state
boolean RearButtonState = 0;      // Rear button state
boolean dimming = false;          // Used to record if we're in the middle of dimming or not

// Initial target power states
int FrontTargetPower = 0;
int MidTargetPower = 0;
int RearTargetPower = 0;
int AuxTargetPower = 0;

// Initial power levels
float FrontPower = 0;
float MidPower = 0;
float RearPower = 0;
float AuxPower = 0;

// The rate at which the lighting level is changed
float IncrementRate = 1.02;

// We're going to set our broker IP and union it to something we can use
union ArrayToIp {
  byte array[4];
  uint32_t ip;
};

void callback (char* topic, byte* payload, unsigned int length) {
  // Handle an incoming message
  Serial.write(topic);
  Serial.print(" : ");
  Serial.write(payload, length);
  Serial.println("");
  String strTopic(topic);
  payload[length] = '\0';
  //String strPayload = String((char*)payload);
  int intPayload = int((char*)payload);
  
  // Instead of using endswith, compare the last character of the topic
  Serial.print("Topic length is ");
  Serial.println(strlen(topic));
  Serial.print("Last character is ");
  Serial.println(topic[strlen(topic)-1]);
  if (topic[strlen(topic)-1] == '1') {
      Serial.println("Setting light number 1 to level ");
      Serial.println(intPayload);
      //FrontTargetPower = intPayload;
  }
  if (topic[strlen(topic)-1] == '2') {
      Serial.println("Setting light number 2 to level ");
      Serial.println(intPayload);
      //MidTargetPower = intPayload;
  }
  if (topic[strlen(topic)-1] == '3') {
      Serial.println("Setting light number 3 to level ");
      Serial.println(intPayload);
      //RearTargetPower = intPayload;
  }
  if (topic[strlen(topic)-1] == '4') {
      Serial.println("Setting light number 4 to level ");
      Serial.println(intPayload);
      //AuxTargetPower = intPayload;
  }


}

ArrayToIp server = { 13, 32, 24, 172 };
cc3000_PubSubClient mqttclient(server.ip, 1883, callback, client, cc3000);

/**************************************************************************/
/*!
    @brief  Displays the driver mode (tiny of normal), and the buffer
            size if tiny mode is not being used

    @note   The buffer size and driver mode are defined in cc3000_common.h
*/
/**************************************************************************/
void displayDriverMode(void)
{
  #ifdef CC3000_TINY_DRIVER
    Serial.println(F("CC3000 is configure in 'Tiny' mode"));
  #else
    Serial.print(F("RX Buffer : "));
    Serial.print(CC3000_RX_BUFFER_SIZE);
    Serial.println(F(" bytes"));
    Serial.print(F("TX Buffer : "));
    Serial.print(CC3000_TX_BUFFER_SIZE);
    Serial.println(F(" bytes"));
  #endif
}

/**************************************************************************/
/*!
    @brief  Tries to read the CC3000's internal firmware patch ID
*/
/**************************************************************************/
uint16_t checkFirmwareVersion(void)
{
  uint8_t major, minor;
  uint16_t version;
  
#ifndef CC3000_TINY_DRIVER  
  if(!cc3000.getFirmwareVersion(&major, &minor))
  {
    Serial.println(F("Unable to retrieve the firmware version!\r\n"));
    version = 0;
  }
  else
  {
    Serial.print(F("Firmware V. : "));
    Serial.print(major); Serial.print(F(".")); Serial.println(minor);
    version = major; version <<= 8; version |= minor;
  }
#endif
  return version;
}

/**************************************************************************/
/*!
    @brief  Tries to read the 6-byte MAC address of the CC3000 module
*/
/**************************************************************************/
void displayMACAddress(void)
{
  uint8_t macAddress[6];
  
  if(!cc3000.getMacAddress(macAddress))
  {
    Serial.println(F("Unable to retrieve MAC Address!\r\n"));
  }
  else
  {
    Serial.print(F("MAC Address : "));
    cc3000.printHex((byte*)&macAddress, 6);
  }
}


/**************************************************************************/
/*!
    @brief  Tries to read the IP address and other connection details
*/
/**************************************************************************/
bool displayConnectionDetails(void)
{
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;
  
  if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
    Serial.println(F("Unable to retrieve the IP Address!\r\n"));
    return false;
  }
  else
  {
    Serial.print(F("\nIP Addr: ")); cc3000.printIPdotsRev(ipAddress);
    Serial.print(F("\nNetmask: ")); cc3000.printIPdotsRev(netmask);
    Serial.print(F("\nGateway: ")); cc3000.printIPdotsRev(gateway);
    Serial.print(F("\nDHCPsrv: ")); cc3000.printIPdotsRev(dhcpserv);
    Serial.print(F("\nDNSserv: ")); cc3000.printIPdotsRev(dnsserv);
    Serial.println();
    return true;
  }
}


void setup(void)
{

  Serial.begin(115200);
  Serial.println(F("Hello, CC3000!\n"));

  // If you want to set the aref to something other than 5v
  analogReference(EXTERNAL);

  displayDriverMode();
  
  Serial.println(F("\nInitialising the CC3000 ..."));
  if (!cc3000.begin()) {
    Serial.println(F("Unable to initialise the CC3000! Check your wiring?"));
    for(;;);
  }

  uint16_t firmware = checkFirmwareVersion();
  if ((firmware != 0x113) && (firmware != 0x118)) {
    Serial.println(F("Wrong firmware version!"));
    for(;;);
  }
  
  displayMACAddress();
  
  Serial.println(F("\nDeleting old connection profiles"));
  if (!cc3000.deleteProfiles()) {
    Serial.println(F("Failed!"));
    while(1);
  }

  /* Attempt to connect to an access point */
  char *ssid = WLAN_SSID;             /* Max 32 chars */
  Serial.print(F("\nAttempting to connect to ")); Serial.println(ssid);
  
  /* NOTE: Secure connections are not available in 'Tiny' mode! */
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Failed!"));
    while(1);
  }
   
  Serial.println(F("Connected!"));
  
  /* Wait for DHCP to complete */
  Serial.println(F("Request DHCP"));
  while (!cc3000.checkDHCP()) {
    delay(100); // ToDo: Insert a DHCP timeout!
  }

  /* Display the IP address DNS, Gateway, etc. */  
  while (!displayConnectionDetails()) {
    delay(1000);
  }
   
   // connect to the broker
   if (!client.connected()) {
     client = cc3000.connectTCP(server.ip, 1883);
   }
   
   // did that last thing work? sweet, let's do something
   if(client.connected()) {
    if (mqttclient.connect("DrivewayLights")) {
      mqttclient.publish("/clients/DrivewayLights/arduino","1");
      mqttclient.subscribe("/clients/DrivewayLights/arduino/level/#");
    }
   } 
}

void checkbuttons() {
     // Check the state of the front button and respond if pressed

     FrontButtonState = digitalRead(FrontButtonPin);
     //if (SERIALDEBUG) { Serial.print("Front button : "); Serial.print(0 + FrontButtonState);}
     RearButtonState = digitalRead(RearButtonPin);
     //if (SERIALDEBUG) { Serial.print(", Rear button : "); Serial.println(0 + RearButtonState);}
}

void checktemperature() {
     // Check and return the temperature from the one-wire sensor
     int temperature = 42;

#ifdef SERIALDEBUG
       Serial.print("Temperature : "); Serial.println(0 + temperature);
#endif
}


void loop(void) {
 
  mqttclient.loop();

  //if (SERIALDEBUG) { Serial.println("----------------"); }
  unsigned long currentMillis = millis();        // Grab the current time

  // Check the button/PIR states
  checkbuttons();

  // Decide what to do based on what buttons are pressed
  if (FrontButtonState == 1) {
    // Send MQTT message to broker
  }

  // If all the channels are at the correct values
  if (FrontPower == FrontTargetPower && MidPower == MidTargetPower && RearPower == RearTargetPower) {
    // The lights are at their target values
    dimming = 0;
  }

  // If we're not at our defined levels, do somethign about it.
  if (FrontPower != FrontTargetPower || MidPower != MidTargetPower || RearPower != RearTargetPower) {
    // We're dimming, as the targets haven't been met
    dimming = 1;
 
#ifdef SERIALDEBUG
      Serial.print("Pre: ");
      Serial.print(FrontTargetPower);
      Serial.print(":");
      Serial.print(MidTargetPower);
      Serial.print(":");
      Serial.println(RearTargetPower);
#endif

    // 23:43 < gordonjcp> something like v=(old_v*rate) + (in*(1-rate)); old_v = v
    // FrontPower = (old_FrontPower * FrontIncrement) + (in*(1-FrontIncrement)); old_FrontPower = FrontPower;

    // If the dim sequence is reset whilst one or two is off, they'll try to climb again but be multiplying against 0
    // FIXME - this will decrement and then increment. Needs a way to skip it
    if (FrontPower > FrontTargetPower) { 
      FrontPower = FrontPower / IncrementRate;                                         // Decrement value
      if (FrontPower < FrontTargetPower || FrontPower < 1) { FrontPower = FrontTargetPower;}   // Fix overshoot
    }
    if (FrontPower < FrontTargetPower) { 
      if (FrontPower == 0) { FrontPower = 1;} //Stop any intermediate dim cycles trying to multiply by 0
      FrontPower = FrontPower * IncrementRate;                                         // Increment value
      if (FrontPower > FrontTargetPower) { FrontPower = FrontTargetPower;}    // Fix overshoot
    }
    if (MidPower > MidTargetPower) { 
            MidPower = MidPower / IncrementRate;
            if (MidPower < MidTargetPower || MidPower < 1) { MidPower = MidTargetPower;}
    }
    if (MidPower < MidTargetPower) { 
            if (MidPower == 0) { MidPower = 1;} // Stop any intermediate dim cycles trying to multiply by 0
            MidPower = MidPower * IncrementRate;
            if (MidPower > MidTargetPower) { MidPower = MidTargetPower;}
    }
    if (RearPower > RearTargetPower) { 
            RearPower = RearPower / IncrementRate;
            if (RearPower < RearTargetPower || RearPower < 1) { RearPower = RearTargetPower;}
    }
    if (RearPower < RearTargetPower) { 
            if (RearPower == 0) { RearPower = 1;} // Stop any intermediate dim cycles trying to multiply by 0
            RearPower = RearPower * IncrementRate;
            if (RearPower > RearTargetPower) { RearPower = RearTargetPower;}
    }

#ifdef SERIALDEBUG
      Serial.print("Post : ");
      Serial.print(FrontPower);
      Serial.print(":");                              
      Serial.print(MidPower);
      Serial.print(":");                                                                              
      Serial.println(RearPower);
#endif
  
  analogWrite(FrontPWMOutPin, FrontPower);
  analogWrite(MidPWMOutPin, MidPower);
  analogWrite(RearPWMOutPin, RearPower);
  analogWrite(AuxPWMOutPin, AuxPower);
  
  }


}



