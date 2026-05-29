#include <Wire.h>
#include <Adafruit_Sensor_Calibration.h>
#include <Adafruit_AHRS.h>
#include <IBusBM.h>

/*----------------------------------------------*/
//If true writes thrust commands to specified LED pins
//If false writes thrust commands to specified motor controller pins
bool TEST_MODE = false;
//If true sends data output to Serial port for use with Python real-time plotting scripts
bool DEBUG_MODE = false;
const bool USE_IMU = true;
const uint32_t IMU_I2C_TIMEOUT_US = 3000;
const unsigned long IMU_RETRY_INTERVAL_MS = 2000;
//Values to plot in Python, valid options are "euler","accelerometer","gyroscope","magnetometer"
char debug_type[] = "euler";   

//Pin assignments
const int anchorMotorPin = 9;

const int rightMotorPin = 6;
const int leftMotorPin = 7;

const int rightForwardLED = 10;
const int leftForwardLED = 11;
const int rightBackwardLED = 12;
const int leftBackwardLED = 13;

//Serial port assignments
IBusBM ibusRC;
HardwareSerial& ibusRCSerial = Serial2;  //RC receiver
HardwareSerial& moos = Serial;   //comms with navigation RPi/MOOS-IvP

/*----------Setup IMU and sensor fusion----------*/
Adafruit_Sensor *accelerometer, *gyroscope, *magnetometer;
bool imuReady = false;
bool imuCalibrationLoaded = false;
unsigned long imuLastAttemptMs = 0;

#include "NXP_FXOS_FXAS.h"  // NXP 9-DoF breakout

// pick your filter! slower == better quality output
//Adafruit_NXPSensorFusion filter; // slowest
Adafruit_Madgwick filter;  // faster than NXP
//Adafruit_Mahony filter;  // fastest/smalleset

/*----------Load IMU calibrations----------*/
#if defined(ADAFRUIT_SENSOR_CALIBRATION_USE_EEPROM)
Adafruit_Sensor_Calibration_EEPROM cal;
#else
Adafruit_Sensor_Calibration_SDFat cal;
#endif
/*-----------------------------------------*/

#define FILTER_UPDATE_RATE_HZ 50
#define PRINT_EVERY_N_UPDATES 5

uint32_t timestamp;
const byte numChars = 32;
char receivedChars[numChars];
char tempChars[numChars];
char nmeaHeader[numChars] = {0};
float thrustLeft = 0.0;
float thrustRight = 0.0;
const unsigned long CONTROL_LOST_TIMEOUT_MS = 3000;
unsigned long moosLastCommandMs = 0;
int curLeft = 188.0;
int curRight = 188.0;
float leftSend = 0.0;
float rightSend = 0.0;
boolean newData = false;
int manualControl = 0;   //0 = manual control off, 1 = manual control on

const float THROTTLE_ZERO_THRESHOLD = 5;
int LIMIT = 32; //this is the PWM step limit, must be <= 65
//forward PWM values are 191-254 (63 steps), and then limited to 191 + LIMIT
//backward PWM values are 120-185 (65 steps), and then limited to 185 - LIMIT
const float STILL = 188;
const float MIN_THROTTLE = 1;
const float LEFT_FORWARD_MIN = STILL + MIN_THROTTLE;
const float LEFT_BACKWARD_MIN = STILL - MIN_THROTTLE;
const float RIGHT_FORWARD_MIN = LEFT_FORWARD_MIN;
const float RIGHT_BACKWARD_MIN = LEFT_BACKWARD_MIN;
const float LEFT_FORWARD_MAX = LEFT_FORWARD_MIN + LIMIT;
const float LEFT_BACKWARD_MAX = LEFT_BACKWARD_MIN - LIMIT;
const float RIGHT_FORWARD_MAX = RIGHT_FORWARD_MIN + LIMIT;
const float RIGHT_BACKWARD_MAX = LEFT_BACKWARD_MIN - LIMIT;
const float TURN_SPEED = 10;

