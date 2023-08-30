/**
 * @genicam_capture.cc
 * @brief Creating and Running Capture
 *
 * This contains the function definitions for creating and running capture using GenICam for camera.
 *
 */

#include <edge-ml-accelerator/capture/genicam_capture.h>
#include <unistd.h>

/**
  Instance of the class
*/
namespace edgeml
{
  namespace capture
  {

    GenicamCapture* GenicamCapture::instance(utils::jsonParser::jValue j, int cameraIndex, SharedMessage<MessageT2C> &trigger2camera)
    {
      static GenicamCapture* inst = 0;

      if (!inst)
      {
        inst = new GenicamCapture(j, cameraIndex, trigger2camera);
      }
      return inst;
    }

    /**
      Creates the class constructor
    */
    GenicamCapture::GenicamCapture(utils::jsonParser::jValue j, int cameraIndex, SharedMessage<MessageT2C> &trigger2camera) : Capture(j, cameraIndex, trigger2camera)
    {
      LOG_ALWAYS("[CAPTURE::GENICAM] Capture mode is set to camera.");
      exposureTime_ = getExposureTime();
      gainValue_ = getGainValue();
    }

    /**
      Creates the class destructor
    */
    GenicamCapture::~GenicamCapture()
    {
    }

    /**
      Initializing a Genicam capture object for camera ID. Based on [https://github.com/roboception/rc_genicam_api/blob/master/tools/gc_stream.cc]
      @return error-code showing if the capture object was created or not
    */
    int GenicamCapture::initCapture(int cameraIndex)
    {
      // Check for all the cameras connected
      std::string camID = getCamID();
      inferenceDetailsBlank = getInferenceDetailsJson();
      std::vector<std::string> connectedCameraIndices;
      bool isCameraDetected = false;
      std::vector<std::shared_ptr<rcg::System> > system;

      try
      {
        rcg::System::getSystems();

      }
      catch(rcg::GenTLException& e)
      {
        LOG_ALWAYS("[CAPTURE::GENICAM] Invalid CTI File or folder");
        LOG_ALWAYS("[CAPTURE::GENICAM] Quitting for lack of connection to camera");
        exit(0);
      }

      system = rcg::System::getSystems();
      LOG_ALWAYS("[CAPTURE::GENICAM] Trying to find Camera ID #" + camID);
      for (size_t i=0; i<system.size(); i++)
      {
        system[i]->open();
        std::vector<std::shared_ptr<rcg::Interface> > interf=system[i]->getInterfaces();
        for (size_t k=0; k<interf.size(); k++)
        {
          interf[k]->open();
          std::vector<std::shared_ptr<rcg::Device> > device=interf[k]->getDevices();
          for (size_t j=0; j<device.size(); j++)
          {
            connectedCameraIndices.push_back(device[j]->getSerialNumber());
            if (device[j]->getSerialNumber() == camID)
            {
              isCameraDetected = true;
              break;
            }
          }
          interf[k]->close();
        }
        system[i]->close();
      }

      // Check if camera is present else select the first of any available cameras and if not return error
      if (connectedCameraIndices.size()>0)
      {
        LOG_ALWAYS("[CAPTURE::GENICAM] Found Camera ID #" + std::to_string(connectedCameraIndices[0]));
        camID = connectedCameraIndices[0];
        setCamID(camID);
      }
      else
      {
        LOG_ALWAYS("[CAPTURE::GENICAM] Cameras are missing");
        return CAMERA_MISSING;
      }

      colorSpace_ = getColorSpace();

      dev_ = rcg::getDevice(camID.c_str());
      if (dev_)
      {
        dev_->open(rcg::Device::CONTROL); // or rcg::Device::READONLY but it doesn not start streaming in READONLY mode

        // Do this only if camera is in CONTROL mode
        // try to enable chunks by default (can be disabed by the user)
        nodemap_ = dev_->getRemoteNodeMap();
        rcg::setBoolean(nodemap_, "ChunkModeActive", true);

        rcg::setInteger(nodemap_, "Height", getInputHeight());
        rcg::setInteger(nodemap_, "Width", static_cast<int>(getInputWidth()));
        auto height = rcg::getInteger(nodemap_, "Height");
        auto width = rcg::getInteger(nodemap_, "Width");
        LOG_ALWAYS("[CAPTURE::GENICAM] Using Camera ID #" + camID + " with [H,W] = [" + std::to_string((int)(height)) + "," + std::to_string((int)(width)) + "]");

        setInputHeight(height);
        setInputWidth(width);

        rcg::setFloat(nodemap_, "ExposureTimeAbs", exposureTime_, true); // set exposure time in microseconds
        rcg::setFloat(nodemap_, "GainRaw", (float)gainValue_, true); // set gain unit value

        auto exposureTimeCurr = rcg::getFloat(nodemap_, "ExposureTimeAbs");
        auto gainCurr = rcg::getFloat(nodemap_, "GainRaw");
        LOG_ALWAYS("[CAPTURE::GENICAM] Camera Settings:");
        LOG_ALWAYS("[CAPTURE::GENICAM]  - ExposureTimeAbs = " + std::to_string((float)exposureTimeCurr));
        LOG_ALWAYS("[CAPTURE::GENICAM]  - GainRaw = " + std::to_string((int)gainCurr));

        if (colorSpace_=="BGR")
        {
          rcg::setEnum(nodemap_, "PixelFormat", "BayerGR8", false);
        }
        else if (colorSpace_=="GRAY" || colorSpace_=="MONO8")
        {
          rcg::setEnum(nodemap_, "PixelFormat", "Mono8", false);
        }
        else
        {
          rcg::setEnum(nodemap_, "PixelFormat", "BayerRG8", false);
        }

        LOG_ALWAYS("[CAPTURE::GENICAM] GenICam Capture index: " + std::to_string(cameraIndex));
        LOG_ALWAYS("[CAPTURE::GENICAM] GenICam Capture class is created for camera: " + jsonParams_["capture"][cameraIndex]["cameraName"].as_string());

        // if (false) // Use this for Camera Hardware Trigger
        // {
        //   hwTriggerDelay_ = getHardwareTriggerDelay();
        //   LOG_ALWAYS("[CAPTURE::GENICAM] Using Hardware Trigger with delay = " + std::to_string(hwTriggerDelay_) + " ms.");
        //   rcg::setEnum(nodemap_, "AcquisitionStatusSelector", "FrameTriggerWait", true);
        //   rcg::setEnum(nodemap_, "TriggerSelector", "FrameStart", true);
        //   rcg::setEnum(nodemap_, "TriggerMode", "On", true);
        //   rcg::setEnum(nodemap_, "TriggerSource", "Line1", true);
        //   rcg::setEnum(nodemap_, "TriggerActivation", "RisingEdge", true);
        //   rcg::setFloat(nodemap_, "TriggerDelayAbs", 100, true);
        // }

        // open stream and get n images
        stream_ = dev_->getStreams();
        stream_[0]->open();
        stream_[0]->attachBuffers(true);

        return CAPTURE_OK;
      }
      else
      {
        return CAMERA_MISSING;
      }
    }

