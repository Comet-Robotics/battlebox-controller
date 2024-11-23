// THE BRAIN (Arcadian)
// Larry - Judges Panel
// Fred - Green (EPICS Side) - Team 2
// Amadeus - Orange (Studio Side) - Team 1
#define FASTLED_ALLOW_INTERRUPTS 0
// Time in milliseconds that the countdown/tapout sequences run
#define COUNTDOWN_TIME_MS 3000
#define COUNTDOWN_OFFSET 500
#define TAPOUT_TIME_MS 5000
#define RGBLED_MAX_BRIGHTNESS 255
#define RGBLED_FPS 120

#include <Arduino.h>

// ESP NOW
#include <WiFi.h>
#include <esp_now.h>

// Button handling
#include <Button.h>

// RGBLED Strips
#include <FastLED.h>

#include "common.h"
#include "main.h"

// Track current state of system (controlled by Arcadian)
StateName currentState = IDLE;
StateName reqState = IDLE;
StateName resumeState = NO_STATE;
StateName changeState = NO_STATE;
uint8_t countdownPhase;

// ESP Now Vars
esp_now_peer_info_t peerInfo_Larry;
uint8_t broadcastAddress[] = LARRY_MAC_ADDR;
String success;
esp_err_t result;
bool retrySend = false;
CRGB color_yellow = CRGB(200,100,0);
// Active low buttons with internal pull-up resistors
// OneButton fredReadyButton = OneButton(
//   FRED_READY_BUTTON,  // Input pin for the button
//   true,        // Button is active LOW
//   true         // Enable internal pull-up resistor
// );
// OneButton fredAssistButton = OneButton(FRED_ASSIST_BUTTON, false, false);
// OneButton fredTapoutButton = OneButton(FRED_TAPOUT_BUTTON, false, false);
// OneButton amadeusReadyButton = OneButton(AMADEUS_READY_BUTTON, true, true);
// OneButton amadeusAssistButton = OneButton(AMADEUS_ASSIST_BUTTON, true, true);
// OneButton amadeusTapoutButton = OneButton(AMADEUS_TAPOUT_BUTTON, true, true);

// Active high buttons with internal pull-down resistors
// OneButton *fredAssistButton;
// OneButton *fredTapoutButton;

Button fredReadyButton(FRED_READY_BUTTON);
Button fredAssistButton(FRED_ASSIST_BUTTON);
Button fredTapoutButton(FRED_TAPOUT_BUTTON);
Button amadeusReadyButton(AMADEUS_READY_BUTTON);
Button amadeusAssistButton(AMADEUS_ASSIST_BUTTON);
Button amadeusTapoutButton(AMADEUS_TAPOUT_BUTTON);

// Timer variables
unsigned long lastMatchStartTick = 0;
unsigned long currentTick = 1;
unsigned long lastTapoutTick = 0;

//RGBLED Strips
#define NUM_LED_STRIPS 4
#define NUM_LEDS_UP 24
#define NUM_LEDS_OVER 48
#define NUM_LEDS_PER_STRIP NUM_LEDS_UP+NUM_LEDS_OVER
#define NUM_LEDS NUM_LEDS_PER_STRIP*NUM_LED_STRIPS
#define SLOW_BPM 60
#define FAST_BPM 60
#define GLITTER_CHANCE 150
CRGB team1Color = CRGB(255,30,0);
CRGB team2Color = CRGB::Green;
CRGB leds[NUM_LED_STRIPS][NUM_LEDS_PER_STRIP];
uint8_t gHue = 0;

