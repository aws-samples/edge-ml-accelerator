/**
 * @local_disk.cc
 * @brief Saving files locally
 *
 * This contains the function definitions for saving image/csv files in the local disk.
 *
 */

#include <edge-ml-accelerator/output/local_disk.h>
#include <unistd.h>

namespace edgeml
{
  namespace output
  {

    /**
      Instance of the class
    */
    LocalDisk *LocalDisk::instance(utils::jsonParser::jValue j, SharedMessage<MessageCaptureInference> &incoming_message)
    {
      static LocalDisk *inst = 0;

      if (!inst)
      {
        inst = new LocalDisk(j, incoming_message);
      }
      return inst;
    }

    /**
      Creates the class constructor
    */
    LocalDisk::LocalDisk(utils::jsonParser::jValue j, SharedMessage<MessageCaptureInference> &incoming_message) : Output(j, incoming_message), jsonParams_(j)
    {
      start_timeout = std::chrono::steady_clock::now();
      LOG_ALWAYS("[OUTPUT::LOCALDISK] Images to be saved locally");

      imageFormat_ = getImageFormat();
      colorSpace_ = getColorSpace();
      if (imageFormat_ == "jpg" || imageFormat_ == "jpeg")
      {
        LOG_ALWAYS("[OUTPUT::LOCALDISK] Trying to save image as JPG");
      }
      else if (imageFormat_ == "png")
      {
        LOG_ALWAYS("[OUTPUT::LOCALDISK] Trying to save image as PNG");
      }
      else
      {
        LOG_ALWAYS("[OUTPUT::LOCALDISK] Not able to determine imageFormat");
      }

      if (!std::experimental::filesystem::exists(savePath))
      {
        std::experimental::filesystem::create_directories(savePath);
        LOG_ALWAYS("[OUTPUT::LOCALDISK] Creating directory to save output");
      }
      LOG_ALWAYS("[OUTPUT::LOCALDISK] Saving output path: " + savePath);

      savePathCapture = savePath + "/capture";
      if (!std::experimental::filesystem::exists(savePathCapture))
      {
        std::experimental::filesystem::create_directories(savePathCapture);
        LOG_ALWAYS("[OUTPUT::LOCALDISK] Creating directory to save capture output: " + savePathCapture);
      }
      LOG_ALWAYS("[OUTPUT::LOCALDISK] Saving Captures: " + savePathCapture);

      savePathInference = savePath + "/inference";
      if (!std::experimental::filesystem::exists(savePathInference))
      {
        std::experimental::filesystem::create_directories(savePathInference);
        LOG_ALWAYS("[OUTPUT::LOCALDISK] Creating Inferences: " + savePathInference);
      }
      LOG_ALWAYS("[OUTPUT::LOCALDISK] Saving Captures: " + savePathInference);

      savePathInferenceImages = savePath + "/inference/images";
      if (!std::experimental::filesystem::exists(savePathInferenceImages))
      {
        std::experimental::filesystem::create_directories(savePathInferenceImages);
        LOG_ALWAYS("[OUTPUT::LOCALDISK] Creating directory to save inference output images: " + savePathInferenceImages);
      }
      LOG_ALWAYS("[OUTPUT::LOCALDISK] Saving Inferences Images: " + savePathInferenceImages);

      savePathInferenceResults = savePath + "/inference/masks";
      if (!std::experimental::filesystem::exists(savePathInferenceResults))
      {
        std::experimental::filesystem::create_directories(savePathInferenceResults);
        LOG_ALWAYS("[OUTPUT::LOCALDISK] Creating directory to save inference output results: " + savePathInferenceResults);
      }

      savePathInferenceResultsJson_ = savePath + "/inference/json";
      if (!std::experimental::filesystem::exists(savePathInferenceResultsJson_))
      {
        std::experimental::filesystem::create_directories(savePathInferenceResultsJson_);
        LOG_ALWAYS("[OUTPUT::LOCALDISK] Creating directory to save inference output results: " + savePathInferenceResultsJson_);
      }
      LOG_ALWAYS("[OUTPUT::LOCALDISK] Saving Inferences Results : " + savePathInferenceResults);
    }

    /**
      Creates the class destructor
    */
    LocalDisk::~LocalDisk()
    {
    }

