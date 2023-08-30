/**
 * @edgemanager_client.cc
 * @brief Creating and Running Inference Client for Edge Manager
 *
 * This contains the function definitions for running inference using SageMaker EdgeManager API.
 * The API will initialize the clients, attach to the model ids and the servers and run inference.
 *
 */

#include <edge-ml-accelerator/inference/edgemanager_client.h>
#include <unistd.h>

namespace edgeml
{
  namespace inference
  {

    /**
      Instance of the class
    */
    EdgeManagerClient* EdgeManagerClient::instance(utils::jsonParser::jValue j, int inferIdx, std::string p, SharedMessage<MessageCaptureInference> & shared_map)
    {
      static EdgeManagerClient* inst = 0;

      if (!inst)
      {
        inst = new EdgeManagerClient(j, inferIdx, p, shared_map);
      }
      return inst;
    }

    /**
      Creates the class constructor
    */
    EdgeManagerClient::EdgeManagerClient(utils::jsonParser::jValue j, int inferIdx, std::string p, SharedMessage<MessageCaptureInference> & shared_map) : Inference(j, shared_map), pipelineName_(p)
    {
      LOG_ALWAYS("[INFERENCE::EdgeManagerClient] Inference mode is set to EdgeManager.");
      LOG_ALWAYS("[INFERENCE::EdgeManagerClient] Pipeline Name: " + pipelineName_);
      start_timeout = std::chrono::steady_clock::now();
      LOG_ALWAYS("[INFERENCE::EdgeManagerClient] Inference Name: " + jsonParams_["inference"][inferIdx]["inferName"].as_string());
      useGpio = jsonParams_["useGpio"].as_bool();
      scaleBy = jsonParams_["preprocess"]["scaleBy"].as_int();
      int ret = initInfer(inferIdx);

      if (ret == MODEL_FAILURE)
      {
        LOG_ALWAYS("[INFERENCE::EdgeManagerClient] Model failure. Cannot run Inference.");
      }

      inferenceBaseInferenceResultsNlohmannJson_["inferenceType"] = "edgemanager";
      inferenceBaseInferenceResultsNlohmannJson_["inferenceResults"] = customResultsNlohmannJson_;

    }

    /**
      Creates the class destructor
    */
    EdgeManagerClient::~EdgeManagerClient()
    {
      for (int i=0; i<numModels; i++)
      {
        UnLoadModel(i, model_name[i]);
      }
    }

    /**
      Find if a model is present or not
      @param id passing the stub id
      @param model_name passing the model name to check if its loaded or not
    */
    bool EdgeManagerClient::findModel(int id, const std::string& model_name)
    {
      AWS::SageMaker::Edge::ListModelsRequest request;
      AWS::SageMaker::Edge::ListModelsResponse reply;
      grpc::ClientContext context;
      grpc::Status status;
      status = stubs[id]->ListModels(&context, request, &reply);
      if (status.ok())
      {
        for (int i = 0; i < reply.models_size(); i++)
        {
          if (reply.models(i).name().c_str() == model_name) { return true; }
        }
      }
      return false;
    }

    /**
      Load the Model
      @param id passing the stub id
      @param model_path passing the path of the model
      @param model_name passing the model name
    */
    int EdgeManagerClient::LoadModel(int id, const std::string model_path, const std::string model_name)
    {
      AWS::SageMaker::Edge::LoadModelRequest request;
      AWS::SageMaker::Edge::LoadModelResponse reply;
      grpc::ClientContext context;
      grpc::Status status;
      try
      {
        request.set_url(model_path);
        request.set_name(model_name);
        status = stubs[id]->LoadModel(&context, request, &reply);
        if (status.ok())
        {
          LOG_ALWAYS("[INFERENCE::EdgeManagerClient::LoadModel] Model " + model_name + " located at " + model_path + " loaded");
          LOG_ALWAYS("[INFERENCE::EdgeManagerClient::LoadModel] LoadModel succeeded");
        }
        else
        {
          if (findModel(id, model_name))
          {
            LOG_ALWAYS("[INFERENCE::EdgeManagerClient::LoadModel] Alias " + model_name + " has been used");
          }
          LOG_ALWAYS("[INFERENCE::EdgeManagerClient::LoadModel] LoadModel failed");
          return MODEL_FAILURE;
        }
      }
      catch (std::exception& e)
      {
        LOG_ALWAYS("[INFERENCE::EdgeManagerClient::LoadModel] Exception: " + std::string(e.what()));
        return MODEL_FAILURE;
      }
      return MODEL_SUCCESS;
    }