void setup() {
  // put your setup code here, to run once:
  // Initilize serial communication
  Serial.begin(115200);

  Serial.print("Initializing ESP NOW");
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  Serial.print(".");

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  } else {
    Serial.print(".");
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);
  Serial.print(".");

  // Register broadcast address for ESP Now
  memcpy(peerInfo_Larry.peer_addr, broadcastAddress, 6);
  peerInfo_Larry.channel = 0;  
  peerInfo_Larry.encrypt = false;
  if (esp_now_add_peer(&peerInfo_Larry) != ESP_OK) {
    Serial.println("Failed to add ESP now peer");
    return;
  } else {
    Serial.println(".SUCCESS!");
  }

  Serial.print("Configuring push buttons...");
  // fredAssistButton = new OneButton();
  // fredTapoutButton = new OneButton();
  pinMode(FRED_READY_BUTTON, INPUT_PULLUP);
  pinMode(FRED_ASSIST_BUTTON, INPUT_PULLDOWN);
  pinMode(FRED_TAPOUT_BUTTON, INPUT_PULLDOWN);
  pinMode(AMADEUS_READY_BUTTON, INPUT_PULLUP);
  pinMode(AMADEUS_ASSIST_BUTTON, INPUT_PULLUP);
  pinMode(AMADEUS_TAPOUT_BUTTON, INPUT_PULLUP);

  // Button handlers
  // fredReadyButton.attachClick(handleFredReady);
  // fredAssistButton.attachClick(handleAssist);
  // fredTapoutButton.attachClick(handleFredTapout);
  // amadeusReadyButton.attachClick(handleAmadeusReady);
  // amadeusAssistButton.attachClick(handleAssist);
  // amadeusTapoutButton.attachClick(handleAmadeusTapout);
  Serial.print("DONE!");

  Serial.print("Configuring LED outputs...");
  // Set up LED pins for output
  pinMode(FRED_READY_LED, OUTPUT);
  pinMode(FRED_ASSIST_LED, OUTPUT);
  pinMode(FRED_TAPOUT_LED, OUTPUT);
  pinMode(AMADEUS_READY_LED, OUTPUT);
  pinMode(AMADEUS_ASSIST_LED, OUTPUT);
  pinMode(AMADEUS_TAPOUT_LED, OUTPUT);
  Serial.println("DONE!");


  // Set up RGBLEDS
  Serial.print("Configuring RGB LED Strips...");
  FastLED.addLeds<WS2811, STRIP_1_PIN, BRG>(leds[STRIP_1], NUM_LEDS_PER_STRIP);
  FastLED.addLeds<WS2811, STRIP_2_PIN, BRG>(leds[STRIP_2], NUM_LEDS_PER_STRIP);
  FastLED.addLeds<WS2811, STRIP_3_PIN, BRG>(leds[STRIP_3], NUM_LEDS_PER_STRIP);
  FastLED.addLeds<WS2811, STRIP_4_PIN, BRG>(leds[STRIP_4], NUM_LEDS_PER_STRIP);

  FastLED.setBrightness(RGBLED_MAX_BRIGHTNESS);
  // for (int i = 0; i < NUM_LED_STRIPS; i++) {
  //   fill_solid(leds[i], NUM_LEDS_PER_STRIP, CRGB::White);
  // }
  // FastLED.show();

  Serial.println("DONE!");

  Serial.println("Setup complete!");
}

