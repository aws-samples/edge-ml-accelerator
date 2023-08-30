/**
 * @onnxruntime_client.cc
 * @brief Creating and Running Inference Client for ONNX Runtime
 *
 * This contains the function definitions for running inference using ONNX Runtime API.
 * The API will initialize the clients, attach to the model ids and the servers and run inference.
 *
 */

#include <edge-ml-accelerator/inference/onnxruntime_client.h>
#include <unistd.h>

namespace edgeml
{
  namespace inference
  {

    /**
      Instance of the class
    */
    OnnxRuntimeClient* OnnxRuntimeClient::instance(utils::jsonParser::jValue j, int inferIdx, std::string p, SharedMessage<MessageCaptureInference> & shared_map)
    {
      static OnnxRuntimeClient* inst = 0;

      if (!inst)
      {
        inst = new OnnxRuntimeClient(j, inferIdx, p, shared_map);
      }
      return inst;
    }

    /**
      Creates the class constructor
    */
    OnnxRuntimeClient::OnnxRuntimeClient(utils::jsonParser::jValue j, int inferIdx, std::string p, SharedMessage<MessageCaptureInference> & shared_map) : Inference(j, shared_map), pipelineName_(p)
    {
      LOG_ALWAYS("[INFERENCE::OnnxRuntimeClient] Inference mode is set to ONNX Runtime.");
      LOG_ALWAYS("[INFERENCE::OnnxRuntimeClient] Pipeline Name: " + pipelineName_);
      start_timeout = std::chrono::steady_clock::now();
      LOG_ALWAYS("[INFERENCE::OnnxRuntimeClient] Inference Name: " + jsonParams_["inference"][inferIdx]["inferName"].as_string());
      useGpio = jsonParams_["useGpio"].as_bool();
      scaleBy = jsonParams_["preprocess"]["scaleBy"].as_int();
      int ret = initInfer(inferIdx);

      if (ret == MODEL_FAILURE)
      {
        LOG_ALWAYS("[INFERENCE::OnnxRuntimeClient] Model failure. Cannot run Inference.");
      }

      inferenceBaseInferenceResultsNlohmannJson_["inferenceType"] = "onnx";
      inferenceBaseInferenceResultsNlohmannJson_["inferenceResults"] = customResultsNlohmannJson_;

    }

    /**
      Creates the class destructor
    */
    OnnxRuntimeClient::~OnnxRuntimeClient()
    {
      g_ort->ReleaseValue(outputTensors);
      g_ort->ReleaseValue(inputTensors);
      for(OrtSession* sess : ort_session_vec)
        g_ort->ReleaseSession(sess);
      g_ort->ReleaseEnv(pOrt_env);
    }

    /**
      Checking Status of ONNX API
    */
    void OnnxRuntimeClient::CheckStatus(OrtStatus* status)
    {
      if (status != NULL)
      {
        const char* msg = g_ort->GetErrorMessage(status);
        LOG_ERROR("[INFERENCE::OnnxRuntimeClient] CheckStatus Failure: " + std::string(msg));
        g_ort->ReleaseStatus(status);
        throw std::runtime_error(msg);
      }
    }

