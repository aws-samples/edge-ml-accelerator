/**
 * @pylon_capture.cc
 * @brief Creating and Running Capture
 *
 * This contains the function definitions for creating and running capture using Pylon for camera.
 *
 */

#include <edge-ml-accelerator/capture/pylon_capture.h>
#include <unistd.h>

namespace edgeml
{
  namespace capture
  {

    /**
      Instance of the class
    */
    PylonCapture* PylonCapture::instance(utils::jsonParser::jValue j, int cameraIndex, SharedMessage<MessageT2C> &trigger2camera)
    {
      static PylonCapture* inst = 0;

      if (!inst)
      {
        inst = new PylonCapture(j, cameraIndex, trigger2camera);
      }
      return inst;
    }

    /**
      Creates the class constructor
    */
    PylonCapture::PylonCapture(utils::jsonParser::jValue j, int cameraIndex, SharedMessage<MessageT2C> &trigger2camera) : Capture(j, cameraIndex, trigger2camera)
    {
      LOG_ALWAYS("[CAPTURE::PYLON] Capture mode is set to camera.");
      controlOrMonitor_ = getCameraOperatingMode();
      exposureTime_ = getExposureTime();
      gainValue_ = getGainValue();
    }

    /**
      Creates the class destructor
    */
    PylonCapture::~PylonCapture()
    {
    }

    /**
      Initializing a Pylon capture object for camera ID
      @return error-code showing if the capture object was created or not
    */
    int PylonCapture::initCapture(int cameraIndex)
    {
      inferenceDetailsBlank = getInferenceDetailsJson();
      fc_.OutputPixelFormat = Pylon::PixelType_RGB8packed;
      info_.SetDeviceClass(Pylon::BaslerGigEDeviceClass);
      Pylon::PylonInitialize();
      camera_.Attach(Pylon::CTlFactory::GetInstance().CreateFirstDevice(info_));

      // if (false) // Use this for Camera Hardware Trigger
      // {
      //   hwTriggerDelay_ = getHardwareTriggerDelay();
      //   camera_.TriggerSelector.SetValue(Basler_UniversalCameraParams::TriggerSelector_FrameStart);
      //   camera_.TriggerActivation.SetValue(Basler_UniversalCameraParams::TriggerActivation_RisingEdge);
      //   camera_.TriggerDelayAbs.SetValue(hwTriggerDelay_);
      // }

      if (controlOrMonitor_==1)
      {
        LOG_ALWAYS("[CAPTURE::PYLON] CONTROL mode is ON.");
        setControlMode();
        camera_.Open();
        camera_.GetStreamGrabberParams().TransmissionType = Basler_UniversalStreamParams::TransmissionType_Multicast;
        camera_.PixelFormat.SetValue(Basler_UniversalCameraParams::PixelFormat_BayerRG8);

        GenApi::CFloatPtr exposure(camera_.GetNodeMap().GetNode("ExposureTimeAbs")); // set exposure time in microseconds
        exposure->SetValue((float)(exposureTime_));
        GenApi::CIntegerPtr gainvalue(camera_.GetNodeMap().GetNode("GainRaw")); // set gain unit values
        gainvalue->SetValue((int)(gainValue_));
        GenApi::CIntegerPtr(camera_.GetNodeMap().GetNode("Height"))->SetValue(getInputHeight());
        GenApi::CIntegerPtr(camera_.GetNodeMap().GetNode("Width"))->SetValue(getInputWidth());
        camera_.StartGrabbing();
      }
      else
      {
        LOG_ALWAYS("[CAPTURE::PYLON] MONITOR mode is ON.");
        setMonitorMode();
        camera_.RegisterConfiguration((Pylon::CConfigurationEventHandler*) NULL, Pylon::RegistrationMode_ReplaceAll, Pylon::Cleanup_None);
        camera_.MonitorModeActive = true;
        camera_.Open();
        camera_.GetStreamGrabberParams().TransmissionType = Basler_UniversalStreamParams::TransmissionType_UseCameraConfig;
        if (camera_.GetStreamGrabberParams().DestinationAddr.GetValue() != "0.0.0.0" && camera_.GetStreamGrabberParams().DestinationPort.GetValue() != 0)
        {
            camera_.StartGrabbing();
        }
        else
        {
            LOG_ERROR("[CAPTURE::PYLON] Failed to open stream grabber (monitor mode): The acquisition is not yet started by the controlling application.");
            return CAMERA_MISSING;
        }
      }

      GenApi::CIntegerPtr height(camera_.GetNodeMap().GetNode("Height"));
      GenApi::CIntegerPtr width(camera_.GetNodeMap().GetNode("Width"));

      LOG_ALWAYS("[CAPTURE::PYLON] Using device " + camera_.GetDeviceInfo().GetModelName() + " with [H,W] = [" + std::to_string((int)(height->GetValue())) + "," + std::to_string((int)(width->GetValue())) + "]");

      setInputHeight((int)(height->GetValue()));
      setInputWidth((int)(width->GetValue()));

      GenApi::CFloatPtr exposureTimeCurr(camera_.GetNodeMap().GetNode("ExposureTimeAbs"));
      GenApi::CIntegerPtr gainCurr(camera_.GetNodeMap().GetNode("GainRaw"));
      LOG_ALWAYS("[CAPTURE::PYLON] Camera Settings:");
      LOG_ALWAYS("[CAPTURE::PYLON]  - ExposureTimeAbs = " + std::to_string((float)(exposureTimeCurr->GetValue())));
      LOG_ALWAYS("[CAPTURE::PYLON]  - GainRaw = " + std::to_string((int)(gainCurr->GetValue())));

      return CAPTURE_OK;
    }