// void loop() {}
void loop() {
  // State controls
  // Change state from Arcadian has priority
  if (reqState || changeState){

    // Handle new state
    if (changeState){
      currentState = changeState;
      result = esp_now_send(broadcastAddress, (uint8_t *) &changeState, sizeof(changeState));
      if (result == ESP_OK){
        // Serial.println("State Request Sent Sucessfully!");
      } else {
        Serial.println("Larry not responding! :(");
        retrySend = true;
      }
      changeState = NO_STATE;
    } else {
      currentState = reqState;
    }
    // clear request state
    reqState = NO_STATE;
    // Output state change to serial
    printStateChange(currentState);

    // Changes based on new state
    runStateChangeActions(currentState);
  } else if (retrySend) {
    // Retry sending state to Larry
    Serial.println("Retrying Larry state update");
    result = esp_now_send(broadcastAddress, (uint8_t *) &currentState, sizeof(currentState));
    if (result == ESP_OK){
      retrySend = false;
    } else {
      Serial.println("Larry STILL not responding! :(");
    }
  }
  
  // Button ticks
  if (fredReadyButton.pressed()) {
    handleFredReady();
  } else if (fredAssistButton.released()) {
    handleAssist();
  } else if (fredTapoutButton.released()) {
    handleFredTapout();
  } else if (amadeusReadyButton.pressed()) {
    handleAmadeusReady();
  } else if (amadeusAssistButton.pressed()) {
    handleAssist();
  } else if (amadeusTapoutButton.pressed()){
    handleAmadeusTapout();
  }
  // fredReadyButton.tick();
  // // bool fredAssistButtonIsPressed = digitalRead(FRED_ASSIST_BUTTON) == LOW;
  // // bool fredTapoutButtonIsPressed = digitalRead(FRED_TAPOUT_BUTTON) == LOW;
  // // fredAssistButton->tick(fredAssistButtonIsPressed);
  // // fredTapoutButton->tick(fredTapoutButtonIsPressed);
  // fredAssistButton.tick();
  // fredTapoutButton.tick();
  // amadeusReadyButton.tick();
  // // amadeusAssistButton.tick();
  // amadeusTapoutButton.tick();

  // Timers for state change from tapout to victory
  currentTick = millis();
  if (currentState == COUNTDOWN) {
    if (currentTick - lastMatchStartTick >= (COUNTDOWN_TIME_MS / 3) * (3-countdownPhase)+COUNTDOWN_OFFSET) {
      switch (countdownPhase) {
        case 3:
        case 2:
          for (int strip_ind=0; strip_ind<NUM_LED_STRIPS; strip_ind++){
            fill_solid(leds[strip_ind], NUM_LEDS_PER_STRIP, CRGB::Red);
          }   
          break;
        case 1:
          for (int strip_ind=0; strip_ind<NUM_LED_STRIPS; strip_ind++){
            fill_solid(leds[strip_ind], NUM_LEDS_PER_STRIP, color_yellow);
          }
          break;
      }
      countdownPhase--;
    }
  }
  if (currentTick - lastMatchStartTick >= COUNTDOWN_TIME_MS + COUNTDOWN_OFFSET && currentState == COUNTDOWN) {
    changeState = MATCH;
  }
  if (currentTick - lastTapoutTick >= TAPOUT_TIME_MS && (currentState == TAPOUT_T1 || currentState == TAPOUT_T2)){
    if (currentState == TAPOUT_T1) {
      changeState == VICTORY_T2;
    } else {
      changeState == VICTORY_T1;
    }
  }

  // Update RGBLED state for changing patterns
  switch (currentState) {
    case IDLE:
      FastLED.setBrightness(150);
      idleAnimation();
      break;
    case READY_CHECK:
      FastLED.setBrightness(150);
      readyCheckAnimation();
      break;
    case READY_T1:
      FastLED.setBrightness(150);
      readyT1Animation();
      break;
    case READY_T2:
      FastLED.setBrightness(150);
      readyT2Animation();
      break;
    case READY_BOTH:
      FastLED.setBrightness(150);
      readyBothAnimation();
      break;
    case COUNTDOWN:
      FastLED.setBrightness(255);
      // FastLED.setBrightness(150);
      countdownAnimation();
      break;
    case MATCH:
      FastLED.setBrightness(255);
      // Green for first second of the match and then white
      if(currentTick - lastMatchStartTick <= COUNTDOWN_TIME_MS + COUNTDOWN_OFFSET + 1300){
        for (int strip_ind=0; strip_ind<NUM_LED_STRIPS; strip_ind++){
          fill_solid(leds[strip_ind], NUM_LEDS_PER_STRIP, CRGB::Green);
        }
        // setSolidColor(CRGB::Green);
      } else {
        FastLED.setBrightness(255);
        // for (int i = 0; i < 4; i++) {
        //   fill_solid(leds[i], NUM_LEDS_PER_STRIP, CRGB::White);
        // }
        for (int strip_ind=0; strip_ind<NUM_LED_STRIPS; strip_ind++){
          fill_solid(leds[strip_ind], NUM_LEDS_PER_STRIP, CRGB::White);
        }
      }
      break;
    case PAUSED:
      FastLED.setBrightness(255);
      // for (int i = 0; i < 4; i++) {
      //   fill_solid(leds[i], NUM_LEDS_PER_STRIP, CRGB::Red);
      // }
      for (int strip_ind=0; strip_ind<NUM_LED_STRIPS; strip_ind++){
        fill_solid(leds[strip_ind], NUM_LEDS_PER_STRIP, CRGB::Red);
      }
      break;
    case ASSIST:
      FastLED.setBrightness(150);
      assistAnimation();
      break;
    case MATCH_END:
      FastLED.setBrightness(255);
      // for (int i = 0; i < 4; i++) {
      //   fill_solid(leds[i], NUM_LEDS_PER_STRIP, CRGB::Red);
      // }
      for (int strip_ind=0; strip_ind<NUM_LED_STRIPS; strip_ind++){
        fill_solid(leds[strip_ind], NUM_LEDS_PER_STRIP, CRGB::Red);
      }
      break;
    case TAPOUT_T1:
      FastLED.setBrightness(255);
      // for (int i = 0; i < 4; i++) {
      //   fill_solid(leds[i], NUM_LEDS_PER_STRIP, CRGB::Red);
      // }
      for (int strip_ind=0; strip_ind<NUM_LED_STRIPS; strip_ind++){
        fill_solid(leds[strip_ind], NUM_LEDS_PER_STRIP, CRGB::Red);
      }
      break;
    case TAPOUT_T2:
      FastLED.setBrightness(255);
      // for (int i = 0; i < 4; i++) {
      //   fill_solid(leds[i], NUM_LEDS_PER_STRIP, CRGB::Red);
      // }
      for (int strip_ind=0; strip_ind<NUM_LED_STRIPS; strip_ind++){
        fill_solid(leds[strip_ind], NUM_LEDS_PER_STRIP, CRGB::Red);
      }
      break;
    case VICTORY_T1:
      FastLED.setBrightness(150);
      victoryT1Animation();
      break;
    case VICTORY_T2:
      FastLED.setBrightness(150);
      victoryT2Animation();
      break;
  }
  // RGBLED state variable changes
  // Update RGBLEDs
  FastLED.delay(1000/RGBLED_FPS);
  gHue = 12;

  EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle RGBLED "base color" through the rainbow
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status ==0){
    success = "Delivery Success :)";
    // changeState = NO_STATE;
  }
  else{
    success = "Delivery Fail :(";
  }
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&reqState, incomingData, sizeof(reqState));
  // Serial.println("New state request recieved");
}


