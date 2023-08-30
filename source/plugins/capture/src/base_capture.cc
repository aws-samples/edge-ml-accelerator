/**
 * @base_capture.cc
 * @brief Creating Base Capture Class
 *
 * This contains the function definitions for creating the base capture class that includes
 * routines for setting and getting parameters.
 *
 */

#include <edge-ml-accelerator/capture/base_capture.h>

namespace edgeml
{
  namespace capture
  {

    /**
      Instance of the class
    */
    Capture* Capture::instance(utils::jsonParser::jValue j, int cameraIndex, SharedMessage<MessageT2C> &trigger2camera)
    {
      static Capture* inst = 0;

      if (!inst)
      {
        inst = new Capture(j, cameraIndex, trigger2camera);
      }
      return inst;
    }

    /**
      Creates the class constructor
    */
    Capture::Capture(utils::jsonParser::jValue j, int cameraIndex, SharedMessage<MessageT2C> &trigger2camera) : jsonParams_(j), trigger2camera_(trigger2camera)
    {
      cameraIndex_ = cameraIndex;
      cameraName_ = jsonParams_["capture"][cameraIndex]["cameraName"].as_string();
      exposureTime_ = jsonParams_["capture"][cameraIndex]["exposureTime"].as_double();
      gainValue_ = jsonParams_["capture"][cameraIndex]["gainValue"].as_int();
      serialNumber_ = jsonParams_["capture"][cameraIndex]["serialNumber"].as_string();
      captureType_ = jsonParams_["capture"][cameraIndex]["cameraType"].as_string();
      captureMode_ = jsonParams_["capture"][cameraIndex]["captureMode"].as_string();
      controlOrMonitor_ = jsonParams_["capture"][cameraIndex]["controlOrMonitor"].as_int();
      height_ = jsonParams_["capture"][cameraIndex]["height"].as_int();
      width_ = jsonParams_["capture"][cameraIndex]["width"].as_int();
      resizeHeight_ = jsonParams_["preprocess"]["resizeHeight"].as_int();
      resizeWidth_ = jsonParams_["preprocess"]["resizeWidth"].as_int();
      colorSpace_ = jsonParams_["preprocess"]["colorSpace"].as_string();
      numInferences_ = jsonParams_["inference"].size();
      LOG_ALWAYS("[CAPTURE::BASE] Base Capture class is created for camera: " + jsonParams_["capture"][cameraIndex]["cameraName"].as_string());

      if (captureType_=="GSTREAMER")
        captureMode_ = "GSTREAMERMODE";
    }

    /**
      Creates the class destructor
    */
    Capture::~Capture()
    {
    }

    /**
      Getting camera ID
      @return get the camera ID as a string
    */
    std::string Capture::getCamID()
    {
      return serialNumber_;
    }

    /**
      Setting camera ID
      @return set the camera ID as a string
    */
    void Capture::setCamID(std::string serialNumber)
    {
      serialNumber_ = serialNumber;
    }

    /**
      Setting input height
      @param in_height set the input image height as int
    */
    void Capture::setInputHeight(int in_height)
    {
      height_ = in_height;
    }

    /**
      Setting input width
      @param in_width set the input image height as int
    */
    void Capture::setInputWidth(int in_width)
    {
      width_ = in_width;
    }

    /**
      Getting input height
      @return get the input image height as int
    */
    int Capture::getInputHeight()
    {
      return height_;
    }

    /**
      Getting input width
      @return get the input image width as int
    */
    int Capture::getInputWidth()
    {
      return width_;
    }

    /**
      Getting height for resizing
      @return get the input image height for resizing as int
    */
    int Capture::getResizeInputHeight()
    {
      return resizeHeight_;
    }

    /**
      Getting width for resizing
      @return get the input image width for resizing as int
    */
    int Capture::getResizeInputWidth()
    {
      return resizeWidth_;
    }

    /**
      Getting the input param if its to be converted to RGB or not
      @return string of color space if its "RGB" or "YUV"
    */
    std::string Capture::getColorSpace()
    {
      return colorSpace_;
    }

    /**
      Getting the camera operating mode if its CONTROL or MONITOR
      @return int for 1 if CONTROL else MONITOR
    */
    int Capture::getCameraOperatingMode()
    {
      return controlOrMonitor_;
    }

    /**
      Getting the camera name
      @return std::string showing the camera name as per config.json
    */
    std::string Capture::getCameraName()
    {
      return cameraName_;
    }

    /**
      Getting the number of inference being used
      @return int showing the number of inferences
    */
    int Capture::getNumInferences()
    {
      return numInferences_;
    }

    /**
      Getting the capture mode as imagefile, videofile, camera -> mainly for OpenCV
      @return std::string showing capture mode
    */
    std::string Capture::getCaptureMode()
    {
      return captureMode_;
    }

    /**
      Getting the value of capture exposure time
      @return double value of capture exposure time
    */
    double Capture::getExposureTime()
    {
      return exposureTime_;
    }

    /**
      Getting the value of capture gain value
      @return int value of capture gain value
    */
    int Capture::getGainValue()
    {
      return gainValue_;
    }

    /**
      Getting the empty inference details json
      @return empty json for filling inference details
    */
    nlohmann::json Capture::getInferenceDetailsJson()
    {
      inferenceDetailsJson["pipelineName"] = "NONE";
      inferenceDetailsJson["numInferencesDone"] = "0";
      inferenceDetailsJson["response"]["image"] = "";
      inferenceDetailsJson["response"]["is_inferred"] = "NONE";
      return inferenceDetailsJson;
    }

  }
}