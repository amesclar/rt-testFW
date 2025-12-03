# Regatta Timer Test Framework - Technical Specification

## Purpose
This test framework exercises the Sailing Regatta Timer by simulating button presses. Log messages are emitted for overall start, each test start and overall end. The overall test sequence is started upon button press on pin 6. A single test sequnce consists of activating the regatta timer for 1min, 2min, 3min, and 5min sequences with 5 second pause between tests. The test sequence is repeated TEST_CYCLE_REPEAT times with a default of 10.

Continuous horn voltage measurement is made on port A0 with log messages emitted when the voltage exceeds 2V. The log message should contain elapsed time (seconds), peak voltage value and duration of voltage >2V. Ignore durations shorter than 10ms. Voltage elapsed time should reset for each test (1m, 2m, 3m, 5m). The voltage split circuit consists of R1=10k ohm and R2=5.1k ohm.

TM1637 should display countdown time in MM:SS format.

Serial port speed is 9600baud.

Log messages should be JUNIT XML.


## Hardware Components

### Display
- **Type**: TM1637 4-digit 7-segment display
- **CLK Pin**: 3
- **DIO Pin**: 5
- **Format**: MM:SS (minutes:seconds with colon separator)
- **Brightness**: Maximum (0x0f)

### Monitoring Inputs
| Component | Pin | Purpose | Input type |
|-----------|-----|---------| ---------- |
| Test Cycle Button | 6 | Initiate automated test cycle | INPUT_PULLUP |
| Voltage Sensor | A0 | Measure power supply voltage | Analog |

### Outputs
| Component | Pin | Purpose | Duration |
|-----------|-----|---------| -------- |
| 1min Button | 2 | start 1min test | 60 seconds |
| 2min Button | 7 | start 2min test | 120 seconds |
| 3min Button | 8 | start 3min test | 180 seconds |
| 5min Button | 12 | start 5min test | 300 seconds |
**Note, button activation duration for debouncing only not held for test duration**

### Voltage Measurement Circuit
- **Voltage Divider Configuration**:
  - R1 = 10kΩ (connected to 12V source)
  - R2 = 5.1kΩ (connected to GND)

**Voltage Calculation Formula**
```
ADC_raw = analogRead(A0)
Vout = (ADC_raw × 5.0) / 1024.0
Vin = Vout × ((R1 + R2) / R2)
Vin = Vout × 3.0
```

## Serial Communication
- **Baud Rate**: 9600
- **Format**: XML testcase fragments
- **Purpose**: Comprehensive event logging for test validation

## Operating Modes
### Automated Test Cycle Mode (ONLY MODE SUPPORTED)
- Triggered by pin 6 button press
- Sequentially simulates all four test sequences
- 5-second wait period after each test completes
- Cannot be interrupted once started

### Sequence Flow - voltage monitoring
Once per second (non-blocking) monitor voltage on pin A0 and emit log message if voltage exceeds 1V.

## Automated Test Cycle Specification
### Trigger
- **Button**: Pin 6 (Test Cycle Button)
- **Condition**: Only activates when no test is currently running
- **Behavior**: Single press initiates full cycle

### Sequence Flow - timing tests
1. **Log cycle start with voltage measurement**
2. **Simulate 1min button press** (pin 2)
   - Start 60-second test
   - Display countdown time
   - Monitor for 60 seconds
   - Log test end
3. **Wait 5 seconds**
4. **Simulate 2min button press** (pin 7)
   - Start 120-second test
   - Display countdown time
   - Monitor for 120 seconds
   - Log test end
5. **Wait 5 seconds**
6. **Simulate 3min button press** (pin 8)
   - Start 180-second test
   - Display countdown time
   - Monitor for 180 seconds
   - Log test end
7. **Wait 5 seconds**
8. **Simulate 5min button press** (pin 12)
   - Start 300-second test
   - Display countdown time
   - Monitor for 300 seconds
   - Log test end
9. **Wait 5 seconds**
10. **Log cycle end**

### Timing
- **Total Duration**: 680 seconds (11 minutes 20 seconds)
  - Test time: 60 + 120 + 180 + 300 = 660 seconds
  - Wait time: 4 × 5 = 20 seconds
- **Wait Period**: 5 seconds after EACH test (including the last one)

