/*
  The Archipelago

  (Github initial commit. Equivalent to WO 10)

  CONFIGURE BEFORE RUNNING
  - Set Module[][5] motor debug enable according to which rails are attached
  - Set the state: test or play

  UPLOADING
  - Upload sketch to Arduino DUE's native USB port (the one furthest away from the power jack)
  - Servial communication via Arduino DUE's programming port (the one closest to the power jack)
  - set 'Board' and 'Port' accordingly in the IDE before uploading.





  CURRENT STATE
  - 18/01/2021: keyframe system works

  TODO
  [v] make testing state where it's easy to test functions via serial print. Easy to isolate problems.
  [v] Test motor-off on a vertical rail. Test also in higher & lower room temperatures.
  [v] - Info entry system
        - An array of shot-change frames + system that does time offsets (frames * millisec per frame)
        - An array that holds the data at which shot the LED becomes active. This way a max patch might not be necessary.
        - An array that holds start positions, end positions and durations.
        - A system that can deal with moving shots and stills.
        - How to define speed? Check manual. Is (startpos, endpos, duration) possible?
          --> Just use runSpeed() (without an end position). The end will be marked by the transition to the next shot
  - Initialisation procedure
    - Arduino and rPi connected startup? For example:
      - 1. start up Arduino
        - Homing
        - Go to starting mositions
        - Starts listening for the video-start-mark (starts the regular sequence from then on).
      - 2. startup rPi (which will start looping the video).
      - 3. If homing is still happening when video already started, that's ok.
           Arduino should wait until it receives the very beginning of the video-time
           when the gliders are in their starting positions
    - Auto homing
  [v] - Make a LED function that takes: (rail number, on-off), and does the switching of the LED driver and transistors.


  CIRCUIT:
  1x rPi
  1x Arduino Due + custom motor & LED shield
  11x TMC 2280 stepper motor drivers
  11 motors
  11 limit switches
  12 LEDs


  COMMUNICATION:
  rPi streams video-time over I2C
  Arduino listens and syncs

  credits:
  - homing procedure: https://www.brainy-bits.com/setting-stepper-motors-home-position-using-accelstepper/
  - button debouncer: Bounce2 by Thomas Fredericks: https://github.com/thomasfredericks/Bounce2
  - stepper motor library: Accelstepper
*/

#include <ArduinoTrace.h>
// TRACE();
// DUMP(value);
// BREAK();


// ****************
// USER VARIABLES
// ****************

// program flow: states
// 0 = play
// 1 = test / configure
byte state = 0;
byte previousState = 255; // don't change this. (255 is the highest valueof a byte. I use it as initializing value to be out of range)


//  If test-state is enabled, select which elements are tested or configured:
//  0: switches
//  1: LED's
//  2: motors (short movement --- manually move gliders to the middle of the rail first!)
//  3: motors (full rail length motion)
//  4: all at once (motor and LED on, when button pressed)
//  5: homing
//  6: find rail length by pressing the motor switch (terminal necessary)
//  7: receive I2C communication
const byte hardwareTest_id = 4;


// Video
const int vid_duration = 730000; // ms (12 minutes)
const int vid_lead_in = 10000; // ms. Amount of black slug in the beginning of the video


int total_keyframes = 255;  // auto-calculate the number of keyframes.
// Don't change this. (255 is the highest valueof a byte. I use it as initializing value to be out of range)
byte currentKeyframe = 0; // don't change this

