/**
 * @s3_upload.cc
 * @brief Uploading data to S3
 *
 * This contains the function definitions for creating and running AWS S3 APIs.
 *
 */

#include <edge-ml-accelerator/output/s3_upload.h>
#include <unistd.h>

namespace edgeml
{
  namespace output
  {

    /**
      Instance of the class
    */
    S3Upload* S3Upload::instance(jsonParser::jValue j, SharedMessage<MessageCaptureInference> &incoming_message)
    {
      static S3Upload* inst = 0;

      if (!inst)
      {
        inst = new S3Upload(j, incoming_message);
      }
      return inst;
    }

    /**
      Creates the class constructor
    */
    S3Upload::S3Upload(jsonParser::jValue j, SharedMessage<MessageCaptureInference> &incoming_message) : Output(j, incoming_message)
    {
      Aws::InitAPI(options);
      local_path = getLocalPath();
      region = getAWSRegion();
      getS3BucketAndKey(bucket, key);
    }

    /**
      Creates the class destructor
    */
    S3Upload::~S3Upload()
    {
      Aws::ShutdownAPI(options);
    }

    /**
      Routine for listing the buckets
      @return error code based on success/failure
    */
    int S3Upload::listBuckets()
    {
      LOG_ALWAYS("[OUTPUT::S3] Listing Buckets");
      Aws::S3::S3Client s3_client;
      Aws::S3::Model::ListBucketsOutcome outcome = s3_client.ListBuckets();

      if (outcome.IsSuccess())
      {
        LOG_ALWAYS("[OUTPUT::S3] Bucket names:");

        Aws::Vector<Aws::S3::Model::Bucket> buckets = outcome.GetResult().GetBuckets();

        for (Aws::S3::Model::Bucket& bucket : buckets)
        {
          LOG_ALWAYS(" |- " + std::string(bucket.GetName()));
        }
            return S3UPLOAD_SUCCESS;
      }
      else
      {
        LOG_ALWAYS("[OUTPUT::S3] Error: ListBuckets: " + std::string(outcome.GetError().GetMessage()));
            return S3UPLOAD_FAILURE;
      }
    }

    /**
      Routine for listing the objects in a bucket
      @param bucket passing the bucket whose objects are to be listed
      @return error code based on success/failure
    */
    int S3Upload::listBucketKeys()
    {
      LOG_ALWAYS("[OUTPUT::S3] Listing Bucket Keys");
      Aws::Client::ClientConfiguration config;

      config.region = region;
      Aws::S3::S3Client s3_client(config);

      Aws::S3::Model::ListObjectsRequest request;
      request.WithBucket(bucket);

      auto outcome = s3_client.ListObjects(request);

      if (outcome.IsSuccess())
      {
        LOG_ALWAYS("[OUTPUT::S3] Objects in bucket '" + bucket + "':");
        Aws::Vector<Aws::S3::Model::Object> objects = outcome.GetResult().GetContents();
        for (Aws::S3::Model::Object& object : objects)
        {
          LOG_ALWAYS(object.GetKey());
        }
        return S3UPLOAD_SUCCESS;
      }
      else
      {
        LOG_ALWAYS("[OUTPUT::S3] Error: ListObjects: " + outcome.GetError().GetMessage());
        return S3UPLOAD_FAILURE;
      }
    }

