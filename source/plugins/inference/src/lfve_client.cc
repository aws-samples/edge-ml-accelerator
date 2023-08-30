/**
 * @lfve_client.cc
 * @brief Creating and Running Inference Client
 *
 * This contains the function definitions for running inference using Lookout For Vision for Edge API.
 * The API will initialize the clients, attach to the model ids and the servers and run inference.
 *
 */

#include <edge-ml-accelerator/inference/lfve_client.h>
#include <unistd.h>

namespace edgeml
{
  namespace inference
  {

    /**
      Instance of the class
    */
    LFVEclient* LFVEclient::instance(utils::jsonParser::jValue j, int inferIdx, std::string p, double anomaly_threshold, SharedMessage<MessageCaptureInference> & shared_map)
    {
      static LFVEclient* inst = 0;

      if (!inst)
      {
        inst = new LFVEclient(j, inferIdx, p, anomaly_threshold, shared_map);
      }
      return inst;
    }

    /**
      Creates the class constructor
    */
    LFVEclient::LFVEclient(utils::jsonParser::jValue j, int inferIdx, std::string p, double anomaly_threshold, SharedMessage<MessageCaptureInference> & shared_map) : Inference(j, shared_map), pipelineName_(p), anomaly_threshold_(anomaly_threshold)
    {
      LOG_ALWAYS("[INFERENCE::LFVEclient] Inference mode is set to LFVE.");
      LOG_ALWAYS("[INFERENCE::LFVEclient] Pipeline Name: " + pipelineName_);
      start_timeout = std::chrono::steady_clock::now();
      inferIdx_ = inferIdx;
      useGpio = jsonParams_["useGpio"].as_bool();

      inferenceBaseInferenceResultsNlohmannJson_["inferenceType"] = "lfve";
      inferenceBaseInferenceResultsNlohmannJson_["inferenceResults"] = lfveResultsNlohmannJson_;
    }

    /**
      Creates the class destructor
    */
    LFVEclient::~LFVEclient()
    {
    }

