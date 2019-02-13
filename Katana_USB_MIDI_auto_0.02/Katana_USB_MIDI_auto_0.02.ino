/**
   This is an example for the MS-3 library.

   What have you done? Let's find out what's happening with all effect blocks.
*/

// Uncomment this to enable verbose debug messages.
#define MS3_DEBUG_MODE

#include "Arduino.h"
#include "MS3.h"
#include <i2c_t3.h> // You can use either Wire or the Teensy i2c_t3 library
//                  // If using the i2c_t3 library you must change the LiquidCrystal_i2c.h and cpp
//#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Bounce2.h>
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
const unsigned long P_BSTR_RGY = 0x00000410; //0x60001210;
const unsigned long P_MOD_RGY =  0x60001211;
const unsigned long P_DD1_RGY =  0x60001212;
const unsigned long P_FX_RGY =   0x00000411; //0x60001213;
const unsigned long P_REVDD2_RGY = 0x00000412; //0x60001214;

// sysex define ################################
const unsigned long PC = 0x00010000; // change channel katana.write(PC, 1, 2) second byte is channel number 3rd is length
const unsigned long fx1_sw = 0x60000030; //turn button 1 on  katana.write(fx1_sw, 1, 1) second byte 0 = off 1 = on
const unsigned long fx2_sw = 0x6000034C; //turn button 2 on  katana.write(fx2_sw, 1, 1) second byte 0 = off 1 = on
const unsigned long fx3_sw = 0x60000610; //turn reverb on  katana.write(fx3_sw, 1, 1) second byte 0 = off 1 = on
const unsigned long Loop_sw = 0x60000655;   // turn loop off on
const unsigned long rvbYellow = 0x60001214; // set reverb type to yellow
const unsigned long express = 0x6000015D; // expression pedal position ??
const unsigned long VOLUME_PEDAL_ADDR = 0x60000633; // volume pedal address
// end sysex define ##############################

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


#define TOTAL_LED 5
#define NUM_BUTTONS 5
const uint8_t BUTTON_PINS[NUM_BUTTONS] = {2, 3, 4, 5, 6};
//MIDI_CREATE_INSTANCE(HardwareSerial,Serial, midiSerial);

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

Bounce * swtch = new Bounce[NUM_BUTTONS];
uint8_t swv[NUM_BUTTONS] = {};
int count = 0;

const int led1 = A0;
const int led2 = A1;
const int led3 = A2;
const int led4 = A3;
const int led5 = A6;
byte ledArray[] = {led1, led2, led3, led4, led5};
int effectsState1 = 0;
int effectsState2 = 0;
int effectsState3 = 0;
int fxLoopstate = 0;
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
//int type, channel, data1, data2, number, value;
const unsigned long msg = 0;
int chnMode = 0;
unsigned long previousMillis = 0;         // will store last time LED was updated - used for blinking when select bank 2 is active
const long interval = 300;                // blink time for  select bank 2 is active
int ledState = LOW;                       // blink time for  select bank 2 is active