    /**
      Routine uploading all files from the local disk location and then deleting the files
      @param errc returning the error code of capture API
      @param completed getting the parameter to know if the other pipeline stages are completed
    */
    void S3Upload::uploadAllFiles(int& errc, bool& completed)
    {
      LOG_ALWAYS("[OUTPUT::S3] Uploading All Files to Bucket");
      std::string baseFilename, fileName;
      struct stat buffer;
      Aws::Client::ClientConfiguration config;
      config.region = region;
      Aws::S3::S3Client s3_client(credentials, config);
      Aws::S3::Model::PutObjectRequest request;
      request.SetBucket(bucket);
      std::string keyFileName = key;
      bool overall_outcome = true;

      struct dirent *entry;
      DIR *dir = opendir(local_path.c_str());

      while (1)
      {
        auto message = incoming_message_.GetMessage();

        if (message.image_file_names_ != "")
        {
          baseFilename = message.image_file_names_;
          fileName = local_path + "/" + baseFilename;
          // Verify that the file exists.
          if (stat(fileName.c_str(), &buffer) == -1)
          {
              LOG_ALWAYS("[OUTPUT::S3] Error: PutObject: File '" + fileName + "' does not exist.");
              errc = S3UPLOAD_FAILURE;
          }
          if (getFileExt(baseFilename)==".png" || getFileExt(baseFilename)==".jpg" || getFileExt(baseFilename)==".jpeg" || getFileExt(baseFilename)==".csv")
          {
            keyFileName = key;
            keyFileName.append("/"); keyFileName.append(baseFilename);
            request.SetKey(keyFileName);
            std::shared_ptr<Aws::IOStream> input_data = Aws::MakeShared<Aws::FStream>("SampleAllocationTag", fileName.c_str(), std::ios_base::in | std::ios_base::binary);
            request.SetBody(input_data);
            Aws::S3::Model::PutObjectOutcome outcome = s3_client.PutObject(request);
            if (outcome.IsSuccess())
            {
              LOG_ALWAYS("[OUTPUT::S3] Added object '" + baseFilename + "' to bucket '" + bucket + "' with key '" + key + "'.");
              std::remove(fileName.c_str());
              overall_outcome = overall_outcome & true;
              uploadRetries = 0;
            }
            else
            {
              LOG_ALWAYS("[OUTPUT::S3] Error: PutObject: " +  outcome.GetError().GetMessage());
              overall_outcome = overall_outcome & false;
            }
            if (overall_outcome)
            {
              errc = S3UPLOAD_SUCCESS;
            }
            else
            {
              errc = S3UPLOAD_FAILURE;
            }
          }
        }

        if(produce_output_)
        {
          output_message_.produce_message(message);
          LOG_ALWAYS("[PIPELINE::GENERAL] Message Size = " + std::to_string(output_message_.size()));
        }
      }
    }

    /**
      Routine uploading a file to a bucket and a key location
      @param fileName passing the local filename which needs to be uploaded
      @return error code based on success/failure
    */
    int S3Upload::uploadFile(std::string fileName)
    {
      std::string baseFilename = fileName.substr(fileName.find_last_of("/\\") + 1);
      LOG_ALWAYS("[OUTPUT::S3] Uploading File to Bucket");
      // Verify that the file exists.
      struct stat buffer;
      if (stat(fileName.c_str(), &buffer) == -1)
      {
          LOG_ALWAYS("[OUTPUT::S3] Error: PutObject: File '" + fileName + "' does not exist.");
          return S3UPLOAD_FAILURE;
      }

      std::string outFileName = getOutputFileName();
      outFileName.append(getFileExt(baseFilename));

      Aws::Client::ClientConfiguration config;
      config.region = region;
      Aws::S3::S3Client s3_client(config);
      Aws::S3::Model::PutObjectRequest request;
      request.SetBucket(bucket);
      std::string keyFileName = key;
      keyFileName.append("/"); keyFileName.append(outFileName);
      request.SetKey(keyFileName);
      std::shared_ptr<Aws::IOStream> input_data = Aws::MakeShared<Aws::FStream>("SampleAllocationTag", fileName.c_str(), std::ios_base::in | std::ios_base::binary);
      request.SetBody(input_data);
      Aws::S3::Model::PutObjectOutcome outcome = s3_client.PutObject(request);

      if (outcome.IsSuccess())
      {
        LOG_ALWAYS("[OUTPUT::S3] Added object '" + outFileName + "' to bucket '" + bucket + "' with key '" + key + "'.");
        return S3UPLOAD_SUCCESS;
      }
      else
      {
        LOG_ALWAYS("[OUTPUT::S3] Error: PutObject: " + outcome.GetError().GetMessage());
        return S3UPLOAD_FAILURE;
      }
    }

  }
}