    /**
      Getting a capture based image file getting as a bytes data. This API will be called in order to get the capture data.
      @param errc returning the error code of capture API
      @param frameData passing by reference to get the frame data as unsigned char array
      @param frameDataSize passing by reference to get the size of the frame data in the form of HxWxC
      @param iter running for total iterations if > 0 else running infinite loop
    */
    void GenicamCapture::getCapture(int& errc, unsigned char*& frameData, int& frameDataSize, int& iter)
    {
      getCaptureAtIndex(errc, frameData, frameDataSize, iter, 0);
    }

    void GenicamCapture::getCaptureAtIndex(int& errc, unsigned char*& frameData, int& frameDataSize, int& iter, int cameraIndex)
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
        LOG_ALWAYS("[CAPTURE::GENICAM] Trigger to Camera Message = " + message.captureTriggersMessage_);

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
          LOG_ALWAYS("[CAPTURE::GENICAM] Changing Config based on following: ");
          LOG_ALWAYS(message.captureTrigger_.captureTriggersMessageFull.to_string());

          if (message.captureTriggersMessageFull_["exposureTime"].as_double()>=10)
          {
            exposureTime_ = message.captureTriggersMessageFull_["exposureTime"].as_double();
            LOG_ALWAYS("[CAPTURE::GENICAM] Changing exposureTime_ = " + exposureTime_);
            rcg::setFloat(nodemap_, "ExposureTimeAbs", exposureTime_, true); // set exposure time in microseconds
          }
          if (message.captureTriggersMessageFull_["gainValue"].as_int()>=0)
          {
            gainValue_ = message.captureTriggersMessageFull_["gainValue"].as_int();
            LOG_ALWAYS("[CAPTURE::GENICAM] Changing gainValue_ = " + gainValue_);
            rcg::setFloat(nodemap_, "GainRaw", (float)gainValue_, true); // set gain unit values
          }
          // container_->captureTriggersMessage.pop();
          // container_->captureTriggersMessageFull.pop();
          continue;
        }

