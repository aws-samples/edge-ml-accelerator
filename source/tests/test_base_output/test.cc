/**
 * @test.cc
 * @brief Unit Test for running Base Output API
 *
 * This contains the test for running Base Output.
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

    LOG_ALWAYS("[TESTS::OUTPUT::BASE] Starting Unit Tests for Base Output.");

    const char *configFileEnvVar = "all_test_configs/TEST_CONFIG_BASE_OUTPUT.json"; // config file
    std::string configFileEnvVarStr(configFileEnvVar);
    jsonParser::jValue jsonParams_;
    std::ifstream configFile(configFileEnvVar); // read the config file
    std::string jsonParamsStr1 = "", jsonParamsStr2;
    while (getline(configFile, jsonParamsStr2)) jsonParamsStr1 += jsonParamsStr2; // read the file line by line
    jsonParams_ = jsonParser::parser::parse(jsonParamsStr1); // parse the json file using the jsonParser

    Trigger* pTrigger;
    Capture* pCapture;
    std::vector<Output*> pOutputVec;

    // Using OpenCV Capture API
    int cameraIndex = 0;
    pTrigger = new SoftTrigger(jsonParams_, cameraIndex);
    pCapture = new Capture(jsonParams_, cameraIndex, pTrigger->trigger2camera_);
    int ret;

    for (int subpipelineIndex=0; subpipelineIndex<jsonParams_["capture"][cameraIndex]["subpipelines"].size(); subpipelineIndex++)
    {
        std::string pipelineName_ = jsonParams_["capture"][cameraIndex]["subpipelines"].to_string_key(subpipelineIndex);
        LOG_ALWAYS("[TESTS::OUTPUT::BASE] For SubPipeline: " + pipelineName_);

        pCapture->camera2forward_.insert(pipelineName_);
        auto tmp_incoming =  pCapture->camera2forward_.GetSharedPointer(pipelineName_);
        pOutputVec.push_back(new Output(jsonParams_, *tmp_incoming));

        assert(typeid(pOutputVec[subpipelineIndex]->GetType())==typeid(std::string));
        LOG_ALWAYS("[TESTS::OUTPUT::BASE] Successfully tested BaseOutput->GetType()");

        assert(typeid(pOutputVec[subpipelineIndex]->getAWSRegion())==typeid(std::string));
        LOG_ALWAYS("[TESTS::OUTPUT::BASE] Successfully tested BaseOutput->getAWSRegion()");

        assert(typeid(pOutputVec[subpipelineIndex]->getLocalPath())==typeid(std::string));
        LOG_ALWAYS("[TESTS::OUTPUT::BASE] Successfully tested BaseOutput->getLocalPath()");

        assert(typeid(pOutputVec[subpipelineIndex]->getImageFormat())==typeid(std::string));
        LOG_ALWAYS("[TESTS::OUTPUT::BASE] Successfully tested BaseOutput->getImageFormat()");

        assert(typeid(pOutputVec[subpipelineIndex]->getOutputFileName())==typeid(std::string));
        LOG_ALWAYS("[TESTS::OUTPUT::BASE] Successfully tested BaseOutput->getOutputFileName()");

        assert(typeid(pOutputVec[subpipelineIndex]->getFileExt("test.ext")==typeid(std::string)));
        LOG_ALWAYS("[TESTS::OUTPUT::BASE] Successfully tested BaseOutput->getFileExt()");

        assert(typeid(pOutputVec[subpipelineIndex]->getS3Bucket()==typeid(std::string)));
        LOG_ALWAYS("[TESTS::OUTPUT::BASE] Successfully tested BaseOutput->getS3Bucket()");

        assert(typeid(pOutputVec[subpipelineIndex]->getS3Key()==typeid(std::string)));
        LOG_ALWAYS("[TESTS::OUTPUT::BASE] Successfully tested BaseOutput->getS3Key()");

        assert(typeid(pOutputVec[subpipelineIndex]->getIpcTopicName()==typeid(std::string)));
        LOG_ALWAYS("[TESTS::OUTPUT::BASE] Successfully tested BaseOutput->getIpcTopicName()");

        assert(typeid(pOutputVec[subpipelineIndex]->getMqttTopicName()==typeid(std::string)));
        LOG_ALWAYS("[TESTS::OUTPUT::BASE] Successfully tested BaseOutput->getMqttTopicName()");

        assert(typeid(pOutputVec[subpipelineIndex]->getColorSpace()==typeid(std::string)));
        LOG_ALWAYS("[TESTS::OUTPUT::BASE] Successfully tested BaseOutput->getColorSpace()");
    }

    delete pTrigger;
    delete pCapture;
    for (int idx=0; idx<pOutputVec.size(); idx++)
    {
        delete pOutputVec[idx];
    }

    return 0;
}