/**
 * @mic730ai_dio.cc
 * @brief Class to set/get GPIO
 *
 * This contains the function definitions for setting and getting the GPIO pin voltages
 *
 */

#ifdef WITH_MIC730AI
#include <edge-ml-accelerator/utils/mic730ai_dio.h>
#endif

namespace edgeml
{
    namespace utils
    {

		/**
		Creates the class constructor
		*/
		GPIO::GPIO()
		{
		}

		/**
		Creates the class destructor
		*/
		GPIO::~GPIO()
		{
		}

		/**
		Routine for setting DO from 0-7 to value 0/1 (MIC-730AI DO output is Anti-logic)
		@param gpio ranging from 0 to 7
		@param value setting to either 0 (low) or 1 (high)
		@return error code based on success/failure
		*/
		int GPIO::gpio_setvalue(int gpio, int value)
		{
			gpio += MIC_730AI_DO0;

			// MIC-730AI DO output is Anti-logic
			if(value == 0)
				value = 1;
			else
				value = 0;

			sprintf(cmd, "%d", gpio);
			fd = open("/sys/class/gpio/export", O_WRONLY | O_TRUNC);
			if (fd < 0)
			{
				// perror("open device failed!");
				return GPIO_FAIL;
			}
			ssize_t wret;
			wret = write(fd, cmd, sizeof(cmd));
			close(fd);

			sprintf(cmd, "out");
			sprintf(dev, "/sys/class/gpio/gpio%d/direction", gpio);
			fd = open(dev, O_WRONLY | O_TRUNC);
			if (fd < 0)
			{
				// perror("open device failed!!");
				return GPIO_FAIL;
			}
			wret = write(fd, cmd, sizeof(cmd));
			close(fd);

			sprintf(cmd, "%d", value);
			sprintf(dev, "/sys/class/gpio/gpio%d/value", gpio);
			fd = open(dev, O_WRONLY | O_TRUNC);
			if (fd < 0)
			{
				// perror("open device failed!!!");
				return GPIO_FAIL;
			}
			wret = write(fd, cmd, sizeof(cmd));
			close(fd);

			sprintf(cmd, "%d", gpio);
			fd = open("/sys/class/gpio/unexport", O_WRONLY | O_TRUNC);
			if (fd < 0)
			{
				// perror("open device unexport failed!!!!");
				return GPIO_FAIL;
			}
			wret = write(fd, cmd, sizeof(cmd));
			close(fd);

			gpio -= MIC_730AI_DO0;

			return GPIO_SUCCESS;
		}

		/**
		Routine for getting D1 from 0-7 to value 0/1 (MIC-730AI DO output is Anti-logic)
		@param gpio ranging from 0 to 7
		@param value setting to either 0 (low) or 1 (high)
		@return error code based on success/failure
		*/
		int GPIO::gpio_getvalue(int gpio, int& value)
		{
			gpio += MIC_730AI_DI0;

			sprintf(cmd, "%d", gpio);
			fd = open("/sys/class/gpio/export", O_WRONLY | O_TRUNC);
			if (fd < 0)
			{
				// perror("open device failed!");
				return GPIO_FAIL;
			}
			ssize_t wret;
			wret = write(fd, cmd, sizeof(cmd));
			close(fd);

			sprintf(cmd, "in");
			sprintf(dev, "/sys/class/gpio/gpio%d/direction", gpio);
			fd = open(dev, O_WRONLY | O_TRUNC);
			if (fd < 0)
			{
				// perror("open device failed!!");
				return GPIO_FAIL;
			}
			wret = write(fd, cmd, sizeof(cmd));
			close(fd);

			sprintf(dev, "/sys/class/gpio/gpio%d/value", gpio);
			fd = open(dev, O_RDONLY | O_TRUNC);
			if (fd < 0)
			{
				// perror("open device failed!!!");
				return GPIO_FAIL;
			}
			ret = read(fd, cmd, sizeof(cmd));
			if (ret < 0)
			{
				// perror("read device failed!");

				return GPIO_FAIL;
			}
			else
			{
				value = atoi(cmd);
			}
			close(fd);

			sprintf(cmd, "%d", gpio);
			fd = open("/sys/class/gpio/unexport", O_WRONLY | O_TRUNC);
			if (fd < 0)
			{
				// perror("open device unexport failed!!!!");
				return GPIO_FAIL;
			}
			wret = write(fd, cmd, sizeof(cmd));
			close(fd);

			gpio -= MIC_730AI_DI0;

			return GPIO_SUCCESS;
		}

	}
}