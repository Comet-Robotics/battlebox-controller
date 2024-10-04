#include "WString.h"
#ifndef COMMON_H
#define COMMON_H

enum StateName : uint8_t {NO_STATE, IDLE, READY_CHECK, READY_T1, READY_T2, READY_BOTH, COUNTDOWN, MATCH, ASSIST, PAUSED, MATCH_END, TAPOUT_T1, TAPOUT_T2, VICTORY_T1, VICTORY_T2};

// larry    MAC Address A8:42:E3:48:1B:1C
// arcadian MAC Address A8:42:E3:4B:15:18
#define LARRY_MAC_ADDR {0xA8, 0x42, 0xE3, 0x48, 0x1B, 0x1C}
#define ARCADIAN_MAC_ADDR {0xA8, 0x42, 0xE3, 0x4B, 0x15, 0x18}

String getStateString(StateName state_name);
void printStateChange(StateName state_name);

#endif