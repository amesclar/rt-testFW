#include <TM1637Display.h>

// Pin definitions
#define CLK 3
#define DIO 5
#define BUZZER_PIN 4
#define BTN_1MIN 2
#define BTN_2MIN 7
#define BTN_3MIN 8
#define BTN_5MIN 12
#define BTN_TEST_CYCLE 6
#define VOLTAGE_PIN A0

// Voltage divider resistors (10k/5k)
#define R1 10000.0   // 10kΩ
#define R2 5000.0    // 5kΩ

// Display object
TM1637Display display(CLK, DIO);

// Test state
bool testRunning = false;
unsigned long testStartMillis = 0;
unsigned long elapsedSeconds = 0;
unsigned long lastSecondUpdate = 0;
int currentTestSequence = 0; // 1, 2, 3, or 5 minutes

// Automated test cycle state
bool autoCycleRunning = false;
int autoCycleStep = 0; // 0=1min, 1=2min, 2=3min, 3=5min
unsigned long autoCycleWaitUntil = 0;
bool autoCycleWaiting = false;

// Test sequence durations
int testDurations[] = {60, 120, 180, 300}; // 1min, 2min, 3min, 5min
int testPins[] = {BTN_1MIN, BTN_2MIN, BTN_3MIN, BTN_5MIN};
int testSequenceNumbers[] = {1, 2, 3, 5};

// Voltage measurement (commented out - hardware not ready)
unsigned long lastVoltageLog = 0;
#define VOLTAGE_LOG_INTERVAL 1000 // Log voltage every 1 second

