/**
 * @pipeline.cc
 * @brief Pipeline class with routines for adding all stages of pipeline
 *
 * This contains the routines of adding different plugins into a single pipeline
 *
 */

#include <edge-ml-accelerator/pipeline/pipeline.h>

namespace edgeml
{
    namespace pipeline
    {

        /**
        Creates the class constructor
        */
        Pipeline::Pipeline(jsonParser::jValue j, int cameraIndex, int numIter) : jsonParams_(j), cameraIndex_(cameraIndex)
        {
            LOG_ALWAYS("[PIPELINE::GENERAL] Creating Pipeline.");

            N = numIter;

            // Set the trigger
            if (jsonParams_["capture"][cameraIndex]["useGpioTrigger"].as_bool())
            {
                LOG_ALWAYS("[PIPELINE::Trigger] Using GPIO Trigger.");
                pTrigger = new GpioTrigger(jsonParams_, cameraIndex);
            }
            else if (jsonParams_["capture"][cameraIndex]["useIpcTrigger"].as_bool())
            {
                LOG_ALWAYS("[PIPELINE::Trigger] Using IPC Trigger.");
                pTrigger = new IpcTrigger(jsonParams_, cameraIndex);
            }
            else if (jsonParams_["capture"][cameraIndex]["useMqttTrigger"].as_bool())
            {
                LOG_ALWAYS("[PIPELINE::Trigger] Using MQTT Trigger.");
                pTrigger = new MqttTrigger(jsonParams_, cameraIndex);
            }
            else
            {
                LOG_ALWAYS("[PIPELINE::Trigger] Using Software Trigger.");
                pTrigger = new SoftTrigger(jsonParams_, cameraIndex);
            }

            // Using OpenCV Capture API
            if (jsonParams_["capture"][cameraIndex]["cameraType"].as_string()=="OPENCV")
            {
                LOG_ALWAYS("[PIPELINE::Capture] Starting test with OpenCV Capture.");
                pCapture = new OpenCVCapture(jsonParams_, cameraIndex, pTrigger->trigger2camera_);
                isOpencv = true;
            }

            // Using GStreamer Capture API
            if (jsonParams_["capture"][cameraIndex]["cameraType"].as_string()=="GSTREAMER")
            {
                LOG_ALWAYS("[PIPELINE::Capture] Starting test with GStreamer Capture.");
                pCapture = new OpenCVCapture(jsonParams_, cameraIndex, pTrigger->trigger2camera_);
                isGstreamer = true;
            }

#ifdef WITH_GENICAM
            // Using GenICam Capture API
            if (jsonParams_["capture"][cameraIndex]["cameraType"].as_string()=="GENICAM")
            {
                LOG_ALWAYS("[PIPELINE::Capture] Starting test with GenICam Capture.");
                pCapture = new GenicamCapture(jsonParams_, cameraIndex, pTrigger->trigger2camera_);
                isGenicam = true;
            }
#endif

#ifdef WITH_PYLON
            // Using Pylon Capture API
            if (jsonParams_["capture"][cameraIndex]["cameraType"].as_string()=="PYLON")
            {
                LOG_ALWAYS("[PIPELINE::Capture] Starting test with Pylon Capture.");
                pCapture = new PylonCapture(jsonParams_, cameraIndex, pTrigger->trigger2camera_);
                isPylon = true;
            }
#endif

            pCapture->initCapture(cameraIndex);
            height = pCapture->getInputHeight();
            width = pCapture->getInputWidth();

            frameBufferSize = height * width * 3;
            frameBuffer = new unsigned char[frameBufferSize]();

            // All Inferences defined
            std::vector<std::string> inferenceNamesVec;
            for (int inferenceIndex=0; inferenceIndex<jsonParams_["inference"].size(); inferenceIndex++)
            {
                LOG_ALWAYS("[PIPELINE::Inference] Starting test with Inference: " + jsonParams_["inference"][inferenceIndex]["inferName"].as_string());
                inferenceNamesVec.push_back(jsonParams_["inference"][inferenceIndex]["inferName"].as_string());
            }

            // All Output Sinks defined
            std::vector<std::string> outputSinkNamesVec;
            for (int outputsinkIndex=0; outputsinkIndex<jsonParams_["outputsink"].size(); outputsinkIndex++)
            {
                LOG_ALWAYS("[PIPELINE::Output] Starting test with Output: " + jsonParams_["inference"][outputsinkIndex]["outputSinkName"].as_string());
                outputSinkNamesVec.push_back(jsonParams_["outputsink"][outputsinkIndex]["outputSinkName"].as_string());
            }

            // Running for all subpipelines
            for (int subpipelineIndex=0; subpipelineIndex<jsonParams_["capture"][cameraIndex]["subpipelines"].size(); subpipelineIndex++)
            {
                std::string pipelineName_ = jsonParams_["capture"][cameraIndex]["subpipelines"].to_string_key(subpipelineIndex);
                pCapture->camera2forward_.insert(pipelineName_);
                auto tmp_incoming =  pCapture->camera2forward_.GetSharedPointer(pipelineName_);

                LOG_ALWAYS("[PIPELINE::Pipeline] Starting test with Subpipeline: " + pipelineName_);

                // Running for all subsections in the subpipelines
                for (int subsections=0; subsections<jsonParams_["capture"][cameraIndex]["subpipelines"][subpipelineIndex].size(); subsections++)
                {
                    bool is_not_last = false;
                    if(subsections<jsonParams_["capture"][cameraIndex]["subpipelines"][subpipelineIndex].size()-1)
                        is_not_last = true;
                    // Check for Output Sinks
                    ptrdiff_t outputPos = find(outputSinkNamesVec.begin(), outputSinkNamesVec.end(), jsonParams_["capture"][cameraIndex]["subpipelines"][subpipelineIndex][subsections].as_string()) - outputSinkNamesVec.begin();
                    if (outputPos<outputSinkNamesVec.size() && (jsonParams_["outputsink"][outputPos]["outputSinkType"].as_string() == "local"))
                    {
                        pLocalDiskVec.push_back(new LocalDisk(jsonParams_, *tmp_incoming));
                        tmp_incoming = pLocalDiskVec[pLocalDiskVec.size()-1]->GetSharedPointer();
                        pLocalDiskVec[pLocalDiskVec.size()-1]->SetToProduceOutput(is_not_last);
                        isLocalsave = true;
                    }
                    else if (outputPos<outputSinkNamesVec.size() && (jsonParams_["outputsink"][outputPos]["outputSinkType"].as_string() == "s3"))
                    {
                        pS3UploadVec.push_back(new S3Upload(jsonParams_, *tmp_incoming));
                        tmp_incoming = pS3UploadVec[pS3UploadVec.size()-1]->GetSharedPointer();
                        pS3UploadVec[pS3UploadVec.size()-1]->SetToProduceOutput(is_not_last);
                        isS3upload = true;
                    }
                    else if (outputPos<outputSinkNamesVec.size() && (jsonParams_["outputsink"][outputPos]["outputSinkType"].as_string() == "ipctopic"))
                    {
                        pPublishToIpcVec.push_back(new PublishToIpc(jsonParams_, *tmp_incoming));
                        tmp_incoming = pPublishToIpcVec[pPublishToIpcVec.size()-1]->GetSharedPointer();
                        pPublishToIpcVec[pPublishToIpcVec.size()-1]->SetToProduceOutput(is_not_last);
                        isPublishIpctopic = true;
                    }
                    else if (outputPos<outputSinkNamesVec.size() && (jsonParams_["outputsink"][outputPos]["outputSinkType"].as_string() == "mqtttopic"))
                    {
                        pPublishToMqttVec.push_back(new PublishToMqtt(jsonParams_, *tmp_incoming));
                        tmp_incoming = pPublishToMqttVec[pPublishToMqttVec.size()-1]->GetSharedPointer();
                        pPublishToMqttVec[pPublishToMqttVec.size()-1]->SetToProduceOutput(is_not_last);
                        isPublishMqtttopic = true;
                    }

                    // Append Inferences
                    ptrdiff_t inferPos = find(inferenceNamesVec.begin(), inferenceNamesVec.end(), jsonParams_["capture"][cameraIndex]["subpipelines"][subpipelineIndex][subsections].as_string()) - inferenceNamesVec.begin();
                    if (inferPos<inferenceNamesVec.size())
                    {
                        if (jsonParams_["inference"][inferPos]["inferType"].as_string() == "LFVE")
                        {
                            double threshold = jsonParams_["inference"][inferPos]["inferType"]["anomaly_threshold"].as_double();
                            pLFVEclientVec.push_back(new LFVEclient(jsonParams_, inferPos, pipelineName_, threshold, *tmp_incoming));
                            pLFVEclientVec[pLFVEclientVec.size()-1]->SetToProduceOutput(is_not_last);
                            tmp_incoming = pLFVEclientVec[pLFVEclientVec.size()-1]->GetSharedPointer();
                            isLFVE = true;
                        }
                        else if (jsonParams_["inference"][inferPos]["inferType"].as_string() == "EDGEMANAGER")
                        {
                            pEdgeManagerClientVec.push_back(new EdgeManagerClient(jsonParams_, inferPos, pipelineName_, *tmp_incoming));
                            pEdgeManagerClientVec[pEdgeManagerClientVec.size()-1]->SetToProduceOutput(is_not_last);
                            tmp_incoming = pEdgeManagerClientVec[pEdgeManagerClientVec.size()-1]->GetSharedPointer();
                            isEdgeManager = true;
                        }
                        else if (jsonParams_["inference"][inferPos]["inferType"].as_string() == "TRITON")
                        {
                            double threshold = jsonParams_["inference"][inferPos]["inferType"]["anomaly_threshold"].as_double();
                            pTritonClientVec.push_back(new TritonClient(jsonParams_, inferPos, pipelineName_, *tmp_incoming));
                            pTritonClientVec[pTritonClientVec.size()-1]->SetToProduceOutput(is_not_last);
                            tmp_incoming = pTritonClientVec[pTritonClientVec.size()-1]->GetSharedPointer();
                            isTritonClient = true;
                        }
                        else if (jsonParams_["inference"][inferPos]["inferType"].as_string() == "ONNX")
                        {
                            pOnnxRuntimeClientVec.push_back(new OnnxRuntimeClient(jsonParams_, inferPos, pipelineName_, *tmp_incoming));
                            pOnnxRuntimeClientVec[pOnnxRuntimeClientVec.size()-1]->SetToProduceOutput(is_not_last);
                            tmp_incoming = pOnnxRuntimeClientVec[pOnnxRuntimeClientVec.size()-1]->GetSharedPointer();
                            isOnnxRuntime = true;
                        }
                        else
                        {
                            LOG_ERROR("[PIPELINE::Pipeline] Inference pipeline do not exist");
                            exit(1);
                        }
                    }
                    // else
                    // {
                    //     pCaptureContainer_->inferTypeMap.insert({pipelineName_, InferenceInputModeE::NONE});
                    // }
                }
            }
            std::string pipelineName_ = jsonParams_["capture"][cameraIndex]["subpipelines"].to_string_key(0);
        }