//RC variables
const byte RC_REPORT_CHANNELS = 8;
const uint16_t RC_VALID_MIN_US = 800;
const uint16_t RC_VALID_MAX_US = 2200;
const unsigned long RC_STALE_TIMEOUT_MS = 500;
uint16_t rcRaw[RC_REPORT_CHANNELS] = {0};
bool rcSignalValid = false;
unsigned long rcLastValidMs = 0;

int rotateCH = 0;
int rotateVal = 0;

int throttleCH = 2;
int throttleVal = 0;

int turnCH = 3;
int turnVal = 0;

int manualCH = 6;
bool MANUAL;

int drivemodeCH = 5;
bool BACKWARD;

int turnMax = 30;
int turnLeft;
int turnRight;
int throttleMap;

void setup(void)
{
  /*------Setup for RC control--------*/
  ibusRC.begin(ibusRCSerial);

  /*------Setup motor pins--------*/
  if (!TEST_MODE) {
    pinMode(rightMotorPin, OUTPUT);
    pinMode(leftMotorPin, OUTPUT);
    analogWrite(rightMotorPin, STILL);
    analogWrite(leftMotorPin, STILL);
  }

  /*------Setup test LED pins--------*/
  if (TEST_MODE) {
    pinMode(rightForwardLED, OUTPUT);
    pinMode(leftForwardLED, OUTPUT);
    pinMode(rightBackwardLED, OUTPUT);
    pinMode(leftBackwardLED, OUTPUT);
    digitalWrite(rightForwardLED, LOW);
    digitalWrite(leftForwardLED, LOW);
    digitalWrite(rightBackwardLED, LOW);
    digitalWrite(leftBackwardLED, LOW);
  }

  /*------Setup for front seat comms--------*/
  moos.begin(115200);
  while (!moos) {
    delay(1);
  }

  configureIMUBus();
  timestamp = millis();

}

const String PREFIX    = "PL";   // Device prefix: PL = PEARL
const String ID_EULER  = "IMU";  // Sentence ID for Euler angles generated from sensor fusion filter
const String ID_RAW    = "RAW";  // Sentence ID for raw IMU readings
const String ID_MOTOR  = "MOT";  // Sentence ID for notifying MOOS-IvP of current motor commands
const String ID_RC     = "RC";   // Sentence ID for RC receiver channel telemetry

