#include <opencv2/opencv.hpp>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <memory>
#include "AdvancedFaceSwapper.hpp"
#include "VirtualCamera.hpp"
#include "ModernGUI.hpp"

// Advanced Face Swapper Wrapper - The only pipeline used
// 
// Architecture:
// Camera Input
//    ↓
// Frame Preprocessing
//    ↓
// YuNet Face Detection
//    ↓
// Landmark Extraction
//    ↓
// Face Alignment
//    ↓
// Face Embedding Extraction (ArcFace)
//    ↓
// Face Swap Model (INSwapper)
//    ↓
// Face Restoration (GFPGAN)
//    ↓
// Mask Generation
//    ↓
// Seamless Blending
//    ↓
// Temporal Stabilization
//    ↓
// Output Renderer
//    ↓
// Virtual Camera Output
class FaceSwapperPipeline {
    AdvancedFaceSwapper swapper;
public:
    FaceSwapperPipeline(const std::string& detectionModel, 
                        const std::string& arcFaceModel,
                        const std::string& inSwapperModel,
                        const std::string& gfpganModel) {
        swapper.loadFaceDetectionModel(detectionModel);
        if (!arcFaceModel.empty()) {
            swapper.loadArcFaceModel(arcFaceModel);
        }
        if (!inSwapperModel.empty()) {
            swapper.loadInSwapperModel(inSwapperModel);
        }
        if (!gfpganModel.empty()) {
            swapper.loadGFPGANModel(gfpganModel);
        }
    }
    
    bool loadSourceFace(const std::string& imagePath) {
        return swapper.loadSourceFace(imagePath);
    }
    
    bool isSourceFaceLoaded() const {
        return swapper.isSourceFaceLoaded();
    }
    
    // Execute the full pipeline: preprocessing → detection → landmarks → alignment → 
    // embedding → swap → restoration → mask → blending → stabilization → output
    void processFrame(cv::Mat& frame) {
        swapper.detectAndSwap(frame);
    }
    
    int getFaceCount() const {
        return swapper.getFaceCount();
    }
    
    void setBlendStrength(float strength) {
        swapper.setBlendStrength(strength);
    }
    
    float getBlendStrength() const {
        return swapper.getBlendStrength();
    }
    
    void setEnableGFPGAN(bool enable) {
        swapper.setEnableGFPGAN(enable);
    }
    
    void setTemporalStabilization(bool enable) {
        swapper.setTemporalStabilization(enable);
    }
};

// Global variables for mouse callback
ModernGUI* g_gui = nullptr;

// Mouse callback function
void onMouse(int event, int x, int y, int flags, void* userdata) {
    if (g_gui) {
        g_gui->handleMouse(event, x, y, flags);
    }
}

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options]" << std::endl;
    std::cout << "\nAdvanced Face Swapping Pipeline" << std::endl;
    std::cout << "Architecture: Camera Input → Frame Preprocessing → YuNet Face Detection →" << std::endl;
    std::cout << "Landmark Extraction → Face Alignment → ArcFace Embedding Extraction →" << std::endl;
    std::cout << "INSwapper Face Swap Model → Face Restoration → Mask Generation →" << std::endl;
    std::cout << "Seamless Blending → Temporal Stabilization → Output Renderer → Virtual Camera" << std::endl;
    std::cout << "\nRequired Options:" << std::endl;
    std::cout << "  --face <path>             Path to source face image" << std::endl;
    std::cout << "\nOptional Parameters:" << std::endl;
    std::cout << "  --camera <index>          Camera index (default: 0)" << std::endl;
    std::cout << "  --device <path>           Virtual camera device path (default: auto-detect)" << std::endl;
    std::cout << "  --no-preview              Disable preview window" << std::endl;
    std::cout << "\nDeep Learning Models:" << std::endl;
    std::cout << "  --detection-model <path>  Face detection model (default: assets/face_detection_yunet_2023mar.onnx)" << std::endl;
    std::cout << "  --arcface <path>          ArcFace ONNX model for face embeddings" << std::endl;
    std::cout << "  --inswapper <path>        INSwapper ONNX model for face swapping" << std::endl;
    std::cout << "  --gfpgan <path>           GFPGAN model for face restoration" << std::endl;
    std::cout << "\nPipeline Options:" << std::endl;
    std::cout << "  --enable-gfpgan           Enable GFPGAN face restoration in pipeline" << std::endl;
    std::cout << "  --disable-stabilization   Disable temporal stabilization" << std::endl;
    std::cout << "\nOther:" << std::endl;
    std::cout << "  --help, -h                Show this help message" << std::endl;
    std::cout << "\nExamples:" << std::endl;
    std::cout << "  " << programName << " --face image.jpg" << std::endl;
    std::cout << "  " << programName << " --arcface models/arcface.onnx --inswapper models/inswapper_128.onnx --face image.jpg" << std::endl;
    std::cout << "  " << programName << " --arcface models/arcface.onnx --inswapper models/inswapper_128.onnx --gfpgan models/gfpgan.onnx --face image.jpg --enable-gfpgan" << std::endl;
}

