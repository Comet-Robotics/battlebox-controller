// Judges Panel (Larry)
// Arcadian - The Brain
// Fred - Team 1 Green (EPICS Side)
// Amadeus - Team 2 Orange (Studio Side)

#include <Arduino.h>

// i2c and screen
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// ESP NOW
#include <WiFi.h>
#include <esp_now.h>
// Sound board
#include <DYPlayerArduino.h>
// Button handling
#include <OneButton.h>

// My sick header files
#include "common.h"
#include "larry.h"
#include "bitmaps.h"

// Changeable Parameters
#define NUM_SEC_DEFAULT 2 * 60

// Screen parameters
#define SSD1306_NO_SPLASH
#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 32    // OLED display height, in pixels
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

// Create display object over i2c
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

bool resetSoundLevel = false;

// Track current state of system (controlled by Arcadian)
StateName currentState = IDLE;
StateName reqState = NO_STATE;
StateName resumeState = NO_STATE;
StateName changeState = IDLE;
StateName firstReady = NO_STATE;

// ESP Now Vars
esp_now_peer_info_t peerInfo_Arcadian;
uint8_t broadcastAddress[] = ARCADIAN_MAC_ADDR;
String success;
esp_err_t result;
bool retrySend = false;

// Active low buttons with internal pull-up resistors
OneButton assistResetButton = OneButton(
    ASSIST_RESET_BUTTON, // Input pin for the button
    true,                // Button is active LOW
    true                 // Enable internal pull-up resistor
);
OneButton victory1Button = OneButton(VICTORY_T1_BUTTON, true, true);
OneButton victory2Button = OneButton(VICTORY_T2_BUTTON, true, true);
OneButton playPauseButton = OneButton(MATCH_PLAY_PAUSE_BUTTON, true, true);

// Active high buttons with internal pull-down resistors
OneButton *stopButton;
OneButton *resetButton;
OneButton *timeDownButton;
OneButton *timeUpButton;

// Active low button with EXTERNAL pull-up resistor
OneButton startButton = OneButton(
    MATCH_START_BUTTON, // Input pin for the button
    true,               // Button is active LOW
    false               // No internal pull-up resistor
);

uint16_t seconds = NUM_SEC_DEFAULT;
unsigned long lastCountdownTick = 0;
unsigned long lastButtonFlickerTick = 0;
unsigned long lastDisplayTick = 0;
unsigned long lastTapoutTick = 0;
unsigned long currentTick = 1;
unsigned long lastReadyTick = 0;
bool flickerLedState;

// Index of currently displayed logo on screen
uint8_t currentDisplay = 0;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len);
void updateClock();
void handleTimeDown();
void handleTimeUp();
void handleMatchStart();
void handleMatchStop();
void handleMatchReset();
void handleMatchPlayPause();
void handleMatchVictory1();
void handleMatchVictory2();
void handleAssistReset();
void runStateChangeActions(StateName the_state);

