// Put all three files in the same directory before compiling!

//Midi serial (5 pin) IN routines.

void MIDIinPC(uint8_t channel, uint8_t number) {
  if (number < 9) {
    Serial.println(String("Prog.Ch") + (" Pgrm # ") + number);
    MS3.write(PC, number, 2);
  }
}

//If a MIDI change control message is received from the Serial1 midi port (5 pin MIDI IN)
// convert it to a System exclusive message.
// Note the CC numbers used below are arbitrary, feel free to change to anything convienient.

void MIDIinCC(uint8_t channel, uint8_t number, uint8_t value) {
  Serial.println(String("Control Ch.") + (", CC#") + number);
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
    MS3.write(P_MOD_SW, value, 1);  // booster knob MOD
  }
  if (number == 23) {
    MS3.write(P_FX_SW, value, 1);  // FX/Delay knob
  }
  if (number == 24) {
    MS3.write(P_SR_SW, value, 1);  // Loop
  }
  if (number == 25) {
    MS3.write(P_DD1_SW, value, 1);  // FX/Delay knob
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
  //if (number == 119) { MS3.write(wah_select, 02, 1); }
  if (number == 120) {
    MS3.write(wah_pedal, value, 2);
  }
}

void MIDIinClock (void) {
  /*
    There is probably a beter way to do this but it's what I came up with. Taking
    the average of 24 ticks yeilds a more stable tempo that taking taking the average of a smaller
    number of clock ticks because the reading time depends on how busy the Teensy is.
    The way this is written tempo seems accurate except at higher BPM.

    1.01818181818 is a correction factor that I determined by divided the tempo reported by the Teensy.
    at 110 BPM on the master clock the Teensy was reading 112. 112/110 = 1.01818181818.
  */
  if (tapT_sel == 3) {
    if (countClk == 23) {
      for (uint8_t k = 1; k < 24; k++) {
        avgTime = (deltaT[k] + deltaT[k - 1]) / 2; // find the time between 2 ticks
        TotalTime = TotalTime + avgTime;
      }
      AvgClkTime = TotalTime / 23;  // find the average difference in time between 24 ticks for better stability
      extTempo = (2500000 / AvgClkTime)  / 1.01818181818; // Tempo (BPM) = (60 sec * 1/24 ticks * 10^6 microseconds)/ time between ticks in micro secs
      Serial.print("extTempo: ");
      Serial.println(extTempo);
      countClk = 0;
      TotalTime = 0;
    }
    MidiClockTime = (micros() - LastClkRead) ;
    LastClkRead = micros();
    deltaT[countClk] = MidiClockTime;
    countClk = countClk + 1;
    if (abs(tempo - round(extTempo)) > 1) { // check to see if tempo has changed
      //if (tempo > round(extTempo) + 1 || tempo < round(extTempo) - 1) {
      tempo = extTempo;
      Serial.print("Tempo: ");
      Serial.println(tempo);
      setTempo();
    }
  }
}
