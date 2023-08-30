/**
 * @base_trigger.h
 * @brief Creating Base Trigger Class
 *
 * This contains the prototypes of setting and getting parameters for base trigger.
 * This will be utilized by derived classes.
 *
 */

#ifndef __BASE_TRIGGER_H__
#define __BASE_TRIGGER_H__

#include <iostream>
#include <fstream>
#include <iomanip>
#include <ctime>
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

#ifdef WITH_MIC730AI
#include <edge-ml-accelerator/utils/mic730ai_dio.h>
#endif

#include <aws/greengrass/GreengrassCoreIpcClient.h>

using namespace edgeml::utils;

namespace edgeml
{
    namespace trigger
    {

        class IpcClientLifecycleHandler : public ConnectionLifecycleHandler
        {
            void OnConnectCallback() override
            {
                LOG_ALWAYS("[TRIGGER::IPC/MQTT Trigger] OnConnectCallback");
            }
            void OnDisconnectCallback(RpcError error) override
            {
                LOG_ERROR("[TRIGGER::IPC/MQTT Trigger] OnDisconnectCallback");
                exit(-1);
            }
            bool OnErrorCallback(RpcError error) override
            {
                LOG_ERROR("[TRIGGER::IPC/MQTT Trigger] OnErrorCallback");
                return true;
            }
        };

        class Trigger
        {
            public:

                static Trigger* instance(edgeml::utils::jsonParser::jValue j, int cameraIndex);

                Trigger(edgeml::utils::jsonParser::jValue j, int cameraIndex);
                ~Trigger();
                long int getSoftwareTriggerDelay();
                long int getHardwareTriggerDelay();
                bool getIsGpioTrigger();
                long int getGpioTriggerDelay();
                bool getIsIpcTrigger();
                std::string getIpcTriggerTopicName();
                bool getIsMqttTrigger();
                std::string getMqttTriggerTopicName();
                virtual void getTrigger(int& errc, int& iter) {};
                std::string getUtcTime();
                SharedMessage<MessageT2C> trigger2camera_;

            protected:
                utils::jsonParser::jValue jsonParams_;
                std::mutex mtx_;
                long int hwTriggerDelay_, swTriggerDelay_, gpioTriggerDelay_;
                bool useGpioTrigger_, useIpcTrigger_, useMqttTrigger_;
                std::string ipcTriggerTopicName, mqttTriggerTopicName;
                utils::GPIO gpio_;
        };

    }
}
#endif