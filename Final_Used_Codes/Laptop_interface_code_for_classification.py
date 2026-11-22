
import cv2
import torch
import torch.nn as nn
import torchvision.transforms as transforms
import torchvision.models as models

import serial
import serial.tools.list_ports

import os
import time
from datetime import datetime


# =====================================================
# 1. CONFIGURATION
# =====================================================

MODEL_PATH = "ConvNeXt_best_model.pth"

# Iriun Webcam camera index
CAMERA_INDEX = 0

# CHANGE THIS according to your ESP32 COM port
COM_PORT = "COM8"

BAUD_RATE = 115200

# Exact class order used during model training
CLASS_NAMES = [
    "metal",
    "organic",
    "paper_cardboard",
    "plastic"
]

DEVICE = torch.device("cpu")

SAVE_DIR = "captured_images"
os.makedirs(SAVE_DIR, exist_ok=True)


# =====================================================
# 2. LOAD CONVNEXT MODEL
# =====================================================

print("\nLoading ConvNeXt model...")

print("Device:", DEVICE)

model = models.convnext_tiny(weights=None)

num_features = model.classifier[2].in_features

model.classifier[2] = nn.Linear(
    num_features,
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

# Remove DataParallel prefix if present
state_dict = {
    key.replace("module.", ""): value
    for key, value in state_dict.items()
}

model.load_state_dict(state_dict)

model.to(DEVICE)

model.eval()

print("Model loaded successfully!")


# =====================================================
# 3. IMAGE PREPROCESSING
# =====================================================

transform = transforms.Compose([

    transforms.ToPILImage(),

    transforms.Resize((224, 224)),

    transforms.ToTensor(),

    transforms.Normalize(
        mean=[0.485, 0.456, 0.406],
        std=[0.229, 0.224, 0.225]
    )

])


# =====================================================
# 4. CONNECT TO ESP32
# =====================================================

print("\nAvailable serial ports:")

for port in serial.tools.list_ports.comports():

    print(
        port.device,
        "-",
        port.description
    )


print("\nConnecting to ESP32...")

try:

    esp = serial.Serial(
        COM_PORT,
        BAUD_RATE,
        timeout=0.5
    )

    # ESP32 may reset when serial connection opens
    time.sleep(2)

    print("\nWaiting for ESP32 READY message...")

    ready = False

    ready_timeout = time.time() + 20

    while time.time() < ready_timeout:

        line = esp.readline().decode(
            "utf-8",
            errors="ignore"
        ).strip()

        if line:

            print("ESP32:", line)

        if line == "READY":

            ready = True
            break

    if not ready:

        raise RuntimeError(
            "ESP32 did not send READY. "
            "Check ESP32 code and COM port."
        )

    print("\nESP32 connected successfully!")

except serial.SerialException as error:

    raise RuntimeError(
        f"Cannot connect to ESP32 on {COM_PORT}. "
        "Check the COM port and close Arduino Serial Monitor."
    ) from error


# =====================================================
# 5. CONNECT TO IRIUN WEBCAM
# =====================================================

print("\nConnecting to Iriun Webcam...")

cap = cv2.VideoCapture(
    CAMERA_INDEX,
    cv2.CAP_MSMF
)

if not cap.isOpened():

    cap.release()

    cap = cv2.VideoCapture(CAMERA_INDEX)

if not cap.isOpened():

    esp.close()

    raise RuntimeError(
        "Iriun Webcam not found. "
        "Check CAMERA_INDEX."
    )

cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)

cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)

print("Iriun camera connected!")


# =====================================================
# 6. CLASSIFICATION FUNCTION
# =====================================================

def classify_image(frame):

    rgb_image = cv2.cvtColor(
        frame,
        cv2.COLOR_BGR2RGB
    )

    input_tensor = transform(rgb_image)

    input_tensor = input_tensor.unsqueeze(0)

    input_tensor = input_tensor.to(DEVICE)

    start_time = time.time()

    with torch.no_grad():

        outputs = model(input_tensor)

        probabilities = torch.softmax(
            outputs,
            dim=1
        )

    inference_time = time.time() - start_time

    confidence, predicted_index = torch.max(
        probabilities,
        dim=1
    )

    predicted_class = CLASS_NAMES[
        predicted_index.item()
    ]

    confidence = confidence.item() * 100

    return (
        predicted_class,
        confidence,
        inference_time
    )


