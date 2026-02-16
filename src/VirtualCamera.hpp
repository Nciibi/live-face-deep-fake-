#ifndef VIRTUAL_CAMERA_HPP
#define VIRTUAL_CAMERA_HPP

#include <opencv2/opencv.hpp>
#include <string>
#include <memory>

class VirtualCamera {
public:
    VirtualCamera();
    ~VirtualCamera();
    
    // Initialize virtual camera with specified device path (e.g., "/dev/video2")
    // Returns true if successful
    bool initialize(const std::string& devicePath, int width = 640, int height = 480);
    
    // Write a frame to the virtual camera
    bool writeFrame(const cv::Mat& frame);
    
    // Check if virtual camera is ready
    bool isReady() const { return ready; }
    
    // Get the device path
    std::string getDevicePath() const { return devicePath; }
    
    void release();

private:
    bool ready;
    std::string devicePath;
    int width;
    int height;
    FILE* ffmpegProcess;
    std::string ffmpegCommand;
    
    // Helper to find available v4l2loopback device
    std::string findVirtualCameraDevice();
};

#endif // VIRTUAL_CAMERA_HPP
