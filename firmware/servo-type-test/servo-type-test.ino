#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// Classic ESP32 + PCA9685: GPIO21=SDA, GPIO22=SCL, 3V3=VCC.
// External regulated 5V -> V+, with supply/ESP32/PCA9685 grounds shared.
// Install Adafruit PWM Servo Driver Library and Adafruit BusIO.
// REMOVE ALL SERVO HORNS / LINKAGES before applying servo power.
// Serial Monitor: 115200 baud, Newline. Send "test" to test channels 0-7.
// Send "stop" to disable signals immediately. This does NOT disconnect power.

Adafruit_PWMServoDriver pwm(0x40);
const uint16_t pulses[] = {1500, 1400, 1500, 1600, 1500};
const uint32_t STEP_MS = 1000;
const uint8_t MOTOR_COUNT = 8;
bool ready = false;
bool running = false;
uint8_t motor = 0;
uint8_t stepIndex = 0;
uint32_t stepStarted = 0;
char command[16];
uint8_t commandLength = 0;
bool commandOverflow = false;

void disableOutputs() {
  running = false;
  for (uint8_t channel = 0; channel < 16; channel++) {
    pwm.setPWM(channel, 0, 4096);
  }
}

void sendStep() {
  pwm.writeMicroseconds(motor, pulses[stepIndex]);
  Serial.print("Channel ");
  Serial.print(motor);
  Serial.print(" -> ");
  Serial.print(pulses[stepIndex]);
  Serial.println(" us (1 second)");
  stepStarted = millis();
}

void handleCommand() {
  if (strcmp(command, "stop") == 0) {
    disableOutputs();
    Serial.println("PWM disabled. Servo power is still connected.");
  } else if (strcmp(command, "test") == 0) {
    disableOutputs();
    motor = 0;
    stepIndex = 0;
    running = true;
    Serial.println("Testing all 8, ONE AT A TIME. Observe each channel.");
    sendStep();
  } else {
    Serial.println("Send test or stop (lowercase), with Newline.");
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  if (!pwm.begin()) {
    Serial.println("PCA9685 not found at 0x40. Check wiring and reset.");
    return;
  }
  disableOutputs();
  pwm.setPWMFreq(50);
  delay(10);
  ready = true;
  Serial.println("Outputs disabled. REMOVE HORNS, then send test.");
  Serial.println("Each channel: 1500, 1400, 1500, 1600, 1500 us; 1s each.");
  Serial.println("Continuous servo: rotates at 1400/1600 in opposite directions.");
  Serial.println("Positional servo: moves a small amount and HOLDS each position.");
  Serial.println("1500us is approximate neutral; continuous servos may creep.");
}

void loop() {
  if (!ready) return;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (commandOverflow) {
        Serial.println("Command too long. Send test or stop.");
      } else if (commandLength) {
        command[commandLength] = '\0';
        handleCommand();
      }
      commandLength = 0;
      commandOverflow = false;
    } else if (commandLength < sizeof(command) - 1) {
      command[commandLength++] = c;
    } else {
      commandOverflow = true;
    }
  }

  if (!running || millis() - stepStarted < STEP_MS) return;
  stepIndex++;
  if (stepIndex == sizeof(pulses) / sizeof(pulses[0])) {
    pwm.setPWM(motor, 0, 4096);
    motor++;
    stepIndex = 0;
    if (motor == MOTOR_COUNT) {
      disableOutputs();
      Serial.println("Finished all 8. Outputs disabled. Send test to repeat.");
      return;
    }
  }
  sendStep();
}
