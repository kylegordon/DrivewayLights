*/// FROM http://www.arduino.cc/playground/Main/PWMallPins
//STATE 1: Birth.
//        Candles flicker on and off in order – FAST
//        1 second per LED
//        For testing, only use PWM (fake brightness for LEDs) on pins 9,10,11
// STATE 2 : DEATH
//           SAME AS BIRTH , BUT MUCH SLOWER.
// STATE 3 : EUTHANASIA
//           LIKE BIRTH BUT CANDLES DON T GET ON IN ORDER
// STATE 4 : CONCEPTION
//           ALL CANDLES ON  + FLICKERING LIKE BIRTHDAY CAKE

int pwmVal[14];                                                               // PWM values for 12 channels – 0 & 1 included but not used
//float pwmFloats[14];
int i, j, k, l, x, y, z, bufsize, pot;                                        // variables for various counters
long time, oldTime;    //All my time variables.
int timeOut=150;         //another timer variable to help with the flicker
int oldSubTime = 0;      //another timer variable to help with the flicker
int whichCandleIsActive, whichCandleWasActive;
int state = 1;      //birth , death , conception, etc
int totalCandles = 3;    //how many candles we have plugged into the system

byte inByte;      // Our Serial INPUT (from Processing)

int timer;
int timerMax = 6;

void setup(){
Serial.begin(57600);      //Create a serial connection
DDRD=0xFC;      // direction variable for port D – make em all outputs except serial pins 0 & 1 -> SAME AS pinMode but talks to more pins at on
DDRB=0xFF;      // direction variable for port B – all outputs
randomSeed(analogRead(0));    //initialize the random function with the analog reading from analog pin 0
time = oldTime = millis();    //set the time !
whichCandleIsActive = 0;    //Starting Candle: references the PIN that the LED is plugged into
}

void loop(){
checkSerial();      //Check to see if Processing OR the Arduino Serial Monitor is sending any data
if (state != 1) handlePWMALLPINS();      //DO THE PWM ALL PINS!!!
timer++;
if (timer >= timerMax) {
timer = 0;
time = millis();    //Set the current Time.
if (state == 1) {      //do whatever LED animations based on what STATE they are set to.
off();
}

else if (state == 2) {
death();
}
else if (state == 3) {
birth();
}
else if (state == 4) {
euthanasia();
}
else if (state == 5) {
conception();
}
}
}

//The SERIAL CHECKING FUNCTION
void checkSerial()
{
if (Serial.available() > 0) {      //if there is something waiting from Processing, then READ it!
inByte = Serial.read();
}
if (inByte == 'A') {      //if its an 'A' then the new state = 1 which is OFF
state = 1;
}
else if (inByte == 'B') {    //if its an 'B' then the new state = 1 which is DEATH
state = 2;
}
else if (inByte == 'C') {      //if its an 'C' then the new state = 1 which is BIRTH
state = 3;
}
else if (inByte == 'D') {      //if its an 'D' then the new state = 1 which is EUTH
state = 4;
}
else if (inByte == 'E') {      //if its an 'D' then the new state = 1 which is CONCEPTION
state = 5;
}
}
