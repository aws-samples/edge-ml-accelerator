/**
 * @opencv_capture.cc
 * @brief Creating and Running Capture
 *
 * This contains the function definitions for creating and running capture using OpenCV for camera, video or image.
 *
 */

#include <edge-ml-accelerator/capture/opencv_capture.h>

#ifdef _WIN32
#undef min
#undef max
#endif

#ifdef _WIN32
   #include <io.h>
   #define access    _access_s
#else
   #include <unistd.h>
#endif

namespace edgeml
{
  namespace capture
  {

    /**
      Check if the file exists
      @param s filename as a string
      @return boolean true if file exists or false
    */
    bool fileExists(std::string& s)
    {
        return access( s.c_str(), 0 ) == 0;
    }

    /**
      Getting file extensions
      @param s filename as a string
      @return filename extension as a string
    */
    std::string getFileExt(std::string& s)
    {
      size_t i = s.rfind('.', s.length());
      if (i != std::string::npos)
      {
          return(s.substr(i+1, s.length() - i));
      }
      return("");
    }

    /**
      Instance of the class
    */
    OpenCVCapture* OpenCVCapture::instance(utils::jsonParser::jValue j, int cameraIndex, SharedMessage<MessageT2C> &trigger2camera)
    {
      static OpenCVCapture* inst = 0;

      if (!inst)
      {
        inst = new OpenCVCapture(j, cameraIndex, trigger2camera);
      }
      return inst;
    }

    /**
      Creates the class constructor
    */
    OpenCVCapture::OpenCVCapture(utils::jsonParser::jValue j, int cameraIndex, SharedMessage<MessageT2C> &trigger2camera) : Capture(j, cameraIndex, trigger2camera)
    {
      captureMode_ = getCaptureMode();
      if (captureMode_=="IMAGEFILEMODE")
      {
        captureModeInt_ = IMAGEFILEMODE;
        imageFile_ = jsonParams_["capture"][cameraIndex_]["imageIn"].as_string();
      }
      else if (captureMode_=="VIDEOFILEMODE")
      {
        captureModeInt_ = VIDEOFILEMODE;
        videoFile_ = jsonParams_["capture"][cameraIndex_]["videoIn"].as_string();
      }
      else if (captureMode_=="GSTREAMERMODE")
      {
        captureModeInt_ = GSTREAMERMODE;
      }
      else {captureModeInt_ = CAMERAMODE;}

      LOG_ALWAYS("[CAPTURE::OPENCV] Capture mode is set to " + getCaptureMode() + ".");
    }

    /**
      Creates the class destructor
    */
    OpenCVCapture::~OpenCVCapture()
    {
      switch (captureModeInt_)
      {
        case VIDEOFILEMODE:
          cap_.release();
          break;
        case CAMERAMODE:
          cap_.release();
          break;
        case GSTREAMERMODE:
          cap_.release();
          break;
        default:
          break;
      }
    }

