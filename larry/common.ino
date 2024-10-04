#include "common.h"
#include "WString.h"

String getStateString(StateName state_name){
  String state_string;
  switch (state_name) {
    case IDLE        :
        state_string = "IDLE";
        break;
      case READY_CHECK :
        state_string = "READY_CHECK";
        break;
      case READY_T1    :
        state_string = "READY_T1";
        break;
      case READY_T2    :
        state_string = "READY_t2";
        break;
      case READY_BOTH  :
        state_string = "READY_BOTH";
        break;
      case COUNTDOWN   :
        state_string = "COUNTDOWN";
        break;
      case MATCH       :
        state_string = "MATCH";
        break;
      case ASSIST      :
        state_string = "ASSIST";
        break;
      case PAUSED      :
        state_string = "PAUSED";
        break;
      case MATCH_END   :
        state_string = "MATCH_END";
        break;
      case TAPOUT_T1   :
        state_string = "TAPOUT_T1";
        break;
      case TAPOUT_T2   :
        state_string = "TAPOUT_T2";
        break;
      case VICTORY_T1  :
        state_string = "VICTORY_T1";
        break;
      case VICTORY_T2  :
        state_string = "VICTORY_T2";
        break;
      default:
        state_string = "UNKNOWN";
  }
  return state_string;
}

void printStateChange(StateName state_name){
  Serial.print("State changed to ");
  Serial.println(getStateString(state_name));
}