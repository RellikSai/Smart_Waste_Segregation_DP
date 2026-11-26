
#include <Arduino.h>
#include <AccelStepper.h>

// =====================================================
// 1. PIN CONFIGURATION
// =====================================================

#define STEP_PIN 25
#define DIR_PIN  26
#define EN_PIN   27

#define STEPS_PER_REV 200

AccelStepper stepper(
  AccelStepper::DRIVER,
  STEP_PIN,
  DIR_PIN
);


// =====================================================
// 2. BIN MOVEMENT DISTANCES
// =====================================================

// Distance from HOME in revolutions

float A = 70.0;    // Metal
float B = 144.0;   // Organic
float C = 217.0;   // Paper/Cardboard
float D = 0.0;     // Plastic = HOME


// =====================================================
// 3. MOTOR PARAMETERS
// =====================================================

float maxSpeedRPM = 210.0;
float accelerationRPM = 20.0;

// Convert RPM into steps per second

float maxSpeedSteps =
  (maxSpeedRPM * STEPS_PER_REV) / 60.0;

float accelerationSteps =
  (accelerationRPM * STEPS_PER_REV) / 60.0;


// =====================================================
// 4. MOVE TO BIN AND RETURN HOME
// =====================================================

void moveToBin(float revolutions, String binName) {

  Serial.print("MOVING TO BIN: ");
  Serial.println(binName);

  long targetSteps =
    (long)(revolutions * STEPS_PER_REV);

  if (targetSteps == 0) {

    Serial.println("BIN IS AT HOME");

    delay(2000);

    Serial.println("DONE");
    return;
  }

  // Move from home to selected bin
  stepper.moveTo(targetSteps);

  stepper.runToPosition();

  Serial.println("BIN REACHED");

  // Wait for waste to drop
  delay(2000);

  // Return to home
  Serial.println("RETURNING HOME");

  stepper.moveTo(0);

  stepper.runToPosition();

  Serial.println("HOME REACHED");

  Serial.println("DONE");
}


// =====================================================
// 5. CLASSIFY RECEIVED COMMAND
// =====================================================

void processWaste(String wasteClass) {

  wasteClass.trim();
  wasteClass.toLowerCase();

  Serial.print("RECEIVED: ");
  Serial.println(wasteClass);

  if (wasteClass == "metal") {

    moveToBin(A, "METAL");

  }

  else if (wasteClass == "organic") {

    moveToBin(B, "ORGANIC");

  }

  else if (wasteClass == "paper_cardboard") {

    moveToBin(C, "PAPER_CARDBOARD");

  }

  else if (wasteClass == "plastic") {

    moveToBin(D, "PLASTIC");

  }

  else {

    Serial.println("ERROR: UNKNOWN CLASS");

  }
}


// =====================================================
// 6. SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  // Enable motor driver
  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, LOW);

  stepper.setMaxSpeed(maxSpeedSteps);
  stepper.setAcceleration(accelerationSteps);

  stepper.setMinPulseWidth(5);

  // Positive direction = DIR HIGH
  // Negative direction = DIR LOW
  stepper.setPinsInverted(false, false, false);

  // Assume motor is physically at HOME
  stepper.setCurrentPosition(0);

  delay(1000);

  Serial.println("SMART WASTE BIN ESP32");
  Serial.println("READY");
}


// =====================================================
// 7. MAIN LOOP
// =====================================================

void loop() {

  if (Serial.available()) {

    String command =
      Serial.readStringUntil('\n');

    command.trim();

    if (command.length() > 0) {

      processWaste(command);

    }
  }
}