// Solid color for entire box
// void setSolidColor(CRGB color){
  
// }

// Random colored speckles that blink in and fade smoothly
void idleAnimation(){
  // Turn off all of these darn LEDs
  // for (int strip_ind=0; strip_ind < NUM_LED_STRIPS; strip_ind++){
  //   fill_solid(leds[strip_ind], NUM_LEDS_PER_STRIP, CRGB::Black);
  // }
  // if (random8() < GLITTER_CHANCE){ // Number between 0 and 255 is change of glittering
  //   for (int strip_ind=0; strip_ind < NUM_LED_STRIPS; strip_ind++){
  //     leds[strip_ind][random16(NUM_LEDS_PER_STRIP)] += CRGB::White;
  //   }
  // }
  for (int strip_ind=0; strip_ind < NUM_LED_STRIPS; strip_ind++){
    fadeToBlackBy( leds[strip_ind], NUM_LEDS_PER_STRIP, 10);
    int pos = random16(NUM_LEDS_PER_STRIP-1);
    leds[strip_ind][pos] += CHSV( gHue + random8(64), 200, 255);
  }
}

// Both teams colors moving from top to bottom on their respective sides
void readyCheckAnimation(){
  for (int strip_ind=0; strip_ind < NUM_LED_STRIPS; strip_ind++){
    fadeToBlackBy(leds[strip_ind], NUM_LEDS_PER_STRIP, 20);
  }
  int pos = scale8(beat8(SLOW_BPM), NUM_LEDS_PER_STRIP - NUM_LEDS_OVER / 2) + NUM_LEDS_OVER/2;
  leds[STRIP_1][NUM_LEDS_PER_STRIP-pos] = team1Color;
  leds[STRIP_2][NUM_LEDS_PER_STRIP-pos] = team1Color;
  leds[STRIP_3][NUM_LEDS_PER_STRIP-pos] = team2Color;
  leds[STRIP_4][NUM_LEDS_PER_STRIP-pos] = team2Color;
  if (pos < NUM_LEDS_OVER){
    leds[STRIP_2][NUM_LEDS_PER_STRIP - NUM_LEDS_OVER + pos] = team2Color;
    leds[STRIP_3][NUM_LEDS_PER_STRIP - NUM_LEDS_OVER + pos] = team2Color;
    leds[STRIP_1][NUM_LEDS_PER_STRIP - NUM_LEDS_OVER + pos] = team1Color;
    leds[STRIP_4][NUM_LEDS_PER_STRIP - NUM_LEDS_OVER + pos] = team1Color;
  }  
}

