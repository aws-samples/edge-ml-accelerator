/**
 * @test.cc
 * @brief Unit Test for running Base Trigger API
 *
 * This contains the test for running Base Trigger.
 *
 */

#include <chrono>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdio>
#include <stdio.h>

#include <edge-ml-accelerator/pipeline/pipeline.h>

using namespace edgeml::utils;
using namespace edgeml::trigger;
using namespace edgeml::capture;
using namespace edgeml::inference;
using namespace edgeml::output;

int main(int argc, char *argv[])
{
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    LOG_ALWAYS("[TESTS::TRIGGER::BASE] Starting Unit Tests for Base Trigger.");

    const char *configFileEnvVar = "all_test_configs/TEST_CONFIG_BASE_TRIGGER.json"; // config file
    std::string configFileEnvVarStr(configFileEnvVar);
    jsonParser::jValue jsonParams_;
    std::ifstream configFile(configFileEnvVar); // read the config file
    std::string jsonParamsStr1 = "", jsonParamsStr2;
    while (getline(configFile, jsonParamsStr2)) jsonParamsStr1 += jsonParamsStr2; // read the file line by line
    jsonParams_ = jsonParser::parser::parse(jsonParamsStr1); // parse the json file using the jsonParser

    Trigger* pTrigger;

    // Using OpenCV Capture API
    int cameraIndex = 0;
    pTrigger = new Trigger(jsonParams_, cameraIndex);
    int ret;

    assert(typeid(pTrigger->getSoftwareTriggerDelay())==typeid(long int));
    LOG_ALWAYS("[TESTS::TRIGGER::BASE] Successfully tested BaseTrigger->getSoftwareTriggerDelay()");

    assert(typeid(pTrigger->getHardwareTriggerDelay())==typeid(long int));
    LOG_ALWAYS("[TESTS::TRIGGER::BASE] Successfully tested BaseTrigger->getHardwareTriggerDelay()");

    assert(typeid(pTrigger->getIsGpioTrigger())==typeid(bool));
    LOG_ALWAYS("[TESTS::TRIGGER::BASE] Successfully tested BaseTrigger->getIsGpioTrigger()");

    assert(typeid(pTrigger->getGpioTriggerDelay())==typeid(long int));
    LOG_ALWAYS("[TESTS::TRIGGER::BASE] Successfully tested BaseTrigger->getGpioTriggerDelay()");

    assert(typeid(pTrigger->getIsIpcTrigger())==typeid(bool));
    LOG_ALWAYS("[TESTS::TRIGGER::BASE] Successfully tested BaseTrigger->getIsIpcTrigger()");

    assert(typeid(pTrigger->getIpcTriggerTopicName())==typeid(std::string));
    LOG_ALWAYS("[TESTS::TRIGGER::BASE] Successfully tested BaseTrigger->getIpcTriggerTopicName()");

    assert(typeid(pTrigger->getIsMqttTrigger())==typeid(bool));
    LOG_ALWAYS("[TESTS::TRIGGER::BASE] Successfully tested BaseTrigger->getIsMqttTrigger()");

    assert(typeid(pTrigger->getMqttTriggerTopicName())==typeid(std::string));
    LOG_ALWAYS("[TESTS::TRIGGER::BASE] Successfully tested BaseTrigger->getMqttTriggerTopicName()");

    assert(typeid(pTrigger->getUtcTime())==typeid(std::string));
    LOG_ALWAYS("[TESTS::TRIGGER::BASE] Successfully tested BaseTrigger->getUtcTime()");

    delete pTrigger;

    return 0;
}