// This is where the sequence is written
// no new motor position when target loc is a negative number
// int type is large enough for frame timestamp
const int keyframe[][5] = {
  // timestamp, Module, Target loc, Target time,  LED stat
  // (frames)           (percent)   (frames)
  // 0          1       2           3             4
  //                    (-1=no move)

  // initialise first positions (only module & target loc counts here)
  { 0,         0,      50,          200,          0},  // keyframe[00][]
  { 0,         1,      -1,          0,            0},  // keyframe[01][]
  { 0,         2,      30,          200,          0},  // keyframe[02][]
  { 0,         3,      50,          75,           0},  // keyframe[03][]
  { 0,         4,      59,          175,          0},  // keyframe[04][]
  { 0,         5,      -1,          0,            0},  // keyframe[05][]
  { 0,         6,      43,          50,           0},  // keyframe[06][]
  { 0,         7,      100,         200,          0},  // keyframe[07][]
  { 0,         8,      100,         200,          0},  // keyframe[08][]
  { 0,         9,      47,          75,           0},  // keyframe[09][]
  { 0,         10,     -1,          0,            0},  // keyframe[10][]
  { 0,         11,     -1,          0,            0},  // keyframe[11][]

  // WO 02 movie sequence
  // timestamp, Module, Target loc, Target time,  LED stat
  { 278,         0,     -1,          0,            1},  // keyframe[12][]
  { 602,         0,     -1,          0,            0},  // keyframe[13][]
  { 603,         0,     0,          690,            0},  // keyframe[14][]
  { 701,         0,     -1,          0,            1},  // keyframe[15][]
  { 1026,         0,     -1,          0,            0},  // keyframe[16][]
  { 1143,         3,     100,         1543,            1},  // keyframe[17][]
  { 1543,         3,     -1,         0,            0},  // keyframe[18][]
  { 1543,         6,     -1,         0,            1},  // keyframe[19][]
  { 1544,         3,     0,         1669,            0},  // keyframe[20][]
  { 1922,         6,     -1,         0,            0},  // keyframe[21][]
  { 1922,         6,     0,         1960,            0},  // keyframe[22][]
  { 2055,         9,     -1,         0,            1},  // keyframe[23][]
  { 2335,         9,     -1,         0,            0},  // keyframe[24][]
  { 2336,         9,     0,         2411,            0},  // keyframe[25][]
  { 2383,         0,     40,         3039,            1},  // keyframe[26][]
  { 3039,         0,     -1,         0,            0},  // keyframe[27][]
  { 3039,         9,     63,         4031,            1},  // keyframe[28][]
  { 3059,         0,     100,         3259,            0},  // keyframe[29][]
  { 4031,         9,     -1,         0,            0},  // keyframe[30][]
  { 4066,         6,     100,         5826,            1},  // keyframe[31][]
  { 5828,         6,     -1,         0,            0},  // keyframe[32][]
  { 5827,         6,     0,         5914,            0},  // keyframe[33][]
  { 5941,         3,     -1,         0,            1},  // keyframe[34][]
  { 6251,         3,     -1,         0,            0},  // keyframe[35][]
  { 6282,         2,     -1,         0,            1},  // keyframe[36][]
  { 6616,         2,     -1,         0,            0},  // keyframe[37][]
  { 6617,         2,     0,         6742,            0},  // keyframe[38][]
  { 6650,         10,     100,         7351,            1},  // keyframe[39][]
  { 7351,         10,     -1,         0,            0}, // keyframe[40][]
  { 7352,         10,     0,         7527,            0},  // keyframe[41][]
  { 7395,         4,     -1,         0,            1},  // keyframe[42][]
  { 7703,         4,     -1,         0,            0},  // keyframe[43][]
  { 7704,         4,     0,         7790,            0},  // keyframe[44][]
  { 7791,         4,     100,         8531,            1},  // keyframe[45][]
  { 8531,         4,     -1,         0,            0},  // keyframe[46][]
  { 8532,         4,     0,         8907,            0},  // keyframe[47][]
  { 8553,         2,     100,         9553,            1},  // keyframe[48][]
  { 9553,         2,     -1,         0,            0},  // keyframe[49][]
  { 9553,         11,     100,         10553,            1},  // keyframe[50][]
  { 9554,         2,     0,         9929,            0},  // keyframe[51][]
  { 10553,         11,     -1,         0,            0},  // keyframe[52][]
  { 10554,         11,     0,         10629,            0},  // keyframe[53][]
  { 10640,         7,     -1,         0,            1},  // keyframe[54][]
  { 10914,         7,     -1,         0,           0},  // keyframe[55][]
  { 10915,         7,     0,         11015,           0},  // keyframe[56][] ????????? (!) fast move
  { 11023,         7,     -1,         0,           1},  // keyframe[57][]
  { 11297,         7,     -1,         0,           0},  // keyframe[58][]
  { 11476,         5,     -1,         0,           1},  // keyframe[59][]
  { 11843,         5,     -1,         0,           0},  // keyframe[60][]
  { 11949,         8,     0,         13949,           1},  // keyframe[61][]
  { 13949,         8,     -1,         0,           0},  // keyframe[62][]
  { 14091,         9,     100,         14599,           1},  // keyframe[63][]
  { 14599,         9,     -1,         0,           0},  // keyframe[64][]
  { 14600,         9,     0,         14725,           0},  // keyframe[65][]
  { 14756,         0,     -1,         0,           1},  // keyframe[66][]
  { 15215,         0,     -1,         0,           0},  // keyframe[67][]
  { 15216,         0,     0,         15591,           0},  // keyframe[68][]
  { 15332,         1,     100,         16832,           1},  // keyframe[69][]
  { 16832,         1,     -1,         0,           0},  // keyframe[70][]
  { 16833,         1,     0,         16958,           0}  // keyframe[71][]


  // AT THE END OF THE SEQUENCE, MAKE ALL MOTORS GO TO POS 0 DURING THE CREDITS AND DO A HOMING.
  // AFTER THE HOMING, ASSUME FIRST POSITIONS AGAIN
  // AND WAIT UNTIL THE RPI MS VALUES CHANGE FROM LARGE TO SMALL. (detect loop)



  // QUICK TEST SEQUENCE
  //  // video start
  //  // timestamp, Module, Target loc, Target time,  LED stat
  //  { 75,         3,      50,         150,          1},  // keyframe[12][] // mod 3 | move with LED
  //  { 150,        3,      -1,         0,            0},  // keyframe[13][] // mod 3 | LED off
  //  { 150,        7,      50,         227,          1},  // keyframe[14][] // mod 7 | move with LED
  //  { 227,        3,      25,         304,          0},  // keyframe[15][] // mod 3 | move without LED
  //  { 227,        7,      -1,         0,            0},  // keyframe[16][] // mod 7 | LED off
  //  { 304,        7,      25,         380,          0},  // keyframe[17][] // mod 7 | move without LED
  //  { 380,        3,      95,         412,          0},  // keyframe[18][] // mod 3 | move without LED
  //  { 424,        3,      0,          436,          0},  // keyframe[19][] // mod 3 | move without LED
  //  { 448,        7,      95,         460,          0},  // keyframe[20][] // mod 7 | move without LED
  //  { 472,        7,      0,          484,          0},  // keyframe[21][] // mod 7 | move without LED




};