// Team 1 color solid on their side, team 2 color moving from top to bottom on their side
void readyT1Animation(){
  for (int strip_ind=0; strip_ind < NUM_LED_STRIPS; strip_ind++){
    fadeToBlackBy(leds[strip_ind], NUM_LEDS_PER_STRIP, 20);
  }
  int pos = scale8(beat8(SLOW_BPM), NUM_LEDS_PER_STRIP - NUM_LEDS_OVER / 2) + NUM_LEDS_OVER/2;
  leds[STRIP_3][NUM_LEDS_PER_STRIP-pos] = team2Color;
  leds[STRIP_4][NUM_LEDS_PER_STRIP-pos] = team2Color;
  if (pos < NUM_LEDS_OVER){
    leds[STRIP_2][NUM_LEDS_PER_STRIP - NUM_LEDS_OVER + pos] = team2Color;
    leds[STRIP_3][NUM_LEDS_PER_STRIP - NUM_LEDS_OVER + pos] = team2Color;
  }
  fill_solid(leds[STRIP_1], NUM_LEDS_PER_STRIP, team1Color);
  fill_solid(leds[STRIP_2], NUM_LEDS_UP+NUM_LEDS_OVER/2, team1Color);
  fill_solid(leds[STRIP_4] + NUM_LEDS_UP+NUM_LEDS_OVER/2, NUM_LEDS_OVER/2, team1Color);
}

// Team 1 color moving from top to bottom on their side, team 2 color solid on their side
void readyT2Animation(){
  for (int strip_ind=0; strip_ind < NUM_LED_STRIPS; strip_ind++){
    fadeToBlackBy(leds[strip_ind], NUM_LEDS_PER_STRIP, 20);
  }
  int pos = scale8(beat8(SLOW_BPM), NUM_LEDS_PER_STRIP - NUM_LEDS_OVER / 2) + NUM_LEDS_OVER/2;
  leds[STRIP_1][NUM_LEDS_PER_STRIP-pos] = team1Color;
  leds[STRIP_2][NUM_LEDS_PER_STRIP-pos] = team1Color;
  if (pos < NUM_LEDS_OVER){
    leds[STRIP_1][NUM_LEDS_PER_STRIP - NUM_LEDS_OVER + pos] = team1Color;
    leds[STRIP_4][NUM_LEDS_PER_STRIP - NUM_LEDS_OVER + pos] = team1Color;
  }
  fill_solid(leds[STRIP_2] + NUM_LEDS_UP+NUM_LEDS_OVER/2, NUM_LEDS_OVER/2, team2Color);
  fill_solid(leds[STRIP_3], NUM_LEDS_PER_STRIP, team2Color);
  fill_solid(leds[STRIP_4], NUM_LEDS_UP+NUM_LEDS_OVER/2, team2Color);
}