void loop(void)
{
  static uint8_t counter = 0;

  /*------Read IMU data and package in NMEA sentence--------*/
  float roll = 0.0;
  float pitch = 0.0;
  float heading = 0.0;
  float new_heading = 0.0;
  float ax = 0.0;
  float ay = 0.0;
  float az = 0.0;
  float gx = 0.0;
  float gy = 0.0;
  float gz = 0.0;
  float mx = 0.0;
  float my = 0.0;
  float mz = 0.0;

  if ((millis() - timestamp) < (1000 / FILTER_UPDATE_RATE_HZ)) {
    return;
  }

  timestamp = millis();
  /*--------Handle manual control inputs from RC--------*/
  handleRC();
  /*--------Read NMEA sentence from serial port--------*/
  readFromMOOS();
  /*--------Convert to PWM, and send to motor controllers-------*/
  commandThrust();
  /*--------Convert back to thrust percentage for report to MOOS-IvP--------*/
  reportThrust();     // if TEST_MODE is true then also commands LED brightness

  tryStartIMU();
  if (imuReady) {
    // Read the motion sensors
    clearIMUTimeout();
    sensors_event_t accel, gyro, mag;
    accelerometer->getEvent(&accel);
    gyroscope->getEvent(&gyro);
    magnetometer->getEvent(&mag);

    if (imuBusTimedOut()) {
      imuReady = false;
    }
    else {
      cal.calibrate(mag);
      cal.calibrate(accel);
      cal.calibrate(gyro);

      ax = accel.acceleration.x;
      ay = accel.acceleration.y;
      az = accel.acceleration.z;

      // Gyroscope needs to be converted from Rad/s to Degree/s
      gx = gyro.gyro.x * SENSORS_RADS_TO_DPS;
      gy = gyro.gyro.y * SENSORS_RADS_TO_DPS;
      gz = gyro.gyro.z * SENSORS_RADS_TO_DPS;

      mx = mag.magnetic.x;
      my = mag.magnetic.y;
      mz = mag.magnetic.z;

      // Update the SensorFusion filter
      filter.update(gx, gy, gz,
                    ax, ay, az,
                    mx, my, mz);

      roll = filter.getRoll();
      pitch = filter.getPitch();
      heading = filter.getYaw();

      // fixes error with how IMU heading angle is reported
      new_heading = mapFloat(heading, 0, 360, 360, 0);
      new_heading += 180;
      if (new_heading > 360.0)
        new_heading -= 360.0;
      if (new_heading < 0.0)
        new_heading += 360.0;
    }
  }


  // only send data to serial port once in a while
  if (counter++ <= PRINT_EVERY_N_UPDATES) {
    return;
  }
  // reset the counter
  counter = 0;

  /*--------Generate NMEA strings for MOOS-IvP--------*/
  //Euler angle NMEA string
  String NMEA_EULER = "";
  if (imuReady) {
    String PAYLOAD_EULER = String(manualControl) + "," + String(new_heading) + "," + String(pitch) + "," + String(roll);
    NMEA_EULER = generateNMEAString(PAYLOAD_EULER, PREFIX, ID_EULER);
  }

  //Raw IMU data NMEA string
  String NMEA_RAW = "";
  if (imuReady) {
    String PAYLOAD_RAW = String(ax) + "," + String(ay) + "," + String(az) + "," +
                         String(gx) + "," + String(gy) + "," + String(gz) + "," +
                         String(mx) + "," + String(my) + "," + String(mz);
    NMEA_RAW = generateNMEAString(PAYLOAD_RAW, PREFIX, ID_RAW);
  }

  //Last motor commands NMEA string
  String PAYLOAD_MOTOR = String(leftSend) + "," + String(rightSend);
  String NMEA_MOTOR = generateNMEAString(PAYLOAD_MOTOR, PREFIX, ID_MOTOR);

  //RC receiver NMEA string
  String PAYLOAD_RC = buildRCPayload();
  String NMEA_RC = generateNMEAString(PAYLOAD_RC, PREFIX, ID_RC);


  if (DEBUG_MODE) {
    if (!imuReady) {
      return;
    }
    if (strcmp(debug_type,"euler")==0)
      sendToPython(&new_heading, &pitch, &roll);
    else if (strcmp(debug_type,"accelerometer")==0)
      sendToPython(&ax, &ay, &az);
    else if (strcmp(debug_type,"gyroscope")==0)
      sendToPython(&gx, &gy, &gz);
    else if (strcmp(debug_type,"magnetometer")==0)
      sendToPython(&mx, &my, &mz);
  }
  else {
    if (imuReady) {
      moos.println(NMEA_EULER);
      moos.println(NMEA_RAW);
    }
    moos.println(NMEA_MOTOR);
    moos.println(NMEA_RC);
  }
}

String generateNMEAString(String payload, String prefix, String id) {
  String nmea = "";
  nmea = prefix + id + "," + payload;
  return "$" + nmea + "*";    // Prefixed with $
}

void configureIMUBus() {
  Wire.begin();
  Wire.setWireTimeout(IMU_I2C_TIMEOUT_US, true);
  Wire.setClock(400000); // 400KHz
}

void clearIMUTimeout() {
  Wire.clearWireTimeoutFlag();
}

bool imuBusTimedOut() {
  bool timedOut = Wire.getWireTimeoutFlag();
  if (timedOut) {
    Wire.clearWireTimeoutFlag();
  }
  return timedOut;
}

