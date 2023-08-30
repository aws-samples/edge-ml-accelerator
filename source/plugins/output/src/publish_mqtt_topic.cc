/**
 * @publish_mqtt_topic.cc
 * @brief Uploading data as topics
 *
 * This contains the function definitions for creating and running AWS greengrass publish topic APIs.
 *
 */

#include <edge-ml-accelerator/output/publish_mqtt_topic.h>
#include <unistd.h>

namespace edgeml
{
  namespace output
  {

    /**
      Instance of the class
    */
    PublishToMqtt* PublishToMqtt::instance(jsonParser::jValue j, SharedMessage<MessageCaptureInference> &incoming_message)
    {
      static PublishToMqtt* inst = 0;

      if (!inst)
      {
        inst = new PublishToMqtt(j, incoming_message);
      }
      return inst;
    }

    /**
      Creates the class constructor
    */
    PublishToMqtt::PublishToMqtt(jsonParser::jValue j, SharedMessage<MessageCaptureInference> &incoming_message) : Output(j, incoming_message), jsonParams_(j)
    {
      local_path = getLocalPath();
      mqttTopicName = getMqttTopicName();

      LOG_ALWAYS("[OUTPUT::PUBLISH MQTT TOPIC] Generating GG Topic");

      LOG_ALWAYS("[OUTPUT::PUBLISH MQTT TOPIC] Topic name = " + mqttTopicName);

      Io::EventLoopGroup eventLoopGroup(1);
      Io::DefaultHostResolver socketResolver(eventLoopGroup, 64, 30);
      Io::ClientBootstrap bootstrap(eventLoopGroup, socketResolver);
      IpcClientLifecycleHandler ipcLifecycleHandler;
      mqttClient = new GreengrassCoreIpcClient(bootstrap);

      auto connectionStatus = mqttClient->Connect(ipcLifecycleHandler).get();
      LOG_ALWAYS("[OUTPUT::PUBLISH MQTT TOPIC] Connection Status: " + std::string(connectionStatus.StatusToString()));
      if (connectionStatus.StatusToString()=="EVENT_STREAM_RPC_NULL_PARAMETER")
      {
        LOG_ALWAYS("[OUTPUT::PUBLISH MQTT TOPIC] Failed to establish MQTT connection: " + std::string(connectionStatus.StatusToString()));
      }
    }

    /**
      Creates the class destructor
    */
    PublishToMqtt::~PublishToMqtt()
    {
    }

    /**
      Routine for publishing json to topic subscribed to
      @param completed getting the parameter to know if the other pipeline stages are completed
    */
    void PublishToMqtt::publishToTopic(bool& completed)
    {
      while(1)
      {
        bool data_available = false;
        auto incoming_message = incoming_message_.GetMessage();
        LOG_ALWAYS("[OUTPUT::PUBLISH MQTT TOPIC] Incoming Message = " + incoming_message.captureTrigger_.captureTriggersMessage_);

        LOG_ALWAYS("[OUTPUT::PUBLISH MQTT TOPIC] Publishing to topic: " + mqttTopicName);

        image_file_names = incoming_message.image_file_names_;
        result_file_names = incoming_message.result_file_names_;

        mqttJsonFormat = incoming_message.captureTrigger_.captureTriggersMessageFull_;
        mqttJsonFormat["status"] = "Success";
        mqttJsonFormat["statusDescription"] = "";
        mqttJsonFormat["response"] = incoming_message.inferenceDetailsMap_["response"];

        if (!inList(incoming_message.captureTrigger_.captureTriggersMessage_,{"live", "capture","capture_"}))
        {
          mqttResponseInferenceJson = mqttJsonFormat["response"]["inferenceResults"];
          if(incoming_message.inferenceMode_ != InferenceInputModeE::TRITON)
            mqttResponseInferenceJson["maskImage"] = result_file_names;
        }

        mqttResponseJson = mqttJsonFormat["response"];
        mqttResponseJson["image"] = image_file_names;
        if (!inList(incoming_message.captureTrigger_.captureTriggersMessage_,{"live", "capture","capture_"}))
        {
          mqttResponseJson["inferenceResults"] = mqttResponseInferenceJson;
        }

        mqttJsonFormat["response"] = mqttResponseJson;

        if (mqttJsonFormat.dump().length()>131072)
        {
          mqttJsonFormat["response"]["inferenceResults"]["results"] = "MESSAGE TOO LONG";
        }

        mqttJsonFormatStr = mqttJsonFormat.dump();

        LOG_ALWAYS("[OUTPUT::PUBLISH MQTT TOPIC] mqttJsonFormatStr = ");
        LOG_ALWAYS(std::string(mqttJsonFormatStr));

        String topic(mqttTopicName.c_str());
        String message(mqttJsonFormatStr.c_str());

        PublishToIoTCoreRequest request_;
        Vector<uint8_t> messageData({message.begin(), message.end()});
        request_.SetTopicName(topic);
        request_.SetQos(qos);
        request_.SetPayload(messageData);

        PublishToIoTCoreOperation operation = mqttClient->NewPublishToIoTCore();

        auto requestStatus = operation.Activate(request_).get();
        if (!requestStatus)
        {
            LOG_ALWAYS("[OUTPUT::PUBLISH MQTT TOPIC] requestStatus = " + std::string(requestStatus.StatusToString()));
        }

        auto responseFuture = operation.GetResult();
        if (responseFuture.wait_for(std::chrono::seconds(timeout)) == std::future_status::timeout)
        {
            LOG_ALWAYS("[OUTPUT::PUBLISH MQTT TOPIC] Operation timed out while waiting for response from Greengrass Core.");
        }

        auto publishResult = responseFuture.get();
        if (publishResult)
        {
            LOG_ALWAYS("[OUTPUT::PUBLISH MQTT TOPIC] Successfully published to topic: " + std::string(topic));
            auto *response = publishResult.GetOperationResponse();
            (void)response;
        }
        else
        {
            auto errorType = publishResult.GetResultType();
            if (errorType == OPERATION_ERROR)
            {
                OperationError *error = publishResult.GetOperationError();
                if (error->GetMessage().has_value())
                    LOG_ALWAYS("[OUTPUT::PUBLISH MQTT TOPIC] Greengrass Core responded with an error: " + std::string(error->GetMessage().value()));
            }
            else
            {
                LOG_ALWAYS("[OUTPUT::PUBLISH MQTT TOPIC] Attempting to receive the response from the server failed with error code: " + std::string(publishResult.GetRpcError().StatusToString()));
            }
        }

        start_timeout = std::chrono::steady_clock::now();
        completed = false;

        if(produce_output_)
        {
          output_message_.produce_message(incoming_message);
          LOG_ALWAYS("[PIPELINE::GENERAL] Message Size = " + std::to_string(output_message_.size()));
        }
      }
    }

  }
}