    /**
      Un-Load the Model
      @param id passing the stub id
      @param model_path passing the path of the model
      @param model_name passing the model name
    */
    int EdgeManagerClient::UnLoadModel(int id, const std::string model_name)
    {
      AWS::SageMaker::Edge::UnLoadModelRequest request;
      AWS::SageMaker::Edge::UnLoadModelResponse reply;
      grpc::ClientContext context;
      grpc::Status status;
      try
      {
        request.set_name(model_name);
        status = stubs[id]->UnLoadModel(&context, request, &reply);
        if (status.ok())
        {
          LOG_ALWAYS("[INFERENCE::EdgeManagerClient::UnLoadModel] Model " + model_name + " has been unloaded");
          LOG_ALWAYS("[INFERENCE::EdgeManagerClient::UnLoadModel] UnLoadModel succeeded");
        }
        else
        {
          if (!findModel(id, model_name))
          {
            LOG_ALWAYS("[INFERENCE::EdgeManagerClient::UnLoadModel] Model " + model_name + " has not been loaded");
          }
          LOG_ALWAYS("[INFERENCE::EdgeManagerClient::UnLoadModel] UnLoadModel failed");
          return MODEL_FAILURE;
        }
      }
      catch (std::exception& e)
      {
        LOG_ALWAYS("[INFERENCE::EdgeManagerClient::UnLoadModel] Exception: " + std::string(e.what()));
        return MODEL_FAILURE;
      }
      return MODEL_SUCCESS;
    }

