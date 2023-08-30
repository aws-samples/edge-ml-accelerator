/**
 * @onnxruntime_client.h
 * @brief Creating and Running Inference Client for ONNX Runtime
 *
 * This contains the prototypes for running inference using ONNX Runtime API.
 *
 */

#ifndef __ONNXRUNTIME_CLIENT_H__
#define __ONNXRUNTIME_CLIENT_H__

#include <iostream>
#include <memory>
#include <queue>
#include <thread>
#include <chrono>
#include <mutex>
#include <string>
#include <stdint.h>
#include <dirent.h>
#include <core/session/onnxruntime_c_api.h>
#include <edge-ml-accelerator/inference/base_inference.h>


namespace edgeml
{
    namespace inference
    {

        class OnnxRuntimeClient: public Inference
        {
            public:
                static OnnxRuntimeClient* instance(utils::jsonParser::jValue j, int inferIdx, std::string p, SharedMessage<MessageCaptureInference> & shared_map);

                OnnxRuntimeClient(utils::jsonParser::jValue j, int inferIdx, std::string p, SharedMessage<MessageCaptureInference> & shared_map);
                ~OnnxRuntimeClient();

                void runInference(int& errc, int& height, int& width, int& iter, bool& completed); // Inference API

            private:
                std::string pipelineName_;
                nlohmann::json inferenceBaseInferenceResultsNlohmannJson_ = inferenceBaseInferenceResultsNlohmannJson;
                int ret, modelID = 0;
                int numModels, scaleBy;
                bool useGpio = false;
                std::vector<std::string> model_name;
                std::vector<std::string> model_path;
                std::vector<std::string> model_type;

                const OrtApi* g_ort = nullptr;
                std::vector<OrtSession*> ort_session_vec;
                std::vector<OrtMemoryInfo*> ort_memory_vec;
                OrtEnv* pOrt_env = nullptr;

                std::vector<std::string> input_tensor_names;
                std::vector<std::vector<int> > input_tensor_shape;
                std::vector<int> input_tensor_size, input_height, input_width, input_channels;
                std::vector<std::string> output_tensor_names;
                std::vector<std::vector<int> > output_tensor_shape;
                std::vector<int> output_tensor_size, output_height, output_width, output_channels;

                OrtValue* inputTensors{nullptr};
                OrtValue* outputTensors{nullptr};
                std::vector<float> inputTensorValues, outputTensorValues;
                std::vector<const char*> inputNames;
                std::vector<const char*> outputNames;
                std::vector<std::vector<const char*>> inputNamesVec;
	            std::vector<std::vector<const char*>> outputNamesVec;
                std::vector<std::vector<unsigned char> > inputDataVec;
                std::vector<float> inputScaled;
                std::vector<std::vector<float> > outputDataVec;
                std::vector<bool> is_chw_vec;

                std::mutex mtx_;
                std::shared_ptr<ThreadSafeCaptureContainer> captureContainer;
                std::chrono::steady_clock::time_point start_timeout = std::chrono::steady_clock::now();
                std::chrono::steady_clock::time_point end_timeout = std::chrono::steady_clock::now();
                std::chrono::duration<double> duration_timeout;
                std::chrono::steady_clock::time_point inference_start_time = std::chrono::steady_clock::now();
                std::chrono::steady_clock::time_point inference_end_time = std::chrono::steady_clock::now();
                std::chrono::duration<double> inference_duration;
                utils::ImagePreProcess imagePreprocess;

                void CheckStatus(OrtStatus* status);
                int initInfer(int inferIdx);
        };

    }
}

#endif
