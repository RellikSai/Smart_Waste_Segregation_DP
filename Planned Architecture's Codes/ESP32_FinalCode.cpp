
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <ESP32Servo.h>
#include <AccelStepper.h>

// =====================================================
// 1. PIN CONFIGURATION
// =====================================================

// Inlet HC-SR04
#define INLET_TRIG 32
#define INLET_ECHO 33

// A4988 stepper driver
#define STEP_PIN 25
#define DIR_PIN 26
#define ENABLE_PIN 27

// Two MG90S servos operating one shared trapdoor
#define SERVO_1_PIN 18
#define SERVO_2_PIN 19

// Raspberry Pi UART2
#define PI_RX_PIN 16
#define PI_TX_PIN 17

// MLX90614 uses I2C
#define I2C_SDA 21
#define I2C_SCL 22

// =====================================================
// 2. CONFIGURATION
// =====================================================

#define SERIAL_BAUD 115200
#define PI_BAUD 115200

// Object detection distances
const float OBJECT_DETECT_DISTANCE_CM = 15.0;
const float OBJECT_CLEAR_DISTANCE_CM = 25.0;

// Ultrasonic timeout
const unsigned long ULTRASONIC_TIMEOUT_US = 30000;

// Safety timeouts
const unsigned long PI_RESPONSE_TIMEOUT_MS = 60000;
const unsigned long MOTOR_TIMEOUT_MS = 30000;

// Calibrated motor positions
const bool MOTION_CALIBRATED = true;

const long BIN_POSITION_STEPS[4] = {
  0,    // Bin 1
  70,   // Bin 2
  140,  // Bin 3
  220   // Bin 4
};

// A4988 speed settings
const float MOTOR_MAX_SPEED = 210.0;
const float MOTOR_ACCELERATION = 20.0;

// Servo angles
const int SERVO_1_CLOSED = 90;
const int SERVO_1_OPEN = 20;

const int SERVO_2_CLOSED = 90;
const int SERVO_2_OPEN = 160;

// =====================================================
// 3. OBJECTS
// =====================================================

HardwareSerial PiSerial(2);

Adafruit_MLX90614 mlx = Adafruit_MLX90614();

Servo trapdoorServo1;
Servo trapdoorServo2;

AccelStepper stepper(
  AccelStepper::DRIVER,
  STEP_PIN,
  DIR_PIN
);

// =====================================================
// 4. VARIABLES
// =====================================================

bool objectDetected = false;
bool sortingInProgress = false;

float currentTemperature = 0.0;
float inletDistance = 0.0;

float binDistances[4] = {0, 0, 0, 0};
float binFillPercentages[4] = {0, 0, 0, 0};

unsigned long lastDetectionTime = 0;

// =====================================================
// 5. ULTRASONIC READING
// =====================================================

float readUltrasonicCM() {

  digitalWrite(INLET_TRIG, LOW);
  delayMicroseconds(3);

  digitalWrite(INLET_TRIG, HIGH);
  delayMicroseconds(10);

  digitalWrite(INLET_TRIG, LOW);

  unsigned long duration = pulseIn(
    INLET_ECHO,
    HIGH,
    ULTRASONIC_TIMEOUT_US
  );

  if (duration == 0) {
    return -1.0;
  }

  return duration * 0.0343 / 2.0;
}

// =====================================================
// 6. THERMAL SENSOR
// =====================================================

bool readTemperature() {

  currentTemperature = mlx.readObjectTempC();

  if (isnan(currentTemperature)) {
    Serial.println("ERROR: MLX90614 reading failed");
    return false;
  }

  Serial.print("Object temperature: ");
  Serial.print(currentTemperature);
  Serial.println(" C");

  return true;
}

// =====================================================
// 7. SEND MESSAGE TO RASPBERRY PI
// =====================================================

void sendToPi(String message) {

  PiSerial.println(message);

  Serial.print("ESP32 -> PI: ");
  Serial.println(message);
}

// =====================================================
// 8. RECEIVE MESSAGE FROM RASPBERRY PI
// =====================================================

String receiveFromPi(unsigned long timeoutMs) {

  unsigned long startTime = millis();

  while (millis() - startTime < timeoutMs) {

    if (PiSerial.available()) {

      String message = PiSerial.readStringUntil('\n');
      message.trim();

      if (message.length() > 0) {

        Serial.print("PI -> ESP32: ");
        Serial.println(message);

        return message;
      }
    }

    delay(5);
  }

  return "";
}

// =====================================================
// 9. WAIT FOR CLASSIFICATION
// =====================================================

