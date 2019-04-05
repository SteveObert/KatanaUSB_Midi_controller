// Parse incoming data from the Katana amp and print status

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