void tryStartIMU() {
  if (!USE_IMU || imuReady) {
    return;
  }

  unsigned long now = millis();
  if (imuLastAttemptMs != 0 && (now - imuLastAttemptMs) < IMU_RETRY_INTERVAL_MS) {
    return;
  }
  imuLastAttemptMs = now;

  configureIMUBus();
  clearIMUTimeout();
  if (!imuCalibrationLoaded) {
    cal.begin();
    cal.loadCalibration();
    imuCalibrationLoaded = true;
  }

  clearIMUTimeout();
  imuReady = init_sensors();
  if (!imuReady || imuBusTimedOut()) {
    imuReady = false;
    return;
  }

  setup_sensors();
  filter.begin(FILTER_UPDATE_RATE_HZ);
}

void readFromMOOS() {
  static boolean recvInProgress = false;
  static byte ndx = 0;
  char startMarker = '$';
  char endMarker = '*';
  char rc;

  while (moos.available() > 0 && newData == false) {
    rc = moos.read();

    if (recvInProgress == true) {
      if (rc != endMarker) {
        receivedChars[ndx] = rc;
        ndx++;
        if (ndx >= numChars) {
          ndx = numChars - 1;
        }
      }
      else {
        receivedChars[ndx] = '\0'; // terminate the string
        recvInProgress = false;
        ndx = 0;
        newData = true;
      }
    }

    else if (rc == startMarker) {
      recvInProgress = true;
    }
  }
}

bool parseNMEA() {
  // split the data into its parts
  char * strtokIndx; // this is used by strtok() as an index

  strtokIndx = strtok(tempChars, ",");     // get the first part - the string
  if (strtokIndx == NULL) return false;
  strcpy(nmeaHeader, strtokIndx); // copy it to nmeaHeader
  if (strcmp(nmeaHeader, "PICOM") != 0) return false;

  strtokIndx = strtok(NULL, ",");
  if (strtokIndx == NULL) return false;
  thrustLeft = atof(strtokIndx);     // convert the second entry to the left thrust percentage command

  strtokIndx = strtok(NULL, ",");
  if (strtokIndx == NULL) return false;
  thrustRight = atof(strtokIndx);     // convert the third entry to the right thrust percentage command

  return true;
}

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  // Re-maps a float number from one range to another.
  // That is, a value of fromLow would get mapped to toLow, a value of fromHigh to toHigh, values in-between to values in-between, etc.
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

bool isRCControlFresh() {
  unsigned long rcAgeMs = millis() - rcLastValidMs;
  return rcLastValidMs != 0 && rcAgeMs <= CONTROL_LOST_TIMEOUT_MS;
}

bool isMOOSCommandFresh() {
  return moosLastCommandMs != 0 &&
         (millis() - moosLastCommandMs) <= CONTROL_LOST_TIMEOUT_MS;
}

bool isControlAvailable() {
  return isRCControlFresh() || isMOOSCommandFresh();
}

void stopMotors() {
  curLeft = STILL;
  curRight = STILL;
  if (!TEST_MODE) {
    analogWrite(leftMotorPin, curLeft);
    analogWrite(rightMotorPin, curRight);
  }
}

