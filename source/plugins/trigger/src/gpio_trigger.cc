/**
 * @gpio_trigger.cc
 * @brief Creating and Running GPIO based capture trigger
 *
 * This contains the function definitions for creating and running GPIO based camera capture trigger.
 *
 */

#include <edge-ml-accelerator/trigger/gpio_trigger.h>

namespace edgeml
{
  namespace trigger
  {

    /**
      Instance of the class
    */
    GpioTrigger* GpioTrigger::instance(utils::jsonParser::jValue j, int cameraIndex)
    {
      static GpioTrigger* inst = 0;

      if (!inst)
      {
        inst = new GpioTrigger(j, cameraIndex);
      }
      return inst;
    }

    /**
      Creates the class constructor
    */
    GpioTrigger::GpioTrigger(utils::jsonParser::jValue j, int cameraIndex) : Trigger(j, cameraIndex)
    {
      LOG_ALWAYS("[TRIGGER::GPIO Trigger] Using Hardware GPIO trigger");

      gpioTriggerDelay_ = getGpioTriggerDelay();

      LOG_ALWAYS("[TRIGGER::GPIO Trigger] GPIO trigger delay = " + std::to_string(gpioTriggerDelay_) + " ms");
    }

    /**
      Creates the class destructor
    */
    GpioTrigger::~GpioTrigger()
    {
    }

    /**
      Getting a trigger
      @param errc returning the error code of trigger API
      @param iter running for total iterations if > 0 else running infinite loop
    */
    void GpioTrigger::getTrigger(int& errc, int& iter)
    {
      std::string outMessageString_ =  outMessageStringJson_["command"];
      int currIter = 0;
      while(1)
      {
        if (iter>0)
        {
          if (currIter>=iter)
          {
            LOG_ALWAYS("[TRIGGER::GPIO Trigger] TRIGGER ENDING");
            break;
          }
        }

        mtx_.lock();

        trigger_start_time_ = std::chrono::steady_clock::now();

        std::this_thread::sleep_for(std::chrono::milliseconds(swTriggerDelay_));

        gpioRetCap_ = gpio_.gpio_getvalue(GPIO_CAMERA_TRIGGER, gpioValueCap_);
        if (gpioRetCap_==GPIO_SUCCESS && gpioValueCap_==1)
        {
          // Wait for the time the capture trigger goes down
          while (1)
          {
            std::this_thread::sleep_for(std::chrono::milliseconds(jsonParams_["clockTime"].as_int()));
            gpioRetCap_ = gpio_.gpio_getvalue(GPIO_CAMERA_TRIGGER, gpioValueCap_);
            if (gpioRetCap_==GPIO_SUCCESS && gpioValueCap_==0)
            {
              break;
            }
          }
          std::this_thread::sleep_for(std::chrono::milliseconds(2*jsonParams_["clockTime"].as_int()));
        }
        else
        {
          continue;
        }

        currIter++;
        LOG_ALWAYS("[TRIGGER::GPIO Trigger] Trigger #" + std::to_string(currIter) + " Successful after " + std::to_string(trigger_elapsed_seconds_.count()) + " ms");
        LOG_ALWAYS("[TRIGGER::GPIO Trigger] MESSAGE: CAPTURE: " + outMessageString_);
        MessageT2C message;

        trigger_end_time_ = std::chrono::steady_clock::now();
        trigger_elapsed_seconds_ = trigger_end_time_ - trigger_start_time_;

        start_timeout_ = std::chrono::steady_clock::now();

        outMessageStringJson_["utcTime"] = getUtcTime();

        message.captureTriggersMessage_ = message.captureTriggersMessage_;
        message.captureTriggersMessageFull_ = outMessageStringJson_;
        message.captureTriggersType_ = "gpio";
        trigger2camera_.produce_message(message);

        mtx_.unlock();
      }
    }

  }
}