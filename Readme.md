# ♻️ Smart Waste Segregation System

### An ML-powered automated waste segregation prototype

![Python](https://img.shields.io/badge/Python-3776AB?style=for-the-badge&logo=python&logoColor=white)
![PyTorch](https://img.shields.io/badge/PyTorch-EE4C2C?style=for-the-badge&logo=pytorch&logoColor=white)
![ESP32](https://img.shields.io/badge/ESP32-E7352C?style=for-the-badge&logo=espressif&logoColor=white)
![OpenCV](https://img.shields.io/badge/OpenCV-5C3EE8?style=for-the-badge&logo=opencv&logoColor=white)

## 🌱 Overview

The **Smart Waste Segregation System** is a Design Practicum project developed at **IIT Mandi** to explore an automated approach to waste management.

The system combines machine learning, computer vision, embedded systems and mechanical automation to identify and segregate waste into four categories: **Metal, Organic, Paper/Cardboard and Plastic**.

Our aim is to reduce dependence on manual waste segregation by developing a prototype that can recognize waste materials and physically direct them into their designated collection compartments.

## 👥 Team Members

This project was initially undertaken as a collaborative team effort as part of the Design Practicum course.

| Team Member | Contact(Github or LinkedIn) |
|---|---|
| Member 1 – Name | Role / Contribution |
| Member 2 – Name | Role / Contribution |
| Member 3 – Name | Role / Contribution |
| Member 4 – Name | Role / Contribution |

*Replace the placeholders with the team members' names and their respective contributions.*

## 🎯 Project Objectives

- Develop a machine learning model to classify different types of waste.
- Integrate computer vision with embedded hardware.
- Design a motor-driven mechanical sorting mechanism.
- Automatically direct identified waste into designated compartments.
- Explore sensor integration for object detection and bin-level monitoring.
- Build a foundation for smarter and more efficient waste management.

## 📊 Dataset and Machine Learning

A custom dataset containing **22,138 images** was prepared for training the waste classification model.

<details>
<summary><strong>📂 View dataset distribution</strong></summary>

| Waste Category | Number of Images |
|---|---:|
| Metal | 5,527 |
| Organic | 5,523 |
| Paper/Cardboard | 5,646 |
| Plastic | 5,442 |
| **Total** | **22,138** |

</details>

🔗 **Explore the dataset:** [Garbage Segregation Dataset – Kaggle](https://www.kaggle.com/datasets/saiprasanthsnaidu/garbage-segregation-dataset/data)

The project uses **ConvNeXt-Tiny**, a deep learning architecture adapted through transfer learning for four-class waste classification.

The trained model analyzes captured images and predicts one of the following categories:

<details>
<summary><strong>♻️ Four waste categories</strong></summary>

- 🔩 **Metal** – Metallic waste and containers
- 🌿 **Organic** – Food scraps and biodegradable waste
- 📦 **Paper/Cardboard** – Paper products and cardboard packaging
- 🧴 **Plastic** – Plastic bottles, containers and packaging

</details>

## ⚙️ System Architecture

The current implementation uses a laptop-based image classification system connected to an ESP32-controlled mechanical sorting assembly.

**The overall workflow:**

1. **Image Acquisition:** An Android smartphone provides the camera feed through Iriun Webcam.
2. **Waste Classification:** The laptop runs the trained ConvNeXt-Tiny model to identify the waste category.
3. **Communication:** The predicted category is transmitted to the ESP32 through USB serial communication.
4. **Mechanical Movement:** The ESP32 controls an A4988 driver and NEMA 17 stepper motor to position the sorting mechanism.
5. **Waste Segregation:** The mechanism directs the waste towards its designated collection compartment and returns to its home position.

## 🔧 Hardware and Technologies

<details>
<summary><strong>🛠️ Hardware components</strong></summary>

| Component | Purpose |
|---|---|
| ESP32 DevKit V1 | Embedded control |
| NEMA 17 Stepper Motor | Mechanical positioning |
| A4988 Driver | Stepper motor control |
| Ball Screw Mechanism | Linear movement |
| Android Smartphone | Image acquisition |
| Raspberry Pi 4 | Explored in the initial architecture |
| Raspberry Pi Camera Module v2 | Initial camera setup |
| MLX90614 | Non-contact temperature measurement |
| HC-SR04 Ultrasonic Sensors | Object detection and bin-level monitoring |
| MG90S Servo Motors | Proposed shared trapdoor mechanism |

</details>

<details>
<summary><strong>💻 Software and technologies</strong></summary>

- Python
- C++
- PyTorch and Torchvision
- ConvNeXt-Tiny
- OpenCV
- ESP32 / Arduino IDE
- Iriun Webcam
- USB Serial Communication
- Streamlit (planned monitoring interface)

</details>

## 🚀 Future Scope

The project can be further improved through:

- Automatic waste detection without manual triggering.
- Integration of ultrasonic sensors for real-time bin-fill monitoring.
- Reliable automated trapdoor operation.
- Integration of thermal sensing.
- A Streamlit-based monitoring dashboard.
- Automatic homing and improved mechanical positioning.
- Expansion to additional waste categories.

## 🎓 Project Information

| Detail | Description |
|---|---|
| Project | Smart Waste Segregation System |
| Course | Design Practicum |
| Institution | Indian Institute of Technology Mandi |
| Domain | Machine Learning, Computer Vision, Embedded Systems and Mechanical Automation |

---

<p align="center">
  <strong>Building smarter solutions for a cleaner and more sustainable future. 🌍</strong>
</p>
