// Gemini
// 20251202-092639
#include <Arduino.h>
#include <TM1637Display.h>

// ==========================================
// Configuration & Pin Definitions
// ==========================================

// Display pins
#define CLK_PIN 3
#define DIO_PIN 5

// Monitoring Inputs
#define BTN_TEST_INITIATE_PIN 6
#define VOLTAGE_SENSOR_PIN A0

// Output Pins (to simulate button presses on target device)
// Assuming Active LOW pulse to trigger target
#define BTN_1MIN_OUT_PIN 2
#define BTN_2MIN_OUT_PIN 7
#define BTN_3MIN_OUT_PIN 8
#define BTN_5MIN_OUT_PIN 12

// Voltage Measurement Configuration
// R1 = 10k, R2 = 5.1k. Spec assumes multiplier of 3.0.
#define VOLTAGE_MULTIPLIER 3.0
#define VOLTAGE_REF 5.0
#define ADC_RESOLUTION 1024.0
#define VOLTAGE_LOG_THRESHOLD 1.0 // Volts
#define MIN_VOLTAGE_DURATION_MS                                                \
  10 // Minimum duration for logging a voltage event

// Test Timing Configuration
#define TEST_CYCLE_REPEAT 100      // Total iterations of the 4-test sequence
#define WAIT_BETWEEN_TESTS_MS 5000 // 5 seconds
#define BUTTON_PULSE_DURATION_MS                                               \
  400 // Duration to hold output low to simulate press
#define DEBOUNCE_DELAY_MS 50

// ==========================================
// Global Objects and Variables
// ==========================================

TM1637Display display(CLK_PIN, DIO_PIN);

// Test Sequence Data Structures (from spec)
const int testDurations[] = {60, 120, 180, 300}; // seconds
const int testPins[] = {BTN_1MIN_OUT_PIN, BTN_2MIN_OUT_PIN, BTN_3MIN_OUT_PIN,
                        BTN_5MIN_OUT_PIN};
const int testSequenceNumbers[] = {1, 2, 3, 5}; // sequence identifiers
const int numTestSteps = sizeof(testDurations) / sizeof(testDurations[0]);

// State Variables
bool autoCycleRunning = false;
int currentCycleIteration = 0; // Counts up to TEST_CYCLE_REPEAT
int autoCycleStep = 0;         // 0 to 3 (for the 4 durations)

bool testRunning = false;
unsigned long testStartMillis = 0; // Start of the current 1/2/3/5 min test
unsigned long elapsedSeconds =
    0; // Elapsed seconds within the current 1/2/3/5 min test
unsigned long lastSecondUpdate = 0;
int currentTestDurationTarget = 0;

bool autoCycleWaiting = false;
unsigned long autoCycleWaitStartMillis = 0;

// Voltage Monitoring State Variables
bool isVoltageHigh = false;
unsigned long voltageHighStartTime = 0;
float voltageHighPeakValue = 0.0; // Track peak voltage during the high event

// ==========================================
// Forward Declarations
// ==========================================
void startAutoCycle();
void handleAutoCycleState();
void startSpecificTest(int stepIndex);
void updateRunningTest();
void endTest();
void simulateButtonPulse(int pin);
float measureVoltage();
void updateDisplay(int secondsRemaining);

// ==========================================
// Setup
// ==========================================
void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // Wait for serial port to connect.
  }

  // Initialize Pins
  pinMode(BTN_TEST_INITIATE_PIN, INPUT_PULLUP);
  pinMode(VOLTAGE_SENSOR_PIN, INPUT);

  // Initialize Outputs. Set HIGH initially because SUT is Active LOW (triggers
  // on LOW with internal pull-ups). Current logic: Idle = HIGH, Active = LOW.
  digitalWrite(BTN_1MIN_OUT_PIN, HIGH);
  pinMode(BTN_1MIN_OUT_PIN, OUTPUT);
  digitalWrite(BTN_2MIN_OUT_PIN, HIGH);
  pinMode(BTN_2MIN_OUT_PIN, OUTPUT);
  digitalWrite(BTN_3MIN_OUT_PIN, HIGH);
  pinMode(BTN_3MIN_OUT_PIN, OUTPUT);
  digitalWrite(BTN_5MIN_OUT_PIN, HIGH);
  pinMode(BTN_5MIN_OUT_PIN, OUTPUT);

  // Initialize Display
  display.setBrightness(0x0f); // Maximum brightness
  updateDisplay(0);

  Serial.println(F("Regatta Timer Test Framework Ready"));
}

