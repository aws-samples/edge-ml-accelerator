/**
 * @pipeline.h
 * @brief Pipeline class for adding all stages of pipeline
 *
 * This contains the prototypes of adding different plugins into a single pipeline
 *
 */

#ifndef __PIPELINE_H__
#define __PIPELINE_H__

#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <atomic>
#include <thread>
#include <chrono>
#include <map>

#include <edge-ml-accelerator/utils/json_parser.h>
#include <edge-ml-accelerator/utils/yaml_parser.h>
#include <edge-ml-accelerator/utils/image_preprocess.h>
#include <edge-ml-accelerator/utils/result_postprocess.h>
#include <edge-ml-accelerator/utils/edge_ml_config.h>
#include <edge-ml-accelerator/utils/logger.h>

#ifdef WITH_MIC730AI
#include <edge-ml-accelerator/utils/mic730ai_dio.h>
#endif

#include <edge-ml-accelerator/trigger/base_trigger.h>
#include <edge-ml-accelerator/trigger/soft_trigger.h>
#include <edge-ml-accelerator/trigger/gpio_trigger.h>
#include <edge-ml-accelerator/trigger/ipc_trigger.h>
#include <edge-ml-accelerator/trigger/mqtt_trigger.h>

#include <edge-ml-accelerator/inference/lfve_client.h>
#include <edge-ml-accelerator/inference/edgemanager_client.h>
#include <edge-ml-accelerator/inference/triton_client.h>
#include <edge-ml-accelerator/inference/onnxruntime_client.h>

#include <edge-ml-accelerator/output/local_disk.h>
#include <edge-ml-accelerator/output/s3_upload.h>
#include <edge-ml-accelerator/output/publish_ipc_topic.h>
#include <edge-ml-accelerator/output/publish_mqtt_topic.h>

#include <edge-ml-accelerator/capture/base_capture.h>
#include <edge-ml-accelerator/capture/opencv_capture.h>
#ifdef WITH_GENICAM
#include <edge-ml-accelerator/capture/genicam_capture.h>
#endif
#ifdef WITH_PYLON
#include <edge-ml-accelerator/capture/pylon_capture.h>
#endif

using namespace edgeml::utils;
using namespace edgeml::trigger;
using namespace edgeml::capture;
using namespace edgeml::inference;
using namespace edgeml::output;

namespace edgeml
{
    namespace pipeline
    {

        class Pipeline
        {
        public:
                Pipeline(jsonParser::jValue j, int cameraIndex, int numIter);
                ~Pipeline();

                void createPipeline();
                void runPipeline();
                std::thread memberThread();

        private:
                jsonParser::jValue jsonParams_;
                int cameraIndex_;

                Trigger* pTrigger;

                Capture* pCapture;
                unsigned char* frameBuffer;
                int height = 0, width = 0, frameBufferSize = 0;

                std::vector<LFVEclient*> pLFVEclientVec;
                std::vector<EdgeManagerClient*> pEdgeManagerClientVec;
                std::vector<TritonClient*> pTritonClientVec;
                std::vector<OnnxRuntimeClient*> pOnnxRuntimeClientVec;

                std::vector<LocalDisk*> pLocalDiskVec;
                std::vector<S3Upload*> pS3UploadVec;
                std::vector<PublishToIpc*> pPublishToIpcVec;
                std::vector<PublishToMqtt*> pPublishToMqttVec;

                LocalDisk* pLocalDisk;
                S3Upload* pS3Upload;
                PublishToIpc* pPublishToIpc;
                PublishToMqtt* pPublishToMqtt;

                // Run the test for N times and find average time taken
                int N = 0, iter = 0, ret = -100;
                bool completed = false;

                // OpenCV, Pylon, GenICam or GStreamer
                bool isOpencv = false, isPylon = false, isGenicam = false, isGstreamer = false;

                // LFVE or EdgeManager
                bool isLFVE = false, isEdgeManager = false, isTritonClient = false, isOnnxRuntime = false;

                // Local, Publish or S3
                bool isLocalsave = false, isPublishIpctopic = false, isPublishMqtttopic = false, isS3upload = false;

                std::thread triggerThread, captureThread, localsaveThread, s3uploadThread, publishToIpcThread, publishToMqttThread;
                std::vector<std::thread> inferLFVEThreadVec, inferEMThreadVec, outputThreadVec_, inferTritonThreadVec_, inferOnnxThreadVec_;
                std::thread pipelineThread;
                std::vector<Output*> outputs_vec_;
        };

    }
}

#endif
