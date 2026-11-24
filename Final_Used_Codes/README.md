# ♻️ Finally Used Achitecture for Smart Waste Segregation

## 🌱 How Does the System Work?

The Smart Waste Segregation System combines **Machine Learning, Computer Vision, Embedded Electronics and Mechanical Automation** to automatically identify and segregate waste into four categories:

- 🔩 Metal
- 🌿 Organic
- 📦 Paper / Cardboard
- 🧴 Plastic

The system operates through two main stages: **waste identification and decision-making**, followed by **electronic control and mechanical segregation**.


## 🔍 1. Image Acquisition and Waste Classification

An Android smartphone is used as the camera through **Iriun Webcam**, which transmits a live video feed to the Windows laptop.

When an image is captured, the laptop processes it using the trained **ConvNeXt-Tiny deep learning model**. The model analyzes the visual characteristics of the waste item and predicts its category among the four trained classes: Metal, Organic, Paper/Cardboard and Plastic.

The laptop therefore acts as the image-processing and decision-making unit of the system.

## 🔌 2. Communication Between Devices

Once the waste category is identified, the laptop sends the predicted class to the ESP32 through **USB serial communication**.

The ESP32 acts as the main embedded controller of the mechanical segregation system. It receives the classification result and determines the corresponding position of the waste bin.

This communication establishes a connection between the machine learning-based identification system and the physical sorting mechanism.

## ⚡ 3. Electrical Working and Motor Control

The ESP32 sends step and direction signals to the **A4988 stepper motor driver**, which controls the NEMA 17 bipolar stepper motor.

The A4988 receives electrical power from an external motor power supply and regulates the current supplied to the motor windings. The ESP32 provides the control signals required to determine the motor's direction, speed and movement.

The NEMA 17 motor drives a ball screw mechanism, converting rotational motion into controlled linear movement. This moves the sorting assembly towards the selected waste compartment.

The laptop handles classification, while the ESP32 and A4988 handle the electrical control required for physical segregation.

## 🗑️ 4. Automated Waste Segregation

Each waste category is assigned a specific position along the ball screw mechanism.

| Waste Category | Assigned Position |
|---|---|
| 🧴 Plastic | Bin 1 - Home Position |
| 🔩 Metal | Bin 2 – 70 revolutions |
| 🌿 Organic | Bin 3 – 144 revolutions |
| 📦 Paper / Cardboard | Bin 4 – 217 revolutions |

After receiving the classification result, the ESP32 moves the sorting assembly to the corresponding bin position. Once the selected position is reached, the mechanism pauses for **2 seconds** to allow the waste to be deposited.

After this pause, the stepper motor reverses its movement and brings the assembly back to its home position, ready for the next waste item.

## 📡 5. Ultrasonic-Based Garbage Fill Measurement

The system uses an **HC-SR04 ultrasonic sensor** to measure the amount of garbage accumulated inside the collection bin.

The sensor works on the principle of ultrasonic wave reflection:

1. It transmits high-frequency ultrasonic sound waves towards the garbage surface.
2. These waves reflect after hitting the surface.
3. The sensor measures the time taken for the reflected waves to return.
4. The measured distance is used to estimate how much space remains inside the bin and calculate its fill level.

This provides a way to monitor garbage accumulation without requiring physical contact with the waste.

## 🔄 Complete Working Sequence

<details>
<summary><strong>Step 1 – Capture the waste image</strong></summary>

The Android smartphone provides the live camera feed to the laptop through Iriun Webcam. An image is captured to initiate classification.

</details>

<details>
<summary><strong>Step 2 – Identify the waste category</strong></summary>

The ConvNeXt-Tiny model analyzes the captured image and predicts whether the item is Metal, Organic, Paper/Cardboard or Plastic.

</details>

<details>
<summary><strong>Step 3 – Transmit the classification result</strong></summary>

The laptop sends the predicted waste category to the ESP32 through USB serial communication.

</details>

<details>
<summary><strong>Step 4 – Move towards the selected bin</strong></summary>

The ESP32 controls the A4988 driver, which operates the NEMA 17 stepper motor and ball screw mechanism to position the sorting assembly.

</details>

<details>
<summary><strong>Step 5 – Deposit the waste and return home</strong></summary>

The mechanism pauses for two seconds at the selected bin and then returns to its home position, completing the segregation cycle.

</details>

<details>
<summary><strong>Step 6 – Monitor garbage fill level</strong></summary>

The HC-SR04 ultrasonic sensor measures the distance to the garbage surface, allowing the bin's fill level to be estimated.

</details>

## 🧩 Main Components and Their Roles

| Component | Function |
|---|---|
| Android Smartphone | Captures waste images |
| Windows Laptop | Runs the waste classification model |
| ConvNeXt-Tiny | Identifies the waste category |
| ESP32 DevKit V1 | Receives classification and controls movement |
| A4988 Driver | Controls the stepper motor |
| NEMA 17 Stepper Motor | Provides rotational movement |
| Ball Screw Mechanism | Converts rotation into linear movement |
| HC-SR04 Ultrasonic Sensor | Measures garbage fill level |
| External Motor Power Supply | Supplies power to the stepper motor driver |

## 💡 Overall Working Principle

The project establishes a link between intelligent waste recognition and physical waste segregation. Computer vision identifies the type of waste, USB serial communication transfers the decision to the embedded controller, and the motor-control circuit executes the required mechanical movement. An ultrasonic sensor independently measures garbage accumulation inside the collection bin.

Through the integration of these components, the system demonstrates how machine learning and electronic automation can be used together to simplify waste segregation and support more efficient waste management.

---

<p align="center">
  <strong>♻️ Identify. Classify. Segregate. 🌍</strong>
</p>
