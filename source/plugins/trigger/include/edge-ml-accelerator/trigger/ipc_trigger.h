/**
 * @ipc_trigger.h
 * @brief Creating and Running IPC based trigger
 *
 * This contains the function definitions for creating and running IPC based capture trigger.
 *
 */

#ifndef __IPC_TRIGGER_H__
#define __IPC_TRIGGER_H__

#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <atomic>
#include <queue>
#include <chrono>
#include <mutex>
#include <set>

#include <aws/crt/Api.h>
#include <aws/crt/io/HostResolver.h>
#include <aws/greengrass/GreengrassCoreIpcClient.h>

#include <edge-ml-accelerator/trigger/base_trigger.h>

using namespace Aws::Crt;
using namespace Aws::Greengrass;
using namespace edgeml::utils;

namespace edgeml
{
    namespace trigger
    {

        Aws::Crt::ApiHandle apiHandleIpc(g_allocator);
        std::string outMessageStringIpc_ = "";

        class SubscribeResponseHandler : public SubscribeToTopicStreamHandler
        {
            virtual ~SubscribeResponseHandler() {}

            public:
                void OnStreamEvent(SubscriptionResponseMessage *response) override
                {
                    auto jsonMessage = response->GetJsonMessage();
                    auto binaryMessage = response->GetBinaryMessage();
                    if (jsonMessage.has_value() && jsonMessage.value().GetMessage().has_value())
                    {
                        // Handle JSON message.
                        outMessageStringIpc_ = std::string(jsonMessage.value().GetMessage().value().View().WriteReadable().c_str(), jsonMessage.value().GetMessage().value().View().WriteReadable().size());
                    }
                    else if (binaryMessage.has_value() && binaryMessage.value().GetMessage().has_value())
                    {
                        // Handle Binary message.
                        auto messageBytes = binaryMessage.value().GetMessage().value();
                        outMessageStringIpc_ = std::string(messageBytes.begin(), messageBytes.end());
                    }
                }

                bool OnStreamError(OperationError *error) override
                {
                    LOG_ERROR("[TRIGGER::IPC Trigger] OnStreamError");
                    return true;
                }

                void OnStreamClosed() override
                {
                    LOG_ALWAYS("[TRIGGER::IPC Trigger] OnStreamClosed");
                }

            private:
                std::mutex mtx_;

        };

        class IpcTrigger : public Trigger
        {
            public:
                SubscribeResponseHandler* streamHandler_;
                std::vector<std::string> topic_list_;

                IpcTrigger(utils::jsonParser::jValue j, int cameraIndex);
                void getTrigger(int& errc, int& iter);
                ~IpcTrigger();

            private:
                GreengrassCoreIpcClient* ipcClient;
                std::mutex mtx_;
                long int timeoutDelay;
                std::set<std::string> captureStrs {"capture", "CAPTURE", "{capture}", "{CAPTURE}"};
                std::set<std::string> inferStrs {"infer", "INFER", "inference", "INFERENCE", "{infer}", "{INFER}", "{inference}", "{INFERENCE}"};
                std::chrono::steady_clock::time_point start_timeout_ = std::chrono::steady_clock::now();
                std::chrono::steady_clock::time_point end_timeout_ = std::chrono::steady_clock::now();
                std::chrono::duration<double> duration_timeout_;
                std::chrono::steady_clock::time_point trigger_start_time_ = std::chrono::steady_clock::now();
                std::chrono::steady_clock::time_point trigger_end_time_ = std::chrono::steady_clock::now();
                std::chrono::duration<double> trigger_elapsed_seconds_;
        };

    }
}

#endif