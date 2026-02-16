#include "AdvancedFaceSwapper.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>

AdvancedFaceSwapper::AdvancedFaceSwapper() 
    : arcFaceLoaded(false)
    , inSwapperLoaded(false)
    , gfpganLoaded(false)
    , sourceFaceLoaded(false)
    , blendStrength(0.95f)
    , enableGFPGAN(false)
    , useTemporalStabilization(true)
    , stabilizationStrength(0.7f)
    , lastFaceCount(0)
{
}

AdvancedFaceSwapper::~AdvancedFaceSwapper() {
    // Cleanup handled by smart pointers
}

bool AdvancedFaceSwapper::loadFaceDetectionModel(const std::string& modelPath) {
    try {
        faceDetector = cv::FaceDetectorYN::create(modelPath, "", cv::Size(320, 320), 0.9f, 0.3f, 5000);
        if (faceDetector.empty()) {
            std::cerr << "Error: Could not load YuNet face detection model." << std::endl;
            return false;
        }
        std::cout << "Face detection model loaded successfully." << std::endl;
        return true;
    } catch (const cv::Exception& e) {
        std::cerr << "Exception loading face detection model: " << e.what() << std::endl;
        return false;
    }
}

bool AdvancedFaceSwapper::loadArcFaceModel(const std::string& modelPath) {
    try {
        arcFaceNet = cv::dnn::readNetFromONNX(modelPath);
        if (arcFaceNet.empty()) {
            std::cerr << "Warning: Could not load ArcFace model from: " << modelPath << std::endl;
            std::cerr << "Face embedding extraction will use fallback method." << std::endl;
            return false;
        }
        
        // Set backend (prefer CPU for compatibility, can be changed to CUDA)
        arcFaceNet.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        arcFaceNet.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
        
        arcFaceLoaded = true;
        std::cout << "ArcFace model loaded successfully." << std::endl;
        return true;
    } catch (const cv::Exception& e) {
        std::cerr << "Exception loading ArcFace model: " << e.what() << std::endl;
        return false;
    }
}

bool AdvancedFaceSwapper::loadInSwapperModel(const std::string& modelPath) {
    try {
        inSwapperNet = cv::dnn::readNetFromONNX(modelPath);
        if (inSwapperNet.empty()) {
            std::cerr << "Warning: Could not load INSwapper model from: " << modelPath << std::endl;
            std::cerr << "Face swapping will use fallback affine transformation." << std::endl;
            return false;
        }
        
        inSwapperNet.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        inSwapperNet.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
        
        inSwapperLoaded = true;
        std::cout << "INSwapper model loaded successfully." << std::endl;
        std::cout << "  Expected inputs: [target] (1,3,128,128) and [source] (1,512)" << std::endl;
        return true;
    } catch (const cv::Exception& e) {
        std::cerr << "Exception loading INSwapper model: " << e.what() << std::endl;
        return false;
    }
}

bool AdvancedFaceSwapper::loadGFPGANModel(const std::string& modelPath) {
    try {
        // GFPGAN models are typically PyTorch format, not ONNX
        // For now, we'll skip this and add support later
        std::cerr << "Warning: GFPGAN model loading not yet implemented." << std::endl;
        std::cerr << "GFPGAN restoration will be skipped." << std::endl;
        gfpganLoaded = false;
        return false;
    } catch (const cv::Exception& e) {
        std::cerr << "Exception loading GFPGAN model: " << e.what() << std::endl;
        return false;
    }
}

bool AdvancedFaceSwapper::loadSourceFace(const std::string& imagePath) {
    cv::Mat image = cv::imread(imagePath);
    if (image.empty()) {
        std::cerr << "Error: Could not load source face image: " << imagePath << std::endl;
        return false;
    }
    return loadSourceFace(image);
}

bool AdvancedFaceSwapper::loadSourceFace(const cv::Mat& image) {
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
    
    // Extract landmarks
    sourceLandmarks = extractLandmarks(faces, 0);
    
    if (sourceLandmarks.empty()) {
        std::cerr << "Error: Could not extract landmarks from source face." << std::endl;
        return false;
    }
    
    // Align source face
    sourceFaceAligned = alignFace(sourceFaceImage, sourceLandmarks, sourceFaceRect, 512);
    
    // Extract face embedding if ArcFace is loaded
    if (arcFaceLoaded && !sourceFaceAligned.empty()) {
        sourceFaceEmbedding = extractFaceEmbedding(sourceFaceAligned);
    }
    
    sourceFaceLoaded = true;
    std::cout << "Source face loaded successfully! Face size: " 
              << sourceFaceRect.width << "x" << sourceFaceRect.height << std::endl;
    return true;
}

