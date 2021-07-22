// program flow: states
// 0 = play
// 1 = ### test ###

#include "Arduino.h"


void test() { // state 1
  if (state != previousState) { // one-trigger
    Serial.println("Test-state enabled");
    switch (hardwareTest_id) {
      case 0:
        Serial.println("Test: switches");
        stopAllMotors();
        break;
      case 1:
        Serial.println("Test: LEDs");
        // stopAllMotors();
        break;
      case 2:
        Serial.println("Test: motors (short motion)");
        //motorsEnable();
        /*
          for (byte i = 0; i < 12; i++) {
          motor[i].setMaxSpeed(4000);
          motor[i].setAcceleration(20000);
          // motor[i].moveTo(2000);
          }
        */
        break;
      case 3:
        Serial.println("Test: motors (full length motion)");
        motorsGoHome();
        /*
          for (byte i = 0; i < 12; i++) {
          motor[i].setMaxSpeed(2000);
          motor[i].setAcceleration(10000);
          motor_enable(i);
          // motor[i].moveTo(setMotorpos(i, 100)); // motor-number, position percentage (0-100)
          }
        */
        break;
      case 4:
        Serial.println("Test: all at once");
        // motorsEnable();
        for (byte i = 0; i < 12; i++) {
          motor[i].setMaxSpeed(4000);
          motor[i].setAcceleration(20000);
          motor[i].moveTo(2000);
        }

        // turn on the LEDS
        powerLEDS_on();
        break;
      case 5:
        Serial.println("Test: homing");
        motorsGoHome();
        break;
      case 6:
        Serial.println("Find rail length");
        Serial.println("Motor runs slowly when button pressed");

        motorsGoHome();

        // initialise motors
        for (byte i = 0; i < 12; i++) {
          motor[i].setMaxSpeed(16000);
          motor[i].setAcceleration(20000);
          motor[i].setCurrentPosition(0);
          motor[i].disableOutputs();

          Serial.print("Motor "); // print current position in motorsteps
          Serial.print(i); // print current position in motorsteps
          Serial.print(" current pos: "); // print current position in motorsteps
          Serial.println(motor[i].currentPosition()); // print current position in motorsteps
        }

        break;
    }
    previousState = state;
  }


  /*
    If test-state is enabled, which elements are tested:
    0: switches
    1: LED's
    2: motors
  */
  switch (hardwareTest_id) {
    case 0: // test switches
      moduleTest_switch();
      break;

    case 1: // test LEDs
      moduleTest_LED();
      delay(2000);
      break;

    case 2: // test motors (short movement)
      moduleTest_motor();
      break;

    case 3: // test motors (full rail length motion)
      moduleTest_railLength();
      break;

    case 4: // test all components at once
      moduleTest_full();
      break;

    case 5: // homing
      // all in 1-trigger section
      break;

    case 6: // Find rail length
      findRailLength();
      break;

    case 7: // Display I2C communication
      I2C_process_data();
      break;

  }
}