    /**
      Initializing the inference API with channels and stubs based on config
    */
    int EdgeManagerClient::initInfer(int inferIdx)
    {
      LOG_ALWAYS("[INFERENCE::EdgeManagerClient]");
      grpc_init();
      channel_arguments.SetMaxReceiveMessageSize(10000000); // Anything greater than 10000000 bytes will run into error
      channel = grpc::CreateCustomChannel("unix:///tmp/aws.greengrass.SageMakerEdgeManager.sock", grpc::InsecureChannelCredentials(), channel_arguments);
      numModels = jsonParams_["inference"][inferIdx]["model_ids"].size();
      for (int i=0; i<numModels; i++)
      {
        LOG_ALWAYS("[INFERENCE::EdgeManagerClient] Starting the client for model #" + std::to_string(i+1));
        model_name.push_back(jsonParams_["inference"][inferIdx]["model_ids"][i]["model_name"].as_string());
        model_path.push_back(jsonParams_["inference"][inferIdx]["model_ids"][i]["model_path"].as_string());
        model_type.push_back(jsonParams_["inference"][inferIdx]["model_ids"][i]["model_type"].as_string());
        stubs.push_back(AWS::SageMaker::Edge::Agent::NewStub(channel)); // Creating Inference Stub
        LOG_ALWAYS("[INFERENCE::EdgeManagerClient] model #" + std::to_string(i+1) + " Name = " + jsonParams_["inference"][inferIdx]["model_ids"][i]["model_name"].as_string());
        LOG_ALWAYS("[INFERENCE::EdgeManagerClient] model #" + std::to_string(i+1) + " Path = " + jsonParams_["inference"][inferIdx]["model_ids"][i]["model_path"].as_string());
        LOG_ALWAYS("[INFERENCE::EdgeManagerClient] Connecting to the server for model #" + std::to_string(i+1));
        grpc::ClientContext context;
        dmRequest.set_name(model_name[i]);
        grpc::Status status = stubs[i]->DescribeModel(&context, dmRequest, &dmResponse);

        if (status.ok())
        {
          LOG_ALWAYS("[INFERENCE::EdgeManagerClient] Model " + std::string(dmResponse.model().name()) + " has been loaded");
          LOG_ALWAYS("[INFERENCE::EdgeManagerClient] DescribeModel succeeded");
        }
        else
        {
          if (!findModel(i, model_name[i]))
          {
              LOG_ALWAYS("[INFERENCE::EdgeManagerClient] Model " + model_name[i] + " has not been loaded");
          }
          int ret = LoadModel(i, model_path[i], model_name[i]);
          if (ret == MODEL_FAILURE)
            return INFERENCE_ERROR;
        }

        grpc::ClientContext contextUpdate;
        dmRequest.set_name(model_name[i]);
        grpc::Status statusUpdate = stubs[i]->DescribeModel(&contextUpdate, dmRequest, &dmResponse);

        // Print the Input Tensor Details and store in a vector for future access
        std::vector<AWS::SageMaker::Edge::TensorMetadata> modelinputdetails(dmResponse.model().input_tensor_metadatas().begin(), dmResponse.model().input_tensor_metadatas().end());
        input_tensor_data_type.push_back(modelinputdetails[0].data_type());
        std::vector<int> modelinputshape(modelinputdetails[0].shape().begin(), modelinputdetails[0].shape().end());
        input_tensor_names.push_back(modelinputdetails[0].name().c_str());
        input_tensor_shape.push_back(modelinputshape);
        LOG_ALWAYS("[INFERENCE::EdgeManagerClient::LoadModel] Model Details = (Input Tensor): [" + input_tensor_names[input_tensor_names.size()-1] + "] (");
        for (int j=0; j<modelinputshape.size()-1; j++)
        {
          LOG_ALWAYS(modelinputshape[j] + ", ");
        }
        LOG_ALWAYS(modelinputshape[modelinputshape.size()-1] + ") (" + std::string(EdgeManagerModelDataTypeE[input_tensor_data_type[input_tensor_data_type.size()-1]]) + ")");

        // Print the Output Tensor Details and store in a vector for future access
        std::vector<AWS::SageMaker::Edge::TensorMetadata> modeloutputdetails(dmResponse.model().output_tensor_metadatas().begin(), dmResponse.model().output_tensor_metadatas().end());
        output_tensor_data_type.push_back(modeloutputdetails[0].data_type());
        std::vector<int> modeloutputshape(modeloutputdetails[0].shape().begin(), modeloutputdetails[0].shape().end());
        output_tensor_names.push_back(modeloutputdetails[0].name().c_str());
        output_tensor_shape.push_back(modeloutputshape);
        LOG_ALWAYS("[INFERENCE::EdgeManagerClient::LoadModel] Model Details = (Output Tensor): [" + output_tensor_names[output_tensor_names.size()-1] + "] (");
        for (int j=0; j<modeloutputshape.size()-1; j++)
        {
          LOG_ALWAYS(modeloutputshape[j] + ", ");
        }
        LOG_ALWAYS(modeloutputshape[modeloutputshape.size()-1] + ") (" + std::string(EdgeManagerModelDataTypeE[output_tensor_data_type[output_tensor_data_type.size()-1]]) + ")");

        predRequestVec.push_back(predRequest);
        predRequestVec[i].set_name(model_name[i]);
        predResponseVec.push_back(predResponse);
        cdRequestVec.push_back(cdRequest);
        cdRequestVec[i].set_model_name(model_name[i]);

        tensormetadataVec.push_back(tensormetadata);
        tensormetadataVec[i].set_name(input_tensor_names[i]);
        tensorVec.push_back(tensor);
        long int indatasize = 1;
        for (int j = 0; j < input_tensor_shape[i].size(); ++j)
        {
          tensormetadataVec[i].add_shape(input_tensor_shape[i][j]);
          indatasize *= input_tensor_shape[i][j];
        }
        unsigned char chardata[indatasize];
        input_tensor_size.push_back(indatasize);
        input_height.push_back(input_tensor_shape[i][1]); input_width.push_back(input_tensor_shape[i][2]); input_channels.push_back(input_tensor_shape[i][3]);
        tensormetadataVec[i].set_data_type(input_tensor_data_type[i]);
        tensorVec[i].mutable_tensor_metadata()->CopyFrom(tensormetadataVec[i]);
        std::string sp(reinterpret_cast<char*>(chardata), indatasize);
        tensorVec[i].set_byte_data(sp);
        predRequestVec[i].add_tensors();
        predRequestVec[i].mutable_tensors(0)->CopyFrom(tensorVec[i]);
        std::vector<unsigned char> tmpInputVec(indatasize, 0);
        inputDataVec.push_back(tmpInputVec);

        outputTensorVec.push_back(outputTensor);
        outputTensorVec[i] = cdRequest.add_output_tensors();
        long int outdatasize = 1;
        for (int j = 0; j < output_tensor_shape[i].size(); ++j)
        {
          outdatasize *= output_tensor_shape[i][j];
        }
        output_tensor_size.push_back(outdatasize);
        output_height.push_back(output_tensor_shape[i][1]); output_width.push_back(output_tensor_shape[i][2]); output_channels.push_back(output_tensor_shape[i][3]);
        std::vector<float> tmpOutputVec(output_tensor_size[i], 0);
        outputDataVec.push_back(tmpOutputVec);

        ret = INFERENCE_OK;
      }

      // Warm-up model for 5 inferences
      for (int iter=0; iter<5; iter++)
      {
        LOG_ALWAYS("[INFERENCE::EdgeManagerClient] Model Warmup : Running Dummy Inference for #" + std::to_string(iter+1) + "/5");
        for (int i=0; i<numModels; i++)
        {
          float chardata[input_tensor_size[i]];
          std::string sp(reinterpret_cast<char*>(chardata), input_tensor_size[i]);
          tensorVec[i].set_byte_data(sp);
          predRequestVec[i].mutable_tensors(0)->CopyFrom(tensorVec[i]);

          grpc::ClientContext context;
          grpc::Status status = stubs[i]->Predict(&context, predRequestVec[i], &predResponseVec[i]);
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
    void EdgeManagerClient::runInference(int& errc, int& height, int& width, int& iter, bool& completed)
    {
      while(1)
      {
        auto message = camera2ongoing_.GetMessage();
        LOG_ALWAYS("[INFERENCE::EdgeManagerClient] Capture Trigger Message = " + message.captureTrigger_.captureTriggersMessage_);
        message.inferenceMode_ = InferenceInputModeE::EDGEMANAGER;

        LOG_ALWAYS("[INFERENCE::EdgeManagerClient]");

        inference_start_time = std::chrono::steady_clock::now();

        unsigned char* inputImage = message.safeCaptureContainer_.front();
        LOG_ALWAYS("[INFERENCE::EdgeManagerClient] Inference started for model(s)");
        for (int i=0; i<numModels; i++)
        {
          ret = imagePreprocess.resize(inputImage, width, height, inputDataVec[i].data(), input_width[i], input_height[i], input_channels[i]);
          inputScaled = imagePreprocess.scale(inputDataVec[i], input_width[i], input_height[i], input_channels[i], scaleBy);

          std::string sp(reinterpret_cast<char*>(inputScaled.data()), input_tensor_size[i]);
          tensorVec[i].set_byte_data(sp);
          predRequestVec[i].mutable_tensors(0)->CopyFrom(tensorVec[i]);

          grpc::ClientContext context;
          grpc::Status status = stubs[i]->Predict(&context, predRequestVec[i], &predResponseVec[i]);

          if (model_type[i]=="classification" || model_type[i]=="objectdetection" || model_type[i]=="segmentation" || model_type[i]=="undefined" || model_type[i]=="none")
          {
            outputTensorVec[i]->CopyFrom(predResponseVec[i].tensors(0));
            AWS::SageMaker::Edge::TensorMetadata tmpmetadata = outputTensorVec[i]->tensor_metadata();
            const std::string tmpstr = outputTensorVec[i]->byte_data();
            const float* values = reinterpret_cast<const float*>(tmpstr.c_str());
            std::memcpy(outputDataVec[i].data(), values, sizeof(float)*output_tensor_size[i]); // [TODO] Reduce usage of memcpy if possible
            std::transform(model_type[i].begin(), model_type[i].end(), model_type[i].begin(), std::ptr_fun<int, int>(std::toupper));
            LOG_ALWAYS("[INFERENCE::EdgeManagerClient] Inference results for [" + model_name[i] + "] is completed");
            std::transform(model_type[i].begin(), model_type[i].end(), model_type[i].begin(), std::ptr_fun<int, int>(std::tolower));
          }
          else
          {
            LOG_ALWAYS("[INFERENCE::EdgeManagerClient] Model type not correctly found. Should be one of: {classification OR objectdetection OR segmentation OR undefined OR none}");
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
