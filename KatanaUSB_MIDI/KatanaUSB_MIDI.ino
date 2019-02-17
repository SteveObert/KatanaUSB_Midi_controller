//#include <Wire.h>

/**
   This is an example for the MS-3 library.

   What have you done? Let's find out what's happening with all effect blocks.
*/

// Uncomment this to enable verbose debug messages.
//#define MS3_DEBUG_MODE
//uncomment below to debug withot the KAtana connected.
//#define MS3_OVERRIDE_MODE

#include "Arduino.h"
#include "MS3.h"
//#include <Wire.h>
#include <i2c_t3.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#include <JC_Button.h>
#include <MIDI.h>

// Initialize the MS3 class.
MS3 MS3;

// These are addresses for the effect states.
const unsigned long P_BSTR = 0x60000030;
const unsigned long P_MOD =  0x60000140;
const unsigned long P_FX =   0x6000034C;
const unsigned long P_SR =   0x60000655;
const unsigned long P_DD1 =  0x60000560;
const unsigned long P_DD2 =  0x6000104E;
const unsigned long P_REV =  0x60000610;
const unsigned long P_AMP =  0x60000051;
const unsigned long P_BSTR_RGY = 0x60001210; //0x00000410;
const unsigned long P_MOD_RGY =  0x60001211;
const unsigned long P_DD1_RGY =  0x60001212;
const unsigned long P_FX_RGY =   0x60001213; //0x00000411;
const unsigned long P_REVDD2_RGY = 0x60001214; //0x00000412;

// sysex control define ################################
const unsigned long PC = 0x00010000; // change channel MS3.write(PC, 1, 2) second byte is channel number 3rd is length
const unsigned long fx1_sw = 0x60000030; //turn button 1 on  MS3.write(fx1_sw, 1, 1) second byte 0 = off 1 = on
const unsigned long fx2_sw = 0x6000034C; //turn button 2 on  MS3.write(fx2_sw, 1, 1) second byte 0 = off 1 = on
const unsigned long fx3_sw = 0x60000610; //turn reverb on  MS3.write(fx3_sw, 1, 1) second byte 0 = off 1 = on
const unsigned long Loop_sw = 0x60000655;   // turn loop off on
const unsigned long rvbYellow = 0x60001214; // set reverb type to yellow
const unsigned long wah_select = 0x6000015C; // Wah Type ??
const unsigned long wah_pedal = 0x60000369; // expression pedal Wah position
const unsigned long VOLUME_PEDAL_ADDR = 0x60000633; // volume pedal address
// end sysex control define ##############################

// define custom lcd character set (gliphs) for inverted R G Y.
byte custom_O[8] = { 0b11111, 0b11111, 0b11111, 0b11111, 0b00000, 0b11111, 0b11111, 0b11111 };
byte custom_Y[8] = { 0b11111, 0b10011, 0b10101, 0b10101, 0b10011, 0b10101, 0b10101, 0b11111 };
byte custom_R[8] = { 0b11111, 0b10101, 0b10101, 0b10101, 0b11011, 0b11011, 0b11011, 0b11111 };
byte custom_G[8] = { 0b11111, 0b10011, 0b01101, 0b01111, 0b01001, 0b01101, 0b10011, 0b11111 };

// This is the address for the program change.
const unsigned long P_PATCH = 0x00010000;

// We're going to check and report these effect blocks.
const unsigned long CHECK_THIS[] = {
  P_BSTR,
  P_MOD,
  P_SR,
  P_DD1,
  P_DD2,
  P_FX,
  P_AMP,
  P_REV,
  P_BSTR_RGY,
  P_MOD_RGY,
  P_DD1_RGY,
  P_FX_RGY,
  P_REVDD2_RGY
};
const byte CHECK_THIS_SIZE = sizeof(CHECK_THIS) / sizeof(CHECK_THIS[0]);

// Some global variables to store effect state and if something changed.
unsigned int states = 0;  // stores true/false bit of the first 8 effects parsed.
unsigned int checked = 0;
bool changed = false;
unsigned long timerStart = 0;
unsigned int byte_array[16]; //store parameter byte from each (13) effects when parsed.

