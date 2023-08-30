/**
 * @triton_client.cc
 * @brief Creating and Running Inference Client
 *
 * This contains the function definitions for running inference using Triton Runtime API.
 * The API will initialize the clients, attach to the model ids and the servers and run inference.
 *
 */

#include <edge-ml-accelerator/inference/triton_client.h>
#include <unistd.h>

namespace triton_inf = inference;

#define FAIL_IF_ERR(X, MSG)                                                                 \
  {                                                                                         \
    tc::Error err = (X);                                                                    \
    if (!err.IsOk()) {                                                                      \
      LOG_ERROR("[INFERENCE::TritonClient] error: " + std::string(MSG) + ": " + std::string(err.Message()));  \
      exit(1);                                                                              \
    }                                                                                       \
  }

namespace edgeml
{
  namespace inference
  {



    /**
      Instance of the class
    */
    TritonClient* TritonClient::instance(utils::jsonParser::jValue j, int inferIdx, std::string p, SharedMessage<MessageCaptureInference> & shared_map)
    {
      static TritonClient* inst = 0;

      if (!inst)
      {
        inst = new TritonClient(j, inferIdx, p, shared_map);
      }
      return inst;
    }

    /**
      Creates the class constructor
    */
    TritonClient::TritonClient(utils::jsonParser::jValue j, int inferIdx, std::string p, SharedMessage<MessageCaptureInference> & shared_map) : Inference(j, shared_map), pipelineName_(p)
    {
        LOG_ALWAYS("[INFERENCE::TritonClient] Inference mode is set to TritonClient.");
        LOG_ALWAYS("[INFERENCE::TritonClient] Pipeline Name: " + pipelineName_);
        start_timeout = std::chrono::steady_clock::now();
        model_name_ = jsonParams_["inference"][inferIdx]["modelName"].as_string();
        metadata_ = jsonParams_["inference"][inferIdx]["metadata"].as_string();
        useGpio = jsonParams_["useGpio"].as_bool();

        inferenceBaseInferenceResultsNlohmannJson_["inferenceType"] = "triton";
        inferenceBaseInferenceResultsNlohmannJson_["inferenceResults"] = customResultsNlohmannJson_;

        initInfer("ensemble");
    }

    int TritonClient::initInfer(std::string model_name)
    {
        model_name_ = model_name;
        triton_inf::ModelConfigResponse model_config;
        std::string url("localhost:8001");
        auto err = tc::InferenceServerGrpcClient::Create(&grpc_client_, url, false);
        // err = grpc_client_->ModelConfig(&model_config, model_name);


        // if (!err.IsOk())
        // {
        //     LOG_ERROR("error: failed to get model config: " + err);
        // }

        batch_size_ = 0;//model_config.config().max_batch_size();

        // Set the number of classification expected
        err = tc::InferRequestedOutput::Create(&output_, "OUTPUT");
        if (!err.IsOk()) {
            LOG_ERROR("[INFERENCE::TritonClient] Unable to get output: " + std::string(err.Message()));
            exit(1);
        }

        return 0;
    }


    void TritonClient::runInference(int& errc, int& height, int& width, int& iter, bool& completed)
    {
    //   int ret = initInfer(inferIdx_);
        LOG_ALWAYS("[INFERENCE::TritonClient] Inference Pipeline Ready is ready");

        std::vector<int64_t> shape{(int64_t)height, (int64_t)width, 3};

        auto err = tc::InferInput::Create(&input_, "INPUT", shape, "UINT8");
        if (!err.IsOk()) {
            LOG_ERROR("[INFERENCE::TritonClient] Unable to get input: " + std::string(err.Message()));
            exit(1);
        }


        std::vector<std::string> path(metadata_.length());
        for(auto k1 = 0; k1 < metadata_.length(); k1++)
        {
            path[k1] = metadata_[k1];
        }

        std::vector<int64_t> path_length{(int64_t)path.size()};
        auto errpath = tc::InferInput::Create(&input_path_, "INPUT2", path_length, "BYTES");
        if (!errpath.IsOk()) {
            LOG_ERROR("[INFERENCE::TritonClient] Unable to get input: " + std::string(errpath.Message()));
            exit(1);
        }

        int local_iter = 0;
        std::shared_ptr<tc::InferInput> input_ptr(input_);
        std::shared_ptr<tc::InferInput> input_ptr_path(input_path_);
        std::shared_ptr<tc::InferRequestedOutput> output_ptr(output_);
        std::vector<tc::InferInput*> inputs = {input_ptr.get(), input_ptr_path.get()};
        std::vector<const tc::InferRequestedOutput*> outputs = {output_ptr.get()};
        tc::InferOptions options(model_name_);
        // options.model_version_ = "-1";
        tc::InferResult* results;
        std::string output_name("OUTPUT");



        while(1)
        {
            auto err = input_ptr->Reset();
            input_ptr_path->Reset();

            if (!err.IsOk()) {
                LOG_ERROR("failed resetting input: " + std::string(err.Message()));
                exit(1);
            }

            auto message = camera2ongoing_.GetMessage();
            LOG_ALWAYS("[INFERENCE::TritonClient] " + message.captureTrigger_.captureTriggersMessage_);
            message.inferenceMode_ = InferenceInputModeE::TRITON;
            LOG_ALWAYS("[INFERENCE::TritonClient]");

            auto inputImage = message.safeCaptureContainer_.front();

            std::vector<uint8_t> image_data(inputImage, inputImage + (height * width) * 3);

            if (!err.IsOk())
            {
                LOG_ERROR("[INFERENCE::TritonClient] Failed setting input: " + std::string(err.Message()));
                exit(1);
            }


            input_ptr->AppendRaw(image_data);
            FAIL_IF_ERR(input_ptr_path->AppendFromString(path),"Error in Triton");

            inference_start_time = std::chrono::steady_clock::now();

            grpc_client_->Infer(&results, options, inputs, outputs);

            if (!results->RequestStatus().IsOk())
            {
              auto err = results->RequestStatus();
              LOG_ERROR("[INFERENCE::TritonClient] Inference  failed with error: " + std::string(err.Message()));
              exit(1);
            }

            std::vector<std::string> result_data;
            auto err2 = results->StringData(output_name, &result_data);
            LOG_ALWAYS(std::string(result_data[0]));

            inference_end_time = std::chrono::steady_clock::now();
            inference_duration = std::chrono::duration_cast<std::chrono::milliseconds>(inference_end_time - inference_start_time);

            customResultsNlohmannJson_["results"] = result_data[0];

            inferenceBaseInferenceResultsNlohmannJson_["inferenceResults"] = customResultsNlohmannJson_;
            inferenceBaseInferenceResultsNlohmannJson_["inferenceTime"] = std::to_string(inference_duration.count());
            inferenceBaseInferenceResultsNlohmannJson_["framesPerSecond"] = std::to_string(1./inference_duration.count());
            inferenceBaseInferenceResultsNlohmannJson_["imageLocation"] = "__undefined__";
            inferenceBaseInferenceResultsNlohmannJson_["resultLocation"] = "__undefined__";

            message.inferenceDetailsMap_["response"] = inferenceBaseInferenceResultsNlohmannJson_;
            message.inferenceDetailsMap_["numInferencesDone"] = std::to_string(1);
            // // TODO
            // message.inferenceEMDetails_.push(result_data[0]);

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

    /**
      Creates the class destructor
    */
    TritonClient::~TritonClient()
    {
    }

  }
}