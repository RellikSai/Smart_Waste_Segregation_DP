
import cv2
import torch
import torch.nn as nn
import torchvision.transforms as transforms
from torchvision import models
from PIL import Image
import numpy as np
import time
import os
from datetime import datetime

# ==========================================
# CONFIGURATION
# ==========================================

PI_CAMERA_URL = "http://10.106.94.31:5000/video"

MODEL_PATH = "ConvNeXt_best_model.pth"

CLASSES = [
    "metal",
    "organic",
    "paper_cardboard",
    "plastic"
]

DEVICE = torch.device("cpu")

# Keep disabled until ESP32 firmware protocol is verified
ENABLE_SERIAL = False
SERIAL_PORT = "COM8"
BAUD_RATE = 115200

SAVE_DIR = "captured_images"
os.makedirs(SAVE_DIR, exist_ok=True)

# ==========================================
# LOAD MODEL
# ==========================================

print("Loading ConvNeXt-Tiny...")

model = models.convnext_tiny(weights=None)

model.classifier[2] = nn.Linear(
    model.classifier[2].in_features,
    4
)

checkpoint = torch.load(
    MODEL_PATH,
    map_location=DEVICE,
    weights_only=True
)

if "model_state_dict" in checkpoint:
    state_dict = checkpoint["model_state_dict"]
elif "state_dict" in checkpoint:
    state_dict = checkpoint["state_dict"]
else:
    state_dict = checkpoint

state_dict = {
    k.replace("module.", "", 1) if k.startswith("module.") else k: v
    for k, v in state_dict.items()
}

model.load_state_dict(state_dict)
model.to(DEVICE)
model.eval()

print("Model loaded successfully!")

# ==========================================
# IMAGE PREPROCESSING
# ==========================================

transform = transforms.Compose([
    transforms.Resize((224, 224)),
    transforms.ToTensor(),
    transforms.Normalize(
        mean=[0.485, 0.456, 0.406],
        std=[0.229, 0.224, 0.225]
    )
])

# ==========================================
# SERIAL CONNECTION
# ==========================================

ser = None

if ENABLE_SERIAL:
    import serial

    ser = serial.Serial(
        SERIAL_PORT,
        BAUD_RATE,
        timeout=2
    )

    time.sleep(2)
    print("ESP32 connected on", SERIAL_PORT)

# ==========================================
# CLASSIFICATION
# ==========================================

def classify_image(frame):

    rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)

    image = Image.fromarray(rgb)

    image_tensor = transform(image).unsqueeze(0)

    start = time.time()

    with torch.no_grad():
        output = model(image_tensor)
        probabilities = torch.softmax(output, dim=1)

    confidence, predicted = torch.max(
        probabilities, dim=1
    )

    elapsed = time.time() - start

    predicted_class = CLASSES[predicted.item()]
    confidence_value = confidence.item() * 100

    return predicted_class, confidence_value, elapsed


# ==========================================
# CAMERA STREAM
# ==========================================

print("Connecting to Raspberry Pi camera...")

cap = cv2.VideoCapture(PI_CAMERA_URL)

if not cap.isOpened():
    print("ERROR: Cannot connect to Raspberry Pi camera!")
    print("Check the Pi IP and camera streaming server.")
    exit()

print("Raspberry Pi camera connected!")
print("Press S to classify waste.")
print("Press Q to quit.")

last_result = "Waiting for classification"

# ==========================================
# MAIN LOOP
# ==========================================

while True:

    ret, frame = cap.read()

    if not ret:
        print("Camera frame unavailable")
        time.sleep(0.1)
        continue

    display = frame.copy()

    cv2.putText(
        display,
        last_result,
        (10, 30),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.7,
        (0, 255, 0),
        2
    )

    cv2.putText(
        display,
        "S = Classify | Q = Quit",
        (10, 65),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.6,
        (255, 255, 255),
        2
    )

    cv2.imshow("Smart Waste Classification - RPi Camera", display)

    key = cv2.waitKey(1) & 0xFF

    if key == ord('s') or key == ord('S'):

        print("\nCapturing image...")

        filename = datetime.now().strftime(
            "garbage_%Y%m%d_%H%M%S.jpg"
        )

        filepath = os.path.join(SAVE_DIR, filename)

        cv2.imwrite(filepath, frame)

        predicted_class, confidence, elapsed = classify_image(frame)

        last_result = f"{predicted_class}: {confidence:.2f}%"

        print("--------------------------------")
        print("PREDICTION:", predicted_class)
        print("CONFIDENCE:", f"{confidence:.2f}%")
        print("INFERENCE TIME:", f"{elapsed:.3f} seconds")
        print("IMAGE SAVED:", filepath)

        # Send class name to ESP32 when enabled
        if ser is not None:
            command = predicted_class + "\n"
            ser.write(command.encode("utf-8"))
            print("ESP32 COMMAND SENT:", predicted_class)

        print("--------------------------------")

    elif key == ord('q') or key == ord('Q'):
        break

# ==========================================
# CLEANUP
# ==========================================

cap.release()
cv2.destroyAllWindows()

if ser is not None:
    ser.close()

print("Program terminated.")
