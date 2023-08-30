/**
 * @mic730ai_dio.h
 * @brief Creating a GPIO set/get class for MIC-730AI
 *
 * This contains the prototypes of setting and getting GPIO for MIC-730AI
 *
 */

#ifndef __MIC730AI_DIO_H__
#define __MIC730AI_DIO_H__
#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>

#include <edge-ml-accelerator/utils/edge_ml_config.h>

// MIC-730AI DI define
#define MIC_730AI_DI0		232
#define MIC_730AI_DI1		233
#define MIC_730AI_DI2		234
#define MIC_730AI_DI3		235
#define MIC_730AI_DI4		236
#define MIC_730AI_DI5		237
#define MIC_730AI_DI6		238
#define MIC_730AI_DI7		239

// MIC-730AI DO define
#define MIC_730AI_DO0		224
#define MIC_730AI_DO1		225
#define MIC_730AI_DO2		226
#define MIC_730AI_DO3		227
#define MIC_730AI_DO4		228
#define MIC_730AI_DO5		229
#define MIC_730AI_DO6		230
#define MIC_730AI_DO7		231

namespace edgeml
{
    namespace utils
    {

        class GPIO
        {
            public:
                GPIO();
                ~GPIO();
                int gpio_setvalue(int gpio, int value);
                int gpio_getvalue(int gpio, int& value);

            private:
                int fd = -1;
                char cmd[64];
                char dev[128];
                int ret;
        };

    }
}

#endif