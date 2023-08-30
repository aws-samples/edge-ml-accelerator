/**
 * @edge_ml_config.h
 * @brief Config file to define the parameters to be used in the project
 *
 * This file is used to define the parameters to be used by the plugins like capture, inference, etc.
 *
 */

#ifndef __EDGEML_CONFIG_H__
#define __EDGEML_CONFIG_H__

#pragma once

#include <chrono>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdio>
#include <queue>
#include <stdio.h>
#include <map>
#include <any>
#include <mutex>
#include <condition_variable>
#include <any>
#include <nlohmann/json.hpp>
#include <edge-ml-accelerator/utils/json_parser.h>

/* Error codes */
#define CAPTURE_OK                          (0)     /* No error */
#define IMAGE_FILE_MISSING                  (-1)    /* Image file not present */
#define VIDEO_FILE_MISSING                  (-2)    /* Video file not present */
#define CAMERA_MISSING                      (-3)    /* Camera not available */
#define INVALID_FILE                        (-4)    /* Not a valid file format */
#define RUN_CAPTURE_ERROR                   (-5)    /* Could not read image/video/camera */
#define INFERENCE_OK                        (0)     /* No error */
#define INFERENCE_ERROR                     (-1)    /* Error in inference */
#define MODEL_SUCCESS                       (0)     /* No error in loading/unloading/finding model */
#define MODEL_FAILURE                       (-1)    /* Error in loading/unloading/finding model */
#define BITMAP_EMPTY                        (-2)    /* Image bitmap is empty */
#define S3UPLOAD_SUCCESS                    (0)     /* Upload to S3 is success */
#define S3UPLOAD_FAILURE                    (-1)    /* Upload to S3 is failure */
#define LOCALSAVE_SUCCESS                   (0)     /* Saving to local disk is success */
#define LOCALSAVE_FAILURE                   (-1)    /* Saving to local disk is failure */
#define GPIO_SUCCESS                        (0)     /* GPIO set/get success */
#define GPIO_FAIL                           (-1)    /* Error for GPIO set/get */

/* Define GPIO PINs DI (data input) codes -> Has to be GET */
#define GPIO_CAMERA_TRIGGER                 (0)     /* DI #0 -> Trigger on HIGH */
#define GPIO_DATA_IN_0                      (0)     /* DI #0 */
#define GPIO_DATA_IN_1                      (1)     /* DI #1 */
#define GPIO_DATA_IN_2                      (2)     /* DI #2 */
#define GPIO_DATA_IN_3                      (3)     /* DI #3 */
#define GPIO_DATA_IN_4                      (4)     /* DI #4 */
#define GPIO_DATA_IN_5                      (5)     /* DI #5 */
#define GPIO_DATA_IN_6                      (6)     /* DI #6 */
#define GPIO_DATA_IN_7                      (7)     /* DI #7 */

/* Define GPIO PINs DO (data output) codes -> Has to be SET */
#define GPIO_DATA_OUT_0                     (0)     /* DO #0 */
#define GPIO_DATA_OUT_1                     (1)     /* DO #1 -> Data Valid Output */
#define GPIO_DATA_OUT_2                     (2)     /* DO #2 -> Anomaly Status Output */
#define GPIO_DATA_OUT_3                     (3)     /* DO #3 -> Strobe Light Output */
#define GPIO_DATA_OUT_4                     (4)     /* DO #4 */
#define GPIO_DATA_OUT_5                     (5)     /* DO #5 */
#define GPIO_DATA_OUT_6                     (6)     /* DO #6 */
#define GPIO_DATA_OUT_7                     (7)     /* DO #7 */

/* Define Capture modes */
typedef enum CaptureInputMode
{
    CAMERAMODE = 0,
	IMAGEFILEMODE = 1,
    VIDEOFILEMODE = 2,
    GSTREAMERMODE = 3
} CaptureInputModeE;

/* Define Inference modes */
typedef enum InferenceInputMode
{
    NONE = -1,
    LFVE = 0,
    EDGEMANAGER = 1,
    ONNX = 2,
    TRITON = 3
} InferenceInputModeE;

extern const char *CaptureTypesE[];
extern const char *ModelTypesE[];
extern const char *LfveModelStatusE[];
extern const char *EdgeManagerModelStatusE[];
extern const char *EdgeManagerModelDataTypeE[];

/* Define Output Sink modes */
typedef enum OutputSinkMode
{
    S3BUCKET = 0,
	DYNAMODB = 1,
    LOCALDISK = 2,
    IPCTOPIC = 3
} OutputSinkModeE;

