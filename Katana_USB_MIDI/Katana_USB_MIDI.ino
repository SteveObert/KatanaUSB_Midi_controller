// Uncomment this to enable verbose debug messages.
//#define MS3_DEBUG_MODE

#include "Arduino.h"
#include "MS3.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Bounce2.h>
#include <MIDI.h>

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
int fxLoopstate = 1;
int currentChannel = 1;
String message1 = " ";
String message2 = " ";
String message3 = " ";
String message4 = " ";
int midiByte = 0 ;
int type, channel, data1, data2, number, value;
const unsigned long msg = 0;
int chnMode = 0;
//char* message[]={"PC 1", "PC 2", "PC 3", "PC 4", "PC 5","PC 6", "PC 7","PC 8"};
//char* effectsButton[] = {"On", "Off"};
unsigned long previousMillis = 0;         // will store last time LED was updated - used for blinking when select bank 2 is active
const long interval = 300;                // blink time for  select bank 2 is active
int ledState = LOW;                       // blink time for  select bank 2 is active


// sysex define ################################
const unsigned long PC = 0x00010000; // change channel katana.write(PC, 1, 2) second byte is channel number 3rd is length
const unsigned long CC16 = 0x60000030; //turn button 1 on  katana.write(CC16, 1, 1) second byte 0 = off 1 = on
const unsigned long CC17 = 0x6000034C; //turn button 2 on  katana.write(CC17, 1, 1) second byte 0 = off 1 = on
const unsigned long CC18 = 0x60000610; //turn reverb on  katana.write(CC18, 1, 1) second byte 0 = off 1 = on
const unsigned long Loop = 0x60000655;   // turn loop off on
//const unsigned long rvbRed = 0x60001214; // set reverb type to red
const unsigned long rvbYellow = 0x60001214; // set reverb type to yellow
// end sysex define ##############################

//###########################
// Initialize the MS3 class.
MS3 katana;

// Initialize LCD
LiquidCrystal_I2C lcd1(0x27, 20, 4); // set the LCD address to 0x27 for a 16 chars and 4 line display

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
  //  lcd1.setCursor(0,1);
  for (int j = 0; j < numTimes; j++) {
    for (int i = 0; i < TOTAL_LED; i++) {
      digitalWrite(ledArray[i], HIGH);
      delay(inDelay);
      digitalWrite(ledArray[i], LOW);
      lcd1.print(".");
    }
  }
}

//###########################
void setup() {
  Wire.begin();
  //Wire.setClock(1000000);
  //while (!Serial);
  //Serial.begin(115200);
  Serial.begin(31250);
  delay(1000);
  MIDI.begin(MIDI_CHANNEL_OMNI);

  for (int i = 0; i < NUM_BUTTONS; i++) {
    swtch[i].attach( BUTTON_PINS[i] , INPUT_PULLUP  );       //setup the bounce instance for the current button
    swtch[i].interval(50);                                   // interval in ms
  }

  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);
  pinMode(led4, OUTPUT);
  pinMode(led5, OUTPUT);

  // set LEDS and initial display text
  lcd1.init();
  lcd1.backlight();
  lcd1.setCursor(0, 0);
  lcd1.print("Initializing...");
  blinkAllLeds(1, 200);
  setAllLEDs(LOW);
  lcd1.clear();

  if (!katana.begin()) {
    Serial.println(F("*** USB / MS3 init error! ***"));
    while (true);
  }
  Serial.println(F("Ready!"));
  Serial.println();

  // Set the katana into BTS edit mode
  // This allows for patch changes and effects to be changed
  unsigned long parameter = 0;
  byte data = 0;
  switch (katana.update(parameter, data)) {
    case MS3_READY:
      katana.setEditorMode();
      katana.read(PC, 0x02);
      lcd1.clear();
      lcd1.setCursor(0, 0);
      lcd1.print("Katana ready");
      break;
  }
}

//###########################
// Query the katana
void parseData(unsigned long parameter, byte data) {
  switch (parameter) {
    case msg:
      break;
    default:
      break;
  }
}