Button footSw5(2, 50);       // define the button to pin 2, 50ms debounce. Function switch
Button footSw1(3, 50);       // define the button to pin 3, 50ms debounce. FX1 switch
Button footSw2(4, 50);       // define the button to pin 4, 50ms debounce. FX2 switch
Button footSw3(5, 50);       // define the button to pin 5, 50ms debounce. FX3 switch
Button footSw4(6, 50);       // define the button to pin 6, 50ms debounce. Tap/Loop switch
const unsigned long LONG_PRESS(1000);     // we define a "long press" to be 1000 milliseconds.
int long_press_release = 0; // so we know a long press occured, and mask out a switch release operation directly after.


#define TOTAL_LED 5

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

int count = 0;
const int led1 = A0;
const int led2 = A1;
const int led3 = A2;
const int led4 = A3;
const int led5 = A6;
byte ledArray[] = {led1, led2, led3, led4, led5};
int fx1State = 0;
int fx2State = 0;
int fx3State = 0;
int loopState = 0;
int bstr_rgy_state = 0;
int mod_rgy_state = 0;
int dd1_rgy_state = 0;
int fx_rgy_state = 0;
int revdd2_rgy_state = 0;
int currentChannel = 0;
String message_1 = " ";
String message_2 = " ";
String message_3 = " ";
String message_4 = " ";
String message_5 = " ";
String message_6 = " ";
int type, MIDIchannel, data1, data2, number, value;
int midiByte = 0 ;
const unsigned long msg = 0;
int chnMode = 0;
unsigned long previousMillis = 0;         // will store last time LED was updated - used for blinking when select bank 2 is active
const long interval = 300;                // blink time for  select bank 2 is active
int ledState = LOW;                       // blink time for  select bank 2 is active
int set_rgy_select = 0;
int rgy_num = 0;
int tempo = 120;
int pedalVal = 0; // pedal position
int lastPedalVal = 0; // used to see if there is a change in pedal position since last reading
int pedalOn = 0; // was the exp pedal used?
unsigned long pedalDelay = 1000; // If epression pedal hasn't moved for this time switch effect off
unsigned long lastRead = 0;

// Initialize LCD
#define I2C_ADDR1    0x27 // LCD1 Address [A0=open   A1=open   A2=open  ]
//#define I2C_ADDR1    0x3F // LCD1 Address [A0=open   A1=open   A2=open  ]
#define BACKLIGHT_PIN 3
#define EN_PIN  2
#define RW_PIN  1
#define RS_PIN  0
#define D4_PIN  4
#define D5_PIN  5
#define D6_PIN  6
#define D7_PIN  7

LiquidCrystal_I2C  lcd1(I2C_ADDR1, EN_PIN, RW_PIN, RS_PIN, D4_PIN, D5_PIN, D6_PIN, D7_PIN);
// Note: some lcd I2C chipsets with all address links open are 0x27 and some are 0x3F.

//###########################
// Loop to turn all LEDs off
void setAllLEDs(int state) {
  for (int i = 0; i < TOTAL_LED; i++) {
    digitalWrite(ledArray[i], state);
  }
}

//###########################
// blink the leds at start up
void blinkAllLeds(int numTimes, int inDelay) {
  lcd1.setCursor(0, 1);
  for (int j = 0; j < numTimes; j++) {
    for (int i = 0; i < TOTAL_LED; i++) {
      digitalWrite(ledArray[i], HIGH);
      delay(inDelay);
      digitalWrite(ledArray[i], LOW);
      lcd1.print(".");
    }
  }
}


/**
   Incoming data handler.
*/
void parseData(unsigned long parameter, byte data) {
  switch (parameter) {

    // Refresh all effect states on patch changes.
    case P_PATCH:
      Serial.print(F("Loaded patch "));
      Serial.print(data);
      Serial.println(F("."));
      if (data != 0) {
        currentChannel = data;
        //Serial.print(" data = ");
        //Serial.println(data);
        //Serial.println(currentChannel);
      }
      delay(40);
      for (byte i = 0; i < CHECK_THIS_SIZE; i++) {
        MS3.read(CHECK_THIS[i], 0x01);
      }
      timerStart = millis();
      checked = 0;
      break;

    // Store the effect state for printing later.
    default:
      for (byte i = 0; i < CHECK_THIS_SIZE; i++) {
        if (CHECK_THIS[i] == parameter) {
          bitWrite(states, i, data);
          byte_array[i] = data;
          bitSet(checked, i);
          changed = true;
          break;
        }
      }
  }
}

