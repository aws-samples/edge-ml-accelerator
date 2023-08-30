/**
 * @test.cc
 * @brief Unit Test for running Base Capture API
 *
 * This contains the test for running Base image Capture.
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

    LOG_ALWAYS("[TESTS::CAPTURE::BASE] Starting Unit Tests for Base Capture.");

    const char *configFileEnvVar = "all_test_configs/TEST_CONFIG_BASE_CAPTURE.json"; // config file
    std::string configFileEnvVarStr(configFileEnvVar);
    jsonParser::jValue jsonParams_;
    std::ifstream configFile(configFileEnvVar); // read the config file
    std::string jsonParamsStr1 = "", jsonParamsStr2;
    while (getline(configFile, jsonParamsStr2)) jsonParamsStr1 += jsonParamsStr2; // read the file line by line
    jsonParams_ = jsonParser::parser::parse(jsonParamsStr1); // parse the json file using the jsonParser

    Trigger* pTrigger;
    Capture* pCapture;

    // Using OpenCV Capture API
    int cameraIndex = 0;
    pTrigger = new SoftTrigger(jsonParams_, cameraIndex);
    pCapture = new Capture(jsonParams_, cameraIndex, pTrigger->trigger2camera_);
    int ret;

    assert(typeid(pCapture->getInputHeight())==typeid(int));
    LOG_ALWAYS("[TESTS::CAPTURE::BASE] Successfully tested BaseCapture->getInputHeight()");

    assert(typeid(pCapture->getInputWidth())==typeid(int));
    LOG_ALWAYS("[TESTS::CAPTURE::BASE] Successfully tested BaseCapture->getInputWidth()");

    assert(typeid(pCapture->getResizeInputHeight())==typeid(int));
    LOG_ALWAYS("[TESTS::CAPTURE::BASE] Successfully tested BaseCapture->getResizeInputHeight()");

    assert(typeid(pCapture->getResizeInputWidth())==typeid(int));
    LOG_ALWAYS("[TESTS::CAPTURE::BASE] Successfully tested BaseCapture->getResizeInputWidth()");

    assert(typeid(pCapture->getColorSpace())==typeid(std::string));
    LOG_ALWAYS("[TESTS::CAPTURE::BASE] Successfully tested BaseCapture->getColorSpace()");

    assert(typeid(pCapture->getHardwareTriggerDelay())==typeid(long int));
    LOG_ALWAYS("[TESTS::CAPTURE::BASE] Successfully tested BaseCapture->getHardwareTriggerDelay()");

    assert(typeid(pCapture->getSoftwareTriggerDelay())==typeid(long int));
    LOG_ALWAYS("[TESTS::CAPTURE::BASE] Successfully tested BaseCapture->getSoftwareTriggerDelay()");

    assert(typeid(pCapture->getIsGpioCaptureTrigger())==typeid(bool));
    LOG_ALWAYS("[TESTS::CAPTURE::BASE] Successfully tested BaseCapture->getIsGpioCaptureTrigger()");

    assert(typeid(pCapture->getGpioCaptureTriggerDelay())==typeid(long int));
    LOG_ALWAYS("[TESTS::CAPTURE::BASE] Successfully tested BaseCapture->getGpioCaptureTriggerDelay()");

    assert(typeid(pCapture->getCameraOperatingMode())==typeid(int));
    LOG_ALWAYS("[TESTS::CAPTURE::BASE] Successfully tested BaseCapture->getCameraOperatingMode()");

    assert(typeid(pCapture->getCameraName())==typeid(std::string));
    LOG_ALWAYS("[TESTS::CAPTURE::BASE] Successfully tested BaseCapture->getCameraName()");

    assert(typeid(pCapture->getNumInferences())==typeid(int));
    LOG_ALWAYS("[TESTS::CAPTURE::BASE] Successfully tested BaseCapture->getNumInferences()");

    assert(typeid(pCapture->getCaptureMode())==typeid(std::string));
    LOG_ALWAYS("[TESTS::CAPTURE::BASE] Successfully tested BaseCapture->getCaptureMode()");

    assert(typeid(pCapture->getExposureTime())==typeid(double));
    LOG_ALWAYS("[TESTS::CAPTURE::BASE] Successfully tested BaseCapture->getExposureTime()");

    assert(typeid(pCapture->getGainValue())==typeid(int));
    LOG_ALWAYS("[TESTS::CAPTURE::BASE] Successfully tested BaseCapture->getGainValue()");

    delete pTrigger;
    delete pCapture;

    return 0;
}