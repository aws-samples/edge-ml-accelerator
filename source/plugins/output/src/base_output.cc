/**
 * @base_output.cc
 * @brief Creating Base Output Sink Class
 *
 * This contains the function definitions for creating the base output sink class that includes
 * routines for setting and getting parameters.
 *
 */

#include <edge-ml-accelerator/output/base_output.h>

namespace edgeml
{
  namespace output
  {

    /**
      Instance of the class
    */
    Output* Output::instance(utils::jsonParser::jValue j, SharedMessage<MessageCaptureInference> &incoming_message)
    {
      static Output* inst = 0;

      if (!inst)
      {
        inst = new Output(j, incoming_message);
      }
      return inst;
    }

    /**
      Creates the class constructor
    */
    Output::Output(utils::jsonParser::jValue j, SharedMessage<MessageCaptureInference> &incoming_message) : jsonParams_(j), incoming_message_(incoming_message)
    {
      for (int i=0; i<jsonParams_["outputsink"].size(); i++)
      {
        if (jsonParams_["outputsink"][i]["outputSinkType"].as_string() == "local")
        {
          localDisk = jsonParams_["outputsink"][i]["localDisk"].as_string();
          imageFormat = jsonParams_["outputsink"][i]["imageFormat"].as_string();
          if (imageFormat=="JPG")
            imageFormat = "jpg";
          if (imageFormat=="JPEG")
            imageFormat = "jpeg";
          if (imageFormat=="PNG")
            imageFormat = "png";
        }
        if (jsonParams_["outputsink"][i]["outputSinkType"].as_string() == "s3")
        {
          s3bucket = jsonParams_["outputsink"][i]["s3bucket"].as_string();
          s3key = jsonParams_["outputsink"][i]["s3key"].as_string();
          s3region = jsonParams_["outputsink"][i]["s3region"].as_string();
        }
        if (jsonParams_["outputsink"][i]["outputSinkType"].as_string() == "ipctopic")
        {
          ipcTopicName = jsonParams_["outputsink"][i]["topicname"].as_string();
        }
        if (jsonParams_["outputsink"][i]["outputSinkType"].as_string() == "mqtttopic")
        {
          mqttTopicName = jsonParams_["outputsink"][i]["topicname"].as_string();
        }
      }
      colorSpace_ = jsonParams_["preprocess"]["colorSpace"].as_string();
      LOG_ALWAYS("[OUTPUT::BASE] Base Output class is created.");
    }

    /**
      Creates the class destructor
    */
    Output::~Output()
    {
    }

    /**
      Creating the output path
      @return path where files need to be saved
    */
    std::string Output::getLocalPath()
    {
      return localDisk;
    }

    /**
      Creating the output image format
      @return image format either jpg or png
    */
    std::string Output::getImageFormat()
    {
      if (imageFormat!="jpg" &&imageFormat!="jpeg" && imageFormat!="png")
        imageFormat = "png";
      return imageFormat;
    }

    /**
      Creating the image colorspace
      @return image colorspace rgb or bgr
    */
    std::string Output::getColorSpace()
    {
      return colorSpace_;
    }

    /**
      Creating the output file name with 'YYYY-MM-DD-HH-MM-SS-MSxxxxxxxx' format
      @return filename with the above mentioned format
    */
    std::string Output::getOutputFileName()
    {
      time_t rawtime;
        struct tm * timeinfo;
        char buffer[80];
        time (&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(buffer,80,"%Y-%m-%d-%H-%M-%S",timeinfo);
        std::chrono::milliseconds ms = std::chrono::duration_cast< std::chrono::milliseconds >(std::chrono::system_clock::now().time_since_epoch());
        return std::string(buffer) + "-" + std::to_string(ms.count());
    }

    /**
      Getting file extensions
      @param s filename as a string
      @return filename extension as a string
    */
    std::string Output::getFileExt(std::string& s)
    {
      size_t i = s.rfind('.', s.length());
      if (i != std::string::npos) {
          return(s.substr(i, s.length() - i));
      }
      return("");
    }

    /**
      Getting the AWS region name
      @return region as defined in the OutputSinkMode
    */
    std::string Output::getAWSRegion()
    {
      return s3region;
    }

    /**
      If it is S3 mode, then get the Bucket and Key already set
      @param bucket the value of bucket passed by reference
      @param key the value of key/prefix passed by reference
    */
    void Output::getS3BucketAndKey(std::string& bucket, std::string& key)
    {
      bucket = s3bucket;
      key = s3key;
    }

    /**
      If it is S3 mode, then get the Bucket
      @return S3 Bucket
    */
    std::string Output::getS3Bucket()
    {
      return s3bucket;
    }

    /**
      If it is S3 mode, then get the Key already set
      @return S3 Key
    */
    std::string Output::getS3Key()
    {
      return s3key;
    }

    /**
      If it is IPC Topic mode, then get the IPC topic for sub/pub
      @return string which is the IPC topic name
    */
    std::string Output::getIpcTopicName()
    {
      return ipcTopicName;
    }

    /**
      If it is MQTT Topic mode, then get the MQTT topic for sub/pub
      @return string which is the MQTT topic name
    */
    std::string Output::getMqttTopicName()
    {
      return mqttTopicName;
    }

  }
}