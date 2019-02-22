
/**
 * Date: 20190222
 *
 * Notes: include Wire.h for Teensy LC or 
 * i2c_t3.h for Teensy 3.1/3.2
 *
 * Adjust lines 194 or 195 for LCD I2C address
 * 
 */

// Uncomment this to enable verbose debug messages.
//#define MS3_DEBUG_MODE
//uncomment below to debug withot the Katana connected.
//#define MS3_OVERRIDE_MODE

#include "Arduino.h"
#include <avr/pgmspace.h>
#include "MS3.h"
//#include <Wire.h>       //use this for Teensy LC
#include <i2c_t3.h>   // use this for Teensy 3.1/3.2
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <JC_Button.h> 
#include <MIDI.h>

// Initialize the MS3 class.
MS3 MS3;

// These are addresses for the effect states.
static const unsigned long P_BSTR_SW =    0x60000030;
static const unsigned long P_BSTR_TYPE =  0x60000031;
static const unsigned long P_AMP =        0x60000051;
static const unsigned long P_MOD_SW =     0x60000140;
static const unsigned long P_FX_SW =      0x6000034C;
static const unsigned long P_DD1_SW =     0x60000560;
static const unsigned long P_DD1_TIME =   0x60000562;
static const unsigned long P_REV_SW =     0x60000610;
static const unsigned long P_SR_SW =      0x60000655;
static const unsigned long P_DD2_SW =     0x6000104E;
static const unsigned long P_DD2_TYPE =   0x6000104F;
static const unsigned long P_DD2_TIME =   0x60001050;
static const unsigned long P_BSTR_RGY =   0x60001210; //0x00000410;
static const unsigned long P_MOD_RGY =    0x60001211;
static const unsigned long P_DD1_RGY =    0x60001212;
static const unsigned long P_FX_RGY =     0x60001213; //0x00000411;
static const unsigned long P_REVDD2_RGY = 0x60001214; //0x00000412;
static const unsigned long P_RGY_EFFECTS= 0x60001201; //15 bytes request


// sysex control define ################################
static const unsigned long PC =        0x00010000; // change channel katana.write(PC, 1, 2) second byte is channel number 3rd is length
static const unsigned long fx1_sw =    0x60000030; //turn button 1 on  katana.write(fx1_sw, 1, 1) second byte 0 = off 1 = on
static const unsigned long fx2_sw =    0x6000034C; //turn button 2 on  katana.write(fx2_sw, 1, 1) second byte 0 = off 1 = on
static const unsigned long fx3_sw =    0x60000610; //turn reverb on  katana.write(fx3_sw, 1, 1) second byte 0 = off 1 = on
static const unsigned long Loop_sw =   0x60000655;   // turn loop off on
static const unsigned long rvbYellow = 0x60001214; // set reverb type to yellow
static const unsigned long express =   0x6000015D; // expression pedal position ??
static const unsigned long FV_PDL =    0x60000633; // volume pedal address
static const unsigned long wah_pedal = 0x60000369; // expression pedal Wah position
// end sysex define ##############################

// define custom lcd character set (gliphs) for inverted R G Y.
static byte custom_O[8] = { 0b11111, 0b11111, 0b11111, 0b11111, 0b00000, 0b11111, 0b11111, 0b11111 };
static byte custom_Y[8] = { 0b11111, 0b10011, 0b10101, 0b10101, 0b10011, 0b10101, 0b10101, 0b11111 };
static byte custom_R[8] = { 0b11111, 0b10101, 0b10101, 0b10101, 0b11011, 0b11011, 0b11011, 0b11111 };
static byte custom_G[8] = { 0b11111, 0b10011, 0b01101, 0b01111, 0b01001, 0b01101, 0b10011, 0b11111 };

// This is the address for the program change.
const unsigned long P_PATCH = 0x00010000;

