/**
 * @test.cc
 * @brief Unit Test for running GeniCam Capture API
 *
 * This contains the test for running GeniCam image Capture.
 * The basic test runs on GeniCam camera and gathers output as a byte array.
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

    LOG_ALWAYS("[TESTS::CAPTURE::GENICAM] Starting Unit Tests for GeniCam Capture.");

    const char *configFileEnvVar = "all_test_configs/TEST_CONFIG_GENICAM.json"; // config file
    std::string configFileEnvVarStr(configFileEnvVar);
    jsonParser::jValue jsonParams_;
    std::ifstream configFile(configFileEnvVar); // read the config file
    std::string jsonParamsStr1 = "", jsonParamsStr2;
    while (getline(configFile, jsonParamsStr2)) jsonParamsStr1 += jsonParamsStr2; // read the file line by line
    jsonParams_ = jsonParser::parser::parse(jsonParamsStr1); // parse the json file using the jsonParser

    Trigger* pTrigger;
    Capture* pCapture;

    // Using GeniCam Capture API
    int cameraIndex = 0;
    pTrigger = new SoftTrigger(jsonParams_, cameraIndex);
    pCapture = new GenicamCapture(jsonParams_, cameraIndex, pTrigger->trigger2camera_);
    int ret;

    ret = pCapture->initCapture(cameraIndex);
    assert((ret==CAPTURE_OK || ret==CAMERA_MISSING) && "Camera: " + std::to_string(ret));
    LOG_ALWAYS("[TESTS::CAPTURE::GENICAM] Successfully tested GenicamCapture->initCapture()");

    delete pTrigger;
    delete pCapture;

    return 0;
}