### Test Sequence Data Structures
```cpp
int testDurations[] = {60, 120, 180, 300};           // seconds
int testPins[] = {BTN_1MIN, BTN_2MIN, BTN_3MIN, BTN_5MIN};  // pins 2,7,8,12
int testSequenceNumbers[] = {1, 2, 3, 5};            // sequence identifiers
```

### State Management
- **autoCycleRunning**: Boolean flag indicating cycle is active
- **autoCycleStep**: Current step (0=1min, 1=2min, 2=3min, 3=5min)
- **autoCycleWaiting**: Boolean flag indicating in wait period
- **autoCycleWaitUntil**: Timestamp for end of wait period (millis)

## Event Logging (XML Format)

### Test Start Event
```xml
<testcase classname="TestStart" testsequence="[1|2|3|5]" elapsed="0" type="Start" duration="[seconds]"/>
```

### Test End Event
```xml
<testcase classname="TestEnd" testsequence="[1|2|3|5]" elapsed="[seconds]" type="End"/>
```

### Voltage Monitor
```xml
<testcase classname="VoltageMonitor" elapsed="[seconds]" voltage="[xx.xx]"/>
```

## Software Architecture
### State Variables
```cpp
// Test timing
bool testRunning = false;
unsigned long testStartMillis = 0;
unsigned long elapsedSeconds = 0;
unsigned long lastSecondUpdate = 0;
int currentTestSequence = 0;

unsigned long lastVoltageLog = 0;
#define VOLTAGE_LOG_INTERVAL 1000
#define VOLTAGE_LOG_THRESHOLD 1
```

### Main Loop Flow
1. **Check test cycle button** (pin 6)
   - If pressed and not running → start auto cycle
2. **Handle auto cycle state machine** (if active)
   - If waiting: check if wait period expired
   - If wait expired: advance step or end cycle
   - If test finished: start 5-second wait
3. **Update test timer** (if test running)
   - Update elapsed seconds every 1000ms
   - Update display countdown
   - Check if test complete

### Non-Blocking Design
- All timing uses `millis()` comparison
- No blocking delays except for button debounce (200ms)
- Wait periods use non-blocking state machine

## Behavioral Requirements

### Startup
1. Initialize serial at 9600 baud
2. Print ready message: "Regatta Timer Test Framework Ready"
3. Set display brightness to maximum
4. Display 00:00
5. Configure all pins
6. Wait for pin 6 button press to begin automated test cycle

### Auto Cycle Initiation
1. Detect pin 6 button press
2. Verify no test currently running
3. Log AutoCycleStart event
4. Set autoCycleRunning = true
5. Set autoCycleStep = 0
6. Immediately simulate first button press (1min)

### Auto Cycle State Machine
**State: Test Running**
- Let test execute normally
- Wait for test to complete

**State: Test Complete → Waiting**
- Log AutoCycleWait event with current and next step
- Set wait timer for 5 seconds

**State: Wait Period → Next Test**
- When wait expires, advance step
- If step < 4: simulate next button press
- If step >= 4: end cycle

**State: Cycle Complete**
- Log AutoCycleEnd event
- Set autoCycleRunning = false
- Return to idle state (wait for next pin 6 press)

### Libraries Required
- `TM1637Display.h` for 7-segment display control

### Memory Optimization
- Use `F()` macro for string literals in serial output
- Static arrays for test configuration
- Minimal dynamic memory allocation

### Display Format
- Leading zeros shown for proper MM:SS format
- Colon bit mask: `0b01000000`
- Format calculation: `minutes * 100 + seconds`

### Timing Accuracy
- Uses `millis()` for all timing
- 1-second resolution for tests
- 1-second resolution for voltage logging
- 5-second wait periods after tests

### Debouncing
- 200ms delay after button press detection
- Prevents multiple triggers from single press
- Applied to both manual and auto cycle buttons

## Design Principles
1. **Non-Intrusive Monitoring**: Framework observes but doesn't control the production timer
2. **Automated Testing Only**: Single button (pin 6) initiates full test cycle - no manual tests
3. **Comprehensive State Tracking**: All pin changes logged for complete visibility
4. **Deterministic Timing**: Precise 5-second gaps and test durations
5. **XML Structured Output**: Machine-parseable logs for automated validation
6. **Non-Blocking Architecture**: Concurrent monitoring and test execution
7. **Voltage Ready**: Code structure ready for voltage monitoring when hardware is implemented
