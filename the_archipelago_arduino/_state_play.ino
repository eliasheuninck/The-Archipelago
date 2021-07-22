// program flow: states
// 0 = ### play ###
// 1 = test

#include "Arduino.h"


void play() { // state 0
  if (state != previousState) { // one-trigger
    Serial.println();
    Serial.println("PLAY");
    delay(10);


    // set high acceleration params to all motors
    for (byte i = 0; i < 12; i++) {
      motor[i].setMaxSpeed(4000);
      motor[i].setAcceleration(160000); // previous value: 40000
    }

    // set up the starting configuration in 'state_startpos'. LED's all off.
    powerLEDS_off();

    motorsGoHome();
    goToStartPos();
    delay(20);

    // default: 12
    currentKeyframe = 12; // the first keyframe after the first-position initialisation keyframes
    Serial.println();
    Serial.print("START PLAY WITH KF: ");
    Serial.println(currentKeyframe);

    // countdown();

    // simulating the video time (when working without a raspberry pi conencted)
    //    fake_time_millis_at_start = millis();
    previousState = state;
  }

  I2C_process_data(); // check for received I2C data

  // simulating the video time (when working without a raspberry pi conencted)
  //  unsigned long videoTime = millis() - fake_time_millis_at_start;
  //  Serial.println(videoTime);

  // If we are in a valid keyframe range: perform actions:
  if (currentKeyframe < total_keyframes) {

    // skip over keyframes that have a disabled motor (used for debugging)
    if (module[keyframe[currentKeyframe][1]][5] == 0) {
      Serial.print("skip kf: ");
      Serial.println(currentKeyframe);
      currentKeyframe++;
    }

    // Keyframes with a module that has the motor enabled
    else {
      if (videoTime >= frameToMs(keyframe[currentKeyframe][0])) {

        // prepare variables
        int kf_timestamp = (frameToMs(keyframe[currentKeyframe][0]));
        // Serial.println();
        // Serial.print("Keyframe: ");
        // Serial.println(currentKeyframe);

        int kf_module = keyframe[currentKeyframe][1];
        int kf_targetLoc = keyframe[currentKeyframe][2];
        int kf_led = keyframe[currentKeyframe][4];

        // set LED
        powerLED(kf_module, kf_led);

        if (kf_targetLoc >= 0) { // if this keyframe has a motor move

          // prep variables
          motor_enable(kf_module);
          int kf_targetTime = (frameToMs(keyframe[currentKeyframe][3]));

          // percent to motor steps --> total motor steps * (percent / 100)
          int kf_targetLoc_steps = setMotorpos(kf_module, kf_targetLoc);


          // Serial.println("motor info: ");
          // Serial.print("current loc: ");
          // Serial.println(motor[kf_module].currentPosition());
          // Serial.print("target loc: ");
          // Serial.println(kf_targetLoc_steps);
          // Serial.print("maxSpeed: ");
          // Serial.println(motor[kf_module].maxSpeed());
          // Serial.print("Speed: ");
          // Serial.println(motor[kf_module].speed());


          // DISABLED FOR LESS SERIAL OVERLOAD -- DEBUGGING
          // Serial.print("- motor module: ");
          // Serial.println(kf_module);


          // calc speed
          // calc speed by seeing what distance need to be traveled in which time.
          // Then calculate the distance per second (in motor steps) for the setMaxSpeed() function.
          int deltaLoc = motor[kf_module].currentPosition() - kf_targetLoc_steps; // the ∆ distance in motor steps

          //          Serial.print("- current pos (steps): ");
          //          Serial.print(motor[kf_module].currentPosition());
          //          Serial.print(" MIN target pos (steps): ");
          //          Serial.print(kf_targetLoc_steps);
          //          Serial.print(" = distance: ");
          //          Serial.println(deltaLoc);


          int deltaTime = kf_targetTime - videoTime; // duration of the motor move (in ms)

          //          Serial.print("- target time (ms): ");
          //          Serial.print(kf_targetTime);
          //          Serial.print(" MIN videoTime (ms): ");
          //          Serial.print(videoTime);
          //          Serial.print(" = duration: ");
          //          Serial.println(deltaTime);


          float SpeedLimit = abs(deltaLoc) / (deltaTime * 0.001); // distance in steps / duration in seconds --> steps per second

          //          Serial.print("- distance (steps): ");
          //          Serial.print(abs(deltaLoc));
          //          Serial.print(" DIVIDED BY duration (sec): ");
          //          Serial.print((deltaTime * 0.001));
          //          Serial.print(" = SpeedLimit (steps per second) (float): ");
          //          Serial.println(SpeedLimit);


          motor[kf_module].setMaxSpeed(SpeedLimit);

          // DISABLED FOR LESS SERIAL OVERLOAD -- DEBUGGING
          // Serial.print("maxspeed: ");
          // Serial.println(SpeedLimit);


          // Serial.print(" ");
          motor[kf_module].moveTo(kf_targetLoc_steps);
          module[kf_module][7] = 1; // flag this motor so it'll be polled later

        }


        Serial.println();
        Serial.print("KF_");
        Serial.println(currentKeyframe);
        Serial.print("— fr: ");
        Serial.println(keyframe[currentKeyframe][0]);
        Serial.print("— ms: ");
        Serial.println(frameToMs(keyframe[currentKeyframe][0]));
        Serial.println();

        currentKeyframe++;


      }

      // poll all flagged motors
      for (byte i = 0; i < 12; i++) {
        //        I2C_process_data(); // check for received I2C data
        if (module[i][7] == 1) {
          motor[i].run();
          if (motor[i].distanceToGo() == 0) { // when a motor ended its run
            motor_disable(i); // disable power
            module[i][7] = 0; // clear the poll flag, so it won't take up time being polled

            // DISABLED FOR LESS SERIAL OVERLOAD -- DEBUGGING
            // Serial.print("- motor ");
            // Serial.print(i);
            // Serial.println(" arrived");
          }
        }
      }



    }

  }


  else { // the end
    Serial.println("finish any motor moves that are still happening");
    bool stillRunning = 1;
    while (stillRunning == 1) {

      // poll all flagged motors
      for (byte i = 0; i < 12; i++) {
        if (module[i][7] == 1) {
          motor[i].run();
          if (motor[i].distanceToGo() == 0) { // when a motor ended it's run
            motor_disable(i); // disable power
            module[i][7] = 0; // clear the poll flag, so it won't take up time being polled
            Serial.print("- motor ");
            Serial.print(i);
            Serial.println(" arrived");
          }
        }
      }

      // re-check if there are still motor-poll-flags set still
      stillRunning = 0;
      for (byte i = 0; i < 12; i++) {
        if (module[i][7] == 1) {
          stillRunning = 1;
        }
      }
    }

    Serial.println();
    Serial.print("The end, at keyframe ");
    Serial.println(currentKeyframe);
    // motors are all at zero now (as configured in the keyframes)

    motorsGoHome();
    goToStartPos();

    // disable motors
    for (byte i = 0; i < 12; i++) {
      motor_disable(i);
    }

    Serial.println("Detect loop point");
    // Detect loop point
    bool looped = false;
    while (looped == false) {
      I2C_process_data(); // check for received I2C data
      if (videoTime < 500) {
        // reset currentKeyframe to 12 in order to be ready for the next loop
        currentKeyframe = 12;
        looped = true;
      }
    }


    //    while (true) {
    //      //      Serial.println("PROGRAM STOPPED");
    //      //      delay(1000); // TODO: now the arduino just waits after a single run. It should reset and be ready to receive the timestamp-stream from Rpi
    //      // reset currentKeyframe to 12
    //
    //      // goto startpos
    //    }
  } // close else (the end)
}
