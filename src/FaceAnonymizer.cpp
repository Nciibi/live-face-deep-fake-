#include "FaceAnonymizer.hpp"
#include <iostream>

FaceAnonymizer::FaceAnonymizer() : blurIntensity(0.5f), lastFaceCount(0) {}

bool FaceAnonymizer::loadModel(const std::string& modelPath) {
    try {
        // Initialize FaceDetectorYN
        // Input size is set to 0x0 initially, will be set on first frame
        // Score threshold: 0.9 (high confidence for accuracy as requested)
        // NMS threshold: 0.3
        // Top K: 5000
        faceDetector = cv::FaceDetectorYN::create(modelPath, "", cv::Size(320, 320), 0.9f, 0.3f, 5000);
        
        if (faceDetector.empty()) {
            std::cerr << "Error: Could not load YuNet model." << std::endl;
            return false;
        }
    } catch (const cv::Exception& e) {
        std::cerr << "Exception loading model: " << e.what() << std::endl;
        return false;
    }
    return true;
}

void FaceAnonymizer::setBlurIntensity(float intensity) {
    blurIntensity = std::max(0.0f, std::min(1.0f, intensity)); // Clamp between 0 and 1
}

void FaceAnonymizer::detectAndBlur(cv::Mat& frame) {
    if (frame.empty() || faceDetector.empty()) return;

    // Set input size if it changed
    faceDetector->setInputSize(frame.size());

    cv::Mat faces;
    faceDetector->detect(frame, faces);
    
    // Store face count
    lastFaceCount = faces.rows;

    // faces format: [x, y, w, h, x_re, y_re, x_le, y_le, x_nt, y_nt, x_rcm, y_rcm, x_lcm, y_lcm, confidence]
    for (int i = 0; i < faces.rows; i++) {
        // Get bounding box
        float x = faces.at<float>(i, 0);
        float y = faces.at<float>(i, 1);
        float w = faces.at<float>(i, 2);
        float h = faces.at<float>(i, 3);

        // Convert to integers and clip to frame
        int x1 = static_cast<int>(x);
        int y1 = static_cast<int>(y);
        int x2 = static_cast<int>(x + w);
        int y2 = static_cast<int>(y + h);

        x1 = std::max(0, x1);
        y1 = std::max(0, y1);
        x2 = std::min(frame.cols, x2);
        y2 = std::min(frame.rows, y2);
        
        if (x2 <= x1 || y2 <= y1) continue;
        
        cv::Rect faceRect(x1, y1, x2 - x1, y2 - y1);
        cv::Mat faceROI = frame(faceRect);

        // --- Elliptical Blur ---
        cv::Mat mask = cv::Mat::zeros(faceROI.size(), CV_8UC1);
        cv::Point center(faceRect.width / 2, faceRect.height / 2);
        cv::Size axes(faceRect.width / 2, faceRect.height / 2);
        cv::ellipse(mask, center, axes, 0, 0, 360, cv::Scalar(255), -1);

        cv::Mat blurredFace;
        // Calculate blur kernel size based on face size and intensity
        // Base size is face width/3, scaled by intensity (0.5 to 2.0x)
        float baseKsize = faceRect.width / 3.0f;
        float intensityMultiplier = 0.5f + (blurIntensity * 1.5f); // Range: 0.5 to 2.0
        int ksize = static_cast<int>(baseKsize * intensityMultiplier);
        ksize = ksize | 1; // Ensure odd number
        ksize = std::max(3, std::min(ksize, 101)); // Clamp between 3 and 101
        
        cv::GaussianBlur(faceROI, blurredFace, cv::Size(ksize, ksize), 0);

        blurredFace.copyTo(faceROI, mask);

        // --- Optional: Draw Landmarks for Debugging (Verification of High Accuracy) ---
        // Right Eye: 4,5; Left Eye: 6,7; Nose: 8,9; Right Mouth: 10,11; Left Mouth: 12,13
        /*
        cv::circle(frame, cv::Point(faces.at<float>(i, 4), faces.at<float>(i, 5)), 2, cv::Scalar(0, 255, 0), 2);
        cv::circle(frame, cv::Point(faces.at<float>(i, 6), faces.at<float>(i, 7)), 2, cv::Scalar(0, 255, 0), 2);
        */
    }
}