void handleRC() {
  updateRCChannels();

  if (!isRCControlFresh()) {
    manualControl = 0;
    rotateVal = 0;
    throttleVal = 0;
    turnVal = 0;
    BACKWARD = false;
    return;
  }

  MANUAL = readSwitch(manualCH, false);
  if (MANUAL) {
    manualControl = 1;
  }
  else if (!MANUAL) {
    manualControl = 0;
  }

  BACKWARD = readSwitch(drivemodeCH, false);

  rotateVal = readChannel(rotateCH, -100, 100, 0);
  throttleVal = readChannel(throttleCH, -100, 100, 0);

  /*--------Handle turn commands--------*/
  turnVal = readChannel(turnCH, -100, 100, 0);
  if (turnVal > 0) {
    turnRight = map(turnVal, 1, 100, 0, turnMax);
    turnLeft = 0;
  }
  else if (turnVal < 0) {
    turnRight = 0;
    turnLeft = map(turnVal, -1, -100, 0, turnMax);
  }
  else {
    turnRight = 0;
    turnLeft = 0;
  }
  /*------------------------------------*/

  if (manualControl == 1) {
    if (rotateVal > 0) {
      curLeft = map(rotateVal, 1, 100, LEFT_FORWARD_MIN, LEFT_FORWARD_MAX);
      curRight = map(rotateVal, 1, 100, RIGHT_BACKWARD_MIN, RIGHT_BACKWARD_MAX);
    }
    else if (rotateVal < 0) {
      curLeft = map(rotateVal, -1, -100, LEFT_BACKWARD_MIN, LEFT_BACKWARD_MAX);
      curRight = map(rotateVal, -1, -100, RIGHT_FORWARD_MIN, RIGHT_FORWARD_MAX);
    }
    else {
      if (!BACKWARD) {
        throttleMap = map(throttleVal, -95, 100, LEFT_FORWARD_MIN, LEFT_FORWARD_MAX);
        if (throttleMap >= LEFT_FORWARD_MIN) {
          curLeft = throttleMap - turnLeft;
          curRight = throttleMap - turnRight;
        }
        else {
          curLeft = STILL;
          curRight = STILL;
        }
      }
      else if (BACKWARD) {
        throttleMap = map(throttleVal, -95, 100, LEFT_BACKWARD_MIN, LEFT_BACKWARD_MAX);
        if (throttleMap <= LEFT_BACKWARD_MIN) {
          curLeft = throttleMap + turnLeft;
          curRight = throttleMap + turnRight;
        }
        else {
          curLeft = STILL;
          curRight = STILL;
        }
      }
    }
    if (!TEST_MODE) {
      analogWrite(leftMotorPin, curLeft);
      analogWrite(rightMotorPin, curRight);
    }
  }
}

bool isValidRCValue(uint16_t ch) {
  return ch >= RC_VALID_MIN_US && ch <= RC_VALID_MAX_US;
}

void updateRCChannels() {
  bool sawValidChannel = false;

  for (byte i = 0; i < RC_REPORT_CHANNELS; i++) {
    rcRaw[i] = ibusRC.readChannel(i);
    if (isValidRCValue(rcRaw[i])) {
      sawValidChannel = true;
    }
  }

  if (sawValidChannel) {
    rcSignalValid = true;
    rcLastValidMs = millis();
  }
  else if ((millis() - rcLastValidMs) > RC_STALE_TIMEOUT_MS) {
    rcSignalValid = false;
  }
}

uint16_t getCachedRCChannel(byte channelInput) {
  if (channelInput < RC_REPORT_CHANNELS) {
    return rcRaw[channelInput];
  }
  return ibusRC.readChannel(channelInput);
}

String buildRCPayload() {
  unsigned long rcAgeMs = millis() - rcLastValidMs;
  bool rcConnected = rcSignalValid && (rcAgeMs <= RC_STALE_TIMEOUT_MS);
  String payload;
  payload.reserve(96);
  payload = String(rcConnected ? 1 : 0) + "," + String(rcAgeMs);

  for (byte i = 0; i < RC_REPORT_CHANNELS; i++) {
    payload += "," + String(rcRaw[i]);
  }

  payload += "," + String(rotateVal);
  payload += "," + String(throttleVal);
  payload += "," + String(turnVal);
  payload += "," + String(manualControl);
  payload += "," + String(BACKWARD ? 1 : 0);

  return payload;
}

int readChannel(byte channelInput, int minLimit, int maxLimit, int defaultValue) {
  // Read the number of a given channel and convert to the range provided.
  // If the channel is off, return the default value
  uint16_t ch = getCachedRCChannel(channelInput);
  if (!isValidRCValue(ch)) return defaultValue;
  return map(ch, 1000, 2000, minLimit, maxLimit);
}

bool readSwitch(byte channelInput, bool defaultValue) {
  // Read the channel and return a boolean value
  int intDefaultValue = (defaultValue) ? 100 : 0;
  int ch = readChannel(channelInput, 0, 100, intDefaultValue);
  return (ch > 50);
}