// Solid team colors on each side
void readyBothAnimation(){
  fill_solid(leds[STRIP_1], NUM_LEDS_PER_STRIP, team1Color);
  fill_solid(leds[STRIP_2], NUM_LEDS_UP+NUM_LEDS_OVER/2, team1Color);
  fill_solid(leds[STRIP_2] + NUM_LEDS_UP+NUM_LEDS_OVER/2, NUM_LEDS_OVER/2, team2Color);
  fill_solid(leds[STRIP_3], NUM_LEDS_PER_STRIP, team2Color);
  fill_solid(leds[STRIP_4], NUM_LEDS_UP+NUM_LEDS_OVER/2, team2Color);
  fill_solid(leds[STRIP_4] + NUM_LEDS_UP+NUM_LEDS_OVER/2, NUM_LEDS_OVER/2, team1Color);
}

// Fade to black
void countdownAnimation(){
  for (int strip_ind=0; strip_ind < NUM_LED_STRIPS; strip_ind++){
    fadeToBlackBy(leds[strip_ind], NUM_LEDS_PER_STRIP, 2);
  }
}

// Yellow dots sweeping around the top and down the sides
void assistAnimation(){
  for (int strip_ind=0; strip_ind < NUM_LED_STRIPS; strip_ind++){
    fadeToBlackBy(leds[strip_ind], NUM_LEDS_PER_STRIP, 20);
  }
  int posTop = NUM_LEDS_PER_STRIP - scale16(beat16(SLOW_BPM, 0) , NUM_LEDS_OVER-1 )-1;
  int posOver = NUM_LEDS_UP - scale16(beat16(FAST_BPM, 1), NUM_LEDS_UP-1)-1;
  for (int strip_ind=0; strip_ind < NUM_LED_STRIPS; strip_ind++){
    leds[strip_ind][posTop] = color_yellow;
    leds[strip_ind][posOver] = color_yellow;
  }
}

// Team 1 color on their side with sparkles
void victoryT1Animation(){
  fill_solid(leds[STRIP_1], NUM_LEDS_PER_STRIP, team1Color);
  fill_solid(leds[STRIP_2], NUM_LEDS_UP+NUM_LEDS_OVER/2, team1Color);
  fill_solid(leds[STRIP_4] + NUM_LEDS_UP+NUM_LEDS_OVER/2, NUM_LEDS_OVER/2, team1Color);

  // Add glitter
  if (random8() < GLITTER_CHANCE){ // Number between 0 and 255 is change of glittering
    leds[STRIP_1][random16(NUM_LEDS_PER_STRIP)] += CRGB::White;
  }
  if (random8() < GLITTER_CHANCE){
    leds[STRIP_2][random16(NUM_LEDS_UP+NUM_LEDS_OVER/2)] += CRGB::White;
  }
  if (random8() < GLITTER_CHANCE){
    leds[STRIP_4][random16(NUM_LEDS_OVER/2) + NUM_LEDS_UP+NUM_LEDS_OVER/2] += CRGB::White;
  }
  // Team 2 off
  fill_solid(leds[STRIP_2] + NUM_LEDS_UP+NUM_LEDS_OVER/2, NUM_LEDS_OVER/2, CRGB::Black);
  fill_solid(leds[STRIP_3], NUM_LEDS_PER_STRIP, CRGB::Black);
  fill_solid(leds[STRIP_4], NUM_LEDS_UP+NUM_LEDS_OVER/2, CRGB::Black);
}

// Team 2 color on their side with sparkles
void victoryT2Animation(){
  fill_solid(leds[STRIP_2] + NUM_LEDS_UP+NUM_LEDS_OVER/2, NUM_LEDS_OVER/2, team2Color);
  fill_solid(leds[STRIP_3], NUM_LEDS_PER_STRIP, team2Color);
  fill_solid(leds[STRIP_4], NUM_LEDS_UP+NUM_LEDS_OVER/2, team2Color);

  if (random8() < GLITTER_CHANCE){ // Number between 0 and 255 is change of glittering
    leds[STRIP_3][random16(NUM_LEDS_PER_STRIP)] += CRGB::White;
  }
  if (random8() < GLITTER_CHANCE){
    leds[STRIP_4][random16(NUM_LEDS_UP+NUM_LEDS_OVER/2)] += CRGB::White;
  }
  if (random8() < GLITTER_CHANCE){
    leds[STRIP_2][random16(NUM_LEDS_OVER/2) + NUM_LEDS_UP+NUM_LEDS_OVER/2] += CRGB::White;
  }

  // Team 1 off
  fill_solid(leds[STRIP_1], NUM_LEDS_PER_STRIP, CRGB::Black);
  fill_solid(leds[STRIP_2], NUM_LEDS_UP+NUM_LEDS_OVER/2, CRGB::Black);
  fill_solid(leds[STRIP_4] + NUM_LEDS_UP+NUM_LEDS_OVER/2, NUM_LEDS_OVER/2, CRGB::Black);
}