std::vector<cv::Point2f> AdvancedFaceSwapper::extractLandmarks(const cv::Mat& faces, int faceIndex) {
    std::vector<cv::Point2f> landmarks;
    
    if (faceIndex >= faces.rows) {
        return landmarks;
    }
    
    // YuNet format: [x, y, w, h, x_re, y_re, x_le, y_le, x_nt, y_nt, x_rcm, y_rcm, x_lcm, y_lcm, confidence]
    landmarks.push_back(cv::Point2f(faces.at<float>(faceIndex, 6), faces.at<float>(faceIndex, 7))); // Left eye
    landmarks.push_back(cv::Point2f(faces.at<float>(faceIndex, 4), faces.at<float>(faceIndex, 5))); // Right eye
    landmarks.push_back(cv::Point2f(faces.at<float>(faceIndex, 8), faces.at<float>(faceIndex, 9))); // Nose tip
    landmarks.push_back(cv::Point2f(faces.at<float>(faceIndex, 12), faces.at<float>(faceIndex, 13))); // Left mouth corner
    landmarks.push_back(cv::Point2f(faces.at<float>(faceIndex, 10), faces.at<float>(faceIndex, 11))); // Right mouth corner
    
    return landmarks;
}

cv::Mat AdvancedFaceSwapper::preprocessFrame(const cv::Mat& frame) {
    // Apply any preprocessing: denoising, contrast enhancement, etc.
    cv::Mat processed = frame.clone();
    
    // Optional: Apply CLAHE for better face detection in varying lighting
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(2.0, cv::Size(8, 8));
    std::vector<cv::Mat> channels;
    cv::split(processed, channels);
    clahe->apply(channels[0], channels[0]);
    clahe->apply(channels[1], channels[1]);
    clahe->apply(channels[2], channels[2]);
    cv::merge(channels, processed);
    
    return processed;
}

cv::Mat AdvancedFaceSwapper::alignFace(const cv::Mat& image, const std::vector<cv::Point2f>& landmarks, 
                                       const cv::Rect& faceRect, int outputSize) {
    if (landmarks.size() < 5) {
        return cv::Mat();
    }
    
    // Standard face alignment points (for 512x512 output)
    std::vector<cv::Point2f> dstPoints = {
        cv::Point2f(0.31556875f * outputSize, 0.46157407f * outputSize), // Left eye
        cv::Point2f(0.68262292f * outputSize, 0.46157407f * outputSize), // Right eye
        cv::Point2f(0.50026250f * outputSize, 0.64050537f * outputSize), // Nose tip
        cv::Point2f(0.37015179f * outputSize, 0.82469196f * outputSize), // Left mouth corner
        cv::Point2f(0.63151667f * outputSize, 0.82469196f * outputSize)  // Right mouth corner
    };
    
    // Adjust source landmarks to be relative to face rect
    std::vector<cv::Point2f> srcPoints;
    for (const auto& pt : landmarks) {
        srcPoints.push_back(cv::Point2f(pt.x - faceRect.x, pt.y - faceRect.y));
    }
    
    // Calculate similarity transformation
    cv::Mat transform = cv::estimateAffinePartial2D(srcPoints, dstPoints);
    
    if (transform.empty()) {
        return cv::Mat();
    }
    
    // Extract face region
    cv::Mat faceROI = image(faceRect);
    
    // Warp face
    cv::Mat aligned;
    cv::warpAffine(faceROI, aligned, transform, cv::Size(outputSize, outputSize));
    
    return aligned;
}

