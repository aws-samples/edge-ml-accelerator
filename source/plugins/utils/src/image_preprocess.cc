/**
 * @image_preprocess.cc
 * @brief Image preprocessing utils and routines
 *
 * This contains the functions used for preprocessing of image
 *
 */

#include <edge-ml-accelerator/utils/image_preprocess.h>

namespace edgeml
{
  namespace utils
  {

    /**
      Creates the class constructor
    */
    ImagePreProcess::ImagePreProcess()
    {
    }

    /**
      Creates the class destructor
    */
    ImagePreProcess::~ImagePreProcess()
    {
    }

    /**
      Routine for scaling the image values by a scalar
      @param image std::vector of unsigned char that is the input image
      @param w int width
      @param h int height
      @param c int channels
      @param scaleBy int scaling value
      @return std::vector of float that is the scaled image
    */
    std::vector<float> ImagePreProcess::scale(std::vector<unsigned char>& image, int& w, int& h, int& c, int& scaleBy)
    {
        std::vector<float> data;
        const unsigned char* ptr = image.data();
        auto dataSize = w * h * c;

        std::vector<float> in_data;
        for (auto i = 0; i < dataSize; i ++)
        {
            in_data.push_back(float(ptr[i] & 0xff));
        }

        if (c == 3)
        {
            data.resize(w * h);
            for (int i = 0; i < (int)in_data.size(); i = i + 3)
            {
                auto me_ = std::max_element(in_data.begin() + i, in_data.begin() + i + 2);
                float value = *me_;
                data[i / 3] = value / (float)scaleBy;
            }
        }
        else
        {
            LOG_ALWAYS("[Utils::ImagePreProcessing] Support 3 channels bmp only, and the input is " + std::to_string(c) + " channels.");
        }
        return data;
    }

    /**
      Routine for scaling the image values by a scalar
      @param inputImage unsigned char array of input image
      @param width int width
      @param height int height
      @param outputImage unsigned char array of output image
      @param input_width int width for output
      @param input_height int height for output
      @param input_channels int channels
      @return error code based on success/failure
    */
    int ImagePreProcess::resize(unsigned char* inputImage, int width, int height, unsigned char* outputImage, int output_width, int output_height, int input_channels)
    {
      if (input_channels==1)
        src = cv::Mat(cv::Size(width, height), CV_8UC1, (void*)(inputImage));
      else if (input_channels==3)
        src = cv::Mat(cv::Size(width, height), CV_8UC3, (void*)(inputImage));
      cv::resize(src, dst, cv::Size(output_width, output_height), 0, 0, cv::INTER_NEAREST_EXACT);
      outputImage = (unsigned char*)(dst.data);

      return true;
    }

    /**
      Routine for scaling the image values by a scalar
      @param filename char const array of filename
      @param w int width
      @param h int height
      @param c int channels
      @param data constant char array of the data to be stored
      @param stride_in_bytes For PNG, "stride_in_bytes" is the distance in bytes from the first byte of a row of pixels to the first byte of the next row of pixels
      @return error code based on success/failure
    */
    int ImagePreProcess::write(const char *filename, int width, int height, int channel, const void *data, std::string colorspace)
    {
      if (channel==1)
      {
        src = cv::Mat(cv::Size(width, height), CV_8UC1, (unsigned char*)(data));
      }
      else if (channel==3)
      {
        src = cv::Mat(cv::Size(width, height), CV_8UC3, (void*)(data));
        if (colorspace=="RGB" || colorspace=="rgb")
          cv::cvtColor(src, src, cv::COLOR_RGB2BGR);
      }

      cv::imwrite(filename, src);

      return true;
    }

  }
}
