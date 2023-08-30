/**
 * @base_trigger.cc
 * @brief Creating Base Trigger Class
 *
 * This contains the function definitions for creating the base trigger class that includes
 * routines for setting and getting parameters.
 *
 */

#include <edge-ml-accelerator/trigger/base_trigger.h>

namespace edgeml
{
  namespace trigger
  {

    /**
      Instance of the class
    */
    Trigger* Trigger::instance(utils::jsonParser::jValue j, int cameraIndex)
    {
      static Trigger* inst = 0;

      if (!inst)
      {
        inst = new Trigger(j, cameraIndex);
      }
      return inst;
    }

    /**
      Creates the class constructor
    */
    Trigger::Trigger(utils::jsonParser::jValue j, int cameraIndex) : jsonParams_(j)
    {
      hwTriggerDelay_ = jsonParams_["capture"][cameraIndex]["hwTriggerDelay"].as_longint();
      gpioTriggerDelay_ = jsonParams_["capture"][cameraIndex]["gpioTriggerDelay"].as_longint();
      swTriggerDelay_ = jsonParams_["capture"][cameraIndex]["swTriggerDelay"].as_longint();
      useGpioTrigger_ = jsonParams_["capture"][cameraIndex]["useGpioTrigger"].as_bool();
      ipcTriggerTopicName = jsonParams_["capture"][cameraIndex]["ipcTriggerTopic"].as_string();
      mqttTriggerTopicName = jsonParams_["capture"][cameraIndex]["mqttTriggerTopic"].as_string();
    }

    /**
      Creates the class destructor
    */
    Trigger::~Trigger()
    {
    }

    /**
      Getting the value of software trigger delay
      @return long int value of software trigger delay
    */
    long int Trigger::getSoftwareTriggerDelay()
    {
      return swTriggerDelay_;
    }

    /**
      Getting the value of hardware trigger delay
      @return long int value of hardware trigger delay
    */
    long int Trigger::getHardwareTriggerDelay()
    {
      return hwTriggerDelay_;
    }

    /**
      Getting if gpio capture trigger is present or not
      @return bool true if gpio capture trigger is present else false
    */
    bool Trigger::getIsGpioTrigger()
    {
      return useGpioTrigger_;
    }

    /**
      Getting the value of gpio capture trigger delay
      @return long int value of gpio capture trigger delay
    */
    long int Trigger::getGpioTriggerDelay()
    {
      return gpioTriggerDelay_;
    }

    /**
      Getting if ipc capture trigger is present or not
      @return bool true if ipc capture trigger is present else false
    */
    bool Trigger::getIsIpcTrigger()
    {
      return useIpcTrigger_;
    }

    /**
      If it is IPC Topic mode, then get the IPC topic for sub/pub
      @return string which is the IPC topic name
    */
    std::string Trigger::getIpcTriggerTopicName()
    {
      return ipcTriggerTopicName;
    }

    /**
      Getting if mqtt capture trigger is present or not
      @return bool true if mqtt capture trigger is present else false
    */
    bool Trigger::getIsMqttTrigger()
    {
      return useMqttTrigger_;
    }

    /**
      If it is MQTT Topic mode, then get the MQTT topic for sub/pub
      @return string which is the MQTT topic name
    */
    std::string Trigger::getMqttTriggerTopicName()
    {
      return mqttTriggerTopicName;
    }

    /**
      Get the UTC Time
      @return string containing the UTC Time
    */
    std::string Trigger::getUtcTime()
    {
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
        auto str = oss.str();
        return str;
    }

  }
}