// Initialize LCD
LiquidCrystal_I2C lcd1(0x27, 20, 4); // set the LCD address to 0x27 for a 16 chars and 4 line display
// some lcd I2C chips with all address links open are 0x27 and some are 0x3F
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
        Serial.print(" data = ");
        Serial.println(data);
        Serial.println(currentChannel);
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

  String txt = "Sneaky";
  int a;
  char state[8];
  for (byte i = 0; i < CHECK_THIS_SIZE; i++) {

    strcpy(state, (bitRead(states, i) ? "on " : "off"));

    switch (CHECK_THIS[i]) {
      case P_BSTR:
        Serial.println(F("BOOSTER:  ") + String(state));
        message_1 = "FX1 " + String(state);
        effectsState1 = bitRead(states, i);
        digitalWrite(led2, effectsState1);
        break;

      case P_MOD:
        Serial.println(F("MOD: ") + String(state));
        break;

      case P_SR:
        Serial.println(F("SR Loop:   ") + String(state));
        message_4 = "Loop " + String(state);
        fxLoopstate = bitRead(states, i);
        digitalWrite(led5, fxLoopstate);
        break;

      case P_DD1:
        Serial.println(F("Delay 1:   ") + String(state));
        break;
      case P_DD2:
        Serial.println(F("Delay 2:   ") + String(state));
        break;
      case P_FX:
        Serial.println(F("FX:  ") + String(state));
        message_2 = "FX2 " + String(state);
        effectsState2 = bitRead(states, i);
        digitalWrite(led3, effectsState2);
        break;

      case P_AMP:
        Serial.print(F("AMP Type:  "));
        //Serial.print(String(byte_array[i])+" ");
        a = (byte_array[i]);
        if (a == 1) {
          txt = "Acustc";
        };
        if (a == 8) {
          txt = "Clean ";
        };
        if (a == 11) {
          txt = "Crunch";
        };
        if (a == 24) {
          txt = "Lead  ";
        };
        if (a == 23) {
          txt = "Brown ";
        };
        Serial.println(txt);
        message_6 = "AMP " + txt;
        break;

      case P_REV:
        Serial.println(F("REV:  ") + String(state));
        message_3 = "FX3 " + String(state);
        effectsState3 = bitRead(states, i);
        digitalWrite(led4, effectsState3);
        break;

      case P_BSTR_RGY:
        a = (byte_array[i]);
        bstr_rgy_state = a;
        Serial.println(F("Booster RGY: ") + String(a));
        rgy_state(a);
        break;

      case P_MOD_RGY:
        a = (byte_array[i]);
        mod_rgy_state = a;
        Serial.println(F("MOD RGY: ") + String(a));
        break;

      case P_DD1_RGY:
        a = (byte_array[i]);
        dd1_rgy_state = a;
        Serial.println(F("Delay 1 RGY: ") + String(a));
        break;

      case P_FX_RGY:
        a = (byte_array[i]);
        fx_rgy_state = a;
        Serial.println(F("FX RGY: ") + String(a));
        break;

      case P_REVDD2_RGY:
        a = (byte_array[i]);
        revdd2_rgy_state = a;
        Serial.println(F("Reverb/Delay 2 RGY: ") + String(a));
        break;

      default:
        Serial.print(CHECK_THIS[i]);
        Serial.print(F(": "));
        Serial.println(state);

    }
  }
  updateLCD1 ();
  Serial.println();
  Serial.print(F("byte_Array:"));
  for (int f = 0; f < 16; f++) {
    Serial.print(String(byte_array[f]) + " ");
  }
  Serial.println();
}



