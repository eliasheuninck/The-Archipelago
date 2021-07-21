// program flow: states
// 0 = idling
// 2 = start positions
// 3 = ### Synchronise, wait for video start ###
// 4 = play
// 5 = test

#include "Arduino.h"


void sync() { // state 3
  if (state != previousState) { // one-trigger
    Serial.println("Sync started");
    previousState = state;
  }
 // some code goes here
}