/**
   Print all effect states.
*/
void printStatus(unsigned long duration) {
  byte dataReceived = 0;
  for (byte i = 0; i < CHECK_THIS_SIZE; i++) {
    if (bitRead(checked, i)) {
      dataReceived++;
    }
  }

  Serial.println();
  Serial.print(F("Received "));
  Serial.print(dataReceived);
  Serial.print(F("/"));
  Serial.print(CHECK_THIS_SIZE);
  Serial.print(F(" effect states in "));
  Serial.print(duration);
  Serial.println(F("ms."));
  Serial.println();

  String txt = "Sneaky ";
  int a;
  char state[8];
  for (byte i = 0; i < CHECK_THIS_SIZE; i++) {

    strcpy(state, (bitRead(states, i) ? "on " : "off"));

    switch (CHECK_THIS[i]) {
      case P_BSTR:
        //Serial.println(F("BOOSTER:  ") + String(state));
        message_1 = "FX1 " + String(state);
        fx1State = bitRead(states, i);
        digitalWrite(led2, fx1State);
        break;

      case P_MOD:
        //Serial.println(F("MOD: ")+String(state));
        break;

      case P_SR:
        //Serial.println(F("SR Loop:   ") + String(state));
        message_4 = "Loop " + String(state);
        loopState = bitRead(states, i);
        digitalWrite(led5, loopState);
        break;

      case P_DD1:
        //Serial.println(F("Delay 1:   ") + String(state));
        break;
      case P_DD2:
        //Serial.println(F("Delay 2:   ") + String(state));
        break;
      case P_FX:
        //Serial.println(F("FX:  ") + String(state));
        message_2 = "FX2 " + String(state);
        fx2State = bitRead(states, i);
        digitalWrite(led3, fx2State);
        break;

      case P_AMP:
        //Serial.print(F("AMP Type:  "));
        //Serial.print(String(byte_array[i])+" ");
        a = (byte_array[i]);
        if (a == 1) {
          txt = "Acoustc";
        };
        if (a == 8) {
          txt = "Clean  ";
        };
        if (a == 11) {
          txt = "Crunch ";
        };
        if (a == 24) {
          txt = "Lead   ";
        };
        if (a == 23) {
          txt = "Brown  ";
        };
        //Serial.println(txt);
        message_6 = "AMP " + txt;
        break;

      case P_REV:
        //Serial.println(F("REV:  ") + String(state));
        message_3 = "FX3 " + String(state);
        fx3State = bitRead(states, i);
        digitalWrite(led4, fx3State);
        break;

      case P_BSTR_RGY:
        a = (byte_array[i]);
        bstr_rgy_state = a;
        //Serial.println(F("Booster RGY: ") + String(a));
        rgy_state(a);
        break;

      case P_MOD_RGY:
        a = (byte_array[i]);
        mod_rgy_state = a;
        //Serial.println(F("MOD RGY: ") + String(a));
        break;

      case P_DD1_RGY:
        a = (byte_array[i]);
        dd1_rgy_state = a;
        //Serial.println(F("Delay 1 RGY: ") + String(a));
        break;

      case P_FX_RGY:
        a = (byte_array[i]);
        fx_rgy_state = a;
        //Serial.println(F("FX RGY: ")  + String(a));
        break;

      case P_REVDD2_RGY:
        a = (byte_array[i]);
        revdd2_rgy_state = a;
        //Serial.println(F("Reverb/Delay 2 RGY: ") + String(a));
        break;

      default:
        Serial.print(CHECK_THIS[i]);
        Serial.print(F(": "));
        Serial.println(state);

    }
  }
  updateLCD1 ();
  //Serial.println();
  Serial.print(F("byte_Array:"));
  for (int f = 0; f < 16; f++) {
    Serial.print(String(byte_array[f]) + " ");
  }
  Serial.println();
}

