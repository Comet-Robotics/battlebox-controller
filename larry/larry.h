#include <sys/_stdint.h>
#ifndef LARRY_H
#define LARRY_H
enum LarryButtonPins : uint8_t {MATCH_START_BUTTON = 35, MATCH_STOP_BUTTON = 12, MATCH_RESET_BUTTON = 13, MATCH_PLAY_PAUSE_BUTTON = 0, 
                                ASSIST_RESET_BUTTON = 14, VICTORY_T1_BUTTON = 5, VICTORY_T2_BUTTON = 15, TIME_DOWN_BUTTON = 4, TIME_UP_BUTTON = 2};
enum LarryLedPins : uint8_t {MATCH_START_LED = 32, MATCH_STOP_LED = 25, MATCH_RESET_LED = 26, ASSIST_RESET_LED = 33, 
                             VICTORY_T1_LED = 27, VICTORY_T2_LED = 18, TIME_DOWN_LED = 23, TIME_UP_LED = 19};

enum SoundNums : uint16_t {READY_SOUND=1, ORANGE_TEAM_READY_SOUND, GREEN_TEAM_READY_SOUND, BOTH_TEAM_READY_SOUND,
                           COUNTDOWN_SOUND, ASSIST_SOUND, PAUSE_SOUND, FIGHT_SOUND, END_SOUND, ORANGE_TEAM_VICTORY_SOUND,
                           GREEN_TEAM_VICTORY_SOUND, TAPOUT_SOUND};

// void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len);
void updateClock();

// Actions taken based on new state
void runStateChangeActions(StateName the_state);

// Timer function
void runTimers();

// Button handlers
void handleTimeDown();
void handleTimeUp();
void handleMatchStart();
void handleMatchStop();
void handleMatchReset();
void handleMatchPlayPause();
void handleMatchVictory1();
void handleMatchVictory2();
void handleAssistReset();

#endif