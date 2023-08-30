/**
 * @opencv_capture.h
 * @brief Creating and Running Capture
 *
 * This contains the prototypes for creating and running capture using OpenCV for camera, video or image.
 *
 */

#ifndef __OPENCV_CAPTURE_H__
#define __OPENCV_CAPTURE_H__

#include <opencv2/opencv.hpp>

#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <atomic>
#include <queue>
#include <chrono>
#include <mutex>

#include <edge-ml-accelerator/capture/base_capture.h>

namespace edgeml
{
    namespace capture
    {

        class OpenCVCapture : public Capture
        {
            public:
                static OpenCVCapture* instance(utils::jsonParser::jValue j, int cameraIndex, SharedMessage<MessageT2C> &);

                OpenCVCapture(utils::jsonParser::jValue j, int cameraIndex, SharedMessage<MessageT2C> &);
                ~OpenCVCapture();
                int initCapture(int cameraIndex = -1) override; // setting videocapture for camera
                void getCapture(int& errc, unsigned char*& frameData, int& frameDataSize, int& iter) override; // For camera -> bytes array
                void getCaptureAtIndex(int& errc, unsigned char*& frameData, int& frameDataSize, int& iter, int cameraIndex); // For video file or camera -> bytes array
                std::string imageFile_, videoFile_;

            private:
                int runCapture(); // Running capture

                std::string colorSpace_;
                nlohmann::json inferenceDetailsBlank, inferenceDetailsFilled;
                cv::Mat frame_, frame_resized_;
                cv::VideoCapture cap_;
                std::string captureMode_;
                int captureModeInt_;
                int captureFlag_;
                int numInferences_;
                std::mutex mtx_;
                int gpioRet_, gpioRetCap_, gpioRetStrobe_, gpioValue_ = 0, gpioValueCap_ = 0, gpioValueStrobe_ = 0;
                bool capTriggerState_ = false, isResize_ = false;
                std::chrono::steady_clock::time_point start_timeout_ = std::chrono::steady_clock::now();
                std::chrono::steady_clock::time_point end_timeout_ = std::chrono::steady_clock::now();
                std::chrono::duration<double> duration_timeout_;
                std::chrono::steady_clock::time_point capture_start_time_ = std::chrono::steady_clock::now();
                std::chrono::steady_clock::time_point capture_end_time_ = std::chrono::steady_clock::now();
                std::chrono::duration<double> capture_elapsed_seconds_;
        };

    }
}
#endif
