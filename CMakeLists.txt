cmake_minimum_required(VERSION 3.10)
project(LiveFaceSwapper)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenCV REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(src)

file(GLOB SOURCES "src/*.cpp")

add_executable(LiveFaceSwapper ${SOURCES})
target_link_libraries(LiveFaceSwapper ${OpenCV_LIBS})

# Enable OpenCV DNN module (required for ONNX model support)
# Note: OpenCV must be compiled with DNN support

# Copy assets to build directory for easy access
file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})