cv::Mat AdvancedFaceSwapper::extractFaceEmbedding(const cv::Mat& alignedFace) {
    if (!arcFaceLoaded || alignedFace.empty()) {
        // Fallback: return empty embedding (will use fallback swapping)
        return cv::Mat();
    }
    
    try {
        // Preprocess for ArcFace (normalize to [-1, 1] and resize to 112x112)
        cv::Mat input;
        cv::resize(alignedFace, input, cv::Size(112, 112));
        input.convertTo(input, CV_32F, 1.0/127.5, -1.0); // Normalize to [-1, 1]
        
        // Create blob
        cv::Mat blob = cv::dnn::blobFromImage(input, 1.0, cv::Size(112, 112), cv::Scalar(), true, false);
        
        // Set input
        arcFaceNet.setInput(blob);
        
        // Forward pass
        std::vector<cv::Mat> outputs;
        arcFaceNet.forward(outputs);
        
        if (!outputs.empty()) {
            // Normalize embedding
            cv::Mat embedding = outputs[0];
            
            // Ensure embedding is 1D
            if (embedding.dims > 1 && embedding.rows > 1) {
                embedding = embedding.reshape(1, 1);
            }
            
            if (embedding.empty() || embedding.total() == 0) {
                std::cerr << "Error: Invalid embedding from ArcFace" << std::endl;
                return cv::Mat();
            }
            
            // Reshape to 1x512 if needed
            if (embedding.cols != 512) {
                embedding = embedding.reshape(0, 1);
                if (embedding.cols != 512) {
                    std::cerr << "Warning: Embedding dimension mismatch. Expected 512, got " << embedding.cols << std::endl;
                    // Still return it, might work
                }
            }
            
            // Normalize using safe method
            cv::Mat norm = embedding.clone();
            double normVal = cv::norm(norm, cv::NORM_L2);
            if (normVal > 0) {
                norm = norm / normVal;
            }
            
            return norm;
        }
    } catch (const cv::Exception& e) {
        std::cerr << "Error extracting face embedding: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown error in extractFaceEmbedding" << std::endl;
    }
    
    return cv::Mat();
}

