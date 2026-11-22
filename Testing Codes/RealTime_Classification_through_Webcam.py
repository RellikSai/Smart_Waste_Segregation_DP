import cv2
import torch
import torch.nn as nn
import torchvision.models as models
from torchvision import transforms
from PIL import Image

# -----------------------------------
# 1. Configuration
# -----------------------------------

MODEL_PATH = "ConvNeXt_best_model.pth"

# IMPORTANT: Keep this order identical to training
CLASSES = [
    "metal",
    "organic",
    "paper_cardboard",
    "plastic"
]

device = torch.device("cuda" if torch.cuda.is_available() else "cpu")

print("Using device:", device)


# -----------------------------------
# 2. Load ConvNeXt-Tiny
# -----------------------------------

model = models.convnext_tiny(weights=None)

# Replace the final classifier layer
in_features = model.classifier[2].in_features

model.classifier[2] = nn.Linear(
    in_features,
    len(CLASSES)
)

# Load trained weights
checkpoint = torch.load(
    MODEL_PATH,
    map_location=device,
    weights_only=True
)

# Supports either a raw state_dict or a
# checkpoint dictionary containing model weights
if "model_state_dict" in checkpoint:
    checkpoint = checkpoint["model_state_dict"]
elif "state_dict" in checkpoint:
    checkpoint = checkpoint["state_dict"]

# Handle DataParallel checkpoints
if any(k.startswith("module.") for k in checkpoint.keys()):
    checkpoint = {
        k.replace("module.", "", 1): v
        for k, v in checkpoint.items()
    }

model.load_state_dict(checkpoint)

model.to(device)
model.eval()

print("ConvNeXt model loaded successfully!")


# -----------------------------------
# 3. Image preprocessing
# -----------------------------------

# IMPORTANT:
# These transformations must match the
# validation/test preprocessing used in training.

transform = transforms.Compose([
    transforms.Resize((224, 224)),
    transforms.ToTensor(),
    transforms.Normalize(
        mean=[0.485, 0.456, 0.406],
        std=[0.229, 0.224, 0.225]
    )
])


# -----------------------------------
# 4. Start webcam
# -----------------------------------

cap = cv2.VideoCapture(0)

if not cap.isOpened():
    print("Could not access webcam!")
    exit()

print("Webcam started. Press Q to quit.")


# -----------------------------------
# 5. Real-time prediction
# -----------------------------------

while True:

    ret, frame = cap.read()

    if not ret:
        print("Failed to capture frame")
        break

    # OpenCV uses BGR; convert to RGB
    rgb_frame = cv2.cvtColor(
        frame,
        cv2.COLOR_BGR2RGB
    )

    # Convert frame to PIL image
    image = Image.fromarray(rgb_frame)

    # Preprocess
    input_tensor = transform(image).unsqueeze(0)
    input_tensor = input_tensor.to(device)

    # Predict
    with torch.inference_mode():

        outputs = model(input_tensor)

        probabilities = torch.softmax(
            outputs,
            dim=1
        )

        confidence, predicted = torch.max(
            probabilities,
            dim=1
        )

    predicted_class = CLASSES[predicted.item()]
    confidence_value = confidence.item() * 100

    # Display prediction
    label = f"{predicted_class}: {confidence_value:.2f}%"

    cv2.putText(
        frame,
        label,
        (20, 40),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.9,
        (0, 255, 0),
        2
    )

    cv2.imshow("Garbage Classification - ConvNeXt", frame)

    # Press Q to exit
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break
  
# -----------------------------------
# 6. Release webcam
# -----------------------------------

cap.release()
cv2.destroyAllWindows()

print("Webcam testing completed.")
