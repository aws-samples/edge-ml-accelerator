/**
 * @publish_ipc_topic.cc
 * @brief Uploading data as topics
 *
 * This contains the function definitions for creating and running AWS greengrass publish topic APIs.
 *
 */

#include <edge-ml-accelerator/output/publish_ipc_topic.h>
#include <unistd.h>

namespace edgeml
{
  namespace output
  {

    /**
      Instance of the class
    */
    PublishToIpc* PublishToIpc::instance(jsonParser::jValue j, SharedMessage<MessageCaptureInference> &incoming_message)
    {
      static PublishToIpc* inst = 0;

      if (!inst)
      {
        inst = new PublishToIpc(j, incoming_message);
      }
      return inst;
    }

    /**
      Creates the class constructor
    */
    PublishToIpc::PublishToIpc(jsonParser::jValue j, SharedMessage<MessageCaptureInference> &incoming_message) : Output(j, incoming_message), jsonParams_(j)
    {
      local_path = getLocalPath();
      ipcTopicName = getIpcTopicName();

      // LOG_ALWAYS("[OUTPUT::PUBLISH IPC TOPIC] Generating GG Topic");

      // LOG_ALWAYS("[OUTPUT::PUBLISH IPC TOPIC] Topic name = " + ipcTopicName);

      Io::EventLoopGroup eventLoopGroup(1);
      Io::DefaultHostResolver socketResolver(eventLoopGroup, 64, 30);
      Io::ClientBootstrap bootstrap(eventLoopGroup, socketResolver);
      IpcClientLifecycleHandler ipcLifecycleHandler;
      ipcClient = new GreengrassCoreIpcClient(bootstrap);

      auto connectionStatus = ipcClient->Connect(ipcLifecycleHandler).get();
      LOG_ALWAYS("[OUTPUT::PUBLISH IPC TOPIC] Connection Status: " + std::string(connectionStatus.StatusToString()));
      if (connectionStatus.StatusToString()=="EVENT_STREAM_RPC_NULL_PARAMETER")
      {
        LOG_ALWAYS("[OUTPUT::PUBLISH IPC TOPIC] Failed to establish IPC connection: " + std::string(connectionStatus.StatusToString()));
      }
    }

    /**
      Creates the class destructor
    */
    PublishToIpc::~PublishToIpc()
    {
    }

