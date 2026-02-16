#ifndef FACE_ANONYMIZER_HPP
#define FACE_ANONYMIZER_HPP

#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <string>

class FaceAnonymizer {
public:
    FaceAnonymizer();
    // Simplified loadModel to take just the ONNX path
    bool loadModel(const std::string& modelPath);
    void detectAndBlur(cv::Mat& frame);
    
    // Set blur intensity (0.0 to 1.0, where 1.0 is maximum blur)
    void setBlurIntensity(float intensity);
    float getBlurIntensity() const { return blurIntensity; }
    
    // Get the number of faces detected in the last frame
    int getFaceCount() const { return lastFaceCount; }

private:
    cv::Ptr<cv::FaceDetectorYN> faceDetector;
    float blurIntensity; // 0.0 to 1.0
    int lastFaceCount; // Number of faces detected in last frame
};

#endif // FACE_ANONYMIZER_HPP