void setup()
{
  // Initialize serial communication
  Serial.begin(115200);
  Serial.print("Initializing SSD1306 Driver...");
  // Display initialization
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
    return;
  }
  display.clearDisplay();
  display.setTextSize(4);
  display.setTextColor(SSD1306_WHITE);
  Serial.println("SUCCESS!");

  Serial.print("Configuring push buttons");
  // Initialize internal pull-down buttons
  stopButton = new OneButton();
  resetButton = new OneButton();
  timeDownButton = new OneButton();
  timeUpButton = new OneButton();
  // Initialize internal pull-down button pins
  pinMode(MATCH_STOP_BUTTON, INPUT_PULLDOWN);
  pinMode(MATCH_RESET_BUTTON, INPUT_PULLDOWN);
  pinMode(TIME_DOWN_BUTTON, INPUT_PULLDOWN);
  pinMode(TIME_UP_BUTTON, INPUT_PULLDOWN);

  // Set button handlers
  timeDownButton->attachClick(handleTimeDown);
  timeUpButton->attachClick(handleTimeUp);
  stopButton->attachClick(handleMatchStop);
  resetButton->attachClick(handleMatchReset);
  assistResetButton.attachClick(handleAssistReset);
  victory1Button.attachClick(handleMatchVictory1);
  victory2Button.attachClick(handleMatchVictory2);
  playPauseButton.attachClick(handleMatchPlayPause);
  startButton.attachClick(handleMatchStart);

  Serial.println("DONE!");

  Serial.print("Initializing ESP NOW");
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  Serial.print(".");
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  Serial.print(".");

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Transmitted packet
  esp_now_register_send_cb(OnDataSent);
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);
  Serial.print(".");

  // Register broadcast address for ESP Now
  memcpy(peerInfo_Arcadian.peer_addr, broadcastAddress, 6);
  peerInfo_Arcadian.channel = 0;
  peerInfo_Arcadian.encrypt = false;
  if (esp_now_add_peer(&peerInfo_Arcadian) != ESP_OK)
  {
    Serial.println("Failed to add ESP now peer");
    return;
  }
  Serial.println(".SUCCESS!");

  Serial.print("Configuring sound board...");
  // Initialize soundboard and set volume
  soundboard.begin();
  soundboard.setVolume(30); // Range 0-30
  Serial.println("DONE!");

  // Initialize LED pins
  Serial.print("Configuring LED outputs...");
  pinMode(MATCH_START_LED, OUTPUT);
  pinMode(MATCH_STOP_LED, OUTPUT);
  pinMode(MATCH_RESET_LED, OUTPUT);
  pinMode(ASSIST_RESET_LED, OUTPUT);
  pinMode(VICTORY_T1_LED, OUTPUT);
  pinMode(VICTORY_T2_LED, OUTPUT);
  pinMode(TIME_DOWN_LED, OUTPUT);
  pinMode(TIME_UP_LED, OUTPUT);
  Serial.println("DONE!");

  lastDisplayTick = millis();

  Serial.println("Setup complete!");
}

