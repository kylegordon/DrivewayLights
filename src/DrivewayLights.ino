/***************************************************
  MQTT Controller for the driveway lights

 ****************************************************/

#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <string.h>

// Used by software_reboot
#include <avr/wdt.h>

#define SERIALDEBUG = 1;

// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   2  // MUST be an interrupt pin!
// These can be any two pins. CHECK THE README. DrivewayLights uses 2 & 4
#define ADAFRUIT_CC3000_VBAT  4
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
int RearButtonPin = 8; // Digital input pin 7

int FrontPWMOutPin = 3;
int MidPWMOutPin = 5;
int RearPWMOutPin = 6;
int AuxPWMOutPin = 9;

long previousMillis = 0;      // last update time
long elapsedMillis = 0;    // elapsed time
long storedMillis = 5;

boolean timestored = 0;
boolean FrontButtonState = 0;     // Front button state
boolean RearButtonState = 0;      // Rear button state
boolean dimming = false;          // Used to record if we're in the middle of dimming or not

// Initial target power states
float FrontTargetPower = 0;
float MidTargetPower = 0;
float RearTargetPower = 0;
float AuxTargetPower = 0;

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

void software_reboot()
{
  wdt_enable(WDTO_8S);
  while(1)
  {
  }
}

void callback (char* topic, byte* payload, unsigned int length) {
  // Handle an incoming message
  Serial.write(topic);
  Serial.print(" : ");
  Serial.write(payload, length);
  Serial.println("");
  String strTopic(topic);
  payload[length] = '\0';
  String strPayload = String((char*)payload);
  int intPayload = strPayload.toInt();

  // Instead of using endswith, compare the last character of the topic
  // Serial.print("Topic length is ");
  // Serial.println(strlen(topic));
  Serial.print("Last topic character is ");
  Serial.println(topic[strlen(topic)-1]);
  if (topic[strlen(topic)-1] == '1') {
      Serial.print("Setting light number 1 to level ");
      Serial.println(intPayload);
      FrontTargetPower = intPayload;
  }
  if (topic[strlen(topic)-1] == '2') {
      Serial.print("Setting light number 2 to level ");
      Serial.println(intPayload);
      MidTargetPower = intPayload;
  }
  if (topic[strlen(topic)-1] == '3') {
      Serial.print("Setting light number 3 to level ");
      Serial.println(intPayload);
      RearTargetPower = intPayload;
  }
  if (topic[strlen(topic)-1] == '4') {
      Serial.print("Setting light number 4 to level ");
      Serial.println(intPayload);
      AuxTargetPower = intPayload;
  }


}


ArrayToIp server = { 13, 32, 24, 172 };
//PubSubClient mqttclient(server.ip, 1883, callback, client, cc3000);
EthernetClient ethClient;
PubSubClient mqttclient(ethClient);

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

  pinMode(FrontPWMOutPin, OUTPUT);
  pinMode(MidPWMOutPin, OUTPUT);
  pinMode(RearPWMOutPin, OUTPUT);
  pinMode(AuxPWMOutPin, OUTPUT);

  //pinMode(FrontButtonPin, INPUT);
  //pinMode(RearButtonPin, INPUT);

  analogWrite(FrontPWMOutPin, 255);
  analogWrite(MidPWMOutPin, 255);
  analogWrite(RearPWMOutPin, 255);
  analogWrite(AuxPWMOutPin, 255);
  delay(500);
  analogWrite(FrontPWMOutPin, 0);
  analogWrite(MidPWMOutPin, 0);
  analogWrite(RearPWMOutPin, 0);
  analogWrite(AuxPWMOutPin, 0);


  // If you want to set the aref to something other than 5v
  analogReference(EXTERNAL);

  displayDriverMode();

  Serial.println(F("\nInitialising the CC3000 ..."));
  if (!cc3000.begin()) {
    Serial.println(F("Unable to initialise the CC3000! Check your wiring?"));
    for(;;);
  }

  // Light the first lamp. CC3000 is Initialised
  analogWrite(FrontPWMOutPin, 255);

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

  // Light the second lamp. Connected to wireless
  analogWrite(MidPWMOutPin, 255);


  /* Wait for DHCP to complete */
  Serial.println(F("Request DHCP"));
  while (!cc3000.checkDHCP()) {
    delay(100); // ToDo: Insert a DHCP timeout!
  }

  /* Display the IP address DNS, Gateway, etc. */
  while (!displayConnectionDetails()) {
    delay(1000);
  }

  analogWrite(RearPWMOutPin, 255);

  // connect to the broker
  if (!client.connected()) {
      Serial.println("About to connect to MQTT server");
      client = cc3000.connectTCP(server.ip, 1883);
      Serial.println("Connected to MQTT server");
  }

  // did that last thing work? sweet, let's do something
  if(client.connected()) {
      Serial.println("MQTT Connected apparently. publishing and subscribing");
      //mqttclient.publish("/clients/DrivewayLights/arduino","2");
      //mqttclient.subscribe("/clients/DrivewayLights/arduino/level/#");

      Serial.println("Connected");
      // Flash to indicate that we're connected

      analogWrite(FrontPWMOutPin, 0);
      analogWrite(MidPWMOutPin, 0);
      analogWrite(RearPWMOutPin, 0);
      analogWrite(AuxPWMOutPin, 0);
      delay(500);
      analogWrite(FrontPWMOutPin, 255);
      analogWrite(MidPWMOutPin, 255);
      analogWrite(RearPWMOutPin, 255);
      analogWrite(AuxPWMOutPin, 255);
      delay(500);
      analogWrite(FrontPWMOutPin, 0);
      analogWrite(MidPWMOutPin, 0);
      analogWrite(RearPWMOutPin, 0);
      analogWrite(AuxPWMOutPin, 0);

  } else {
    Serial.println("No longer connected");
  }
  Serial.println("Setup complete");
}