void runStateChangeActions(StateName the_state){
  // Set button LEDs
    switch(the_state)
    {
      case IDLE        :
        digitalWrite(FRED_READY_LED, LOW);
        digitalWrite(FRED_ASSIST_LED, LOW);
        digitalWrite(FRED_TAPOUT_LED, LOW);
        digitalWrite(AMADEUS_READY_LED, LOW);
        digitalWrite(AMADEUS_ASSIST_LED, LOW);
        digitalWrite(AMADEUS_TAPOUT_LED, LOW);
        break;
      case READY_CHECK :
        digitalWrite(FRED_READY_LED, HIGH);
        digitalWrite(FRED_ASSIST_LED, LOW);
        digitalWrite(FRED_TAPOUT_LED, LOW);
        digitalWrite(AMADEUS_READY_LED, HIGH);
        digitalWrite(AMADEUS_ASSIST_LED, LOW);
        digitalWrite(AMADEUS_TAPOUT_LED, LOW);
        break;
      case READY_T1    :
        digitalWrite(FRED_READY_LED, HIGH);
        digitalWrite(FRED_ASSIST_LED, LOW);
        digitalWrite(FRED_TAPOUT_LED, LOW);
        digitalWrite(AMADEUS_READY_LED, LOW);
        digitalWrite(AMADEUS_ASSIST_LED, LOW);
        digitalWrite(AMADEUS_TAPOUT_LED, LOW);
        break;
      case READY_T2    :
        digitalWrite(FRED_READY_LED, LOW);
        digitalWrite(FRED_ASSIST_LED, LOW);
        digitalWrite(FRED_TAPOUT_LED, LOW);
        digitalWrite(AMADEUS_READY_LED, HIGH);
        digitalWrite(AMADEUS_ASSIST_LED, LOW);
        digitalWrite(AMADEUS_TAPOUT_LED, LOW);
        break;
      case READY_BOTH  :
        digitalWrite(FRED_READY_LED, LOW);
        digitalWrite(FRED_ASSIST_LED, LOW);
        digitalWrite(FRED_TAPOUT_LED, LOW);
        digitalWrite(AMADEUS_READY_LED, LOW);
        digitalWrite(AMADEUS_ASSIST_LED, LOW);
        digitalWrite(AMADEUS_TAPOUT_LED, LOW);
        break;
      case COUNTDOWN   :
        lastMatchStartTick = millis();
        countdownPhase = 3;
        digitalWrite(FRED_READY_LED, LOW);
        digitalWrite(FRED_ASSIST_LED, LOW);
        digitalWrite(FRED_TAPOUT_LED, LOW);
        digitalWrite(AMADEUS_READY_LED, LOW);
        digitalWrite(AMADEUS_ASSIST_LED, LOW);
        digitalWrite(AMADEUS_TAPOUT_LED, LOW);
        break;
      case MATCH       :
        digitalWrite(FRED_READY_LED, LOW);
        digitalWrite(FRED_ASSIST_LED, HIGH);
        digitalWrite(FRED_TAPOUT_LED, HIGH);
        digitalWrite(AMADEUS_READY_LED, LOW);
        digitalWrite(AMADEUS_ASSIST_LED, HIGH);
        digitalWrite(AMADEUS_TAPOUT_LED, HIGH);
        break;
      case ASSIST      :
        digitalWrite(FRED_READY_LED, LOW);
        digitalWrite(FRED_ASSIST_LED, LOW);
        digitalWrite(FRED_TAPOUT_LED, HIGH);
        digitalWrite(AMADEUS_READY_LED, LOW);
        digitalWrite(AMADEUS_ASSIST_LED, LOW);
        digitalWrite(AMADEUS_TAPOUT_LED, HIGH);
        break;
      case PAUSED      :
        digitalWrite(FRED_READY_LED, LOW);
        digitalWrite(FRED_ASSIST_LED, LOW);
        digitalWrite(FRED_TAPOUT_LED, LOW);
        digitalWrite(AMADEUS_READY_LED, LOW);
        digitalWrite(AMADEUS_ASSIST_LED, LOW);
        digitalWrite(AMADEUS_TAPOUT_LED, LOW);
        break;
      case MATCH_END   :
        digitalWrite(FRED_READY_LED, LOW);
        digitalWrite(FRED_ASSIST_LED, LOW);
        digitalWrite(FRED_TAPOUT_LED, LOW);
        digitalWrite(AMADEUS_READY_LED, LOW);
        digitalWrite(AMADEUS_ASSIST_LED, LOW);
        digitalWrite(AMADEUS_TAPOUT_LED, LOW);
        break;
      case TAPOUT_T1   :
        digitalWrite(FRED_READY_LED, LOW);
        digitalWrite(FRED_ASSIST_LED, LOW);
        digitalWrite(FRED_TAPOUT_LED, LOW);
        digitalWrite(AMADEUS_READY_LED, LOW);
        digitalWrite(AMADEUS_ASSIST_LED, LOW);
        digitalWrite(AMADEUS_TAPOUT_LED, LOW);
        break;
      case TAPOUT_T2   :
        digitalWrite(FRED_READY_LED, LOW);
        digitalWrite(FRED_ASSIST_LED, LOW);
        digitalWrite(FRED_TAPOUT_LED, LOW);
        digitalWrite(AMADEUS_READY_LED, LOW);
        digitalWrite(AMADEUS_ASSIST_LED, LOW);
        digitalWrite(AMADEUS_TAPOUT_LED, LOW);
        break;
      case VICTORY_T1  :
        digitalWrite(FRED_READY_LED, LOW);
        digitalWrite(FRED_ASSIST_LED, LOW);
        digitalWrite(FRED_TAPOUT_LED, LOW);
        digitalWrite(AMADEUS_READY_LED, LOW);
        digitalWrite(AMADEUS_ASSIST_LED, LOW);
        digitalWrite(AMADEUS_TAPOUT_LED, LOW);
        break;
      case VICTORY_T2  :
        digitalWrite(FRED_READY_LED, LOW);
        digitalWrite(FRED_ASSIST_LED, LOW);
        digitalWrite(FRED_TAPOUT_LED, LOW);
        digitalWrite(AMADEUS_READY_LED, LOW);
        digitalWrite(AMADEUS_ASSIST_LED, LOW);
        digitalWrite(AMADEUS_TAPOUT_LED, LOW);
        break;
    }
}

void handleFredReady(){
  if (currentState==READY_CHECK) {
    changeState = READY_T2;
  } else if (currentState==READY_T1) {
    changeState = READY_BOTH;
  }
}
void handleFredTapout(){
  if (currentState==MATCH || currentState==ASSIST) {
    changeState = TAPOUT_T2;
    lastTapoutTick = millis();
  }
}
void handleAmadeusReady(){
  Serial.println("Amadaeus ready pressed!");
  if (currentState==READY_CHECK) {
    changeState = READY_T1;
  } else if (currentState==READY_T2) {
    changeState = READY_BOTH;
  }
}
void handleAmadeusTapout(){
  if (currentState==MATCH || currentState==ASSIST) {
    changeState = TAPOUT_T1;
    lastTapoutTick = millis();
  }
}

void handleAssist(){
  if (currentState==MATCH) {
    changeState = ASSIST;
  }
}