// If USB connection is lost set the katana back in BTS edit mode
void setEdit(void) {
  unsigned long test = 0;
  byte dataTest = 0;
  Serial.println();
  Serial.println(F("Waiting..."));
  Serial.println();
  MS3.begin();
  delay(100);
  switch (MS3.update(test, dataTest)) {
    case MS3_READY:
      Serial.println(F("############ Now I'm ready!"));
      Serial.println();
      MS3.setEditorMode();
      MS3.read(PC, 0x02);
      break;
  }
}
/**
   Setup routine.*****************************************************
*/
void setup() {
  // Wire.begin();
  Serial.begin(115200);
  //Serial.begin(31250);
  delay(1000);
  MIDI.begin(MIDI_CHANNEL_OMNI);

  footSw1.begin();
  footSw2.begin();
  footSw3.begin();
  footSw4.begin();
  footSw5.begin();

  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);
  pinMode(led4, OUTPUT);
  pinMode(led5, OUTPUT);

  // set LEDS and initial display text
  lcd1.begin (20, 4); //  LCD1 is 20x4

  lcd1.setBacklightPin(BACKLIGHT_PIN, POSITIVE);
  lcd1.setBacklight(HIGH);
  lcd1.createChar(0, custom_O);  // load custom characters in LCD CGRAM
  lcd1.createChar(1, custom_G);
  lcd1.createChar(2, custom_Y);
  lcd1.createChar(3, custom_R);
  lcd1.setCursor(0, 0);
  lcd1.print("Initializing...");
  blinkAllLeds(1, 200);
  setAllLEDs(LOW);
  lcd1.clear();

  //while (!Serial) {}  // comment out if not deBugging

  if (!MS3.begin()) {
    Serial.println(F("*** USB / MS3 init error! ***"));
    while (true);
  }

  Serial.println(F("Ready!"));
  Serial.println();
  Wire.setClock(1000000L); // Set wire clock speed - this may depend on LCD/i2c interface
  //Wire.setClock(400000L); // if 1000000L doesn't work try this
  Serial.println("Wire speed:  ");
  Serial.println(Wire.getClock()); // remove this line if using Wire.h instead of i2c_t3.h

  // Set editor mode - This is necessary because the Katana unsets
  // editor mode if powered off
  unsigned long parameter = 0;
  byte data = 0;
  int editMode = 0;
  while (editMode == 0) {
    switch (MS3.update(parameter, data)) {
      case MS3_NOT_READY:
        Serial.println(F("Katana OFFLINE !"));
        Serial.println();
        message_1 = "*      KATANA      *";
        message_2 = "*      OFFLINE     *";
        updateLCD1 ();
        delay(500);
        break;
      case MS3_READY:
        MS3.unSetEditorMode(); // Just to be safe, setting editor mode twice might cause problems, TBD
        MS3.setEditorMode();
        MS3.read(P_PATCH, 0x02);
        lcd1.clear();
        updateLCD1 ();
        editMode = 1;
        Serial.println(F("Ready!"));
        break;
    }
  }

}