    /**
      Creating a VideoCapture object for camera
      @return error-code showing if the capture object was created or not
    */
    int OpenCVCapture::initCapture(int cameraIndex)
    {
      inferenceDetailsBlank = getInferenceDetailsJson();

      switch (captureModeInt_)
      {
        case IMAGEFILEMODE:
        {
          LOG_ALWAYS("[CAPTURE::OPENCV] Capture mode is set to image file.");
          break;
        }
        case VIDEOFILEMODE:
        {
          LOG_ALWAYS("[CAPTURE::OPENCV] Capture mode is set to video file.");
          break;
        }
        case CAMERAMODE:
        {
          LOG_ALWAYS("[CAPTURE::OPENCV] Capture mode is set to camera.");
          break;
        }
        case GSTREAMERMODE:
        {
          LOG_ALWAYS("[CAPTURE::OPENCV] Capture mode is set to gstreamer pipeline.");
          break;
        }
        default:
        {
          LOG_ALWAYS("[CAPTURE::OPENCV] No capture mode set.");
          break;
        }
      }

      switch(captureModeInt_)
      {
        case IMAGEFILEMODE:
        {
          LOG_ALWAYS("[CAPTURE::OPENCV] Image file is: " + imageFile_);
          frame_ = cv::imread(imageFile_);
          setInputHeight(static_cast<int>(frame_.rows));
          setInputWidth(static_cast<int>(frame_.cols));
          break;
        }
        case VIDEOFILEMODE:
        {
          LOG_ALWAYS("[CAPTURE::OPENCV] Video file is: " + videoFile_);
          cap_.open(videoFile_);
          cap_.set(cv::CAP_PROP_FRAME_HEIGHT, getInputHeight());
          cap_.set(cv::CAP_PROP_FRAME_WIDTH, getInputWidth());
          setInputHeight(static_cast<int>(cap_.get(cv::CAP_PROP_FRAME_HEIGHT)));
          setInputWidth(static_cast<int>(cap_.get(cv::CAP_PROP_FRAME_WIDTH)));
          break;
        }
        case CAMERAMODE:
        {
          std::string camID = getCamID();
          camID = "/dev/video" + camID;
          LOG_ALWAYS("[CAPTURE::OPENCV] Camera selected is: " + camID);
          cap_.open(camID, cv::CAP_V4L2);
          if (!cap_.isOpened())
          {
            cap_.open("/dev/video0", cv::CAP_V4L2);
            if (cap_.isOpened())
            {
              camID = "/dev/video0";
              setCamID(camID);
            }
            else
            {
              LOG_ALWAYS("[CAPTURE::OPENCV] Camera is missing.");
              return CAMERA_MISSING;
            }
          }
          cap_.set(cv::CAP_PROP_FRAME_HEIGHT, getInputHeight());
          cap_.set(cv::CAP_PROP_FRAME_WIDTH, getInputWidth());
          setInputHeight(static_cast<int>(cap_.get(cv::CAP_PROP_FRAME_HEIGHT)));
          setInputWidth(static_cast<int>(cap_.get(cv::CAP_PROP_FRAME_WIDTH)));
          break;
        }
        case GSTREAMERMODE:
        {
          std::string camID = getCamID();
          LOG_ALWAYS("[CAPTURE::OPENCV] GStramer Pipeline selected is: [" + camID + "]");
          cap_.open(camID.c_str(), cv::CAP_GSTREAMER);
          if (!cap_.isOpened())
          {
            LOG_ALWAYS("[CAPTURE::OPENCV] Camera is missing.");
            return CAMERA_MISSING;
          }
          setInputHeight(static_cast<int>(cap_.get(cv::CAP_PROP_FRAME_HEIGHT)));
          setInputWidth(static_cast<int>(cap_.get(cv::CAP_PROP_FRAME_WIDTH)));
          break;
        }
        default:
          break;
      }

      height_ = getInputHeight();
      width_ = getInputWidth();
      resizeHeight_ = getResizeInputHeight();
      resizeWidth_ = getResizeInputWidth();
      colorSpace_ = getColorSpace();

      if ((height_==resizeHeight_) && (width_==resizeWidth_))
      {
        isResize_ = false;
      }
      else
      {
        isResize_ = true;
        height_ = resizeHeight_; width_ = resizeWidth_;
        setInputHeight(height_);
        setInputWidth(width_);
      }

      LOG_ALWAYS("[CAPTURE::OPENCV] " + std::string(CaptureTypesE[captureModeInt_]) + " has [H,W] = [" + std::to_string((int)(height_)) + "," + std::to_string((int)(width_)) + "]");

      return CAPTURE_OK;
    }

    /**
      Getting a capture based image file getting as a cv::Mat
      @param errc returning the error code of capture API
      @param frameData passing by reference to get the frame data as unsigned char array
      @param frameDataSize passing by reference to get the size of the frame data in the form of HxWxC
      @param iter running for total iterations if > 0 else running infinite loop
    */
    void OpenCVCapture::getCapture(int& errc, unsigned char*& frameData, int& frameDataSize, int& iter)
    {
      getCaptureAtIndex(errc, frameData, frameDataSize, iter, 0);
    }

    void OpenCVCapture::getCaptureAtIndex(int& errc, unsigned char*& frameData, int& frameDataSize, int& iter, int cameraIndex)
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
        LOG_ALWAYS("[CAPTURE::OPENCV] Trigger to Camera Message = " + message.captureTriggersMessage_);

        message.captureTriggersMessageFull_["captureID"] = "#" + std::to_string(currIter);

        // Create a message forward with local scope
        MessageCaptureInference forward_message;
        forward_message.captureTrigger_ = message;
        forward_message.cameraName_ = cameraName_;

        capture_start_time_ = std::chrono::steady_clock::now();

        // check if it's an active pipeline
        if(!camera2forward_.find(forward_message.captureTrigger_.captureTriggersMessage_))
        {
          LOG_ALWAYS("[CAPTURE::OPENCV] Command Pipeline NOT available ");
          continue;
        }

        if (message.captureTriggersMessage_ == "configchange")
        {
          LOG_ALWAYS("[CAPTURE::OPENCV] Changing Config based on following: ");
          LOG_ALWAYS(std::string(message.captureTriggersMessageFull_.dump()));

          LOG_ALWAYS("[CAPTURE::OPENCV] Cannot make changes for OpenCV Cameras");
          continue;
        }

