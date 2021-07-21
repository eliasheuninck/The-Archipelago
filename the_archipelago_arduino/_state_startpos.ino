// Deprecated










// program flow: states
// 0 = idling
// 2 = ### start positions ###
// 3 = Synchronise, wait for video start
// 4 = play
// 5 = test

#include "Arduino.h"


// Go to the first position
void startPos() {
  if (state != previousState) { // one-trigger
    Serial.println("goto firstPos");

    //     LEDblink(1, 5); // blink the LED a few times to know it's alive
    motorsGoHome();

    for (byte i = 0; i < 12; i++) {
      motor[i].setMaxSpeed(4000);
      motor[i].setAcceleration(10000);
    }
    previousState = state;
  }

  for (byte i = 0; i < 12; i++) {
    motor_enable(i);
    powerLED(i, 0);
  }

  // move all motors to the first positions, 1 by 1
  for (currentKeyframe; currentKeyframe < 12; currentKeyframe++) {
    if (module[currentKeyframe][5] == 1) { // only move motors that are debug-enabled

      motor[keyframe[currentKeyframe][1]].moveTo(setMotorpos(keyframe[currentKeyframe][1], keyframe[currentKeyframe][2]));

      while (motor[keyframe[currentKeyframe][1]].run());
    }
  }

  // currentKeyframe is 12 now

  for (byte i = 0; i < 12; i++) {
    motor_disable(i);
  }

  delay(10);
  
/*
  // blink power LED
  for (byte i = 0; i < 12; i++) {
    powerLED(i, 1);
  }
  delay(40);
  for (byte i = 0; i < 12; i++) {
    powerLED(i, 0);
  }

  delay(500);
*/
  // program flow: states
  // 0 = idling
  // 2 = start positions
  // 3 = Synchronise, wait for video start
  // 4 = play
  // 5 = test
  state = 4; // switch to the PLAY state


  /*

     Initialize as all lights dark

      // motorDriver settings for going to the first position
      rail_01_MOTOR.setMaxSpeed(10000);
      // ...

      // CHANGE: adapt array for this project
      // Set motor speeds

      STAGE_1_MOTOR.setSpeed(keyframe[keyframeIndex][1]);
      STAGE_1_MOTOR.setSpeed(keyframe[keyframeIndex][3]);
      STAGE_1_MOTOR.setSpeed(keyframe[keyframeIndex][5]);
      STAGE_1_MOTOR.setSpeed(keyframe[keyframeIndex][7]);

      // set motor positions
      // CHANGE: adapt array for this project
      // read only the first line from the sequence file
      positions[0] = keyframe[keyframeIndex][0]; // keyframe index / value index (S1 speed or S3 pos, ...)
      positions[1] = keyframe[keyframeIndex][2]; // keyframe index / value index (S1 speed or S3 pos, ...)
      positions[2] = keyframe[keyframeIndex][4]; // keyframe index / value index (S1 speed or S3 pos, ...)
      positions[3] = keyframe[keyframeIndex][6]; // keyframe index / value index (S1 speed or S3 pos, ...)

      // CHANGE: no multistepper here
      OBSERVATORY.moveTo(positions); // set new positions for the mutistepper collection
  */


  /*
    arrivedFlagState = OBSERVATORY.run(); // run motors and report arrived state

    // Check arrivedFlagState when value changed
    if (arrivedFlagState != arrivedFlagLastState) {
      if (arrivedFlagState == 0) { // motors have arrived
        Serial.println("motors have arrived at starting positions");
        stopAllMotors();
        state = 0; // go to idle
      }
      arrivedFlagLastState = arrivedFlagState;
    }
  */
}