/*
  PINS

  module 0 - WO_01_rail_1  (Traveling around)
  module 1 - WO_01_rail_2  (Traveling vertical)
  module 2 - WO_02_rail_1  (Traveling around)
  module 3 - WO_03_rail_1  (Traveling vertical)
  module 4 - WO_04_rail_1  (Traveling around)
  module 5 - WO_04_rail_2  (!) (stationary LED only, no motor or switches here)
  module 6 - WO_09_rail_1  (Traveling horizontally)
  module 7 - WO_09_rail_2  (2 stills)
  module 8 - WO_09_rail_3  (Diving down)
  module 9 - WO_10_rail_1  (Traveling around)
  module 10 - WO_11_rail_1  (Traveling horizontal front)
  module 11 - WO_11_rail_2  (Close up horizontal travel)

  initialise 2D array:
*/
int module[][8] = {

  // module[0][] | connector 01 | WO_01_rail_1
  {
    /* 0 */ 8, // LED pin
    /* 1 */ 9, // switch pin
    /* 2 */ 10, // motor enable pin
    /* 3 */ 11, // motor step pin
    /* 4 */ 12, // motor dir pin
    /* 5 */ 1, // motor debug enable (enable or disable motor (for debugging))
    /* 6 */ 74850, // rail length (in motorsteps). (minus 250 for the zero point offset)
    /* 7 */ 0, // poll motor
  },

  // module[1][] | connector 02 | WO_01_rail_2
  {
    /* 0 */ 3, // LED pin
    /* 1 */ 4, // switch pin
    /* 2 */ 5, // motor enable pin
    /* 3 */ 6, // motor step pin
    /* 4 */ 7, // motor dir pin
    /* 5 */ 1, // motor debug enable (enable or disable motor (for debugging))
    /* 6 */ 75350, // rail length (in motorsteps). (minus 250 for the zero point offset)
    /* 7 */ 0, // poll motor
  },

  // module[2][] | connector 03 | WO_02_rail_1
  {
    /* 0 */ 17, // LED pin
    /* 1 */ 16, // switch pin
    /* 2 */ 15, // motor enable pin
    /* 3 */ 14, // motor step pin
    /* 4 */ 2, // motor dir pin
    /* 5 */ 1, // motor debug enable (enable or disable motor (for debugging))
    /* 6 */ 75350, // rail length (in motorsteps). (minus 250 for the zero point offset)
    /* 7 */ 0, // poll motor
  },

  // module[3][] | connector 05 | WO_03_rail_1
  {
    /* 0 */ 45, // LED pin
    /* 1 */ 46, // switch pin
    /* 2 */ 44, // motor enable pin
    /* 3 */ 43, // motor step pin
    /* 4 */ 38, // motor dir pin
    /* 5 */ 1, // motor debug enable (enable or disable motor (for debugging))
    /* 6 */ 19750, // rail length (in motorsteps). (minus 250 for the zero point offset)
    /* 7 */ 0, // poll motor
  },

  // module[4][] | connector 06 | WO_04_rail_1
  {
    /* 0 */ 30, // LED pin
    /* 1 */ 37, // switch pin
    /* 2 */ 36, // motor enable pin
    /* 3 */ 35, // motor step pin
    /* 4 */ 34, // motor dir pin
    /* 5 */ 1, // motor debug enable (enable or disable motor (for debugging))
    /* 6 */ 51490, // rail length (in motorsteps). 51490 is 90% of the total rail length, due to a short cable. (minus 250 for the zero point offset)
    /* 7 */ 0, // poll motor
  },

  // module[5][] | connector 04 | WO_04_rail_2
  {
    /* 0 */ 22, // LED pin
    /* 1 */ 29, // switch pin         (!) (stationary LED only, no motor or switches here)
    // this was originally pin 21, but this pin is now used by I2C comm. PCB traces were cut.
    // Pin 29 is available, but not used. Purely to avoid weird things happening when for-looping over the motor[] array.
    /* 2 */ 28, // motor enable pin   (!) (stationary LED only, no motor or switches here)
    // this was originally pin 20, but this pin is now used by I2C comm. PCB traces were cut.
    // Pin 28 is available, but not used. Purely to avoid weird things happening when for-looping over the motor[] array.
    /* 3 */ 19, // motor step pin     (!) (stationary LED only, no motor or switches here)
    /* 4 */ 18, // motor dir pin      (!) (stationary LED only, no motor or switches here)
    /* 5 */ 1,  // motor debug enable (enable or disable motor (for debugging)).
                // Leave at 1, even though there's no motor attached
    /* 6 */ 111, // rail length (in motorsteps). (!) dummy value here
    /* 7 */ 0, // poll motor
  },

  // module[6][] | connector 07 | WO_09_rail_1
  {
    /* 0 */ 39, // LED pin
    /* 1 */ 32, // switch pin
    /* 2 */ 42, // motor enable pin
    /* 3 */ 41, // motor step pin
    /* 4 */ 40, // motor dir pin
    /* 5 */ 1, // motor debug enable (enable or disable motor (for debugging))
    /* 6 */ 14320, // rail length (in motorsteps). (minus 250 for the zero point offset)
    /* 7 */ 0, // poll motor
  },

  // module[7][] | connector 08 | WO_09_rail_2
  {
    /* 0 */ 23, // LED pin
    /* 1 */ 24, // switch pin
    /* 2 */ 27, // motor enable pin
    /* 3 */ 26, // motor step pin
    /* 4 */ 25, // motor dir pin
    /* 5 */ 1, // motor debug enable (enable or disable motor (for debugging))
    /* 6 */ 38350, // rail length (in motorsteps). (minus 250 for the zero point offset)
    /* 7 */ 0, // poll motor
  },

  // module[8][] | connector 09 | WO_09_rail_3
  {
    /* 0 */ 54, // LED pin            See: https://www.arduino.cc/en/Hacking/PinMappingSAM3X
    /* 1 */ 55, // switch pin         See: https://www.arduino.cc/en/Hacking/PinMappingSAM3X
    /* 2 */ 58, // motor enable pin   See: https://www.arduino.cc/en/Hacking/PinMappingSAM3X
    /* 3 */ 57, // motor step pin     See: https://www.arduino.cc/en/Hacking/PinMappingSAM3X
    /* 4 */ 56, // motor dir pin      See: https://www.arduino.cc/en/Hacking/PinMappingSAM3X
    /* 5 */ 1, // motor debug enable (enable or disable motor (for debugging))
    /* 6 */ 38470, // rail length (in motorsteps). (minus 250 for the zero point offset)
    /* 7 */ 0, // poll motor
  },

  // module[9][] | connector 10 | WO_10_rail_1
  {
    /* 0 */ 59, // LED pin            See: https://www.arduino.cc/en/Hacking/PinMappingSAM3X
    /* 1 */ 60, // switch pin         See: https://www.arduino.cc/en/Hacking/PinMappingSAM3X
    /* 2 */ 63, // motor enable pin   See: https://www.arduino.cc/en/Hacking/PinMappingSAM3X
    /* 3 */ 62, // motor step pin     See: https://www.arduino.cc/en/Hacking/PinMappingSAM3X
    /* 4 */ 61, // motor dir pin      See: https://www.arduino.cc/en/Hacking/PinMappingSAM3X
    /* 5 */ 1, // motor debug enable (enable or disable motor (for debugging))
    /* 6 */ 27250, // rail length (in motorsteps). (minus 250 for the zero point offset)
    /* 7 */ 0, // poll motor
  },

  // module[10][] | connector 11 | WO_11_rail_1
  {
    /* 0 */ 64, // LED pin            See: https://www.arduino.cc/en/Hacking/PinMappingSAM3X
    /* 1 */ 65, // switch pin         See: https://www.arduino.cc/en/Hacking/PinMappingSAM3X
    /* 2 */ 53, // motor enable pin
    /* 3 */ 52, // motor step pin
    /* 4 */ 50, // motor dir pin
    /* 5 */ 1, // motor debug enable (enable or disable motor (for debugging))
    /* 6 */ 31750, // rail length (in motorsteps). (minus 250 for the zero point offset)
    /* 7 */ 0, // poll motor
  },

  // module[11][] | connector 12 | WO_11_rail_2
  {
    /* 0 */ 48, // LED pin
    /* 1 */ 51, // switch pin
    /* 2 */ 33, // motor enable pin
    /* 3 */ 47, // motor step pin
    /* 4 */ 49, // motor dir pin
    /* 5 */ 1, // motor debug enable (enable or disable motor (for debugging))
    /* 6 */ 8110, // rail length (in motorsteps). (minus 250 for the zero point offset)
    /* 7 */ 0, // poll motor
  }
};







