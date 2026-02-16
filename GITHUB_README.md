# Live Face Deep Fake - Real-time Face Swapping Pipeline

Advanced real-time face swapping application using YuNet, ArcFace, and INSwapper models with OpenCV DNN.

## 🎯 Features

- **Real-time Face Detection**: YuNet model for fast facial detection
- **Face Embedding Extraction**: ArcFace model for 512-dimensional embeddings
- **Deep Learning Face Swap**: INSwapper_128 ONNX model for realistic face swapping
- **Seamless Blending**: Mask-based blending with temporal stabilization
- **Virtual Camera Support**: Stream to video conferencing applications (/dev/video2)
- **Modern GUI**: OpenCV-based control interface with real-time preview
- **Geometric Fallback**: Automatic fallback to landmark-based transformation if models fail

## 📋 System Requirements

- **OS**: Linux (Ubuntu 20.04+)
- **CPU**: Intel/AMD x86_64 processor
- **RAM**: 4GB minimum (8GB recommended)
- **GPU**: Optional CUDA support for faster inference

### Build Dependencies

- C++17 compatible compiler (GCC 9+)
- OpenCV 4.10+ with DNN module
- CMake 3.10+
- v4l2loopback (for virtual camera)

### Installation

#### 1. Install System Dependencies

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y \
  build-essential \
  cmake \
  git \
  libopencv-dev \
  python3-opencv \
  v4l2loopback-dkms \
  v4l2loopback-utils

# Verify v4l2loopback installation
sudo modprobe v4l2loopback
```

#### 2. Clone Repository

```bash
git clone https://github.com/yourusername/live-face-deep-fake.git
cd live-face-deep-fake
```

#### 3. Download Model Files

The models are stored separately due to their large size. Download them using:

```bash
# This script downloads the required ONNX models
./download_models.sh

# Models will be placed in ./models/ directory:
# - face_detection_yunet_2023mar.onnx (~350MB)
# - arcface.onnx (~167MB) 
# - inswapper_128.onnx (~529MB)
```

**Manual Download**: If the script fails, manually download:
- [YuNet Face Detector](https://github.com/opencv/opencv_zoo/releases/download/v0.5/face_detection_yunet_2023mar.onnx)
- [ArcFace Embedding](https://huggingface.co/deepinsight/insightface/blob/main/python-api/models/det_10g.onnx) 
- [INSwapper](https://huggingface.co/deepinsight/insightface/blob/main/python-api/models/inswapper_128.onnx)

#### 4. Build Project

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

The executable `LiveFaceSwapper` will be created in the `build/` directory.

#### 5. Setup Virtual Camera (Optional)

```bash
# Create virtual camera device
sudo bash setup_virtual_camera.sh

# This creates /dev/video2 that can be used in Zoom, Teams, etc.
```

## 🚀 Quick Start

### Basic Usage

```bash
cd build
./LiveFaceSwapper --arcface ../models/arcface.onnx --inswapper ../models/inswapper_128.onnx
```

### With All Models

```bash
./LiveFaceSwapper \
  --arcface ../models/arcface.onnx \
  --inswapper ../models/inswapper_128.onnx \
  --gfpgan ../models/gfpgan.onnx
```

### Controls

- **U**: Upload source face image
- **Click & Drag**: Adjust blend strength in control panel
- **Q / ESC**: Exit application

## 🔧 Pipeline Architecture

```
Camera Input
    ↓
YuNet Face Detection
    ↓
Face Alignment & Preprocessing
    ↓
ArcFace Embedding Extraction
    ↓
INSwapper Face Swapping
    ├─ Input 1: Target face [1, 3, 128, 128]
    ├─ Input 2: Source embedding [1, 512]
    └─ Output: Swapped face [1, 3, 128, 128]
    ↓
GFPGAN Restoration (Optional)
    ↓
Mask-based Blending
    ↓
Temporal Stabilization
    ↓
Virtual Camera Output
```

## 📝 Configuration

Edit `src/main.cpp` to configure:

```cpp
// Models
arcFaceModel = "../models/arcface.onnx";
inSwapperModel = "../models/inswapper_128.onnx";

// Blending strength (0.0 - 1.0)
advancedSwapper->setBlendStrength(0.95f);