    /**
      Initializing the inference API with channels and stubs based on config
    */
    int LFVEclient::initInfer(int inferIdx)
    {
      LOG_ALWAYS("[INFERENCE::LFVEclient]");
      grpc::ChannelArguments channel_options;
      channel_options.SetMaxReceiveMessageSize( 4000 * 4000 * 4);
      channel = grpc::CreateCustomChannel("unix:///tmp/aws.iot.lookoutvision.EdgeAgent.sock", grpc::InsecureChannelCredentials(), channel_options);
      numModels = jsonParams_["inference"][inferIdx]["model_ids"].size();
      stubs.resize(numModels);
      model_name.resize(numModels);
      model_input_height.resize(numModels);
      model_input_width.resize(numModels);
      is_anomaly_overall.resize(numModels);
      confidence_overall.resize(numModels);
      threshold.resize(numModels);

      #pragma omp parallel for
      for (int i=0; i<numModels; i++)
      {
        LOG_ALWAYS("[INFERENCE::LFVEclient] Starting the client for model #" + std::to_string(i+1));
        model_name[i] = jsonParams_["inference"][inferIdx]["model_ids"][i]["model_name"].as_string();
        model_input_height[i] = jsonParams_["inference"][inferIdx]["model_ids"][i]["model_height"].as_int();
        model_input_width[i] = jsonParams_["inference"][inferIdx]["model_ids"][i]["model_width"].as_int();
        request.set_model_component(model_name[i]);
        stubs[i] = AWS::LookoutVision::EdgeAgent::NewStub(channel); // Creating Inference Stub
        LOG_ALWAYS("[INFERENCE::LFVEclient] model #" + std::to_string(i+1) + " ID = " + jsonParams_["inference"][inferIdx]["model_ids"][i]["model_name"].as_string());
        LOG_ALWAYS("[INFERENCE::LFVEclient] Connecting to the server for model #" + std::to_string(i+1));
        is_anomaly_overall[i] = true;
        confidence_overall[i] = 1.0;
        threshold[i] = jsonParams_["inference"][inferIdx]["model_ids"][i]["threshold"].as_double();
        grpc::ClientContext context;
        dmRequest.set_model_component(model_name[i]);
        stubs[i]->DescribeModel(&context, dmRequest, &dmResponse);
        modelDescription = dmResponse.model_description();
        if (modelDescription.status() == AWS::LookoutVision::STOPPED || modelDescription.status() == AWS::LookoutVision::STOPPING || modelDescription.status() == AWS::LookoutVision::STARTING) // model is STOPPED -> START
        {
          if (modelDescription.status() == AWS::LookoutVision::STOPPED || modelDescription.status() == AWS::LookoutVision::STOPPING)
          {
            grpc::ClientContext context;
            LOG_ALWAYS("[INFERENCE::LFVEclient] model " + jsonParams_["inference"][inferIdx]["model_ids"][i]["model_name"].as_string() + " #" + std::to_string(i+1) + " is STOPPED. Starting now...");
            AWS::LookoutVision::StartModelRequest smRequest; smRequest.set_model_component(model_name[i]);
            AWS::LookoutVision::StartModelResponse smResponse;
            stubs[i]->StartModel(&context, smRequest, &smResponse);
          }
          grpc::ClientContext context;
          dmRequest.set_model_component(model_name[i]);
          stubs[i]->DescribeModel(&context, dmRequest, &dmResponse);
          modelDescription = dmResponse.model_description();
          LOG_ALWAYS("[INFERENCE::LFVEclient] model #" + std::to_string(i+1) + " is starting with status: " + std::string(LfveModelStatusE[modelDescription.status()]));

          // Check until model is running for 6 iterations each of 30 seconds
          int check_iter = 0, max_check_iter = 6;
          bool model_isRunning = false;
          while (check_iter<max_check_iter)
          {
            check_iter++;
            grpc::ClientContext context;
            dmRequest.set_model_component(model_name[i]);
            stubs[i]->DescribeModel(&context, dmRequest, &dmResponse);
            modelDescription = dmResponse.model_description();
            LOG_ALWAYS("[INFERENCE::LFVEclient] model " + jsonParams_["inference"][inferIdx]["model_ids"][i]["model_name"].as_string() + " #" + std::to_string(i+1) + " is starting with status: " + std::string(LfveModelStatusE[modelDescription.status()]) + " (" + std::to_string(check_iter) + "/" + std::to_string(max_check_iter) + ")");
            if (modelDescription.status() == AWS::LookoutVision::RUNNING)
            {
              LOG_ALWAYS("[INFERENCE::LFVEclient] model " + jsonParams_["inference"][inferIdx]["model_ids"][i]["model_name"].as_string() + " #" + std::to_string(i+1) + " has status: " + std::string(LfveModelStatusE[modelDescription.status()]));
              model_isRunning = true;
              break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(30000)); // Wait for 30 seconds before checking again
          }
        }
        else if (modelDescription.status() == AWS::LookoutVision::RUNNING)
        {
          LOG_ALWAYS("[INFERENCE::LFVEclient] model " + jsonParams_["inference"][inferIdx]["model_ids"][i]["model_name"].as_string() + " #" + std::to_string(i+1) + " has status: " + std::string(LfveModelStatusE[modelDescription.status()]));
          LOG_ALWAYS("[INFERENCE::LFVEclient] Inference results for [" + model_name[i] + "] is completed");
        }
        else if (modelDescription.status() == AWS::LookoutVision::FAILED)
        {
          LOG_ALWAYS("[INFERENCE::LFVEclient] model " + jsonParams_["inference"][inferIdx]["model_ids"][i]["model_name"].as_string() + " #" + std::to_string(i+1) + std::string(LfveModelStatusE[modelDescription.status()]));
        }
      }

      // Warm-up model for 5 inferences
      unsigned char tempBuffer[1280*720*3] = {0};
      for (int iter=0; iter<5; iter++)
      {
        LOG_ALWAYS("[INFERENCE::LFVEclient] Model Warmup : Running Dummy Inference for #" + std::to_string(iter+1) + "/5");
        bitmap.set_width(1280);
        bitmap.set_height(720);
        bitmap.set_byte_data(tempBuffer, 1280*720*3);
        for (int i=0; i<numModels; i++)
        {
          grpc::ClientContext context;
          request.set_model_component(model_name[i]);
          request.set_allocated_bitmap(&bitmap);
          grpc::Status status = stubs[i]->DetectAnomalies(&context, request, &response);
          reply = response.detect_anomaly_result();
          lfveBMret = request.release_bitmap();
        }
      }

      ret = INFERENCE_OK;

      return ret;
    }