        errc = runCapture();

        if (errc!=CAPTURE_OK)
        {
          LOG_ALWAYS("[CAPTURE::OPENCV] ERROR READING IMAGES");
          continue;
        }

        if (isResize_)
        {
          cv::resize(frame_, frame_resized_, cv::Size(resizeWidth_, resizeHeight_));
          LOG_ALWAYS("[CAPTURE::OPENCV] Resized image to [H,W] = [" + std::to_string((int)(frame_resized_.rows)) + "," + std::to_string((int)(frame_resized_.cols)) + "]");
        }
        else
        {
          frame_resized_ = frame_;
        }

        //todo get rid of bgr, just rgb from now on
        if (colorSpace_=="RGB")
        {
          cv::cvtColor(frame_resized_, frame_resized_, cv::COLOR_BGR2RGB);
        }

        frameData = (unsigned char*)(frame_resized_.data);
        frameDataSize = frame_resized_.total() * frame_resized_.elemSize();

        if (errc==0)
        {
          forward_message.safeCaptureContainer_.push(frameData);
          forward_message.safeCaptureSizeContainer_.push(frameDataSize);

          LOG_ALWAYS("[CAPTURE::OPENCV] Trigger: " + message.captureTriggersMessage_);

          inferenceDetailsFilled = inferenceDetailsBlank;
          inferenceDetailsFilled["pipelineName"] = message.captureTriggersMessage_;
          inferenceDetailsFilled["is_inferred"] = "true";
          forward_message.inferenceDetailsMap_ = inferenceDetailsFilled;

          LOG_ALWAYS("[CAPTURE::OPENCV] Successful Capture #" + std::to_string(currIter+1) + " for camera");

          capture_end_time_ = std::chrono::steady_clock::now();
          capture_elapsed_seconds_ = capture_end_time_ - capture_start_time_;
          LOG_ALWAYS("[CAPTURE::OPENCV] Capture Elapsed Time: " + std::to_string(capture_elapsed_seconds_.count()) + " seconds.");

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
      Running a capture based on the capture input mode if it was camera/video/image
      @return error-code showing if the capture run was successful or not
    */
    int OpenCVCapture::runCapture()
    {
      switch (captureModeInt_)
      {
        case IMAGEFILEMODE:
        {
          LOG_ALWAYS("[CAPTURE::OPENCV] Reading image from image file.");
          std::string ext = getFileExt(imageFile_);
          if (ext.compare("jpg")==0 || ext.compare("JPG")==0 || ext.compare("png")==0 ||
              ext.compare("PNG")==0 || ext.compare("jpeg")==0 || ext.compare("JPEG")==0)
          {
            if (!fileExists(imageFile_)) { return IMAGE_FILE_MISSING; }
            else
            {
              frame_ = cv::imread(imageFile_);
              LOG_ALWAYS("[CAPTURE::OPENCV] Using ImageFileMode with [H,W] = [" + std::to_string((int)(frame_.rows)) + "," + std::to_string((int)(frame_.cols)) + "]");
            }
            return CAPTURE_OK;
          }
          else
          {
            return INVALID_FILE;
          }
          break;
        }
        case VIDEOFILEMODE:
        {
          LOG_ALWAYS("[CAPTURE::OPENCV] Reading frame from video file.");
          cap_.read(frame_);
          LOG_ALWAYS("[CAPTURE::OPENCV] Using VideoFileMode with [H,W] = [" + std::to_string((int)(frame_.rows)) + "," + std::to_string((int)(frame_.cols)) + "]");
          return CAPTURE_OK;
          break;
        }
        case CAMERAMODE:
        {
          LOG_ALWAYS("[CAPTURE::OPENCV] Reading frame from camera.");
          cap_.read(frame_);
          LOG_ALWAYS("[CAPTURE::OPENCV] Using CameraMode with [H,W] = [" + std::to_string((int)(frame_.rows)) + "," + std::to_string((int)(frame_.cols)) + "]");
          return CAPTURE_OK;
          break;
        }
        case GSTREAMERMODE:
        {
          LOG_ALWAYS("[CAPTURE::OPENCV] Reading frame from gstreamer pipeline.");
          cap_.read(frame_);
          LOG_ALWAYS("[CAPTURE::OPENCV] Using GStreamerMode with [H,W] = [" + std::to_string((int)(frame_.rows)) + "," + std::to_string((int)(frame_.cols)) + "]");
          return CAPTURE_OK;
          break;
        }
        default:
        {
          LOG_ERROR("[CAPTURE::OPENCV] Could not capture from any source.");
          return RUN_CAPTURE_ERROR;
          break;
        }
      }
      return CAPTURE_OK;
    }

  }
}
