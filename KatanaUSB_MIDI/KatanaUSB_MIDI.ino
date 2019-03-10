// Put all three files in the same directory before compiling!

// Uncomment this to enable verbose debug messages.
//#define MS3_DEBUG_MODE
//uncomment below to debug withot the Katana connected.
//#define MS3_OVERRIDE_MODE

#include "MS3.h"
//#include <Wire.h>       //use this for Teensy LC
#include <i2c_t3.h>   // use this for Teensy 3.1/3.2
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <JC_Button.h>
#include <MIDI.h>

#define TOTAL_LED 5

// Initialize the MS3 class.
MS3 MS3;

// These are addresses for the effect states.
static const uint32_t P_BSTR_SW =    0x60000030;
static const uint32_t P_BSTR_TYPE =  0x60000031;
static const uint32_t P_AMP =        0x60000051;
static const uint32_t P_MOD_SW =     0x60000140;
static const uint32_t P_FX_SW =      0x6000034C;
static const uint32_t P_DD1_SW =     0x60000560;
static const uint32_t P_DD1_TIME =   0x60000562;
static const uint32_t P_REV_SW =     0x60000610;
static const uint32_t P_SR_SW =      0x60000655;
static const uint32_t P_DD2_SW =     0x6000104E;
static const uint32_t P_DD2_TYPE =   0x6000104F;
static const uint32_t P_DD2_TIME =   0x60001050;
static const uint32_t P_BSTR_RGY =   0x60001210; //0x00000410;
static const uint32_t P_MOD_RGY =    0x60001211;
static const uint32_t P_DD1_RGY =    0x60001212;
static const uint32_t P_FX_RGY =     0x60001213; //0x00000411;
static const uint32_t P_REVDD2_RGY = 0x60001214; //0x00000412;
static const uint32_t P_RGY_EFFECTS = 0x60001201; //15 bytes request


// sysex control define ################################
static const uint32_t PC =        0x00010000; // change channel MS3.write(PC, 1, 2) second byte is channel number 3rd is length
static const uint32_t fx1_sw =    0x60000030; //turn button 1 on  MS3.write(fx1_sw, 1, 1) second byte 0 = off 1 = on
static const uint32_t fx2_sw =    0x6000034C; //turn button 2 on  MS3.write(fx2_sw, 1, 1) second byte 0 = off 1 = on
static const uint32_t fx3_sw =    0x60000610; //turn reverb on  MS3.write(fx3_sw, 1, 1) second byte 0 = off 1 = on
static const uint32_t Loop_sw =   0x60000655;   // turn loop off on
static const uint32_t rvbYellow = 0x60001214; // set reverb type to yellow
static const uint32_t express =   0x6000015D; // expression pedal position ??
static const uint32_t FV_PDL =    0x60000710; // GA-FC volume pedal address MS3.write(FV_PDL, 0-63, 1)
static const uint32_t FV2_PDL =   0x60000633; // GA-FC FX pedal address also for foot pedal connected to amp
static const uint32_t testPdl =   0x00000630; // foot vol??
static const uint32_t wah_pedal = 0x60000369; // expression pedal Wah position
//static const uint32_t rotary_expPedal = 0x6000051A //  rotary expression pedal position
//0x00000428 // acts like GA-FC delay/fx button
// end sysex define ##############################

// define custom lcd character set (gliphs) for inverted R G Y.
static uint8_t custom_O[8] = { 0b11111, 0b11111, 0b11111, 0b11111, 0b00000, 0b11111, 0b11111, 0b11111 };
static uint8_t custom_Y[8] = { 0b11111, 0b10011, 0b10101, 0b10101, 0b10011, 0b10101, 0b10101, 0b11111 };
static uint8_t custom_R[8] = { 0b11111, 0b10101, 0b10101, 0b10101, 0b11011, 0b11011, 0b11011, 0b11111 };
static uint8_t custom_G[8] = { 0b11111, 0b10011, 0b01101, 0b01111, 0b01001, 0b01101, 0b10011, 0b11111 };

// This is the address for the program change.
const uint32_t P_PATCH = 0x00010000;

// We're going to check and report these effect blocks.
const uint32_t CHECK_THIS[] = {
  P_BSTR_SW,
  P_MOD_SW,
  P_FX_SW,
  P_DD1_SW,
  P_SR_SW,
  P_REV_SW,
  P_DD2_SW,
  P_AMP,
  P_BSTR_RGY,
  P_MOD_RGY,
  P_DD1_RGY,
  P_FX_RGY,
  P_REVDD2_RGY,
  P_DD1_TIME,
  P_RGY_EFFECTS
};
const uint8_t CHECK_THIS_SIZE = sizeof(CHECK_THIS) / sizeof(CHECK_THIS[0]);