int waitForBinNumber() {

  unsigned long startTime = millis();

  while (millis() - startTime < PI_RESPONSE_TIMEOUT_MS) {

    if (PiSerial.available()) {

      String message = PiSerial.readStringUntil('\n');
      message.trim();

      Serial.print("PI -> ESP32: ");
      Serial.println(message);

      if (message.startsWith("GO,")) {

        int binNumber = message.substring(3).toInt();

        if (binNumber >= 1 && binNumber <= 4) {
          return binNumber;
        }
      }

      if (message.startsWith("ERROR")) {
        Serial.println("PI reported a classification error");
        return -1;
      }
    }

    delay(5);
  }

  Serial.println("ERROR: Raspberry Pi response timeout");

  return -1;
}

// =====================================================
// 10. MOVE STEPPER TO SELECTED BIN
// =====================================================

bool moveToBin(int binNumber) {

  if (binNumber < 1 || binNumber > 4) {
    return false;
  }

  if (!MOTION_CALIBRATED) {

    Serial.println("MOTOR DISABLED: calibrate bin positions first");
    return false;
  }

  long targetPosition = BIN_POSITION_STEPS[binNumber - 1];

  Serial.print("Moving to bin ");
  Serial.println(binNumber);

  // Enable A4988 (active LOW)
  digitalWrite(ENABLE_PIN, LOW);

  stepper.moveTo(targetPosition);

  unsigned long startTime = millis();

  while (stepper.distanceToGo() != 0) {

    stepper.run();

    if (millis() - startTime > MOTOR_TIMEOUT_MS) {

      stepper.stop();

      // Disable driver after timeout
      digitalWrite(ENABLE_PIN, HIGH);

      Serial.println("ERROR: Stepper movement timeout");

      return false;
    }

    delay(1);
  }

  // Disable A4988 after reaching target
  digitalWrite(ENABLE_PIN, HIGH);

  Serial.println("Stepper reached target position");

  return true;
}

// =====================================================
// 11. TRAPDOOR CONTROL
// =====================================================

void closeTrapdoor() {

  Serial.println("Closing trapdoor");

  trapdoorServo1.write(SERVO_1_CLOSED);
  trapdoorServo2.write(SERVO_2_CLOSED);

  delay(1000);
}

void openTrapdoor() {

  Serial.println("Opening trapdoor");

  trapdoorServo1.write(SERVO_1_OPEN);
  trapdoorServo2.write(SERVO_2_OPEN);

  delay(1500);
}

// =====================================================
// 12. READ BIN LEVELS RECEIVED FROM RASPBERRY PI
// =====================================================

// Expected response:
// LEVELS,d1,d2,d3,d4,p1,p2,p3,p4
//
// d = distance in cm
// p = fill percentage calculated by Raspberry Pi

bool parseBinLevels(String message) {

  if (!message.startsWith("LEVELS,")) {
    return false;
  }

  message.remove(0, 7);

  float values[8];

  for (int i = 0; i < 8; i++) {

    int commaPosition = message.indexOf(',');

    String token;

    if (i < 7) {

      if (commaPosition < 0) {
        return false;
      }

      token = message.substring(0, commaPosition);
      message = message.substring(commaPosition + 1);

    } else {

      if (commaPosition >= 0) {
        return false;
      }

      token = message;
    }

    values[i] = token.toFloat();
  }

  for (int i = 0; i < 4; i++) {

    binDistances[i] = values[i];
    binFillPercentages[i] = values[i + 4];

    if (binDistances[i] < 0 ||
        binFillPercentages[i] < 0 ||
        binFillPercentages[i] > 100) {

      return false;
    }
  }

  Serial.println("Updated bin-level readings:");

  for (int i = 0; i < 4; i++) {

    Serial.print("Bin ");
    Serial.print(i + 1);

    Serial.print(" | Distance: ");
    Serial.print(binDistances[i]);

    Serial.print(" cm | Fill: ");
    Serial.print(binFillPercentages[i]);

    Serial.println("%");
  }

  return true;
}

// =====================================================
// 13. REQUEST BIN LEVELS FROM RASPBERRY PI
// =====================================================

void requestBinLevels(int binNumber) {

  sendToPi("DONE," + String(binNumber));

  sendToPi("READ_LEVELS");

  Serial.println("Waiting for Raspberry Pi bin readings...");

  unsigned long startTime = millis();

  while (millis() - startTime < PI_RESPONSE_TIMEOUT_MS) {

    if (PiSerial.available()) {

      String message = PiSerial.readStringUntil('\n');
      message.trim();

      Serial.print("PI -> ESP32: ");
      Serial.println(message);

      if (parseBinLevels(message)) {
        Serial.println("Bin monitoring data received");
        return;
      }

      if (message.startsWith("ERROR")) {
        Serial.println("ERROR: Raspberry Pi could not read bin levels");
        return;
      }
    }

    delay(5);
  }

  Serial.println("ERROR: Bin-level response timeout");
}