    /**
      Initializing the inference API with channels and stubs based on config
    */
    int OnnxRuntimeClient::initInfer(int inferIdx)
    {
      LOG_ALWAYS("[INFERENCE::OnnxRuntimeClient]");

      std::string instanceName = "onnx-env";
      g_ort = OrtGetApiBase()->GetApi(ORT_API_VERSION);
      CheckStatus(g_ort->CreateEnv(ORT_LOGGING_LEVEL_WARNING, instanceName.c_str(), &pOrt_env));
      OrtSessionOptions* session_options;

      CheckStatus(g_ort->CreateSessionOptions(&session_options));
      CheckStatus(g_ort->SetIntraOpNumThreads(session_options, 1));
      CheckStatus(g_ort->SetInterOpNumThreads(session_options, 1));
      CheckStatus(g_ort->SetSessionGraphOptimizationLevel(session_options, ORT_ENABLE_ALL));
      CheckStatus(g_ort->DisableMemPattern(session_options));

      // If possible, using CUDA to improve performance of ONNX models
      bool USE_CUDA = false;
      int provider_length = 0; char** providers;
      CheckStatus(g_ort->GetAvailableProviders(&providers, &provider_length));
      for (int pl=0; pl<provider_length; pl++)
      {
        if (std::string(providers[pl]) == "CUDAExecutionProvider")
        {
          USE_CUDA = true;
          break;
        }
      }
      if (USE_CUDA)
      {
        LOG_ALWAYS("[INFERENCE::OnnxRuntimeClient] GPU FOUND -> USING CUDA");
        OrtCUDAProviderOptionsV2* cuda_options = nullptr;
        CheckStatus(g_ort->CreateCUDAProviderOptions(&cuda_options));
        std::vector<const char*> keys{"device_id", "gpu_mem_limit", "arena_extend_strategy", "cudnn_conv_algo_search", "do_copy_in_default_stream", "cudnn_conv_use_max_workspace", "cudnn_conv1d_pad_to_nc1d"};
        std::vector<const char*> values{"0", "1073741824", "kSameAsRequested", "DEFAULT", "1", "1", "1"};
        CheckStatus(g_ort->UpdateCUDAProviderOptions(cuda_options, keys.data(), values.data(), keys.size()));
        CheckStatus(g_ort->SessionOptionsAppendExecutionProvider_CUDA_V2(session_options, cuda_options));
      }
      else
      {
        LOG_ALWAYS("[INFERENCE::OnnxRuntimeClient] NO GPU FOUND -> USING CPU");
      }

      g_ort->ReleaseValue(outputTensors);
      g_ort->ReleaseValue(inputTensors);
      OrtStatus* status;
      OrtAllocator* allocator;
      CheckStatus(g_ort->GetAllocatorWithDefaultOptions(&allocator));

      numModels = jsonParams_["inference"][inferIdx]["model_ids"].size();
      for (int i=0; i<numModels; i++)
      {
        LOG_ALWAYS("[INFERENCE::OnnxRuntimeClient] Starting the client for model #" + std::to_string(i+1));
        model_name.push_back(jsonParams_["inference"][inferIdx]["model_ids"][i]["model_name"].as_string());
        model_path.push_back(jsonParams_["inference"][inferIdx]["model_ids"][i]["model_path"].as_string());
        model_type.push_back(jsonParams_["inference"][inferIdx]["model_ids"][i]["model_type"].as_string());
        LOG_ALWAYS("[INFERENCE::OnnxRuntimeClient] model #" + std::to_string(i+1) + " Name = " + jsonParams_["inference"][inferIdx]["model_ids"][i]["model_name"].as_string());
        LOG_ALWAYS("[INFERENCE::OnnxRuntimeClient] model #" + std::to_string(i+1) + " Path = " + jsonParams_["inference"][inferIdx]["model_ids"][i]["model_path"].as_string());

        ort_session_vec.push_back(nullptr);
        CheckStatus(g_ort->CreateSession(pOrt_env, model_path[i].c_str(), session_options, &ort_session_vec[i]));

        LOG_ALWAYS("[INFERENCE::OnnxRuntimeClient] Model " + model_name[i] + " is loaded");

        size_t inputDims, outputDims;

        // Print the Input Tensor Details and store in a vector for future access
        std::vector<int64_t> input_node_dims;
        char* inputName;
        status = g_ort->SessionGetInputName(ort_session_vec[i], 0, allocator, &inputName);
        input_tensor_names.push_back(std::string(inputName));
        inputNames.push_back(inputName);
        OrtTypeInfo* inputTypeInfo;
        status = g_ort->SessionGetInputTypeInfo(ort_session_vec[i], 0, &inputTypeInfo);
        const OrtTensorTypeAndShapeInfo* inputTensorInfo;
        CheckStatus(g_ort->CastTypeInfoToTensorInfo(inputTypeInfo, &inputTensorInfo));
        CheckStatus(g_ort->GetDimensionsCount(inputTensorInfo, &inputDims));
        input_node_dims.resize(inputDims);
        CheckStatus(g_ort->GetDimensions(inputTensorInfo, (int64_t*)input_node_dims.data(), inputDims));
        if (input_node_dims[0] < 0) {input_node_dims[0] = 1;};
        input_tensor_shape.push_back(std::vector<int>(input_node_dims.begin(), input_node_dims.end()));
        LOG_ALWAYS("[INFERENCE::OnnxRuntimeClient] Model Details = (Input Tensor): [" + input_tensor_names[input_tensor_names.size()-1] + "] (");
        for (int j=0; j<input_node_dims.size()-1; j++)
        {
          LOG_ALWAYS(input_node_dims[j] + ", ");
        }
        LOG_ALWAYS(input_node_dims[input_node_dims.size()-1] + ")");
        if (input_node_dims[1]==1 || input_node_dims[1]==2 || input_node_dims[1]==3)
        {
          is_chw_vec.push_back(true);
          LOG_ALWAYS("[INFERENCE::OnnxRuntimeClient] Input Format = NCHW");
        }
        else
        {
          is_chw_vec.push_back(false);
          LOG_ALWAYS("[INFERENCE::OnnxRuntimeClient] Input Format = NHWC");
        }

        // Print the Output Tensor Details and store in a vector for future access
        std::vector<int64_t> output_node_dims;
        char* outputName;
        status = g_ort->SessionGetOutputName(ort_session_vec[i], 0, allocator, &outputName);
        output_tensor_names.push_back(std::string(outputName));
        outputNames.push_back(outputName);
        OrtTypeInfo* outputTypeInfo;
        status = g_ort->SessionGetOutputTypeInfo(ort_session_vec[i], 0, &outputTypeInfo);
        const OrtTensorTypeAndShapeInfo* outputTensorInfo;
        CheckStatus(g_ort->CastTypeInfoToTensorInfo(outputTypeInfo, &outputTensorInfo));
        CheckStatus(g_ort->GetDimensionsCount(outputTensorInfo, &outputDims));
        output_node_dims.resize(outputDims);
        CheckStatus(g_ort->GetDimensions(outputTensorInfo, (int64_t*)output_node_dims.data(), outputDims));
        if (output_node_dims[0] < 0) {output_node_dims[0] = 1;};
        output_tensor_shape.push_back(std::vector<int>(output_node_dims.begin(), output_node_dims.end()));
        LOG_ALWAYS("[INFERENCE::OnnxRuntimeClient] Model Details = (Output Tensor): [" + output_tensor_names[output_tensor_names.size()-1] + "] (");
        for (int j=0; j<output_node_dims.size()-1; j++)
        {
          LOG_ALWAYS(output_node_dims[j] + ", ");
        }
        LOG_ALWAYS(output_node_dims[output_node_dims.size()-1] + ")");

        // Set tensor shapes and sizes
        long int indatasize = 1;
        for (int j = 0; j < input_tensor_shape[i].size(); ++j)
        {
          indatasize *= input_tensor_shape[i][j];
        }
        input_tensor_size.push_back(indatasize);
        input_height.push_back(input_tensor_shape[i][1]); input_width.push_back(input_tensor_shape[i][2]); input_channels.push_back(input_tensor_shape[i][3]);
        if (is_chw_vec[i])
        {
          input_height[i] = input_tensor_shape[i][2];
          input_width[i] = input_tensor_shape[i][3];
          input_channels[i] = input_tensor_shape[i][1];
        }
        float chardata[indatasize] = {0};
        inputTensorValues = std::vector<float>(indatasize);
        std::vector<unsigned char> tmpInputVec(indatasize, 0);
        inputDataVec.push_back(tmpInputVec);
        long int outdatasize = 1;
        for (int j = 0; j < output_tensor_shape[i].size(); ++j)
        {
          outdatasize *= output_tensor_shape[i][j];
        }
        output_tensor_size.push_back(outdatasize);
        output_height.push_back(output_tensor_shape[i][1]); output_width.push_back(output_tensor_shape[i][2]); output_channels.push_back(output_tensor_shape[i][3]);
        outputTensorValues = std::vector<float>(outdatasize);
        std::vector<float> tmpOutputVec(output_tensor_size[i], 0);
        outputDataVec.push_back(tmpOutputVec);
        inputNamesVec.push_back(inputNames);
        outputNamesVec.push_back(outputNames);

        // Create input tensor object from data values
        LOG_ALWAYS("[INFERENCE::OnnxRuntimeClient] Setting up memory with input(" + std::to_string(indatasize * sizeof(float)) + ") and output(" + std::to_string(outdatasize * sizeof(float)) + ")");
        ort_memory_vec.push_back(nullptr);
        CheckStatus(g_ort->CreateCpuMemoryInfo(OrtArenaAllocator, OrtMemTypeDefault, &ort_memory_vec[i]));
        CheckStatus(g_ort->CreateTensorWithDataAsOrtValue(ort_memory_vec[i], inputTensorValues.data(), indatasize * sizeof(float), input_node_dims.data(), input_node_dims.size(), ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT, &inputTensors));
        CheckStatus(g_ort->CreateTensorWithDataAsOrtValue(ort_memory_vec[i], outputTensorValues.data(), outdatasize * sizeof(float), output_node_dims.data(), output_node_dims.size(), ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT, &outputTensors));

        ret = INFERENCE_OK;
      }

      // Warm-up model for 5 inferences
      for (int iter=0; iter<5; iter++)
      {
        LOG_ALWAYS("[INFERENCE::OnnxRuntimeClient] Model Warmup : Running Dummy Inference for #" + std::to_string(iter+1) + "/5");
        for (int i=0; i<numModels; i++)
        {
          float chardata[input_tensor_size[i]] = {0};
          inputTensorValues = std::vector<float>(chardata, chardata + input_tensor_size[i]);
          CheckStatus(g_ort->Run(ort_session_vec[i], NULL, inputNamesVec[i].data(), &inputTensors, 1, outputNamesVec[i].data(), 1, &outputTensors));
        }
      }

      return ret;
    }