/**
   Main loop.*********************************************************************
*/
void loop() {
  static unsigned long timerStop = 0;

  // The MS-3 library stores the parameter and data in these variables.
  unsigned long parameter = 0;
  byte data = 0;

  // Check for incoming data or send a queued item.
  switch (MS3.update(parameter, data)) {
    case MS3_NOT_READY:
      Serial.println(F("Katana OFF-LINE !"));
      Serial.println();
      message_1 = "*      KATANA      *";
      message_2 = "*     OFF-LINE     *";
      updateLCD1 ();
      break;

    case MS3_READY:
      MS3.unSetEditorMode(); // Needed if Katana is powered off
      MS3.setEditorMode();  // Needed if Katana is powered off
      MS3.read(P_PATCH, 0x02);
      lcd1.clear();
      updateLCD1 ();
      Serial.println("Katan Ready again");
      break;

    // Parse the incoming data.
    case MS3_DATA_RECEIVED:
      parseData(parameter, data);
#ifdef MS3_OVERRIDE_MODE
      printStatus(millis() - timerStart);
#endif
      break;

    // If all communication is done, print the result.
    case MS3_IDLE:
      if (changed)
      {
        printStatus(millis() - timerStart);
        changed = false;
      }
      break;
  }


  // Blink LED1 if chnMode == 2)
  unsigned long currentMillis = millis();
  if (chnMode == 2 ) {
    if (currentMillis - previousMillis >= interval) {
      // save the last time you blinked the LED
      previousMillis = currentMillis;
      // if the LED is off turn it on and vice-versa:
      if (ledState == LOW) {
        ledState = HIGH;
      } else {
        ledState = LOW;
      }
      // set the LED with the ledState of the variable:
      digitalWrite(led1, ledState);
    }
  }
  // End blink LED1 if chnMode == 2)


  // Blink LED1,2,3 if in rgy select mode)
  if (set_rgy_select > 0) {
    if (currentMillis - previousMillis >= interval) {
      // save the last time you blinked the LED
      previousMillis = currentMillis;
      // if the LED is off turn it on and vice-versa:
      if (ledState == LOW) {
        ledState = HIGH;
      } else {
        ledState = LOW;
      }
      // set the LED with the ledState of the variable:
      digitalWrite(led2, ledState);
      digitalWrite(led3, ledState);
      digitalWrite(led4, ledState);
    }
  }
  // End blink LED1,2,3 if in rgy select mode)


  //************************************************** FOOT SWITCH ROUTINES *********************
  read_footSw();
  if (set_rgy_select == 0) {



    //****************************** FX1 FOOTSWITCH ********************************

    if (footSw1.pressedFor(LONG_PRESS) && long_press_release < 1) {
      setAllLEDs(LOW);
      set_rgy_select = 1;
      long_press_release = 1;
      rgy_num = 1;
      rgy_set();
    }
    if (footSw1.wasReleased()) {
      if (chnMode == 1) {
        setAllLEDs(LOW);
        currentChannel = 1;
        MS3.write(PC, 0x0001, 2);
        //MS3.write(PC, 0x0001, 2);
        chnMode = 0;
      }
      else if (chnMode == 2) {
        setAllLEDs(LOW);
        currentChannel = 5;
        MS3.write(PC, 0x0005, 2);
        //MS3.write(PC, 0x0005, 2);
        chnMode = 0;
      }
      else if  (fx1State == 0 ) {
        digitalWrite(led2, HIGH);
        fx1State = 1;
        MS3.write(fx1_sw, 0x01, 1);
        //MS3.write(P_MOD, 0x01, 1);
      }
      else if  (fx1State == 1 )  {
        digitalWrite(led2, LOW);
        fx1State = 0;
        MS3.write(fx1_sw, 0x00, 1);
        //MS3.write(P_MOD, 0x00, 1);
      }
    } else {
      long_press_release = 0;
    }


    //****************************************FX2 FOOTSWITCH *********************************

    if (footSw2.pressedFor(LONG_PRESS) && long_press_release < 1) {
      setAllLEDs(LOW);
      set_rgy_select = 1;
      long_press_release = 1;
      rgy_num = 2;
      rgy_set();
    }
    if (footSw2.wasReleased()) {
      if (chnMode == 1) {
        setAllLEDs(LOW);
        currentChannel = 2;
        MS3.write(PC, 0x0002, 2);
        //MS3.write(PC, 0x0002, 2);
        chnMode = 0;
      }
      else if (chnMode == 2) {
        setAllLEDs(LOW);
        currentChannel = 6;
        MS3.write(PC, 0x0006, 2);
        //MS3.write(PC, 0x0006, 2);
        chnMode = 0;
      }
      else if  (fx2State == 0 ) {
        digitalWrite(led3, HIGH);
        fx2State = 1;
        MS3.write(fx2_sw, 0x01, 1);
        //MS3.write(P_DD1, 0x01, 1);
      }
      else if (fx2State == 1 ) {
        digitalWrite(led3, LOW);
        fx2State = 0;
        MS3.write(fx2_sw, 0x00, 1);
        //MS3.write(P_DD1, 0x00, 1);
      }
    } else {
      long_press_release = 0;
    }


    //****************************************FX3 FOOTSWITCH *********************************
    if (footSw3.pressedFor(LONG_PRESS) && long_press_release < 1) {
      setAllLEDs(LOW);
      set_rgy_select = 1;
      long_press_release = 1;
      rgy_num = 3;
      rgy_set();
    }
    if (footSw3.wasReleased()) {
      if (chnMode == 1) {
        setAllLEDs(LOW);
        currentChannel = 3;
        MS3.write(PC, 0x0003, 2);
        //MS3.write(PC, 0x0003, 2);
        chnMode = 0;
      }
      else if (chnMode == 2) {
        setAllLEDs(LOW);
        currentChannel = 7;
        MS3.write(PC, 0x0007, 2);
        // MS3.write(PC, 0x0007, 2);
        chnMode = 0;
      }
      else if  (fx3State == 0 ) {
        digitalWrite(led4, HIGH);
        fx3State = 1;
        MS3.write(fx3_sw, 0x01, 1);
        //MS3.write(P_DD2, 0x01, 1);
        //message_3 = "FX3 on    ";
      }
      else if  (fx3State == 1 ) {
        digitalWrite(led4, LOW);
        fx3State = 0;
        MS3.write(fx3_sw, 0x00, 1);
        //MS3.write(P_DD2, 0x00, 1);
      }
    } else {
      long_press_release = 0;
    }


    //****************************************Tap/Loop FOOTSWITCH *********************************
    if (footSw4.pressedFor(LONG_PRESS) && long_press_release < 1) {
      setAllLEDs(LOW);
      if  (loopState == 0 ) {
        digitalWrite(led5, HIGH);
        loopState = 1;
        MS3.write(Loop_sw, 0x01, 1);
        MS3.write(Loop_sw, 0x01, 1);
      }
      else if (loopState == 1) {
        digitalWrite(led5, LOW);
        loopState = 0;
        MS3.write(Loop_sw, 0x00, 1);
        MS3.write(Loop_sw, 0x00, 1);
      }
      long_press_release = 1;
      while (footSw4.isPressed()) {
        footSw4.read();
      }; // wait until released so not to repeat.
    }

    if (footSw4.wasReleased()) {
      if (long_press_release < 1) {
        if (chnMode == 1) {
          setAllLEDs(LOW);
          currentChannel = 4;
          MS3.write(PC, 0x0004, 2);
          //MS3.write(PC, 0x0004, 2);
          chnMode = 0;
        }
        else if (chnMode == 2) {
          setAllLEDs(LOW);
          currentChannel = 8;
          MS3.write(PC, 0x0008, 2);
          //MS3.write(PC, 0x0008, 2);
          chnMode = 0;
        } else {
          long_press_release = 0;
        }
      }
      else {
        // do tap tempo thing here
      }
    }


    // Set switch 5 (Function) to toggle bank 1,2 or fx mode, long press selects Panel channel.************

    if (footSw5.pressedFor(LONG_PRESS)) {
      setAllLEDs(LOW);
      currentChannel = 0;
      MS3.write(PC, 0x0000, 2);
      long_press_release = 1;
      while (footSw5.isPressed()) {
        footSw5.read();
      }; // wait until released.
    }

    if (footSw5.wasReleased()) {
      if (long_press_release < 1) {
        if (chnMode == 0) {
          digitalWrite(led1, HIGH);
          chnMode = 1;
          lcd1.setCursor(9, 0);
          lcd1.print("    Bank 1 ");
        }
        else if (chnMode == 1) {
          chnMode = 2;
          lcd1.setCursor(9, 0);
          lcd1.print("    Bank 2 ");
        }
        else {
          chnMode = 0;
          digitalWrite(led1, LOW);
          lcd1.setCursor(9, 0);
          lcd1.print("    FX mode");
        }
      } else {
        long_press_release = 0;
      }
    }
    // End set switch 5 (Function) to toggle bank 1,2 or fx mode, long press selects Panel channel.

  } else {
    rgy_set();  // rgy_set end.
  }
  //////////
  //expression pedal
  if (millis() - lastRead > pedalDelay && pedalVal >= 63 && pedalOn == 1) {
    MS3.write(fx2_sw, 0x00, 1);
    lastRead = millis();
    pedalOn = 0;
    Serial.println("Pedal Off");
  }
  if (millis() - lastRead > 35) {   // This delay is necessary not to overwhelm the MS3 queue
    pedalVal = analogRead(7) / 16;  // Divide by 16 to get range of 0-63 for midi
    if (pedalVal != lastPedalVal) { // If the value does not = the last value the following command is made. This is because the pot has been turned. Otherwise the pot remains the same and no midi message is output.

      if (pedalOn == 0) {
        MS3.write(fx2_sw, 0x01, 1);
        Serial.println("Pedal on command sent!");
        pedalOn = 1;
      }
      else {
        MS3.write(wah_pedal, pedalVal, 2);
        Serial.println(pedalVal);
        lastRead = millis();
      }
    }
  }
  lastPedalVal = pedalVal;  // remeber the last value of the pedal position so we can see if it changed later
  //end expression pedal
  //////////////////


  // Check incoming serial MIDI and translate CC and PC mesages to Katana sysex
  if (MIDI.read()) {                // Is there a MIDI message incoming ?
    MIDIchannel = MIDI.getChannel();
    if (MIDIchannel == 2) {             // Listen to messages only on MIDI channel 2
      switch (MIDI.getType()) {     // Get the type of the message we caught

        case midi::ProgramChange:
          number = MIDI.getData1();
          if (number < 9) {
            Serial.println(String("Prog.Ch") + (" Pgrm # ") + number);
            Serial.println(String("MIDI Chan ") + MIDIchannel + (" Value ") + MIDI.getData2());
            MS3.write(PC, number, 2);
          }
          break;

        case midi::ControlChange:
          number = MIDI.getData1();
          value = MIDI.getData2();
          Serial.println(String("Control Ch.") + (", CC#") + number);
          Serial.println(String("MIDI chan. ") + MIDIchannel + (",Value ") + MIDI.getData2());
          if (number == 16) {
            MS3.write(fx1_sw, value, 1);
          }
          if (number == 17) {
            MS3.write(fx2_sw, value, 1);
          }
          if (number == 18) {
            MS3.write(fx3_sw, value, 1);
          }
          if (number == 19) {
            MS3.write(Loop_sw, value, 1);
          }
          if (number == 20) {
            MS3.write(rvbYellow, value, 1);
          }

          if (number == 21) {
            MS3.write(P_BSTR, value, 1);
          }

          if (number == 22) {
            MS3.write(P_MOD, value, 1); // booster knob MOD
          }

          if (number == 23) {
            MS3.write(P_FX, value, 1); // FX/Delay knob
          }

          if (number == 24) {
            MS3.write(P_SR, value, 1); // Loop
          }

          if (number == 25) {
            MS3.write(P_DD1, value, 1); // FX/Delay knob
          }

          if (number == 26) {
            MS3.write(P_DD2, value, 1);
          }

          if (number == 27) {
            MS3.write(P_REV, value, 1);
          }

          if (number == 28) {
            MS3.write(P_AMP, value, 1);
          }

          if (number == 29) {
            MS3.write(P_BSTR_RGY, value, 1);
          }

          if (number == 30) {
            MS3.write(P_MOD_RGY, value, 1);
          }

          if (number == 40) {
            MS3.write(P_DD1_RGY, value, 1);
          }

          if (number == 41) {
            MS3.write(P_FX_RGY, value, 1);
          }

          if (number == 42) {
            MS3.write(P_REVDD2_RGY, value, 1);
          }

          //          if (number == 119) {
          //            MS3.write(wah_select, 02, 1);
          //          }

          if (number == 120) {
            MS3.write(wah_pedal, value, 2);

          }

          break;

        case midi::SystemExclusive:
          lcd1.clear();
          lcd1.setCursor(0, 0);
          lcd1.print("Sysex");
          lcd1.setCursor(0, 1);
          lcd1.print(String("MIDI Chan ") + MIDIchannel + (",Value ") + MIDI.getData2());
          Serial.println("Sysex");
          Serial.println(String("MIDI Chan ") + MIDIchannel + (",Value ") + MIDI.getData2());
          break;
        default:
          break;
      }
    }
  }
}