void setup() {
  Serial.begin(9600);

  // Initialize display
  display.setBrightness(0x0f);
  display.showNumberDecEx(0, 0b01000000, true);

  // Initialize pins
  pinMode(BTN_1MIN, INPUT);
  pinMode(BTN_2MIN, INPUT);
  pinMode(BTN_3MIN, INPUT);
  pinMode(BTN_5MIN, INPUT);
  pinMode(BTN_TEST_CYCLE, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(VOLTAGE_PIN, INPUT);
  digitalWrite(BUZZER_PIN, LOW);

  Serial.println(F("Regatta Timer Test Framework Ready"));
}

void loop() {
  // Voltage measurement commented out - hardware not ready
  /*
  unsigned long currentMillis = millis();
  if (currentMillis - lastVoltageLog >= VOLTAGE_LOG_INTERVAL) {
    lastVoltageLog = currentMillis;
    float voltage = measureVoltage();

    Serial.print(F("<testcase classname=\"VoltageMonitor\" elapsed=\""));
    if (testRunning) {
      Serial.print(elapsedSeconds);
    } else {
      Serial.print(0);
    }
    Serial.print(F("\" type=\"Voltage\" voltage=\""));
    Serial.print(voltage, 2);
    Serial.println(F("\"/>"));
  }
  */

  // Check for test cycle button press (pin 6) - only way to start tests
  static bool testCyclePressed = false;
  if (digitalRead(BTN_TEST_CYCLE) == HIGH && !testCyclePressed && !autoCycleRunning) {
    testCyclePressed = true;
    startAutoCycle();
    delay(200); // Debounce
  } else if (digitalRead(BTN_TEST_CYCLE) == LOW) {
    testCyclePressed = false;
  }

  // Handle automated test cycle (only if cycle has been started)
  if (autoCycleRunning) {
    handleAutoCycle();
  }

  // Update test timer
  if (testRunning) {
    unsigned long currentMillis = millis();

    // Update elapsed seconds
    if (currentMillis - lastSecondUpdate >= 1000) {
      lastSecondUpdate = currentMillis;
      elapsedSeconds++;

      // Update display (countdown)
      int totalSeconds = currentTestSequence * 60;
      int remaining = totalSeconds - elapsedSeconds;
      if (remaining < 0) remaining = 0;
      int minutes = remaining / 60;
      int seconds = remaining % 60;
      display.showNumberDecEx(minutes * 100 + seconds, 0b01000000, true);

      // Log test event every second
      logTestEvent();

      // Check if test finished
      if (elapsedSeconds >= totalSeconds) {
        endTest();
      }
    }
  }

  // Monitor button states for logging
  logButtonStates();
}

void startTest(int duration, int sequence) {
  testRunning = true;
  elapsedSeconds = 0;
  testStartMillis = millis();
  lastSecondUpdate = testStartMillis;
  currentTestSequence = sequence;

  // Display initial time
  int minutes = duration / 60;
  int seconds = duration % 60;
  display.showNumberDecEx(minutes * 100 + seconds, 0b01000000, true);

  // Log start event
  Serial.print(F("<testcase classname=\"TestStart\" testsequence=\""));
  Serial.print(currentTestSequence);
  Serial.print(F("\" elapsed=\""));
  Serial.print(elapsedSeconds);
  Serial.print(F("\" type=\"Start\" duration=\""));
  Serial.print(duration);
  Serial.println(F("\"/>"));
}

void endTest() {
  testRunning = false;

  // Log end event
  Serial.print(F("<testcase classname=\"TestEnd\" testsequence=\""));
  Serial.print(currentTestSequence);
  Serial.print(F("\" elapsed=\""));
  Serial.print(elapsedSeconds);
  Serial.println(F("\" type=\"End\"/>"));

  // Display 00:00
  display.showNumberDecEx(0, 0b01000000, true);
}

void logTestEvent() {
  Serial.print(F("<testcase classname=\"TestTick\" testsequence=\""));
  Serial.print(currentTestSequence);
  Serial.print(F("\" elapsed=\""));
  Serial.print(elapsedSeconds);
  Serial.println(F("\" type=\"Tick\"/>"));
}

void logButtonStates() {
  static bool lastBtn1 = LOW;
  static bool lastBtn2 = LOW;
  static bool lastBtn3 = LOW;
  static bool lastBtn5 = LOW;
  static bool lastBuzzer = LOW;

  bool currentBtn1 = digitalRead(BTN_1MIN);
  bool currentBtn2 = digitalRead(BTN_2MIN);
  bool currentBtn3 = digitalRead(BTN_3MIN);
  bool currentBtn5 = digitalRead(BTN_5MIN);
  bool currentBuzzer = digitalRead(BUZZER_PIN);

  // Log button state changes
  if (currentBtn1 != lastBtn1) {
    Serial.print(F("<testcase classname=\"ButtonMonitor\" pin=\""));
    Serial.print(BTN_1MIN);
    Serial.print(F("\" elapsed=\""));
    Serial.print(testRunning ? elapsedSeconds : 0);
    Serial.print(F("\" type=\"Button\" state=\""));
    Serial.print(currentBtn1 ? "HIGH" : "LOW");
    Serial.println(F("\"/>"));
    lastBtn1 = currentBtn1;
  }

  if (currentBtn2 != lastBtn2) {
    Serial.print(F("<testcase classname=\"ButtonMonitor\" pin=\""));
    Serial.print(BTN_2MIN);
    Serial.print(F("\" elapsed=\""));
    Serial.print(testRunning ? elapsedSeconds : 0);
    Serial.print(F("\" type=\"Button\" state=\""));
    Serial.print(currentBtn2 ? "HIGH" : "LOW");
    Serial.println(F("\"/>"));
    lastBtn2 = currentBtn2;
  }

  if (currentBtn3 != lastBtn3) {
    Serial.print(F("<testcase classname=\"ButtonMonitor\" pin=\""));
    Serial.print(BTN_3MIN);
    Serial.print(F("\" elapsed=\""));
    Serial.print(testRunning ? elapsedSeconds : 0);
    Serial.print(F("\" type=\"Button\" state=\""));
    Serial.print(currentBtn3 ? "HIGH" : "LOW");
    Serial.println(F("\"/>"));
    lastBtn3 = currentBtn3;
  }

  if (currentBtn5 != lastBtn5) {
    Serial.print(F("<testcase classname=\"ButtonMonitor\" pin=\""));
    Serial.print(BTN_5MIN);
    Serial.print(F("\" elapsed=\""));
    Serial.print(testRunning ? elapsedSeconds : 0);
    Serial.print(F("\" type=\"Button\" state=\""));
    Serial.print(currentBtn5 ? "HIGH" : "LOW");
    Serial.println(F("\"/>"));
    lastBtn5 = currentBtn5;
  }

  // Log buzzer state changes
  if (currentBuzzer != lastBuzzer) {
    Serial.print(F("<testcase classname=\"BuzzerMonitor\" pin=\""));
    Serial.print(BUZZER_PIN);
    Serial.print(F("\" elapsed=\""));
    Serial.print(testRunning ? elapsedSeconds : 0);
    Serial.print(F("\" type=\"Buzzer\" state=\""));
    Serial.print(currentBuzzer ? "HIGH" : "LOW");
    Serial.println(F("\"/>"));
    lastBuzzer = currentBuzzer;
  }
}

float measureVoltage() {
  // Take multiple readings for accuracy
  int sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(VOLTAGE_PIN);
    delay(1);
  }
  int raw = sum / 10;

  // Convert to voltage
  float vout = (raw * 5.0) / 1024.0;
  float vin = vout * ((R1 + R2) / R2);

  return vin;
}

