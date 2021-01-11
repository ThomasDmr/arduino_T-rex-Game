// arduino_dino_async.ino
/**
   This code allows an Arduino combined with 3 LDR sensors and a servomotor to play the Google Chrome
   Trex game.
   It was inspired by other projects but this code has the particularity to run in an asynchronous mode.
   This allows to deal and buffer several obstacles and go further into the game.
   It was developped for educational purposes, so some functions are simplified and the overal structure could be improved.

   The philosophy goes as follows :
    - The LDR sensors (one for the cactus, one for the birds) are placed as far away as possible from the dino to
      detect the obstacles as soon as possible.
    - Once an obstacle is detected, the timestamp gets added to a circular buffer. The obstacleIndex keeps track of
      where in the buffer the new timestamp needs to be added.
    - In the meantime (that's wy we need asynchronous code), the jumpIndex keeps track of the next comming obstacle.
      Once we have waited long enough, we allow the dino to jump and start focusing on the next obstacle.
    - Aside of these tasks, we also slowly decrease the waiting time before we jump to deal with the speed increase.
      We also use a third LDR to check if the game changed to dark mode or back to day mode.

   @warning : several parameters used in this code (intervals, thresholds...) are project and computer-specific.
              don't forget to readjust the code to your environment.

   Feel free to send feedback, questions or suggestions to thomas.demmer.pro@gmail.com

   © Thomas Demmer - 2020 | www.thomasdmr.com
*/

#include <Servo.h>

// LDR parameters
// Photodiode responsible for detecting the cactus
int cactusAnalogPin = A0;
int cactusMeanValue = 0;
int cactusMinValue = 1023;

// Photodiode responsible for detecting the pterodactyls
int birdAnalogPin = A1;
int birdMeanValue = 0;
int birdMinValue = 1023;

// Photodiode responsib le for detecting the dark mode
int darkModeAnalogPin = A2;
int darkModeMeanValue = 0;

// Servo parameters
Servo jumpServo;
int jumpServoPin = 9;
int idlePosition = 155;
int pressPosition = 145;

// Other variables
uint32_t        obstacleBuffer[4] = {0}; // table that records the time of obstacle detection
unsigned int    obstacleIndex = 0;
unsigned int    jumpIndex = 0;
bool            darkModeActive = false;
unsigned int    timeBeforeJump = 1183; // initial time in ms between obstacle detection and jump
bool            allowJump = false;
uint32_t        accelTimer = 0;

void setup() {
  Serial.begin(115200);
  jumpServo.attach(jumpServoPin);  // attaches the servo on pin 9 to the servo object
  jumpServo.write(idlePosition);

  // Compute the mean values of the LDR sensors before beginning the game
  cactusMeanValue = computeMeanValue(cactusAnalogPin);
  birdMeanValue = computeMeanValue(birdAnalogPin);
  darkModeMeanValue  = computeMeanValue(darkModeAnalogPin);

  Serial.println(String(cactusMeanValue) + "\t" + String(birdMeanValue) + "\t" + String(darkModeMeanValue));
}

void loop()
{
  int cactusRawValue = analogRead(cactusAnalogPin);
  int birdRawValue = analogRead(birdAnalogPin);
  int darkModeRawValue = analogRead(darkModeAnalogPin);

  //=============  Block responsible decreasing the interval before we jump  ==============
  if (accelTimer != 0 && millis() - accelTimer > 1200)
  {
    // Every 2 seconds, we decrease the jump delay by 20 milliseconds
    accelTimer = millis();
    timeBeforeJump -= 12;
  }

  //=====================  Block responsible for detecting color changes  =================
  if (!darkModeActive && detectDarkMode(darkModeRawValue, darkModeMeanValue))
  {
    Serial.println("Dark Mode");
    darkModeActive = true;
    // We decrement the jump index because the color change is wrongly detected as an obstacle
    obstacleIndex--;
  }
  else if (darkModeActive && !detectDarkMode(darkModeRawValue, darkModeMeanValue))
  {
    Serial.println("Day Mode");
    darkModeActive = false;
    // We decrement the jump index because the color change is wrongly detected as an obstacle
    obstacleIndex--;
  }

  //=====================  Block responsible for detecting obstacles  =================
  if (!darkModeActive)
  {
    // We store the min values of the sensors during day mode. These values will be used as reference in the dark mode.
    cactusMinValue = updateMinValue(cactusMinValue, cactusRawValue);
    birdMinValue = updateMinValue(birdMinValue, birdRawValue);

    if (detectObstacle(cactusRawValue, cactusMeanValue) || detectObstacle(birdRawValue, birdMeanValue))
    {
      addObstacleTimeToBuffer();
    }
  }
  else
  {
    if (detectObstacle(cactusRawValue, 80) || detectObstacle(birdRawValue, 88))
    {
      //Serial.println(String(cactusRawValue) + "\t" + String(cactusMinValue - 10) + "\t" + String(birdRawValue) + "\t" + String(birdMinValue - 10));
      addObstacleTimeToBuffer();
    }
  }

  //================= Block responsible for deciding if we need to jump ===============
  allowJump = checkIfTimeToJump();

  //======================    Block responsible for jumping    ========================
  async_jump(allowJump);

}

