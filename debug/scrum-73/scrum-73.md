| Connections | Continuity (Y/N) |
| :--- | :--- |
| 1 - 2 | y |
| 2 - 3 | y |
| 3 - 4 | n |
| 4 - 5 | y |
| 5 - 6 | 10k |
| 6 - 7 | na |
| 6 - 8 | 5.1k |
| 6 - 9 | y |
| 7 - 8 | y |
| 8 - 9 | n |
| 9 - 10 | y|
| GND/left-1 | y |
| GND/right-7 | y |
| GND/left-GND/right | y |
| A0 - 10 | y |

---
<<debug sequence 1>>
step 1
Hole 4 to Resistor continuity: With everything off, measure ohms/continuity between Hole 4 (where the red wire goes) and the input leg of the 10k resistor.
```continuity buzzes```

step 2
Battery Polarity: Ensure the Positive (+) side of the 9V battery is on Pin 4 and the Negative (-) is on Pin 3.
```check```

step 3
Voltage at the holes: With the 9V battery plugged into the male pins:
Measure with your voltmeter directly from Hole 4 to Hole 3.
Do you see 9V here? If you see 0V at the holes (on the board), then the male pins are not making physical contact with the sockets.
```8.5v```

step 4
The "Golden Proof": While your d (Raw Debug) mode is running, if you touch a jumper wire from the Arduino's 5V pin directly to the top of the 10k resistor, does the A0 Raw jump to ~344?

If the "5V-to-resistor" test still works but the "9V-to-connector" test fails, the problem is 100% the physical connection at Hole 4.

What do you see when you measure voltage directly across Hole 4 and Hole 3 with the battery plugged in?

```
9:36:07.423 -> <testcase classname="VoltageMonitor" elapsedSec="0" type="VoltageStart"/>
09:36:13.190 -> --- Raw Debug ON ---
09:36:13.191 -> A0 Raw: 617
09:36:13.449 -> A0 Raw: 620
09:36:13.675 -> A0 Raw: 616
```