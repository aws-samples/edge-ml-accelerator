/**
 * @result_postprocess.cc
 * @brief Results postprocessing utils and routines
 *
 * This contains the functions used for postprocessing of the output results
 *
 */

#include <edge-ml-accelerator/utils/result_postprocess.h>

namespace edgeml
{
  namespace utils
  {

    /**
      Creates the class constructor
    */
    ResultPostProcess::ResultPostProcess()
    {
    }

    /**
      Creates the class destructor
    */
    ResultPostProcess::~ResultPostProcess()
    {
    }

    /**
      Routine to postprocess Classification output
      @param outputConf output to be stored as a vector of float
      @param results vector of the output results in float
      @param results_shape vector of the output shape
      @param thresh confidence higher than this is considered
      @return BBoxResultsS
    */
    void ResultPostProcess::getClassifyResults(std::vector<float>& outputConf, float* results, std::vector<int> results_shape, float thresh)
    {
      // Output is of type: 0,1,2,3,... ->confidence
      rows = results_shape[1];
      for (int i=0; i<rows; ++i)
      {
        if (results[i] <= thresh)
          outputConf.push_back(0.0f);
        else
          outputConf.push_back(results[i]);
      }
    }

    /**
      Routine to postprocess ObjectDetection output
      @param outputBboxes output to be stored as a vector of vector of float
      @param results vector of the output results in float
      @param models_shape vector of the model input shape
      @param results_shape vector of the output shape
      @param height of the image
      @param width of the image
      @param thresh confidence higher than this is considered
      @return BBoxResultsS
    */
    void ResultPostProcess::getBBoxResults(std::vector<std::vector<float> >& outputBboxes, float* results, std::vector<int> models_shape, std::vector<int> results_shape, int height, int width, float thresh)
    {
      // Output is of type: 0,1,2,3 ->box, 4->confidence，5-85 -> coco classes confidence
      rows = results_shape[1]; cols = results_shape[2];
      xGain = (float) models_shape[2]/ (float) width;
      yGain = (float) models_shape[1]/ (float) height;
      std::vector<float> output;

      for (int i=0; i<rows; ++i)
      {
        confidence = results[i*cols + 4];
        if ((confidence <= thresh) || (std::isnan(confidence)) || (confidence > 1.0f)) continue;

        for (int j=5; j<cols; ++j)
        {
          if ((results[i*cols + j]*confidence) <= thresh) continue;

          x1 = (results[i*cols + 0] - results[i*cols + 2] / 2) / xGain;
          y1 = (results[i*cols + 1] - results[i*cols + 3] / 2) / yGain;
          x2 = (results[i*cols + 0] + results[i*cols + 2] / 2) / xGain;
          y2 = (results[i*cols + 1] + results[i*cols + 3] / 2) / yGain;

          std::vector<float> output;
          output.push_back(x1); // bbox x1
          output.push_back(y1); // bbox y1
          output.push_back(x2); // bbox x2
          output.push_back(y2); // bbox y2
          output.push_back(j-5); // class
          output.push_back(results[i*cols + j]*confidence); // confidence score

          outputBboxes.push_back(output);
          output.clear();
        }
      }
    }

  }
}