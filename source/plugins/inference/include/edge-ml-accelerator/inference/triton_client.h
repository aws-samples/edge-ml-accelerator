/**
 * @triton_client.h
 * @brief Creating and Running Inference Client
 *
 * This contains the prototypes for running inference using Triton Runtime API.
 *
 *
 */

#ifndef __TRITON_CLIENT_H__
#define __TRITON_CLIENT_H__

#include <iostream>
#include <memory>
#include <queue>
#include <thread>
#include <chrono>
#include <mutex>
#include <string>
#include <stdint.h>
#include <dirent.h>
#include <edge-ml-accelerator/inference/base_inference.h>

#include <grpc_client.h>

namespace tc = triton::client;

namespace edgeml
{
    namespace inference
    {

        class TritonClient: public Inference
        {
            public:
                static TritonClient* instance(utils::jsonParser::jValue j, int inferIdx, std::string p, SharedMessage<MessageCaptureInference> & shared_map);

                TritonClient(utils::jsonParser::jValue j, int inferIdx, std::string p, SharedMessage<MessageCaptureInference> & shared_map);

                void runInference(int& errc, int& height, int& width, int& iter, bool& completed);
                ~TritonClient();

                // void runInference(int& errc, int& height, int& width, int& iter, bool& completed); // Inference API

            private:

                std::string pipelineName_;
                nlohmann::json inferenceBaseInferenceResultsNlohmannJson_ = inferenceBaseInferenceResultsNlohmannJson;
                int ret, modelID = 0;
                int numModels, scaleBy;
                bool useGpio = false;

                std::mutex mtx_;
                std::chrono::steady_clock::time_point start_timeout = std::chrono::steady_clock::now();
                std::chrono::steady_clock::time_point end_timeout = std::chrono::steady_clock::now();
                std::chrono::duration<double> duration_timeout;
                std::chrono::steady_clock::time_point inference_start_time = std::chrono::steady_clock::now();
                std::chrono::steady_clock::time_point inference_end_time = std::chrono::steady_clock::now();
                std::chrono::duration<double> inference_duration;

                utils::GPIO gpio;
                int gpioRet, gpioValue = 0;

                int initInfer(std::string model_name);
                std::unique_ptr<tc::InferenceServerGrpcClient> grpc_client_;

                //triton things
                tc::InferInput* input_;
                tc::InferInput* input_path_;
                tc::InferRequestedOutput* output_;
                std::string model_name_, metadata_;
                // tc::InferOptions options_;
                int batch_size_;
        };

    }
}

#endif