// ==========================================
// Main Loop
// ==========================================
void loop() {
  unsigned long currentMillis = millis();

  // 1. Continuous Voltage Monitoring
  // This runs every loop iteration to capture duration accurately.
  float currentVoltage = measureVoltage();

  if (currentVoltage > VOLTAGE_LOG_THRESHOLD) {
    if (!isVoltageHigh) {
      // Transition detected: Voltage went from LOW to HIGH
      isVoltageHigh = true;
      voltageHighStartTime = currentMillis;
      voltageHighPeakValue = currentVoltage; // Initialize peak value
    } else {
      // Voltage is still high, update peak if current is higher
      if (currentVoltage > voltageHighPeakValue) {
        voltageHighPeakValue = currentVoltage;
      }
    }
  } else if (currentVoltage <= VOLTAGE_LOG_THRESHOLD && isVoltageHigh) {
    // Transition detected: Voltage went from HIGH to LOW (Event ended)
    isVoltageHigh = false;
    unsigned long durationMs = currentMillis - voltageHighStartTime;

    // Log the event if its duration meets the minimum threshold
    if (durationMs >= MIN_VOLTAGE_DURATION_MS) {
      Serial.print(F("<testcase classname=\"VoltageMonitor\" elapsedSec=\""));
      // Use elapsedSeconds from the current running test
      Serial.print(
          elapsedSeconds); // Log elapsed seconds relative to current test start
      Serial.print(F("\" durationMs=\""));
      Serial.print(durationMs);
      Serial.print(F("\" peakVoltage=\""));
      Serial.print(voltageHighPeakValue, 2); // Log with 2 decimal places
      Serial.println(F("\"/>"));
    }
  }

  // 2. Check Test Initiate Button (Pin 6)
  // Edge-detection (Falling Edge) to prevent auto-start if button is stuck LOW
  static int lastButtonRead = HIGH; // Assume HIGH (not pressed) initially
  static unsigned long lastDebounceTime = 0;
  static int validButtonState = HIGH; // The stable, debounced state
  static bool waitingForRelease =
      true; // Safety: Require a HIGH state before first trigger

  int currentButtonRead = digitalRead(BTN_TEST_INITIATE_PIN);

  // If the switch changed, reset the debouncing timer
  if (currentButtonRead != lastButtonRead) {
    lastDebounceTime = currentMillis;
  }
  lastButtonRead = currentButtonRead;

  // If stable for DEBOUNCE_DELAY_MS, update the valid state
  if ((currentMillis - lastDebounceTime) > DEBOUNCE_DELAY_MS) {
    if (currentButtonRead != validButtonState) {
      validButtonState = currentButtonRead;

      // If we are waiting for release, check if we got it
      if (waitingForRelease) {
        if (validButtonState == HIGH) {
          waitingForRelease =
              false; // Button is released, ready to accept press
        }
      } else {
        // Normal operation: detected a valid state change (after release was
        // confirmed)
        if (validButtonState == LOW) {
          // Falling Edge detected (HIGH -> LOW)
          if (!autoCycleRunning) {
            startAutoCycle();
          }
        }
      }
    }
  }

  // 3. Handle Automated Cycle State Machine
  if (autoCycleRunning) {
    handleAutoCycleState();
  }

  // 4. Serial Command 's' to start
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if ((cmd == 's' || cmd == 'S') && !autoCycleRunning) {
      startAutoCycle();
    }
  }
}

// ==========================================
// Helper Functions
// ==========================================

// Initiates the entire multi-iteration test process
void startAutoCycle() {
  Serial.println(F("<testsuite name=\"RegattaTimerAutomatedTest\">"));
  Serial.print(F("<testcase classname=\"AutoCycleStart\" iterations=\""));
  Serial.print(TEST_CYCLE_REPEAT);
  Serial.println(F("\" type=\"CycleStart\"/>"));

  autoCycleRunning = true;
  currentCycleIteration = 1;
  autoCycleStep = 0;
  autoCycleWaiting = false;
  testRunning = false;

  // Reset voltage state for the start of a new auto cycle
  isVoltageHigh = false;
  voltageHighStartTime = 0;
  voltageHighPeakValue = 0.0;

  // Kick off the first test immediately
  startSpecificTest(autoCycleStep);
}

