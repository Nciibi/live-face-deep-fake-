#ifndef MODERN_GUI_HPP
#define MODERN_GUI_HPP

#include <opencv2/opencv.hpp>
#include <string>
#include <functional>
#include <cstdio>

class ModernGUI {
public:
    ModernGUI();
    ~ModernGUI();
    
    // Initialize GUI with window dimensions
    bool initialize(int width = 1280, int height = 720);
    
    // Process events and render GUI
    bool processFrame(const cv::Mat& frame);
    
    // Getters for GUI state
    float getBlendStrength() const { return blendStrength; }
    bool shouldExit() const { return exitRequested; }
    bool isVirtualCameraEnabled() const { return virtualCameraEnabled; }
    bool isSourceFaceLoaded() const { return sourceFaceLoaded; }
    
    // Setters
    void setBlendStrength(float strength) { 
        blendStrength = std::max(0.0f, std::min(1.0f, strength));
        if (blendStrengthCallback) {
            blendStrengthCallback(blendStrength);
        }
    }
    void setFaceCount(int count) { faceCount = count; }
    void setVirtualCameraStatus(bool enabled, const std::string& device = "") {
        virtualCameraEnabled = enabled;
        virtualCameraDevice = device;
    }
    void setFPS(float fps) { currentFPS = fps; }
    void setSourceFaceLoaded(bool loaded) { sourceFaceLoaded = loaded; }
    
    // Callbacks
    void setBlendStrengthCallback(std::function<void(float)> callback) {
        blendStrengthCallback = callback;
    }
    void setImageUploadCallback(std::function<void(const std::string&)> callback) {
        imageUploadCallback = callback;
    }
    
    // Mouse callback handler
    void handleMouse(int event, int x, int y, int flags);

private:
    void renderGUI(const cv::Mat& frame);
    void renderControlPanel(cv::Mat& panel);
    void renderStatsPanel(cv::Mat& canvas, int previewWidth);
    cv::Mat frameTexture;
    
    // GUI state
    float blendStrength;
    int faceCount;
    bool virtualCameraEnabled;
    std::string virtualCameraDevice;
    float currentFPS;
    bool exitRequested;
    bool sourceFaceLoaded;
    
    // Callbacks
    std::function<void(float)> blendStrengthCallback;
    std::function<void(const std::string&)> imageUploadCallback;
    
    // Window dimensions
    int windowWidth;
    int windowHeight;
    
    // OpenCV window for display (we'll enhance it)
    std::string windowName;
    bool initialized;
    
    // Slider bounds (calculated during rendering)
    int sliderX, sliderY, sliderWidth, sliderHeight;
    int uploadButtonX, uploadButtonY, uploadButtonWidth, uploadButtonHeight;
    int previewWidth;
    bool draggingSlider;
    
    // Helper function to open file dialog
    std::string openFileDialog();
};

#endif // MODERN_GUI_HPP