void updateLCD1 (void) {
  if (message_1 == "*      KATANA      *") { // if offline message, clear display and do something different.
    lcd1.clear();
    lcd1.setCursor(0, 0);
    for (int x = 0; x < 20; x++) {
      lcd1.print("*");
    };
    lcd1.setCursor(0, 1);
    lcd1.print(message_1);
    lcd1.setCursor(0, 2);
    lcd1.print(message_2);
    lcd1.setCursor(0, 3);
    for (int x = 0; x < 20; x++) {
      lcd1.print("*");
    };
    delay(200);
    lcd1.clear();
  } else
  {
    lcd1.setCursor(0, 0);
    if (currentChannel < 1) {
      lcd1.print("Panel    ");
    } else {
      lcd1.print("Channel ");
      lcd1.print(String(currentChannel));
    }

    lcd1.setCursor(0, 1);
    lcd1.print(message_1);
    rgy_state(bstr_rgy_state);

    lcd1.setCursor(0, 2);
    lcd1.print(message_2);
    rgy_state(fx_rgy_state);

    lcd1.setCursor(0, 3);
    lcd1.print(message_3);
    rgy_state(revdd2_rgy_state);

    if (chnMode == 0) {
      lcd1.setCursor(9, 0);
      lcd1.print("    FX mode");
    }

    lcd1.setCursor(12, 1);
    lcd1.print(message_4);

    lcd1.setCursor(10, 2);
    lcd1.print("TAP ");
    lcd1.print(String(tempo)); // message_5
    lcd1.print("bpm");

    lcd1.setCursor(9, 3);
    lcd1.print(message_6);
  }
}