    /**
      Run inference for LFVE to find anamolies and probablities
      @param errc returning the error code of capture API
      @param height height of the input image
      @param width width of the input image
      @param is_anomaly boolean for anomaly or not
      @param confidence float value of the probability of anomaly or not
      @param iter running for total iterations if > 0 else running infinite loop
    */
    void LFVEclient::runInference(int& errc, int& height, int& width, int& iter, bool& completed)
    {
      int ret = initInfer(inferIdx_);
      LOG_ALWAYS("[INFERENCE::LFVEclient]  Inference Pipeline Ready is ready");
      int local_iter = 0;
      while(1)
      {

        bool data_available = true;

        auto message = camera2ongoing_.GetMessage();
        LOG_ALWAYS("[INFERENCE::LFVEclient] Capture Trigger Message = " + message.captureTrigger_.captureTriggersMessage_);
        message.inferenceMode_ = InferenceInputModeE::LFVE;

        LOG_ALWAYS("[INFERENCE::LFVEclient]");

        inference_start_time = std::chrono::steady_clock::now();

        is_anomaly = true; confidence = 1.0;
        bool original_is_anomaly = true;

        LOG_ALWAYS("[INFERENCE::LFVEclient] Inference started for model(s)");
        for (int i=0; i<numModels; i++)
        {

          unsigned char* inputImage = message.safeCaptureContainer_.front();
          int inputImageSize = message.safeCaptureSizeContainer_.front();

          if (width!=model_input_width[i] || height!=model_input_height[i])
          {
            LOG_ALWAYS("[INFERENCE::LFVEclient] Capture image size is different from Model input size");
            LOG_ALWAYS("[INFERENCE::LFVEclient]     Capture size [HxW] = [" + std::to_string(height) + "," + std::to_string(width) + "]");
            LOG_ALWAYS("[INFERENCE::LFVEclient]     Model input size [HxW] = [" + std::to_string(model_input_height[i]) + "," + std::to_string(model_input_width[i]) + "]");
            LOG_ALWAYS("[INFERENCE::LFVEclient] Resizing image based on model input size");
            unsigned char* outputImage;
            ret = imagePreprocess.resize(inputImage, width, height, outputImage, model_input_width[i], model_input_height[i], 3);
            inputImageSize = model_input_width[i]*model_input_height[i]*3;
            inputImage = outputImage;
          }

          bitmap.set_width((int) model_input_width[i]);
          bitmap.set_height((int) model_input_height[i]);
          bitmap.set_byte_data(inputImage, inputImageSize);

          grpc::ClientContext context;
          request.set_model_component(model_name[i]);
          request.set_allocated_bitmap(&bitmap);
          grpc::Status status = stubs[i]->DetectAnomalies(&context, request, &response);
          reply = response.detect_anomaly_result();

          is_anomaly_overall[i] = reply.is_anomalous(); // true:anomaly, false:normal
          confidence_overall[i] = reply.confidence(); // confidence
          original_is_anomaly = is_anomaly_overall[i];
          if ((threshold[i]>0 && confidence_overall[i]<threshold[i] && !is_anomaly_overall[i]) || confidence_overall[i]==0.0 && !is_anomaly_overall[i])
          {
            is_anomaly_overall[i] = true;
            if (threshold[i]>=0)
              LOG_ALWAYS("[INFERENCE::LFVEclient] Acceptable confidence threshold: " + std::to_string(threshold[i]));
            LOG_ALWAYS("[INFERENCE::LFVEclient] Actual [anomaly]: " + std::to_string(original_is_anomaly));
            LOG_ALWAYS("[INFERENCE::LFVEclient] Final [anomaly] based on threshold and confidence: " + std::to_string(is_anomaly_overall[i]));
          }

          anomaly_mask = reply.anomaly_mask(); // output anomaly mask
          anomaliesVec = std::vector<AWS::LookoutVision::Anomaly>(reply.anomalies().begin(), reply.anomalies().end()); // output anomalies
          lfveBMret = request.release_bitmap();
          is_anomaly = is_anomaly & is_anomaly_overall[i]; // is_anomaly & is_anomaly_overall[i];
          confidence = confidence * confidence_overall[i]; // confidence * confidence_overall[i];
        }

        // Update the Anomalies
        lfveResultsNlohmannJson_["anomalies"][anomaliesVec.size()] = {};
        for (int avec=0; avec<anomaliesVec.size(); avec++)
        {
          lfveAnomaliesNlohmannJson_["totalPercentageArea"] = std::to_string(anomaliesVec[avec].pixel_anomaly().total_percentage_area());
          lfveAnomaliesNlohmannJson_["hexColor"] = anomaliesVec[avec].pixel_anomaly().hex_color();
			    lfveAnomaliesNlohmannJson_["name"] = anomaliesVec[avec].name();
          lfveResultsNlohmannJson_["anomalies"][avec] = lfveAnomaliesNlohmannJson_;

          // LookoutForVision has issues so using this to flip the Anomaly in certain cases
          if (anomaliesVec[avec].name().compare("background") != 0 and anomaliesVec[avec].pixel_anomaly().total_percentage_area() > anomaly_threshold_)
          {
            if (is_anomaly == false)
            {
              LOG_ALWAYS("[INFERENCE::LFVEclient] Re-adjusting anomaly flag");
            }
            is_anomaly = true;
          }
        }

        int is_anomaly_int = int(is_anomaly);
        LOG_ALWAYS("[INFERENCE::LFVEclient] Overall inference results: [anomaly,confidence] = [" + std::to_string(is_anomaly_int) + "," + std::to_string(confidence) + "]");
        LOG_ALWAYS("[INFERENCE::LFVEclient] Anomaly Mask size = " + std::to_string(anomaly_mask.width()) + " x " + std::to_string(anomaly_mask.height()));

        if (useGpio)
        {
          // Running GPIO logic here
          gpioRet = GPIO_FAIL;
          while (gpioRet==GPIO_FAIL)
          {
            gpioRet = gpio.gpio_setvalue(GPIO_DATA_OUT_1, 1); // Setting the Data Valid Output
          }

          if (is_anomaly_int==0) // Only if Anomaly is NOT detected
          {
            std::this_thread::sleep_for(std::chrono::milliseconds(jsonParams_["clockTime"].as_int())); // Wait for 10ms
            gpioRet = GPIO_FAIL;
            while (gpioRet==GPIO_FAIL)
            {
              gpioRet = gpio.gpio_setvalue(GPIO_DATA_OUT_2, 1); // Setting the Anomaly Status Output
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(4*jsonParams_["clockTime"].as_int())); // Wait for 40ms
            gpioRet = GPIO_FAIL;
            while (gpioRet==GPIO_FAIL)
            {
              gpioRet = gpio.gpio_setvalue(GPIO_DATA_OUT_2, 0); // Un-Setting the Anomaly Status Output
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(jsonParams_["clockTime"].as_int()));
          }
          else
          {
            std::this_thread::sleep_for(std::chrono::milliseconds(6*jsonParams_["clockTime"].as_int())); // Wait for 60ms for Normal without Setting/Un-Setting the Status Output
          }

          gpioRet = GPIO_FAIL;
          while (gpioRet==GPIO_FAIL)
          {
            gpioRet = gpio.gpio_setvalue(GPIO_DATA_OUT_1, 0); // Un-Setting the Data Valid Output
          }
        }

        inference_end_time = std::chrono::steady_clock::now();
        inference_duration = std::chrono::duration_cast<std::chrono::milliseconds>(inference_end_time - inference_start_time);

        // Queue updates
        lfveResultsNlohmannJson_["isOriginallyAnomalous"] = original_is_anomaly ? "true" : "false";
        lfveResultsNlohmannJson_["isAnomalous"] = is_anomaly ? "true" : "false";
        lfveResultsNlohmannJson_["confidence"] = std::to_string(confidence);
        lfveResultsNlohmannJson_["anomalyMask"] = "long-byte-array";

        if ((int)anomaly_mask.width() != model_input_width[0] or (int)anomaly_mask.height() != model_input_height[0])
        {
          LOG_ALWAYS("[INFERENCE::LFVEclient] #######################################");
          LOG_ALWAYS("[INFERENCE::LFVEclient] Model and Images are of different sizes");
          LOG_ALWAYS("[INFERENCE::LFVEclient] #######################################");
        }

        unsigned char* resizedMask;
        if (((int)anomaly_mask.width())*((int)anomaly_mask.height())>0 && (int)anomaly_mask.width() == model_input_width[0] && (int)anomaly_mask.height() == model_input_height[0])
        {
          if (model_input_width[0]==width && model_input_height[0]==height)
          {
            resizedMask = reinterpret_cast<unsigned char*>(const_cast<char*>(anomaly_mask.byte_data().c_str()));
          }
          else
          {
            ret = imagePreprocess.resize(reinterpret_cast<unsigned char*>(const_cast<char*>(anomaly_mask.byte_data().c_str())), model_input_width[0], model_input_height[0], resizedMask, width, height, 3);
          }
          LOG_ALWAYS("[INFERENCE::LFVEclient] Mask is detected");
        }
        else
        {
          resizedMask = message.safeCaptureContainer_.front();
          LOG_ALWAYS("[INFERENCE::LFVEclient] Mask is NOT detected");
        }

        // // TODO
        // lfveResultsNlohmannJson_["anomalyMask"] = resizedMask;
        inferenceBaseInferenceResultsNlohmannJson_["inferenceResults"] = lfveResultsNlohmannJson_;
        inferenceBaseInferenceResultsNlohmannJson_["inferenceTime"] = std::to_string(inference_duration.count());
        inferenceBaseInferenceResultsNlohmannJson_["framesPerSecond"] = std::to_string(1./inference_duration.count());
        inferenceBaseInferenceResultsNlohmannJson_["imageLocation"] = "__undefined__";
        inferenceBaseInferenceResultsNlohmannJson_["resultLocation"] = "__undefined__";

        message.inferenceDetailsMap_["response"] = inferenceBaseInferenceResultsNlohmannJson_;
        message.inferenceDetailsMap_["numInferencesDone"] = std::to_string(1);

        errc = INFERENCE_OK;
        start_timeout = std::chrono::steady_clock::now();
        completed = false;

        if(produce_output_)
        {
          output_inference_.produce_message(message);
          LOG_ALWAYS("[INFERENCE::LFVEclient] Output Inference Size = " + std::to_string(output_inference_.size()));
        }
      }
    }

  }
}
