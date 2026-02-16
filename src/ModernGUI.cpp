#include "ModernGUI.hpp"
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>

ModernGUI::ModernGUI() 
    : blendStrength(0.95f)
    , faceCount(0)
    , virtualCameraEnabled(false)
    , currentFPS(0.0f)
    , exitRequested(false)
    , sourceFaceLoaded(false)
    , windowWidth(1280)
    , windowHeight(720)
    , initialized(false)
    , sliderX(0)
    , sliderY(0)
    , sliderWidth(0)
    , sliderHeight(0)
    , uploadButtonX(0)
    , uploadButtonY(0)
    , uploadButtonWidth(0)
    , uploadButtonHeight(0)
    , previewWidth(0)
    , draggingSlider(false)
{
    windowName = "Face Swapper - Modern GUI";
}

ModernGUI::~ModernGUI() {
    if (initialized) {
        cv::destroyAllWindows();
    }
}

bool ModernGUI::initialize(int width, int height) {
    windowWidth = width;
    windowHeight = height;
    
    cv::namedWindow(windowName, cv::WINDOW_NORMAL);
    cv::resizeWindow(windowName, windowWidth, windowHeight);
    cv::setWindowProperty(windowName, cv::WND_PROP_TOPMOST, 0);
    
    initialized = true;
    return true;
}

bool ModernGUI::processFrame(const cv::Mat& frame) {
    if (!initialized || frame.empty()) {
        return false;
    }
    
    renderGUI(frame);
    
    // Check for keys - use longer wait time to ensure key press is captured
    int key = cv::waitKey(30) & 0xFF;
    
    if (key != 255 && key != -1) { // Valid key pressed
        if (key == 27 || key == 'q' || key == 'Q') { // ESC or 'q'
            exitRequested = true;
            return false;
        } else if (key == 'u' || key == 'U') { // 'u' for upload
            std::cout << "\n=== Opening file dialog ===" << std::endl;
            std::cout << "Please check for a file dialog window (it may appear behind this window)." << std::endl;
            
            // Temporarily lower window priority so dialog can appear on top
            cv::setWindowProperty(windowName, cv::WND_PROP_TOPMOST, 0);
            
            std::string filePath = openFileDialog();
            
            if (!filePath.empty()) {
                std::cout << "Selected file: " << filePath << std::endl;
                if (imageUploadCallback) {
                    imageUploadCallback(filePath);
                } else {
                    std::cerr << "Warning: Image upload callback not set!" << std::endl;
                }
            } else {
                std::cout << "No file selected or file dialog failed." << std::endl;
                std::cout << "Tip: You can also use --face /path/to/image.jpg on the command line." << std::endl;
            }
        }
    }
    
    return true;
}

void ModernGUI::renderGUI(const cv::Mat& frame) {
    cv::Mat displayFrame = frame.clone();
    
    // Create a larger canvas for the GUI
    int canvasWidth = windowWidth;
    int canvasHeight = windowHeight;
    
    // Resize frame to fit in the left portion (70% width)
    previewWidth = static_cast<int>(canvasWidth * 0.7);
    int previewHeight = canvasHeight;
    
    cv::Mat resizedFrame;
    cv::resize(displayFrame, resizedFrame, cv::Size(previewWidth, previewHeight));
    
    // Create control panel area (30% width on the right)
    int panelWidth = canvasWidth - previewWidth;
    cv::Mat controlPanel = cv::Mat::zeros(canvasHeight, panelWidth, CV_8UC3);
    controlPanel.setTo(cv::Scalar(45, 45, 48)); // Dark gray background
    
    // Render control panel
    renderControlPanel(controlPanel);
    
    // Combine preview and control panel
    cv::Mat combinedCanvas = cv::Mat::zeros(canvasHeight, canvasWidth, CV_8UC3);
    
    // Place preview on the left
    resizedFrame.copyTo(combinedCanvas(cv::Rect(0, 0, previewWidth, previewHeight)));
    
    // Place control panel on the right
    controlPanel.copyTo(combinedCanvas(cv::Rect(previewWidth, 0, panelWidth, canvasHeight)));
    
    // Add separator line
    cv::line(combinedCanvas, cv::Point(previewWidth, 0), cv::Point(previewWidth, canvasHeight),
             cv::Scalar(60, 60, 63), 2);
    
    // Add overlay info on preview
    renderStatsPanel(combinedCanvas, previewWidth);
    
    cv::imshow(windowName, combinedCanvas);
}

