#!/bin/bash

# Script to download face swapping models
# Usage: ./download_models.sh

set -e

echo "=== Face Swapper Model Downloader ==="
echo ""

# Create models directory
mkdir -p models
cd models

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to download file with progress
download_file() {
    local url=$1
    local filename=$2
    local description=$3
    
    if [ -f "$filename" ] && [ -s "$filename" ]; then
        echo -e "${YELLOW}$filename already exists. Skipping...${NC}"
        return 0
    fi
    
    echo -e "${GREEN}Downloading $description...${NC}"
    echo "URL: $url"
    
    if command -v wget &> /dev/null; then
        if wget --progress=bar:force:noscroll -O "$filename" "$url" 2>&1; then
            if [ -f "$filename" ] && [ -s "$filename" ]; then
                echo -e "${GREEN}✓ $filename downloaded successfully${NC}"
                echo ""
                return 0
            else
                echo -e "${RED}Downloaded file is empty or missing${NC}"
                rm -f "$filename"
                return 1
            fi
        else
            echo -e "${RED}Failed to download from this source${NC}"
            rm -f "$filename"
            return 1
        fi
    elif command -v curl &> /dev/null; then
        if curl -L --progress-bar -o "$filename" "$url" 2>&1; then
            if [ -f "$filename" ] && [ -s "$filename" ]; then
                echo -e "${GREEN}✓ $filename downloaded successfully${NC}"
                echo ""
                return 0
            else
                echo -e "${RED}Downloaded file is empty or missing${NC}"
                rm -f "$filename"
                return 1
            fi
        else
            echo -e "${RED}Failed to download from this source${NC}"
            rm -f "$filename"
            return 1
        fi
    else
        echo -e "${RED}Neither wget nor curl found. Please install one of them.${NC}"
        return 1
    fi
}

echo "This script will download the following models:"
echo "1. INSwapper model (for face swapping) - ~500MB"
echo "2. ArcFace model (for face embeddings) - Manual download required"
echo ""
read -p "Do you want to continue? (y/n) " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Download cancelled."
    exit 0
fi

echo ""
echo "=== Downloading Models ==="
echo ""

# INSwapper Model - Multiple sources with fallbacks
INSWAPPER_FILE="inswapper_128.onnx"

echo -e "${BLUE}Attempting to download INSwapper model...${NC}"
echo -e "${YELLOW}Note: INSwapper models may require licensing. See: https://github.com/deepinsight/insightface${NC}"
echo ""

# Try multiple sources in order
INSWAPPER_URLS=(
    "https://huggingface.co/fofr/comfyui/resolve/4dccd71e17017ccad11b92171f5f24aa408a1407/insightface/inswapper_128.onnx"
    "https://huggingface.co/Aitrepreneur/insightface/resolve/fd887cdef0c73f32251198b8160d6771ac413fc0/inswapper_128.onnx"
    "https://huggingface.co/deepinsight/inswapper/resolve/main/inswapper_128.onnx"
)

DOWNLOADED=false
for url in "${INSWAPPER_URLS[@]}"; do
    echo "Trying: $url"
    if download_file "$url" "$INSWAPPER_FILE" "INSwapper Face Swap Model"; then
        DOWNLOADED=true
        break
    else
        echo "This source failed, trying next..."
        echo ""
    fi
done

if [ "$DOWNLOADED" = false ]; then
    echo -e "${RED}Failed to download INSwapper model from all sources.${NC}"
    echo ""
    echo "Manual download options:"
    echo "1. Hugging Face: https://huggingface.co/deepinsight/inswapper"
    echo "2. Search for 'inswapper_128.onnx' on Hugging Face"
    echo "3. Alternative: https://huggingface.co/fofr/comfyui"
    echo ""
    echo "Note: Some models may require licensing. Check InsightFace repository for details."
    echo ""
    echo "You can still use the application in basic mode or advanced mode with fallback methods."
    echo ""
fi

# Check file sizes
if [ -f "$INSWAPPER_FILE" ]; then
    SIZE=$(du -h "$INSWAPPER_FILE" | cut -f1)
    echo -e "${GREEN}✓ INSwapper model size: $SIZE${NC}"
fi

echo ""
echo "=== ArcFace Model ==="
echo ""
echo -e "${YELLOW}ArcFace model needs to be downloaded manually.${NC}"
echo ""
echo "Option 1: Hugging Face (Recommended):"
echo "  https://huggingface.co/deepinsight/insightface"
echo "  Look for 'buffalo_l' or 'w600k_r50' models"
echo ""
echo "Option 2: Download pre-converted ONNX models:"
echo "  https://github.com/harisreedhar/Face-Swappers-ONNX"
echo "  or search Hugging Face for 'ArcFace ONNX'"
echo ""
echo "Option 3: Use InsightFace Python package to export:"
echo "  pip install insightface"
echo "  python -c 'import insightface; app = insightface.app.FaceAnalysis(); app.prepare(ctx_id=0)'"
echo "  Models will be downloaded to ~/.insightface/"
echo ""
echo "Common ArcFace model files:"
echo "  - w600k_r50.onnx"
echo "  - w600k_r50_fp16.onnx"
echo "  - buffalo_l.zip (contains multiple models, extract w600k_r50.onnx)"
echo ""
echo "After downloading, place the ArcFace ONNX model in the models/ directory"
echo "and name it 'arcface.onnx' or use --arcface flag to specify the path."
echo ""
echo -e "${YELLOW}Note: Some InsightFace models may require licensing.${NC}"
echo "Check: https://github.com/deepinsight/insightface for details."
echo ""

cd ..

echo ""
echo "=== Download Summary ==="
echo ""
echo "Downloaded models:"
if [ -d "models" ]; then
    ls -lh models/*.onnx 2>/dev/null || echo "No ONNX models found in models/ directory"
else
    echo "models/ directory not found"
fi

echo ""
echo -e "${GREEN}=== Setup Complete ===${NC}"
echo ""
echo "Usage examples:"
echo ""
echo "Basic mode (no models required):"
echo "  ./build/LiveFaceSwapper --face /path/to/image.jpg"
echo ""
echo "Advanced mode with downloaded models:"
echo "  ./build/LiveFaceSwapper --mode advanced --inswapper models/inswapper_128.onnx --face /path/to/image.jpg"
echo ""
echo "Advanced mode with all models:"
echo "  ./build/LiveFaceSwapper --mode advanced --arcface models/arcface.onnx --inswapper models/inswapper_128.onnx --face /path/to/image.jpg"
echo ""
echo "Note: If models are not found, the application will use fallback methods."
echo ""
