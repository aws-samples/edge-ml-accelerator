/**
 * @genicam_capture.h
 * @brief Creating and Running Capture
 *
 * This contains the prototypes for creating and running capture using GenICam for camera.
 *
 */

#ifndef __GENICAM_CAPTURE_H__
#define __GENICAM_CAPTURE_H__

#include <rc_genicam_api/system.h>
#include <rc_genicam_api/interface.h>
#include <rc_genicam_api/device.h>
#include <rc_genicam_api/stream.h>
#include <rc_genicam_api/buffer.h>
#include <rc_genicam_api/image.h>
#include <rc_genicam_api/image_store.h>
#include <rc_genicam_api/config.h>
#include <rc_genicam_api/exception.h>

#include <rc_genicam_api/pixel_formats.h>

#include <Base/GCException.h>

#include <signal.h>

#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <atomic>
#include <queue>
#include <thread>
#include <chrono>
#include <mutex>

#include <edge-ml-accelerator/capture/base_capture.h>


namespace edgeml
{
    namespace capture
    {

        class GenicamCapture : public Capture
        {
            public:
                static GenicamCapture* instance(utils::jsonParser::jValue j, int cameraIndex, SharedMessage<MessageT2C> &);

                GenicamCapture(utils::jsonParser::jValue j, int cameraIndex, SharedMessage<MessageT2C> &);
                ~GenicamCapture();
                int initCapture(int cameraIndex = -1) override; // setting camera ID
                void getCapture(int& errc, unsigned char*& frameData, int& frameDataSize, int& iter) override; // For camera -> bytes array
                void getCaptureAtIndex(int& errc, unsigned char*& frameData, int& frameDataSize, int& iter, int cameraIndex); // For video file or camera -> bytes array

            private:
                int runCapture(unsigned char*& frameData, int& frameDataSize); // Running capture

                std::string colorSpace_;
                nlohmann::json inferenceDetailsBlank, inferenceDetailsFilled;
                std::shared_ptr<rcg::Device> dev_ = rcg::getDevice("0");
                std::shared_ptr<GenApi::CNodeMapRef> nodemap_;
                int captureFlag_;
                std::vector<std::shared_ptr<rcg::Stream>> stream_;
                int buffers_received_ = 0, buffers_incomplete_ = 0;
                const rcg::Buffer *buffer_;
                uint32_t npart_;
                uint64_t format_;
                size_t yoffset_, px;
                bool firstFrame_ = true;
                int ret_;
                int delay_ = -1; //milliseconds
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