int main(int argc, char** argv) {
    // Default values - Always use advanced pipeline
    std::string detectionModel = "assets/face_detection_yunet_2023mar.onnx";
    std::string arcFaceModel = "";
    std::string inSwapperModel = "";
    std::string gfpganModel = "";
    std::string sourceFacePath = "";
    int cameraIndex = 0;
    std::string virtualCameraDevice = "";
    bool showPreview = true;
    bool enableGFPGAN = false;
    bool useTemporalStabilization = true;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--detection-model" && i + 1 < argc) {
            detectionModel = argv[++i];
        } else if (arg == "--arcface" && i + 1 < argc) {
            arcFaceModel = argv[++i];
        } else if (arg == "--inswapper" && i + 1 < argc) {
            inSwapperModel = argv[++i];
        } else if (arg == "--gfpgan" && i + 1 < argc) {
            gfpganModel = argv[++i];
        } else if (arg == "--camera" && i + 1 < argc) {
            cameraIndex = std::stoi(argv[++i]);
        } else if (arg == "--device" && i + 1 < argc) {
            virtualCameraDevice = argv[++i];
        } else if (arg == "--face" && i + 1 < argc) {
            sourceFacePath = argv[++i];
        } else if (arg == "--no-preview") {
            showPreview = false;
        } else if (arg == "--enable-gfpgan") {
            enableGFPGAN = true;
        } else if (arg == "--disable-stabilization") {
            useTemporalStabilization = false;
        } else if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        }
    }
    
    // Check if detection model exists
    FILE* file = fopen(detectionModel.c_str(), "r");
    if (!file) {
        // Fallback to check relative to build dir if run from build/
        std::string altPath = "../" + detectionModel;
        FILE* altFile = fopen(altPath.c_str(), "r");
        if (altFile) {
            detectionModel = altPath;
            fclose(altFile);
        } else {
            std::cerr << "Error: Face detection model not found: " << detectionModel << std::endl;
            std::cerr << "Please ensure the model file exists or use --detection-model to specify the path." << std::endl;
            return -1;
        }
    } else {
        fclose(file);
    }
    
    // Create and initialize the advanced face swapping pipeline
    std::cout << "=== Advanced Face Swapping Pipeline ===" << std::endl;
    std::cout << "Pipeline: Camera → Preprocessing → Detection → Landmarks → Alignment" << std::endl;
    std::cout << "           → Embedding → Swap → Restoration → Mask → Blending" << std::endl;
    std::cout << "           → Stabilization → Output → Virtual Camera" << std::endl;
    
    auto faceSwapper = std::make_unique<FaceSwapperPipeline>(
        detectionModel, arcFaceModel, inSwapperModel, gfpganModel);
    faceSwapper->setEnableGFPGAN(enableGFPGAN);
    faceSwapper->setTemporalStabilization(useTemporalStabilization);
    
    // Adjust model paths if relative and validate
    if (!arcFaceModel.empty()) {
        FILE* f = fopen(arcFaceModel.c_str(), "r");
        if (!f) {
            std::string altPath = "../" + arcFaceModel;
            FILE* altF = fopen(altPath.c_str(), "r");
            if (altF) {
                fclose(altF);
            } else {
                std::cerr << "Warning: ArcFace model not found: " << arcFaceModel << std::endl;
                std::cerr << "Will use fallback embedding extraction." << std::endl;
                arcFaceModel = "";
            }
        } else {
            fclose(f);
        }
        if (!arcFaceModel.empty()) {
            std::cout << "✓ ArcFace model loaded" << std::endl;
        }
    }
    
    if (!inSwapperModel.empty()) {
        FILE* f = fopen(inSwapperModel.c_str(), "r");
        if (!f) {
            std::string altPath = "../" + inSwapperModel;
            FILE* altF = fopen(altPath.c_str(), "r");
            if (altF) {
                fclose(altF);
            } else {
                std::cerr << "Warning: INSwapper model not found: " << inSwapperModel << std::endl;
                std::cerr << "Will use fallback face swapping." << std::endl;
                inSwapperModel = "";
            }
        } else {
            fclose(f);
        }
        if (!inSwapperModel.empty()) {
            std::cout << "✓ INSwapper model loaded" << std::endl;
        }
    }
    
    if (arcFaceModel.empty() || inSwapperModel.empty()) {
        std::cout << "Note: Some models not found. Using geometric fallback methods." << std::endl;
        std::cout << "For optimal results with INSwapper, download models using: ./download_models.sh" << std::endl;
    }
    
    // Load source face if provided via command line
    if (!sourceFacePath.empty()) {
        FILE* f = fopen(sourceFacePath.c_str(), "r");
        if (!f) {
            std::string altPath = "../" + sourceFacePath;
            FILE* altF = fopen(altPath.c_str(), "r");
            if (altF) {
                sourceFacePath = altPath;
                fclose(altF);
            }
        } else {
            fclose(f);
        }
        
        if (!faceSwapper->loadSourceFace(sourceFacePath)) {
            std::cerr << "Warning: Could not load source face from: " << sourceFacePath << std::endl;
            std::cerr << "You can upload a face image using the GUI (press 'U' key)." << std::endl;
        } else {
            std::cout << "✓ Source face loaded: " << sourceFacePath << std::endl;
        }
    }

    cv::VideoCapture cap(cameraIndex);
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open camera " << cameraIndex << "." << std::endl;
        return -1;
    }

    // Set a reasonable resolution
    int width = 640;
    int height = 480;
    cap.set(cv::CAP_PROP_FRAME_WIDTH, width);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, height);
    
    // Get actual resolution
    width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));

    // Initialize virtual camera
    VirtualCamera virtualCam;
    if (!virtualCam.initialize(virtualCameraDevice, width, height)) {
        std::cerr << "Warning: Virtual camera initialization failed." << std::endl;
        std::cerr << "The swapped video will only be shown in the preview window." << std::endl;
        std::cerr << "To use with Zoom/video calls, set up v4l2loopback first." << std::endl;
    } else {
        std::cout << "✓ Virtual camera ready! Select '" << virtualCam.getDevicePath() 
                  << "' as your camera in Zoom or other video call applications." << std::endl;
    }

    // Initialize modern GUI
    ModernGUI gui;
    if (showPreview) {
        if (!gui.initialize(1280, 720)) {
            std::cerr << "Failed to initialize GUI." << std::endl;
            return -1;
        }
        
        // Set up blend strength callback
        gui.setBlendStrengthCallback([&faceSwapper](float strength) {
            faceSwapper->setBlendStrength(strength);
        });
        
        // Set initial blend strength
        gui.setBlendStrength(0.95f);
        faceSwapper->setBlendStrength(0.95f);
        
        // Set up image upload callback
        gui.setImageUploadCallback([&faceSwapper, &gui](const std::string& imagePath) {
            if (faceSwapper->loadSourceFace(imagePath)) {
                gui.setSourceFaceLoaded(true);
                std::cout << "✓ Source face loaded from: " << imagePath << std::endl;
            } else {
                gui.setSourceFaceLoaded(false);
                std::cerr << "Failed to load source face from: " << imagePath << std::endl;
            }
        });
        
        // Set initial source face status
        gui.setSourceFaceLoaded(faceSwapper->isSourceFaceLoaded());
        
        // Set virtual camera status
        gui.setVirtualCameraStatus(virtualCam.isReady(), virtualCam.getDevicePath());
        
        // Set up mouse callback
        g_gui = &gui;
        cv::setMouseCallback("Face Swapper - Modern GUI", onMouse, nullptr);
    }

    cv::Mat frame;
    std::cout << "\n=== Face Swapper - Advanced Pipeline ===" << std::endl;
    std::cout << "Mode: Advanced Deep Learning (YuNet → ArcFace → INSwapper → GFPGAN)" << std::endl;
    std::cout << "Press 'q' or 'ESC' to exit." << std::endl;
    std::cout << "Press 'U' to upload a source face image." << std::endl;
    std::cout << "Camera resolution: " << width << "x" << height << std::endl;
    if (showPreview) {
        if (!faceSwapper->isSourceFaceLoaded()) {
            std::cout << "No source face loaded. Press 'U' to upload a face image." << std::endl;
        }
        std::cout << "Click and drag the blend slider in the control panel to adjust strength." << std::endl;
    }

    // FPS calculation
    auto lastTime = std::chrono::steady_clock::now();
    int frameCount = 0;
    float fps = 0.0f;

    while (true) {
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastTime).count();
        
        cap >> frame;
        if (frame.empty()) {
            std::cerr << "Error: Captured empty frame." << std::endl;
            break;
        }

        // Process frame through the advanced pipeline
        // Pipeline: Preprocessing → Detection → Landmarks → Alignment → 
        // Embedding → Swap → Restoration → Mask → Blending → Stabilization → Output
        if (faceSwapper->isSourceFaceLoaded()) {
            faceSwapper->processFrame(frame);
        }

        // Write to virtual camera
        if (virtualCam.isReady()) {
            virtualCam.writeFrame(frame);
        }

        // Update GUI
        if (showPreview) {
            // Update GUI state
            gui.setFaceCount(faceSwapper->getFaceCount());
            gui.setSourceFaceLoaded(faceSwapper->isSourceFaceLoaded());
            gui.setVirtualCameraStatus(virtualCam.isReady(), virtualCam.getDevicePath());
            
            // Calculate FPS
            frameCount++;
            if (elapsed >= 1000) { // Update FPS every second
                fps = frameCount * 1000.0f / elapsed;
                frameCount = 0;
                lastTime = currentTime;
            }
            gui.setFPS(fps);
            
            // Process frame and check for exit
            if (!gui.processFrame(frame)) {
                break;
            }
        } else {
            // No GUI mode - just check for exit key
            char c = (char)cv::waitKey(10);
            if (c == 27 || c == 'q' || c == 'Q') {
                break;
            }
        }
    }

    cap.release();
    virtualCam.release();
    std::cout << "\nExiting..." << std::endl;
    return 0;
}