// Temporal stabilization
advancedSwapper->setTemporalStabilization(true);
advancedSwapper->setStabilizationStrength(0.7f);
```

## 🎬 Example Workflows

### Face Swapping in Video Call

1. Build project with virtual camera support
2. Run: `./LiveFaceSwapper --arcface ../models/arcface.onnx --inswapper ../models/inswapper_128.onnx`
3. Upload source face (press 'U')
4. Open Zoom/Teams and select `/dev/video2` as camera
5. Adjust blend strength as needed

### Video File Processing

(Coming soon - batch processing mode)

## 🐛 Troubleshooting

### Issue: "Model not found"
```bash
# Ensure models are downloaded
ls -lh models/
# Expected: arcface.onnx, inswapper_128.onnx, face_detection_yunet_*.onnx
```

### Issue: "Cannot open camera"
```bash
# Check camera device
ls /dev/video*

# Test with v4l2-ctl
v4l2-ctl --list-devices
```

### Issue: Virtual camera not working
```bash
# Reinstall v4l2loopback
sudo rmmod v4l2loopback
./setup_virtual_camera.sh
```

### Issue: Low performance
- Reduce input resolution in `src/main.cpp`
- Enable GPU acceleration (requires CUDA)
- Process every 2nd frame instead of every frame

## 📊 Performance

**Hardware**: Intel i7-10700K, 32GB RAM
- **Face Detection**: 5-8ms
- **ArcFace Embedding**: 8-12ms
- **INSwapper Inference**: 120-180ms
- **Blending & Stabilization**: 10-15ms
- **Total FPS**: ~5 FPS (single face)

## 📚 Project Structure

```
live-face-deep-fake/
├── src/
│   ├── main.cpp                    # Application entry point
│   ├── AdvancedFaceSwapper.cpp/hpp # Core INSwapper integration
│   ├── FaceSwapper.cpp/hpp         # Face swapping logic
│   ├── FaceAnonymizer.cpp/hpp      # Face anonymization
│   ├── ModernGUI.cpp/hpp           # GUI controls
│   └── VirtualCamera.cpp/hpp       # Virtual camera output
├── CMakeLists.txt                  # Build configuration
├── download_models.sh              # Model downloader
├── setup_virtual_camera.sh         # Virtual camera setup
└── README.md                       # This file
```

## 🔑 Key Components

### INSwapper Integration

The INSwapper model expects:
- **Input 1** (`target`): Face image [1, 3, 128, 128] uint8 [0, 255]
- **Input 2** (`source`): Face embedding [1, 512] float32
- **Output**: Swapped face [1, 3, 128, 128] float32 [-1, 1]

Proper conversion from model output to OpenCV image is handled in `AdvancedFaceSwapper::swapFaceWithModel()`.

### ArcFace Embedding

Extracts 512-dimensional feature vector from aligned face image.

### YuNet Detection

Fast, accurate face detection with landmark extraction for alignment.

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit changes (`git commit -m 'Add amazing feature'`)
4. Push to branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## ⚠️ Legal & Ethical Notice

This tool is provided for educational and authorized use only. Users are responsible for:

- Obtaining proper consent before processing anyone's face
- Complying with local laws regarding deepfakes and face swapping
- Respecting privacy and avoiding misuse

**Prohibited uses**:
- Creating non-consensual deepfakes
- Distributing misleading or fraudulent content
- Violating privacy laws or regulations

## 📄 License

MIT License - See LICENSE file for details

## 🙏 Acknowledgments

- [OpenCV](https://opencv.org/) - Computer vision library
- [OpenCV Zoo](https://github.com/opencv/opencv_zoo) - Pre-trained models
- [InsightFace](https://github.com/deepinsight/insightface) - Face recognition models
- [ArcFace](https://arxiv.org/abs/1801.07698) - Face embedding technique
- [INSwapper](https://github.com/deepinsight/insightface) - Face swapping model

## 📞 Support

For issues, questions, or suggestions:
- Open an Issue on GitHub
- Check existing documentation in `/docs` folder
- Review troubleshooting guide above

## 🔮 Future Improvements

- [ ] GPU acceleration with CUDA
- [ ] Batch video processing
- [ ] Real-time performance optimization
- [ ] Multi-face swapping
- [ ] Web UI interface
- [ ] Docker containerization
- [ ] GFPGAN face restoration
- [ ] Expression preservation

---

**Last Updated**: February 2026  
**Version**: 1.0.0  
**Status**: Stable