void loop()
{
  // State controls
  // Change state from Arcadian has priority
  if (reqState || changeState)
  {
    if (changeState)
    {
      currentState = changeState;
      changeState = NO_STATE;
    }
    else if (reqState)
    {
      currentState = reqState;
      result = esp_now_send(broadcastAddress, (uint8_t *)&reqState, sizeof(reqState));
      if (result == ESP_OK)
      {
        // Serial.println("State Request Sent Successfully!");
      }
      else
      {
        Serial.println("Arcadian not responding! :(");
      }
    }
    // Clear request state
    reqState = NO_STATE;
    // Output state change to serial
    printStateChange(currentState);

    // Run state change actions
    runStateChangeActions(currentState);
  }
  else if (retrySend)
  {
    // Retry sending state to Arcadian
    Serial.println("Retrying Arcadian state update");
    result = esp_now_send(broadcastAddress, (uint8_t *)&currentState, sizeof(currentState));
    if (result == ESP_OK)
    {
      retrySend = false;
    }
    else
    {
      Serial.println("Arcadian STILL not responding! :(");
    }
  }

  // Button ticks
  // Active low, pull-up
  assistResetButton.tick();
  victory1Button.tick();
  victory2Button.tick();
  playPauseButton.tick();
  // Active high, pull-down
  bool stopButtonIsPressed = digitalRead(MATCH_STOP_BUTTON) == LOW;
  bool resetButtonIsPressed = digitalRead(MATCH_RESET_BUTTON) == LOW;
  bool timeDownButtonIsPressed = digitalRead(TIME_DOWN_BUTTON) == LOW;
  bool timeUpButtonIsPressed = digitalRead(TIME_UP_BUTTON) == LOW;
  stopButton->tick(stopButtonIsPressed);
  resetButton->tick(resetButtonIsPressed);
  timeDownButton->tick(timeDownButtonIsPressed);
  timeUpButton->tick(timeUpButtonIsPressed);
  // Active low, EXTERNAL pull-up
  startButton.tick();

  // All timer based actions in timers.ino
  // Get current time states
  currentTick = millis();

  // Flicker match start or assist button
  if (currentTick - lastButtonFlickerTick >= 500 && (currentState == READY_BOTH || currentState == ASSIST))
  {
    flickerLedState = !flickerLedState;
    if (currentState == READY_BOTH)
    {
      digitalWrite(MATCH_START_LED, flickerLedState);
    }
    else
    {
      digitalWrite(ASSIST_RESET_LED, flickerLedState);
    }
    lastButtonFlickerTick = currentTick;
  }

  // Update clock every 1 second during match
  if (currentTick - lastCountdownTick >= 1000 && (currentState == MATCH || currentState == ASSIST || currentState == PAUSED))
  {
    if (currentState == PAUSED)
    {
      flickerLedState = !flickerLedState;
      if (flickerLedState)
      {
        updateClock();
      }
      else
      {
        display.clearDisplay();
        display.display();
      }
    }
    else
    {
      seconds--;
      updateClock();
    }
    /* TODO
    This is the match clock. Make this better
    Since the time is set based on the current tick, clock drift can happen over the course of the match
    lastCountdownTick += 1000; is probably the solution to avoid clock drift
    */
    lastCountdownTick = currentTick;
  }
  else if (currentTick - lastCountdownTick >= 2000 && (currentState != PAUSED && currentState != MATCH_END))
  {
    display.clearDisplay();
    switch (currentDisplay)
    {
    case 0:
      display.drawBitmap(0, 0, cometrobotics, 128, 32, WHITE);
      break;
    case 1:
      display.drawBitmap(0, 0, utdesign, 128, 32, WHITE);
      break;
    default:
      display.drawBitmap(0, 0, techtitans, 128, 32, WHITE);
    }
    display.display();
    currentDisplay = (currentDisplay + 1) % 3;
    lastCountdownTick = currentTick;
  }
  else if (currentTick - lastCountdownTick >= 5000 && currentState == MATCH && resetSoundLevel)
  {
    soundboard.setVolume(30);
    resetSoundLevel = false;
  }

  // Handle tapout countdown
  if (currentTick - lastTapoutTick >= 5000 && (currentState == TAPOUT_T1 || currentState == TAPOUT_T2))
  {
    if (currentState == TAPOUT_T1)
    {
      reqState = VICTORY_T2;
    }
    else
    {
      reqState = VICTORY_T1;
    }
  }

  if (currentTick - lastReadyTick >= 1500 && (currentState == READY_BOTH))
  {
    if (firstReady)
    {
      soundboard.playSpecified(BOTH_TEAM_READY_SOUND);
      firstReady = NO_STATE;
    }
  }
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status == 0)
  {
    success = "Delivery Success :)";
    // reqState = NO_STATE;
  }
  else
  {
    success = "Delivery Fail :(";
  }
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  memcpy(&changeState, incomingData, sizeof(changeState));
  Serial.println("New state change received");
}

void updateClock()
{
  uint16_t minutes = seconds / 60;
  uint16_t actualSeconds = seconds % 60;
  display.clearDisplay();
  Serial.print("Clock set to ");
  Serial.print(minutes);
  Serial.print(":");
  if (actualSeconds < 10)
    Serial.print("0");
  Serial.println(actualSeconds);
  // Do actual clock update
  display.setCursor(18, 0);
  display.print(minutes);
  display.print(F(":"));
  if (actualSeconds < 10)
    display.print(F("0"));
  display.print(actualSeconds);
  display.display();
  if (currentState == MATCH || currentState == ASSIST)
  {
    if (seconds == 0)
    {
      reqState = MATCH_END;
    }
    else if (seconds == 6)
    {
      soundboard.playSpecified(END_SOUND);
    }
  }
  return;
}

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