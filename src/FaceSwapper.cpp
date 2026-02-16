#include "FaceSwapper.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>

FaceSwapper::FaceSwapper() 
    : sourceFaceLoaded(false)
    , blendStrength(0.95f)
    , lastFaceCount(0)
{
}

bool FaceSwapper::loadModel(const std::string& modelPath) {
    try {
        // Initialize FaceDetectorYN with YuNet model
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

bool FaceSwapper::loadSourceFace(const std::string& imagePath) {
    cv::Mat image = cv::imread(imagePath);
    if (image.empty()) {
        std::cerr << "Error: Could not load source face image: " << imagePath << std::endl;
        return false;
    }
    return loadSourceFace(image);
}

bool FaceSwapper::loadSourceFace(const cv::Mat& image) {
    if (image.empty() || faceDetector.empty()) {
        return false;
    }
    
    sourceFaceImage = image.clone();
    
    // Set input size for face detection
    faceDetector->setInputSize(image.size());
    
    // Detect face in source image
    cv::Mat faces;
    faceDetector->detect(image, faces);
    
    if (faces.rows == 0) {
        std::cerr << "Error: No face detected in source image." << std::endl;
        return false;
    }
    
    // Use the first detected face
    float x = faces.at<float>(0, 0);
    float y = faces.at<float>(0, 1);
    float w = faces.at<float>(0, 2);
    float h = faces.at<float>(0, 3);
    
    sourceFaceRect = cv::Rect(
        std::max(0, static_cast<int>(x)),
        std::max(0, static_cast<int>(y)),
        std::min(image.cols - static_cast<int>(x), static_cast<int>(w)),
        std::min(image.rows - static_cast<int>(y), static_cast<int>(h))
    );
    
    // Extract landmarks (absolute coordinates)
    sourceLandmarks = extractLandmarks(faces, 0);
    
    if (sourceLandmarks.empty()) {
        std::cerr << "Error: Could not extract landmarks from source face." << std::endl;
        return false;
    }
    
    sourceFaceLoaded = true;
    std::cout << "Source face loaded successfully! Face size: " 
              << sourceFaceRect.width << "x" << sourceFaceRect.height << std::endl;
    return true;
}

std::vector<cv::Point2f> FaceSwapper::extractLandmarks(const cv::Mat& faces, int faceIndex) {
    std::vector<cv::Point2f> landmarks;
    
    if (faceIndex >= faces.rows) {
        return landmarks;
    }
    
    // YuNet format: [x, y, w, h, x_re, y_re, x_le, y_le, x_nt, y_nt, x_rcm, y_rcm, x_lcm, y_lcm, confidence]
    // Right eye: 4,5; Left eye: 6,7; Nose: 8,9; Right mouth corner: 10,11; Left mouth corner: 12,13
    
    landmarks.push_back(cv::Point2f(faces.at<float>(faceIndex, 6), faces.at<float>(faceIndex, 7))); // Left eye
    landmarks.push_back(cv::Point2f(faces.at<float>(faceIndex, 4), faces.at<float>(faceIndex, 5))); // Right eye
    landmarks.push_back(cv::Point2f(faces.at<float>(faceIndex, 8), faces.at<float>(faceIndex, 9))); // Nose tip
    landmarks.push_back(cv::Point2f(faces.at<float>(faceIndex, 12), faces.at<float>(faceIndex, 13))); // Left mouth corner
    landmarks.push_back(cv::Point2f(faces.at<float>(faceIndex, 10), faces.at<float>(faceIndex, 11))); // Right mouth corner
    
    return landmarks;
}

void FaceSwapper::setBlendStrength(float strength) {
    blendStrength = std::max(0.0f, std::min(1.0f, strength));
}

void FaceSwapper::detectAndSwap(cv::Mat& frame) {
    if (frame.empty() || faceDetector.empty() || !sourceFaceLoaded) {
        return;
    }
    
    // Set input size if it changed
    faceDetector->setInputSize(frame.size());
    
    // Detect faces in current frame
    cv::Mat faces;
    faceDetector->detect(frame, faces);
    
    lastFaceCount = faces.rows;
    
    // Swap each detected face
    for (int i = 0; i < faces.rows; i++) {
        float x = faces.at<float>(i, 0);
        float y = faces.at<float>(i, 1);
        float w = faces.at<float>(i, 2);
        float h = faces.at<float>(i, 3);
        
        cv::Rect faceRect(
            std::max(0, static_cast<int>(x)),
            std::max(0, static_cast<int>(y)),
            std::min(frame.cols - static_cast<int>(x), static_cast<int>(w)),
            std::min(frame.rows - static_cast<int>(y), static_cast<int>(h))
        );
        
        if (faceRect.width <= 0 || faceRect.height <= 0) continue;
        
        // Extract landmarks for target face
        std::vector<cv::Point2f> targetLandmarks = extractLandmarks(faces, i);
        if (targetLandmarks.empty()) continue;
        
        // Perform face swap
        swapFace(frame, faceRect, targetLandmarks);
    }
}

void FaceSwapper::swapFace(cv::Mat& targetFrame, const cv::Rect& targetFaceRect, 
                           const std::vector<cv::Point2f>& targetLandmarks) {
    if (sourceLandmarks.size() != 5 || targetLandmarks.size() != 5) {
        return;
    }
    
    // Adjust landmarks to be relative to face rectangles
    std::vector<cv::Point2f> sourcePoints = getFacePoints(sourceLandmarks);
    std::vector<cv::Point2f> targetPoints = getFacePoints(targetLandmarks);
    
    // Adjust target points to be relative to target face rect
    for (auto& pt : targetPoints) {
        pt.x -= targetFaceRect.x;
        pt.y -= targetFaceRect.y;
    }
    
    // Calculate affine transformation matrix
    cv::Mat transform = cv::getAffineTransform(sourcePoints, targetPoints);
    
    // Extract source face region
    cv::Mat sourceFaceROI = sourceFaceImage(sourceFaceRect);
    
    // Warp source face to match target face size
    cv::Mat warpedSource;
    cv::warpAffine(sourceFaceROI, warpedSource, transform, targetFaceRect.size());
    
    // Adjust target landmarks to be relative to face rect for mask creation
    std::vector<cv::Point2f> targetLandmarksRelative = targetLandmarks;
    for (auto& pt : targetLandmarksRelative) {
        pt.x -= targetFaceRect.x;
        pt.y -= targetFaceRect.y;
    }
    
    // Create mask for face region (relative to face rect)
    cv::Mat mask = getFaceMask(targetFaceRect.size(), targetLandmarksRelative);
    
    // Create full-size mask
    cv::Mat fullMask = cv::Mat::zeros(targetFrame.size(), CV_8UC1);
    mask.copyTo(fullMask(targetFaceRect));
    
    // Extract target face region
    cv::Mat targetFaceROI = targetFrame(targetFaceRect);
    
    // Blend warped source onto target face region
    cv::Mat blendedFace;
    cv::addWeighted(warpedSource, blendStrength, targetFaceROI, 1.0f - blendStrength, 0, blendedFace);
    
    // Apply mask for smooth blending
    cv::Mat mask3Channel;
    cv::cvtColor(mask, mask3Channel, cv::COLOR_GRAY2BGR);
    mask3Channel.convertTo(mask3Channel, CV_32F, 1.0/255.0);
    
    cv::Mat blendedFaceFloat, targetFaceFloat;
    blendedFace.convertTo(blendedFaceFloat, CV_32F);
    targetFaceROI.convertTo(targetFaceFloat, CV_32F);
    
    // Properly compute inverse mask to avoid scaleAdd errors
    cv::Mat invMask = cv::Mat::ones(mask3Channel.size(), mask3Channel.type()) - mask3Channel;
    
    cv::Mat resultFloat = blendedFaceFloat.mul(mask3Channel) + targetFaceFloat.mul(invMask);
    resultFloat.convertTo(targetFaceROI, CV_8U);
}

std::vector<cv::Point2f> FaceSwapper::getFacePoints(const std::vector<cv::Point2f>& landmarks) {
    // Use 3 key points for affine transformation: left eye, right eye, nose
    // These should be relative to the source face rect
    std::vector<cv::Point2f> points;
    if (landmarks.size() >= 3) {
        // Adjust landmarks to be relative to source face rect
        points.push_back(cv::Point2f(landmarks[0].x - sourceFaceRect.x, 
                                     landmarks[0].y - sourceFaceRect.y)); // Left eye
        points.push_back(cv::Point2f(landmarks[1].x - sourceFaceRect.x, 
                                     landmarks[1].y - sourceFaceRect.y)); // Right eye
        points.push_back(cv::Point2f(landmarks[2].x - sourceFaceRect.x, 
                                     landmarks[2].y - sourceFaceRect.y)); // Nose
    }
    return points;
}

cv::Mat FaceSwapper::getFaceMask(const cv::Size& size, const std::vector<cv::Point2f>& landmarks) {
    cv::Mat mask = cv::Mat::zeros(size, CV_8UC1);
    
    if (landmarks.size() < 5) {
        // Fallback: elliptical mask
        cv::ellipse(mask, cv::Point(size.width/2, size.height/2),
                   cv::Size(size.width/2 * 0.9, size.height/2 * 0.9), 0, 0, 360, cv::Scalar(255), -1);
        cv::GaussianBlur(mask, mask, cv::Size(21, 21), 0);
        return mask;
    }
    
    // Adjust landmarks to be relative to the mask size (they should already be relative to face rect)
    std::vector<cv::Point> hullPoints;
    for (const auto& pt : landmarks) {
        int x = static_cast<int>(pt.x);
        int y = static_cast<int>(pt.y);
        // Clamp to mask bounds
        x = std::max(0, std::min(size.width - 1, x));
        y = std::max(0, std::min(size.height - 1, y));
        hullPoints.push_back(cv::Point(x, y));
    }
    
    // Add boundary points for better coverage
    int margin = std::min(size.width, size.height) / 10;
    hullPoints.push_back(cv::Point(margin, margin));
    hullPoints.push_back(cv::Point(size.width - margin, margin));
    hullPoints.push_back(cv::Point(margin, size.height - margin));
    hullPoints.push_back(cv::Point(size.width - margin, size.height - margin));
    
    // Add forehead point (estimated above eyes)
    if (landmarks.size() >= 2) {
        float eyeY = (landmarks[0].y + landmarks[1].y) / 2.0f;
        float foreheadY = eyeY - (landmarks[2].y - eyeY) * 0.5f; // Above eyes
        hullPoints.push_back(cv::Point(size.width/2, static_cast<int>(std::max(0.0f, foreheadY))));
    }
    
    // Create convex hull
    std::vector<cv::Point> hull;
    cv::convexHull(hullPoints, hull);
    
    // Fill convex hull
    cv::fillConvexPoly(mask, hull, cv::Scalar(255));
    
    // Apply Gaussian blur for smooth edges
    int blurSize = std::max(5, std::min(size.width, size.height) / 10);
    if (blurSize % 2 == 0) blurSize++;
    cv::GaussianBlur(mask, mask, cv::Size(blurSize, blurSize), 0);
    
    return mask;
}
