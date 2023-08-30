/**
 * @lfve_client.h
 * @brief Creating and Running Inference Client
 *
 * This contains the prototypes for running inference using Lookout For Vision for Edge API.
 *
 */

#ifndef __LFVE_CLIENT_H__
#define __LFVE_CLIENT_H__

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

#include "edge-agent.grpc.pb.h"

namespace edgeml
{
    namespace inference
    {

        class LFVEclient: public Inference
        {
            public:
                static LFVEclient* instance(utils::jsonParser::jValue j, int inferIdx, std::string p, double anomaly_threhold, SharedMessage<MessageCaptureInference> & shared_map);

                LFVEclient(utils::jsonParser::jValue j, int inferIdx, std::string p, double anomaly_threshold, SharedMessage<MessageCaptureInference> & shared_map);
                ~LFVEclient();

                void runInference(int& errc, int& height, int& width, int& iter, bool& completed); // Inference API

            private:
                std::string pipelineName_;
                nlohmann::json inferenceBaseInferenceResultsNlohmannJson_ = inferenceBaseInferenceResultsNlohmannJson;
                int modelID = 0;
                int ret;
                int numModels;
                bool useGpio = false;
                int inferIdx_;
                bool is_anomaly = true; float confidence = 1.0;
                double anomaly_threshold_ = 1.0;
                std::vector<std::string> model_name;
                std::vector<int> model_input_height;
                std::vector<int> model_input_width;
                std::vector<bool> is_anomaly_overall;
                std::vector<float> confidence_overall;
                std::vector<float> threshold;

                std::shared_ptr< grpc::Channel> channel;
                std::vector<std::unique_ptr< AWS::LookoutVision::EdgeAgent::Stub>> stubs; // vector of inference stubs for multiple models
                AWS::LookoutVision::DescribeModelRequest dmRequest;
                AWS::LookoutVision::DescribeModelResponse dmResponse;
                AWS::LookoutVision::ModelDescription modelDescription;
                AWS::LookoutVision::DetectAnomaliesRequest request; // single request will be used for multiple models
                AWS::LookoutVision::DetectAnomaliesResponse response; // single response will be used for multiple models
                AWS::LookoutVision::DetectAnomalyResult reply; // single reply will be used for multiple models
                grpc::ClientContext context; // single context will be used for multiple models
                AWS::LookoutVision::Bitmap bitmap; // bitmap image creation
                AWS::LookoutVision::Bitmap anomaly_mask; // output bitmap mask
                AWS::LookoutVision::Anomaly anomalies; // output anomalies
                std::vector<AWS::LookoutVision::Anomaly> anomaliesVec;
                AWS::LookoutVision::Bitmap* lfveBMret;

                std::mutex mtx_;
                std::chrono::steady_clock::time_point start_timeout = std::chrono::steady_clock::now();
                std::chrono::steady_clock::time_point end_timeout = std::chrono::steady_clock::now();
                std::chrono::duration<double> duration_timeout;
                std::chrono::steady_clock::time_point inference_start_time = std::chrono::steady_clock::now();
                std::chrono::steady_clock::time_point inference_end_time = std::chrono::steady_clock::now();
                std::chrono::duration<double> inference_duration;

                utils::ImagePreProcess imagePreprocess;

                int initInfer(int inferIdx);
        };

    }
}

#endif
