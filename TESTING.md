# Testing Guide

## Quick Test

### 1. Test Basic Mode (No Models)
```bash
./build/LiveFaceSwapper --face /path/to/source_face.jpg
```
This should work immediately and use geometric transformation.

### 2. Test Advanced Mode with INSwapper
```bash
./build/LiveFaceSwapper --mode advanced \
    --inswapper models/inswapper_128.onnx \
    --face /path/to/source_face.jpg
```

### 3. Test Advanced Mode (Fallback)
```bash
./build/LiveFaceSwapper --mode advanced --face /path/to/source_face.jpg
```
This will use fallback methods if models aren't found.

## Expected Behavior

### Basic Mode
- ✅ Fast (30-60 FPS)
- ✅ Works immediately
- ✅ Good quality face swap
- Uses geometric transformation

### Advanced Mode with INSwapper
- ✅ Higher quality face swap
- ✅ Better identity preservation
- ⚠️ Slower (5-15 FPS on CPU)
- Uses deep learning model

### Advanced Mode (Fallback)
- ✅ Works even without models
- ✅ Uses geometric transformation
- ✅ Same as basic mode

## Troubleshooting

### Model Not Loading
```bash
# Check if model exists
ls -lh models/inswapper_128.onnx

# Try with absolute path
./build/LiveFaceSwapper --mode advanced \
    --inswapper $(pwd)/models/inswapper_128.onnx \
    --face image.jpg
```

### Low FPS
- Use basic mode for better performance
- Disable temporal stabilization: `--disable-stabilization`
- Reduce camera resolution in code

### Face Not Detected
- Ensure good lighting
- Face should be clearly visible
- Try different source images

## Performance Tips

1. **For Video Calls**: Use basic mode for smooth 30+ FPS
2. **For Recording**: Use advanced mode for best quality
3. **For Testing**: Start with basic mode, then try advanced

## Next Steps

1. ✅ INSwapper model downloaded
2. ⏭️ (Optional) Download ArcFace model for even better results
3. 🎬 Test with your camera
4. 📹 Use in video calls!