static const char *amp_list[] {"Nat Cln", "ACOUSTC", "CmCrnch", "StCrnch", "HiGnStk", "Pwr Drv", "ExtLead", "CoreMtl", "CLEAN  ", "ClnTwin",
  "PrCrnch", "CRUNCH ", "DxCrnch", "VoDrive", "VO Lead", "MtchDrv", "BG Lead", "BgDrive", "MS-59 I", "MS59 II",
  "RFr Vnt", "RFrMdrn", "TAmp Ld", "BROWN  ", "LEAD   ", "Custom "
};
static const char *bstr_list[] {"Mid Bst", "Cln Bst", "TrblBst", "Crunch", "Ntrl OD", "Warm OD", "Fat DS", "Lead DS", "Mtl DS", "OCT Fuz",
  "BluesOD", "OD-1", "TubScrm", "TurboOD", "Dist", "Rat", "GuV DS", "DST+", "MtlZone", "60sFuzz", "Muf Fuz", "Custom"
};
static const char *fx_list[] {"T.Wah", "AutoWah", "PdlWah", "AdvComp", "Limitr", "OD/DS", "GrphcEQ", "ParaEQ", "T Modfy", "Gtr Sim", "SlwGear",
  "DeFretr", "WavSyn", "SitrSim", "Octave", "P Shift", "Harmony", "SndHold", "AC.Proc", "Phaser", "Flanger", "Tremolo",
  "Rotary1", "Uni-V", "Panner", "Slicer", "Vibrato", "RingMod", "Humnizr", "2X2 CHR", "SubDly", "AcGtSim", "Rotary2",
  "TeraEco", "OvrTone", "Phser90", "Flgr117", "Wah95E", "DlyCh30"
};
static const char *delay_list[] {"Single", "Pan", "Stereo", "Series", "Paralel", "L/R", "Reverse", "Analog", "Tape", "Mod", "SDE3000"};
static const char *reverb_list[] {"Ambienc", "Room", "Hall 1", "Hall 2", "Plate", "Spring", "Modulte"};

// Some global variables to store effect state and if something changed.
uint32_t states = 0;  // stores true/false bit of the first 8 effects parsed.
uint32_t checked = 0;
bool changed = false;
uint32_t timerStart = 0;
uint32_t tapTimerStart = 0;
uint32_t tempoMillis = 0;
uint32_t tapTimerMode = 0;
uint32_t byte_array[15]; //store parameter byte from each (13) effects when parsed.

Button footSw5(2, 50);       // define the button to pin 2, 50ms debounce. Function switch
Button footSw1(3, 50);       // define the button to pin 3, 50ms debounce. FX1 switch
Button footSw2(4, 50);       // define the button to pin 4, 50ms debounce. FX2 switch
Button footSw3(5, 50);       // define the button to pin 5, 50ms debounce. FX3 switch
Button footSw4(6, 50);       // define the button to pin 6, 50ms debounce. Tap/Loop switch
bool long_press_release = 0;  // so we know a long press occured, and mask out a switch release operation directly after.
uint16_t LONG_PRESS((EEPROM.read(0) + 2) * 100); // we define a "long press" hold time to be stored value+2 x 100 milliseconds.
uint8_t fx1_sel = EEPROM.read(1);  // read the stored setting for FX1 whether Both, Booster only, MOD only.
uint8_t fx2_sel = EEPROM.read(2);  // read the stored setting for FX2 whether Both, Delay1 only, FX only.
uint8_t fx3_sel = EEPROM.read(3);  // read the stored setting for FX3 whether Both, Reverb only, Delay2 only.
uint8_t tapT_sel = EEPROM.read(4); // read the stored setting for TAP delay whether from Patch, Global, or external carried to next patch.
uint8_t tapDD2_sel = EEPROM.read(5); // read the stored setting for Delay2 TAP whether Patch (unaffected), or tempo signature of Delay1 TAP.
uint8_t exp1Connected_sel = EEPROM.read(6); // read the stored setting to see if expression pedal 1 is connected. 1 = yes, 2 = no
uint8_t exp2Connected_sel = EEPROM.read(7); // read the stored setting to see if expression pedal 1 is connected. 1 = yes, 2 = no
uint8_t exp1Calibrated_sel = EEPROM.read(8); // read the stored setting to see if expression pedal 1 is connected. 1 = yes, 2 = no
uint16_t exp1Min = EEPROM.read(9); // The minimum value read from expression pedal 1 the last time it was calibrated
uint16_t exp1Max = 0;
// exp1Max is the maximum value read from expression pedal 1 the last time it was calibrated -- EEprom.put is run in setup funct
//EEPROM.get( 10, exp1Max );


//uint32_t count = 0;
const uint8_t led1 = A0;
const uint8_t led2 = A1;
const uint8_t led3 = A2;
const uint8_t led4 = A3;
const uint8_t led5 = A6;
const uint8_t ledArray[] = {led1, led2, led3, led4, led5};
bool fx1State = 1;
bool fx2State = 1;
bool fx3State = 1;
bool modState = 1;
bool fxState = 1;
bool dd2State = 1;
bool loopState = 1;
uint8_t bstr_rgy_state = 0;
uint8_t mod_rgy_state = 0;
uint8_t dd1_rgy_state = 1;
uint8_t fx_rgy_state = 1;
uint8_t revdd2_rgy_state = 2;
/*uint8_t bstr_G_type = 1;
  uint8_t bstr_R_type = 2;
  uint8_t bstr_Y_type = 3;
  uint8_t mod_G_type = 1;
  uint8_t mod_R_type = 5;
  uint8_t mod_Y_type = 8;
  uint8_t fx_G_type = 2;
  uint8_t fx_R_type = 6;
  uint8_t fx_Y_type = 9;
  uint8_t dd1_G_type = 1;
  uint8_t dd1_R_type = 2;
  uint8_t dd1_Y_type = 3;
  uint8_t rev_G_type = 1;
  uint8_t rev_R_type = 2;
  uint8_t rev_Y_type = 3;*/
uint8_t dd2_G_type = 1;
uint8_t dd2_R_type = 2;
uint8_t dd2_Y_type = 3;
uint8_t currentChannel = 1; // active amp channel
String message_1 = "FX1 on ";  // predefined lcd text for test mode, current state is read in from the Katana.
String message_2 = "FX2 on ";
String message_3 = "FX3 on ";
String message_4 = "Loop on";
String message_5 = "";
String message_6 = "AMP Brown";
uint32_t type, MIDIchannel, data1, data2, number, value;
//uint8_t midiByte = 0 ;
const uint32_t msg = 0;
uint8_t chnMode = 0;    // Bank 1, bank 2, or fx button mode
uint32_t previousMillis = 0;         // will store last time LED was updated - used for blinking when select bank 2 is active
const long interval = 300;                // blink time for  select bank 2 is active
uint8_t ledState = LOW;                       // blink time for  select bank 2 is active
uint8_t  set_rgy_select = 0;
uint8_t  rgy_num = 0;
uint16_t tempo = 120;
uint8_t menu = 0;

