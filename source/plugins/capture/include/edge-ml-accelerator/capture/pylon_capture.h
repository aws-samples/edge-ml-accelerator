/**
 * @pylon_capture.h
 * @brief Creating and Running Capture
 *
 * This contains the prototypes for creating and running capture using Pylon for camera.
 *
 */

#ifndef __PYLON_CAPTURE_H__
#define __PYLON_CAPTURE_H__

#include <signal.h>

#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <atomic>
#include <queue>
#include <chrono>
#include <mutex>

#include <pylon/PylonIncludes.h>
#include <pylon/PylonImage.h>
#include <pylon/Pixel.h>
#include <pylon/ImageFormatConverter.h>
#include <pylon/BaslerUniversalInstantCamera.h>

#include <edge-ml-accelerator/capture/base_capture.h>


namespace edgeml
{
    namespace capture
    {

        class PylonCapture : public Capture
        {
            public:
                static PylonCapture* instance(utils::jsonParser::jValue j, int cameraIndex, SharedMessage<MessageT2C> &);

                PylonCapture(utils::jsonParser::jValue j, int cameraIndex, SharedMessage<MessageT2C> &);
                ~PylonCapture();
                int initCapture(int cameraIndex = -1) override; // setting camera ID
                void getCapture(int& errc, unsigned char*& frameData, int& frameDataSize, int& iter) override;
                void getCaptureAtIndex(int& errc, unsigned char*& frameData, int& frameDataSize, int& iter, int cameraIndex); // For video file or camera -> bytes array
                void setControlMode();
                void setMonitorMode();

            private:
                int runCapture(unsigned char*& frameData, int& frameDataSize);

                nlohmann::json inferenceDetailsBlank, inferenceDetailsFilled;
                Pylon::CDeviceInfo info_;
                Pylon::PylonAutoInitTerm autoInitTerm_;
                Pylon::CBaslerUniversalInstantCamera camera_;
                Pylon::CGrabResultPtr ptrGrabResult_;
                Pylon::CPylonImage image_;
                Pylon::CImageFormatConverter fc_;
                int delay_ = -1; //milliseconds
                bool controlMode_ = true;
                bool monitorMode_ = !controlMode_;
                int numInferences_;
                std::mutex mtx_;
                int gpioRet_, gpioRetCap_, gpioRetStrobe_, gpioValue_ = 0, gpioValueCap_ = 0, gpioValueStrobe_ = 0;
                bool capTriggerState_ = false;
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