// We're going to check and report these effect blocks.
const unsigned long CHECK_THIS[] = {
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
const byte CHECK_THIS_SIZE = sizeof(CHECK_THIS) / sizeof(CHECK_THIS[0]);

static const char *amp_list[]{"Nat Cln", "ACOUSTC", "CmCrnch", "StCrnch", "HiGnStk", "Pwr Drv", "ExtLead", "CoreMtl", "CLEAN  ", "ClnTwin", 
                                      "PrCrnch", "CRUNCH ", "DxCrnch", "VoDrive", "VO Lead", "MtchDrv", "BG Lead", "BgDrive", "MS-59 I", "MS59 II",
                                       "RFr Vnt", "RFrMdrn", "TAmp Ld", "BROWN  ", "LEAD   ", "Custom "}; 
static const char *bstr_list[]{"Mid Bst", "Cln Bst", "TrblBst", "Crunch", "Ntrl OD", "Warm OD", "Fat DS", "Lead DS", "Mtl DS", "OCT Fuz",
                                       "BluesOD", "OD-1", "TubScrm", "TurboOD", "Dist", "Rat", "GuV DS", "DST+", "MtlZone", "60sFuzz", "Muf Fuz", "Custom"};
static const char *fx_list[]{"T.Wah", "AutoWah", "PdlWah", "AdvComp", "Limitr", "OD/DS", "GrphcEQ", "ParaEQ", "T Modfy", "Gtr Sim", "SlwGear",
                                     "DeFretr", "WavSyn", "SitrSim", "Octave", "P Shift", "Harmony", "SndHold", "AC.Proc", "Phaser", "Flanger", "Tremolo",
                                      "Rotary1", "Uni-V", "Panner", "Slicer", "Vibrato", "RingMod", "Humnizr", "2X2 CHR", "SubDly", "AcGtSim", "Rotary2",
                                       "TeraEco", "OvrTone", "Phser90", "Flgr117", "Wah95E", "DlyCh30"}; 
static const char *delay_list[]{"Single", "Pan", "Stereo", "Series", "Paralel", "L/R", "Reverse", "Analog", "Tape", "Mod", "SDE3000"};
static const char *reverb_list[]{"Ambienc", "Room", "Hall 1", "Hall 2", "Plate", "Spring", "Modulte"};
                 
// Some global variables to store effect state and if something changed.
unsigned int states = 0;  // stores true/false bit of the first 8 effects parsed.
unsigned int checked = 0;
bool changed = false;
unsigned long timerStart = 0;
unsigned long tapTimerStart = 0;
unsigned long tempoMillis = 0;
int tapTimerMode = 0;
unsigned int byte_array[15]; //store parameter byte from each (13) effects when parsed.

Button footSw5(2, 50);       // define the button to pin 2, 50ms debounce. Function switch
Button footSw1(3, 50);       // define the button to pin 3, 50ms debounce. FX1 switch
Button footSw2(4, 50);       // define the button to pin 4, 50ms debounce. FX2 switch
Button footSw3(5, 50);       // define the button to pin 5, 50ms debounce. FX3 switch
Button footSw4(6, 50);       // define the button to pin 6, 50ms debounce. Tap/Loop switch
bool long_press_release = 0;  // so we know a long press occured, and mask out a switch release operation directly after.
unsigned long LONG_PRESS((EEPROM.read(0)+2)*100);     // we define a "long press" hold time to be stored value+2 x 100 milliseconds.
unsigned int fx1_sel = EEPROM.read(1);  // read the stored setting for FX1 whether Both, Booster only, MOD only.
unsigned int fx2_sel = EEPROM.read(2);  // read the stored setting for FX2 whether Both, Delay1 only, FX only.
unsigned int fx3_sel = EEPROM.read(3);  // read the stored setting for FX3 whether Both, Reverb only, Delay2 only.
unsigned int tapT_sel = EEPROM.read(4); // read the stored setting for TAP delay whether from Patch, or Global carried to next patch.
unsigned int tapDD2_sel = EEPROM.read(5); // read the stored setting for Delay2 TAP whether Patch (unaffected), or tempo signature of Delay1 TAP.

#define TOTAL_LED 5

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

int count = 0;
const int led1 = A0;
const int led2 = A1;
const int led3 = A2;
const int led4 = A3;
const int led5 = A6;
const byte ledArray[] = {led1, led2, led3, led4, led5};
bool fx1State = 1;
bool fx2State = 1;
bool fx3State = 1;
bool modState = 1;
bool fxState = 1;
bool dd2State = 1;
bool loopState = 1;
int bstr_rgy_state = 0;
int mod_rgy_state = 0;
int dd1_rgy_state = 1;
int fx_rgy_state = 1;
int revdd2_rgy_state = 2;
/*int bstr_G_type = 1;
int bstr_R_type = 2;
int bstr_Y_type = 3;
int mod_G_type = 1;
int mod_R_type = 5;
int mod_Y_type = 8;
int fx_G_type = 2;
int fx_R_type = 6;
int fx_Y_type = 9;
int dd1_G_type = 1;
int dd1_R_type = 2;
int dd1_Y_type = 3;
int rev_G_type = 1;
int rev_R_type = 2;
int rev_Y_type = 3;*/
int dd2_G_type = 1;
int dd2_R_type = 2;
int dd2_Y_type = 3;
int currentChannel = 1;  
String message_1 = "FX1 on ";  // predefined lcd text for test mode, current state is read in from the Katana.
String message_2 = "FX2 on ";  
String message_3 = "FX3 on ";  
String message_4 = "Loop on";  
String message_5 = "";  
String message_6 = "AMP Brown";  
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
int menu = 0;
int pedalVal = 0;
int lastPedalVal = 0; // used to see if there is a change in pedal position since last reading
int pedalOn = 0; // was the exp pedal used?
unsigned long pedalDelay = 1000; // If epression pedal is toe down and hasn't moved for this time switch effect off
unsigned long lastRead = 0;

// Initialize LCD
//#define I2C_ADDR1    0x3F // LCD1 Address [A0=open   A1=open   A2=open  ]   //use either one of these
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
 * Incoming data handler.
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
      }
        
      delay(700); // This is necessary to make sure all 14 effect statuses are received on channels 1 - 4.
      
      if(tapT_sel > 1){   // if TAP delay mode is global, send tempo change.
      byte  dd_time_msb = ((60000/tempo)/128);
      byte dd_time_lsb = (60000/tempo) - (dd_time_msb*128);
      Serial.println(F("dd1_time = ")+ String((dd_time_msb*128) + dd_time_lsb));
      byte data[2];
      data[0] = {dd_time_msb}; 
      data[1] = {dd_time_lsb};           
      MS3.send(P_DD1_TIME, data, 2, MS3_WRITE);  // bypassing the MS3 Queue, but only way to send 2 byte data
      if(tapDD2_sel > 1){   // if Delay 2 Tap is enabled 
      unsigned long dd2_tempo = tempo; 
      if(tapDD2_sel == 2){dd2_tempo = tempo*4;} // fancy time signatures
      if(tapDD2_sel == 3){dd2_tempo = tempo*2;} 
      if(tapDD2_sel == 5){dd2_tempo = tempo/2;} 
      if(tapDD2_sel == 6){dd2_tempo = tempo/4;}
      if(dd2_tempo < 15){dd2_tempo = dd2_tempo*4;}  // keep the tempo within bounds of Delay 2 range.
      if(dd2_tempo < 30){dd2_tempo = dd2_tempo*2;} 
      dd_time_msb = ((60000/dd2_tempo)/128);
      dd_time_lsb = (60000/dd2_tempo) - (dd_time_msb*128);
      Serial.println(F("dd2_time = ")+ String((dd_time_msb*128) + dd_time_lsb));
      data[2];
      data[0] = {dd_time_msb}; 
      data[1] = {dd_time_lsb};   
       MS3.send(P_DD2_TIME, data, 2, MS3_WRITE);  
        }
      }
      
      for (byte i = 0;  i < CHECK_THIS_SIZE; i++){byte ds = 0x01; if(i==CHECK_THIS_SIZE-1){ds=0x0F;} MS3.read(CHECK_THIS[i], ds); }
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
 * Print all effect states.
 */
