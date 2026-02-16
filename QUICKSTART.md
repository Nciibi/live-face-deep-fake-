# Quick Start Guide

## Step 1: Install Dependencies

```bash
sudo apt-get install build-essential cmake libopencv-dev libopencv-contrib-dev ffmpeg v4l2loopback-dkms zenity
```

## Step 2: Download Face Detection Model

```bash
mkdir -p assets
cd assets
wget https://github.com/opencv/opencv_zoo/raw/master/models/face_detection_yunet/face_detection_yunet_2023mar.onnx
cd ..
```

## Step 3: Build the Project

```bash
mkdir -p build
cd build
cmake ..
make
```

## Step 4: Set Up Virtual Camera

```bash
sudo ./setup_virtual_camera.sh
```

Or manually:
```bash
sudo modprobe v4l2loopback devices=1 video_nr=2 card_label="FaceSwapper" exclusive_caps=1
```

## Step 5: Run the Face Swapper

### Basic Mode (Fast, No Models Required)
```bash
./build/LiveFaceSwapper --face /path/to/face/image.jpg
```

### Advanced Mode (High Quality, Requires Models)

**Optional: Download models for better quality:**
```bash
./download_models.sh
```

**Then run with advanced mode:**
```bash
./build/LiveFaceSwapper --mode advanced \
    --inswapper models/inswapper_128.onnx \
    --face /path/to/face/image.jpg
```

**Or run without models (uses fallback):**
```bash
./build/LiveFaceSwapper --mode advanced --face /path/to/face/image.jpg
```

## Step 6: Upload Source Face (if not using command line)

1. Press **'U'** key in the GUI to open file dialog
2. Select an image file containing the face you want to swap
3. The face will be detected and loaded automatically

## Step 7: Use in Video Calls

1. Open Zoom, Teams, or any video call application
2. Go to Settings → Video
3. Select "FaceSwapper" or "/dev/video2" as your camera
4. Start your video call!

All faces detected in the camera feed will be swapped with your source face! 🎭

## Controls

- **Press 'U'**: Upload source face image
- **Click and drag slider**: Adjust blend strength
- **Press 'Q' or ESC**: Exit

## Troubleshooting

- **Virtual camera not found?** Run: `sudo modprobe v4l2loopback devices=1 video_nr=2`
- **Camera not opening?** Try: `./build/LiveFaceSwapper --camera 1`
- **File dialog not working?** Install zenity: `sudo apt-get install zenity`
- **Need help?** Check the full README.md for detailed instructions
