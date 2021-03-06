#include "Arduino.h"

// program flow: states
// 0 = play
// 1 = test
void statePoll() {
  if (state == 0) play();
  else if (state == 1) test();
}


void stopAllMotors() {
  for (byte i = 0; i < 12; i++) {
    if (i != 5) { // skip module 5 at connector 4 (only LED)
      motor[i].stop();
    }
  }
  // Serial.println("All motors stopped");
}

void setupInternalLED() {
  pinMode(LED_BUILTIN, OUTPUT);
  delay(10);

  // blink the built-in LED on the teensy a few times on startup
  internalLEDflash_n(5);
}

void internalLED( byte state ) {
  if (state == 0 ) {
    digitalWrite(LED_BUILTIN, LOW);   // LED OFF
  }
  if (state == 1) {
    digitalWrite(LED_BUILTIN, HIGH);   // LED ON
  }
}

void internalLEDflash() {
  internalLED(1);
  delay(30);
  internalLED(0);
}

void internalLEDflash_n(byte times) {
  for (byte i = 0; i < times; i++) {
    internalLEDflash();
    delay(90);
  }
}


void setupPowerLEDS() {
  for (byte i = 0; i < 12; i++) {
    pinMode(module[i][0], OUTPUT);
  }
}

void setupLimitSwitches() {
  for (byte i = 0; i < 12; i++) {
    if (i != 5) { // skip module 5 at connector 4 (only LED)
      pinMode(module[i][1], INPUT_PULLUP); // Setup button with internal pull-up
      limitSwitch_debouncer[i].attach(module[i][1]); // After setting up the button, setup the Bounce instance
      limitSwitch_debouncer[i].interval(5); // interval in ms
    }
  }
}


void setupMotors() {
  for (byte i = 0; i < 12; i++) {
    if (i != 5) { // skip module 5 at connector 4 (only LED)
      // pinMode(module[i][2], OUTPUT); // motor enable pin
      motor[i].setEnablePin(module[i][2]);

      // Reverse direction for normal operation
      // Except module 5: the glider of WO_04_rail_1 grips the PU belt on the other side. Do not reverse the motion here.
      // true = inverted
      // false = non-inverted
      // https://www.airspayce.com/mikem/arduino/AccelStepper/classAccelStepper.html#ac62cae590c2f9c303519a3a1c4adc8ab
      motor[i].setPinsInverted(true, false, true); // directionInvert, stepInvert, enableInvert

      if (i == 4) {
        motor[i].setPinsInverted(false, false, true); // directionInvert, stepInvert, enableInvert
      }

      pinMode(module[i][3], OUTPUT); // step pin
      pinMode(module[i][4], OUTPUT); // dir pin

      // Configure each stepper motor
      motor[i].setMaxSpeed(1000);
      motor[i].setAcceleration(10000);
    }
  }
}

void motor_enable (byte moduleID) {
  if (moduleID != 5) { // skip module 5 at connector 4 (only LED)
    motor[moduleID].enableOutputs();
  }
}

void motor_disable (byte moduleID) {
  if (moduleID != 5) { // skip module 5 at connector 4 (only LED)
    motor[moduleID].disableOutputs();
  }
}