void checkbuttons() {
     // Check the state of the front button and respond if pressed

     FrontButtonState = digitalRead(FrontButtonPin);
     RearButtonState = digitalRead(RearButtonPin);
     #ifdef SERIALDEBUG
        Serial.print("Front button : "); Serial.print(0 + FrontButtonState);
        Serial.print(", Rear button : "); Serial.println(0 + RearButtonState);
     #endif
}

void checkhumidity() {
     // Check and return the humidity levels
     int humidity = 42;

#ifdef SERIALDEBUG
       Serial.print("Humidity : "); Serial.println(0 + humidity);
#endif
}

void loop(void) {

  Serial.print("Looping "); Serial.println(millis());

  if (client.connected()) {
    Serial.println("About to MQTT loop");
    mqttclient.loop();
  } else {
    Serial.println("Disconnected!");
    Serial.println("Rebooting in 30 seconds");
    delay(30000);
    software_reboot();
  }

  //if (SERIALDEBUG) { Serial.println("----------------"); }
  // unsigned long currentMillis = millis();        // Grab the current time

  // Check the button/PIR states
  //checkbuttons();

  // Decide what to do based on what buttons are pressed
  /*
  if(client.connected()) {
    if (FrontButtonState == 1) {
        mqttclient.publish("/events/DrivewayLights/FrontButton","1");
    }
    if (RearButtonState == 1) {
        mqttclient.publish("/events/DrivewayLights/RearButton","1");
    }
    if (FrontButtonState == 0) {
        mqttclient.publish("/events/DrivewayLights/FrontButton","0");
    }
    if (RearButtonState == 0) {
        mqttclient.publish("/events/DrivewayLights/RearButton","0");
    }
  }
  */

  Serial.println("Buttons checked");

  // If all the channels are at the correct values
  if (FrontPower == FrontTargetPower && MidPower == MidTargetPower && RearPower == RearTargetPower && AuxPower == AuxTargetPower) {
    // The lights are at their target values
    dimming = 0;
  }

  // If we're not at our defined levels, do somethign about it.
  if (FrontPower != FrontTargetPower || MidPower != MidTargetPower || RearPower != RearTargetPower || AuxPower != AuxTargetPower) {
    // We're dimming, as the targets haven't been met
    dimming = 1;

#ifdef SERIALDEBUG
      Serial.print("Pre  : ");
      Serial.print(FrontTargetPower);
      Serial.print(":");
      Serial.print(MidTargetPower);
      Serial.print(":");
      Serial.print(RearTargetPower);
      Serial.print(":");
      Serial.println(AuxTargetPower);
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
    if (AuxPower > AuxTargetPower) {
            AuxPower = AuxPower / IncrementRate;
            if (AuxPower < AuxTargetPower || AuxPower < 1) { AuxPower = AuxTargetPower;}
    }
    if (AuxPower < AuxTargetPower) {
            if (AuxPower == 0) { AuxPower = 1;} // Stop any intermediate dim cycles trying to multiply by 0
            AuxPower = AuxPower * IncrementRate;
            if (AuxPower > AuxTargetPower) { AuxPower = AuxTargetPower;}
    }


#ifdef SERIALDEBUG
      Serial.print("Post : ");
      Serial.print(FrontPower);
      Serial.print(":");
      Serial.print(MidPower);
      Serial.print(":");
      Serial.print(RearPower);
      Serial.print(":");
      Serial.println(AuxPower);
#endif

  analogWrite(FrontPWMOutPin, FrontPower);
  analogWrite(MidPWMOutPin, MidPower);
  analogWrite(RearPWMOutPin, RearPower);
  analogWrite(AuxPWMOutPin, AuxPower);

  }


}