    /**
      Routine to save image data as a PNG or JPG file
      @param errc returning the error code of capture API
      @param height the height of the image to be saved
      @param width the widht of the image to be saved
    */
    void LocalDisk::saveImageAsPNGorJPG(int &errc, int &height, int &width, bool &completed)
    {
      using namespace utils;
      int img_c = 3;
      while (1)
      {
        bool data_available = false;
        auto message = incoming_message_.GetMessage();
        LOG_ALWAYS("[OUTPUT::LOCALDISK] Incoming Message = " + std::to_string(incoming_message_.size()));

          LOG_ALWAYS("----- LOCAL DISK -----");
          LOG_ALWAYS("----------------------");

          // saving as cameraname-infername to determine output in filename
          std::string outFileName = "";
          if (message.captureTrigger_.captureTriggersMessage_ == "live")
          {
            outFileName = "live-view";
            outFileName += "-" + message.cameraName_;
            outFileName.append("." + imageFormat_);
            filesSavePath = "/dev/shm/" + outFileName;
          }
          else if (inList(message.captureTrigger_.captureTriggersMessage_,{"capture", "capture_"}))
          {
            if (outFileName == "")
            {
              outFileName = getOutputFileName();
              outFileName += "-" + message.cameraName_;
              outFileName += "-" + message.captureTrigger_.captureTriggersMessage_;
            }
            outFileName.append("." + imageFormat_);
            filesSavePath = savePathCapture + "/" + outFileName;
          }
          else
          {
            if (outFileName == "")
            {
              outFileName = getOutputFileName();
              outFileName += "-" + message.cameraName_;
              outFileName += "-" + message.captureTrigger_.captureTriggersMessage_;
            }
            outFileName.append("." + imageFormat_);
            filesSavePath = savePathInferenceImages + "/" + outFileName;
          }
          filesSavePathResults = "";

          imageData = message.safeCaptureContainer_.front();

          ret = imagePreprocess.write((const char *)filesSavePath.c_str(), width, height, img_c, (const void *)imageData, colorSpace_);

          LOG_ALWAYS("[OUTPUT::LOCALDISK] Saving the image as: " + filesSavePath + " of size [H,W] = [" + std::to_string(height) + "," + std::to_string(width) + "]");

          message.image_file_names_ = filesSavePath;
          message.inferenceDetailsMap_["response"]["imageLocation"] = filesSavePath;

          if ((message.captureTrigger_.captureTriggersType_ == "ipc") && (message.captureTrigger_.captureTriggersMessage_ == "capture"))
          {
            LOG_ALWAYS("[OUTPUT::LOCALDISK] Triggered by capture");
          }
          else if ((message.captureTrigger_.captureTriggersType_ == "ipc") && (message.captureTrigger_.captureTriggersMessage_ == "live"))
          {
            LOG_ALWAYS("[OUTPUT::LOCALDISK] Triggered by live");
          }
          else if (message.captureTrigger_.captureTriggersType_ == "ipc" and inList(message.captureTrigger_.captureTriggersMessage_,{"capture_"}))
          {
            LOG_ALWAYS("[OUTPUT::LOCALDISK] Triggered by capture of view");
          }
          else
          {
            // Save inference results for SageMaker EdgeManager model
            if (message.inferenceMode_ == InferenceInputModeE::EDGEMANAGER || message.inferenceMode_ == InferenceInputModeE::ONNX)
            {
              std::vector<std::vector<float>> outputDataVec = message.inferenceEMDetails_.front();
              for (int i = 0; i < message.em_model_type_.size(); ++i)
              {
                if (message.em_model_type_[i] == "classification" || message.em_model_type_[i] == "objectdetection" || message.em_model_type_[i] == "undefined" || message.em_model_type_[i] == "none")
                {
                  std::ofstream myfile;
                  filesSavePathResults = filesSavePath.replace(filesSavePath.find("." + imageFormat_), 4, std::string("-CLASSIFICATION.json"));
                  filesSavePathResults = filesSavePathResults.replace(savePathInferenceImages.find(savePathInferenceImages), savePathInferenceImages.length(), savePathInferenceResultsJson_);
                  LOG_ALWAYS("[OUTPUT::LOCALDISK] Saving inference results for " + std::string(ModelTypesE[message.inferenceMode_]) + " " + message.em_model_type_[i] + " to " + filesSavePathResults);
                  message.inferenceDetailsMap_["response"]["resultLocation"] = filesSavePathResults;
                  myfile.open(filesSavePathResults.c_str());
                  myfile << message.inferenceDetailsMap_["response"].dump();
                  myfile.close();
                }
                else if (message.em_model_type_[i] == "segmentation")
                {
                  filesSavePathResults = filesSavePath.replace(filesSavePath.find("." + imageFormat_), 4, std::string("-SEGMENTATION." + imageFormat_));
                  filesSavePathResults = filesSavePathResults.replace(savePathInferenceImages.find(savePathInferenceImages), savePathInferenceImages.length(), savePathInferenceResults);
                  LOG_ALWAYS("[OUTPUT::LOCALDISK] Saving inference results for " + std::string(ModelTypesE[message.inferenceMode_]) + " " + message.em_model_type_[i] + " to " + filesSavePathResults);
                  message.inferenceDetailsMap_["response"]["resultLocation"] = filesSavePathResults;
                  for (int channel=0; channel<message.em_results_shape_[i][3]; channel++)
                  {
                    std::string filesSavePathResultsChannel = filesSavePathResults.replace(filesSavePathResults.find("-SEGMENTATION"), -1, std::string("-SEGMENTATION-CLASS-" + std::to_string(channel) + "." + imageFormat_));
                    ret = imagePreprocess.write((const char *)filesSavePathResultsChannel.c_str(), message.em_results_shape_[i][2], message.em_results_shape_[i][1], 1, outputDataVec[i].data()+message.em_results_shape_[i][2]*message.em_results_shape_[i][1]*channel, colorSpace_);
                  }
                }
              }
            }
            // Save inference results for Lookout for vision model
            else if (message.inferenceMode_ == InferenceInputModeE::LFVE)
            {
              std::ofstream myfile;
              filesSavePathResults = filesSavePath.replace(filesSavePath.find("." + imageFormat_), 4, std::string("-GENERAL.json"));
              filesSavePathResults = filesSavePathResults.replace(savePathInferenceImages.find(savePathInferenceImages), savePathInferenceImages.length(), savePathInferenceResultsJson_);
              LOG_ALWAYS("[OUTPUT::LOCALDISK] Saving inference results for LFVE Anomaly/Normal to " + filesSavePathResults);
              message.inferenceDetailsMap_["response"]["resultLocation"] = filesSavePathResults;
              myfile.open(filesSavePathResults.c_str());
              myfile << message.inferenceDetailsMap_["response"].dump();
              myfile.close();

              // // TODO
              // filesSavePathResults = filesSavePathResults.replace(filesSavePathResults.find(".json"), 5, std::string("." + imageFormat_));
              // ret = imagePreprocess.write((const char *)filesSavePathResults.c_str(), width, height, img_c, (const void *)outputVecLFVE, colorSpace_);
            }
            else if(message.inferenceMode_ != InferenceInputModeE::TRITON)
            {
              std::ofstream myfile;
              filesSavePathResults = filesSavePath.replace(filesSavePath.find("." + imageFormat_), 4, std::string("-GENERAL.json"));
              filesSavePathResults = filesSavePathResults.replace(savePathInferenceImages.find(savePathInferenceImages), savePathInferenceImages.length(), savePathInferenceResultsJson_);
              LOG_ALWAYS("[OUTPUT::LOCALDISK] Saving inference results to " + filesSavePathResults);
              message.inferenceDetailsMap_["response"]["resultLocation"] = filesSavePathResults;
              myfile.open(filesSavePathResults.c_str());
              myfile << message.inferenceDetailsMap_["response"].dump();
              myfile.close();
            }
            else if(message.inferenceMode_ != InferenceInputModeE::NONE)
            {
              std::vector<std::vector<float>> outputDataVec = message.inferenceEMDetails_.front();
              std::ofstream myfile;
              filesSavePathResults = filesSavePath.replace(filesSavePath.find("." + imageFormat_), 4, std::string("-GENERAL.json"));
              filesSavePathResults = filesSavePathResults.replace(savePathInferenceImages.find(savePathInferenceImages), savePathInferenceImages.length(), savePathInferenceResultsJson_);
              LOG_ALWAYS("[OUTPUT::LOCALDISK] Saving inference results to " + filesSavePathResults);
              message.inferenceDetailsMap_["response"]["resultLocation"] = filesSavePathResults;
              myfile.open(filesSavePathResults.c_str());
              myfile << message.inferenceDetailsMap_["response"].dump();
              myfile.close();
            }
          }
          message.result_file_names_ = filesSavePathResults;

          errc = LOCALSAVE_SUCCESS;
          start_timeout = std::chrono::steady_clock::now();
          completed = false;

        if(produce_output_)
        {
          output_message_.produce_message(message);
          LOG_ALWAYS("[PIPELINE::GENERAL] Message Size = " + std::to_string(output_message_.size()));
        }
      }
    }

  }
}
