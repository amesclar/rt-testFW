/*
  test framework for sailing regatta timer

*/

#include <TM1637Display.h>
#define CLK 5
#define DIO 3

TM1637Display tm(CLK, DIO);

// constants won't change. They're used here to set pin numbers:
const bool debug = 0;            // serial out
const int buttonPinTimer1 = 2;   // 1min timer
const int buttonPinTimer2 = 7;   // 2min timer
const int buttonPinTimer3 = 8;   // 3min timer
const int buttonPinTimer5 = 12;  // 5min timer
const int buzzerVoltagePin = 5;  // monitor buzzer voltage
const int testStartPin = 6;      // start test sequence
const int ledPin = 4;            // the number of the LED pin
// const int buzzerPin = 2;              // buzzer pin - output
const int buzzerOnLongMillis = 400;
const int buzzerOnShortMillis = 200;

int currentMin;
int currentSec;
int displayDigits;

void setup() {

  // set brightness; 0-7
  tm.setBrightness(2);

  // initialize the LED pin as an output:
  pinMode(ledPin, OUTPUT);
  // initialize the pushbutton pin as an input:
  pinMode(buttonPinTimer1, OUTPUT);
  pinMode(buttonPinTimer2, OUTPUT);
  pinMode(buttonPinTimer3, OUTPUT);
  pinMode(buttonPinTimer5, OUTPUT);
  // other pins
  pinMode(buzzerVoltagePin, INPUT_PULLUP);
  pinMode(testStartPin, INPUT_PULLUP);
  // initialize serial output
  Serial.begin(9600);
}

bool logMsg(String msg) {
  Serial.println(msg);
  return true;
}

bool debugInfo(int doTest, int loopCurrent, unsigned long currentMillis, unsigned long testEndMillis, int testCurrent) {
  if (debug) {
    logMsg("doTest - " + String(doTest));
    logMsg("loopCurrent - " + String(loopCurrent));
    logMsg("currentMillis - " + String(currentMillis));
    logMsg("testEndMillis - " + String(testEndMillis));
    logMsg("testCurrent - " + String(testCurrent));
  }
  return true;
}

bool buttonPress(int buttonPin, int delayMS = 20) {
  digitalWrite(buttonPin, HIGH);
  delay(delayMS);
  digitalWrite(buttonPin, LOW);
  return true;
}

bool ledBlink(int ledPin, int delayMS = 500) {
  digitalWrite(ledPin, HIGH);
  delay(delayMS);
  digitalWrite(ledPin, LOW);
  return true;
}

bool displayElapsedTime(unsigned long currentMillis) {
      // display elapsed time
    currentMin = currentMillis / 1000 / 60;
    currentSec = (currentMillis / 1000) % 60;
    displayDigits = (currentMin*100) + currentSec;
    
    // logMsg("currentMin - " + String(currentMin));
    // logMsg("currentSec - " + String(currentSec));
    // logMsg("displayDigits - " + String(displayDigits));
    
    tm.showNumberDecEx(displayDigits,0b01000000);
  
    return true;
}

int doTest = 0;  // 0/1 instead of boolean to support log message out
int const loopMax = 2;
int loopCurrent = 0;
int testCurrent = 1;
float interval;
unsigned long currentMillis = 0;
unsigned long testEndMillis = 0;

void loop() {

  currentMillis = millis();
  displayElapsedTime(currentMillis);

  if (digitalRead(testStartPin) == LOW) {
    logMsg("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
    doTest = 1;
    ledBlink(ledPin, 500);

  }

  if ((currentMillis >= testEndMillis) && (loopCurrent <= loopMax) && (doTest)) {
    debugInfo(doTest, loopCurrent, currentMillis, testEndMillis, testCurrent);

    // use if/else if - switch/case doesn't seem to work
    if (testCurrent == 1) {
      // 1min
      testEndMillis = millis() + (60.0 * 1000.0) + 5000.0;
      logMsg("<Tests id=" + String(loopCurrent) + ">");
      logMsg("<Test which=1min id=" + String(millis()) + "/>  ");
      ledBlink(ledPin, 500);
      buttonPress(buttonPinTimer1);
      doTest = 1;
      ++testCurrent;
      debugInfo(doTest, loopCurrent, currentMillis, testEndMillis, testCurrent);
    } else if (testCurrent == 2) {
      // 2min
      testEndMillis = millis() + (120.0 * 1000.0) + 5000.0;
      logMsg("<Test which=2min id=" + String(millis()) + "/>  ");
      ledBlink(ledPin, 500);
      buttonPress(buttonPinTimer2);
      doTest = 1;
      ++testCurrent;
      debugInfo(doTest, loopCurrent, currentMillis, testEndMillis, testCurrent);
    } else if (testCurrent == 3) {
      // 3min
      testEndMillis = millis() + (180.0 * 1000.0) + 5000.0;
      logMsg("<Test which=3min id=" + String(millis()) + "/>  ");
      ledBlink(ledPin, 500);
      buttonPress(buttonPinTimer3);
      doTest = 1;
      ++testCurrent;
      debugInfo(doTest, loopCurrent, currentMillis, testEndMillis, testCurrent);
    } else if (testCurrent == 4) {
      // 5min
      testEndMillis = millis() + (300.0 * 1000.0) + 5000.0;
      logMsg("<Test which=5min id=" + String(millis()) + "/>  ");
      logMsg("</Tests>");
      ledBlink(ledPin, 500);
      buttonPress(buttonPinTimer5);
      doTest = 1;
      testCurrent = 1;
      ++loopCurrent;
      debugInfo(doTest, loopCurrent, currentMillis, testEndMillis, testCurrent);
    }
  }

  if ((loopCurrent > loopMax) && doTest) {
    doTest = 0;
  }
}