    /**
      Setting to control mode for multicast
    */
    void PylonCapture::setControlMode()
    {
      controlMode_ = true;
      monitorMode_ = false;
    }

    /**
      Setting to monitor mode for multicast
    */
    void PylonCapture::setMonitorMode()
    {
      monitorMode_ = true;
      controlMode_ = false;
    }

    /**
      Getting a capture based image file getting as a bytes data. This API will be called in order to get the capture data.
      @param errc returning the error code of capture API
      @param frameData passing by reference to get the frame data as unsigned char array
      @param frameDataSize passing by reference to get the size of the frame data in the form of HxWxC
      @param iter running for total iterations if > 0 else running infinite loop
    */
    void PylonCapture::getCapture(int& errc, unsigned char*& frameData, int& frameDataSize, int& iter)
    {
      getCaptureAtIndex(errc, frameData, frameDataSize, iter, 0);
    }

    void PylonCapture::getCaptureAtIndex(int& errc, unsigned char*& frameData, int& frameDataSize, int& iter, int cameraIndex)
    {
      int currIter = 0;
      while(1)
      {
        if (iter>0)
        {
          if (currIter>iter)
          {
            break;
          }
        }
        currIter++;

        // Wait for message
        auto message = trigger2camera_.GetMessage();
        LOG_ALWAYS("[CAPTURE::PYLON] Trigger to Camera Message = " + message.captureTriggersMessage_);

        message.captureTriggersMessageFull_["captureID"] = "#" + std::to_string(currIter);

        // Create a message forward with local scope
        MessageCaptureInference forward_message;
        forward_message.captureTrigger_ = message;
        forward_message.cameraName_ = cameraName_;

        capture_start_time_ = std::chrono::steady_clock::now();

        // check if it's an active pipeline
        if(!camera2forward_.find(forward_message.captureTrigger_.captureTriggersMessage_))
        {
          LOG_ALWAYS("[CAPTURE::GENICAM] Command Pipeline NOT available ");
          continue;
        }

        if (message.captureTriggersMessage_ == "configchange")
        {
          LOG_ALWAYS("[CAPTURE::PYLON] Changing Config based on following: ");
          LOG_ALWAYS(message.captureTriggersMessageFull.front().to_string());

          if (message.captureTriggersMessageFull_["exposureTime"].as_double()>=10)
          {
            exposureTime_ = message.captureTriggersMessageFull_["exposureTime"].as_double();
            LOG_ALWAYS("[CAPTURE::PYLON] Changing exposureTime_ = " + std::to_string(exposureTime_));
            GenApi::CFloatPtr exposure(camera_.GetNodeMap().GetNode("ExposureTime")); // set exposure time in microseconds
            exposure->SetValue((float)(exposureTime_));
          }
          if (message.captureTriggersMessageFull_["gainValue"].as_int()>=0)
          {
            gainValue_ = message.captureTriggersMessageFull_["gainValue"].as_int();
            LOG_ALWAYS("[CAPTURE::PYLON] Changing gainValue_ = " + std::to_string(gainValue_));
            GenApi::CIntegerPtr gainvalue(camera_.GetNodeMap().GetNode("GainRaw")); // set gain unit values
            gainvalue->SetValue((int)(gainValue_));
          }
          continue;
        }

        int errc = runCapture(frameData, frameDataSize);
        if (errc==0)
        {
          forward_message.safeCaptureContainer_.push(frameData);
          forward_message.safeCaptureSizeContainer_.push(frameDataSize);

          LOG_ALWAYS("[CAPTURE::PYLON] Trigger: " + message.captureTriggersMessage_);

          inferenceDetailsFilled = inferenceDetailsBlank;
          inferenceDetailsFilled["pipelineName"] = message.captureTriggersMessage_;
          inferenceDetailsFilled["is_inferred"] = "true";
          forward_message.inferenceDetailsMap_ = inferenceDetailsFilled;

          LOG_ALWAYS("[CAPTURE::PYLON] Successful Capture #" + std::to_string(currIter+1) + " using " + getCameraName());

          capture_end_time_ = std::chrono::steady_clock::now();
          capture_elapsed_seconds_ = capture_end_time_ - capture_start_time_;
          LOG_ALWAYS("[CAPTURE::PYLON] Capture Elapsed Time: " + std::to_string(capture_elapsed_seconds_.count()) + " seconds.");

          start_timeout_ = std::chrono::steady_clock::now();
          capTriggerState_ = false;

          // Sending message to next step in pipeline
          if(camera2forward_.find(message.captureTriggersMessage_))
          {
            LOG_ALWAYS("[CAPTURE::OPENCV] Sending message forward");
            camera2forward_.produce_message(message.captureTriggersMessage_, forward_message);
          }
        }
      }
    }

    /**
      Running the capture using the Pylon API
      @param frameData passing by reference to get the frame data as unsigned char array
      @param frameDataSize passing by reference to get the size of the frame data in the form of HxWxC
      @return error-code showing if the capture object was created or not
    */
    int PylonCapture::runCapture(unsigned char*& frameData, int& frameDataSize)
    {
      if (camera_.IsGrabbing())
      {
        camera_.RetrieveResult(delay_, ptrGrabResult_, Pylon::TimeoutHandling_ThrowException);
        if (ptrGrabResult_->GrabSucceeded())
        {
          fc_.Convert(image_, ptrGrabResult_);
          frameData = (uint8_t*)image_.GetBuffer();
          frameDataSize = ptrGrabResult_->GetWidth() * ptrGrabResult_->GetHeight() * 3;
        }
        else
        {
          return RUN_CAPTURE_ERROR;
        }
      }
      else
      {
        return RUN_CAPTURE_ERROR;
      }
      return CAPTURE_OK;
    }

  }
}
