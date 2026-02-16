# Advanced Face Swapping Pipeline

This document describes the advanced face swapping pipeline implementation.

## Pipeline Overview

```
Camera Input
   ↓
Frame Preprocessing (CLAHE, denoising)
   ↓
YuNet Face Detection
   ↓
Landmark Extraction (5 key points)
   ↓
Face Alignment (512x512)
   ↓
Face Embedding Extraction (ArcFace) [Optional]
   ↓
Face Swap Model (INSwapper) [Optional]
   ↓
Face Restoration (GFPGAN) [Optional]
   ↓
Mask Generation (Convex hull + Gaussian blur)
   ↓
Seamless Blending (Weighted alpha blending)
   ↓
Temporal Stabilization [Optional]
   ↓
Output Renderer
   ↓
Virtual Camera Output
```

## Components

### 1. Frame Preprocessing
- **CLAHE (Contrast Limited Adaptive Histogram Equalization)**: Improves face detection in varying lighting conditions
- Applied to each color channel independently

### 2. Face Detection (YuNet)
- High-accuracy face detection model
- Outputs bounding boxes and 5 facial landmarks:
  - Left eye
  - Right eye
  - Nose tip
  - Left mouth corner
  - Right mouth corner

### 3. Face Alignment
- Standard face alignment to 512x512 resolution
- Uses similarity transformation based on eye positions
- Ensures consistent face orientation for model inference

### 4. Face Embedding (ArcFace)
- Extracts 512-dimensional face embedding
- Used for identity-preserving face swapping
- **Model**: ArcFace ONNX model (typically `w600k_r50.onnx` or similar)

### 5. Face Swap Model (INSwapper)
- Deep learning model for high-quality face swapping
- Takes aligned face and source embedding as input
- **Model**: `inswapper_128.onnx` (128x128) or `inswapper_128_fp16.onnx`

### 6. Face Restoration (GFPGAN)
- Enhances swapped face quality
- Reduces artifacts and improves realism
- **Model**: `GFPGANv1.4.pth` (PyTorch format - requires conversion to ONNX)

### 7. Mask Generation
- Creates smooth blending mask using convex hull of facial landmarks
- Includes forehead estimation for better coverage
- Gaussian blur for smooth edges

### 8. Seamless Blending
- Weighted alpha blending based on mask
- Adjustable blend strength (0.0 to 1.0)
- Preserves natural skin tones and lighting

### 9. Temporal Stabilization
- Smooths face swaps across frames
- Reduces flickering and jitter
- Uses exponential moving average of previous frames

## Model Files Required

### Required
- `face_detection_yunet_2023mar.onnx` - Face detection (already included)

### Optional (for advanced features)
- `w600k_r50.onnx` or similar - ArcFace embedding model
- `inswapper_128.onnx` - INSwapper face swap model
- `GFPGANv1.4.onnx` - GFPGAN restoration model (needs conversion from PyTorch)

## Usage

### Basic Mode (Current Implementation)
Uses affine transformation for face swapping (no models required):
```bash
./build/LiveFaceSwapper --face /path/to/source/image.jpg
```

### Advanced Mode (Requires Models)
To use the advanced pipeline with INSwapper and ArcFace:

1. Download models:
```bash
mkdir -p models
# Download ArcFace model
wget -O models/arcface.onnx <arcface_model_url>

# Download INSwapper model  
wget -O models/inswapper_128.onnx <inswapper_model_url>
```

2. Update code to use AdvancedFaceSwapper:
```cpp
#include "AdvancedFaceSwapper.hpp"

AdvancedFaceSwapper faceSwapper;
faceSwapper.loadFaceDetectionModel("assets/face_detection_yunet_2023mar.onnx");
faceSwapper.loadArcFaceModel("models/arcface.onnx");
faceSwapper.loadInSwapperModel("models/inswapper_128.onnx");
faceSwapper.loadSourceFace("/path/to/source.jpg");
```

## Model Download Links

### ArcFace Models
- InsightFace models: https://github.com/deepinsight/insightface
- ONNX format models available from various repositories

### INSwapper Models
- Available from: https://github.com/deepinsight/insightface/tree/master/python-package
- Model: `inswapper_128.onnx` or `inswapper_128_fp16.onnx`

### GFPGAN Models
- Original: https://github.com/TencentARC/GFPGAN
- Requires conversion from PyTorch to ONNX format

## Performance Considerations

### CPU Mode
- Basic mode: ~30-60 FPS (depending on hardware)
- Advanced mode with models: ~5-15 FPS (CPU inference)

### GPU Acceleration
To enable GPU acceleration:
1. Install OpenCV with CUDA support
2. Install ONNX Runtime GPU
3. Update model loading to use GPU backend:
```cpp
net.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
net.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
```

Expected performance with GPU: ~20-30 FPS

## Fallback Behavior

The pipeline gracefully falls back when models are not available:
- **No ArcFace**: Uses affine transformation for face alignment
- **No INSwapper**: Uses geometric transformation for face swapping
- **No GFPGAN**: Skips face restoration step
- **No temporal stabilization**: Processes each frame independently

## Configuration Options

```cpp
faceSwapper.setBlendStrength(0.95f);           // 0.0 to 1.0
faceSwapper.setEnableGFPGAN(true);            // Enable/disable restoration
faceSwapper.setTemporalStabilization(true);   // Enable/disable stabilization
faceSwapper.setStabilizationStrength(0.7f);   // 0.0 to 1.0
```

## Integration Notes

The `AdvancedFaceSwapper` class is designed to be a drop-in replacement for `FaceSwapper`. To switch:

1. Replace `#include "FaceSwapper.hpp"` with `#include "AdvancedFaceSwapper.hpp"`
2. Replace `FaceSwapper` with `AdvancedFaceSwapper` in your code
3. Add model loading calls after face detection model loading
4. The rest of the API remains compatible

## Future Enhancements

- [ ] Full ONNX Runtime integration (better performance)
- [ ] GFPGAN PyTorch model support
- [ ] Multi-face tracking and stabilization
- [ ] Background preservation options
- [ ] Real-time model switching
- [ ] Quality presets (fast/balanced/high quality)
