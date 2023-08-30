/**
 * @base_output.h
 * @brief Creating Base Output Sink Class
 *
 * This contains the prototypes of setting and getting parameters for base output.
 * This will be utilized by derived classes.
 *
 */

#ifndef __BASE_OUTPUT_H__
#define __BASE_OUTPUT_H__

#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <atomic>
#include <thread>
#include <chrono>
#include <experimental/filesystem>
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

#include <aws/greengrass/GreengrassCoreIpcClient.h>

using namespace edgeml::utils;

namespace edgeml
{
    namespace output
    {

        class IpcClientLifecycleHandler : public ConnectionLifecycleHandler
        {
            void OnConnectCallback() override
            {
                LOG_ALWAYS("[OUTPUT::PUBLISH IPC/MQTT TOPIC] OnConnectCallback");
            }
            void OnDisconnectCallback(RpcError error) override
            {
                LOG_ERROR("[OUTPUT::PUBLISH IPC/MQTT TOPIC] OnDisconnectCallback: " + std::string(error.StatusToString()));
                exit(-1);
            }
            bool OnErrorCallback(RpcError error) override
            {
                LOG_ERROR("[OUTPUT::PUBLISH IPC/MQTT TOPIC] OnErrorCallback: " + std::string(error.StatusToString()));
                return true;
            }
        };

        class Output
        {
            public:

                static Output* instance(utils::jsonParser::jValue j, SharedMessage<MessageCaptureInference> &incoming_message);

                Output(utils::jsonParser::jValue j, SharedMessage<MessageCaptureInference> &incoming_message);
                SharedMessage<MessageCaptureInference> & GetSharedMessage(){return output_message_;}
                SharedMessage<MessageCaptureInference>* GetSharedPointer(){return &output_message_;}
                void SetToProduceOutput(bool val = true){produce_output_ = val;}
                std::string GetType(){return TYPE;}
                ~Output();

                std::string getAWSRegion();
                std::string getLocalPath();
                std::string getImageFormat();
                std::string getOutputFileName();
                std::string getFileExt(std::string& s);
                void getS3BucketAndKey(std::string& bucket, std::string& key);
                std::string getS3Bucket();
                std::string getS3Key();
                std::string getIpcTopicName();
                std::string getMqttTopicName();
                std::string getColorSpace();

            private:
                utils::jsonParser::jValue jsonParams_;
                std::string s3bucket, s3key, s3region, localDisk, imageFormat, ipcTopicName, mqttTopicName;
                utils::GPIO gpio;
                std::string colorSpace_;

            protected:
                SharedMessage<MessageCaptureInference> &incoming_message_;
                SharedMessage<MessageCaptureInference> output_message_;
                bool produce_output_ = false;
                std::string TYPE = "NONE";
            private:
        };

    }
}

#endif
