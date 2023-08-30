/**
 * @local_disk.h
 * @brief Saving files locally
 *
 * This contains the prototypes for saving files locally of the type image/csv.
 *
 */

#ifndef __LOCAL_DISK_H__
#define __LOCAL_DISK_H__

#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <atomic>
#include <thread>
#include <queue>
#include <chrono>
#include <mutex>
#include <sys/stat.h>

#ifdef __APPLE__
    #include "/usr/local/include/png.h"
#else
    #include "png.h"
#endif

#include <edge-ml-accelerator/output/base_output.h>


namespace edgeml
{
    namespace output
    {

        class LocalDisk : public Output
        {
            public:
                static LocalDisk* instance(utils::jsonParser::jValue j, SharedMessage<MessageCaptureInference> &incoming_message);

                LocalDisk(utils::jsonParser::jValue j, SharedMessage<MessageCaptureInference> &incoming_message);
                ~LocalDisk();
                void saveImageAsPNGorJPG(int& errc, int& height, int& width, bool& completed);

            private:
                utils::jsonParser::jValue jsonParams_;
                int modelID = 0, ret;
                std::string savePath = getLocalPath(), savePathCapture, savePathInference, savePathInferenceImages, savePathInferenceResults;
                std::string savePathInferenceResultsJson_;
                std::string filesSavePath, filesSavePathResults;
                std::string imageFormat_ = "png", colorSpace_ = "BGR";
                unsigned char* imageData;
                unsigned char* outputVecLFVE;
                std::vector<std::vector<float> > outputVec;
                std::mutex mtx_;
                std::chrono::steady_clock::time_point start_timeout = std::chrono::steady_clock::now();
                std::chrono::steady_clock::time_point end_timeout = std::chrono::steady_clock::now();
                std::chrono::duration<double> duration_timeout;
                utils::GPIO gpio;

                utils::ImagePreProcess imagePreprocess;
                utils::ResultPostProcess resultPostprocess;
                std::string confParamStr1 = "{}", confParamStr2 = "[]", annStr = "annotations", lfveClassStr = "isAnomalous", classStr = "class", confStr = "confidence";
                std::string bboxParamStr1 = "{}", bboxParamStr2 = "[]", x1Str = "x1", y1Str = "y1", x2Str = "x2", y2Str = "y2";
                std::string TYPE = "LOCAL_DISK";
        };

    }
}

#endif
