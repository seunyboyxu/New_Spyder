# ESP32 + PCA9685 motor tester

`sesame-motor-tester.ino` targets a classic ESP32 development board (including
HW-394 / ESP-32D) with a PCA9685, rather than servos connected directly to GPIO.
The main robot firmware also uses the same PCA9685 channel mapping and pulse limits.

## Wiring

| ESP32 / supply | PCA9685 |
| --- | --- |
| GPIO21 | SDA |
| GPIO22 | SCL |
| 3V3 | VCC (logic power) |
| GND | GND |
| External regulated 5 V, at least 3 A | V+ (servo power) |
| External supply ground | GND (shared with ESP32) |

Power the ESP32 through USB for testing. Do not feed servo power through the
ESP32. Leave address jumpers open for the default address `0x40`.
The board's OE input must be low for outputs to work; check your breakout's
pull-down arrangement. PWM disable is not an electrical power disconnect.

## Upload and use

1. Install the **esp32 by Espressif Systems** board package in Arduino IDE.
2. Install **Adafruit PWM Servo Driver Library** and its **Adafruit BusIO** dependency.
3. Open the sketch. If Arduino asks to move it into a matching
   `sesame-motor-tester` folder, accept. Select **ESP32 Dev Module** and your port.
4. Upload; open Serial Monitor at **115200 baud** with **Newline** enabled.
5. With servo horns removed, test one servo first using `0,90`. Outputs remain
   disabled after initialization until a movement command is received.

| Command | Effect |
| --- | --- |
| `0,90` | Motor 0 to 90 degrees |
| `7,90` | Motor 7 to 90 degrees |
| `all,90` | All eight motors to 90 degrees |
| `stop` | Disable PWM on all 16 channels; power remains connected |

Motor IDs 0–7 map to PCA9685 channels 0–7 respectively, in order:
**R1, R2, L1, L2, R4, R3, L3, L4**. Plug signal wires into the signal row;
check your board's markings for servo power and ground polarity.

The sketch uses 50 Hz and retains upstream's **732–2929 microsecond** mapping.
This is a wide range and may exceed your servos' limits. Check their specifications
and adjust `MIN_PULSE` / `MAX_PULSE` before exploring endpoints. Start with one
unloaded servo at 90 degrees. Do not mount horns until calibrated.

If the PCA9685 is not detected, the sketch refuses movement commands. Check
wiring, logic power and `PCA9685_ADDRESS`, then reset the ESP32.
