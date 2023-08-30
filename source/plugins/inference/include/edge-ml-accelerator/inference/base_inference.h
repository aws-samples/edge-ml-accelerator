#ifndef __BASE_INFERENCE_H__
#define __BASE_INFERENCE_H__

#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <atomic>
#include <thread>
#include <chrono>
#include <mutex>
#include <edge-ml-accelerator/utils/json_parser.h>
#include <edge-ml-accelerator/utils/yaml_parser.h>
#include <edge-ml-accelerator/utils/image_preprocess.h>
#include <edge-ml-accelerator/utils/result_postprocess.h>
#include <edge-ml-accelerator/utils/edge_ml_config.h>
#include <edge-ml-accelerator/utils/logger.h>

#include <nlohmann/json.hpp>

#ifdef WITH_MIC730AI
#include <edge-ml-accelerator/utils/mic730ai_dio.h>
#endif

using namespace edgeml::utils;

namespace edgeml
{
    namespace inference
    {

        class Inference
        {
            public:

                static Inference* instance(utils::jsonParser::jValue j, SharedMessage<MessageCaptureInference> &);

                Inference(utils::jsonParser::jValue j, SharedMessage<MessageCaptureInference> &);
                SharedMessage<MessageCaptureInference> & GetSharedMessage(){return output_inference_;}
                SharedMessage<MessageCaptureInference>* GetSharedPointer(){return &output_inference_;}
                ~Inference();
                void SetToProduceOutput(bool val = true){produce_output_ = val;}
                nlohmann::json lfveAnomaliesNlohmannJson_, lfveResultsNlohmannJson_;
                nlohmann::json customResultsNlohmannJson_;
                nlohmann::json inferenceBaseInferenceResultsNlohmannJson;
            
            protected:
                utils::jsonParser::jValue jsonParams_;                
                std::mutex mtx_;

                std::chrono::steady_clock::time_point start_timeout = std::chrono::steady_clock::now();
                std::chrono::steady_clock::time_point end_timeout = std::chrono::steady_clock::now();
                std::chrono::duration<double> duration_timeout;
                SharedMessage<MessageCaptureInference> &camera2ongoing_;
                SharedMessage<MessageCaptureInference> output_inference_;
                bool produce_output_ = true;
                utils::GPIO gpio;
                int gpioRet, gpioValue = 0;
        };

    }
}

#endif