// variables for expression pedal 1
uint16_t pedalVal1 = 0;           // Expression pedal 1 reading
uint16_t lastPedalVal1 = 0;       // used to see if there is a change in pedal position since last reading
uint8_t pedalOn1 = 0;             // was the exp pedal used?
uint32_t pedalDelay = 1000;       // If epression pedal is toe down and hasn't moved for this time switch effect off
uint8_t pedalOnThreshold = 62;    // An expression pedal value greater than this value for more than the value above will automatically switch FX2 off.
uint32_t lastRead1 = 0;           // last reading of expression pedal 1
uint8_t expressionPedal1Pin = 8;  // TRS tip for expression pedal 1
uint8_t expressionPedal1RingPin = 7;// TRS ring for expression pedal 1


// variables for external MIDI IN clock
// these variables are use when an external MIDI clock
// is sent to the 5 pin DIN MIDI IN. Can be set in tempo
// options in the settings menu.
uint32_t MidiClockTime = 0;
uint32_t LastClkRead = 0;
float extTempo = 0;
uint8_t countClk = 0;
uint32_t deltaT[25];
float AvgClkTime = 0;
uint32_t TotalTime = 0;
uint32_t avgTime = 0;
uint32_t oldTemp = 0;


// Initialize LCD
// Choose or set your display's I2C address. Two common values are below.
// use one of the two unless yours is different.
//#define I2C_ADDR1    0x3F // LCD1 Address [A0=open   A1=open   A2=open  ]
#define I2C_ADDR1    0x27 // LCD1 Address [A0=open   A1=open   A2=open  ] 
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

// This creatce a MIDI instance for the 5 pin MIDI IN/OUT connection.
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

// Loop to turn all LEDs off
void setAllLEDs(uint8_t state) {
  for (uint8_t i = 0; i < TOTAL_LED; i++) {
    digitalWrite(ledArray[i], state);
  }
}

// blink the leds at start up
void blinkAllLeds(uint8_t numTimes, uint16_t inDelay) {
  lcd1.setCursor(0, 1);
  for (uint8_t j = 0; j < numTimes; j++) {
    for (uint8_t i = 0; i < TOTAL_LED; i++) {
      digitalWrite(ledArray[i], HIGH);
      delay(inDelay);
      digitalWrite(ledArray[i], LOW);
      lcd1.print(".");
    }
  }
}