void sendToPython(float* x, float* y, float* z) {
  byte* byteX  = (byte*)(x);
  byte* byteY  = (byte*)(y);
  byte* byteZ  = (byte*)(z);
  moos.write(byteX, 4);
  moos.write(byteY, 4);
  moos.write(byteZ, 4);
}

void commandThrust() {
  if (newData == true) {
    strcpy(tempChars, receivedChars);
    bool validMOOSCommand = parseNMEA();
    newData = false;
    if (validMOOSCommand) {
      moosLastCommandMs = millis();
    }
    if (validMOOSCommand && manualControl == 0) {
      float leftVal, rightVal;
      // Map left thrust value to PWM
      if (thrustLeft > 0.05) {
        leftVal = mapFloat(thrustLeft, 0.0, 100.0, LEFT_FORWARD_MIN, LEFT_FORWARD_MAX);
      }
      else if (thrustLeft < 0.05) {
        leftVal = mapFloat(thrustLeft, -100.0, 0.0, LEFT_BACKWARD_MAX, LEFT_BACKWARD_MIN);
      }
      else {
        leftVal = STILL;
      }
      // Map right thrust value to PWM
      if (thrustRight > 0.05) {
        rightVal = mapFloat(thrustRight, 0.0, 100.0, RIGHT_FORWARD_MIN, RIGHT_FORWARD_MAX);
      }
      else if (thrustRight < 0.05) {
        rightVal = mapFloat(thrustRight, -100.0, 0.0, RIGHT_BACKWARD_MAX, RIGHT_BACKWARD_MIN);
      }
      else {
        rightVal = STILL;
      }
      curLeft = round(leftVal);
      curRight = round(rightVal);

      if (!TEST_MODE) {
        analogWrite(leftMotorPin, curLeft);
        analogWrite(rightMotorPin, curRight);
      }
    }
  }

  if (!isControlAvailable()) {
    stopMotors();
  }
}

void reportThrust() {
  if (curLeft > 190) {
    leftSend = mapFloat(float(curLeft), LEFT_FORWARD_MIN, LEFT_FORWARD_MAX, 0.0, 100.0);
    if (TEST_MODE) {
      analogWrite(leftForwardLED, map(curLeft, LEFT_FORWARD_MIN, LEFT_FORWARD_MAX, 0, 255));
      analogWrite(leftBackwardLED, 0);
    }
  }
  else if (curLeft < 186) {
    leftSend = mapFloat(float(curLeft), LEFT_BACKWARD_MAX, LEFT_BACKWARD_MIN, -100.0, 0.0);
    if (TEST_MODE) {
      analogWrite(leftBackwardLED, map(curLeft, LEFT_BACKWARD_MIN, LEFT_BACKWARD_MAX, 0, 255));
      analogWrite(leftForwardLED, 0);
    }
  }
  else {
    leftSend = 0.0;
    if (TEST_MODE) {
      analogWrite(leftForwardLED, 0);
      analogWrite(leftBackwardLED, 0);
    }
  }
  // Map right thrust value to PWM
  if (curRight > 190) {
    rightSend = mapFloat(float(curRight), RIGHT_FORWARD_MIN, RIGHT_FORWARD_MAX, 0.0, 100.0);
    if (TEST_MODE) {
      analogWrite(rightForwardLED, map(curRight, RIGHT_FORWARD_MIN, RIGHT_FORWARD_MAX, 0, 255));
      analogWrite(rightBackwardLED, 0);
    }
  }
  else if (curRight < 186) {
    rightSend = mapFloat(float(curRight), RIGHT_BACKWARD_MAX, RIGHT_BACKWARD_MIN, -100.0, 0.0);
    if (TEST_MODE) {
      analogWrite(rightBackwardLED, map(curRight, RIGHT_BACKWARD_MIN, RIGHT_BACKWARD_MAX, 0, 255));
      analogWrite(rightForwardLED, 0);
    }
  }
  else {
    rightSend = 0.0;
    if (TEST_MODE) {
      analogWrite(rightForwardLED, 0);
      analogWrite(rightBackwardLED, 0);
    }
  }
}
