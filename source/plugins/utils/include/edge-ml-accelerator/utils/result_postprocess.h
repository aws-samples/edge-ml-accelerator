/**
 * @result_postprocess.h
 * @brief Utils for postprocessing results before inference
 *
 * This contains the routines of postprocessing of the output results
 *
 */

#ifndef __RESULT_POSTPROCESS_H__
#define __RESULT_POSTPROCESS_H__

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <algorithm>
#include <iostream>
#include <vector>
#include <cmath>

namespace edgeml
{
    namespace utils
    {

        // For Classification
        typedef struct ClassResults
        {
            float score;
            int label;
        } ClassResultsS;

        // For ObjectDetection
        typedef struct BBoxResults
        {
            float x;
            float y;
            float w;
            float h;
            int label;
            float score;
        } BBoxResultsS;

        // For Segmentation
        typedef struct MaskResults
        {
            int w;
            int h;
            unsigned char* mask;
        } MaskResultsS;

        class ResultPostProcess
        {
            public:
                ResultPostProcess();
                ~ResultPostProcess();
                void getClassifyResults(std::vector<float>& outputConf, float* results, std::vector<int> results_shape, float thresh);
                void getBBoxResults(std::vector<std::vector<float> >& outputBboxes, float* results, std::vector<int> models_shape, std::vector<int> results_shape, int height, int width, float thresh);
                MaskResultsS getMaskResults(std::vector<float>& results);

            private:
                int rows, cols;
                float xGain, yGain, x1, y1, x2, y2, confidence;
        };

    }
}

#endif