/**
   async_jump : Function that calls a quick back and forth on the servo so that it presses the
   touchboard as quickly as possible.
   This function is time based and needs to be called often in the event-loop
   @param allowJump : boolean that gives the signal to start the servo movement
*/
void async_jump(bool allowJump)
{
  static uint32_t lastJump = 0;
  const int pressTime = 120;

  if (allowJump)
  {
    lastJump = millis();
    jumpServo.write(pressPosition);

    if (jumpIndex == 1)
    {
      accelTimer = millis();
      Serial.println("First Jump\t" + String(accelTimer));
    }
  }

  else if (millis() - lastJump > pressTime)
  {
    jumpServo.write(idlePosition);
  }
}

/**
   addObstacleTimeToBuffer : adds a timestamp to a timestamp buffer.
   This function need to be called as soon as an obstacle is detected so that we record when the
   obstacle was detected.
   The buffer allows for several obstacle to be detected between two jumps.
*/
void addObstacleTimeToBuffer()
{
  // check that we are not adding twice the same obstacle
  if (millis() - obstacleBuffer[(obstacleIndex - 1) % 4] > 150)
  {
    obstacleBuffer[obstacleIndex % 4] = millis(); // we store the time at which the obstacle is detected
    obstacleIndex++;
  }
}

/**
   checkIfTimeToJump : checks if the dino should jump or not.
   This functions compares the time interval between an obstacle timestamp in the obstacle buffer
   to the timeBeforeJump interval. As soon as we have waited enough, we allow the dino to jump.
   @return a boolean that says if we should jump or not
*/
bool checkIfTimeToJump()
{
  if (jumpIndex != obstacleIndex)
  {
    if (millis() - obstacleBuffer[jumpIndex % 4] > timeBeforeJump)
    {
      jumpIndex++;
      //Serial.println(String(obstacleBuffer[0]) + "\t" + String(obstacleBuffer[1]) + "\t" + String(obstacleBuffer[2]) + "\t"
      //+ String(obstacleBuffer[3]) + "\t" + String(obstacleIndex) + "\t" +String(jumpIndex));
      Serial.println(String(millis()) + "\t" + String(obstacleIndex) + "\t" + String(jumpIndex));
      return true;
    }
  }
  return false;
}

/**
   computeMeanValue : a blocking function that computes the mean value received by the luminosity
   sensor over a period of time.
   This functions has to be called in the setup on a static screen.
   @param analogPin : analog pin on which we want to compute the mean value
   @return the mean value (int) computed.
*/
int computeMeanValue(int analogPin)
{
  long mean = 0;
  for (int i = 0; i < 200; i++)
  {
    mean += analogRead(analogPin);
    delay(4);
  }

  return mean / 200;
}

/**
   detectObstacle : Detects an obstacle by comparing the actual value (rawValue) to the a reference value (the mean
   value of the beginning). If the difference is bigger than a fixed interval, it means we detected an obstacle.
   To avoid jumping twice the same obstacle we use a boolean that avoids detecting twice the same obstacle.
   @param rawvalue : the raw value of the analog pin we want to detect an obstacle on
   @param meanValue : the reference value we compare the raw value to.
   @return a boolean saying if an obstacle was detected or not.
*/
bool detectObstacle(int rawValue, int meanValue)
{
  static bool samePeakAsBefore = false;

  if (abs(meanValue - rawValue) > 80 && !samePeakAsBefore)
  {
    samePeakAsBefore = true;
    return true;
  }
  else if (abs(meanValue - rawValue) < 40)
  {
    samePeakAsBefore = false;
  }

  return false;
}


/**
   updateMinValue :updates and stores the minimum value red on an analog pin.
   @param rawvalue : the raw value of the analog pin we want to measure the min value
   @param previousMinValue : the previous minimum value
   @return the new minimum value
*/
int updateMinValue(int previousMinValue, int rawValue)
{
  if (rawValue < previousMinValue)
  {
    return rawValue;
  }
  else
  {
    return previousMinValue;
  }
}

/**
   detectDarkMode :Detects if the dark mode is activated
   @param rawvalue : the raw value of the analog pin we want to detect the dark mode
   @param meanValue : the reference value we compare the raw value to.
   @return true if we are in dark mode, false otherwise.
*/
//
bool detectDarkMode(int rawValue, int meanValue)
{
  static uint32_t lastModeChange = 0;
  static bool     lastMode = false;
  bool            currentMode = false;

  if (meanValue - rawValue > 50 )
  {
    currentMode = true;
  }
  else
  {
    currentMode = false;
  }

  // on state change, we record the time
  if (currentMode != lastMode)
  {
    if (lastModeChange == 0)
    {
      lastModeChange = millis();
    }
  }

  // we wait a short time for the color mode to change
  if (lastModeChange != 0 && millis() - lastModeChange > 100)
  {
    Serial.println(lastMode);
    lastMode = currentMode;
    lastModeChange = 0;
  }
  return lastMode;
}
