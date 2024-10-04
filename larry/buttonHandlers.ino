void handleTimeDown() {
  if (seconds >= 10) {
    seconds -= 10;
    lastCountdownTick = millis();
    updateClock();
  }
}

void handleTimeUp() {
    seconds += 10;
    lastCountdownTick = millis();
    updateClock();
}

void handleMatchStart(){
  Serial.println("Match Started!");
  if (currentState == IDLE){
    soundboard.playSpecified(READY_SOUND);
    reqState = READY_CHECK;
  } else if (currentState == READY_BOTH) {
    // soundboard.playSpecified(FIGHT_SOUND);
    reqState = COUNTDOWN;
  }
}
void handleMatchStop(){
  Serial.println("Match Stopped!");
  switch (currentState) {
    case MATCH:
    case ASSIST:
    case PAUSED:
      // soundboard.playSpecified(END_SOUND);
      reqState = MATCH_END;
      break;
  }
}
void handleMatchReset(){
  Serial.println("Match Reset!");
  if (currentState != COUNTDOWN && currentState != MATCH && currentState != ASSIST && currentState != PAUSED) {
    reqState = IDLE;
    seconds = NUM_SEC_DEFAULT;
  }
    // updateClock();
}
void handleMatchPlayPause(){
  Serial.println("Match Played/Paused!");
  if (currentState == MATCH) {
    // soundboard.playSpecified(PAUSE_SOUND);
    reqState = PAUSED;
    resumeState = MATCH;
  } else if (currentState == ASSIST) {
    soundboard.playSpecified(FIGHT_SOUND);
    reqState = PAUSED;
    resumeState = ASSIST;
  }else if (currentState == PAUSED) {
    soundboard.playSpecified(FIGHT_SOUND);
    reqState = resumeState;
    lastCountdownTick = millis();
  }
}
void handleMatchVictory1(){
  Serial.println("Match Victory - Team 1!");
  if (currentState == MATCH_END) {
    // soundboard.playSpecified(VICTORY_SOUND);
    reqState = VICTORY_T1;
  }
}
void handleMatchVictory2(){
  Serial.println("Match Victory - Team 2!");
  if (currentState == MATCH_END) {
    // soundboard.playSpecified(VICTORY_SOUND);
    reqState = VICTORY_T2;
  }
}
void handleAssistReset(){
  Serial.println("Match Assist Reset!");
  if (currentState == ASSIST)
    reqState = MATCH;
}