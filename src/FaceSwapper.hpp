#ifndef FACE_SWAPPER_HPP
#define FACE_SWAPPER_HPP

#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <string>
#include <vector>

class FaceSwapper {
public:
    FaceSwapper();
    
    // Load face detection model
    bool loadModel(const std::string& modelPath);
    
    // Load source face image for swapping
    bool loadSourceFace(const std::string& imagePath);
    bool loadSourceFace(const cv::Mat& image);
    
    // Check if source face is loaded
    bool isSourceFaceLoaded() const { return sourceFaceLoaded; }
    
    // Perform face swapping on frame
    void detectAndSwap(cv::Mat& frame);
    
    // Get the number of faces detected in the last frame
    int getFaceCount() const { return lastFaceCount; }
    
    // Set blending strength (0.0 to 1.0, where 1.0 is full swap)
    void setBlendStrength(float strength);
    float getBlendStrength() const { return blendStrength; }

private:
    cv::Ptr<cv::FaceDetectorYN> faceDetector;
    
    // Source face data
    cv::Mat sourceFaceImage;
    cv::Mat sourceFaceAligned;
    std::vector<cv::Point2f> sourceLandmarks;
    cv::Rect sourceFaceRect;
    bool sourceFaceLoaded;
    
    // Face swapping parameters
    float blendStrength;
    int lastFaceCount;
    
    // Helper functions
    std::vector<cv::Point2f> extractLandmarks(const cv::Mat& faces, int faceIndex);
    cv::Mat alignFace(const cv::Mat& image, const std::vector<cv::Point2f>& landmarks, const cv::Rect& faceRect);
    void swapFace(cv::Mat& targetFrame, const cv::Rect& targetFaceRect, 
                  const std::vector<cv::Point2f>& targetLandmarks);
    cv::Mat getFaceMask(const cv::Size& size, const std::vector<cv::Point2f>& landmarks);
    std::vector<cv::Point2f> getFacePoints(const std::vector<cv::Point2f>& landmarks);
};

#endif // FACE_SWAPPER_HPP
