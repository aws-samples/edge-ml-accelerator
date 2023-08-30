/**
 * @image_preprocess.h
 * @brief Utils for pre-processing image before inference
 *
 * This contains the routines of pre-processing image like resize and scaling
 *
 */

#ifndef __IMAGE_PREPROCESS_H__
#define __IMAGE_PREPROCESS_H__

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <algorithm>
#include <cmath>

#include <opencv2/opencv.hpp>

#include <edge-ml-accelerator/utils/logger.h>

namespace edgeml
{
    namespace utils
    {

        class ImagePreProcess
        {
            public:
                ImagePreProcess();
                ~ImagePreProcess();
                std::vector<float> scale(std::vector<unsigned char>& image, int& w, int& h, int& c, int& scaleBy);
                int resize(unsigned char* inputImage, int width, int height, unsigned char* outputImage, int output_width, int output_height, int input_channels);
                int write(const char *filename, int width, int height, int channel, const void *data, std::string colorspace);
            private:
                int ret;
                cv::Mat src, dst;
        };

    }
}

#endif