cv::Mat AdvancedFaceSwapper::swapFaceWithModel(const cv::Mat& targetFace, const cv::Mat& sourceEmbedding) {
    // If no embedding provided or model not loaded, use fallback
    if (!inSwapperLoaded || sourceEmbedding.empty() || targetFace.empty()) {
        return cv::Mat(); // Return empty to trigger fallback
    }
    
    try {
        // INSwapper model expects:
        // - Input 1: target face image (512x512, BGR, normalized to [-1, 1])
        // - Input 2: source face embedding (512-dim vector, normalized)
        // - Output: swapped face (512x512, BGR, normalized to [-1, 1])
        
        // Make sure target face is the right size
        cv::Mat inputFace = targetFace.clone();
        if (inputFace.size() != cv::Size(512, 512)) {
            cv::resize(inputFace, inputFace, cv::Size(512, 512));
        }
        
        // Convert to float and normalize to [-1, 1]
        if (inputFace.type() != CV_32F) {
            inputFace.convertTo(inputFace, CV_32F);
        }
        inputFace = (inputFace - 127.5) / 127.5;
        
        std::cout << "DEBUG: Prepared face - size: " << inputFace.size() << ", channels: " << inputFace.channels() << std::endl;
        std::cout << "DEBUG: Face type: " << inputFace.type() << std::endl;
        
        // Prepare for blob creation - convert to uint8 if needed
        cv::Mat blobInput;
        
        if (inputFace.type() == CV_32F) {
            // Denormalize from [-1, 1] to [0, 255]
            cv::Mat temp = (inputFace + 1.0) * 127.5;
            temp.convertTo(blobInput, CV_8U);
        } else {
            blobInput = inputFace.clone();
            if (blobInput.type() != CV_8U) {
                blobInput.convertTo(blobInput, CV_8U);
            }
        }
        
        // Ensure it's 128x128 for INSwapper_128
        if (blobInput.rows != 128 || blobInput.cols != 128) {
            cv::resize(blobInput, blobInput, cv::Size(128, 128));
        }
        
        // Ensure continuous memory
        blobInput = blobInput.clone();
        
        // Create blob using standard method - uint8 input, scale to [0, 1]
        cv::Mat faceBlob = cv::dnn::blobFromImage(
            blobInput,
            1.0 / 255.0,           // scalefactor to convert [0, 255] to [0, 1]
            cv::Size(128, 128),    // target size
            cv::Scalar(0, 0, 0),
            true,                  // swapRB
            false                  // crop
        );
        
        if (faceBlob.empty()) {
            std::cerr << "Error: Failed to create blob from image" << std::endl;
            return cv::Mat();
        }
        
        // Print blob shape CORRECTLY for 4D tensor
        std::cout << "DEBUG: Face blob dims: " << faceBlob.dims << std::endl;
        for (int i = 0; i < faceBlob.dims; i++) {
            std::cout << "  dim " << i << ": " << faceBlob.size[i] << std::endl;
        }
        
        // Prepare embedding with CORRECT shape [1 x 512]
        cv::Mat embeddingInput = sourceEmbedding.clone();
        if (embeddingInput.type() != CV_32F) {
            embeddingInput.convertTo(embeddingInput, CV_32F);
        }
        
        // FIX: Reshape to [1 x 512] not [512 x 1]
        if (embeddingInput.rows != 1 || embeddingInput.cols != 512) {
            embeddingInput = embeddingInput.reshape(1, 1);
        }
        
        if (embeddingInput.cols != 512) {
            std::cerr << "Error: Embedding has wrong dimension: " << embeddingInput.cols << std::endl;
            return cv::Mat();
        }
        
        std::cout << "DEBUG: Embedding shape - " << embeddingInput.rows << " x " << embeddingInput.cols << std::endl;
        
        // INSwapper requires TWO inputs with EXACT names: "target" and "source"
        // target: [1, 3, 128, 128] - face image
        // source: [1, 512] - embedding vector
        try {
            // Set the target face image input
            inSwapperNet.setInput(faceBlob, "target");
            std::cout << "DEBUG: Face blob set to 'target' layer: " << faceBlob.size << std::endl;
            
            // Set the source embedding input
            inSwapperNet.setInput(embeddingInput, "source");
            std::cout << "DEBUG: Embedding set to 'source' layer: " << embeddingInput.size << std::endl;
            
            // Forward pass
            cv::Mat swapped = inSwapperNet.forward();
            
            std::cout << "DEBUG: Forward pass completed" << std::endl;
            
            // Print output shape
            if (!swapped.empty()) {
                std::cout << "DEBUG: Output dims: " << swapped.dims << std::endl;
                if (swapped.dims > 0) {
                    std::cout << "  Shape: ";
                    for (int i = 0; i < swapped.dims; i++) {
                        std::cout << swapped.size[i];
                        if (i < swapped.dims - 1) std::cout << " x ";
                    }
                    std::cout << std::endl;
                }
            }
            
            if (swapped.empty()) {
                std::cerr << "INSwapper model: empty output, using geometric fallback" << std::endl;
                return cv::Mat();
            }
            
            // CRITICAL: Convert INSwapper output from NCHW float [-1,1] to HWC uint8 [0,255]
            // Input: [1, 3, 128, 128] float32 in range [-1, 1]
            // Output needed: [128, 128, 3] uint8 in range [0, 255]
            
            try {
                // Remove batch dimension: [1,3,128,128] → [3,128,128]
                cv::Mat outputReshaped = swapped.reshape(1, {3, 128, 128});
                std::cout << "DEBUG: Reshaped to: 3 x 128 x 128" << std::endl;
                
                // Split channels from CHW to separate C matrices
                std::vector<cv::Mat> channels(3);
                for (int i = 0; i < 3; i++) {
                    channels[i] = cv::Mat(128, 128, CV_32F, outputReshaped.ptr<float>(i));
                }
                std::cout << "DEBUG: Channels extracted" << std::endl;
                
                // Merge channels into HWC format
                cv::Mat swappedFaceFloat;
                cv::merge(channels, swappedFaceFloat);
                std::cout << "DEBUG: Merged to HWC, shape: " << swappedFaceFloat.size() << std::endl;
                
                // Convert range [-1,1] → [0,255]
                swappedFaceFloat = (swappedFaceFloat + 1.0) * 127.5;
                std::cout << "DEBUG: Denormalized range [-1,1] to [0,255]" << std::endl;
                
                // Convert float32 → uint8
                cv::Mat swappedFaceUint8;
                swappedFaceFloat.convertTo(swappedFaceUint8, CV_8UC3);
                std::cout << "DEBUG: Converted to uint8, channels: " << swappedFaceUint8.channels() << std::endl;
                
                // Convert RGB → BGR (model outputs RGB, OpenCV expects BGR)
                cv::Mat swappedFaceBGR;
                cv::cvtColor(swappedFaceUint8, swappedFaceBGR, cv::COLOR_RGB2BGR);
                std::cout << "DEBUG: Converted RGB→BGR" << std::endl;
                
                // Now it's ready: [128, 128, 3] uint8 BGR
                std::cout << "✓ INSwapper output conversion successful!" << std::endl;
                std::cout << "  Final shape: " << swappedFaceBGR.size() << ", type: " << swappedFaceBGR.type() << std::endl;
                return swappedFaceBGR;
                
            } catch (const cv::Exception& e) {
                std::cerr << "Error in INSwapper output conversion: " << e.what() << std::endl;
                std::cerr << "Falling back to geometric transformation." << std::endl;
                return cv::Mat();
            }
        } catch (const cv::Exception& e) {
            std::cerr << "Error in INSwapper inference: " << e.what() << std::endl;
            std::cerr << "Falling back to geometric transformation." << std::endl;
            return cv::Mat();
        }
    } catch (const cv::Exception& e) {
        std::cerr << "Error in face swap model: " << e.what() << std::endl;
        std::cerr << "Falling back to geometric transformation." << std::endl;
        return cv::Mat(); // Return empty to trigger fallback
    } catch (...) {
        std::cerr << "Unknown error in face swap model. Falling back." << std::endl;
        return cv::Mat(); // Return empty to trigger fallback
    }
}

