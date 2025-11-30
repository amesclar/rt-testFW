# Regatta Timer Test Framework - Technical Specification

## Purpose
This test framework monitors and validates the Sailing Regatta Timer by tracking button states, monitoring buzzer activity, and providing automated test cycle execution. It does NOT execute timer sequences itself - it monitors the production timer's behavior.

**IMPORTANT**: This framework ONLY supports automated test cycles triggered by pin 6. Manual individual button tests are not supported.

## Hardware Components

### Display
- **Type**: TM1637 4-digit 7-segment display
- **CLK Pin**: 3
- **DIO Pin**: 5
- **Format**: MM:SS (minutes:seconds with colon separator)
- **Brightness**: Maximum (0x0f)
- **Usage**: Shows countdown during manual tests

### Monitoring Inputs
| Component | Pin | Purpose |
|-----------|-----|---------|
| 1min Button | 2 | Monitor button state |
| 2min Button | 7 | Monitor button state |
| 3min Button | 8 | Monitor button state |
| 5min Button | 12 | Monitor button state |
| Test Cycle Button | 6 | Initiate automated test cycle |
| Buzzer Monitor | 4 | Monitor buzzer output (INPUT) |
| Voltage Sensor | A0 | Measure power supply voltage |

**Pin Modes**: 
- All buttons: INPUT (not INPUT_PULLUP)
- Buzzer: OUTPUT (for compatibility, but primarily monitored as state)
- Voltage: INPUT (analog)

### Voltage Measurement Circuit (DISABLED - Hardware Not Ready)
- **Voltage Divider Configuration**:
  - R1 = 10kΩ (connected to 12V source)
  - R2 = 5kΩ (connected to GND)
  - Division ratio: 3:1
  - Input range: 0-15V DC
  - Output to A0: 0-5V (safe for Arduino)

**STATUS**: All voltage measurement code is commented out. Hardware circuit not yet implemented.

**Voltage Calculation Formula** (for future use):
```
ADC_raw = analogRead(A0)
Vout = (ADC_raw × 5.0) / 1024.0
Vin = Vout × ((R1 + R2) / R2)
Vin = Vout × 3.0
```

**Sampling Method**: 10-sample average for stability (when enabled)

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
- **Manual individual button tests are NOT supported**

## Automated Test Cycle Specification

### Trigger
- **Button**: Pin 6 (Test Cycle Button)
- **Condition**: Only activates when no test is currently running
- **Behavior**: Single press initiates full cycle

### Sequence Flow
1. **Log cycle start with voltage measurement**
2. **Simulate 1min button press** (pin 2)
   - Start 60-second test
   - Monitor for 60 seconds
   - Log test end
3. **Wait 5 seconds**
4. **Simulate 2min button press** (pin 7)
   - Start 120-second test
   - Monitor for 120 seconds
   - Log test end
5. **Wait 5 seconds**
6. **Simulate 3min button press** (pin 8)
   - Start 180-second test
   - Monitor for 180 seconds
   - Log test end
7. **Wait 5 seconds**
8. **Simulate 5min button press** (pin 12)
   - Start 300-second test
   - Monitor for 300 seconds
   - Log test end
9. **Wait 5 seconds**
10. **Log cycle end with voltage measurement**

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

### Test Tick Event (Every Second During Test)
```xml
<testcase classname="TestTick" testsequence="[1|2|3|5]" elapsed="[seconds]" type="Tick"/>
```

### Test End Event
```xml
<testcase classname="TestEnd" testsequence="[1|2|3|5]" elapsed="[seconds]" type="End"/>
```

### Button Monitor Event (State Changes Only)
```xml
<testcase classname="ButtonMonitor" pin="[2|7|8|12]" elapsed="[seconds]" type="Button" state="[HIGH|LOW]"/>
```

### Buzzer Monitor Event (State Changes Only)
```xml
<testcase classname="BuzzerMonitor" pin="4" elapsed="[seconds]" type="Buzzer" state="[HIGH|LOW]"/>
```

### Auto Cycle Start Event
```xml
<testcase classname="AutoCycleStart" type="CycleStart"/>
```

### Auto Cycle Simulate Button Press
```xml
<testcase classname="AutoCycleAction" type="SimulatePress" pin="[2|7|8|12]" testsequence="[1|2|3|5]"/>
```

### Auto Cycle Wait Period
```xml
<testcase classname="AutoCycleWait" type="WaitStart" duration="5" currentstep="[1|2|3|5]" nextstep="[1|2|3|5|Complete]"/>
```
- **currentstep**: Test sequence that just completed
- **nextstep**: Test sequence that will start after wait, or "Complete"

### Auto Cycle End Event
```xml
<testcase classname="AutoCycleEnd" type="CycleEnd"/>
```

### Voltage Monitor (DISABLED - Hardware Not Ready)
```xml
<!-- COMMENTED OUT IN CODE -->
<testcase classname="VoltageMonitor" elapsed="[seconds]" type="Voltage" voltage="[xx.xx]"/>
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

// Automated cycle
bool autoCycleRunning = false;
int autoCycleStep = 0;
unsigned long autoCycleWaitUntil = 0;
bool autoCycleWaiting = false;

// Voltage monitoring (commented out - hardware not ready)
unsigned long lastVoltageLog = 0;
#define VOLTAGE_LOG_INTERVAL 1000
```

