# Model Download Guide

## INSwapper Model

The INSwapper model (`inswapper_128.onnx`) is required for high-quality face swapping in advanced mode.

### Automatic Download

Run the download script:
```bash
./download_models.sh
```

The script will try multiple sources automatically.

### Manual Download Options

If automatic download fails, try these sources:

#### Option 1: Hugging Face (Recommended)
1. Visit: https://huggingface.co/deepinsight/inswapper
2. Look for `inswapper_128.onnx` file
3. Click "Download" button
4. Save to `models/inswapper_128.onnx`

#### Option 2: Alternative Hugging Face Repositories
- https://huggingface.co/fofr/comfyui (search for inswapper_128.onnx)
- https://huggingface.co/Aitrepreneur/insightface (search for inswapper_128.onnx)

#### Option 3: Direct Download URLs
Try these direct download links (may require authentication):
```bash
# Option A
wget https://huggingface.co/fofr/comfyui/resolve/4dccd71e17017ccad11b92171f5f24aa408a1407/insightface/inswapper_128.onnx -O models/inswapper_128.onnx

# Option B
wget https://huggingface.co/Aitrepreneur/insightface/resolve/fd887cdef0c73f32251198b8160d6771ac413fc0/inswapper_128.onnx -O models/inswapper_128.onnx
```

### Model Size
- Expected size: ~500-600 MB
- Format: ONNX (`.onnx`)

### Licensing Note
⚠️ **Important**: As of 2024, InsightFace INSwapper models may require licensing for commercial use. 
- Check: https://github.com/deepinsight/insightface
- Contact: contact@insightface.ai for licensing information
- For personal/research use, models from Hugging Face are typically acceptable

## ArcFace Model

The ArcFace model is used for extracting face embeddings (identity features).

### Download Options

#### Option 1: Hugging Face
1. Visit: https://huggingface.co/deepinsight/insightface
2. Look for models:
   - `buffalo_l.zip` (contains multiple models)
   - `w600k_r50.onnx` (standalone)
   - `w600k_r50_fp16.onnx` (FP16 version, smaller)
3. Download and extract if needed
4. Save as `models/arcface.onnx` or use `--arcface` flag

#### Option 2: InsightFace Python Package
```bash
pip install insightface
python -c "import insightface; app = insightface.app.FaceAnalysis(); app.prepare(ctx_id=0)"
```
Models will be downloaded to `~/.insightface/models/`
Look for `w600k_r50.onnx` and copy to `models/arcface.onnx`

#### Option 3: Pre-converted ONNX Models
- https://github.com/harisreedhar/Face-Swappers-ONNX
- Search Hugging Face for "ArcFace ONNX"

### Model Files
- `w600k_r50.onnx` - Full precision (recommended)
- `w600k_r50_fp16.onnx` - Half precision (smaller, slightly faster)
- `buffalo_l.zip` - Contains multiple models (extract `w600k_r50.onnx`)

## Verification

After downloading, verify models:
```bash
# Check if models exist
ls -lh models/*.onnx

# Expected output:
# models/inswapper_128.onnx (~500-600 MB)
# models/arcface.onnx (~250-300 MB)
```

## Usage After Download

### With Both Models
```bash
./build/LiveFaceSwapper --mode advanced \
    --arcface models/arcface.onnx \
    --inswapper models/inswapper_128.onnx \
    --face source_image.jpg
```

### With INSwapper Only (ArcFace fallback)
```bash
./build/LiveFaceSwapper --mode advanced \
    --inswapper models/inswapper_128.onnx \
    --face source_image.jpg
```

### Without Models (Fallback Mode)
```bash
./build/LiveFaceSwapper --mode advanced \
    --face source_image.jpg
# Uses geometric transformation fallback
```

## Troubleshooting

### Download Fails
- Check internet connection
- Try manual download from Hugging Face
- Verify URL is still valid
- Check file permissions in `models/` directory

### Model Not Found Error
- Verify file exists: `ls -lh models/inswapper_128.onnx`
- Check file size (should be ~500-600 MB)
- Try absolute path: `--inswapper /full/path/to/inswapper_128.onnx`

### Model Loading Error
- Ensure OpenCV is compiled with DNN support
- Check model format (must be ONNX)
- Verify model is not corrupted (check file size)

### Performance Issues
- Use basic mode for better FPS: `--mode basic`
- Disable temporal stabilization: `--disable-stabilization`
- Consider GPU acceleration (requires OpenCV with CUDA)

## Alternative: Use Without Models

The application works perfectly fine without these models using fallback methods:
- Basic mode: Fast geometric transformation
- Advanced mode: Falls back to geometric methods if models not found

Models are optional for enhanced quality, not required for functionality.
