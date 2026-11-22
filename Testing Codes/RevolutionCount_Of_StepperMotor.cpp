#define STEP_PIN 25
#define DIR_PIN  26
#define EN_PIN   27

// Motor configuration
#define STEPS_PER_REV 200  // 1.8-degree motor, full-step mode

// ===== CHANGE THESE PARAMETERS =====

float revolutions = 144.0;
float maxSpeedRPM = 210.0;   // Maximum speed in RPM
float accelerationRPM = 20.0;  // Acceleration in RPM per second
int direction = HIGH;  // HIGH means Forwards | LOW means Backwards

// ===================================

void setup() {
  Serial.begin(115200);

  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(EN_PIN, OUTPUT);

  digitalWrite(STEP_PIN, LOW);
  digitalWrite(EN_PIN, LOW);  // Enable driver

  digitalWrite(DIR_PIN, direction);
  delay(10);

  long totalSteps = revolutions * STEPS_PER_REV;

  // Convert RPM to steps per second
  float maxSpeed = (maxSpeedRPM * STEPS_PER_REV) / 60.0;

  // Convert RPM/second to steps/second squared
  float acceleration = (accelerationRPM * STEPS_PER_REV) / 60.0;

  // Start at a low speed
  float currentSpeed = 0;
  unsigned long previousMicros = micros();

  long stepsDone = 0;

  Serial.println("Stepper movement started");

  while (stepsDone < totalSteps) {

    unsigned long currentMicros = micros();
    float elapsed = (currentMicros - previousMicros) / 1000000.0;
    previousMicros = currentMicros;

    // Remaining distance
    long remainingSteps = totalSteps - stepsDone;

    // Calculate speed needed to decelerate before target
    float stoppingSpeed = sqrt(2.0 * acceleration * remainingSteps);

    // Accelerate or decelerate
    if (currentSpeed < maxSpeed && currentSpeed < stoppingSpeed) {
      currentSpeed += acceleration * elapsed;

      if (currentSpeed > maxSpeed) {
        currentSpeed = maxSpeed;
      }
    } 
    else if (currentSpeed > stoppingSpeed) {
      currentSpeed -= acceleration * elapsed;

      if (currentSpeed < stoppingSpeed) {
        currentSpeed = stoppingSpeed;
      }
    }

    if (currentSpeed < 1.0) {
      currentSpeed = 1.0;
    }

    unsigned long stepDelay = 1000000.0 / currentSpeed;

    // Generate one step pulse
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(5);
    digitalWrite(STEP_PIN, LOW);

    delayMicroseconds(stepDelay - 5);

    stepsDone++;
  }

  Serial.println("Movement completed");

  // Disable driver
  digitalWrite(EN_PIN, HIGH);
}

void loop() {
  // Motor moves only once after startup
}