    /**
      Run inference for EdgeManager for Classification, Object Detection and Segmentation models
      @param errc returning the error code of capture API
      @param height height of the input image
      @param width width of the input image
      @param iter running for total iterations if > 0 else running infinite loop
    */
    void OnnxRuntimeClient::runInference(int& errc, int& height, int& width, int& iter, bool& completed)
    {
      while(1)
      {
        auto message = camera2ongoing_.GetMessage();
        LOG_ALWAYS("[INFERENCE::OnnxRuntimeClient] Capture Trigger Message = " + message.captureTrigger_.captureTriggersMessage_);
        message.inferenceMode_ = InferenceInputModeE::ONNX;

        LOG_ALWAYS("[INFERENCE::OnnxRuntimeClient]");

        inference_start_time = std::chrono::steady_clock::now();

        unsigned char* inputImage = message.safeCaptureContainer_.front();
        LOG_ALWAYS("[INFERENCE::OnnxRuntimeClient] Inference started for model(s)");
        for (int i=0; i<numModels; i++)
        {
          ret = imagePreprocess.resize(inputImage, width, height, inputDataVec[i].data(), input_width[i], input_height[i], input_channels[i]);
          inputScaled = imagePreprocess.scale(inputDataVec[i], input_width[i], input_height[i], input_channels[i], scaleBy);
          inputTensorValues = inputScaled;

          CheckStatus(g_ort->Run(ort_session_vec[i], NULL, inputNamesVec[i].data(), &inputTensors, 1, outputNamesVec[i].data(), 1, &outputTensors));

          if (model_type[i]=="classification" || model_type[i]=="objectdetection" || model_type[i]=="segmentation" || model_type[i]=="undefined" || model_type[i]=="none")
          {
            outputDataVec[i] = outputTensorValues;
            std::transform(model_type[i].begin(), model_type[i].end(), model_type[i].begin(), std::ptr_fun<int, int>(std::toupper));
            std::transform(model_type[i].begin(), model_type[i].end(), model_type[i].begin(), std::ptr_fun<int, int>(std::tolower));
            LOG_ALWAYS("[INFERENCE::OnnxRuntimeClient] Inference results for [" + model_name[i] + "] is completed");
          }
          else
          {
            LOG_ALWAYS("[INFERENCE::OnnxRuntimeClient] Model type not correctly found. Should be one of: {classification OR objectdetection OR segmentation OR undefined OR none}");
          }
        }

        if (useGpio)
        {
          // Running GPIO logic here
          gpioRet = GPIO_FAIL;
          while (gpioRet==GPIO_FAIL)
          {
            gpioRet = gpio.gpio_setvalue(GPIO_DATA_OUT_1, 1); // Setting the Data Valid Output
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
        customResultsNlohmannJson_["inputShape"] = input_tensor_shape;
        customResultsNlohmannJson_["outputShape"] = output_tensor_shape;
        customResultsNlohmannJson_["modelType"] = model_type[0];
        if (model_type[0]=="classification" || model_type[0]=="objectdetection" || model_type[0]=="undefined" || model_type[0]=="none")
        {
          customResultsNlohmannJson_["resultType"] = "json";
        }
        else if (model_type[0]=="segmentation")
        {
          customResultsNlohmannJson_["resultType"] = "mask";
        }
        else
        {
          customResultsNlohmannJson_["resultType"] = "__undefined__";
        }
        customResultsNlohmannJson_["results"] = outputDataVec;

        inferenceBaseInferenceResultsNlohmannJson_["inferenceResults"] = customResultsNlohmannJson_;
        inferenceBaseInferenceResultsNlohmannJson_["inferenceTime"] = std::to_string(inference_duration.count());
        inferenceBaseInferenceResultsNlohmannJson_["framesPerSecond"] = std::to_string(1./inference_duration.count());
        inferenceBaseInferenceResultsNlohmannJson_["imageLocation"] = "__undefined__";
        inferenceBaseInferenceResultsNlohmannJson_["resultLocation"] = "__undefined__";

        message.inferenceDetailsMap_["response"] = inferenceBaseInferenceResultsNlohmannJson_;
        message.inferenceDetailsMap_["numInferencesDone"] = std::to_string(1);
        message.em_models_shape_ = input_tensor_shape;
        message.em_results_shape_ = output_tensor_shape;
        message.em_model_type_ = model_type;
        message.inferenceEMDetails_.push(outputDataVec);

        errc = INFERENCE_OK;
        start_timeout = std::chrono::steady_clock::now();
        completed = false;

        if(produce_output_)
        {
          output_inference_.produce_message(message);
          LOG_ALWAYS("[PIPELINE::GENERAL] Message Size = " + std::to_string(output_inference_.size()));
        }
      }
    }

  }
}
