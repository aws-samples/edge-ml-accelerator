/**
 * @publish_mqtt_topic.h
 * @brief Publishing topic for greengrass
 *
 * This contains the prototypes for creating and running greengrass publish topics.
 *
 */

#ifndef __PUBLISH_MQTT_TOPIC_H__
#define __PUBLISH_MQTT_TOPIC_H__

#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <atomic>
#include <thread>
#include <chrono>
#include <mutex>
#include <sys/stat.h>
#include <dirent.h>

#include <aws/iot/MqttClient.h>
#include <aws/crt/Api.h>
#include <aws/crt/io/HostResolver.h>
#include <aws/greengrass/GreengrassCoreIpcClient.h>

#include <edge-ml-accelerator/output/base_output.h>

using namespace Aws::Crt;
using namespace Aws::Greengrass;
using namespace edgeml::utils;

namespace edgeml
{
    namespace output
    {

        Aws::Crt::ApiHandle apiHandleMqtt(g_allocator);

        class PublishToMqtt : public Output
        {
            public:
                static PublishToMqtt* instance(jsonParser::jValue j, SharedMessage<MessageCaptureInference> &incoming_message);

                PublishToMqtt(jsonParser::jValue j, SharedMessage<MessageCaptureInference> &incoming_message);
                ~PublishToMqtt();
                void publishToTopic(bool& completed);

            private:
                GreengrassCoreIpcClient* mqttClient;
                QOS qos = QOS_AT_MOST_ONCE;
                jsonParser::jValue jsonParams_;
                std::string region, local_path, mqttTopicName;
                nlohmann::json mqttJsonFormat, mqttResponseJson, mqttResponseInferenceJson;
                std::string mqttJsonFormatStr;
                std::string image_file_names, result_file_names, inference_details;
                std::mutex mtx_;
                int timeout = 10;
                std::chrono::steady_clock::time_point start_timeout = std::chrono::steady_clock::now();
                std::chrono::steady_clock::time_point end_timeout = std::chrono::steady_clock::now();
                std::chrono::duration<double> duration_timeout;
                std::chrono::steady_clock::time_point start_timeout_gpio = std::chrono::steady_clock::now();
                std::chrono::steady_clock::time_point end_timeout_gpio = std::chrono::steady_clock::now();
                std::chrono::duration<double> duration_timeout_gpio;
                GPIO gpio;
                int gpioRet, gpioValue, gpioValueRes;
                std::string TYPE = "PUBLISH_MQTT_TOPIC";
        };

    }
}

#endif