cv::Mat AdvancedFaceSwapper::restoreFace(const cv::Mat& swappedFace) {
    if (!enableGFPGAN || !gfpganLoaded || swappedFace.empty()) {
        return swappedFace.clone();
    }
    
    // GFPGAN restoration would go here
    // For now, return as-is
    return swappedFace.clone();
}

cv::Mat AdvancedFaceSwapper::generateFaceMask(const cv::Size& size, const std::vector<cv::Point2f>& landmarks) {
    cv::Mat mask = cv::Mat::zeros(size, CV_8UC1);
    
    if (landmarks.size() < 5) {
        // Fallback: elliptical mask
        cv::ellipse(mask, cv::Point(size.width/2, size.height/2),
                   cv::Size(size.width/2 * 0.9, size.height/2 * 0.9), 0, 0, 360, cv::Scalar(255), -1);
        cv::GaussianBlur(mask, mask, cv::Size(21, 21), 0);
        return mask;
    }
    
    // Create convex hull from landmarks
    std::vector<cv::Point> hullPoints;
    for (const auto& pt : landmarks) {
        int x = std::max(0, std::min(size.width - 1, static_cast<int>(pt.x)));
        int y = std::max(0, std::min(size.height - 1, static_cast<int>(pt.y)));
        hullPoints.push_back(cv::Point(x, y));
    }
    
    // Add boundary points
    int margin = std::min(size.width, size.height) / 10;
    hullPoints.push_back(cv::Point(margin, margin));
    hullPoints.push_back(cv::Point(size.width - margin, margin));
    hullPoints.push_back(cv::Point(margin, size.height - margin));
    hullPoints.push_back(cv::Point(size.width - margin, size.height - margin));
    
    // Add forehead point
    if (landmarks.size() >= 2) {
        float eyeY = (landmarks[0].y + landmarks[1].y) / 2.0f;
        float foreheadY = eyeY - (landmarks[2].y - eyeY) * 0.5f;
        hullPoints.push_back(cv::Point(size.width/2, static_cast<int>(std::max(0.0f, foreheadY))));
    }
    
    // Create convex hull and fill
    std::vector<cv::Point> hull;
    cv::convexHull(hullPoints, hull);
    cv::fillConvexPoly(mask, hull, cv::Scalar(255));
    
    // Smooth edges
    int blurSize = std::max(5, std::min(size.width, size.height) / 10);
    if (blurSize % 2 == 0) blurSize++;
    cv::GaussianBlur(mask, mask, cv::Size(blurSize, blurSize), 0);
    
    return mask;
}

cv::Mat AdvancedFaceSwapper::blendFace(const cv::Mat& swappedFace, const cv::Mat& originalFrame, 
                                       const cv::Rect& faceRect, const cv::Mat& mask) {
    cv::Mat result = originalFrame.clone();
    
    // Validate inputs
    if (swappedFace.empty() || originalFrame.empty() || mask.empty()) {
        std::cerr << "Error: Invalid input to blendFace" << std::endl;
        return result;
    }
    
    std::cout << "DEBUG blendFace: swappedFace size " << swappedFace.size() << ", rect " << faceRect << std::endl;
    
    // Check if face rect is valid
    if (faceRect.x < 0 || faceRect.y < 0 || 
        faceRect.x + faceRect.width > originalFrame.cols ||
        faceRect.y + faceRect.height > originalFrame.rows) {
        std::cerr << "Error: Face rect out of bounds: rect=" << faceRect << ", frame=" << originalFrame.size() << std::endl;
        return result;
    }
    
    // Resize swapped face to match face rect size
    cv::Mat resizedFace;
    if (swappedFace.size() == faceRect.size()) {
        resizedFace = swappedFace.clone();
    } else {
        cv::resize(swappedFace, resizedFace, faceRect.size());
    }
    
    // Resize mask to match face rect size
    cv::Mat resizedMask;
    if (mask.size() == faceRect.size()) {
        resizedMask = mask.clone();
    } else {
        cv::resize(mask, resizedMask, faceRect.size());
    }
    
    // Ensure resizedFace has 3 channels
    if (resizedFace.channels() != 3) {
        if (resizedFace.channels() == 1) {
            cv::cvtColor(resizedFace, resizedFace, cv::COLOR_GRAY2BGR);
        } else if (resizedFace.channels() == 4) {
            cv::cvtColor(resizedFace, resizedFace, cv::COLOR_BGRA2BGR);
        }
    }
    
    // Convert mask to single channel if needed
    cv::Mat mask1Channel = resizedMask;
    if (mask1Channel.channels() > 1) {
        cv::cvtColor(mask1Channel, mask1Channel, cv::COLOR_BGR2GRAY);
    }
    
    // Convert mask to 3 channels and normalize
    cv::Mat mask3Channel;
    cv::cvtColor(mask1Channel, mask3Channel, cv::COLOR_GRAY2BGR);
    mask3Channel.convertTo(mask3Channel, CV_32F, 1.0/255.0);
    
    // Extract face region
    cv::Mat faceROI = result(faceRect).clone();
    
    std::cout << "DEBUG: faceROI size " << faceROI.size() << ", resizedFace size " << resizedFace.size() 
              << ", mask3Channel size " << mask3Channel.size() << std::endl;
    
    // Blend
    cv::Mat faceFloat, swappedFloat;
    faceROI.convertTo(faceFloat, CV_32F);
    resizedFace.convertTo(swappedFloat, CV_32F);
    
    // Compute blend
    cv::Mat blended = faceFloat * 0.5f + swappedFloat * 0.5f;  // Simple 50/50 blend for now
    
    blended.convertTo(result(faceRect), CV_8U);
    
    std::cout << "DEBUG: Blending completed successfully" << std::endl;
    
    return result;
}