void ModernGUI::renderControlPanel(cv::Mat& panel) {
    int x = 20;
    int y = 30;
    int lineHeight = 35;
    int sectionSpacing = 50;
    
    // Title
    cv::putText(panel, "FACE SWAPPER", cv::Point(x, y),
                cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(100, 200, 255), 2);
    y += lineHeight + 10;
    
    // Separator
    cv::line(panel, cv::Point(x, y), cv::Point(panel.cols - x, y), cv::Scalar(80, 80, 85), 1);
    y += sectionSpacing;
    
    // Source Face Upload Section
    cv::putText(panel, "Source Face", cv::Point(x, y),
                cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
    y += lineHeight;
    
    // Upload button
    uploadButtonX = x;
    uploadButtonY = y;
    uploadButtonWidth = panel.cols - 2 * x;
    uploadButtonHeight = 40;
    
    cv::Scalar buttonColor = sourceFaceLoaded ? cv::Scalar(60, 200, 100) : cv::Scalar(60, 120, 200);
    cv::rectangle(panel, cv::Rect(uploadButtonX, uploadButtonY, uploadButtonWidth, uploadButtonHeight),
                  buttonColor, -1);
    cv::rectangle(panel, cv::Rect(uploadButtonX, uploadButtonY, uploadButtonWidth, uploadButtonHeight),
                  cv::Scalar(100, 100, 105), 2);
    
    std::string buttonText = sourceFaceLoaded ? "Source Face Loaded" : "Upload Face Image (U)";
    cv::Size textSize = cv::getTextSize(buttonText, cv::FONT_HERSHEY_SIMPLEX, 0.6, 2, nullptr);
    cv::putText(panel, buttonText, 
                cv::Point(uploadButtonX + (uploadButtonWidth - textSize.width) / 2,
                         uploadButtonY + (uploadButtonHeight + textSize.height) / 2),
                cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 2);
    
    y += uploadButtonHeight + sectionSpacing;
    
    // Blend Strength Section
    cv::putText(panel, "Blend Strength", cv::Point(x, y),
                cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
    y += lineHeight;
    
    // Blend strength value with colored background
    std::ostringstream blendOss;
    blendOss << std::fixed << std::setprecision(0) << (blendStrength * 100) << "%";
    
    // Draw value box
    textSize = cv::getTextSize(blendOss.str(), cv::FONT_HERSHEY_SIMPLEX, 0.9, 2, nullptr);
    cv::Rect valueRect(x, y - textSize.height - 5, textSize.width + 20, textSize.height + 10);
    cv::rectangle(panel, valueRect, cv::Scalar(60, 120, 200), -1);
    cv::putText(panel, blendOss.str(), cv::Point(x + 10, y),
                cv::FONT_HERSHEY_SIMPLEX, 0.9, cv::Scalar(255, 255, 255), 2);
    y += lineHeight + 10;
    
    // Visual slider representation
    sliderX = x;
    sliderY = y;
    sliderWidth = panel.cols - 2 * x;
    sliderHeight = 30;
    
    // Slider track
    cv::rectangle(panel, cv::Rect(x, sliderY, sliderWidth, sliderHeight),
                  cv::Scalar(60, 60, 65), -1);
    cv::rectangle(panel, cv::Rect(x, sliderY, sliderWidth, sliderHeight),
                  cv::Scalar(100, 100, 105), 1);
    
    // Slider fill (showing current value)
    int fillWidth = static_cast<int>(sliderWidth * blendStrength);
    cv::rectangle(panel, cv::Rect(x, sliderY, fillWidth, sliderHeight),
                  cv::Scalar(60, 150, 255), -1);
    
    // Slider handle
    int handlePos = x + fillWidth - 8;
    cv::circle(panel, cv::Point(handlePos, sliderY + sliderHeight / 2), 12,
               cv::Scalar(255, 255, 255), -1);
    cv::circle(panel, cv::Point(handlePos, sliderY + sliderHeight / 2), 12,
               cv::Scalar(200, 200, 200), 2);
    
    y += sliderHeight + sectionSpacing;
    
    // Stats Section
    cv::putText(panel, "Statistics", cv::Point(x, y),
                cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
    y += lineHeight;
    
    // Face count
    std::ostringstream faceOss;
    faceOss << "Faces Detected: " << faceCount;
    cv::Scalar faceColor = (faceCount > 0) ? cv::Scalar(100, 255, 150) : cv::Scalar(150, 150, 150);
    cv::putText(panel, faceOss.str(), cv::Point(x, y),
                cv::FONT_HERSHEY_SIMPLEX, 0.6, faceColor, 2);
    y += lineHeight;
    
    // FPS
    std::ostringstream fpsOss;
    fpsOss << "FPS: " << std::fixed << std::setprecision(1) << currentFPS;
    cv::putText(panel, fpsOss.str(), cv::Point(x, y),
                cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(200, 200, 255), 2);
    y += sectionSpacing;
    
    // Virtual Camera Status
    cv::putText(panel, "Virtual Camera", cv::Point(x, y),
                cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
    y += lineHeight;
    
    if (virtualCameraEnabled) {
        cv::putText(panel, "Status: Active", cv::Point(x, y),
                    cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(100, 255, 100), 2);
        y += lineHeight;
        if (!virtualCameraDevice.empty()) {
            std::string deviceText = "Device: " + virtualCameraDevice;
            cv::putText(panel, deviceText, cv::Point(x, y),
                        cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(200, 200, 200), 1);
        }
    } else {
        cv::putText(panel, "Status: Inactive", cv::Point(x, y),
                    cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(100, 100, 255), 2);
    }
    y += sectionSpacing;
    
    // Instructions
    cv::putText(panel, "Controls", cv::Point(x, y),
                cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
    y += lineHeight;
    
    cv::putText(panel, "Press 'U' to upload", cv::Point(x, y),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(180, 180, 180), 1);
    y += lineHeight - 5;
    cv::putText(panel, "face image", cv::Point(x, y),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(180, 180, 180), 1);
    y += lineHeight;
    cv::putText(panel, "Mouse: Click slider", cv::Point(x, y),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(180, 180, 180), 1);
    y += lineHeight - 5;
    cv::putText(panel, "to adjust blend", cv::Point(x, y),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(180, 180, 180), 1);
    y += lineHeight;
    cv::putText(panel, "Press 'Q' or ESC", cv::Point(x, y),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(180, 180, 180), 1);
    y += lineHeight - 5;
    cv::putText(panel, "to exit", cv::Point(x, y),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(180, 180, 180), 1);
}

void ModernGUI::renderStatsPanel(cv::Mat& canvas, int previewWidth) {
    // Add overlay stats in top-left corner of preview
    int x = 20;
    int y = 30;
    
    // Semi-transparent background
    cv::Rect bgRect(x - 10, y - 25, 300, 100);
    cv::Mat overlay = canvas(bgRect).clone();
    cv::rectangle(canvas, bgRect, cv::Scalar(0, 0, 0), -1);
    cv::addWeighted(canvas(bgRect), 0.7, overlay, 0.3, 0, canvas(bgRect));
    
    // Source face status
    std::string statusText = sourceFaceLoaded ? "Face Loaded" : "No Face";
    cv::Scalar statusColor = sourceFaceLoaded ? cv::Scalar(100, 255, 100) : cv::Scalar(100, 100, 255);
    cv::putText(canvas, statusText, cv::Point(x, y),
                cv::FONT_HERSHEY_SIMPLEX, 0.8, statusColor, 2);
    y += 30;
    
    // Blend strength
    std::ostringstream blendOss;
    blendOss << "Blend: " << std::fixed << std::setprecision(0) << (blendStrength * 100) << "%";
    cv::putText(canvas, blendOss.str(), cv::Point(x, y),
                cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(100, 255, 100), 2);
    y += 30;
    
    // Face count
    std::ostringstream faceOss;
    faceOss << "Faces: " << faceCount;
    cv::Scalar faceColor = (faceCount > 0) ? cv::Scalar(100, 200, 255) : cv::Scalar(150, 150, 150);
    cv::putText(canvas, faceOss.str(), cv::Point(x, y),
                cv::FONT_HERSHEY_SIMPLEX, 0.8, faceColor, 2);
    y += 30;
    
    // FPS
    std::ostringstream fpsOss;
    fpsOss << "FPS: " << std::fixed << std::setprecision(1) << currentFPS;
    cv::putText(canvas, fpsOss.str(), cv::Point(x, y),
                cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(255, 200, 100), 2);
}

void ModernGUI::handleMouse(int event, int x, int y, int flags) {
    // Check if click is in the control panel area
    int panelX = x - previewWidth;
    
    // Check upload button
    if (panelX >= uploadButtonX && panelX <= uploadButtonX + uploadButtonWidth &&
        y >= uploadButtonY && y <= uploadButtonY + uploadButtonHeight) {
        if (event == cv::EVENT_LBUTTONDOWN) {
            std::string filePath = openFileDialog();
            if (!filePath.empty() && imageUploadCallback) {
                imageUploadCallback(filePath);
            }
        }
    }
    
    // Check if click is in the slider area
    if (panelX >= sliderX && panelX <= sliderX + sliderWidth &&
        y >= sliderY && y <= sliderY + sliderHeight) {
        
        if (event == cv::EVENT_LBUTTONDOWN || 
            (event == cv::EVENT_MOUSEMOVE && (flags & cv::EVENT_FLAG_LBUTTON))) {
            // Calculate new blend strength based on mouse X position
            float relativePos = static_cast<float>(panelX - sliderX) / sliderWidth;
            relativePos = std::max(0.0f, std::min(1.0f, relativePos));
            setBlendStrength(relativePos);
            draggingSlider = true;
        }
    }
    
    if (event == cv::EVENT_LBUTTONUP) {
        draggingSlider = false;
    }
}

std::string ModernGUI::openFileDialog() {
    std::cout << "Attempting to open file dialog..." << std::endl;
    
    FILE* pipe = nullptr;
    
    // Try kdialog first (since it's installed)
    std::string kdialogCmd = "/usr/bin/kdialog --title 'Select Face Image' --getopenfilename $HOME 'Image files (*.jpg *.jpeg *.png *.bmp)' 2>&1";
    pipe = popen(kdialogCmd.c_str(), "r");
    
    if (!pipe) {
        // Try without full path
        kdialogCmd = "kdialog --title 'Select Face Image' --getopenfilename $HOME 'Image files (*.jpg *.jpeg *.png *.bmp)' 2>&1";
        pipe = popen(kdialogCmd.c_str(), "r");
    }
    
    if (!pipe) {
        // Fallback: try zenity (GNOME/GTK)
        std::cout << "kdialog not available, trying zenity..." << std::endl;
        std::string zenityCmd = "zenity --file-selection --title='Select Face Image' "
                                "--file-filter='Image files (*.jpg *.jpeg *.png *.bmp) | *.jpg *.jpeg *.png *.bmp' "
                                "--width=800 --height=600 2>&1";
        pipe = popen(zenityCmd.c_str(), "r");
    }
    
    if (!pipe) {
        std::cerr << "Error: Could not open file dialog. Neither zenity nor kdialog found." << std::endl;
        std::cerr << "Please install one of them:" << std::endl;
        std::cerr << "  sudo apt-get install zenity    (for GNOME/GTK)" << std::endl;
        std::cerr << "  sudo apt-get install kdialog   (for KDE)" << std::endl;
        std::cerr << "Alternatively, use --face /path/to/image.jpg on the command line." << std::endl;
        return "";
    }
    
    char buffer[1024];
    std::string result = "";
    std::string errorOutput = "";
    
    // Read both stdout and stderr
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        std::string line(buffer);
        // Check if this is an error message
        if (line.find("Error") != std::string::npos || 
            line.find("error") != std::string::npos ||
            line.find("Gtk-") != std::string::npos) {
            errorOutput += line;
        } else if (!line.empty() && line[0] == '/') {
            // Likely a file path
            result += line;
        }
    }
    
    int status = pclose(pipe);
    
    // Check if command failed
    if (status != 0 || !errorOutput.empty()) {
        if (!errorOutput.empty()) {
            std::cerr << "File dialog error: " << errorOutput << std::endl;
        }
        if (result.empty()) {
            std::cerr << "No file was selected or dialog was cancelled." << std::endl;
            return "";
        }
    }
    
    // Remove trailing newline and whitespace
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r' || result.back() == ' ')) {
        result.pop_back();
    }
    
    if (!result.empty()) {
        std::cout << "File dialog returned: " << result << std::endl;
    }
    
    return result;
}