### Main Loop Flow
1. **Check test cycle button** (pin 6)
   - If pressed and not running → start auto cycle
   - This is the ONLY way to initiate tests
2. **Handle auto cycle state machine** (if active)
   - If waiting: check if wait period expired
   - If wait expired: advance step or end cycle
   - If test finished: start 5-second wait
3. **Update test timer** (if test running)
   - Update elapsed seconds every 1000ms
   - Update display countdown
   - Log TestTick event
   - Check if test complete
4. **Monitor button and buzzer states**
   - Log state changes for all monitored pins

**Note**: Manual individual test button presses (pins 2, 7, 8, 12) are not supported. Code for manual tests has been removed.

### Non-Blocking Design
- All timing uses `millis()` comparison
- No blocking delays except for button debounce (200ms)
- Wait periods use non-blocking state machine

### Button State Monitoring
The framework continuously monitors button and buzzer pin states:
- Uses static variables to remember previous state
- Logs only on state transitions (HIGH↔LOW)
- Prevents duplicate logging of same state

```cpp
static bool lastBtn1 = LOW;
bool currentBtn1 = digitalRead(BTN_1MIN);
if (currentBtn1 != lastBtn1) {
  // Log state change
  lastBtn1 = currentBtn1;
}
```

## Behavioral Requirements

### Startup
1. Initialize serial at 9600 baud
2. Print ready message: "Regatta Timer Test Framework Ready"
3. Set display brightness to maximum
4. Display 00:00
5. Configure all pins
6. Wait for pin 6 button press to begin automated test cycle

### Manual Test Initiation
**NOT SUPPORTED** - Manual individual button tests have been removed. Only automated test cycles via pin 6 are supported.

### Manual Test Execution
**NOT SUPPORTED** - Only automated test cycles are available.

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

### Voltage Measurement (DISABLED)
**All voltage measurement code is commented out pending hardware implementation.**

When enabled in the future:
1. Take 10 analog readings with 1ms delays
2. Calculate average ADC value
3. Convert to voltage using divider formula
4. Return voltage with 2 decimal precision

## Implementation Notes

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

## Test Validation Use Cases

### Use Case 1: Button Validation
- Monitor all button presses during automated cycle
- Verify simulated button presses trigger correct sequences
- Detect bounce or contact issues

### Use Case 2: Buzzer Timing Validation
- Monitor buzzer pin state changes
- Verify buzzer activation timing
- Measure buzzer duration (HIGH state duration)
- Count buzzer pulses per event

### Use Case 3: Full Cycle Test
- Run complete automated test cycle (pin 6 press)
- Verify all four sequences execute
- Verify 5-second gaps between tests
- Verify total cycle time (680 seconds)

### Use Case 4: Regression Testing
- Run automated cycle after code changes
- Compare XML logs against baseline
- Verify no timing changes

### Use Case 5: Power Supply Monitoring (Future)
- When voltage hardware is ready, uncomment voltage code
- Continuously log voltage during entire test cycle
- Verify voltage remains stable (±0.5V)
- Detect voltage drops during buzzer activation

## Example Test Output

### Automated Test Cycle Log (Abbreviated)
```xml
<testcase classname="AutoCycleStart" type="CycleStart"/>
<testcase classname="AutoCycleAction" type="SimulatePress" pin="2" testsequence="1"/>
<testcase classname="TestStart" testsequence="1" elapsed="0" type="Start" duration="60"/>
<testcase classname="TestTick" testsequence="1" elapsed="1" type="Tick"/>
<testcase classname="ButtonMonitor" pin="2" elapsed="1" type="Button" state="HIGH"/>
<testcase classname="BuzzerMonitor" pin="4" elapsed="1" type="Buzzer" state="HIGH"/>
<testcase classname="BuzzerMonitor" pin="4" elapsed="1" type="Buzzer" state="LOW"/>
...
<testcase classname="TestTick" testsequence="1" elapsed="60" type="Tick"/>
<testcase classname="TestEnd" testsequence="1" elapsed="60" type="End"/>
<testcase classname="AutoCycleWait" type="WaitStart" duration="5" currentstep="1" nextstep="2"/>
...
<testcase classname="AutoCycleAction" type="SimulatePress" pin="7" testsequence="2"/>
<testcase classname="TestStart" testsequence="2" elapsed="0" type="Start" duration="120"/>
...
<testcase classname="TestEnd" testsequence="5" elapsed="300" type="End"/>
<testcase classname="AutoCycleWait" type="WaitStart" duration="5" currentstep="5" nextstep="Complete"/>
<testcase classname="AutoCycleEnd" type="CycleEnd"/>
```

## Design Principles

1. **Non-Intrusive Monitoring**: Framework observes but doesn't control the production timer
2. **Automated Testing Only**: Single button (pin 6) initiates full test cycle - no manual tests
3. **Comprehensive State Tracking**: All pin changes logged for complete visibility
4. **Deterministic Timing**: Precise 5-second gaps and test durations
5. **XML Structured Output**: Machine-parseable logs for automated validation
6. **Non-Blocking Architecture**: Concurrent monitoring and test execution
7. **Voltage Ready**: Code structure ready for voltage monitoring when hardware is implemented