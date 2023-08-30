/**
 * @soft_trigger.cc
 * @brief Creating and Running Software based capture trigger
 *
 * This contains the function definitions for creating and running Software based camera capture trigger.
 *
 */

#include <edge-ml-accelerator/trigger/soft_trigger.h>

namespace edgeml
{
  namespace trigger
  {

    /**
      Instance of the class
    */
    SoftTrigger* SoftTrigger::instance(utils::jsonParser::jValue j, int cameraIndex)
    {
      static SoftTrigger* inst = 0;

      if (!inst)
      {
        inst = new SoftTrigger(j, cameraIndex);
      }
      return inst;
    }

    /**
      Creates the class constructor
    */
    SoftTrigger::SoftTrigger(utils::jsonParser::jValue j, int cameraIndex) : Trigger(j, cameraIndex)
    {
      LOG_ALWAYS("[TRIGGER::Software Trigger] Using Software trigger");

      swTriggerDelay_ = getSoftwareTriggerDelay();

      LOG_ALWAYS("[TRIGGER::Software Trigger] Software trigger delay = " + std::to_string(swTriggerDelay_) + " ms");
    }

    /**
      Creates the class destructor
    */
    SoftTrigger::~SoftTrigger()
    {
    }

    /**
      Getting a trigger
      @param errc returning the error code of trigger API
      @param iter running for total iterations if > 0 else running infinite loop
    */
    void SoftTrigger::getTrigger(int& errc, int& iter)
    {
      std::string outMessageString_ =  outMessageStringJson_["command"];
      int currIter = 0;
      while(1)
      {
        if (iter>0)
        {
          if (currIter>=iter)
          {
            LOG_ALWAYS("[TRIGGER::Software Trigger] TRIGGER ENDING");
            break;
          }
        }

        mtx_.lock();

        trigger_start_time_ = std::chrono::steady_clock::now();

        std::this_thread::sleep_for(std::chrono::milliseconds(swTriggerDelay_));

        currIter++;
        LOG_ALWAYS("[TRIGGER::Software Trigger] Trigger #" + std::to_string(currIter) + " Successful after " + std::to_string(trigger_elapsed_seconds_.count()) + " seconds");
        LOG_ALWAYS("[TRIGGER::Software Trigger] MESSAGE: CAPTURE: " + outMessageString_);
        MessageT2C message;

        trigger_end_time_ = std::chrono::steady_clock::now();
        trigger_elapsed_seconds_ = trigger_end_time_ - trigger_start_time_;

        start_timeout_ = std::chrono::steady_clock::now();

        outMessageStringJson_["utcTime"] = getUtcTime();

        message.captureTriggersMessage_ = outMessageString_;
        message.captureTriggersMessageFull_ = outMessageStringJson_;
        message.captureTriggersType_ = "soft";

        trigger2camera_.produce_message(message);

        mtx_.unlock();
      }
    }

  }
}
