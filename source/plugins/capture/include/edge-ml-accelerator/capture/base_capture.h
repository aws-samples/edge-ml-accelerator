/**
 * @base_capture.h
 * @brief Creating Base Capture Class
 *
 * This contains the prototypes of setting and getting parameters for base capture.
 * This will be utilized by derived classes.
 *
 */

#ifndef __BASE_CAPTURE_H__
#define __BASE_CAPTURE_H__

#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <atomic>
#include <thread>
#include <chrono>
#include <edge-ml-accelerator/utils/json_parser.h>
#include <edge-ml-accelerator/utils/yaml_parser.h>
#include <edge-ml-accelerator/utils/logger.h>

#ifdef WITH_MIC730AI
#include <edge-ml-accelerator/utils/mic730ai_dio.h>
#endif

#include <edge-ml-accelerator/utils/edge_ml_config.h>

#include <nlohmann/json.hpp>

using namespace edgeml::utils;

namespace edgeml
{
    namespace capture
    {

        class Capture
        {
            public:
                static Capture* instance(edgeml::utils::jsonParser::jValue j, int cameraIndex, SharedMessage<MessageT2C> &);
                Capture(edgeml::utils::jsonParser::jValue j, int cameraIndex, SharedMessage<MessageT2C> &);
                ~Capture();

                std::string getCamID();
                void setCamID(std::string serialNumber);
                int getInputHeight();
                int getInputWidth();
                int getResizeInputHeight();
                int getResizeInputWidth();
                std::string getColorSpace();
                long int getHardwareTriggerDelay();
                long int getSoftwareTriggerDelay();
                bool getIsGpioCaptureTrigger();
                long int getGpioCaptureTriggerDelay();
                int getCameraOperatingMode();
                void setInputHeight(int in_height);
                void setInputWidth(int in_width);
                std::string getCameraName();
                int getNumInferences();
                std::string getCaptureMode();
                double getExposureTime();
                int getGainValue();
                nlohmann::json getInferenceDetailsJson();
                void setGenericTrigger(bool value){genericTrigger_ = value;}
                virtual int initCapture(int cameraIndex = -1){return -1;}
                virtual void getCapture(int& errc, unsigned char*& frameData, int& frameDataSize, int& iter) {};
                virtual void getCaptureAtIndex(int& errc, unsigned char*& frameData, int& frameDataSize, int& iter) {};
                virtual void getCaptureImageLoop(int& errc, unsigned char*& frameData, int& frameDataSize, int& iter, std::string inImageFile, int cameraIndex = 0) {};
                virtual void getCaptureImageLoopAtIndex(int& errc, unsigned char*& frameData, int& frameDataSize, int& iter, int cameraIndex) {};

            protected:
                utils::jsonParser::jValue jsonParams_;
                std::string cameraName_;
                int cameraIndex_;
                int height_, width_, resizeHeight_, resizeWidth_;
                double exposureTime_;
                int gainValue_;
                int controlOrMonitor_;
                std::string captureType_, serialNumber_, captureMode_;
                std::string colorSpace_;
                int numInferences_;
                bool genericTrigger_ = false;
                nlohmann::json inferenceDetailsJson;
                SharedMessage<MessageT2C> &trigger2camera_;
                utils::GPIO gpio_;

            public:
                SharedMap<MessageCaptureInference> camera2forward_;
        };

    }
}
#endif