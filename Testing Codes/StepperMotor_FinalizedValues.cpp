
#define STEP_PIN 25
#define DIR_PIN  26
#define EN_PIN   27

#define STEPS_PER_REV 200

// ==================================
//         USER PARAMETERS
// ==================================

float a = 70.0;   // No.of Revolutions to reach Second bin from Home Bin
float b = 144.0;   // No.of Revolutions to reach Third bin from Home Bin
float c = 217.0;   // No.of Revolutions to reach Fourth bin from Home Bin

float maxSpeedRPM = 210.0;
float accelerationRPM = 20.0;

// Direction settings
#define AHEAD_DIR LOW
#define RETURN_DIR HIGH

// ==================================

// Function to move the motor
void moveMotor(float revolutions, int direction) {

  long totalSteps = revolutions * STEPS_PER_REV;

  // Convert RPM to steps per second
  float maxSpeed = (maxSpeedRPM * STEPS_PER_REV) / 60.0;

  // Convert RPM/s to steps/s^2
  float acceleration = (accelerationRPM * STEPS_PER_REV) / 60.0;

  digitalWrite(DIR_PIN, direction);
  delay(10);

  float currentSpeed = 0;
  unsigned long previousMicros = micros();

  long stepsDone = 0;

  while (stepsDone < totalSteps) {

    unsigned long currentMicros = micros();

    float elapsed =
      (currentMicros - previousMicros) / 1000000.0;

    previousMicros = currentMicros;

    long remainingSteps = totalSteps - stepsDone;

    // Calculate deceleration speed
    float stoppingSpeed =
      sqrt(2.0 * acceleration * remainingSteps);

    // Acceleration
    if (currentSpeed < maxSpeed &&
        currentSpeed < stoppingSpeed) {

      currentSpeed += acceleration * elapsed;

      if (currentSpeed > maxSpeed) {
        currentSpeed = maxSpeed;
      }
    }

    // Deceleration
    else if (currentSpeed > stoppingSpeed) {

      currentSpeed -= acceleration * elapsed;

      if (currentSpeed < stoppingSpeed) {
        currentSpeed = stoppingSpeed;
      }
    }

    if (currentSpeed < 1.0) {
      currentSpeed = 1.0;
    }

    unsigned long stepDelay =
      1000000.0 / currentSpeed;

    // Generate step pulse
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(5);

    digitalWrite(STEP_PIN, LOW);

    delayMicroseconds(stepDelay - 5);

    stepsDone++;
  }
}

// ==================================
//              SETUP
// ==================================

void setup() {

  Serial.begin(115200);

  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(EN_PIN, OUTPUT);

  digitalWrite(STEP_PIN, LOW);

  // Enable A4988
  digitalWrite(EN_PIN, LOW);

  Serial.println("Starting movement sequence");

  // A movement
  Serial.println("Moving ahead by A");
  moveMotor(a, AHEAD_DIR);

  delay(2000);

  Serial.println("Returning by A");
  moveMotor(a, RETURN_DIR);

  delay(5000);

  // B movement
  Serial.println("Moving ahead by B");
  moveMotor(b, AHEAD_DIR);

  delay(2000);

  Serial.println("Returning by B");
  moveMotor(b, RETURN_DIR);

  delay(5000);

  // C movement
  Serial.println("Moving ahead by C");
  moveMotor(c, AHEAD_DIR);

  delay(2000);

  Serial.println("Returning by C");
  moveMotor(c, RETURN_DIR);

  Serial.println("Sequence completed");

  // Disable driver
  digitalWrite(EN_PIN, HIGH);
}

// ==================================

void loop() {
  // Nothing here. Sequence executes once.
}