/**
   Setup routine.*****************************************************
*/
void setup() {
  Wire.begin();
  //Wire.begin(I2C_MASTER, 0x27, 19, 18, I2C_PULLUP_EXT); // This was for testing, it's not needed now. Setting the speed here doesn't work properly
  //Serial.begin(115200);
  //Serial1.begin(31250); // Teensy MIDI Library seems to automaticaly take care of this
  delay(1000);
  MIDI.begin(MIDI_CHANNEL_OMNI);

  for (int i = 0; i < NUM_BUTTONS; i++) {
    swtch[i].attach( BUTTON_PINS[i] , INPUT_PULLUP  );       //setup the bounce instance for the current button
    swtch[i].interval(20);                                   // interval in ms
  }

  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);
  pinMode(led4, OUTPUT);
  pinMode(led5, OUTPUT);

  // set LEDS and initial display text
  lcd1.init();
  lcd1.backlight();
  lcd1.setBacklight(HIGH);
  lcd1.createChar(0, custom_O);  // load custom characters in LCD CGRAM
  lcd1.createChar(1, custom_G);
  lcd1.createChar(2, custom_Y);
  lcd1.createChar(3, custom_R);
  lcd1.setCursor(0, 0);
  blinkAllLeds(1, 200);
  lcd1.print("Initializing...");
  setAllLEDs(LOW);
  lcd1.clear();

  // while (!Serial) {}  comment out if not deBugging

  if (!MS3.begin()) {
    Serial.println(F("*** USB / MS3 init error! ***"));
    while (true);
  }
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
      Serial.println(F("Katana OFFLINE !"));
      Serial.println();
      message_1 = "*      KATANA      *";
      message_2 = "*      OFFLINE     *";
      updateLCD1 ();
      break;
    // Fetch the current active patch on the MS-3.
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
      break;

    // If all communication is done, print the result.
    case MS3_IDLE:
      if (changed) {
        printStatus(millis() - timerStart);
        changed = false;
      }
      break;
  }

  // read the switches
  for (int i = 0; i < NUM_BUTTONS; i++)  {
    swtch[i].update();
  }

  // Set switch 5 to toggle bank 1,2 or fx mode
  if (swtch[0].fell()) {
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
  }
  // End set switch 5 to toggle bank 1,2 or fx mode

  // Blink LED1 if chnMode == 2)
  unsigned long currentMillis = millis();
  if (chnMode == 2) {
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


  if (swtch[1].fell()) {
    if (chnMode == 1) {
      setAllLEDs(LOW);
      currentChannel = 1;
      MS3.write(PC, 0x0001, 2);
      chnMode = 0;
    }
    else if (chnMode == 2) {
      setAllLEDs(LOW);
      currentChannel = 5;
      MS3.write(PC, 0x0005, 2);
      chnMode = 0;
    }
    else if  (effectsState1 == 0 ) {
      //digitalWrite(led2, HIGH);
      effectsState1 = 1;
      MS3.write(fx1_sw, 1, 1);
      //MS3.write(P_MOD, 1, 1); //*** Commented out just because I didn't want to turn on two fx at once
    }
    else if  (effectsState1 == 1 )  {
      //digitalWrite(led2, LOW);
      effectsState1 = 0;
      MS3.write(fx1_sw, 0, 1);
      //MS3.write(P_MOD, 0, 1); //*** Commented out just because I didn't want to turn on two fx at once
    }
  }

  if (swtch[2].fell()) {
    if (chnMode == 1) {
      setAllLEDs(LOW);
      currentChannel = 2;
      MS3.write(PC, 0x0002, 2);
      chnMode = 0;
    }
    else if (chnMode == 2) {
      setAllLEDs(LOW);
      currentChannel = 6;
      MS3.write(PC, 0x0006, 2);
      chnMode = 0;
    }
    else if  (effectsState2 == 0 ) {
      //digitalWrite(led3, HIGH);
      effectsState2 = 1;
      MS3.write(fx2_sw, 1, 1);
      //MS3.write(P_DD1, 1, 1); //*** Commented out just because I didn't want to turn on two fx at once
    }
    else  {
      //digitalWrite(led3, LOW);
      effectsState2 = 0;
      MS3.write(fx2_sw, 0, 1);
      //MS3.write(P_DD1, 0, 1); //*** Commented out just because I didn't want to turn on two fx at once
    }
  }

  if (swtch[3].fell()) {
    if (chnMode == 1) {
      setAllLEDs(LOW);
      currentChannel = 3;
      MS3.write(PC, 0x0003, 2);
      chnMode = 0;
    }
    else if (chnMode == 2) {
      setAllLEDs(LOW);
      currentChannel = 7;
      MS3.write(PC, 0x0007, 2);
      chnMode = 0;
    }
    else if  (effectsState3 == 0 ) {
      //digitalWrite(led4, HIGH);
      effectsState3 = 1;
      MS3.write(fx3_sw, 1, 1);
      //MS3.write(P_DD2, 1, 1); //*** Commented out just because I didn't want to turn on two fx at once
    }
    else  {
      //digitalWrite(led4, LOW);
      effectsState3 = 0;
      MS3.write(fx3_sw, 0, 1);
      //MS3.write(P_DD2, 0, 1); //*** Commented out just because I didn't want to turn on two fx at once
    }
  }

  if (swtch[4].fell()) {
    if (chnMode == 1) {
      setAllLEDs(LOW);
      currentChannel = 4;
      MS3.write(PC, 0x0004, 2);
      chnMode = 0;
    }
    else if (chnMode == 2) {
      setAllLEDs(LOW);
      currentChannel = 8;
      MS3.write(PC, 0x0008, 2);
      chnMode = 0;
    }
    else if  (fxLoopstate == 0 ) {
      //digitalWrite(led5, HIGH);
      fxLoopstate = 1;
      MS3.write(Loop_sw, 1, 1);;
    }
    else if (fxLoopstate == 1) {
      //digitalWrite(led5, LOW);
      fxLoopstate = 0;
      MS3.write(Loop_sw, 0, 1);;
    }
  }

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

          //        if (channel == 2 && number == 119) {
          //          katana.write(wah, 02, 1);
          //        }
          if (number == 120) {
            MS3.write(express, value, 2);
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
    for (int x = 0; x < 20; x++)
    {
      lcd1.print("*");
    }
    lcd1.setCursor(0, 1);
    lcd1.print(message_1);
    lcd1.setCursor(0, 2);
    lcd1.print(message_2);
    lcd1.setCursor(0, 3);
    for (int x = 0; x < 20; x++)
    {
      lcd1.print("*");
    }
    delay(200);
    lcd1.clear();
  } else {
    lcd1.setCursor(0, 0);
    lcd1.print("Channel ");
    lcd1.print(String(currentChannel));

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

    //lcd1.setCursor(10, 2);
    //lcd1.print(message_5);

    lcd1.setCursor(10, 3);
    lcd1.print(message_6);
  }
}

void rgy_state(int a) {
  if (a == 0) {
    lcd1.write(byte(0));
  }
  if (a == 1) {
    lcd1.write(byte(1));
  }
  if (a == 2) {
    lcd1.write(byte(2));
  }
  if (a == 3) {
    lcd1.write(byte(3));
  }
}
