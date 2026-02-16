# Quick Reference Guide

## Quick Start Commands

### Basic Mode (No Models Required)
```bash
# Run with source face image
./build/LiveFaceSwapper --face /path/to/image.jpg

# Or upload face via GUI (press 'U')
./build/LiveFaceSwapper
```

### Advanced Mode (High Quality)

**Step 1: Download Models**
```bash
./download_models.sh
```

**Step 2: Run with Models**
```bash
./build/LiveFaceSwapper --mode advanced \
    --arcface models/arcface.onnx \
    --inswapper models/inswapper_128.onnx \
    --face /path/to/image.jpg
```

## Model Download

### Automatic Download
```bash
./download_models.sh
```
Downloads INSwapper model automatically. ArcFace model needs manual download.

### Manual Model Download

**INSwapper Model:**
- URL: https://github.com/deepinsight/insightface/releases/download/v0.7/inswapper_128.onnx
- Save to: `models/inswapper_128.onnx`

**ArcFace Model:**
- Download from: https://github.com/deepinsight/insightface
- Look for: `buffalo_l.zip` or `w600k_r50.onnx`
- Extract and save as: `models/arcface.onnx`

## Common Use Cases

### Video Call Setup
```bash
# 1. Set up virtual camera
sudo ./setup_virtual_camera.sh

# 2. Run face swapper
./build/LiveFaceSwapper --mode advanced \
    --inswapper models/inswapper_128.onnx \
    --face source_face.jpg

# 3. In Zoom/Teams: Select "/dev/video2" as camera
```

### Testing Without Camera
```bash
# Use a test image as camera input (requires v4l2loopback setup)
./build/LiveFaceSwapper --camera 0 --face swap_face.jpg
```

### High Quality Mode
```bash
./build/LiveFaceSwapper --mode advanced \
    --arcface models/arcface.onnx \
    --inswapper models/inswapper_128.onnx \
    --enable-gfpgan \
    --face source.jpg
```

## Troubleshooting

### Models Not Found
- Application will use fallback methods automatically
- Check model paths with `ls -lh models/*.onnx`
- Use `--arcface` and `--inswapper` flags to specify custom paths

### Virtual Camera Not Working
```bash
# Check if v4l2loopback is loaded
lsmod | grep v4l2loopback

# Load if not present
sudo modprobe v4l2loopback devices=1 video_nr=2
```

### Low FPS in Advanced Mode
- Use basic mode for better performance: `--mode basic`
- Disable temporal stabilization: `--disable-stabilization`
- Consider GPU acceleration (requires OpenCV with CUDA)

## GUI Controls

- **Press 'U'**: Upload source face image
- **Click slider**: Adjust blend strength
- **Press 'Q' or ESC**: Exit application

## Performance Comparison

| Mode | FPS | Quality | Models Required |
|------|-----|---------|----------------|
| Basic | 30-60 | Good | None |
| Advanced (CPU) | 5-15 | Excellent | ArcFace + INSwapper |
| Advanced (GPU) | 20-30 | Excellent | ArcFace + INSwapper |
