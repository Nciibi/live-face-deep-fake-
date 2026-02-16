#ifndef ADVANCED_FACE_SWAPPER_HPP
#define ADVANCED_FACE_SWAPPER_HPP

#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/dnn.hpp>
#include <string>
#include <vector>
#include <deque>

class AdvancedFaceSwapper {
public:
    AdvancedFaceSwapper();
    ~AdvancedFaceSwapper();
    
    // Load models
    bool loadFaceDetectionModel(const std::string& modelPath);
    bool loadArcFaceModel(const std::string& modelPath);
    bool loadInSwapperModel(const std::string& modelPath);
    bool loadGFPGANModel(const std::string& modelPath);
    
    // Load source face image for swapping
    bool loadSourceFace(const std::string& imagePath);
    bool loadSourceFace(const cv::Mat& image);
    
    // Check if source face is loaded
    bool isSourceFaceLoaded() const { return sourceFaceLoaded; }
    
    // Perform face swapping on frame (full pipeline)
    void detectAndSwap(cv::Mat& frame);
    
    // Get the number of faces detected in the last frame
    int getFaceCount() const { return lastFaceCount; }
    
    // Settings
    void setBlendStrength(float strength);
    float getBlendStrength() const { return blendStrength; }
    
    void setEnableGFPGAN(bool enable) { enableGFPGAN = enable; }
    bool getEnableGFPGAN() const { return enableGFPGAN; }
    
    void setTemporalStabilization(bool enable) { useTemporalStabilization = enable; }
    bool getTemporalStabilization() const { return useTemporalStabilization; }
    
    void setStabilizationStrength(float strength) { stabilizationStrength = std::max(0.0f, std::min(1.0f, strength)); }
    float getStabilizationStrength() const { return stabilizationStrength; }

private:
    // Models
    cv::Ptr<cv::FaceDetectorYN> faceDetector;
    cv::dnn::Net arcFaceNet;
    cv::dnn::Net inSwapperNet;
    cv::dnn::Net gfpganNet;
    
    bool arcFaceLoaded;
    bool inSwapperLoaded;
    bool gfpganLoaded;
    
    // Source face data
    cv::Mat sourceFaceImage;
    cv::Mat sourceFaceAligned;
    std::vector<cv::Point2f> sourceLandmarks;
    cv::Rect sourceFaceRect;
    cv::Mat sourceFaceEmbedding;
    bool sourceFaceLoaded;
    
    // Face swapping parameters
    float blendStrength;
    bool enableGFPGAN;
    bool useTemporalStabilization;
    float stabilizationStrength;
    int lastFaceCount;
    
    // Temporal stabilization buffers
    std::deque<cv::Mat> previousFaces;
    std::deque<std::vector<cv::Point2f>> previousLandmarks;
    static const int MAX_HISTORY = 5;
    
    // Pipeline steps
    std::vector<cv::Point2f> extractLandmarks(const cv::Mat& faces, int faceIndex);
    cv::Mat preprocessFrame(const cv::Mat& frame);
    cv::Mat alignFace(const cv::Mat& image, const std::vector<cv::Point2f>& landmarks, const cv::Rect& faceRect, int outputSize = 512);
    cv::Mat extractFaceEmbedding(const cv::Mat& alignedFace);
    cv::Mat swapFaceWithModel(const cv::Mat& targetFace, const cv::Mat& sourceEmbedding);
    cv::Mat restoreFace(const cv::Mat& swappedFace);
    cv::Mat generateFaceMask(const cv::Size& size, const std::vector<cv::Point2f>& landmarks);
    cv::Mat blendFace(const cv::Mat& swappedFace, const cv::Mat& originalFrame, const cv::Rect& faceRect, const cv::Mat& mask);
    cv::Mat stabilizeFace(const cv::Mat& currentFace, const std::vector<cv::Point2f>& currentLandmarks);
    
    // Helper functions
    std::vector<cv::Point2f> getFacePoints(const std::vector<cv::Point2f>& landmarks);
    cv::Mat normalizeImage(const cv::Mat& image);
    cv::Mat denormalizeImage(const cv::Mat& image);
};

#endif // ADVANCED_FACE_SWAPPER_HPP
