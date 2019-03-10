// Put all three files in the same directory before compiling!

// Expression pedal routines


//expression pedal 1 reading function
void expressionPedal1() {
  //Serial.print("analogRead(7): ");
  //Serial.println(analogRead(7)); // test analog 7 pin, if it's < 1000 then no pedal is plugged into jack

  // The lines below can be uncommented and the expression pedal will automatically turn on/off button 2 (FX2).

  pedalVal1 = analogRead(expressionPedal1Pin);
  pedalVal1 = map(pedalVal1, exp1Min, exp1Max, 0, 63);     // apply the calibration to the sensor reading
  pedalVal1 = constrain(pedalVal1, 0, 63);     // in case the pedal value is outside the range seen during calibration

  //
  //  if (millis() - lastRead1 > pedalDelay && pedalVal1 > pedalOnThreshold && pedalOn1 == 1) { // pedal pedalOnThreshold should be calibrated for pedals that don't read all the way to 63
  //    MS3.write(fx2_sw, 0x00, 1);   // <---- Act like fx2_sw - code set with menu option later
  //    lastRead1 = millis();
  //    pedalOn1 = 0;
  //    Serial.println("Pedal Off");
  //  }
  if (abs(pedalVal1 - lastPedalVal1) > 1) { // If the value does not = the last value the pedal has moved.
    //    if (pedalOn1 == 0) {
    //      MS3.write(fx2_sw, 0x01, 1); // <---- Act like fx2_sw - code set with menu option later
    //      Serial.println("Pedal on command sent!");
    //      pedalOn1 = 1;
    //    }
    //    else {
    //MS3.write(FV_PDL, pedalVal1, 1); // volume pedal
    MS3.write(wah_pedal, pedalVal1, 1); // pedal wah position
    lastRead1 = millis();
    lastPedalVal1 = pedalVal1;  // remeber the last value of the pedal position so we can see if it changed later
    Serial.print("expression pedal 1 position: ");
    Serial.println(pedalVal1);
    //}
  }
}

// Calibrate expression pedal 1 and save min/max values to eeprom
void exp1Calibration(void) {
  // Need to create a question; ready to calibrate?
  //  lcd1.clear();
  //  lcd1.setCursor(0, 1);
  //  lcd1.print("Press any button to     start claibration");
  //  read_footSw();
  //  if (footSw1.isPressed() || footSw2.isPressed() || footSw3.isPressed() || footSw4.isPressed() || footSw5.isPressed() ) { // If the value does not = the last value the pedal has moved.
  uint32_t timer = 0;
  exp1Max = 0;    // set intital value low so it will increase
  exp1Min = 1023; // set intital value high so it will decrease
  timer = millis();
  lcd1.clear();
  lcd1.print("Calibrating pedal 1");
  lcd1.setCursor(0, 1);
  lcd1.print("     move pedal     ");
  lcd1.setCursor(0, 2);
  lcd1.print("    heel to toe     ");
  lcd1.setCursor(0, 3);
  lcd1.print("   several times.   ");
  // calibrate for five seconds
  while (millis() - timer < 5000) {
    pedalVal1 = analogRead(8);
    if (pedalVal1 > exp1Max) {  // record the maximum exppression pedal 1 value
      exp1Max = pedalVal1;
    }
    if (pedalVal1 < exp1Min) {  // record the minimum exppression pedal 1 value
      exp1Min = pedalVal1;
    }
  }
  EEPROM.update(9, exp1Min);
  EEPROM.put(10, exp1Max);
  //  Serial.print("exp1Max");
  //  Serial.println(exp1Max);
  //  Serial.print("exp1Min");
  //  Serial.println(exp1Min);
  lcd1.clear();
  lcd1.print("Calibration complete");
  delay(2000);
  updateLCD1();
  //  }
  //  else {
  //    exp1Calibration();
  //    delay(500);
  //  }
}