void motorsGoHome() {
  // this function is run once to home all motors
  Serial.println("Homing motors");
  // Homing states
  //  0: not homing
  //  1: clearing switches (prevents being trapped behind limit switch)
  //  2: approaching limit switch
  //  3: limit switch is pressed, reverse stages slowly
  //  4: limit switch is no longer pressed, homing is complete
  byte homing_state = 0;
  byte previous_homing_state = 255; // don't change this. (255 is the highest valueof a byte. I use it as initializing value to be out of range)

  int homing_clearing_distance = 400; // amount of steps necessary to move from hard-stop out of limit-switch-click

  // initialise motor_homed variable
  bool motor_homed[12];
  for (byte i = 0; i < 12; i++) {
    motor_homed[i] = false;
  }

  for (byte i = 0; i < 12; i++) {
    if (i != 5) { // skip module 5 at connector 4 (only LED)
      motor_disable(i); // disable all motors
    }
  }

  // run one motor a time
  for (byte i = 0; i < 12; i++) {
    if (i != 5) { // skip module 5 at connector 4 (only LED)
      if (module[i][5] == 1) { // only home motors that are debug-enabled
        while (motor_homed[i] == false) {

          // initialise homing
          if (homing_state == 0) {
            if (homing_state != previous_homing_state) { // one-trigger
              Serial.print("- module ");
              Serial.print(i);
              // Serial.println(": initialising");
              previous_homing_state = homing_state;
            }
            // blink power LED
            //            powerLED(i, 1);
            //            delay(40);
            //            powerLED(i, 0);

            motor_enable(i);

            stopAllMotors(); // make sure all motion is stopped before starting to home

            // set all stages in the approaching state to start the homing
            //  0: not homing
            //  1: clearing switches (prevents being trapped behind limit switch)
            //  2: approaching limit switch
            //  3: limit switch is pressed, reverse stages slowly
            //  4: limit switch is no longer pressed, homing is complete
            homing_state = 1;
          }

          // Homing state 1: clearing switches (prevents being trapped behind limit switch)
          else if (homing_state == 1) {
            if (homing_state != previous_homing_state) { // one-trigger
              /*
              /Serial.print("homing module ");
              Serial.print(i);
              Serial.println(": clearing switch");
              */
              previous_homing_state = homing_state;
            }
            // relative move away from switch
            motor[i].setCurrentPosition(0); // set current position as zero
            motor[i].setAcceleration(50000);
            motor[i].setMaxSpeed(2000);
            motor[i].setSpeed(2000); // turn away from the switch
            motor[i].moveTo(homing_clearing_distance);

            while (motor[i].distanceToGo() > 0) {
              motor[i].run();
            }

            // Serial.println("motor[0] cleared the switch");
            motor[i].setSpeed(-1000); // set speed to turn motor toward switch at moderate speed
            homing_state = 2;
          }

          // when the motor is approaching the switch
          else if (homing_state == 2) {
            if (homing_state != previous_homing_state) { // one-trigger
              /*
              Serial.print("homing module ");
              Serial.print(i);
              Serial.println(": approaching switch");
              */
              previous_homing_state = homing_state;
            }
            limitSwitch_debouncer[i].update(); // Update the Bounce instances
            delayMicroseconds(10); // give a little bit of time for the debouncer (less erroneous behaviour
              motor[i].runSpeed();

              if ( limitSwitch_debouncer[i].rose() ) { // when button is pressed
                motor[i].setMaxSpeed(12000);
                motor[i].setSpeed(500); // change direction to turn away from the switch and run slowly until the button isn't pressed anymore
                homing_state = 3; // button is pressed
              }
            }

            // limit switch is pressed, motor is reversing slowly
            else if (homing_state == 3) {
              if (homing_state != previous_homing_state) { // one-trigger
                /*
                Serial.print("homing module ");
                Serial.print(i);
                Serial.println(": moving away from switch");
                */
                previous_homing_state = homing_state;
              }
              motor[i].runSpeed();

              limitSwitch_debouncer[i].update(); // Update the Bounce instances
              delayMicroseconds(500); // wait a little bit for debouncer
              if ( limitSwitch_debouncer[i].fell() ) { // when button is not pressed anymore
                motor[i].setSpeed(0); // stop moving
                delay(20);

                // move the zero-point away from the switch
                // to avoid clicking the switch when returning to zero.
                motor[i].setCurrentPosition(0); // set current position as zero
                motor[i].moveTo(250);
                // 80 motor steps will have been subtracted from each rail length value to compensate.

                while (motor[i].distanceToGo() > 0) {
                  motor[i].run();
                }
                delay(20);
                motor[i].setCurrentPosition(0); // set current position as zero

                homing_state = 4; //  limit switch is no longer pressed, homing is complete
              }
            }

            else if (homing_state == 4) {
              if (homing_state != previous_homing_state) { // one-trigger
                // Serial.print("homing module ");
                // Serial.print(i);
                // Serial.println(": complete");
                Serial.println(" - complete");
                previous_homing_state = homing_state;
              }

              internalLEDflash_n(1); // LED on when homed (for debugging)
              motor_homed[i] = true;

              // blink power LED
              //            powerLED(i, 1);
              //            delay(40);
              //            powerLED(i, 0);

              motor_disable(i);
              homing_state = 0;
            }
          }
        }
      } // close: if (i != 5)
    } // close for
    Serial.println("All motors are home");
  }

  long setMotorpos (int moduleID, int pos) { // pos: position on the rail (in percent)

    // motor position = rail length in steps * Position percentage as a decimal value
    long motorposition = module[moduleID][6] * ((float)pos * 0.01);
    /*
    Serial.println("set motor position: ");
    Serial.print("- full rail length: ");
    Serial.println(module[moduleID][6]);
    Serial.print("- position in percent: ");
    Serial.println(pos);
    Serial.print("- position in steps: ");
    Serial.println(motorposition);
    */
    return motorposition;
  }


  void moduleTest_switch () {
    for (byte i = 0; i < 12; i++) {
      limitSwitch_debouncer[i].update();
      delayMicroseconds(10); // give a little bit of time for the debouncer (less erroneous behaviour
        if ( limitSwitch_debouncer[i].rose() ) { // when button is pressed

          // Flash the power LED of same rail too
          powerLED(i, 1);
          delay(40);
          powerLED(i, 0);

          internalLEDflash(); // Flash the internal LED
        }
      }
    }

    void moduleTest_LED () {
      // Blink all LED's one after another
      for (byte j = 0; j < 12; j++) { // one full round
        /*
        for (byte i = 0; i < 12; i++) { // turn all LEDS off
        powerLED(i, 0);
      }
      */
      powerLED(j, 1); // turn on one LED a time
      delay(40);
      powerLED(j, 0); // turn off LED
    }
  }

  void moduleTest_motor () {
    int motorDistance = 4000;
    byte motorRunCount = 2; // run n times back and forth the distance per rail
    byte currentCount = 0;

    // disable all motors
    for (byte i = 0; i < 12; i++) {
      if (i != 5) { // skip module 5 at connector 4 (only LED)
        motor[i].disableOutputs();
      }
    }

    // run one motor a time
    for (byte i = 0; i < 12; i++) {
      if (i != 5) { // skip module 5 at connector 4 (only LED)

        // check if 'i' yields a motor that is enabled
        // if not, just skip over this run in the for loop
        if (module[i][5] == 1) {

          // initialise motor
          motor[i].setMaxSpeed(4000);
          motor[i].setAcceleration(20000);
          motor[i].setCurrentPosition(0);
          motor[i].moveTo(motorDistance);
          motor[i].enableOutputs();

          while (true) {
            motor[i].run();
            if (motor[i].distanceToGo() == 0) {
              if (motor[i].currentPosition() == motorDistance) {
                motor[i].moveTo(0);
              }
              if (motor[i].currentPosition() == 0) {
                currentCount ++;
                if (currentCount == motorRunCount) {
                  motor[i].disableOutputs();
                  currentCount = 0;
                  break;
                }
                motor[i].moveTo(motorDistance);
              }
            }
          } // close while
        } // close if (module[i][5] == 1)
      } // close if (i != 5)
    } // close for
  } // close moduleTest_motor


  // Test all components
  // motor and LED on, when button pressed
  void moduleTest_full () {

    int activeModule = -1;
    int motorDistance = 2000;

    // initialise motors
    for (byte i = 0; i < 12; i++) {
      if (i != 5) { // skip module 5 at connector 4 (only LED)
        motor[i].setMaxSpeed(16000);
        motor[i].setAcceleration(20000);
        motor[i].setCurrentPosition(0);
        motor[i].moveTo(motorDistance);
        motor[i].disableOutputs();
      }
    }

    // initialise leds
    for (byte i = 0; i < 12; i++) {
      powerLED(i, 0); // turn all LEDS off
    }

    moduleTest_LED(); // flash all LEDs (useful for module 5 - connector 4)

    // update switches
    while (activeModule == -1) {
      for (byte i = 0; i < 12; i++) {
        limitSwitch_debouncer[i].update(); // Update the Bounce instances
        delayMicroseconds(10); // give a little bit of time for the debouncer (less erroneous behaviour)

        if ( limitSwitch_debouncer[i].rose() ) { // when button is pressed
          activeModule = i; // turn on LED
          motor[i].enableOutputs(); // energize selected motor
          powerLED(activeModule, 1);
          break;
        }
      }
    }

    // when a module is active
    while (activeModule > -1) {

      // run motor back and forth
      if (motor[activeModule].distanceToGo() == 0)
      motor[activeModule].moveTo(-motor[activeModule].currentPosition());

      motor[activeModule].run();

      limitSwitch_debouncer[activeModule].update(); // Update the Bounce instances
      delayMicroseconds(10); // give a little bit of time for the debouncer (less erroneous behaviour)
      if ( limitSwitch_debouncer[activeModule].fell() ) { // when button is released
        powerLED(activeModule, 0); // turn off LED
        motor[activeModule].disableOutputs();
        activeModule = -1; // no active module
      }
    }
  }















  // Motor test - full rail length
  // run motor when button pressed
  void moduleTest_railLength () {

    int activeModule = -1;
    unsigned long startMillis = 0;
    unsigned long endMillis = 0;

    // initialise motors
    for (byte i = 0; i < 12; i++) {

      // careful speeds
      motor[i].setMaxSpeed(10000);
      motor[i].setAcceleration(20000);

      // fast speeds
      //    motor[i].setMaxSpeed(32768); // fastest possible speed on the duo
      //    motor[i].setAcceleration(2097152); // fastest possible acceleration on the duo

      motor[i].disableOutputs();
    }

    // update switches
    while (activeModule == -1) {
      for (byte i = 0; i < 12; i++) {
        limitSwitch_debouncer[i].update(); // Update the Bounce instances
        delayMicroseconds(10); // give a little bit of time for the debouncer (less erroneous behaviour)

        if ( limitSwitch_debouncer[i].rose() ) { // when button is pressed
          activeModule = i;
          motor[i].enableOutputs(); // energize selected motor
          startMillis = millis();
          break;
        }
      }
    }

    // when a module is active
    while (activeModule > -1) {

      // check if 'i' yields a motor that is enabled
      // if not, just skip over this run in the for loop
      if (module[activeModule][5] == 1) {
        if (motor[activeModule].distanceToGo() == 0) {
          if (motor[activeModule].currentPosition() == 0) {
            motor[activeModule].moveTo(setMotorpos(activeModule, 100)); // motor-number, position percentage (0-100)

            // timer
            Serial.print((millis() - startMillis) * 0.001);
            Serial.println("s");
            startMillis = millis();
          }
          else {
            motor[activeModule].moveTo(setMotorpos(activeModule, 0)); // motor-number, position percentage (0-100)

            // timer
            Serial.print((millis() - startMillis) * 0.001);
            Serial.println("s");
            startMillis = millis();
          }
        }
        motor[activeModule].run();

        // check for button release
        limitSwitch_debouncer[activeModule].update(); // Update the Bounce instances
        delayMicroseconds(10); // give a little bit of time for the debouncer (less erroneous behaviour)
        if ( limitSwitch_debouncer[activeModule].fell() ) { // when button is released
          motor[activeModule].disableOutputs();
          activeModule = -1; // no active module
        }
      }
    }
  }






















  // ############################################


  // Find rail length
  void findRailLength () {
    int activeModule = -1;

    // update switches
    while (activeModule == -1) {
      for (byte i = 0; i < 12; i++) {
        limitSwitch_debouncer[i].update(); // Update the Bounce instances
        delayMicroseconds(10); // give a little bit of time for the debouncer (less erroneous behaviour)

        if ( limitSwitch_debouncer[i].rose() ) { // when button is pressed
          activeModule = i; // set variable to module number of active rail
          motor[i].enableOutputs(); // energize selected motor
          break;
        }
      }
    }

    motor[activeModule].setSpeed(600);

    // when a module is active
    while (activeModule > -1) {
      motor[activeModule].runSpeed();   // run motor slowly forwards

      limitSwitch_debouncer[activeModule].update(); // Update the Bounce instances
      delayMicroseconds(50); // give a little bit of time for the debouncer (less erroneous behaviour)
      if ( limitSwitch_debouncer[activeModule].fell() ) { // when button is released
        motor[activeModule].disableOutputs();

        Serial.print("Motor "); // print current position in motorsteps
        Serial.print(activeModule); // print current position in motorsteps
        Serial.print(": "); // print current position in motorsteps
        Serial.println(motor[activeModule].currentPosition()); // print current position in motorsteps

        activeModule = -1; // no active module
      }
    }
  }


  // ############################################











  // use the LED to indicate 3 seconds, so I can start the video more or less in sync (just for testing)
  void countdown() {
    int shortDelay = 100;
    int longDelay = 1000;

    // 1
    LEDflash(1);
    delay(longDelay);

    // 2
    LEDblink(1, 2);
    delay(longDelay);

    // 3
    LEDblink(1, 3);
    delay(longDelay);

    // flash to press 'start' (fourth second)
    LEDflash(1);
  }


  void LEDblink(byte moduleID, byte repetitions) {
    for (byte i = 0; i < repetitions; i++) {
      LEDflash(moduleID);
      delay(80);
    }
  }

  void LEDflash(byte moduleID) {
    powerLED(moduleID, 1);
    delay(5);
    powerLED(moduleID, 0);
  }

  // for debugging purposes
  void LEDflashFast(byte moduleID) {
    powerLED(moduleID, 1);
    powerLED(moduleID, 0);
  }

  void powerLED(byte moduleID, bool state) {
    if (state == true)
    digitalWrite(module[moduleID][0], HIGH);
    else
    digitalWrite(module[moduleID][0], LOW);
  }

  void powerLEDS_off() {
    for (byte i = 0; i < 12; i++) {
      digitalWrite(module[i][0], LOW);
    }
  }

  void powerLEDS_on() {
    for (byte i = 0; i < 12; i++) {
      digitalWrite(module[i][0], HIGH);
    }
  }


  int frameToMs(int frame) {
    return (frame * 40);
  }



  // All motors assume the position of the first keyframe
  void goToStartPos() {
    Serial.println();
    Serial.println("Move all motors to first pos");

    powerLEDS_off();

    // initialise motors
    for (byte i = 0; i < 12; i++) {
      if (i != 5) { // skip module 5 at connector 4 (only LED)
        motor[i].setMaxSpeed(16000);
        motor[i].setAcceleration(40000);
      }
    }

    for (byte i = 0; i < 12; i++) {
      if (i != 5) { // skip module 5 at connector 4 (only LED)

        Serial.print("Checking motor ");
        Serial.println(i);

        // check if 'i' yields a motor that is enabled
        // if not, just skip over this run in the for loop
        if (module[i][5] == 1) {
          if (keyframe[i][2] > 0) { // only run if target pos is larger than zero

            Serial.println("??? moving to start pos");

            motor_enable(i);
            delay(20);

            motor[i].moveTo(setMotorpos(i, keyframe[i][2])); // motor-number, position percentage (0-100)

            while (motor[i].distanceToGo() > 0) {
              motor[i].run();
            }

            if (motor[i].distanceToGo() == 0) {
              motor_disable(i);
              internalLEDflash();
              delay(20);
              // Serial.print("??? motor ");
              // Serial.print(i);
              Serial.println("??? has arrived");
            }
          }
          else {
            Serial.println("??? is already in position");
          }
        }
        else {
          Serial.println("??? is disabled");
        }
      }
    }
    Serial.println("All motors arrived at start positions");
    Serial.println("Waiting for the video playhead-time...");
  }



  // I2C: receive number from raspberry pi
  // Function is called by the onReceive event (interrupt)
  // TODO: test with byte instead of int
  void receiveEvents(int numBytes) {
    // Serial.print("-> ");
    // Serial.println(numBytes);

    if (numBytes >= 2) {

      // Serial.print("."); // for debugging
      // set the numBytes value to a global variable
      // minus-one because of the unwanted leading zero
      I2C_numberOfDigits = numBytes - 1;

      // dump the first byte since it's a prepended zero we don't want.
      // The prepended zero is the I2C register I think (not the I2C address).
      // after every read, the current byte disappears and the next becomes available
      // Index 0 will be overwritten in the next step
      I2C_byteBuffer[0] = Wire.read();

      // put the incoming bytes in a byte-array
      for (int i = 0; i < I2C_numberOfDigits; i++) {
        I2C_byteBuffer[i] = Wire.read(); // TODO: test with a byte instead of int
      }

      I2C_dataReceived = true;

    } // close if (numBytes >= 2)
  } // close receiveEvents

  // I2C: When a full array has been received, process it and put it in a global integer variable
  void I2C_process_data() {
    if (I2C_dataReceived == true) {
      videoTime_assemble = 0; // reset the value to zero

      //    Serial.println();
      //    Serial.print("prevT: ");
      //    Serial.println(previousVideoTime);
      //    Serial.print("digits: ");
      //    Serial.println(I2C_numberOfDigits);

      // add the separate bytes to a single integer
      for (byte i = 0; i < I2C_numberOfDigits; i++) {

        // cast the byte array to int with (int)
        // add one digit a time to the single integer value
        // by multiplying it's value by 10 every time a new one gets added
        videoTime_assemble = videoTime_assemble * 10 + (int)I2C_byteBuffer[i];
      }
      //    Serial.print("MS: ");
      //    Serial.println(videoTime_assemble);

      if (looping == false) {
        I2C_verify_data();
      } else {
        I2C_detect_loop();
      }

      // reset the flag
      I2C_dataReceived = false;
    } // close if
  } // close I2C_process_data()


  void I2C_verify_data() {
    videoTime = 0; // reset the value to zero

    // ONLY assign when number is in a reasonable scale (within video duration)
    if (videoTime_assemble <= vid_duration) {

      // TODO: AlLOW FOR ROLLOVER TO ZERO AFTER THE END

      if (videoTime_assemble > previousVideoTime) {

        //        Serial.print("previousVideoTime + I2C_skip = ");
        //        Serial.println(previousVideoTime + I2C_skip);

        if (videoTime_assemble <= (previousVideoTime + I2C_skip)) { // be able to skip about 2 frames (2x 40 ms)

          // videoTime is now verified and trusted for use
          videoTime = videoTime_assemble; // only assign the value to videoTime when it's a bit bigger than the previous frame.

          previousVideoTime = videoTime;
          // Print out the integer (for debugging)
          //    Serial.print("from pi: ");
          Serial.println(videoTime);
        } else {

          Serial.println();
          Serial.print("ERR: curr_t jumped (>= ");
          Serial.print(I2C_skip);
          Serial.print("ms): prev_t: ");
          Serial.print(previousVideoTime);
          Serial.print(" curr_t: ");
          Serial.print(videoTime_assemble);
          Serial.print(". Diff: ");
          Serial.print(videoTime_assemble - previousVideoTime);
          Serial.println("ms");
          Serial.println("assigned prev_t to curr_t");
          Serial.println();

          videoTime = previousVideoTime;
        }
      } else {

        Serial.println();
        Serial.print("ERR: curr_t: ");
        Serial.print(videoTime_assemble);
        Serial.print(" < prev_t: ");
        Serial.print(previousVideoTime);
        Serial.print(". Diff: ");
        Serial.print(previousVideoTime - videoTime_assemble);
        Serial.println("ms");
        Serial.println("assigned prev_t to curr_t");
        Serial.println();

        videoTime = previousVideoTime;
      }
    } else {
      Serial.println();
      Serial.print("ERR: curr_t out of bounds (> ");
      Serial.print(vid_duration);
      Serial.println("ms)");
      Serial.println("assigned prev_t to curr_t");
      Serial.println();

      videoTime = previousVideoTime;
    }
  }

  void I2C_detect_loop() {

    // look at videoTime_assemble and find out when it rolls back to the beginning

    // Fill buffer with values using a global counter
    loopBuffer[loopIndex] = videoTime_assemble;
    loopIndex++;

    // if loopBuffer is 1 over max:
    if (loopIndex == 6) {
      // test the array

      // set looped var false when encountering any false value
      bool inrange = true;
      bool incrementing = true;
      bool looped = false;

      // Test: are the values in range (below 10 seconds)?
      for (byte i = 0; i < 6; i++) {
        if (loopBuffer[i] > vid_lead_in) {
          inrange = false;
        }
      }
      if (inrange == false) {
        loopIndex = 0;
      }
      if (inrange == true) {
        Serial.println("Values in range");


        // Test: is each entry larger than the previous one?
        for (byte i = 0; i < 5; i++) {
          if (loopBuffer[i] > loopBuffer[i + 1]) {
            incrementing = false;
          }
        }
        if (incrementing == false) {
          loopIndex = 0;
        }
        if (incrementing == true) {
          Serial.println("Values are incrementing");

          Serial.println("Both tests OK. Looped = TRUE");
          looped = true;
        }
      }

      if (looped == true) {
        Serial.println("Looped");
        Serial.println("tested values for loop point:");

        // print array
        for (byte i = 0; i < 6; i++) {
          Serial.println(loopBuffer[i]);
        }
        Serial.println();

        // reset variables in order to be ready for the next loop
        looping = false;
        loopIndex = 0;

        currentKeyframe = 12;

        videoTime = videoTime_assemble;
        previousVideoTime = videoTime_assemble;

        I2C_verify_data();
        Serial.print("Loop point detected: curr_t: ");
        Serial.println(videoTime);

        previousVideoTime = videoTime;
        Serial.println("Assign curr_t to prev_t");
      }
    }
  }
