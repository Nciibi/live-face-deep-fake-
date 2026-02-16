#include "VirtualCamera.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

VirtualCamera::VirtualCamera() : ready(false), ffmpegProcess(nullptr), width(640), height(480) {}

VirtualCamera::~VirtualCamera() {
    release();
}

std::string VirtualCamera::findVirtualCameraDevice() {
    // Common v4l2loopback device paths
    const char* possibleDevices[] = {
        "/dev/video2",
        "/dev/video3",
        "/dev/video4",
        "/dev/video5",
        "/dev/video6",
        "/dev/video7",
        "/dev/video8",
        "/dev/video9",
        "/dev/video10",
        nullptr
    };
    
    for (int i = 0; possibleDevices[i] != nullptr; i++) {
        struct stat st;
        if (stat(possibleDevices[i], &st) == 0) {
            // Check if it's a character device
            if (S_ISCHR(st.st_mode)) {
                // Try to open it to verify it's accessible
                int fd = open(possibleDevices[i], O_RDWR);
                if (fd >= 0) {
                    close(fd);
                    return std::string(possibleDevices[i]);
                }
            }
        }
    }
    
    return "";
}

bool VirtualCamera::initialize(const std::string& devicePath, int width, int height) {
    release();
    
    this->width = width;
    this->height = height;
    
    // If no device path provided, try to find one
    if (devicePath.empty()) {
        this->devicePath = findVirtualCameraDevice();
        if (this->devicePath.empty()) {
            std::cerr << "Error: No virtual camera device found." << std::endl;
            std::cerr << "Please set up v4l2loopback first:" << std::endl;
            std::cerr << "  sudo modprobe v4l2loopback devices=1 video_nr=2 card_label=\"FaceAnonymizer\"" << std::endl;
            return false;
        }
    } else {
        this->devicePath = devicePath;
    }
    
    // Check if device exists
    struct stat st;
    if (stat(this->devicePath.c_str(), &st) != 0) {
        std::cerr << "Error: Virtual camera device " << this->devicePath << " does not exist." << std::endl;
        std::cerr << "Please set up v4l2loopback first:" << std::endl;
        std::cerr << "  sudo modprobe v4l2loopback devices=1 video_nr=2 card_label=\"FaceAnonymizer\"" << std::endl;
        return false;
    }
    
    // Build ffmpeg command to pipe frames to v4l2loopback
    std::ostringstream cmd;
    cmd << "ffmpeg -f rawvideo -pixel_format bgr24 -video_size " 
        << width << "x" << height 
        << " -framerate 30 -i - -vf format=yuv420p -f v4l2 " 
        << this->devicePath << " 2>/dev/null";
    
    ffmpegCommand = cmd.str();
    
    // Open ffmpeg process
    ffmpegProcess = popen(ffmpegCommand.c_str(), "w");
    if (!ffmpegProcess) {
        std::cerr << "Error: Failed to start ffmpeg process." << std::endl;
        std::cerr << "Make sure ffmpeg is installed: sudo apt-get install ffmpeg" << std::endl;
        return false;
    }
    
    // Set buffer to unbuffered for low latency
    setvbuf(ffmpegProcess, nullptr, _IONBF, 0);
    
    ready = true;
    std::cout << "Virtual camera initialized: " << this->devicePath << std::endl;
    std::cout << "Resolution: " << width << "x" << height << std::endl;
    return true;
}

bool VirtualCamera::writeFrame(const cv::Mat& frame) {
    if (!ready || !ffmpegProcess) {
        return false;
    }
    
    if (frame.empty()) {
        return false;
    }
    
    // Resize frame if needed
    cv::Mat resizedFrame;
    if (frame.cols != width || frame.rows != height) {
        cv::resize(frame, resizedFrame, cv::Size(width, height));
    } else {
        resizedFrame = frame;
    }
    
    // Write frame data to ffmpeg stdin
    size_t written = fwrite(resizedFrame.data, 1, resizedFrame.total() * resizedFrame.elemSize(), ffmpegProcess);
    
    if (written != resizedFrame.total() * resizedFrame.elemSize()) {
        std::cerr << "Warning: Failed to write complete frame to virtual camera." << std::endl;
        return false;
    }
    
    fflush(ffmpegProcess);
    return true;
}

void VirtualCamera::release() {
    if (ffmpegProcess) {
        pclose(ffmpegProcess);
        ffmpegProcess = nullptr;
    }
    ready = false;
}