# =====================================================
# 7. SEND CLASSIFICATION TO ESP32
# =====================================================

def send_to_esp32(waste_class):

    print("\n========================================")

    print("SENDING CLASS TO ESP32:", waste_class)

    print("========================================")

    command = waste_class + "\n"

    esp.write(command.encode("utf-8"))

    esp.flush()

    print("Command sent successfully!")

    print("\nWaiting for ESP32 operation...")

    start_time = time.time()

    # Allow time for the longest stepper movement
    timeout = 240

    expected_response = "DONE:" + waste_class

    completed = False

    while time.time() - start_time < timeout:

        line = esp.readline().decode(
            "utf-8",
            errors="ignore"
        ).strip()

        if not line:

            continue

        print("ESP32:", line)

        if line.startswith("ERROR:"):

            print("ESP32 reported an error!")

            break

        if line == expected_response:

            completed = True

            break

    if completed:

        print("\n========================================")

        print("SORTING CYCLE COMPLETED SUCCESSFULLY!")

        print("========================================")

    else:

        print("\nWARNING: Sorting completion not confirmed.")

        print("Check the ESP32 and motor before continuing.")

    return completed


# =====================================================
# 8. MAIN CAMERA LOOP
# =====================================================

print("\n========================================")

print("SMART WASTE SEGREGATION SYSTEM")

print("========================================")

print("Press S to capture, classify and sort.")

print("Press Q to quit.")

print("========================================")


try:

    while True:

        ret, frame = cap.read()

        if not ret:

            print("Camera frame unavailable!")

            continue

        display_frame = frame.copy()

        cv2.putText(
            display_frame,
            "S: CLASSIFY AND SORT | Q: QUIT",
            (10, 30),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.65,
            (0, 255, 0),
            2
        )

        cv2.imshow(
            "Smart Waste Segregation - Iriun",
            display_frame
        )

        key = cv2.waitKey(1) & 0xFF

        # -------------------------------------------------
        # CAPTURE AND CLASSIFY
        # -------------------------------------------------

        if key == ord('s') or key == ord('S'):

            print("\n========================================")

            print("CAPTURING WASTE IMAGE...")

            print("========================================")

            # Capture a few fresh frames
            for _ in range(5):

                ret, latest_frame = cap.read()

                if ret:

                    frame = latest_frame

            if not ret:

                print("Image capture failed!")

                continue

            # Classify captured image
            predicted_class, confidence, inference_time = (
                classify_image(frame)
            )

            # Save image with unique timestamp
            timestamp = datetime.now().strftime(
                "%Y%m%d_%H%M%S_%f"
            )

            image_path = os.path.join(
                SAVE_DIR,
                f"garbage_{timestamp}.jpg"
            )

            cv2.imwrite(
                image_path,
                frame
            )

            # Display classification details
            print("\n----------------------------------------")

            print(
                "PREDICTED WASTE:",
                predicted_class
            )

            print(
                f"CONFIDENCE: {confidence:.2f}%"
            )

            print(
                f"INFERENCE TIME: {inference_time:.3f} seconds"
            )

            print(
                "IMAGE SAVED:",
                os.path.abspath(image_path)
            )

            print("----------------------------------------")

            # Display result on screen
            result_frame = frame.copy()

            cv2.putText(
                result_frame,
                predicted_class,
                (20, 45),
                cv2.FONT_HERSHEY_SIMPLEX,
                1,
                (0, 255, 0),
                2
            )

            cv2.putText(
                result_frame,
                f"Confidence: {confidence:.2f}%",
                (20, 85),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.7,
                (0, 255, 0),
                2
            )

            cv2.imshow(
                "Classification Result",
                result_frame
            )

            cv2.waitKey(1000)

            cv2.destroyWindow(
                "Classification Result"
            )

            # Send predicted class to ESP32
            completed = send_to_esp32(
                predicted_class
            )

            if not completed:

                print(
                    "\nStopping further classifications "
                    "because completion was not confirmed."
                )

                break

            print("\nReady for the next waste item.")

        # -------------------------------------------------
        # QUIT
        # -------------------------------------------------

        elif key == ord('q') or key == ord('Q'):

            print("\nStopping system...")

            break


# =====================================================
# 9. CLEANUP
# =====================================================

finally:

    cap.release()

    esp.close()

    cv2.destroyAllWindows()

    print("\nCamera disconnected.")

    print("ESP32 serial connection closed.")

    print("SYSTEM STOPPED.")