    /**
      Routine for publishing json to topic subscribed to
      @param completed getting the parameter to know if the other pipeline stages are completed
    */
    void PublishToIpc::publishToTopic(bool& completed)
    {
      while(1)
      {
        bool data_available = false;
        auto incoming_message = incoming_message_.GetMessage();
        LOG_ALWAYS("[OUTPUT::PUBLISH IPC TOPIC] Incoming Message = " + incoming_message.captureTrigger_.captureTriggersMessage_);

        LOG_ALWAYS("[OUTPUT::PUBLISH IPC TOPIC] Publishing to topic: " + ipcTopicName);

        image_file_names = incoming_message.image_file_names_;
        result_file_names = incoming_message.result_file_names_;

        ipcJsonFormat = incoming_message.captureTrigger_.captureTriggersMessageFull_;
        ipcJsonFormat["status"] = "Success";
        ipcJsonFormat["statusDescription"] = "";
        ipcJsonFormat["response"] = incoming_message.inferenceDetailsMap_["response"];

        if (!inList(incoming_message.captureTrigger_.captureTriggersMessage_,{"live", "capture","capture_"}))
        {
          ipcResponseInferenceJson = ipcJsonFormat["response"]["inferenceResults"];
          if(incoming_message.inferenceMode_ != InferenceInputModeE::TRITON)
            ipcResponseInferenceJson["maskImage"] = result_file_names;
        }

        ipcResponseJson = ipcJsonFormat["response"];
        ipcResponseJson["image"] = image_file_names;
        if (!inList(incoming_message.captureTrigger_.captureTriggersMessage_,{"live", "capture","capture_"}))
        {
          ipcResponseJson["inferenceResults"] = ipcResponseInferenceJson;
        }

        ipcJsonFormat["response"] = ipcResponseJson;

        if (ipcJsonFormat.dump().length()>131072)
        {
          ipcJsonFormat["response"]["inferenceResults"]["results"] = "MESSAGE TOO LONG";
        }

        ipcJsonFormatStr = ipcJsonFormat.dump();

        LOG_ALWAYS("[OUTPUT::PUBLISH IPC TOPIC] ipcJsonFormatStr = ");
        LOG_ALWAYS(std::string(ipcJsonFormatStr));

        String topic(ipcTopicName.c_str());
        String message(ipcJsonFormatStr.c_str());

        PublishToTopicRequest request_;
        Vector<uint8_t> messageData({message.begin(), message.end()});
        BinaryMessage binaryMessage;
        binaryMessage.SetMessage(messageData);
        PublishMessage publishMessage;
        publishMessage.SetBinaryMessage(binaryMessage);
        request_.SetTopic(topic);
        request_.SetPublishMessage(publishMessage);

        PublishToTopicOperation operation = ipcClient->NewPublishToTopic();

        auto requestStatus = operation.Activate(request_).get();
        if (!requestStatus)
        {
            LOG_ALWAYS("[OUTPUT::PUBLISH IPC TOPIC] requestStatus = " + std::string(requestStatus.StatusToString()));
        }

        auto responseFuture = operation.GetResult();
        if (responseFuture.wait_for(std::chrono::seconds(timeout)) == std::future_status::timeout)
        {
            LOG_ALWAYS("[OUTPUT::PUBLISH IPC TOPIC] Operation timed out while waiting for response from Greengrass Core.");
        }

        auto publishResult = responseFuture.get();
        if (publishResult)
        {
            LOG_ALWAYS("[OUTPUT::PUBLISH IPC TOPIC] Successfully published to topic: " + std::string(topic));
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
                    LOG_ALWAYS("[OUTPUT::PUBLISH IPC TOPIC] Greengrass Core responded with an error: " + std::string(error->GetMessage().value()));
            }
            else
            {
                LOG_ALWAYS("[OUTPUT::PUBLISH IPC TOPIC] Attempting to receive the response from the server failed with error code: " + std::string(publishResult.GetRpcError().StatusToString()));
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

    /**
      Routine for publishing json to topic subscribed to based on GPIO
      @param completed getting the parameter to know if the other pipeline stages are completed
    */
    void PublishToIpc::publishGpioToTopic(bool& completed)
    {
      int iteration = 0;
      bool is_recorded = false;
      while(1)
      {
        mtx_.lock();
        gpioRet = gpio.gpio_getvalue(GPIO_DATA_IN_6, gpioValue); // to find if data valid or not
        if (gpioRet==GPIO_SUCCESS && gpioValue==1 && !is_recorded)
        {
          auto start_time = std::chrono::steady_clock::now();
          LOG_ALWAYS("[OUTPUT::PUBLISH IPC TOPIC w/ GPIO] Iteration #" + std::to_string(iteration));
          std::this_thread::sleep_for(std::chrono::milliseconds(1*jsonParams_["clockTime"].as_int())); // wait for 1 clock cycle

          gpioRet = gpio.gpio_getvalue(GPIO_DATA_IN_7, gpioValueRes); // to find if anomaly or not

          LOG_ALWAYS("[OUTPUT::PUBLISH IPC TOPIC w/ GPIO] Publishing to topic: " + ipcTopicName + " with Anomaly/Normal as: " + std::to_string(gpioValue));

          image_file_names = ipcJsonFormat["imageLocation"].dump();
          inference_details = ipcJsonFormat["inferenceDetails"].dump();

          ipcJsonFormatStr = "";
          ipcJsonFormatStr.append("""{\n\"inferenceMode\": \"existing\",\n");
          ipcJsonFormatStr.append("\"inferenceResults\": ");
          if (gpioValueRes==1)
            ipcJsonFormatStr.append("\"anomaly\",\n");
          else
            ipcJsonFormatStr.append("\"normal\",\n");
          ipcJsonFormatStr.append("\"imageLocation\": \"");
          ipcJsonFormatStr.append(image_file_names.c_str());
          ipcJsonFormatStr.append("\",\n");
          ipcJsonFormatStr.append("\"inferenceDetails\": ");
          ipcJsonFormatStr.append(inference_details.c_str());
          ipcJsonFormatStr.append("\n}""");

          LOG_ALWAYS("[OUTPUT::PUBLISH IPC TOPIC w/ GPIO] ipcJsonFormatStr = " + ipcJsonFormatStr);

          std::this_thread::sleep_for(std::chrono::milliseconds(4*jsonParams_["clockTime"].as_int())); // wait for 4 clock cycle and wait until anomaly/normal signal returns

          String topic(ipcTopicName.c_str());
          String message(ipcJsonFormatStr.c_str());

          PublishToTopicRequest request_;
          Vector<uint8_t> messageData({message.begin(), message.end()});
          BinaryMessage binaryMessage;
          binaryMessage.SetMessage(messageData);
          PublishMessage publishMessage;
          publishMessage.SetBinaryMessage(binaryMessage);
          request_.SetTopic(topic);
          request_.SetPublishMessage(publishMessage);

          PublishToTopicOperation operation = ipcClient->NewPublishToTopic();

          auto requestStatus = operation.Activate(request_).get();
          if (!requestStatus)
          {
              LOG_ALWAYS("[OUTPUT::PUBLISH IPC TOPIC w/ GPIO] requestStatus = " + std::string(requestStatus.StatusToString()));
          }

          auto responseFuture = operation.GetResult();
          if (responseFuture.wait_for(std::chrono::seconds(timeout)) == std::future_status::timeout)
          {
              LOG_ALWAYS("[OUTPUT::PUBLISH IPC TOPIC w/ GPIO] Operation timed out while waiting for response from Greengrass Core.");
          }

          auto publishResult = responseFuture.get();
          if (publishResult)
          {
              LOG_ALWAYS("[OUTPUT::PUBLISH IPC TOPIC w/ GPIO] Successfully published to topic: " + std::string(topic));
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
                      LOG_ALWAYS("[OUTPUT::PUBLISH IPC TOPIC w/ GPIO] Greengrass Core responded with an error: " + std::string(error->GetMessage().value()));
              }
              else
              {
                  LOG_ALWAYS("[OUTPUT::PUBLISH IPC TOPIC w/ GPIO] Attempting to receive the response from the server failed with error code: " + std::string(publishResult.GetRpcError().StatusToString()));
              }
          }

          start_timeout_gpio = std::chrono::steady_clock::now();
          completed = false;

          auto end_time = std::chrono::steady_clock::now();
          std::chrono::duration<double> elapsed_seconds = (end_time - start_time);

          LOG_ALWAYS("[OUTPUT::PUBLISH IPC TOPIC w/ GPIO] Average time to running GPIO PUBLISH is: " + std::to_string(elapsed_seconds.count()) + " seconds.");

          is_recorded = true;
          iteration++;
        }
        else if (gpioValue==1 && is_recorded)
        {
          std::this_thread::sleep_for(std::chrono::milliseconds(jsonParams_["clockTime"].as_int())); // wait for 1 clock cycle
        }
        else if (gpioValue==0 && is_recorded)
        {
          is_recorded = false;
        }
        else
        {
          end_timeout_gpio = std::chrono::steady_clock::now();
          duration_timeout_gpio = (end_timeout_gpio - start_timeout_gpio);

          // if the timeout duration is greater than 'timeout' seconds, complete the stage
          if (jsonParams_["timeout"].as_longint()<0)
            continue;

          if (duration_timeout_gpio.count()>jsonParams_["timeout"].as_longint()/1000)
          {
            LOG_ALWAYS("[OUTPUT::PUBLISH IPC TOPIC w/ GPIO] Closing due to Timeout of " + std::to_string(duration_timeout_gpio.count()) + " seconds.");
            completed = true;
            break;
          }
        }
        mtx_.unlock();
      }
    }

  }
}