cv::Mat AdvancedFaceSwapper::stabilizeFace(const cv::Mat& currentFace, const std::vector<cv::Point2f>& currentLandmarks) {
    if (!useTemporalStabilization || previousFaces.empty() || currentFace.empty()) {
        return currentFace.clone();
    }
    
    // Average with previous faces
    cv::Mat stabilized = currentFace.clone();
    stabilized.convertTo(stabilized, CV_32F);
    
    float weight = 1.0f / (previousFaces.size() + 1);
    stabilized = stabilized * (1.0f - stabilizationStrength);
    
    for (const auto& prevFace : previousFaces) {
        // Skip faces with mismatched sizes
        if (prevFace.size() != currentFace.size()) {
            continue;
        }
        
        if (prevFace.empty()) {
            continue;
        }
        
        cv::Mat prevFloat;
        prevFace.convertTo(prevFloat, CV_32F);
        
        // Ensure size match before arithmetic
        if (prevFloat.size() != stabilized.size()) {
            continue;
        }
        
        try {
            stabilized = stabilized + prevFloat * (stabilizationStrength * weight);
        } catch (const cv::Exception& e) {
            std::cerr << "Warning: Skipping frame stabilization due to size mismatch: " << e.what() << std::endl;
            continue;
        }
    }
    
    stabilized.convertTo(stabilized, CV_8U);
    return stabilized;
}

void AdvancedFaceSwapper::setBlendStrength(float strength) {
    blendStrength = std::max(0.0f, std::min(1.0f, strength));
}