// ****************
// SYSTEM VARIABLES
// ****************

byte moduleTestState = 0;
byte moduleTestPreviousState = 255; // don't change this. (255 is the highest valueof a byte. I use it as initializing value to be out of range)

// simulating the video time (when working without a raspberry pi conencted)
// unsigned long fake_time_millis_at_start = 0;




// SINGLE TRIGGER VARIABLES
byte previousShot = 255; // don't change this. (255 is the highest valueof a byte. I use it as initializing value to be out of range)






// MOTORS

// Homing states
//  0: not homing
//  1: clearing switches (prevents being trapped behind limit switch)
//  2: approaching limit switch
//  3: limit switch is pressed, reverse stages slowly
//  4: limit switch is no longer pressed, homing is complete
byte motor_01_homing_state = 0;

// Value index (columns): S1 pos | S1 speed | S2 pos | S2 speed | S3 pos | S3 speed | S4 pos | S4 speed
long positions[4]; // Array of desired stepper positions

#include <AccelStepper.h> // Define a stepper and the pins it will use

/*
  Define individual motors

  Array numbers VS rails:
   00 ??? WO_01_rail_1  (Traveling around)
   01 ??? WO_01_rail_2  (Traveling vertical)
   02 ??? WO_02_rail_1  (Traveling around)
   03 ??? WO_03_rail_1  (Traveling vertical)
   04 ??? WO_04_rail_1  (Traveling around)
   05 ??? WO_04_rail_2  (!) (stationary LED only, no motor or switches here)
   06 ??? WO_09_rail_1  (Traveling horizontally)
   07 ??? WO_09_rail_2  (2 stills)
   08 ??? WO_09_rail_3  (Diving down)
   09 ??? WO_10_rail_1  (Traveling around)
   10 ??? WO_11_rail_1  (Traveling horizontal front)
   11 ??? WO_11_rail_2  (Close up horizontal travel)
*/
AccelStepper motor[12] = {
  {AccelStepper::DRIVER, module[0][3], module[0][4]}, // 0    step pin, direction pin
  {AccelStepper::DRIVER, module[1][3], module[1][4]}, // 1
  {AccelStepper::DRIVER, module[2][3], module[2][4]}, // 2
  {AccelStepper::DRIVER, module[3][3], module[3][4]}, // 3
  {AccelStepper::DRIVER, module[4][3], module[4][4]}, // 4
  {AccelStepper::DRIVER, module[5][3], module[5][4]}, // 5
  {AccelStepper::DRIVER, module[6][3], module[6][4]}, // 6
  {AccelStepper::DRIVER, module[7][3], module[7][4]}, // 7
  {AccelStepper::DRIVER, module[8][3], module[8][4]}, // 8
  {AccelStepper::DRIVER, module[9][3], module[9][4]}, // 9
  {AccelStepper::DRIVER, module[10][3], module[10][4]}, // 10
  {AccelStepper::DRIVER, module[11][3], module[11][4]} // 11
};