void printStatus(unsigned long duration) {
    byte dataReceived = 0;
    for (byte i = 0; i < CHECK_THIS_SIZE; i++) {
        if (bitRead(checked, i)) { dataReceived++; }
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
    int a;
    char state[8];
    for (byte i = 0; i < CHECK_THIS_SIZE; i++) {

            strcpy(state, (bitRead(states, i) ? "on " : "off"));

        switch (CHECK_THIS[i]) {
            case P_BSTR_SW:
                //Serial.println(F("BOOSTER:  ") + String(state));
                if(fx1_sel != 3){
                message_1 = "FX1 "+String(state);
                fx1State = bitRead(states, i);
                digitalWrite(led2, fx1State); }
                break;
                
            case P_MOD_SW:
                //Serial.println(F("MOD: ")+String(state));
                if(fx1_sel == 3){
                message_1 = "FX1 "+String(state);
                fx1State = bitRead(states, i);
                digitalWrite(led2, fx1State); }
                modState = bitRead(states, i);
                break;

            case P_FX_SW:
                //Serial.println(F("FX:  ") + String(state));
                 if(fx2_sel == 3){ 
                message_2 = "FX2 " + String(state);
                fx2State = bitRead(states, i);
                digitalWrite(led3, fx2State); }
                fxState = bitRead(states, i);                
                break;
                
            case P_DD1_SW:
                //Serial.println(F("Delay 1:   ") + String(state));
                if(fx2_sel != 3){
                message_2 = "FX2 "+String(state);
                fx2State = bitRead(states, i);
                digitalWrite(led3, fx2State); }
                break;
                
            case P_SR_SW:
                //Serial.println(F("SR Loop:   ") + String(state));
                message_4 = "Loop "+String(state);
                loopState = bitRead(states, i);
                digitalWrite(led5, loopState);
                break;
                
            case P_REV_SW:
                //Serial.println(F("REV:  ") + String(state));
                if(fx3_sel != 3){
                message_3 = "FX3 " + String(state);
                fx3State = bitRead(states, i);
                digitalWrite(led4, fx3State); }
                break;     
                
            case P_DD2_SW:
                //Serial.println(F("Delay 2:   ") + String(state));
                if(fx3_sel == 3){
                message_3 = "FX3 "+String(state);
                fx3State = bitRead(states, i);
                digitalWrite(led4, fx3State); }
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
                  if(tapT_sel != 2){
                  tempo = 60000/((dataOut2*128)+(byte_array[i])); }
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
    //Serial.println(); 
                 Serial.print(F("byte_Array:"));
                for(int f=0; f<21; f++){Serial.print(String(byte_array[f]) + " ");}
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
/**
 * Setup routine.******************************************************
 */
void setup() {
    Wire.begin();
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

    Serial.println(F("Ready!"));
    Serial.println();
#ifdef MS3_OVERRIDE_MODE
    updateLCD1();
#endif
}

/**
 * Main loop.**********************************************************************
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
  unsigned long currentMillis = millis();
  if (chnMode == 2 ) {
    if (currentMillis - previousMillis >= interval) {
      // save the last time you blinked the LED
      previousMillis = currentMillis;
      // if the LED is off turn it on and vice-versa:
      if (ledState == LOW){ ledState = HIGH;} else { ledState = LOW; };
      // set the LED with the ledState of the variable:
      digitalWrite(led1, ledState);
    }
  }
  // End blink LED1 if chnMode == 2)


    // Blink LED1,2,3 if in rgy select mode)
  if (set_rgy_select>0) {
    if (currentMillis - previousMillis >= interval) {
      // save the last time you blinked the LED
      previousMillis = currentMillis;
      // if the LED is off turn it on and vice-versa:
      if (ledState == LOW){ ledState = HIGH; } else { ledState = LOW; };
      // set the LED with the ledState of the variable:
      digitalWrite(led2, ledState);
      digitalWrite(led3, ledState);
      digitalWrite(led4, ledState);
    }
  }
  // End blink LED1,2,3 if in rgy select mode)

  
//************************************************** FOOT SWITCH ROUTINES *********************
read_footSw();
if(set_rgy_select == 0){


  
//****************************** FX1 FOOTSWITCH ********************************

  if(footSw1.pressedFor(LONG_PRESS) && long_press_release<1){
      setAllLEDs(LOW);
      set_rgy_select = 1;
      long_press_release=1;
      rgy_num = 1;
      rgy_set(); 
  }
  if(footSw1.wasReleased()){
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
       if(fx1_sel != 3){ MS3.write( fx1_sw, 0x01, 1); } // Both or Booster on
       if(fx1_sel != 2){ MS3.write(P_MOD_SW, 0x01, 1); }   // Both or MOD on
    }
    else if  (fx1State == 1 )  {
      digitalWrite(led2, LOW);
      fx1State = 0;
      if(fx1_sel != 3){ MS3.write(fx1_sw, 0x00, 1); }  // Both or Booster off
      if(fx1_sel != 2){ MS3.write(P_MOD_SW, 0x00, 1); }   // Both or MOD off
    }
   }else{long_press_release = 0;
  }


//****************************************FX2 FOOTSWITCH *********************************

  if(footSw2.pressedFor(LONG_PRESS) && long_press_release<1){
    setAllLEDs(LOW);
    set_rgy_select = 1;
    long_press_release = 1;
    rgy_num = 2;
    rgy_set(); 
  }
  if(footSw2.wasReleased()){
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
      if(fx2_sel != 2){ MS3.write(fx2_sw, 0x01, 1); }  // Both or FX on
      if(fx2_sel != 3){ MS3.write(P_DD1_SW, 0x01, 1); }   // Both or Delay1 on
    }
    else if (fx2State == 1 ){
      digitalWrite(led3, LOW);
      fx2State = 0;
      if(fx2_sel != 2){ MS3.write(fx2_sw, 0x00, 1); }   // Both or FX off
      if(fx2_sel != 3){ MS3.write(P_DD1_SW, 0x00, 1); }    // Both or Delay1 off
    }
   }else{long_press_release = 0;
  }


//****************************************FX3 FOOTSWITCH *********************************
  if(footSw3.pressedFor(LONG_PRESS) && long_press_release<1){
    setAllLEDs(LOW);
    set_rgy_select = 1;
    long_press_release = 1;
    rgy_num = 3;
    rgy_set(); 
  }
  if(footSw3.wasReleased()){
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
      if(fx3_sel != 3){ MS3.write(fx3_sw, 0x01, 1); }    // Both or Reverb on.
      if(fx3_sel != 2){ MS3.write(P_DD2_SW, 0x01, 1); }     // Both or Delay2 on.
    }
    else if  (fx3State == 1 ) {
      digitalWrite(led4, LOW);
      fx3State = 0;
      if(fx3_sel != 3){ MS3.write(fx3_sw, 0x00, 1); }   // Both or Reverb off.
      if(fx3_sel != 2){ MS3.write(P_DD2_SW, 0x00, 1); }    // Both or Delay2 off.
     }
    }else{long_press_release = 0;
   }


//****************************************Tap/Loop FOOTSWITCH *********************************
  if(footSw4.pressedFor(LONG_PRESS) && long_press_release<1){
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
      while(footSw4.isPressed()){footSw4.read();}; // wait until released so not to repeat.
  }
  
  if(footSw4.wasReleased()){
   if(long_press_release<1){
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
         }else if (chnMode == 0){

      //***  do tap tempo thing here
      if(tapTimerMode == 0){tapTimerStart = millis(); tapTimerMode = 1; }
      else if(tapTimerMode == 1){tempoMillis = millis() - tapTimerStart;
      tapTimerStart = millis();
      tempo = 60000/tempoMillis;
      if(tempo>400){tempo = 400; }
      if(tempo<30){tempo = 30;}
      byte  dd_time_msb = ((60000/tempo)/128);
      byte dd_time_lsb = (60000/tempo) - (dd_time_msb*128);
      Serial.println(F("dd1_time = ")+ String((dd_time_msb*128) + dd_time_lsb));
      byte data[2];
      data[0] = {dd_time_msb}; 
      data[1] = {dd_time_lsb};           
      MS3.send(P_DD1_TIME, data, 2, MS3_WRITE);  // bypassing the MS3 Queue, but only way to send 2 byte data
      if(tapDD2_sel > 1){
      unsigned long dd2_tempo = tempo; 
      if(tapDD2_sel == 2){dd2_tempo = tempo*4;} // fancy time signatures
      if(tapDD2_sel == 3){dd2_tempo = tempo*2;} 
      if(tapDD2_sel == 5){dd2_tempo = tempo/2;} 
      if(tapDD2_sel == 6){dd2_tempo = tempo/4;}
      if(dd2_tempo < 15){dd2_tempo = dd2_tempo*4;}  // keep the tempo within bounds of Delay 2 range.
      if(dd2_tempo < 30){dd2_tempo = dd2_tempo*2;} 
      dd_time_msb = ((60000/dd2_tempo)/128);
      dd_time_lsb = (60000/dd2_tempo) - (dd_time_msb*128);
      Serial.println(F("dd2_time = ")+ String((dd_time_msb*128) + dd_time_lsb));
      data[2];
      data[0] = {dd_time_msb}; 
      data[1] = {dd_time_lsb};   
       MS3.send(P_DD2_TIME, data, 2, MS3_WRITE);  
      }
      lcd1.setCursor(8, 2); 
      lcd1.print("  TAP    bpm");
      lcd1.setCursor(14, 2);
      lcd1.print(String(tempo)); // message_5      
      lcd1.print("bpm");
      }; 
    }
         else{long_press_release = 0;
      }
    }
  }
  if((millis() - tapTimerStart) > 2000){tapTimerMode = 0; tapTimerStart = 0; } // if the TAP switch is not used for more than 2 seconds, then no more TAP until pressed twice within 2 seconds.

 // Set switch 5 (Function) to toggle bank 1,2 or fx mode, long press selects Panel channel.************

  if(footSw5.pressedFor(LONG_PRESS)){
      setAllLEDs(LOW);
      currentChannel = 0;
      MS3.write(PC, 0x0000, 2);
      long_press_release = 1;
      while(footSw5.isPressed()){footSw5.read();}; // wait until released.
  }

  if(footSw5.wasReleased()){
    if(long_press_release<1){
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
   }else{long_press_release = 0;
  }
 }
// End set switch 5 (Function) to toggle bank 1,2 or fx mode, long press selects Panel channel.

//***************************************** MENU FUNCTIONS **********************************************
 if(footSw1.isPressed() && footSw5.isPressed()){
  unsigned int page = 99;
  unsigned int item = 1;
  unsigned int longP_sel = (LONG_PRESS/100)-1;
  unsigned int fx1_sel_b4 = 99;  // set previous values to something not current, menu updates if there is change.
  unsigned int fx2_sel_b4 = 99;
  unsigned int fx3_sel_b4 = 99;
  unsigned int tapT_sel_b4 = 99;
  unsigned int tapDD2_sel_b4 = 99;
  unsigned int longP_sel_b4 = 99;
 const String fx1_list[3]{"Both", "Booster", "MOD"}; 
 const String fx2_list[3]{"Both", "Delay1", "FX"}; 
 const String fx3_list[3]{"Both", "Reverb", "Delay2"};
 const String tapT_list[2]{"Patch", "Global"}; 
 const String tapDD2_list[6]{"Patch", "1/16th", "1/8th", "1/4", "1/2", "Whole"}; 
 const String longP_list[9]{"2", "3", "4", "5", "6", "7", "8", "9", "10" };  // + "00ms"
MENU:
    if(item < 1){item = 1;} if(item > 6){item = 6;}             // set the menu scroll boundaries
    if(fx1_sel > 3){fx1_sel = 3;} if(fx1_sel < 1){fx1_sel = 1;}
    if(fx2_sel > 3){fx2_sel = 3;} if(fx2_sel < 1){fx2_sel = 1;}
    if(fx3_sel > 3){fx3_sel = 3;} if(fx3_sel < 1){fx3_sel = 1;}
    if(tapT_sel > 2){tapT_sel = 2;} if(tapT_sel < 1){tapT_sel = 1;}
    if(tapDD2_sel > 6){tapDD2_sel = 6;} if(tapDD2_sel < 1){tapDD2_sel = 1;}
    if(longP_sel > 9){longP_sel = 9;} if(longP_sel < 1){longP_sel = 1;}
    if(item < 4){      // display the first 3 items on page 1
    if(fx1_sel != fx1_sel_b4 || fx2_sel != fx2_sel_b4 || fx3_sel != fx3_sel_b4 || page != 1){
    lcd1.clear();  
    lcd1.setCursor(0, 0);
    lcd1.print("Settings:");
    lcd1.setCursor(0, 1);
    lcd1.print(" FX1 switch: " + fx1_list[fx1_sel-1]);  //Both, Booster, MOD
    lcd1.setCursor(0, 2);
    lcd1.print(" FX2 switch: " + fx2_list[fx2_sel-1]);  //Both, Delay1, FX
    lcd1.setCursor(0, 3);
    lcd1.print(" FX3 switch: " + fx3_list[fx3_sel-1]);  //Both, Reverb, Delay2     
    lcd1.blink();
    fx1_sel_b4 = fx1_sel; fx2_sel_b4 = fx2_sel; fx3_sel_b4 = fx3_sel; page = 1; 
      }
    }
    if(item > 3){   // display items 4~6 on page 2
    if(tapT_sel != tapT_sel_b4 || tapDD2_sel != tapDD2_sel_b4 || longP_sel != longP_sel_b4 || page != 2){
    lcd1.clear();  
    lcd1.setCursor(0, 0);
    lcd1.print("Settings:");
    lcd1.setCursor(0, 1);
    lcd1.print(" TAP tempo: " + tapT_list[tapT_sel-1]);   //Patch, Global
    lcd1.setCursor(0, 2);
    lcd1.print(" TAP Delay2: " + tapDD2_list[tapDD2_sel-1]); //None, 1/16th, 1/8th, 1/4, 1/2, Whole
    lcd1.setCursor(0, 3);
    lcd1.print(" Long Press: " + longP_list[longP_sel-1] + "00ms");
    tapT_sel_b4 = tapT_sel; tapDD2_sel_b4 = tapDD2_sel; longP_sel_b4 = longP_sel; page = 2;
        }
     }
    if(item<4){lcd1.setCursor(0, item);}else{lcd1.setCursor(0, item-3);};
    while(footSw1.isPressed() || footSw2.isPressed() || footSw3.isPressed() || footSw4.isPressed() || footSw5.isPressed()){read_footSw();}
    while(footSw5.isReleased()){
      delay(100);
      read_footSw(); 
      if(footSw1.isPressed()){item++; goto MENU;}  
      if(footSw2.isPressed()){item--; goto MENU;}
      if(footSw3.isPressed()){if(item == 1){fx1_sel--;} if(item == 2){fx2_sel--;} if(item == 3){fx3_sel--;} if(item == 4){tapT_sel--;} if(item == 5){tapDD2_sel--;} if(item == 6){longP_sel--;} goto MENU;   }
      if(footSw4.isPressed()){if(item == 1){fx1_sel++;} if(item == 2){fx2_sel++;} if(item == 3){fx3_sel++;} if(item == 4){tapT_sel++;} if(item == 5){tapDD2_sel++;} if(item == 6){longP_sel++;} goto MENU;   }
    
    
    }

    while(footSw5.isPressed()){footSw5.read(); }
    if(footSw5.isReleased()){ footSw5.read(); lcd1.noBlink(); lcd1.clear(); updateLCD1(); LONG_PRESS = (longP_sel+1)*100; EEPROM.write(0, longP_sel-1); EEPROM.write(1, fx1_sel); EEPROM.write(2, fx2_sel);
                              EEPROM.write(3, fx3_sel); EEPROM.write(4, tapT_sel); EEPROM.write(5, tapDD2_sel); goto MEXIT;}
    goto MENU;   
 }
 //*************************************** END OF MENU FUNCTIONS ******************************************************* 
}
else{rgy_set(); }   // rgy_set end.
MEXIT:

  // Check incoming serial MIDI and translate CC and PC mesages to Katana sysex
  if (MIDI.read()) {                // Is there a MIDI message incoming ?
    handleExtMIDI();        // call function to deal with MIDI IN messages
  }

   //expression pedal
/*  if (millis() - lastRead > pedalDelay && pedalVal >= 63 && pedalOn == 1) { // pedal value should be calibrated for pedals that don't read all the way to 63
    MS3.write(fx2_sw, 0x00, 1);   // <---- Act like fx2_sw - code set with menu option later
    lastRead = millis();
    pedalOn = 0;
    Serial.println("Pedal Off");
  }
  if (millis() - lastRead > 35) {   // This delay is necessary not to overwhelm the MS3 queue
    pedalVal = analogRead(7) / 16;  // Divide by 16 to get range of 0-63 for midi
    if (pedalVal != lastPedalVal) { // If the value does not = the last value the pedal has moved.

      if (pedalOn == 0) {
        MS3.write(fx2_sw, 0x01, 1); // <---- Act like fx2_sw - code set with menu option later
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
*/
}


void updateLCD1 (void) {
  if(message_1 == "*      KATANA      *"){ // if offline message, clear display and do something different.
  lcd1.clear();  
  lcd1.setCursor(0, 0);
  for(int x=0; x<20; x++) {lcd1.print("*");};
  lcd1.setCursor(0, 1);
  lcd1.print(message_1);
  lcd1.setCursor(0, 2);
  lcd1.print("*     OFF-LINE     *");
  lcd1.setCursor(0, 3);
  for(int x=0; x<20; x++) {lcd1.print("*");};
  delay(200);
  lcd1.clear();
  }else
{
  lcd1.setCursor(0, 0);
  if(currentChannel<1){
  lcd1.print("Panel    ");
  }else{
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

void rgy_state(int a){
 lcd1.write(byte(a+1));
}

void rgy_set(){
  if(menu <1){
    const String selected("*");
    lcd1.clear();
    lcd1.setCursor(2, 0);  //Display GRY effect selection page depending on which FX button was long pressed - "*" indicates the effect which will be switched on/off.
    if(rgy_num == 1){lcd1.print("BOOSTER"); lcd1.setCursor(12, 0); lcd1.print("MOD"); if(fx1_sel != 3){lcd1.setCursor(0, 0); lcd1.print(selected);} if(fx1_sel != 2){lcd1.setCursor(10, 0); lcd1.print(selected);}}
    if(rgy_num == 2){lcd1.print("DELAY 1"); lcd1.setCursor(12, 0); lcd1.print("FX");  if(fx2_sel != 3){lcd1.setCursor(0, 0); lcd1.print(selected);} if(fx2_sel != 2){lcd1.setCursor(10, 0); lcd1.print(selected);}}
    if(rgy_num == 3){lcd1.print("REVERB"); lcd1.setCursor(12, 0); lcd1.print("DELAY 2");  if(fx3_sel != 3){lcd1.setCursor(0, 0); lcd1.print(selected);} if(fx2_sel != 2){lcd1.setCursor(10, 0); lcd1.print(selected);}} 
     
    lcd1.setCursor(0, 1);
    lcd1.print("G:");   // lcd print the GREEN effects leftside and rightside. Type depending on FX1.2.3 switch long press selected.
    if(rgy_num == 1){lcd1.print(String(bstr_list[bstr_G_type])); lcd1.setCursor(10, 1); lcd1.print("G:" + String(fx_list[mod_G_type])); }
    if(rgy_num == 2){lcd1.print(String(delay_list[dd1_G_type])); lcd1.setCursor(10, 1); lcd1.print("G:" + String(fx_list[fx_G_type])); }
    if(rgy_num == 3){lcd1.print(String(reverb_list[rev_G_type])); lcd1.setCursor(10, 1); lcd1.print("G:" + String(delay_list[dd2_G_type])); }
    
    lcd1.setCursor(0, 2);
    lcd1.print("R:");   // lcd print the RED effects leftside and rightside. Type depending on FX1.2.3 switch long press selected.
    if(rgy_num == 1){lcd1.print(String(bstr_list[bstr_R_type])); lcd1.setCursor(10, 2); lcd1.print("R:" + String(fx_list[mod_R_type])); }
    if(rgy_num == 2){lcd1.print(String(delay_list[dd1_R_type])); lcd1.setCursor(10, 2); lcd1.print("R:" + String(fx_list[fx_R_type])); }
    if(rgy_num == 3){lcd1.print(String(reverb_list[rev_R_type])); lcd1.setCursor(10, 2); lcd1.print("R:" + String(delay_list[dd2_R_type])); }
    
    lcd1.setCursor(0, 3);
    lcd1.print("Y:");   // lcd print the YELLOW effects leftside and rightside. Type depending on FX1.2.3 switch long press selected.
    if(rgy_num == 1){lcd1.print(String(bstr_list[bstr_Y_type])); lcd1.setCursor(10, 3); lcd1.print("Y:" + String(fx_list[mod_Y_type])); }
    if(rgy_num == 2){lcd1.print(String(delay_list[dd1_Y_type])); lcd1.setCursor(10, 3); lcd1.print("Y:" + String(fx_list[fx_Y_type])); }
    if(rgy_num == 3){lcd1.print(String(reverb_list[rev_Y_type])); lcd1.setCursor(10, 3); lcd1.print("Y:" + String(delay_list[dd2_Y_type])); }
    menu = 1; // set so the lcd is not continuously refreshing while nothing happening.
  }                //set the RGY icon to the currently active effect leftside and rightside. If effect is Off, display off icon.
    if(rgy_num == 1){lcd1.setCursor(0, bstr_rgy_state+1); if(fx1State==1){lcd1.write(byte(bstr_rgy_state+1));}else{lcd1.write(byte(0));};
                      lcd1.setCursor(10, mod_rgy_state+1); if(modState==1){lcd1.write(byte(mod_rgy_state+1));}else{lcd1.write(byte(0));};}
                      
    if(rgy_num == 2){lcd1.setCursor(0, dd1_rgy_state+1); if(fx2State==1){lcd1.write(byte(dd1_rgy_state+1));}else{lcd1.write(byte(0));};
                      lcd1.setCursor(10, fx_rgy_state+1); if(fxState==1){lcd1.write(byte(fx_rgy_state+1));}else{lcd1.write(byte(0));};}
                      
    if(rgy_num == 3){lcd1.setCursor(0, revdd2_rgy_state+1); if(dd2State==1){lcd1.write(byte(revdd2_rgy_state+1));}else{lcd1.write(byte(0));};
                      lcd1.setCursor(10, revdd2_rgy_state+1); if(fx3State==1){lcd1.write(byte(revdd2_rgy_state+1));}else{lcd1.write(byte(0));};}
        
            
                
loop1:
    read_footSw();
    if((footSw1.isPressed() || footSw2.isPressed() || footSw3.isPressed()) && long_press_release == 1 ){
    goto loop1;  // wait here until button is released
}
 
read_footSw();

  if(footSw1.isPressed()){
    long_press_release = 0;
    set_rgy_select = 0;
    //Serial.println(F("GREEN selected for FX")+String(rgy_num));
    if(rgy_num == 1){ MS3.write(P_BSTR_RGY, 0x00, 1); MS3.write(P_MOD_RGY, 0x00, 1); }
    else if(rgy_num == 2){ MS3.write(P_FX_RGY, 0x00, 1); MS3.write(P_DD1_RGY, 0x00, 1); }
    else if(rgy_num == 3){ MS3.write(P_REVDD2_RGY, 0x00, 1); };
  }
  if(footSw2.isPressed()){
    long_press_release = 0;
    set_rgy_select = 0;
    //Serial.println(F("RED selected for FX")+String(rgy_num));
    if(rgy_num == 1){ MS3.write(P_BSTR_RGY, 0x01, 1); MS3.write(P_MOD_RGY, 0x01, 1); }
    else if(rgy_num == 2){ MS3.write(P_FX_RGY, 0x01, 1); MS3.write(P_DD1_RGY, 0x01, 1); }
    else if(rgy_num == 3){ MS3.write(P_REVDD2_RGY, 0x01, 1); };
  }
  if(footSw3.isPressed()){
    long_press_release = 0;
    set_rgy_select = 0;
    //Serial.println(F("YELLOW selected for FX")+String(rgy_num));
    if(rgy_num == 1){ MS3.write(P_BSTR_RGY, 0x02, 1); MS3.write(P_MOD_RGY, 0x02, 1); }
    else if(rgy_num == 2){ MS3.write(P_FX_RGY, 0x02, 1); MS3.write(P_DD1_RGY, 0x02, 1); }
     else if(rgy_num == 3){ MS3.write(P_REVDD2_RGY, 0x02, 1); };
  }
  if(footSw4.isPressed()){
    long_press_release = 0;
    set_rgy_select = 0;
  }
  if(footSw5.isPressed()){
    long_press_release = 0;
    set_rgy_select = 0;
  }
  
  if(set_rgy_select == 1){ return; }
loop2:
    read_footSw();
    if(footSw1.isPressed() || footSw2.isPressed() || footSw3.isPressed()){
    goto loop2;
    }
     //Serial.println(F("finished RGY loop"));
     rgy_num = 0;
     menu = 0;
     lcd1.clear(); 
     updateLCD1();
}

void read_footSw(){
  footSw1.read();
  footSw2.read();
  footSw3.read();
  footSw4.read();
  footSw5.read();
}

void handleExtMIDI() {          // function to handle external MIDI in messages
MIDIchannel = MIDI.getChannel();
    if (MIDIchannel == 2) {             // Listen to messages only on MIDI channel 2
      switch (MIDI.getType()) {       // Get the type of the message we caught

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
            MS3.write(P_BSTR_SW, value, 1);
          }

          if (number == 22) {
            MS3.write(P_MOD_SW, value, 1); // booster knob MOD
          }

          if (number == 23) {
            MS3.write(P_FX_SW, value, 1); // FX/Delay knob
          }

          if (number == 24) {
            MS3.write(P_SR_SW, value, 1); // Loop
          }

          if (number == 25) {
            MS3.write(P_DD1_SW, value, 1); // FX/Delay knob
          }

          if (number == 26) {
            MS3.write(P_DD2_SW, value, 1);
          }

          if (number == 27) {
            MS3.write(P_REV_SW, value, 1);
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