typedef struct
{
    // Trigger
    int triggerRefCounter = -1;
    std::string captureTriggersType = "soft";
    std::queue<bool> captureTriggers; // storing the capture trigger
    std::queue<std::string> captureTriggersMessage; // storing the capture trigger messages for IPC
    std::queue<nlohmann::json> captureTriggersMessageFull; // storing the complete capture trigger messages for IPC
    std::vector<std::string> pipelineNames;
    std::map<std::string, int> numInferences;

    // Capture
    int captureRefCounter = -1;
    std::queue<unsigned char*> safeCaptureContainer; // storing the streaming data
    std::queue<int> safeCaptureSizeContainer; // storing the streaming data size

    // Inference
    std::map<std::string, int> inferTypeMap; // Pipeline to inferType map
    std::map<std::string, std::queue<edgeml::utils::jsonParser::jValue>> inferenceDetailsMap; // details of inference like bounding boxes
    std::map<std::string, std::queue<unsigned char*>> inferenceLFVEDetailsMap; // details of lfve related inference like bounding boxes
    std::map<std::string, std::queue<std::vector<std::vector<float>>>> inferenceEMDetailsMap; // details of edge manager related inference like bounding boxes
    std::queue<std::string> lfve_results;
    std::vector<std::vector<int>> em_models_shape;
    std::vector<std::vector<int>> em_results_shape;
    std::vector<std::string> em_model_type;

    // Output
    int localsaveRefCounter = -1;
    std::queue<std::string> image_file_names; // storing the image file names to upload
    std::queue<std::string> result_file_names; // storing the result file names to upload

    int publishtopicRefCounter = -1;
    std::queue<std::string> topics_to_publish; // storing the topics to be pubilshed

    std::string cameraName, inferenceName;
} ThreadSafeCaptureContainer;

inline bool inList(std::string input, std::vector<std::string> list)
{
    bool return_val = false;
    for(auto k1=0;k1 < list.size();k1++)
    {
        if(input == list[k1] or input.find(list[k1]) != std::string::npos)
            return true;
        }

    return false;
}

inline bool inList(int input, std::vector<int> list)
{
    bool return_val = false;
    for(auto k1=0;k1 < list.size();k1++)
    {
    if(input == list[k1])
        return true;
    }
    return false;
}

struct MessageT2C
{
    nlohmann::json captureTriggersMessageFull_;
    std::string captureTriggersMessage_ , captureTriggersType_ = "soft";
    // bool captureTriggers_;

    void operator = (const MessageT2C &input ) {
         captureTriggersMessageFull_ = input.captureTriggersMessageFull_;
         captureTriggersMessage_ = input.captureTriggersMessage_;
         captureTriggersType_ = input.captureTriggersType_;

    }

};

struct MessageCaptureInference
{
    std::queue<unsigned char*> safeCaptureContainer_; // storing the streaming data
    std::queue<int> safeCaptureSizeContainer_; // storing the streaming data size
    std::queue<std::vector<std::vector<float>>> inferenceEMDetails_;
    nlohmann::json inferenceDetailsMap_;
    std::queue<unsigned char*> inferenceLFVEDetails_;
    std::string lfve_results_;
    std::vector<std::vector<int>> em_models_shape_;
    std::vector<std::vector<int>> em_results_shape_;
    std::vector<std::string> em_model_type_;
    std::string image_file_names_ = "";
    std::string result_file_names_ = "";
    std::string topics_to_publish_;
    //i should just use a message2tc instead
    MessageT2C captureTrigger_;
    std::string cameraName_;
    InferenceInputMode inferenceMode_ = InferenceInputModeE::NONE;

};


template<class T = MessageT2C>
class SharedMessage{
    public:
        std::queue<T> message_queue_;
        std::mutex shared_mutex_;
        std::condition_variable cv_;

        SharedMessage() = default;

        SharedMessage(const SharedMessage &bla)
        {
            message_queue_ = bla.message_queue_;
        }

        void produce_message(T message)
        {
            std::unique_lock<std::mutex> lk(shared_mutex_); //lock variable
            message_queue_.push(message);
            cv_.notify_one();
            lk.unlock();
        }

        T GetMessage() {
            T message;
            std::unique_lock<std::mutex> lock(shared_mutex_);

            cv_.wait(lock, [&](){
                return not message_queue_.empty();
                });
            message = message_queue_.front();
            message_queue_.pop();
            lock.unlock();
            return message;
        }

        int size()
        {
            return message_queue_.size();
        }

        T& operator=(const T& other)
        {
            // Guard self assignment
            if (this == &other)
                return *this;

            // assume *this manages a reusable resource, such as a heap-allocated buffer mArray

            return *this;
        }

        ~SharedMessage() = default;
};

//this is for single input multiple queues
template<class T>
struct SharedMap{
    public:
        void insert(std::string pipeline)
        {
            SharedMessage<T> tmp;
            std::pair<std::string, SharedMessage<T>> bla(pipeline, tmp) ;
            shared_map_.insert(bla);
        }

        bool find(std::string pipeline)
        {
            return shared_map_.count(pipeline) > 0;
        }

    private:
        std::map<std::string, SharedMessage<T>> shared_map_;
    public:

        void produce_message(std::string pipeline, T message)
        {
            shared_map_.at(pipeline).produce_message(message);
        }

        T GetMessage(std::string pipeline)
        {
            // todo handle non-existing pipeline
            return shared_map_.at(pipeline).GetMessage();
        }

        int size_queue(std::string pipeline)
        {
            return shared_map_.at(pipeline).message_queue_.size();
        }

        SharedMessage<T> &GetSharedMessage(std::string pipeline)
        {
            if (find(pipeline))
                return shared_map_[pipeline];
            // todo handle else case
            // return nullptr;
        }

        SharedMessage<T> * GetSharedPointer(std::string pipeline)
        {
            if (find(pipeline))
                return &shared_map_[pipeline];
            return nullptr;
        }

};


#endif
