// program flow: states
// 0 = ### idling ###
// 2 = start positions
// 3 = Synchronise, wait for video start
// 4 = play
// 5 = test

#include "Arduino.h"


void idling() { // state 0
  if (state != previousState) { // one-trigger
    Serial.println("Idling");

    /*
      delay(50);
      stopAllMotors();
      motorsDisable();
    */    
    previousState = state;
  }

  // Currently there's no way out

}
