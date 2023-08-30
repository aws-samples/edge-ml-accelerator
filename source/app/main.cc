/**
 * @main.cc
 * @brief Pipeline App
 *
 * This is the main Pipeline App for running the complete pipeline using
 * Capture, Inference, Output and Utils
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

using namespace edgeml::pipeline;

int main(int argc, char *argv[])
{
    if (argc!=2)
    {
        LOG_ALWAYS("[EdgeMLAccelerator::PipelineApp] Expecting 1 argument as follows:");
        LOG_ALWAYS("[EdgeMLAccelerator::PipelineApp] $ ./pipeline_app 10");
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(3000));

    LOG_ALWAYS("[EdgeMLAccelerator::PipelineApp] Running Pipeline App");

    const char *configFileEnvVar = std::getenv("EDGE_ML_CONFIG"); // read environment variable for the config file
    if (configFileEnvVar==NULL)
    {
        LOG_ALWAYS("[EdgeMLAccelerator::PipelineApp] EDGE_ML_CONFIG is not defined as environment variable");
        LOG_ALWAYS("[EdgeMLAccelerator::PipelineApp] Define it as follows:");
        LOG_ALWAYS("[EdgeMLAccelerator::PipelineApp] export EDGE_ML_CONFIG=/path/to/config.json -or- EDGE_ML_CONFIG=/path/to/config.yaml");
        LOG_ALWAYS("[EdgeMLAccelerator::PipelineApp] Exiting ...");
        return 0;
    }

    std::string configFileEnvVarStr(configFileEnvVar);
    jsonParser::jValue jsonParams_;
    if ((configFileEnvVarStr.substr(configFileEnvVarStr.find_last_of(".") + 1) == "json") || (configFileEnvVarStr.substr(configFileEnvVarStr.find_last_of(".") + 1) == "JSON"))
    {
        LOG_ALWAYS("[EdgeMLAccelerator::PipelineApp] Using JSON file");
        std::ifstream configFile(configFileEnvVar); // read the config file
        std::string jsonParamsStr1 = "", jsonParamsStr2;
        while (getline(configFile, jsonParamsStr2))
        {
            jsonParamsStr1 += jsonParamsStr2; // read the file line by line
        }
        jsonParams_ = jsonParser::parser::parse(jsonParamsStr1); // parse the json file using the jsonParser
    }
    else if ((configFileEnvVarStr.substr(configFileEnvVarStr.find_last_of(".") + 1) == "yaml") || (configFileEnvVarStr.substr(configFileEnvVarStr.find_last_of(".") + 1) == "YAML") || (configFileEnvVarStr.substr(configFileEnvVarStr.find_last_of(".") + 1) == "yml") || (configFileEnvVarStr.substr(configFileEnvVarStr.find_last_of(".") + 1) == "YML"))
    {
        LOG_ALWAYS("[EdgeMLAccelerator::PipelineApp] Using YAML file");
        yamlParser yaml;
        jsonParams_ = yaml.yaml2json(configFileEnvVar);
    }

    std::vector<std::unique_ptr<Pipeline>> pPipelinePtrVec;
    std::vector<std::thread> pipelineThreadVec;

    // Run the test for N times and find average time taken
    int N = std::atoi(argv[1]);
    auto start_time = std::chrono::steady_clock::now();

    if (jsonParams_["capture"].size()==0)
    {
        LOG_ERROR("[EdgeMLAccelerator::PipelineApp] ERROR: NO CAPTURE AVAILABLE");
        return 0;
    }

    int cameraIndex = -1, subpipelineIndex = -1; // Initialize the camera and inference indices

    for (cameraIndex=0; cameraIndex<jsonParams_["capture"].size(); cameraIndex++)
    {
        pPipelinePtrVec.push_back(std::make_unique<Pipeline>(jsonParams_, cameraIndex, N));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));

    for (int pipelineIndex=0; pipelineIndex<pPipelinePtrVec.size(); pipelineIndex++)
    {
        std::thread th = pPipelinePtrVec[pipelineIndex]->memberThread();
        pipelineThreadVec.push_back(std::move(th));
    }

    for (auto& t : pipelineThreadVec)
    {
        if (t.joinable())
        {
            t.join();
        }
    }

    auto end_time = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = (end_time - start_time)/N;

    return 0;
}