// Main state machine logic for the automated sequence
void handleAutoCycleState() {
  unsigned long currentMillis = millis();

  if (testRunning) {
    // State: A specific duration test is currently executing
    updateRunningTest();

  } else if (autoCycleWaiting) {
    // State: Waiting the 5 seconds between tests
    if (currentMillis - autoCycleWaitStartMillis >= WAIT_BETWEEN_TESTS_MS) {
      autoCycleWaiting = false;
      autoCycleStep++;

      if (autoCycleStep < numTestSteps) {
        // Move to next test in current sequence (e.g., 1min done, start 2min)
        startSpecificTest(autoCycleStep);
      } else {
        // Current sequence of 4 tests done. Check iterations.
        Serial.print(
            F("<testcase classname=\"SequenceComplete\" iteration=\""));
        Serial.print(currentCycleIteration);
        Serial.println(F("\"/>"));

        if (currentCycleIteration < TEST_CYCLE_REPEAT) {
          // Reset for next iteration iteration
          currentCycleIteration++;
          autoCycleStep = 0;
          // The wait just happened. We can start immediately.
          startSpecificTest(autoCycleStep);
        } else {
          // All iterations complete
          Serial.println(
              F("<testcase classname=\"AutoCycleEnd\" type=\"CycleEnd\"/>"));
          Serial.println(F("</testsuite>"));
          autoCycleRunning = false;
          updateDisplay(0);
        }
      }
    }
  }
}

// Starts a specific duration test (1, 2, 3, or 5 min)
void startSpecificTest(int stepIndex) {
  int duration = testDurations[stepIndex];
  int seqNumber = testSequenceNumbers[stepIndex];
  int pinToPulse = testPins[stepIndex];

  Serial.print(F("<testcase classname=\"TestStart\" testsequence=\""));
  Serial.print(seqNumber);
  Serial.print(F("\" iteration=\""));
  Serial.print(currentCycleIteration);
  Serial.print(F("\" elapsed=\"0\" type=\"Start\" duration=\""));
  Serial.print(duration);
  Serial.println(F("\"/>"));

  // Simulate button press to trigger target device
  simulateButtonPulse(pinToPulse);

  // Reset test timers
  testRunning = true;
  testStartMillis = millis();
  lastSecondUpdate = testStartMillis;
  elapsedSeconds = 0; // Reset elapsedSeconds for the new test
  currentTestDurationTarget = duration;

  // Reset voltage state for the start of a new individual test
  isVoltageHigh = false;
  voltageHighStartTime = 0;
  voltageHighPeakValue = 0.0;

  updateDisplay(duration - elapsedSeconds);
}

// Updates the timer and display once per second during a test
void updateRunningTest() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastSecondUpdate >= 1000) {
    lastSecondUpdate += 1000;
    elapsedSeconds = (currentMillis - testStartMillis) / 1000;

    int remaining = currentTestDurationTarget - elapsedSeconds;
    if (remaining < 0)
      remaining = 0;

    updateDisplay(remaining);

    if (elapsedSeconds >= currentTestDurationTarget) {
      endTest();
    }
  }
}

// Completes a specific duration test and enters waiting state
void endTest() {
  testRunning = false;
  int seqNumber = testSequenceNumbers[autoCycleStep];

  Serial.print(F("<testcase classname=\"TestEnd\" testsequence=\""));
  Serial.print(seqNumber);
  Serial.print(F("\" iteration=\""));
  Serial.print(currentCycleIteration);
  Serial.print(F("\" elapsed=\""));
  Serial.print(elapsedSeconds);
  Serial.println(F("\" type=\"End\"/>"));

  // Start the wait period
  autoCycleWaiting = true;
  autoCycleWaitStartMillis = millis();
}

// Simulates a momentary button press by pulsing an output pin LOW
// NOTE: SUT is Active LOW (with pull-ups), so we Pulse LOW from HIGH
void simulateButtonPulse(int pin) {
  digitalWrite(pin, LOW);
  delay(BUTTON_PULSE_DURATION_MS);
  digitalWrite(pin, HIGH);
}

// Measures voltage on A0 using spec formula
float measureVoltage() {
  int adcRaw = analogRead(VOLTAGE_SENSOR_PIN);
  float vOut = (adcRaw * VOLTAGE_REF) / ADC_RESOLUTION;
  // Vin = Vout * ((R1 + R2) / R2) => Vin = Vout * 3.0
  float vIn = vOut * VOLTAGE_MULTIPLIER;
  return vIn;
}

// Helper to format and show time on TM1637
void updateDisplay(int secondsRemaining) {
  int minutes = secondsRemaining / 60;
  int seconds = secondsRemaining % 60;
  // 0b01000000 is the colon bitmask for TM1637
  display.showNumberDecEx(minutes * 100 + seconds, 0b01000000, true);
}
