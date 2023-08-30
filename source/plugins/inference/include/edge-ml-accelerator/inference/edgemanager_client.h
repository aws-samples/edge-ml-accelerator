/**
 * @edgemanager_client.h
 * @brief Creating and Running Inference Client for Edge Manager
 *
 * This contains the prototypes for running inference using SageMaker EdgeManager API.
 *
 */

#ifndef __EDGEMANAGER_CLIENT_H__
#define __EDGEMANAGER_CLIENT_H__

#include <iostream>
#include <memory>
#include <queue>
#include <thread>
#include <chrono>
#include <mutex>
#include <string>
#include <stdint.h>
#include <dirent.h>
#include <grpcpp/grpcpp.h>
#include <edge-ml-accelerator/inference/base_inference.h>

#include "edgemanager-agent.grpc.pb.h"


namespace edgeml
{
    namespace inference
    {

        class EdgeManagerClient: public Inference
        {
            public:
                static EdgeManagerClient* instance(utils::jsonParser::jValue j, int inferIdx, std::string p, SharedMessage<MessageCaptureInference> & shared_map);

                EdgeManagerClient(utils::jsonParser::jValue j, int inferIdx, std::string p, SharedMessage<MessageCaptureInference> & shared_map);
                ~EdgeManagerClient();

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

                grpc::ChannelArguments channel_arguments;
                std::shared_ptr< grpc::Channel> channel;
                std::vector<std::unique_ptr< AWS::SageMaker::Edge::Agent::Stub>> stubs; // vector of inference stubs for multiple models
                std::vector<std::string> input_tensor_names;
                std::vector<AWS::SageMaker::Edge::DataType> input_tensor_data_type;
                std::vector<std::vector<int> > input_tensor_shape;
                std::vector<int> input_tensor_size, input_height, input_width, input_channels;
                std::vector<std::string> output_tensor_names;
                std::vector<AWS::SageMaker::Edge::DataType> output_tensor_data_type;
                std::vector<std::vector<int> > output_tensor_shape;
                std::vector<int> output_tensor_size, output_height, output_width, output_channels;

                AWS::SageMaker::Edge::ListModelsRequest lmRequest;
                AWS::SageMaker::Edge::ListModelsResponse lmResponse;
                AWS::SageMaker::Edge::DescribeModelRequest dmRequest;
                AWS::SageMaker::Edge::DescribeModelResponse dmResponse;
                AWS::SageMaker::Edge::PredictRequest predRequest;
                AWS::SageMaker::Edge::PredictResponse predResponse;
                AWS::SageMaker::Edge::CaptureDataRequest cdRequest;
                std::vector<AWS::SageMaker::Edge::PredictRequest> predRequestVec;
                std::vector<AWS::SageMaker::Edge::PredictResponse> predResponseVec;
                std::vector<AWS::SageMaker::Edge::CaptureDataRequest> cdRequestVec;

                AWS::SageMaker::Edge::Model model;
                AWS::SageMaker::Edge::Tensor tensor; // tensor creation
                AWS::SageMaker::Edge::TensorMetadata tensormetadata; // tensor meta data creation
                std::vector<AWS::SageMaker::Edge::Tensor> tensorVec; // tensor vector creation
                std::vector<AWS::SageMaker::Edge::TensorMetadata> tensormetadataVec; // tensor meta data vector creation
                AWS::SageMaker::Edge::Tensor* outputTensor; // output tensor creation
                std::vector<AWS::SageMaker::Edge::Tensor*> outputTensorVec; // output tensor vector creation

                std::vector<std::vector<unsigned char> > inputDataVec;
                std::vector<float> inputScaled;
                std::vector<std::vector<float> > outputDataVec;

                grpc::ClientContext context; // single context will be used for multiple models

                std::mutex mtx_;
                std::shared_ptr<ThreadSafeCaptureContainer> captureContainer;
                std::chrono::steady_clock::time_point start_timeout = std::chrono::steady_clock::now();
                std::chrono::steady_clock::time_point end_timeout = std::chrono::steady_clock::now();
                std::chrono::duration<double> duration_timeout;
                std::chrono::steady_clock::time_point inference_start_time = std::chrono::steady_clock::now();
                std::chrono::steady_clock::time_point inference_end_time = std::chrono::steady_clock::now();
                std::chrono::duration<double> inference_duration;
                utils::ImagePreProcess imagePreprocess;

                int initInfer(int inferIdx);
                bool findModel(int id, const std::string& model_name);
                int LoadModel(int id, const std::string model_path, const std::string model_name);
                int UnLoadModel(int id, const std::string model_name);
        };

    }
}

#endif