// =====================================================
// 14. COMPLETE SORTING CYCLE
// =====================================================

void sortGarbage(int binNumber) {

  sortingInProgress = true;

  Serial.println();
  Serial.println("========== SORTING STARTED ==========");

  // Move to the selected bin
  if (!moveToBin(binNumber)) {

    sendToPi("ERROR,MOTOR");
    sortingInProgress = false;

    return;
  }

  // Open shared trapdoor
  openTrapdoor();

  // Allow garbage to fall
  delay(1500);

  // Close shared trapdoor
  closeTrapdoor();

  // Tell Pi sorting cycle is complete and request bin readings
  requestBinLevels(binNumber);

  Serial.println("========== SORTING COMPLETED ==========");
  Serial.println();

  sortingInProgress = false;
}

// =====================================================
// 15. DETECT INCOMING GARBAGE
// =====================================================

void monitorInlet() {

  inletDistance = readUltrasonicCM();

  if (inletDistance < 0) {
    return;
  }

  Serial.print("Inlet distance: ");
  Serial.print(inletDistance);
  Serial.println(" cm");

  // Trigger only once while an object remains present
  if (!objectDetected &&
      inletDistance > 0 &&
      inletDistance <= OBJECT_DETECT_DISTANCE_CM) {

    objectDetected = true;

    Serial.println("GARBAGE DETECTED!");

    // Read thermal sensor
    if (!readTemperature()) {
      sendToPi("ERROR,TEMPERATURE");
      return;
    }

    // Send classification request
    sendToPi("ITEM," + String(currentTemperature, 2));

    // Wait for Pi classification
    int selectedBin = waitForBinNumber();

    if (selectedBin >= 1 && selectedBin <= 4) {

      Serial.print("Selected bin: ");
      Serial.println(selectedBin);

      sortGarbage(selectedBin);

    } else {

      Serial.println("Sorting cancelled: invalid Pi response");
    }

    lastDetectionTime = millis();
  }

  // Rearm after the object has moved away
  if (objectDetected &&
      inletDistance >= OBJECT_CLEAR_DISTANCE_CM) {

    objectDetected = false;

    Serial.println("Inlet clear. Ready for next item.");
  }
}

// =====================================================
// 16. SETUP
// =====================================================

void setup() {

  Serial.begin(SERIAL_BAUD);

  delay(1000);

  Serial.println();
  Serial.println("==================================");
  Serial.println("SMART WASTE SEGREGATION BIN");
  Serial.println("ESP32 CONTROLLER");
  Serial.println("==================================");

  // Ultrasonic
  pinMode(INLET_TRIG, OUTPUT);
  pinMode(INLET_ECHO, INPUT);

  digitalWrite(INLET_TRIG, LOW);

  // UART with Raspberry Pi
  PiSerial.begin(
    PI_BAUD,
    SERIAL_8N1,
    PI_RX_PIN,
    PI_TX_PIN
  );

  // MLX90614
  Wire.begin(I2C_SDA, I2C_SCL);

  if (!mlx.begin()) {

    Serial.println("ERROR: MLX90614 not detected");

  } else {

    Serial.println("MLX90614 connected");
  }

  // Servos
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);

  trapdoorServo1.setPeriodHertz(50);
  trapdoorServo2.setPeriodHertz(50);

  trapdoorServo1.attach(SERVO_1_PIN, 500, 2400);
  trapdoorServo2.attach(SERVO_2_PIN, 500, 2400);

  closeTrapdoor();

  // Stepper
  pinMode(ENABLE_PIN, OUTPUT);

  // A4988 ENABLE is active LOW
  digitalWrite(ENABLE_PIN, HIGH);

  stepper.setMaxSpeed(MOTOR_MAX_SPEED);
  stepper.setAcceleration(MOTOR_ACCELERATION);

  // Assumes carriage is physically positioned at bin 1
  stepper.setCurrentPosition(0);

  Serial.println("ESP32 initialization complete");

  if (!MOTION_CALIBRATED) {
    Serial.println("WARNING: Motor movement is disabled.");
    Serial.println("Calibrate bin positions before enabling motion.");
  }

  Serial.println("Monitoring inlet...");
}

// =====================================================
// 17. MAIN LOOP
// =====================================================

void loop() {

  if (!sortingInProgress) {

    monitorInlet();
  }

  delay(50);
}