void updateLCD1 (void) {
  lcd1.setCursor(0, 0);
  lcd1.print("Channel ");
  lcd1.print(currentChannel);
  lcd1.setCursor(0, 1);
  lcd1.print(message1);
  lcd1.setCursor(0, 2);
  lcd1.print(message2);
  lcd1.setCursor(0, 3);
  lcd1.print(message3);
  if (chnMode == 0) {
    lcd1.setCursor(9, 0);
    lcd1.print("    FX mode");
  }
  lcd1.setCursor(12, 1);
  lcd1.print(message4);
  //    Serial.print("channel: ");
  //    Serial.println(currentChannel);
  //    Serial.println(message1);
  //    Serial.println(message2);
  //    Serial.println(message3);
}

void clearFX (void) {
  message1 = "FX1 off    ";
  message2 = "FX2 off    ";
  message3 = "FX3 off    ";
  message4 = "Loop on ";
}

//###########################
//###########################
// If USB connection is lost set the katana back in BTS edit mode
void setEdit(void) {
  unsigned long test = 0;
  byte dataTest = 0;
  Serial.println();
  Serial.println(F("Waiting..."));
  Serial.println();
  //MS3 katana;
  katana.begin();
  //katana.update(test, dataTest);
  delay(100);
  switch (katana.update(test, dataTest)) {
    case MS3_READY:
      Serial.println(F("############ Now I'm ready!"));
      Serial.println();
      katana.setEditorMode();
      katana.read(PC, 0x02);
      //lcd1.clear();
      lcd1.setCursor(12, 2);
      lcd1.print("Katana");
      lcd1.setCursor(5, 3);
      lcd1.print("       Ready   ");

      break;
  }
}
//###########################
//###########################
void loop() {

  // Check for incoming data or send a queued item.
  unsigned long parameter = 0;
  byte data = 0;
  switch (katana.update(parameter, data)) {
    case MS3_NOT_READY:
      Serial.println(F("Katana NOT READY!"));
      Serial.println();
      lcd1.setCursor(12, 2);
      lcd1.print("Katana");
      lcd1.setCursor(5, 3);
      lcd1.print("Not Responding! ");
      //delay(500);  // removed 20180119 10:02
      setEdit();
      break;

    // Read incoming data
    case MS3_DATA_RECEIVED:
      //parseData(parameter, data);
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
      //digitalWrite(led2, HIGH);
      currentChannel = 1;
      katana.write(PC, currentChannel, 2);
      chnMode = 0;
      clearFX();
      updateLCD1();
    }
    else if (chnMode == 2) {
      setAllLEDs(LOW);
      //digitalWrite(led2, HIGH);
      currentChannel = 5;
      katana.write(PC, currentChannel, 2);
      chnMode = 0;
      clearFX();
      updateLCD1();
    }
    else if  (effectsState1 == 0 ) {
      //setAllLEDs(LOW);
      digitalWrite(led2, HIGH);
      effectsState1 = 1;
      katana.write(CC16, 1, 1);;
      message1 = "FX1 on    ";
      updateLCD1();
    }
    else if  (effectsState1 == 1 )  {
      //setAllLEDs(LOW);
      digitalWrite(led2, LOW);
      effectsState1 = 0;
      katana.write(CC16, 0, 1);;
      message1 = "FX1 off    ";
      updateLCD1();
    }
  }

  if (swtch[2].fell()) {
    if (chnMode == 1) {
      setAllLEDs(LOW);
      //digitalWrite(led2, HIGH);
      currentChannel = 2;
      katana.write(PC, currentChannel, 2);
      chnMode = 0;
      clearFX();
      updateLCD1();
    }
    else if (chnMode == 2) {
      setAllLEDs(LOW);
      //digitalWrite(led2, HIGH);
      currentChannel = 6;
      katana.write(PC, currentChannel, 2);
      chnMode = 0;
      clearFX();
      updateLCD1();
    }
    else if  (effectsState2 == 0 ) {
      //setAllLEDs(LOW);
      digitalWrite(led3, HIGH);
      effectsState2 = 1;
      katana.write(CC17, 1, 1);;
      message2 = "FX2 on    ";
      updateLCD1();
    }
    else  {
      //setAllLEDs(LOW);
      digitalWrite(led3, LOW);
      effectsState2 = 0;
      katana.write(CC17, 0, 1);;
      message2 = "FX2 off    ";
      updateLCD1();
    }
  }

  if (swtch[3].fell()) {
    if (chnMode == 1) {
      setAllLEDs(LOW);
      //digitalWrite(led2, HIGH);
      currentChannel = 3;
      katana.write(PC, currentChannel, 2);
      chnMode = 0;
      clearFX();
      updateLCD1();
    }
    else if (chnMode == 2) {
      setAllLEDs(LOW);
      //digitalWrite(led2, HIGH);
      currentChannel = 7;
      katana.write(PC, currentChannel, 2);
      chnMode = 0;
      clearFX();
      updateLCD1();
    }
    else if  (effectsState3 == 0 ) {
      //setAllLEDs(LOW);
      digitalWrite(led4, HIGH);
      effectsState3 = 1;
      katana.write(CC18, 1, 1);;
      message3 = "FX3 on    ";
      updateLCD1();
    }
    else  {
      //setAllLEDs(LOW);
      digitalWrite(led4, LOW);
      effectsState3 = 0;
      katana.write(CC18, 0, 1);;
      message3 = "FX3 off    ";
      updateLCD1();
    }
  }

  if (swtch[4].fell()) {
    if (chnMode == 1) {
      setAllLEDs(LOW);
      //digitalWrite(led2, HIGH);
      currentChannel = 4;
      katana.write(PC, currentChannel, 2);
      chnMode = 0;
      clearFX();
      updateLCD1();
    }
    else if (chnMode == 2) {
      setAllLEDs(LOW);
      //digitalWrite(led2, HIGH);
      currentChannel = 8;
      katana.write(PC, currentChannel, 2);
      chnMode = 0;
      clearFX();
      updateLCD1();
    }
    else if  (fxLoopstate == 0 ) {
      //setAllLEDs(LOW);
      digitalWrite(led5, HIGH);
      fxLoopstate = 1;
      katana.write(Loop, 1, 1);;
      message4 = "Loop on ";
      updateLCD1();
    }
    else if (fxLoopstate == 1) {
      //setAllLEDs(LOW);
      digitalWrite(led5, LOW);
      fxLoopstate = 0;
      katana.write(Loop, 0, 1);;
      message4 = "Loop off";
      updateLCD1();
    }
  }


  // Check incoming serial MIDI and translate CC and PC mesages to Katana sysex
  if (MIDI.read())                // Is there a MIDI message incoming ?
  {
    switch (MIDI.getType())     // Get the type of the message we caught
    {

      case midi::ProgramChange:
        channel = MIDI.getChannel();
        number = MIDI.getData1();
        //lcd1.clear();
        lcd1.setCursor(0, 1);
        lcd1.print(String("Amp Ch:  ") + number + (" ,Value ") + MIDI.getData2());
        //lcd1.setCursor(0, 1);
        //lcd1.print(String("Chan ") + channel + (",Value ") + MIDI.getData2());
        Serial.println(String("Prog.Ch") + (",Pgrm#") + number);
        Serial.println(String("Chan ") + channel + (",Value ") + MIDI.getData2());
        if (channel == 2) {
          katana.write(PC, number, 2);
        }
        break;
      case midi::ControlChange:
        channel = MIDI.getChannel();
        number = MIDI.getData1();
        value = MIDI.getData2();
        lcd1.clear();
        lcd1.setCursor(0, 2);
        lcd1.print(String("Ctl.Ch") + (", CC# ") + number);
        lcd1.setCursor(0, 3);
        lcd1.print(String("Value: ") + MIDI.getData2());
        Serial.println(String("Ctl.Ch") + (", CC#") + number);
        Serial.println(String("Chan ") + channel + (",Value ") + MIDI.getData2());
        if (channel == 2 && number == 16) {
          katana.write(CC16, value, 1);
        }
        if (channel == 2 && number == 17) {
          katana.write(CC17, value, 1);
        }
        if (channel == 2 && number == 18) {
          katana.write(CC18, value, 1);
        }
        if (channel == 2 && number == 19) {
          katana.write(Loop, value, 1);
        }
        if (channel == 2 && number == 20) {
          katana.write(rvbYellow, value, 1);
        }
        break;
      case midi::SystemExclusive:
        channel = MIDI.getChannel();
        lcd1.clear();
        lcd1.setCursor(0, 2);
        lcd1.print("Sysex");
        lcd1.setCursor(0, 3);
        lcd1.print(String("Chan ") + channel + (",Value ") + MIDI.getData2());
        Serial.println("Sysex");
        Serial.println(String("Chan ") + channel + (",Value ") + MIDI.getData2());
        break;
      default:
        break;
    }
  }
}
