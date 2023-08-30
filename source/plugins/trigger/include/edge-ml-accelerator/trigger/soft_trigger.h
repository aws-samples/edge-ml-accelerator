/**
 * @soft_trigger.h
 * @brief Creating and Running Software based trigger
 *
 * This contains the function definitions for creating and running Software based capture trigger.
 *
 */

#ifndef __SOFT_TRIGGER_H__
#define __SOFT_TRIGGER_H__

#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <atomic>
#include <queue>
#include <chrono>
#include <mutex>

#include <edge-ml-accelerator/trigger/base_trigger.h>

using namespace edgeml::utils;

namespace edgeml
{
    namespace trigger
    {

        class SoftTrigger : public Trigger
        {
            public:
                static SoftTrigger* instance(utils::jsonParser::jValue j, int cameraIndex);

                SoftTrigger(utils::jsonParser::jValue j, int cameraIndex);
                ~SoftTrigger();
                void getTrigger(int& errc, int& iter);

            private:
                std::mutex mtx_;
                long int swTriggerDelay_;
                nlohmann::json outMessageStringJson_ = nlohmann::json::parse(R"({"command": "pipeline1"})");
                std::chrono::steady_clock::time_point start_timeout_ = std::chrono::steady_clock::now();
                std::chrono::steady_clock::time_point end_timeout_ = std::chrono::steady_clock::now();
                std::chrono::duration<double> duration_timeout_;
                std::chrono::steady_clock::time_point trigger_start_time_ = std::chrono::steady_clock::now();
                std::chrono::steady_clock::time_point trigger_end_time_ = std::chrono::steady_clock::now();
                std::chrono::duration<double> trigger_elapsed_seconds_;
        };

    }
}

#endif