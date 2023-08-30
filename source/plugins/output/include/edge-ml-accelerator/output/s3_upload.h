/**
 * @s3_upload.h
 * @brief Uploading data to S3 Bucket
 *
 * This contains the prototypes for creating and running AWS S3 APIs.
 *
 */

#ifndef __S3_UPLOAD_H__
#define __S3_UPLOAD_H__

#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <atomic>
#include <thread>
#include <chrono>
#include <mutex>
#include <sys/stat.h>
#include <dirent.h>

#include <aws/s3/S3Client.h>
#include <aws/s3/model/Bucket.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/ListObjectsRequest.h>
#include <aws/core/Aws.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <aws/core/auth/AWSCredentialsProvider.h>

#include <edge-ml-accelerator/output/base_output.h>

using namespace edgeml::utils;

namespace edgeml
{
    namespace output
    {

        class S3Upload : public Output
        {
            public:
                static S3Upload* instance(jsonParser::jValue j, SharedMessage<MessageCaptureInference> &incoming_message);

                S3Upload(jsonParser::jValue j, SharedMessage<MessageCaptureInference> &incoming_message);
                ~S3Upload();
                int listBuckets();
                int listBucketKeys();
                void uploadAllFiles(int& errc, bool& completed);
                int uploadFile(std::string fileName);

            private:
                int uploadRetries = 0, maxUploadRetries = 10;
                Aws::SDKOptions options;
                Aws::Auth::AWSCredentials credentials;
                std::string region, bucket, key, local_path;
                std::mutex mtx_;
                std::chrono::steady_clock::time_point start_timeout = std::chrono::steady_clock::now();
                std::chrono::steady_clock::time_point end_timeout = std::chrono::steady_clock::now();
                std::chrono::duration<double> duration_timeout;
                GPIO gpio;
                std::string TYPE = "S3_UPLOAD";
        };

    }
}

#endif