/**
 * @mqtt_trigger.cc
 * @brief Creating and Running MQTT based capture trigger
 *
 * This contains the function definitions for creating and running MQTT based camera capture trigger.
 *
 */

#include <edge-ml-accelerator/trigger/mqtt_trigger.h>

namespace edgeml
{
  namespace trigger
  {

    MqttTrigger::MqttTrigger(utils::jsonParser::jValue j, int cameraIndex) : Trigger(j, cameraIndex)
    {
        LOG_ALWAYS("[TRIGGER::MQTT Trigger] Using MQTT trigger");

        timeoutDelay = getSoftwareTriggerDelay();
        std::vector<std::string> topic_list;
        topic_list.push_back(getMqttTriggerTopicName());
        topic_list_ = topic_list;
        for (int i=0; i<topic_list_.size(); i++)
        {
            LOG_ALWAYS("[TRIGGER::MQTT Trigger] Topic name = " + topic_list_[i]);
        }

        Io::EventLoopGroup eventLoopGroup(1);
        Io::DefaultHostResolver socketResolver(eventLoopGroup, 64, 30);
        Io::ClientBootstrap bootstrap(eventLoopGroup, socketResolver);
        IpcClientLifecycleHandler ipcLifecycleHandler;
        mqttClient = new GreengrassCoreIpcClient(bootstrap);

        auto connectionStatus = mqttClient->Connect(ipcLifecycleHandler).get();
        LOG_ALWAYS("[TRIGGER::MQTT Trigger] Connection Status: " + std::string(connectionStatus.StatusToString()));
        if (connectionStatus.StatusToString()=="EVENT_STREAM_RPC_NULL_PARAMETER")
        {
            LOG_ALWAYS("[TRIGGER::MQTT Trigger] Failed to establish MQTT connection: " + std::string(connectionStatus.StatusToString()));
        }
    }

    /**
      Creates the class destructor
    */
    MqttTrigger::~MqttTrigger()
    {
    }

    void MqttTrigger::getTrigger(int& errc, int& iter)
    {
        String topic(topic_list_[0].c_str());
        SubscribeToIoTCoreRequest request_;
        request_.SetTopicName(topic);
        request_.SetQos(qos);
        streamHandler_ = new IoTCoreResponseHandler();
        SubscribeToIoTCoreOperation operation = mqttClient->NewSubscribeToIoTCore(*streamHandler_);
        auto activate = operation.Activate(request_, nullptr);
        activate.wait();

        auto responseFuture = operation.GetResult();
        if (responseFuture.wait_for(std::chrono::milliseconds(timeoutDelay)) == std::future_status::timeout)
        {
            LOG_ERROR("[TRIGGER::MQTT Trigger] Operation timed out while waiting for response from Greengrass Core.");
        }

        auto response = responseFuture.get();
        LOG_ALWAYS("[TRIGGER::MQTT Trigger] Waiting for future response " + response);
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
                    LOG_ALWAYS("[TRIGGER::MQTT Trigger] TRIGGER ENDING");
                    break;
                }
            }

            trigger_start_time_ = std::chrono::steady_clock::now();

            nlohmann::json outMessageStringJson_;
            std::string commandStr = "";
            if (outMessageStringMqtt_ != "")
            {
                currIter++;
                LOG_ALWAYS("[TRIGGER::MQTT Trigger] Trigger #" + std::to_string(currIter) + " Successful after " + std::to_string(trigger_elapsed_seconds_.count()) + " seconds");
                MessageT2C message;
                mtx_.lock();

                trigger_end_time_ = std::chrono::steady_clock::now();
                trigger_elapsed_seconds_ = trigger_end_time_ - trigger_start_time_;

                outMessageStringJson_ = nlohmann::json::parse(outMessageStringMqtt_);

                LOG_ALWAYS("[TRIGGER::MQTT Trigger] FULL MESSAGE: " + outMessageStringMqtt_);

                commandStr = outMessageStringJson_["command"];

                // Using the message as: inference-top / inference-bottom / inference-{view}
                // Else pass the messages as is for: capture / live / configchange
                if (!inList(commandStr, {"capture","live","configchange"}) && !outMessageStringJson_["view"].empty())
                {
                    commandStr += "_" + outMessageStringJson_["view"].get<std::string>();
                }

                LOG_ALWAYS("[TRIGGER::MQTT Trigger] COMMAND: " + commandStr);

                outMessageStringJson_["utcTime"] = getUtcTime();

                message.captureTriggersMessage_ = commandStr;
                message.captureTriggersMessageFull_ = outMessageStringJson_;
                message.captureTriggersType_ = "mqtt";
                trigger2camera_.produce_message(message);

                outMessageStringMqtt_ = "";

                // Keep the main thread alive, or the process will exit.
                std::this_thread::sleep_for(std::chrono::milliseconds(jsonParams_["clockTime"].as_int()));

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