        int errc = runCapture(frameData, frameDataSize);
        if (errc==0)
        {
          forward_message.safeCaptureContainer_.push(frameData);
          forward_message.safeCaptureSizeContainer_.push(frameDataSize);

          LOG_ALWAYS("[CAPTURE::GENICAM] Trigger: " + message.captureTriggersMessage_);

          inferenceDetailsFilled = inferenceDetailsBlank;
          inferenceDetailsFilled["pipelineName"] = message.captureTriggersMessage_;
          inferenceDetailsFilled["is_inferred"] = "true";
          forward_message.inferenceDetailsMap_ = inferenceDetailsFilled;

          LOG_ALWAYS("[CAPTURE::GENICAM] Successful Capture #" + std::to_string(currIter+1) + " using " << getCameraName());

          capture_end_time_ = std::chrono::steady_clock::now();
          capture_elapsed_seconds_ = capture_end_time_ - capture_start_time_;
          LOG_ALWAYS("[CAPTURE::GENICAM] Capture Elapsed Time: " + std::to_string(capture_elapsed_seconds_.count()) + " seconds.");

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
      Running the capture using the genicam API. Based on [https://github.com/roboception/rc_genicam_api/blob/master/tools/gc_stream.cc]
      @param frameData passing by reference to get the frame data as unsigned char array
      @param frameDataSize passing by reference to get the size of the frame data in the form of HxWxC
      @return error-code showing if the capture object was created or not
    */
    int GenicamCapture::runCapture(unsigned char*& frameData, int& frameDataSize)
    {
      if (stream_.size() > 0)
      {
        stream_[0]->startStreaming(1);
        buffer_ = stream_[0]->grab(delay_);

        buffers_received_ = 0;
        buffers_incomplete_ = 0;

        if (buffer_ != 0)
        {
          buffers_received_++;

          if (!buffer_->getIsIncomplete())
          {
            npart_ = buffer_->getNumberOfParts();
            for (uint32_t part = 0; part < npart_; part++)
            {
              if (buffer_->getImagePresent(part))
              {
                rcg::Image image(buffer_, part);
                height_ = (int)(image.getHeight());
                width_ = (int)(image.getWidth());
                frameDataSize = 3*width_*height_;

                format_ = image.getPixelFormat();
                yoffset_ = 0;
                px = image.getXPadding();
                yoffset_ = std::min(yoffset_, (size_t)(height_));
                const unsigned char *p = static_cast<const unsigned char *>(image.getPixels());
                std::unique_ptr<uint8_t []> rgb_pixel(new uint8_t [frameDataSize]);
                p += (width_ + px) * yoffset_;
                rcg::convertImage(rgb_pixel.get(), 0, p, format_, width_, height_, px); // convert to RGB pixels

                memcpy(frameData, static_cast<unsigned char*>(rgb_pixel.get()), frameDataSize); // memory copy to the frameData
              }
            }
            ret_ = CAPTURE_OK;
          }
          else
          {
            LOG_ERROR("[CAPTURE::GENICAM] Incomplete buffer received");
            buffers_incomplete_++;
          }
        }
        else
        {
          LOG_ERROR("[CAPTURE::GENICAM] Cannot grab images");
          ret_ = RUN_CAPTURE_ERROR;
        }

        ret_ = CAPTURE_OK;
      }
      else
      {
        ret_ = RUN_CAPTURE_ERROR;
      }

      ret_ = CAPTURE_OK;
      return ret_;
    }

  }
}
