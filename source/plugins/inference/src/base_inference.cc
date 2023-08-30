/**
 * @base_inference.cc
 * @brief Creating Base Inference Class
 *
 * This contains the function definitions for creating the base inference class that includes
 * routines for setting and getting parameters.
 *
 */

#include <edge-ml-accelerator/inference/base_inference.h>

namespace edgeml
{
    namespace inference
    {

		/**
		Instance of the class
		*/
		Inference* Inference::instance(utils::jsonParser::jValue j, SharedMessage<MessageCaptureInference> & shared_map)
		{
			static Inference* inst = 0;

			if (!inst)
			{
				inst = new Inference(j, shared_map);
			}
			return inst;
		}

		/**
		Creates the class constructor
		*/
		Inference::Inference(utils::jsonParser::jValue j, SharedMessage<MessageCaptureInference> & shared_map) : jsonParams_(j), camera2ongoing_(shared_map)
		{
			LOG_ALWAYS("[INFERENCE::BASE] Base Inference class is created.");

			/* For Lookout for Vision Edge
			{
				"inferenceType": "lfve",
				"inferenceTime": "0.05"
				"inferenceResults":
				{
					"isOriginallyAnomalous": "true/false",
					"isAnomalous": "true/false",
					"confidence": "0.99",
					"anomalyMask": "[[]]"
					"anomalies":
					[
						{
							"totalPercentageArea": "0.5",
							"hexColor": "abcdef",
							"name": "xyz",
							"pixelAnomaly": "..."
						}
					]
				},
				"imageLocation": "s3/datalake/local",
				"resultLocation": "s3/datalake/local"
			}
			*/
			lfveAnomaliesNlohmannJson_["totalPercentageArea"] = "__undefined__";
			lfveAnomaliesNlohmannJson_["hexColor"] = "__undefined__";
			lfveAnomaliesNlohmannJson_["name"] = "__undefined__";
			lfveResultsNlohmannJson_["isOriginallyAnomalous"] = "__undefined__";
			lfveResultsNlohmannJson_["isAnomalous"] = "__undefined__";
			lfveResultsNlohmannJson_["confidence"] = "__undefined__";
			lfveResultsNlohmannJson_["anomalyMask"] = "__undefined__";
			lfveResultsNlohmannJson_["anomalies"] = {lfveAnomaliesNlohmannJson_, lfveAnomaliesNlohmannJson_, lfveAnomaliesNlohmannJson_};

			/* For Custom Model
			{
				"inferenceType": "edgemanager/onnx/triton",
				"inferenceTime": "0.05"
				"inferenceResults":
				{
					"modelType": "segmentation/classification/objectdetection/undefined/none",
					"resultType": "mask/json",
					"results": "[[]]"
				},
				"imageLocation": "s3/datalake/local",
				"resultLocation": "s3/datalake/local"
			}
			*/
			customResultsNlohmannJson_["inputShape"] = "__undefined__";
			customResultsNlohmannJson_["outputShape"] = "__undefined__";
			customResultsNlohmannJson_["modelType"] = "__undefined__";
			customResultsNlohmannJson_["resultType"] = "__undefined__";
			customResultsNlohmannJson_["results"] = "__undefined__";

			inferenceBaseInferenceResultsNlohmannJson["inferenceType"] = "__undefined__";
			inferenceBaseInferenceResultsNlohmannJson["inferenceTime"] = "__undefined__";
			inferenceBaseInferenceResultsNlohmannJson["inferenceResults"] = "__undefined__";
			inferenceBaseInferenceResultsNlohmannJson["imageLocation"] = "__undefined__";
			inferenceBaseInferenceResultsNlohmannJson["resultLocation"] = "__undefined__";

			LOG_ALWAYS("[INFERENCE::BASE] Output Format of LFVE:");
			LOG_ALWAYS(std::string(lfveResultsNlohmannJson_.dump()));
			LOG_ALWAYS("[INFERENCE::BASE] Output Format of EdgeManager:");
			LOG_ALWAYS(std::string(customResultsNlohmannJson_.dump()));
		}

		/**
		Creates the class destructor
		*/
		Inference::~Inference()
		{
		}

	}
}
