# Live Face Swapper

A real-time face swapping application with GUI support for uploading source face images. Perfect for video calls, meetings, and live streaming!

## Features

- 🎭 **Real-time Face Swapping**: Swap faces in real-time from your webcam
- 🖼️ **GUI Image Upload**: Upload a source face image through an intuitive GUI
- 📹 **Virtual Camera Support**: Use the swapped video feed in Zoom, Teams, or any video call application
- ⚙️ **Adjustable Blend Strength**: Control how strongly the face swap is applied
- 🎯 **High Accuracy**: Uses YuNet face detection model for precise face detection and landmark extraction
- 🚀 **Two Modes**: 
  - **Basic Mode**: Fast geometric transformation (no models needed)
  - **Advanced Mode**: High-quality deep learning models (ArcFace + INSwapper)
- 🔄 **Temporal Stabilization**: Smooth face swaps across frames (advanced mode)
- 🎨 **Face Restoration**: Optional GFPGAN support for enhanced quality (advanced mode)

## Requirements

- Linux (tested on Ubuntu/Debian)
- OpenCV 4.x with contrib modules
- CMake 3.10+
- FFmpeg (for virtual camera)
- v4l2loopback (for virtual camera)
- zenity or kdialog (for file dialog, optional)

## Installation

### 1. Install System Dependencies

```bash
sudo apt-get update
sudo apt-get install build-essential cmake libopencv-dev libopencv-contrib-dev ffmpeg v4l2loopback-dkms zenity
```

### 2. Set Up Virtual Camera

```bash
sudo modprobe v4l2loopback devices=1 video_nr=2 card_label="FaceSwapper" exclusive_caps=1
```

Or use the provided script:
```bash
sudo ./setup_virtual_camera.sh
```

### 3. Download Face Detection Model

```bash
mkdir -p assets
cd assets
wget https://github.com/opencv/opencv_zoo/raw/master/models/face_detection_yunet/face_detection_yunet_2023mar.onnx
cd ..
```

### 4. Build the Project

```bash
mkdir -p build
cd build
cmake ..
make
```

## Usage

### Basic Mode (Default - Fast, No Models Required)

```bash
./build/LiveFaceSwapper --face /path/to/face/image.jpg
```

Uses geometric transformation for face swapping. Fast and works out of the box.

### Advanced Mode (High Quality - Requires Models)

First, download the models:
```bash
./download_models.sh
```

Then run with advanced mode:
```bash
./build/LiveFaceSwapper --mode advanced \
    --arcface models/arcface.onnx \
    --inswapper models/inswapper_128.onnx \
    --face /path/to/face/image.jpg
```

### Command Line Options

**Basic Options:**
- `--camera <index>`: Camera index (default: 0)
- `--device <path>`: Virtual camera device path (default: auto-detect)
- `--face <path>`: Path to source face image
- `--no-preview`: Disable preview window
- `--help, -h`: Show help message

**Mode Selection:**
- `--mode <basic|advanced>`: Swapping mode (default: basic)
  - `basic`: Fast geometric transformation (no models needed)
  - `advanced`: High-quality deep learning models (requires models)

**Advanced Mode Options:**
- `--arcface <path>`: Path to ArcFace ONNX model (for embeddings)
- `--inswapper <path>`: Path to INSwapper ONNX model (for face swapping)
- `--gfpgan <path>`: Path to GFPGAN model (for face restoration)
- `--enable-gfpgan`: Enable GFPGAN face restoration
- `--disable-stabilization`: Disable temporal stabilization
- `--detection-model <path>`: Face detection model path (default: assets/face_detection_yunet_2023mar.onnx)

### Examples

**Basic mode:**
```bash
./build/LiveFaceSwapper --face image.jpg
```

**Advanced mode (with models):**
```bash
./build/LiveFaceSwapper --mode advanced \
    --arcface models/arcface.onnx \
    --inswapper models/inswapper_128.onnx \
    --face image.jpg
```

**Advanced mode (fallback if models not found):**
```bash
./build/LiveFaceSwapper --mode advanced --face image.jpg
# Will use fallback methods if models not available
```

### GUI Controls

- **Press 'U'**: Upload a source face image
- **Click and drag slider**: Adjust blend strength (0-100%)
- **Press 'Q' or ESC**: Exit the application

### Using in Video Calls

1. Run the application:
   ```bash
   ./build/LiveFaceSwapper
   ```

2. Upload a source face image by pressing 'U' in the GUI

3. Open your video call application (Zoom, Teams, etc.)

4. Go to Settings → Video

5. Select "FaceSwapper" or "/dev/video2" as your camera

6. Start your video call!

The face swap will be applied in real-time to all faces detected in the camera feed.

## How It Works

1. **Face Detection**: Uses YuNet model to detect faces and extract facial landmarks
2. **Landmark Extraction**: Extracts key facial points (eyes, nose, mouth corners)
3. **Face Alignment**: Aligns the source face to match the target face using affine transformation
4. **Face Swapping**: Warps and blends the source face onto detected faces in the video stream
5. **Virtual Camera**: Outputs the processed video to a virtual camera device for use in other applications

## Troubleshooting

### Virtual Camera Not Found

```bash
sudo modprobe v4l2loopback devices=1 video_nr=2
```

### Camera Not Opening

Try a different camera index:
```bash
./build/LiveFaceSwapper --camera 1
```

### File Dialog Not Working

Install zenity or kdialog:
```bash
sudo apt-get install zenity
```

Or use command-line argument:
```bash
./build/LiveFaceSwapper --face /path/to/image.jpg
```

### Face Not Detected in Source Image

- Ensure the image contains a clear, front-facing face
- Use images with good lighting
- Try different images if detection fails

### Poor Face Swap Quality

- Adjust blend strength using the slider
- Use high-quality source images
- Ensure good lighting in both source image and camera feed

## Technical Details

- **Face Detection Model**: YuNet (ONNX format)
- **Landmark Points**: 5 key points (left eye, right eye, nose tip, mouth corners)
- **Transformation**: Affine transformation for face alignment
- **Blending**: Weighted blending with Gaussian mask for smooth edges
- **Performance**: Optimized for real-time processing (30+ FPS on modern hardware)

## License

This project is open source. Please use responsibly and respect privacy.

## Contributing

Contributions are welcome! Please feel free to submit issues or pull requests.