// SWITCHES
#include <Bounce2.h>

// Instantiate bounce objects (0-11)
Bounce limitSwitch_debouncer[12] = {
  Bounce(),
  Bounce(),
  Bounce(),
  Bounce(),
  Bounce(),
  Bounce(),
  Bounce(),
  Bounce(),
  Bounce(),
  Bounce(),
  Bounce(),
  Bounce()
};


/*
  Variables for I2C communication

  The number is broken down into an array of bytes on the raspberry pi and sent over to be reconstructed here.
  Tested with numbers up to 6 digits.

  Setup:  Raspberry Pi v4 connected via I2C with an Arduino Due

  based on: https://www.aranacorp.com/en/communication-between-raspberry-pi-and-arduino-with-i2c/
*/
# include <Wire.h>
# define I2C_ADDRESS 0x08 // hexadecimal: 0x08 is decimal 8. This is the first available I2C address
volatile bool I2C_dataReceived = false;
volatile byte I2C_numberOfDigits = 0;
volatile byte I2C_byteBuffer[10] = "";
int videoTime_assemble = 0; // used to collect I2C data bafore cleaning
int videoTime = 0; // video time in millisec (this datatype is large enough for the running time of the video).
int previousVideoTime = 0;
const int I2C_skip = 500; // ms. Allow a certain jump in the time number stream