void startAutoCycle() {
  autoCycleRunning = true;
  autoCycleStep = 0;
  autoCycleWaiting = false;

  Serial.println(F("<testcase classname=\"AutoCycleStart\" type=\"CycleStart\"/>"));

  // Trigger first test immediately
  simulateButtonPress(testPins[autoCycleStep]);
}

void handleAutoCycle() {
  unsigned long currentMillis = millis();

  // If waiting between tests
  if (autoCycleWaiting) {
    if (currentMillis >= autoCycleWaitUntil) {
      autoCycleWaiting = false;
      autoCycleStep++;

      // Check if cycle complete
      if (autoCycleStep >= 4) {
        endAutoCycle();
      } else {
        // Trigger next test
        simulateButtonPress(testPins[autoCycleStep]);
      }
    }
  } else if (!testRunning) {
    // Test just finished, start waiting period (5 seconds)
    autoCycleWaiting = true;
    autoCycleWaitUntil = currentMillis + 5000;

    Serial.print(F("<testcase classname=\"AutoCycleWait\" type=\"WaitStart\" duration=\"5\" currentstep=\""));
    Serial.print(testSequenceNumbers[autoCycleStep]);
    Serial.print(F("\" nextstep=\""));
    if (autoCycleStep + 1 < 4) {
      Serial.print(testSequenceNumbers[autoCycleStep + 1]);
    } else {
      Serial.print(F("Complete"));
    }
    Serial.println(F("\"/>"));
  }
}

void simulateButtonPress(int pin) {
  Serial.print(F("<testcase classname=\"AutoCycleAction\" type=\"SimulatePress\" pin=\""));
  Serial.print(pin);
  Serial.print(F("\" testsequence=\""));
  Serial.print(testSequenceNumbers[autoCycleStep]);
  Serial.println(F("\"/>"));

  // Simulate button press by directly calling startTest
  startTest(testDurations[autoCycleStep], testSequenceNumbers[autoCycleStep]);
}

void endAutoCycle() {
  autoCycleRunning = false;

  // Voltage measurement commented out - hardware not ready
  // float voltage = measureVoltage();

  Serial.print(F("<testcase classname=\"AutoCycleEnd\" type=\"CycleEnd\""));
  // Serial.print(F(" voltage=\""));
  // Serial.print(voltage, 2);
  // Serial.print(F("\""));
  Serial.println(F("/>"));
}
