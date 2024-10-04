#include <sys/_stdint.h>
#ifndef ARCADIAN_H
#define ARCADIAN_H
enum ArcadianButtonPins : uint8_t {FRED_READY_BUTTON=14, FRED_ASSIST_BUTTON=13, FRED_TAPOUT_BUTTON=12,
                                   AMADEUS_READY_BUTTON=5, AMADEUS_ASSIST_BUTTON=0, AMADEUS_TAPOUT_BUTTON=15};

enum ArcadianLedPins : uint8_t {FRED_READY_LED=25, FRED_ASSIST_LED=27, FRED_TAPOUT_LED=26,
                                AMADEUS_READY_LED=17, AMADEUS_ASSIST_LED=16, AMADEUS_TAPOUT_LED=2};

enum RGBLED_PINS : uint8_t {STRIP_1_PIN=21, STRIP_2_PIN=18, STRIP_3_PIN=33, STRIP_4_PIN=32};
enum RGBLED_STRIP_INDS : uint8_t {STRIP_1, STRIP_2, STRIP_3, STRIP_4};

// void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len);

// State change actions
void runStateChangeActions(StateName the_state);

// RGBLED functions
// void setSolidColor(CRGB color);
// void idleAnimation();
// void readyCheckAnimation();
// void readyT1Animation();
// void readyT2Animation();
// void readyBothAnimation();
// void countdownAnimation();
// void assistAnimation();
// void victoryT1Animation();
// void victoryT2Animation();

// // Button handler functions
// void handleFredReady();
// void handleFredTapout();
// void handleAmadeusReady();
// void handleAmadeusTapout();
// void handleAssist();

#endif