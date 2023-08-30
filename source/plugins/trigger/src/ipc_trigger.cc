/**
 * @ipc_trigger.cc
 * @brief Creating and Running IPC based capture trigger
 *
 * This contains the function definitions for creating and running IPC based camera capture trigger.
 *
 */

#include <edge-ml-accelerator/trigger/ipc_trigger.h>

namespace edgeml
{
  namespace trigger
  {

    IpcTrigger::IpcTrigger(utils::jsonParser::jValue j, int cameraIndex) : Trigger(j, cameraIndex)
    {
        LOG_ALWAYS("[TRIGGER::IPC Trigger] Using IPC trigger");

        timeoutDelay = getSoftwareTriggerDelay();
        std::vector<std::string> topic_list;
        topic_list.push_back(getIpcTriggerTopicName());
        topic_list_ = topic_list;
        for (int i=0; i<topic_list_.size(); i++)
        {
            LOG_ALWAYS("[TRIGGER::IPC Trigger] Topic name = " + topic_list_[i]);
        }

        Io::EventLoopGroup eventLoopGroup(1);
        Io::DefaultHostResolver socketResolver(eventLoopGroup, 64, 30);
        Io::ClientBootstrap bootstrap(eventLoopGroup, socketResolver);
        IpcClientLifecycleHandler ipcLifecycleHandler;
        ipcClient = new GreengrassCoreIpcClient(bootstrap);

        auto connectionStatus = ipcClient->Connect(ipcLifecycleHandler).get();
        LOG_ALWAYS("[TRIGGER::IPC Trigger] Connection Status: " + std::string(connectionStatus.StatusToString()));
        if (connectionStatus.StatusToString()=="EVENT_STREAM_RPC_NULL_PARAMETER")
        {
            LOG_ALWAYS("[TRIGGER::IPC Trigger] Failed to establish IPC connection: " + std::string(connectionStatus.StatusToString()));
        }
    }

    /**
      Creates the class destructor
    */
    IpcTrigger::~IpcTrigger()
    {
    }

    void IpcTrigger::getTrigger(int& errc, int& iter)
    {
        String topic(topic_list_[0].c_str());
        SubscribeToTopicRequest request_;
        request_.SetTopic(topic);
        streamHandler_ = new SubscribeResponseHandler();
        SubscribeToTopicOperation operation = ipcClient->NewSubscribeToTopic(*streamHandler_);
        auto activate = operation.Activate(request_, nullptr);
        activate.wait();

        auto responseFuture = operation.GetResult();
        if (responseFuture.wait_for(std::chrono::milliseconds(timeoutDelay)) == std::future_status::timeout)
        {
            LOG_ERROR("[TRIGGER::IPC Trigger] Operation timed out while waiting for response from Greengrass Core.");
        }

        auto response = responseFuture.get();
        LOG_ALWAYS("[TRIGGER::IPC Trigger] Waiting for future response " + response);
        if (!response)
        {
            // Handle error.
            auto errorType = response.GetResultType();
            if (errorType == OPERATION_ERROR)
            {
                auto *error = response.GetOperationError();
                // Handle operation error.
            }
            else
            {
                // Handle RPC error.
            }
        }

        int currIter = 0;
        while (true)
        {
            if (iter>0)
            {
                if (currIter>=iter)
                {
                    LOG_ALWAYS("[TRIGGER::IPC Trigger] TRIGGER ENDING");
                    break;
                }
            }

            trigger_start_time_ = std::chrono::steady_clock::now();

            nlohmann::json outMessageStringJson_;
            std::string commandStr = "";
            if (outMessageStringIpc_ != "")
            {
                currIter++;
                LOG_ALWAYS("[TRIGGER::MQTT Trigger] Trigger #" + std::to_string(currIter) + " Successful after " + std::to_string(trigger_elapsed_seconds_.count()) + " seconds");
                MessageT2C message;
                mtx_.lock();

                trigger_end_time_ = std::chrono::steady_clock::now();
                trigger_elapsed_seconds_ = trigger_end_time_ - trigger_start_time_;

                outMessageStringJson_ = nlohmann::json::parse(outMessageStringIpc_);

                LOG_ALWAYS("[TRIGGER::IPC Trigger] FULL MESSAGE: " + outMessageStringIpc_);

                commandStr = outMessageStringJson_["command"];

                // Using the message as: inference-top / inference-bottom / inference-{view}
                // Else pass the messages as is for: capture / live / configchange
                if (!inList(commandStr, {"capture","live","configchange"}))
                {
                    commandStr += "_" + outMessageStringJson_["view"].dump();
                }

                LOG_ALWAYS("[TRIGGER::IPC Trigger] COMMAND: " + commandStr);

                outMessageStringJson_["utcTime"] = getUtcTime();

                message.captureTriggersMessage_ = commandStr;
                message.captureTriggersMessageFull_ = outMessageStringJson_;
                message.captureTriggersType_ = "ipc";
                trigger2camera_.produce_message(message);

                outMessageStringIpc_ = "";

                // Keep the main thread alive, or the process will exit.
                std::this_thread::sleep_for(std::chrono::milliseconds(jsonParams_["clockTime"].as_int()));

                currIter++;

                mtx_.unlock();
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(jsonParams_["clockTime"].as_int()));
            }
        }

        operation.Close();
    }

  }
}