bool looping = false; // flag for detecting loop point
int loopBuffer[5] = {}; // initialise all 6 elements to 0 (default for ints)
byte loopIndex = 0;
byte detect_loop_state = 0;
    // state 0: filling the loopBuffer
    // state 1: check 1
    // state 2: check 2

// adding an explicit prototype of the main functions at the beginning of the sketch
void play();
void test();


void setup() {

  // Setup Power lEDS and turn them off
  setupPowerLEDS();
  for (byte i = 0; i < 12; i++) {
    powerLED(i, 0); // turn off all LEDs
  }

  // Open I2C bus
  Wire.begin(I2C_ADDRESS);
  delay(1000);

  // Open serial communications
  // Don't wait for port to open, since it should work standalone
  Serial.begin(57600);
  Serial.println();
  Serial.println();
  Serial.println("serial is open");
  Serial.print("Listening on I2C address: ");
  Serial.println(I2C_ADDRESS);
  delay(500);

  // count the total amount of keyframes
  total_keyframes = sizeof(keyframe) / sizeof(keyframe[0]);
  Serial.print("total keyframe count: ");
  Serial.println(total_keyframes);

  setupLimitSwitches();
  setupMotors();
  setupInternalLED();




  // give time to switch on the power (TODO: this is only for during development)
  Serial.println("waiting for 3 seconds");
  for (byte i = 0; i < 3; i++) {
    Serial.println("...");
    delay(1000);
  }

  delay(30);
  Wire.onReceive(receiveEvents); // this function will launch when a communication package arrives
}

void loop() {
  statePoll();
}
