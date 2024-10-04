void runStateChangeActions(StateName the_state){
    switch(the_state)
    {
      case IDLE        :
        digitalWrite(MATCH_START_LED, HIGH);
        digitalWrite(MATCH_STOP_LED, LOW);
        digitalWrite(MATCH_RESET_LED, LOW);
        digitalWrite(ASSIST_RESET_LED, LOW);
        digitalWrite(VICTORY_T1_LED, LOW);
        digitalWrite(VICTORY_T2_LED, LOW);
        digitalWrite(TIME_DOWN_LED, HIGH);
        digitalWrite(TIME_UP_LED, HIGH);
        break;
      case READY_CHECK :
        soundboard.playSpecified(READY_SOUND);
        digitalWrite(MATCH_START_LED, LOW);
        digitalWrite(MATCH_STOP_LED, LOW);
        digitalWrite(MATCH_RESET_LED, HIGH);
        digitalWrite(ASSIST_RESET_LED, LOW);
        digitalWrite(VICTORY_T1_LED, LOW);
        digitalWrite(VICTORY_T2_LED, LOW);
        digitalWrite(TIME_DOWN_LED, HIGH);
        digitalWrite(TIME_UP_LED, HIGH);
        break;
      case READY_T1    :
        soundboard.playSpecified(GREEN_TEAM_READY_SOUND);
        firstReady = READY_T1;
        digitalWrite(MATCH_START_LED, LOW);
        digitalWrite(MATCH_STOP_LED, LOW);
        digitalWrite(MATCH_RESET_LED, HIGH);
        digitalWrite(ASSIST_RESET_LED, LOW);
        digitalWrite(VICTORY_T1_LED, LOW);
        digitalWrite(VICTORY_T2_LED, LOW);
        digitalWrite(TIME_DOWN_LED, HIGH);
        digitalWrite(TIME_UP_LED, HIGH);
        break;
      case READY_T2    :
        firstReady = READY_T2;
        soundboard.playSpecified(ORANGE_TEAM_READY_SOUND);
        digitalWrite(MATCH_START_LED, LOW);
        digitalWrite(MATCH_STOP_LED, LOW);
        digitalWrite(MATCH_RESET_LED, HIGH);
        digitalWrite(ASSIST_RESET_LED, LOW);
        digitalWrite(VICTORY_T1_LED, LOW);
        digitalWrite(VICTORY_T2_LED, LOW);
        digitalWrite(TIME_DOWN_LED, HIGH);
        digitalWrite(TIME_UP_LED, HIGH);
        break;
      case READY_BOTH  :
        lastReadyTick = millis();
        if (firstReady == READY_T1) {
          soundboard.playSpecified(ORANGE_TEAM_READY_SOUND);
        } else if (firstReady == READY_T2) {
          soundboard.playSpecified(GREEN_TEAM_READY_SOUND);
        }
        // soundboard.playSpecified(BOTH_TEAM_READY_SOUND);
        digitalWrite(MATCH_START_LED, HIGH);
        digitalWrite(MATCH_STOP_LED, LOW);
        digitalWrite(MATCH_RESET_LED, HIGH);
        digitalWrite(ASSIST_RESET_LED, LOW);
        digitalWrite(VICTORY_T1_LED, LOW);
        digitalWrite(VICTORY_T2_LED, LOW);
        digitalWrite(TIME_DOWN_LED, HIGH);
        digitalWrite(TIME_UP_LED, HIGH);
        flickerLedState = HIGH;
        lastButtonFlickerTick = millis();
        break;
      case COUNTDOWN   :
        soundboard.setVolume(20);
        resetSoundLevel = true;
        soundboard.playSpecified(COUNTDOWN_SOUND);
        digitalWrite(MATCH_START_LED, LOW);
        digitalWrite(MATCH_STOP_LED, LOW);
        digitalWrite(MATCH_RESET_LED, HIGH);
        digitalWrite(ASSIST_RESET_LED, LOW);
        digitalWrite(VICTORY_T1_LED, LOW);
        digitalWrite(VICTORY_T2_LED, LOW);
        digitalWrite(TIME_DOWN_LED, HIGH);
        digitalWrite(TIME_UP_LED, HIGH);
        break;
      case MATCH       :
        digitalWrite(MATCH_START_LED, LOW);
        digitalWrite(MATCH_STOP_LED, HIGH);
        digitalWrite(MATCH_RESET_LED, LOW);
        digitalWrite(ASSIST_RESET_LED, LOW);
        digitalWrite(VICTORY_T1_LED, LOW);
        digitalWrite(VICTORY_T2_LED, LOW);
        digitalWrite(TIME_DOWN_LED, HIGH);
        digitalWrite(TIME_UP_LED, HIGH);
        updateClock();
        break;
      case ASSIST      :
        soundboard.playSpecified(ASSIST_SOUND);
        digitalWrite(MATCH_START_LED, LOW);
        digitalWrite(MATCH_STOP_LED, HIGH);
        digitalWrite(MATCH_RESET_LED, LOW);
        digitalWrite(ASSIST_RESET_LED, HIGH);
        digitalWrite(VICTORY_T1_LED, LOW);
        digitalWrite(VICTORY_T2_LED, LOW);
        digitalWrite(TIME_DOWN_LED, HIGH);
        digitalWrite(TIME_UP_LED, HIGH);
        flickerLedState = HIGH;
        lastButtonFlickerTick = millis();
        break;
      case PAUSED      :
        soundboard.playSpecified(PAUSE_SOUND);
        digitalWrite(MATCH_START_LED, LOW);
        digitalWrite(MATCH_STOP_LED, HIGH);
        digitalWrite(MATCH_RESET_LED, LOW);
        digitalWrite(ASSIST_RESET_LED, LOW);
        digitalWrite(VICTORY_T1_LED, LOW);
        digitalWrite(VICTORY_T2_LED, LOW);
        digitalWrite(TIME_DOWN_LED, HIGH);
        digitalWrite(TIME_UP_LED, HIGH);
        break;
      case MATCH_END   :
        digitalWrite(MATCH_START_LED, LOW);
        digitalWrite(MATCH_STOP_LED, LOW);
        digitalWrite(MATCH_RESET_LED, HIGH);
        digitalWrite(ASSIST_RESET_LED, LOW);
        digitalWrite(VICTORY_T1_LED, HIGH);
        digitalWrite(VICTORY_T2_LED, HIGH);
        digitalWrite(TIME_DOWN_LED, LOW);
        digitalWrite(TIME_UP_LED, LOW);
        break;
      case TAPOUT_T1   :
        soundboard.playSpecified(TAPOUT_SOUND);
        digitalWrite(MATCH_START_LED, LOW);
        digitalWrite(MATCH_STOP_LED, LOW);
        digitalWrite(MATCH_RESET_LED, LOW);
        digitalWrite(ASSIST_RESET_LED, LOW);
        digitalWrite(VICTORY_T1_LED, LOW);
        digitalWrite(VICTORY_T2_LED, LOW);
        digitalWrite(TIME_DOWN_LED, LOW);
        digitalWrite(TIME_UP_LED, LOW);
        lastTapoutTick = millis();
        break;
      case TAPOUT_T2   :
        soundboard.playSpecified(TAPOUT_SOUND);
        digitalWrite(MATCH_START_LED, LOW);
        digitalWrite(MATCH_STOP_LED, LOW);
        digitalWrite(MATCH_RESET_LED, LOW);
        digitalWrite(ASSIST_RESET_LED, LOW);
        digitalWrite(VICTORY_T1_LED, LOW);
        digitalWrite(VICTORY_T2_LED, LOW);
        digitalWrite(TIME_DOWN_LED, LOW);
        digitalWrite(TIME_UP_LED, LOW);
        lastTapoutTick = millis();
        break;
      case VICTORY_T1  :
        soundboard.playSpecified(GREEN_TEAM_VICTORY_SOUND);
        digitalWrite(MATCH_START_LED, LOW);
        digitalWrite(MATCH_STOP_LED, LOW);
        digitalWrite(MATCH_RESET_LED, HIGH);
        digitalWrite(ASSIST_RESET_LED, LOW);
        digitalWrite(VICTORY_T1_LED, LOW);
        digitalWrite(VICTORY_T2_LED, LOW);
        digitalWrite(TIME_DOWN_LED, LOW);
        digitalWrite(TIME_UP_LED, LOW);
        break;
      case VICTORY_T2  :
        soundboard.playSpecified(ORANGE_TEAM_VICTORY_SOUND);
        digitalWrite(MATCH_START_LED, LOW);
        digitalWrite(MATCH_STOP_LED, LOW);
        digitalWrite(MATCH_RESET_LED, HIGH);
        digitalWrite(ASSIST_RESET_LED, LOW);
        digitalWrite(VICTORY_T1_LED, LOW);
        digitalWrite(VICTORY_T2_LED, LOW);
        digitalWrite(TIME_DOWN_LED, LOW);
        digitalWrite(TIME_UP_LED, LOW);
        break;
    }
}