void AdvancedFaceSwapper::detectAndSwap(cv::Mat& frame) {
    if (frame.empty() || faceDetector.empty() || !sourceFaceLoaded) {
        return;
    }
    
    try {
        // Preprocess frame
        cv::Mat processedFrame = preprocessFrame(frame);
        
        // Set input size if it changed
        faceDetector->setInputSize(processedFrame.size());
        
        // Detect faces
        cv::Mat faces;
        faceDetector->detect(processedFrame, faces);
        
        std::cout << "DEBUG: Detected " << faces.rows << " faces" << std::endl;
        lastFaceCount = faces.rows;
    
    // Process each detected face
    for (int i = 0; i < faces.rows; i++) {
        float x = faces.at<float>(i, 0);
        float y = faces.at<float>(i, 1);
        float w = faces.at<float>(i, 2);
        float h = faces.at<float>(i, 3);
        
        cv::Rect faceRect(
            std::max(0, static_cast<int>(x)),
            std::max(0, static_cast<int>(y)),
            std::min(processedFrame.cols - static_cast<int>(x), static_cast<int>(w)),
            std::min(processedFrame.rows - static_cast<int>(y), static_cast<int>(h))
        );
        
        if (faceRect.width <= 0 || faceRect.height <= 0) continue;
        
        // Extract landmarks
        std::vector<cv::Point2f> targetLandmarks = extractLandmarks(faces, i);
        if (targetLandmarks.empty()) continue;
        
        // === FULL PIPELINE ===
        
        // 1. Align target face
        cv::Mat targetFaceAligned = alignFace(processedFrame, targetLandmarks, faceRect, 512);
        if (targetFaceAligned.empty()) continue;
        
        // 2. Extract target face embedding (if using model-based swap)
        cv::Mat targetEmbedding;
        if (arcFaceLoaded) {
            targetEmbedding = extractFaceEmbedding(targetFaceAligned);
        }
        
        // 3. Swap face using INSwapper model or fallback
        cv::Mat swappedFace;
        bool useModelSwap = false;
        
        // Only try INSwapper if we have both the model AND the source embedding
        // (embedding requires ArcFace model)
        if (inSwapperLoaded && arcFaceLoaded && !sourceFaceEmbedding.empty()) {
            swappedFace = swapFaceWithModel(targetFaceAligned, sourceFaceEmbedding);
            if (!swappedFace.empty()) {
                useModelSwap = true;
            }
        } else if (inSwapperLoaded && !arcFaceLoaded) {
            // INSwapper requires ArcFace for embeddings - use fallback
            static bool warned = false;
            if (!warned) {
                std::cerr << "Note: INSwapper model loaded but ArcFace model not found." << std::endl;
                std::cerr << "INSwapper requires ArcFace for face embeddings. Using geometric fallback." << std::endl;
                std::cerr << "For best results, download ArcFace model: ./download_models.sh" << std::endl;
                warned = true;
            }
        } else if (inSwapperLoaded && arcFaceLoaded && sourceFaceEmbedding.empty()) {
            // ArcFace loaded but couldn't extract embedding
            static bool warnedEmbedding = false;
            if (!warnedEmbedding) {
                std::cerr << "Warning: Could not extract source face embedding from ArcFace model." << std::endl;
                std::cerr << "Using geometric fallback method." << std::endl;
                warnedEmbedding = true;
            }
        }
        
        // Fallback to geometric transformation if model swap failed or not available
        if (!useModelSwap) {
            // Get source landmarks relative to source face rect
            std::vector<cv::Point2f> sourcePoints;
            if (sourceLandmarks.size() >= 3) {
                // Take first 3 landmarks
                sourcePoints.push_back(cv::Point2f(sourceLandmarks[0].x - sourceFaceRect.x, 
                                                   sourceLandmarks[0].y - sourceFaceRect.y));
                sourcePoints.push_back(cv::Point2f(sourceLandmarks[1].x - sourceFaceRect.x, 
                                                   sourceLandmarks[1].y - sourceFaceRect.y));
                sourcePoints.push_back(cv::Point2f(sourceLandmarks[2].x - sourceFaceRect.x, 
                                                   sourceLandmarks[2].y - sourceFaceRect.y));
            }
            
            // Get target landmarks relative to target face rect
            std::vector<cv::Point2f> targetPoints;
            if (targetLandmarks.size() >= 3) {
                // Take first 3 landmarks
                targetPoints.push_back(cv::Point2f(targetLandmarks[0].x - faceRect.x, 
                                                   targetLandmarks[0].y - faceRect.y));
                targetPoints.push_back(cv::Point2f(targetLandmarks[1].x - faceRect.x, 
                                                   targetLandmarks[1].y - faceRect.y));
                targetPoints.push_back(cv::Point2f(targetLandmarks[2].x - faceRect.x, 
                                                   targetLandmarks[2].y - faceRect.y));
            }
            
            // Validate points
            if (sourcePoints.size() != 3 || targetPoints.size() != 3) {
                std::cerr << "Error: Need exactly 3 landmark points for affine transform. Got source=" 
                          << sourcePoints.size() << ", target=" << targetPoints.size() << std::endl;
                continue;
            }
            
            // Check points are valid (not zero or negative)
            bool sourceValid = true, targetValid = true;
            for (const auto& pt : sourcePoints) {
                if (pt.x < 0 || pt.y < 0 || pt.x > sourceFaceRect.width || pt.y > sourceFaceRect.height) {
                    sourceValid = false;
                    break;
                }
            }
            for (const auto& pt : targetPoints) {
                if (pt.x < 0 || pt.y < 0 || pt.x > faceRect.width || pt.y > faceRect.height) {
                    targetValid = false;
                    break;
                }
            }
            
            if (!sourceValid || !targetValid) {
                std::cerr << "Error: Landmark points out of bounds. sourceValid=" << sourceValid 
                          << ", targetValid=" << targetValid << std::endl;
                continue;
            }
            
            // Validate source face region
            if (sourceFaceRect.x < 0 || sourceFaceRect.y < 0 ||
                sourceFaceRect.x + sourceFaceRect.width > sourceFaceImage.cols ||
                sourceFaceRect.y + sourceFaceRect.height > sourceFaceImage.rows) {
                std::cerr << "Error: Source face rect out of bounds" << std::endl;
                continue;
            }
            
            // Extract source face region
            cv::Mat sourceROI = sourceFaceImage(sourceFaceRect);
            if (sourceROI.empty()) {
                std::cerr << "Error: Could not extract source face ROI" << std::endl;
                continue;
            }
            
            try {
                // Calculate affine transformation
                cv::Mat transform = cv::getAffineTransform(sourcePoints, targetPoints);
                
                if (transform.empty()) {
                    std::cerr << "Error: Could not calculate affine transformation" << std::endl;
                    continue;
                }
                
                // Warp source face to match target
                cv::Mat tempSwapped;
                cv::warpAffine(sourceROI, tempSwapped, transform, faceRect.size());
                
                // Validate and ensure proper format
                if (tempSwapped.empty()) {
                    std::cerr << "Error: Warped face is empty" << std::endl;
                    continue;
                }
                
                // Ensure it's 8-bit BGR
                if (tempSwapped.type() != CV_8UC3) {
                    if (tempSwapped.channels() == 1) {
                        cv::cvtColor(tempSwapped, tempSwapped, cv::COLOR_GRAY2BGR);
                    } else if (tempSwapped.channels() == 4) {
                        cv::cvtColor(tempSwapped, tempSwapped, cv::COLOR_BGRA2BGR);
                    }
                    if (tempSwapped.type() != CV_8U) {
                        tempSwapped.convertTo(tempSwapped, CV_8U);
                    }
                }
                
                swappedFace = tempSwapped;
            } catch (const cv::Exception& e) {
                std::cerr << "Error in geometric transformation: " << e.what() << std::endl;
                continue;
            }
        }
        
        // 4. Face restoration with GFPGAN
        if (swappedFace.empty()) {
            std::cerr << "Warning: swappedFace is empty, skipping this face" << std::endl;
            continue;
        }
        
        if (enableGFPGAN && gfpganLoaded) {
            swappedFace = restoreFace(swappedFace);
        }
        
        // 5. Temporal stabilization
        if (useTemporalStabilization) {
            swappedFace = stabilizeFace(swappedFace, targetLandmarks);
            
            // Update history
            previousFaces.push_back(swappedFace.clone());
            previousLandmarks.push_back(targetLandmarks);
            if (previousFaces.size() > MAX_HISTORY) {
                previousFaces.pop_front();
                previousLandmarks.pop_front();
            }
        }
        
        // 6. Generate mask
        std::vector<cv::Point2f> targetLandmarksRelative = targetLandmarks;
        for (auto& pt : targetLandmarksRelative) {
            pt.x -= faceRect.x;
            pt.y -= faceRect.y;
        }
        cv::Mat mask = generateFaceMask(faceRect.size(), targetLandmarksRelative);
        
        if (mask.empty()) {
            std::cerr << "Warning: mask is empty, skipping blending" << std::endl;
            continue;
        }
        
        // 7. Seamless blending
        frame = blendFace(swappedFace, frame, faceRect, mask);
    }
    } catch (const cv::Exception& e) {
        std::cerr << "Exception in detectAndSwap: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Standard exception in detectAndSwap: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception in detectAndSwap" << std::endl;
    }
}

std::vector<cv::Point2f> AdvancedFaceSwapper::getFacePoints(const std::vector<cv::Point2f>& landmarks) {
    std::vector<cv::Point2f> points;
    if (landmarks.size() >= 3) {
        points.push_back(cv::Point2f(landmarks[0].x - sourceFaceRect.x, 
                                     landmarks[0].y - sourceFaceRect.y));
        points.push_back(cv::Point2f(landmarks[1].x - sourceFaceRect.x, 
                                     landmarks[1].y - sourceFaceRect.y));
        points.push_back(cv::Point2f(landmarks[2].x - sourceFaceRect.x, 
                                     landmarks[2].y - sourceFaceRect.y));
    }
    return points;
}

cv::Mat AdvancedFaceSwapper::normalizeImage(const cv::Mat& image) {
    cv::Mat normalized;
    image.convertTo(normalized, CV_32F, 1.0/127.5, -1.0);
    return normalized;
}

cv::Mat AdvancedFaceSwapper::denormalizeImage(const cv::Mat& image) {
    cv::Mat denormalized;
    image.convertTo(denormalized, CV_8U, 127.5, 127.5);
    return denormalized;
}