/**
   Parse through incoming data sent from the Katana
*/
void parseData(uint32_t parameter, uint8_t data) {
  switch (parameter) {

    // Refresh all effect states on patch changes.
    case P_PATCH:
      Serial.print(F("Loaded patch "));
      Serial.print(data);
      Serial.println(F("."));
      if (data != 0) {
        currentChannel = data;
      }

      delay(700); // This is necessary to make sure all 14 effect statuses are received on channels 1 - 4.

      if (tapT_sel > 1) { // if TAP delay mode is global, send tempo change.
        setTempo();
      }

      for (uint8_t i = 0;  i < CHECK_THIS_SIZE; i++) {
        uint8_t ds = 0x01;
        if (i == CHECK_THIS_SIZE - 1) {
          ds = 0x0F;
        } MS3.read(CHECK_THIS[i], ds);
      }
      timerStart = millis();
      checked = 0;
      break;

    // Store the effect state for printing later.
    default:
      for (uint8_t i = 0; i < CHECK_THIS_SIZE; i++) {
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
void printStatus(uint32_t duration) {
  uint8_t dataReceived = 0;
  for (uint8_t i = 0; i < CHECK_THIS_SIZE; i++) {
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

  String txt;
  uint32_t a;
  char state[8];
  for (uint8_t i = 0; i < CHECK_THIS_SIZE; i++) {

    strcpy(state, (bitRead(states, i) ? "on " : "off"));

    switch (CHECK_THIS[i]) {
      case P_BSTR_SW:
        //Serial.println(F("BOOSTER:  ") + String(state));
        if (fx1_sel != 3) {
          message_1 = "FX1 " + String(state);
          fx1State = bitRead(states, i);
          digitalWrite(led2, fx1State);
        }
        break;

      case P_MOD_SW:
        //Serial.println(F("MOD: ")+String(state));
        if (fx1_sel == 3) {
          message_1 = "FX1 " + String(state);
          fx1State = bitRead(states, i);
          digitalWrite(led2, fx1State);
        }
        modState = bitRead(states, i);
        break;

      case P_FX_SW:
        //Serial.println(F("FX:  ") + String(state));
        if (fx2_sel == 3) {
          message_2 = "FX2 " + String(state);
          fx2State = bitRead(states, i);
          digitalWrite(led3, fx2State);
        }
        fxState = bitRead(states, i);
        break;

      case P_DD1_SW:
        //Serial.println(F("Delay 1:   ") + String(state));
        if (fx2_sel != 3) {
          message_2 = "FX2 " + String(state);
          fx2State = bitRead(states, i);
          digitalWrite(led3, fx2State);
        }
        break;

      case P_SR_SW:
        //Serial.println(F("SR Loop:   ") + String(state));
        message_4 = "Loop " + String(state);
        loopState = bitRead(states, i);
        digitalWrite(led5, loopState);
        break;

      case P_REV_SW:
        //Serial.println(F("REV:  ") + String(state));
        if (fx3_sel != 3) {
          message_3 = "FX3 " + String(state);
          fx3State = bitRead(states, i);
          digitalWrite(led4, fx3State);
        }
        break;

      case P_DD2_SW:
        //Serial.println(F("Delay 2:   ") + String(state));
        if (fx3_sel == 3) {
          message_3 = "FX3 " + String(state);
          fx3State = bitRead(states, i);
          digitalWrite(led4, fx3State);
        }
        dd2State = bitRead(states, i);
        break;

      case P_AMP:
        a = (byte_array[i]);
        txt = amp_list[a];
        //Serial.println(F("AMP Type: ") + txt);
        message_6 = "AMP " + txt;
        break;

      case P_DD2_TYPE:
        //  dd2_G_type = (byte_array[i]);
        //Serial.println(F("DELAY 2 TYPE:  ") + delay_list[dd2_G_type]);
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
      case P_DD1_TIME:
        //Serial.println(F("DD1 time ") + String(dataOut2)+" " + String(byte_array[i]));
        if (tapT_sel != 2) {
          tempo = 60000 / ((dataOut2 * 128) + (byte_array[i]));
        }
        break;

      case P_RGY_EFFECTS:
        break;

      default:
        Serial.print(CHECK_THIS[i]);
        Serial.print(F(": "));
        Serial.println(state);

    }
  }
  updateLCD1();
  //  //Serial.println();
  //  Serial.print(F("byte_Array:"));
  //  for (uint8_t f = 0; f < 21; f++) {
  //    Serial.print(String(byte_array[f]) + " ");
  //  }
  //  Serial.println();
}

// If USB connection is lost set the katana back in BTS edit mode
void setEdit(void) {
  uint32_t test = 0;
  uint8_t dataTest = 0;
  Serial.println();
  Serial.println(F("Waiting..."));
  Serial.println();
  MS3.begin();
  //katana.update(test, dataTest);
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

// ###########################################
// Begin setup
// ###########################################
/**
   Setup routine.*****************************************************
*/
void setup() {
  Wire.begin();
  Serial.begin(115200);
  //Serial.begin(31250);
  delay(1000);
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.setHandleProgramChange(MIDIinPC);
  MIDI.setHandleControlChange(MIDIinCC);
  MIDI.setHandleClock(MIDIinClock);

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
  Wire.setClock(1000000L);
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

  Serial.println();
#ifdef MS3_OVERRIDE_MODE
  updateLCD1();
#endif

  // Set editor mode - This is necessary because the Katana unsets
  // editor mode if powered off
  uint32_t parameter = 0;
  uint8_t data = 0;
  uint8_t editMode = 0;
  while (editMode == 0) {
    switch (MS3.update(parameter, data)) {
      case MS3_NOT_READY:
        Serial.println(F("Katana OFFLINE !"));
        Serial.println();
        message_1 = "*      KATANA      *";
        message_2 = "*      OFFLINE     *";
        updateLCD1 ();
        delay(100);
        break;
      case MS3_READY:
        MS3.setEditorMode();
        MS3.read(P_PATCH, 0x02);
        lcd1.clear();
        updateLCD1 ();
        editMode = 1;
        Serial.println(F("Ready!"));
        break;
    }
  }
  EEPROM.get( 10, exp1Max ); // The maximum value read from expression pedal 1 the last time it was calibrated
}
// ###########################################
// End setup
// ###########################################
/**
   Main loop.*********************************************************************
*/
void loop() {
  //static uint32_t timerStop = 0;

  // The MS-3 library stores the parameter and data in these variables.
  uint32_t parameter = 0;
  uint8_t data = 0;

  // Check for incoming data or send a queued item.
  switch (MS3.update(parameter, data)) {
    case MS3_NOT_READY:
      Serial.println(F("Katana OFF-LINE !"));
      Serial.println();
      message_1 = "*      KATANA      *";
      //message_2 = ;
      updateLCD1 ();
      setEdit();
      break;

    // Fetch the current active patch on the MS-3.
    case MS3_READY:
      MS3.setEditorMode();
      MS3.read(P_PATCH, 0x02);
      lcd1.clear();
      updateLCD1 ();
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
  uint32_t currentMillis = millis();
  if (chnMode == 2 ) {
    if (currentMillis - previousMillis >= interval) {
      // save the last time you blinked the LED
      previousMillis = currentMillis;
      // if the LED is off turn it on and vice-versa:
      if (ledState == LOW) {
        ledState = HIGH;
      } else {
        ledState = LOW;
      };
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
      };
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
        chnMode = 0;
      }
      else if (chnMode == 2) {
        setAllLEDs(LOW);
        currentChannel = 5;
        MS3.write(PC, 0x0005, 2);
        chnMode = 0;
      }
      else if  (fx1State == 0 ) {
        digitalWrite(led2, HIGH);
        fx1State = 1;
        if (fx1_sel != 3) {
          MS3.write( fx1_sw, 0x01, 1);  // Both or Booster on
        }
        if (fx1_sel != 2) {
          MS3.write(P_MOD_SW, 0x01, 1);  // Both or MOD on
        }
      }
      else if  (fx1State == 1 )  {
        digitalWrite(led2, LOW);
        fx1State = 0;
        if (fx1_sel != 3) {
          MS3.write(fx1_sw, 0x00, 1);  // Both or Booster off
        }
        if (fx1_sel != 2) {
          MS3.write(P_MOD_SW, 0x00, 1);  // Both or MOD off
        }
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
        chnMode = 0;
      }
      else if (chnMode == 2) {
        setAllLEDs(LOW);
        currentChannel = 6;
        MS3.write(PC, 0x0006, 2);
        chnMode = 0;
      }
      else if  (fx2State == 0 ) {
        digitalWrite(led3, HIGH);
        fx2State = 1;
        if (fx2_sel != 2) {
          MS3.write(fx2_sw, 0x01, 1);  // Both or FX on
        }
        if (fx2_sel != 3) {
          MS3.write(P_DD1_SW, 0x01, 1);  // Both or Delay1 on
        }
      }
      else if (fx2State == 1 ) {
        digitalWrite(led3, LOW);
        fx2State = 0;
        if (fx2_sel != 2) {
          MS3.write(fx2_sw, 0x00, 1);  // Both or FX off
        }
        if (fx2_sel != 3) {
          MS3.write(P_DD1_SW, 0x00, 1);  // Both or Delay1 off
        }
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
        chnMode = 0;
      }
      else if (chnMode == 2) {
        setAllLEDs(LOW);
        currentChannel = 7;
        MS3.write(PC, 0x0007, 2);
        chnMode = 0;
      }
      else if  (fx3State == 0 ) {
        digitalWrite(led4, HIGH);
        fx3State = 1;
        if (fx3_sel != 3) {
          MS3.write(fx3_sw, 0x01, 1);  // Both or Reverb on.
        }
        if (fx3_sel != 2) {
          MS3.write(P_DD2_SW, 0x01, 1);  // Both or Delay2 on.
        }
      }
      else if  (fx3State == 1 ) {
        digitalWrite(led4, LOW);
        fx3State = 0;
        if (fx3_sel != 3) {
          MS3.write(fx3_sw, 0x00, 1);  // Both or Reverb off.
        }
        if (fx3_sel != 2) {
          MS3.write(P_DD2_SW, 0x00, 1);  // Both or Delay2 off.
        }
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
          chnMode = 0;
        }
        else if (chnMode == 2) {
          setAllLEDs(LOW);
          currentChannel = 8;
          MS3.write(PC, 0x0008, 2);
          chnMode = 0;
        } else if (chnMode == 0) {

          //***  do tap tempo thing here
          if (tapTimerMode == 0) {
            tapTimerStart = millis();
            tapTimerMode = 1;
          }
          else if (tapTimerMode == 1) {
            tempoMillis = millis() - tapTimerStart;
            tapTimerStart = millis();
            tempo = 60000 / tempoMillis;
            setTempo();
          };
        }
        else {
          long_press_release = 0;
        }
      }
    }
    if ((millis() - tapTimerStart) > 2000) {
      tapTimerMode = 0;  // if the TAP switch is not used for more than 2 seconds, then no more TAP until pressed twice within 2 seconds.
      tapTimerStart = 0;
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

    //***************************************** MENU FUNCTIONS **********************************************
    if (footSw1.isPressed() && footSw5.isPressed()) {
      uint32_t page = 99;
      uint8_t item = 1;
      uint32_t longP_sel = (LONG_PRESS / 100) - 1;
      uint8_t fx1_sel_b4 = 99;  // set previous values to something not current, menu updates if there is change.
      uint8_t fx2_sel_b4 = 99;
      uint8_t fx3_sel_b4 = 99;
      uint32_t tapT_sel_b4 = 99;
      uint32_t tapDD2_sel_b4 = 99;
      uint32_t longP_sel_b4 = 99;
      uint8_t exp1Connected_sel_b4 = 99;
      uint8_t exp2Connected_sel_b4 = 99;
      uint8_t exp1Calibrated_sel_b4 = 99;
      const String fx1_list[3] {"Both", "Booster", "MOD"};
      const String fx2_list[3] {"Both", "Delay1", "FX"};
      const String fx3_list[3] {"Both", "Reverb", "Delay2"};
      String tapT_list[3] {"Patch", "Global", "Extern"};
      const String tapDD2_list[6] {"Patch", "1/16th", "1/8th", "1/4", "1/2", "Whole"};
      const String longP_list[9] {"2", "3", "4", "5", "6", "7", "8", "9", "10" }; // + "00ms"
      const String exp1_list[3] {"yes", "no", "auto"};
      const String exp2_list[2] {"yes", "no"};
      const String exp1Cal_list[2] {"cal now", "no"};

MENU:
      if (item < 1) {
        item = 1; // set the menu scroll boundaries
      } if (item > 9) {
        item = 9;
      }
      if (fx1_sel > 3) {
        fx1_sel = 3;
      } if (fx1_sel < 1) {
        fx1_sel = 1;
      }
      if (fx2_sel > 3) {
        fx2_sel = 3;
      } if (fx2_sel < 1) {
        fx2_sel = 1;
      }
      if (fx3_sel > 3) {
        fx3_sel = 3;
      } if (fx3_sel < 1) {
        fx3_sel = 1;
      }
      if (tapT_sel > 3) {
        tapT_sel = 3;
      } if (tapT_sel < 1) {
        tapT_sel = 1;
      }
      if (tapDD2_sel > 6) {
        tapDD2_sel = 6;
      } if (tapDD2_sel < 1) {
        tapDD2_sel = 1;
      }
      if (longP_sel > 9) {
        longP_sel = 9;
      } if (longP_sel < 1) {
        longP_sel = 1;
      }

      if (exp1Connected_sel > 3) {
        exp1Connected_sel = 3;
      } if (exp1Connected_sel < 1) {
        exp1Connected_sel = 1;
      }
      if (exp2Connected_sel > 2) {
        exp2Connected_sel = 2;
      } if (exp1Connected_sel < 1) {
        exp2Connected_sel = 1;
      }
      if (exp1Calibrated_sel > 2) {
        exp1Calibrated_sel = 2;
      } if (exp1Calibrated_sel < 1) {
        exp1Calibrated_sel = 1;
      }

      if (item < 4) {    // display the first 3 items on page 1
        if (fx1_sel != fx1_sel_b4 || fx2_sel != fx2_sel_b4 || fx3_sel != fx3_sel_b4 || page != 1) {
          lcd1.clear();
          lcd1.setCursor(0, 0);
          lcd1.print("Settings:");
          lcd1.setCursor(0, 1);
          lcd1.print(" FX1 switch: " + fx1_list[fx1_sel - 1]); //Both, Booster, MOD
          lcd1.setCursor(0, 2);
          lcd1.print(" FX2 switch: " + fx2_list[fx2_sel - 1]); //Both, Delay1, FX
          lcd1.setCursor(0, 3);
          lcd1.print(" FX3 switch: " + fx3_list[fx3_sel - 1]); //Both, Reverb, Delay2
          lcd1.blink();
          fx1_sel_b4 = fx1_sel; fx2_sel_b4 = fx2_sel; fx3_sel_b4 = fx3_sel; page = 1;
        }
      }
      if (item > 3) { // display items 4~6 on page 2
        if (tapT_sel != tapT_sel_b4 || tapDD2_sel != tapDD2_sel_b4 || longP_sel != longP_sel_b4 || page != 2) {
          lcd1.clear();
          lcd1.setCursor(0, 0);
          lcd1.print("Settings:");
          lcd1.setCursor(0, 1);
          lcd1.print(" TAP tempo: " + tapT_list[tapT_sel - 1]); //Patch, Global
          lcd1.setCursor(0, 2);
          lcd1.print(" TAP Delay2: " + tapDD2_list[tapDD2_sel - 1]); //None, 1/16th, 1/8th, 1/4, 1/2, Whole
          lcd1.setCursor(0, 3);
          lcd1.print(" Long Press: " + longP_list[longP_sel - 1] + "00ms");
          tapT_sel_b4 = tapT_sel; tapDD2_sel_b4 = tapDD2_sel; longP_sel_b4 = longP_sel; page = 2;
        }
      }

      // Epression pedal settings
      if (item > 6) { // display items 7~9 on page 3
        //exp1Connected_sel = 1 // Expression pedal 1 is connected
        //exp1Connected_sel = 2 // Expression pedal 1 is not connected
        //exp1Connected_sel = 3 // auto detect expression pedal 1
        if (exp1Connected_sel != exp1Connected_sel_b4 || exp2Connected_sel != exp2Connected_sel_b4 || exp1Calibrated_sel != exp1Calibrated_sel_b4 || page != 3) {
          lcd1.clear();
          lcd1.setCursor(0, 0);
          lcd1.print("Settings:");
          lcd1.setCursor(0, 1);
          lcd1.print(" Exp 1: " + exp1_list[exp1Connected_sel - 1]); // expression 1 connected
          lcd1.setCursor(0, 2);
          lcd1.print(" Exp 2: " + exp2_list[exp2Connected_sel - 1]); // expression 1 connected
          lcd1.setCursor(0, 3);
          lcd1.print(" Calibrate: " + exp1Cal_list[exp1Calibrated_sel - 1]);
          exp1Connected_sel_b4 = exp1Connected_sel; exp2Connected_sel_b4 = exp1Connected_sel; exp1Calibrated_sel_b4 = exp1Calibrated_sel; page = 3;
        }
      }



      if (item < 4) {
        lcd1.setCursor(0, item);
      }
      else if (item > 3 && item < 7) {
        lcd1.setCursor(0, item - 3);
      }
      else {
        lcd1.setCursor(0, item - 6);
      };
      while (footSw1.isPressed() || footSw2.isPressed() || footSw3.isPressed() || footSw4.isPressed() || footSw5.isPressed()) {
        read_footSw();
      }
      while (footSw5.isReleased()) {
        delay(100);
        read_footSw();
        if (footSw1.isPressed()) {
          item++;
          goto MENU;
        }
        if (footSw2.isPressed()) {
          item--;
          goto MENU;
        }
        if (footSw3.isPressed()) {
          if (item == 1) {
            fx1_sel--;
          } if (item == 2) {
            fx2_sel--;
          } if (item == 3) {
            fx3_sel--;
          } if (item == 4) {
            tapT_sel--;
          } if (item == 5) {
            tapDD2_sel--;
          } if (item == 6) {
            longP_sel--;
          } if (item == 7) {
            exp1Connected_sel--;
          } if (item == 8) {
            exp2Connected_sel--;
          } if (item == 9) {
            exp1Calibrated_sel--;
          }
          goto MENU;
        }
        if (footSw4.isPressed()) {
          if (item == 1) {
            fx1_sel++;
          } if (item == 2) {
            fx2_sel++;
          } if (item == 3) {
            fx3_sel++;
          } if (item == 4) {
            tapT_sel++;
          } if (item == 5) {
            tapDD2_sel++;
          } if (item == 6) {
            longP_sel++;
          } if (item == 7) {
            exp1Connected_sel++;
          } if (item == 8) {
            exp2Connected_sel++;
          } if (item == 9) {
            exp1Calibrated_sel++;
          }
          goto MENU;
        }
      }

      while (footSw5.isPressed()) {
        footSw5.read();
      }
      if (footSw5.isReleased()) {
        footSw5.read(); lcd1.noBlink(); lcd1.clear(); updateLCD1(); LONG_PRESS = (longP_sel + 1) * 100; EEPROM.update(0, longP_sel - 1); EEPROM.update(1, fx1_sel); EEPROM.update(2, fx2_sel);
        EEPROM.update(3, fx3_sel); EEPROM.update(4, tapT_sel); EEPROM.update(5, tapDD2_sel); EEPROM.update(6, exp1Connected_sel); EEPROM.update(7, exp2Connected_sel); EEPROM.update(8, exp1Calibrated_sel);
        if ((exp1Calibrated_sel == 1 && exp1Connected_sel == 1) || (exp1Connected_sel == 3  && analogRead(7) < 1000)) {
          exp1Calibration();
        }
        goto MEXIT;
      }
      goto MENU;
    }
    //*************************************** END OF MENU FUNCTIONS *******************************************************
  }
  else {
    rgy_set();  // rgy_set end.
  }
MEXIT:

  //###########################################################################
  // Check incoming serial MIDI messages
  // If a message is received the setHandle functions
  // created in the setup loop wil be called.
  MIDI.read();

  //handle expression pedal 1 input
  // first condition, if analog (7) < 1000 means an expression pedal isn't plugged in, exp1Connected_sel == 1 means a pedal is plugged in and manually set in the menu
  if ((millis() - lastRead1 > 40 && analogRead(expressionPedal1RingPin) < 1000 ) || (millis() - lastRead1 > 40 && exp1Connected_sel == 1)) { // This delay is necessary to prevent overwhelming the MS3 queue
    expressionPedal1(); //  expression pedal 1 function
  }
}

void updateLCD1 (void) {
  if (message_1 == "*      KATANA      *") { // if offline message, clear display and do something different.
    lcd1.clear();
    lcd1.setCursor(0, 0);
    for (uint8_t x = 0; x < 20; x++) {
      lcd1.print("*");
    };
    lcd1.setCursor(0, 1);
    lcd1.print(message_1);
    lcd1.setCursor(0, 2);
    lcd1.print("*     OFF-LINE     *");
    lcd1.setCursor(0, 3);
    for (uint8_t x = 0; x < 20; x++) {
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
    lcd1.setCursor(7, 1);
    rgy_state(bstr_rgy_state);

    lcd1.setCursor(0, 2);
    lcd1.print(message_2);
    lcd1.setCursor(7, 2);
    rgy_state(fx_rgy_state);

    lcd1.setCursor(0, 3);
    lcd1.print(message_3);
    lcd1.setCursor(7, 3);
    rgy_state(revdd2_rgy_state);

    if (chnMode == 0) {
      lcd1.setCursor(9, 0);
      lcd1.print("    FX mode");
    }

    lcd1.setCursor(8, 1);
    lcd1.print("    " + message_4);

    lcd1.setCursor(8, 2);
    lcd1.print("  TAP    bpm");
    lcd1.setCursor(14, 2);
    lcd1.print(String(tempo)); // message_5

    lcd1.setCursor(8, 3);
    lcd1.print(" " + message_6);
  }
}

void rgy_state(uint8_t a) {
  lcd1.write(byte(a + 1));
}

void rgy_set() {
  if (menu < 1) {
    const String selected("*");
    lcd1.clear();
    lcd1.setCursor(2, 0);  //Display GRY effect selection page depending on which FX button was long pressed - "*" indicates the effect which will be switched on/off.
    if (rgy_num == 1) {
      lcd1.print("BOOSTER");
      lcd1.setCursor(12, 0);
      lcd1.print("MOD");
      if (fx1_sel != 3) {
        lcd1.setCursor(0, 0);
        lcd1.print(selected);
      } if (fx1_sel != 2) {
        lcd1.setCursor(10, 0);
        lcd1.print(selected);
      }
    }
    if (rgy_num == 2) {
      lcd1.print("DELAY 1");
      lcd1.setCursor(12, 0);
      lcd1.print("FX");
      if (fx2_sel != 3) {
        lcd1.setCursor(0, 0);
        lcd1.print(selected);
      } if (fx2_sel != 2) {
        lcd1.setCursor(10, 0);
        lcd1.print(selected);
      }
    }
    if (rgy_num == 3) {
      lcd1.print("REVERB");
      lcd1.setCursor(12, 0);
      lcd1.print("DELAY 2");
      if (fx3_sel != 3) {
        lcd1.setCursor(0, 0);
        lcd1.print(selected);
      } if (fx2_sel != 2) {
        lcd1.setCursor(10, 0);
        lcd1.print(selected);
      }
    }

    lcd1.setCursor(0, 1);
    lcd1.print("G:");   // lcd print the GREEN effects leftside and rightside. Type depending on FX1.2.3 switch long press selected.
    if (rgy_num == 1) {
      lcd1.print(String(bstr_list[bstr_G_type]));
      lcd1.setCursor(10, 1);
      lcd1.print("G:" + String(fx_list[mod_G_type]));
    }
    if (rgy_num == 2) {
      lcd1.print(String(delay_list[dd1_G_type]));
      lcd1.setCursor(10, 1);
      lcd1.print("G:" + String(fx_list[fx_G_type]));
    }
    if (rgy_num == 3) {
      lcd1.print(String(reverb_list[rev_G_type]));
      lcd1.setCursor(10, 1);
      lcd1.print("G:" + String(delay_list[dd2_G_type]));
    }

    lcd1.setCursor(0, 2);
    lcd1.print("R:");   // lcd print the RED effects leftside and rightside. Type depending on FX1.2.3 switch long press selected.
    if (rgy_num == 1) {
      lcd1.print(String(bstr_list[bstr_R_type]));
      lcd1.setCursor(10, 2);
      lcd1.print("R:" + String(fx_list[mod_R_type]));
    }
    if (rgy_num == 2) {
      lcd1.print(String(delay_list[dd1_R_type]));
      lcd1.setCursor(10, 2);
      lcd1.print("R:" + String(fx_list[fx_R_type]));
    }
    if (rgy_num == 3) {
      lcd1.print(String(reverb_list[rev_R_type]));
      lcd1.setCursor(10, 2);
      lcd1.print("R:" + String(delay_list[dd2_R_type]));
    }

    lcd1.setCursor(0, 3);
    lcd1.print("Y:");   // lcd print the YELLOW effects leftside and rightside. Type depending on FX1.2.3 switch long press selected.
    if (rgy_num == 1) {
      lcd1.print(String(bstr_list[bstr_Y_type]));
      lcd1.setCursor(10, 3);
      lcd1.print("Y:" + String(fx_list[mod_Y_type]));
    }
    if (rgy_num == 2) {
      lcd1.print(String(delay_list[dd1_Y_type]));
      lcd1.setCursor(10, 3);
      lcd1.print("Y:" + String(fx_list[fx_Y_type]));
    }
    if (rgy_num == 3) {
      lcd1.print(String(reverb_list[rev_Y_type]));
      lcd1.setCursor(10, 3);
      lcd1.print("Y:" + String(delay_list[dd2_Y_type]));
    }
    menu = 1; // set so the lcd is not continuously refreshing while nothing happening.
  }                //set the RGY icon to the currently active effect leftside and rightside. If effect is Off, display off icon.
  if (rgy_num == 1) {
    lcd1.setCursor(0, bstr_rgy_state + 1); if (fx1State == 1) {
      lcd1.write(byte(bstr_rgy_state + 1));
    } else {
      lcd1.write(byte(0));
    };
    lcd1.setCursor(10, mod_rgy_state + 1); if (modState == 1) {
      lcd1.write(byte(mod_rgy_state + 1));
    } else {
      lcd1.write(byte(0));
    };
  }

  if (rgy_num == 2) {
    lcd1.setCursor(0, dd1_rgy_state + 1); if (fx2State == 1) {
      lcd1.write(byte(dd1_rgy_state + 1));
    } else {
      lcd1.write(byte(0));
    };
    lcd1.setCursor(10, fx_rgy_state + 1); if (fxState == 1) {
      lcd1.write(byte(fx_rgy_state + 1));
    } else {
      lcd1.write(byte(0));
    };
  }

  if (rgy_num == 3) {
    lcd1.setCursor(0, revdd2_rgy_state + 1); if (dd2State == 1) {
      lcd1.write(byte(revdd2_rgy_state + 1));
    } else {
      lcd1.write(byte(0));
    };
    lcd1.setCursor(10, revdd2_rgy_state + 1); if (fx3State == 1) {
      lcd1.write(byte(revdd2_rgy_state + 1));
    } else {
      lcd1.write(byte(0));
    };
  }



loop1:
  read_footSw();
  if ((footSw1.isPressed() || footSw2.isPressed() || footSw3.isPressed()) && long_press_release == 1 ) {
    goto loop1;  // wait here until button is released
  }

  read_footSw();

  if (footSw1.isPressed()) {
    long_press_release = 0;
    set_rgy_select = 0;
    //Serial.println(F("GREEN selected for FX")+String(rgy_num));
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
    //Serial.println(F("RED selected for FX")+String(rgy_num));
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
    //Serial.println(F("YELLOW selected for FX")+String(rgy_num));
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
  delay(700);
  rgy_num = 0;
  menu = 0;
  lcd1.clear();
  updateLCD1();
}

void read_footSw() {
  footSw1.read();
  footSw2.read();
  footSw3.read();
  footSw4.read();
  footSw5.read();
}

void setTempo(void) {
  if (tempo > 400) {
    tempo = 400;
  }
  if (tempo < 30) {
    tempo = 30;
  }
  uint8_t dd_time_msb = ((60000 / tempo) / 128);
  uint8_t dd_time_lsb = (60000 / tempo) - (dd_time_msb * 128);
  Serial.println(F("dd1_time = ") + String((dd_time_msb * 128) + dd_time_lsb));
  uint8_t data[2];
  data[0] = {dd_time_msb};
  data[1] = {dd_time_lsb};
  MS3.send(P_DD1_TIME, data, 2, MS3_WRITE);  // bypassing the MS3 Queue, but only way to send 2 byte data
  if (tapDD2_sel > 1) {
    uint32_t dd2_tempo = tempo;
    if (tapDD2_sel == 2) {
      dd2_tempo = tempo * 4; // fancy time signatures
    }
    if (tapDD2_sel == 3) {
      dd2_tempo = tempo * 2;
    }
    if (tapDD2_sel == 5) {
      dd2_tempo = tempo / 2;
    }
    if (tapDD2_sel == 6) {
      dd2_tempo = tempo / 4;
    }
    if (dd2_tempo < 15) {
      dd2_tempo = dd2_tempo * 4; // keep the tempo within bounds of Delay 2 range.
    }
    if (dd2_tempo < 30) {
      dd2_tempo = dd2_tempo * 2;
    }
    dd_time_msb = ((60000 / dd2_tempo) / 128);
    dd_time_lsb = (60000 / dd2_tempo) - (dd_time_msb * 128);
    Serial.println(F("dd2_time = ") + String((dd_time_msb * 128) + dd_time_lsb));
    //data[2];
    data[0] = {dd_time_msb};
    data[1] = {dd_time_lsb};
    MS3.send(P_DD2_TIME, data, 2, MS3_WRITE);
  }
  lcd1.setCursor(8, 2);
  lcd1.print("  TAP    bpm");
  lcd1.setCursor(14, 2);
  lcd1.print(String(tempo)); // message_5
  lcd1.print("bpm");
}