void rgy_state(int a) {
  lcd1.write(byte(a + 1));
}

void rgy_set() {
loop1:
  read_footSw();
  if ((footSw1.isPressed() || footSw2.isPressed() || footSw3.isPressed()) && long_press_release == 1 ) {
    goto loop1;  // wait here until button is released
  }

  read_footSw();

  if (footSw1.isPressed()) {
    long_press_release = 0;
    set_rgy_select = 0;
    Serial.println(F("GREEN selected for FX") + String(rgy_num));
    if (rgy_num == 1) {
      MS3.write(P_BSTR_RGY, 0x00, 1);
      MS3.write(P_MOD_RGY, 0x00, 1);
    }
    else if (rgy_num == 2) {
      MS3.write(P_FX_RGY, 0x00, 1);
      MS3.write(P_DD1_RGY, 0x00, 1);
    }
    else if (rgy_num == 3) {
      MS3.write(P_REVDD2_RGY, 0x00, 1);
    };
  }
  if (footSw2.isPressed()) {
    long_press_release = 0;
    set_rgy_select = 0;
    Serial.println(F("RED selected for FX") + String(rgy_num));
    if (rgy_num == 1) {
      MS3.write(P_BSTR_RGY, 0x01, 1);
      MS3.write(P_MOD_RGY, 0x01, 1);
    }
    else if (rgy_num == 2) {
      MS3.write(P_FX_RGY, 0x01, 1);
      MS3.write(P_DD1_RGY, 0x01, 1);
    }
    else if (rgy_num == 3) {
      MS3.write(P_REVDD2_RGY, 0x01, 1);
    };
  }
  if (footSw3.isPressed()) {
    long_press_release = 0;
    set_rgy_select = 0;
    Serial.println(F("YELLOW selected for FX") + String(rgy_num));
    if (rgy_num == 1) {
      MS3.write(P_BSTR_RGY, 0x02, 1);
      MS3.write(P_MOD_RGY, 0x02, 1);
    }
    else if (rgy_num == 2) {
      MS3.write(P_FX_RGY, 0x02, 1);
      MS3.write(P_DD1_RGY, 0x02, 1);
    }
    else if (rgy_num == 3) {
      MS3.write(P_REVDD2_RGY, 0x02, 1);
    };
  }
  if (footSw4.isPressed()) {
    long_press_release = 0;
    set_rgy_select = 0;
  }
  if (footSw5.isPressed()) {
    long_press_release = 0;
    set_rgy_select = 0;
  }

  if (set_rgy_select == 1) {
    return;
  }
loop2:
  read_footSw();
  if (footSw1.isPressed() || footSw2.isPressed() || footSw3.isPressed()) {
    goto loop2;
  }
  //Serial.println(F("finished RGY loop"));
  rgy_num = 0;
}

void read_footSw() {
  footSw1.read();
  footSw2.read();
  footSw3.read();
  footSw4.read();
  footSw5.read();
}
