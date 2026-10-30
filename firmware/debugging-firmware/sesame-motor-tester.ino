#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// Arduino IDE: select ESP32 Dev Module; install Adafruit PWM Servo Driver Library
// (and its Adafruit BusIO dependency). Serial Monitor: 115200 baud, Newline.
// ESP32 GPIO21 -> SDA, GPIO22 -> SCL, 3V3 -> VCC, GND -> GND.
// External regulated 5V (at least 3A) -> V+; share ground with the ESP32.
// Do not power the servos from the ESP32's 3V3 pin or USB supply.
// Keep servo horns off for initial calibration. `stop` removes PWM, not power.

// ======================================================================
// --- CONFIGURATION ---
// ======================================================================

const uint8_t I2C_SDA = 21;
const uint8_t I2C_SCL = 22;
const uint8_t PCA9685_ADDRESS = 0x40; // Default with address jumpers open
const uint16_t SERVO_FREQUENCY = 50;
Adafruit_PWMServoDriver pwm(PCA9685_ADDRESS);
bool driverReady = false;

// Motor ID -> PCA9685 channel (not ESP32 GPIO).
// Order matches the main firmware: R1, R2, L1, L2, R4, R3, L3, L4.
const uint8_t servoChannels[8] = {0, 1, 2, 3, 4, 5, 6, 7};

// Initial positional-servo range in microseconds: 90 degrees = 1500us.
// Match these values to the main firmware. Verify travel with horns removed;
// adjust for your particular servos before attaching joints.
const int MIN_PULSE = 1000;
const int MAX_PULSE = 2000;

// ======================================================================
// --- SETUP ---
// ======================================================================

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("-----------------------------------");
  Serial.println("   Sesame Motor Tester Interface   ");
  Serial.println("-----------------------------------");
  Serial.println("Commands:");
  Serial.println("1. id,angle   -> e.g. '0,90'");
  Serial.println("2. all,angle  -> e.g. 'all,90'");
  Serial.println("3. stop       -> Disables PWM signals (servo power stays on)");
  Serial.println("-----------------------------------");
  Wire.begin(I2C_SDA, I2C_SCL);
  if (!pwm.begin()) {
    Serial.println("Error: PCA9685 not found. Check power, wiring and address; reset to retry.");
    return;
  }

  // Clear ALL outputs before accepting commands, including unused channels.
  for (uint8_t channel = 0; channel < 16; channel++) {
    pwm.setPWM(channel, 0, 4096); // PCA9685 full-OFF bit
  }
  pwm.setPWMFreq(SERVO_FREQUENCY);
  delay(10);
  driverReady = true;
  Serial.println("Status: PWM outputs disabled; no movement until commanded.");
}

// ======================================================================
// --- MAIN LOOP ---
// ======================================================================

void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim(); // Remove whitespace/newlines

    if (input.length() == 0) return;

    // --- Check for STOP command ---
    if (input.equalsIgnoreCase("stop")) {
      stopMotors();
      return;
    }

    // --- Parse "key,value" format ---
    int commaIndex = input.indexOf(',');
    
    if (commaIndex != -1) {
      String cmd = input.substring(0, commaIndex);
      String valStr = input.substring(commaIndex + 1);
      int angle = valStr.toInt();

      // Check Angle Limits
      if (angle < 0) angle = 0;
      if (angle > 180) angle = 180;

      // Check if command is "all" or a motor ID
      if (cmd.equalsIgnoreCase("all")) {
        moveAll(angle);
      } else {
        // Assume it's a number
        int motorId = cmd.toInt();
        // Basic check to ensure "0" wasn't actually text garbage
        if (motorId == 0 && cmd.charAt(0) != '0') {
           Serial.println("Error: Invalid Motor ID");
        } else {
           moveMotor(motorId, angle);
        }
      }
      
    } else {
      Serial.println("Error: Invalid format. Use 'id,angle', 'all,angle', or 'stop'.");
    }
  }
}

// ======================================================================
// --- HELPER FUNCTIONS ---
// ======================================================================

void moveMotor(int id, int angle) {
  if (id < 0 || id > 7) {
    Serial.println("Error: Motor ID must be 0-7");
    return;
  }

  if (!driverReady) {
    Serial.println("Error: PCA9685 unavailable; check wiring and reset.");
    return;
  }

  const int pulseUs = map(constrain(angle, 0, 180), 0, 180, MIN_PULSE, MAX_PULSE);
  // Library converts microseconds using the configured PCA9685 frequency.
  pwm.writeMicroseconds(servoChannels[id], pulseUs);
  
  Serial.print("OK: Motor ");
  Serial.print(id);
  Serial.print(" -> ");
  Serial.println(angle);
}

void moveAll(int angle) {
  Serial.print("Moving ALL to ");
  Serial.println(angle);
  for (int i = 0; i < 8; i++) {
    moveMotor(i, angle);
  }
}

void stopMotors() {
  if (!driverReady) {
    Serial.println("Error: PCA9685 unavailable; check wiring and reset.");
    return;
  }
  for (uint8_t channel = 0; channel < 16; channel++) {
    pwm.setPWM(channel, 0, 4096);
  }
  Serial.println("PWM signals disabled. Servo power is still connected.");
}