        /**
        Creates the class destructor
        */
        Pipeline::~Pipeline()
        {
            if (triggerThread.joinable()) {triggerThread.join();}

            if (captureThread.joinable()) {captureThread.join();}

            for (auto& t : inferLFVEThreadVec)
            {
                if (t.joinable())
                {
                    t.join();
                }
            }

            for (auto& t : inferEMThreadVec)
            {
                if (t.joinable())
                {
                    t.join();
                }
            }

            for (auto& t : inferTritonThreadVec_)
            {
                if (t.joinable())
                {
                    t.join();
                }
            }

            for (auto& t : inferOnnxThreadVec_)
            {
                if (t.joinable())
                {
                    t.join();
                }
            }

            if (localsaveThread.joinable()) {localsaveThread.join();}
            if (s3uploadThread.joinable()) {s3uploadThread.join();}
            if (publishToIpcThread.joinable()) {publishToIpcThread.join();}
            if (publishToMqttThread.joinable()) {publishToMqttThread.join();}

            if (pipelineThread.joinable()) {pipelineThread.join();}
        }

        /**
        Creating the pipeline
        */
        void Pipeline::createPipeline()
        {
            triggerThread = std::thread(&Trigger::getTrigger, pTrigger, std::ref(ret), std::ref(N));
            LOG_ALWAYS("[PIPELINE::Trigger] Created TRIGGER Thread.");

            captureThread = std::thread(&Capture::getCapture, pCapture, std::ref(ret), std::ref(frameBuffer), std::ref(frameBufferSize), std::ref(N)); // in order to get image data from camera capture
            LOG_ALWAYS("[PIPELINE::Capture] Created CAPTURE Thread.");

            for (int lfveindex=0; lfveindex<pLFVEclientVec.size(); lfveindex++)
            {
                inferLFVEThreadVec.push_back(std::thread(&LFVEclient::runInference, pLFVEclientVec[lfveindex], std::ref(ret), std::ref(height), std::ref(width), std::ref(N), std::ref(completed)));
                LOG_ALWAYS("[PIPELINE::Inference::LFVE] Created LFVE Threads.");
            }

            for (int emindex=0; emindex<pEdgeManagerClientVec.size(); emindex++)
            {
                inferEMThreadVec.push_back(std::thread(&EdgeManagerClient::runInference, pEdgeManagerClientVec[emindex], std::ref(ret), std::ref(height), std::ref(width), std::ref(N), std::ref(completed)));
                LOG_ALWAYS("[PIPELINE::Inference::EDGEMANAGER] Created EDGEMANAGER Threads.");
            }

            for (int tritonex=0; tritonex<pTritonClientVec.size(); tritonex++)
            {
                inferTritonThreadVec_.push_back(std::thread(&TritonClient::runInference, pTritonClientVec[tritonex], std::ref(ret), std::ref(height), std::ref(width), std::ref(N), std::ref(completed)));
                LOG_ALWAYS("[PIPELINE::Inference::TRITONCLIENT] Created TRITONCLIENT Threads.");
            }

            for (int onnxex=0; onnxex<pOnnxRuntimeClientVec.size(); onnxex++)
            {
                inferOnnxThreadVec_.push_back(std::thread(&OnnxRuntimeClient::runInference, pOnnxRuntimeClientVec[onnxex], std::ref(ret), std::ref(height), std::ref(width), std::ref(N), std::ref(completed)));
                LOG_ALWAYS("[PIPELINE::Inference::ONNXRUNTIME] Created ONNXRUNTIME Threads.");
            }

            for (auto output_idx = 0; output_idx < pLocalDiskVec.size(); output_idx++)
            {
                outputThreadVec_.push_back(std::thread(&LocalDisk::saveImageAsPNGorJPG, pLocalDiskVec[output_idx], std::ref(ret), std::ref(height), std::ref(width), std::ref(completed)));
                LOG_ALWAYS("[PIPELINE::Output::LOCALDISK] Created LOCALDISK Threads.");
            }

            for (auto output_idx = 0; output_idx < pPublishToIpcVec.size(); output_idx++)
            {
                outputThreadVec_.push_back(std::thread(&PublishToIpc::publishToTopic, pPublishToIpcVec[output_idx], std::ref(completed)));
                LOG_ALWAYS("[PIPELINE::Output::PUBLISH2IPC] Created PUBLISH2IPC Threads.");
            }

            for (auto output_idx = 0; output_idx < pPublishToMqttVec.size(); output_idx++)
            {
                outputThreadVec_.push_back(std::thread(&PublishToMqtt::publishToTopic, pPublishToMqttVec[output_idx], std::ref(completed)));
                LOG_ALWAYS("[PIPELINE::Output::PUBLISH2MQTT] Created PUBLISH2MQTT Threads.");
            }

            for (auto output_idx = 0; output_idx < pS3UploadVec.size(); output_idx++)
            {
                outputThreadVec_.push_back(std::thread(&S3Upload::uploadAllFiles, pS3UploadVec[output_idx], std::ref(ret), std::ref(completed)));
                LOG_ALWAYS("[PIPELINE::Output::S3UPLOAD] Created S3UPLOAD Threads.");
            }


            pipelineThread = std::thread(&Pipeline::runPipeline, this);
            LOG_ALWAYS("[PIPELINE::Pipeline] Created PIPELINE Thread.");
        }

        /**
        Running the pipeline
        */
        void Pipeline::runPipeline()
        {
            while (1)
            {
                if (completed)
                    break;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }

        /**
        Creating a thread for the createPipeline routine
        @return std:thread which will be used for multi threading
        */
        std::thread Pipeline::memberThread()
        {
            return std::thread( [this] { createPipeline(); } );
        }

    }
}
