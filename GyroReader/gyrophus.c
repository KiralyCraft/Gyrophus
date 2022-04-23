/*
 * gyro.c
 *
 *  Created on: Apr 21, 2022
 *      Author: KiralyCraft
 */
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <iio.h>
#include <string.h>
#include <inttypes.h>

#define GYRO_AVERAGE_SAMPLES 64
#define IIO_BUFFER_SIZE 1

typedef struct
{
	uint16_t sampleCount;
	int32_t averageValue;
} gyroDataAverage;

int main()
{
	setvbuf( stdout, NULL, _IONBF, 0 );
	setvbuf( stderr, NULL, _IONBF, 0 );

	struct iio_context* iioContext =  iio_create_local_context();
	struct iio_device* theGyro = iio_context_find_device(iioContext, "lsm6ds3_gyro");
	struct iio_buffer* iioBuffer;

	if (theGyro == NULL)
	{
		printf("Can't open the thing\n");
		return 1;
	}
	else
	{
		unsigned int attrCount = iio_device_get_attrs_count(theGyro);
		for (int attrWalker=0;attrWalker<attrCount;attrWalker++)
		{
			const char* attrName = iio_device_get_attr(theGyro,attrWalker);
			if (strcmp(attrName,"sampling_frequency") == 0)
			{
				iio_device_attr_write(theGyro,attrName,"208");
				break;
			}
		}

		struct iio_channel** foundChannels = malloc(sizeof(void*)*4);

		for (uint8_t channelWalker = 0; channelWalker < iio_device_get_channels_count(theGyro); channelWalker++)
		{
			struct iio_channel *foundChannel = iio_device_get_channel(theGyro, channelWalker);
			if (iio_channel_is_scan_element(foundChannel))
			{
				const char* channelName = iio_channel_get_id(foundChannel);

				uint8_t shouldEnable = 0;

				if (strcmp(channelName,"anglvel_x") == 0)
				{
					foundChannels[0] = foundChannel;
					shouldEnable = 1;
				}
				else if (strcmp(channelName,"anglvel_y") == 0)
				{
					foundChannels[1] = foundChannel;
					shouldEnable = 1;
				}
				else if (strcmp(channelName,"anglvel_z") == 0)
				{
					foundChannels[2] = foundChannel;
					shouldEnable = 1;
				}
				else if (strcmp(channelName,"timestamp") == 0)
				{
					foundChannels[3] = foundChannel;
					shouldEnable = 1;
				}

				if (shouldEnable)
				{
					iio_channel_enable(foundChannel);
				}
			}
		}

		iioBuffer = iio_device_create_buffer(theGyro, IIO_BUFFER_SIZE, 0);


		gyroDataAverage** calibrationOffset = malloc(sizeof(gyroDataAverage*)*3);
		for (uint8_t averageBuilder = 0;averageBuilder < 3;averageBuilder++)
		{
			calibrationOffset[averageBuilder] = malloc(sizeof(gyroDataAverage));
			gyroDataAverage* theAverage = calibrationOffset[averageBuilder];
			theAverage -> averageValue = 0;
			theAverage -> sampleCount = 0;
		}

		double* angularDataScale = malloc(sizeof(double)*3);
		int16_t* angularData = malloc(sizeof(int16_t)*3);
		uint8_t isGyroCalibrated = 0;
		uint8_t isDataDirty = 0;
		uint8_t measurementsToSkip = 0;
		uint8_t stabilizationMeasurements = 16;
		int64_t lastTimestamp = 0;

		double* eulerAngles = malloc(sizeof(double)*3);
		eulerAngles[0] = 0;
		eulerAngles[1] = 0;
		eulerAngles[2] = 0;

		while(1)
		{
			ssize_t readBytes = iio_buffer_refill(iioBuffer);

			if (readBytes < 0)
			{
				printf("Could read the sensor!\n");
				break;
			}

			ptrdiff_t bufferStep = iio_buffer_step(iioBuffer);
			void* bufferEndAddress = iio_buffer_end(iioBuffer);

			int16_t x,y,z;
			int64_t currentTimestamp;
			int64_t timestampDifference;

			for (uint8_t channelIterator = 0; channelIterator < 4; channelIterator++)
			{

				const struct iio_data_format *channelDataFormat = iio_channel_get_data_format(foundChannels[channelIterator]); //You can check the data length associated with this channel, remember :)
				unsigned int repeatCount = channelDataFormat -> repeat;

				if (repeatCount > 1)
				{
					printf("Sensor is giving us more data despite not asking for this much!\n");
				}

				void* bufferPointerLocation = iio_buffer_first(iioBuffer, foundChannels[channelIterator]);
				for (;bufferPointerLocation < bufferEndAddress; bufferPointerLocation += bufferStep)
				{
					if (channelIterator < 3)
					{
						angularData[channelIterator] = *(int16_t *)bufferPointerLocation;
						angularDataScale[channelIterator] = channelDataFormat->scale;
					}
					else if (channelIterator == 3) //The last channel read
					{
						currentTimestamp = *(int64_t *)bufferPointerLocation;
						/*
						 * The gyro sometimes reports bogus data. It's been observed that these are correlated with an increase in reading latency, and usually come in pairs of two
						 * A sampling rate of 208 Hz is ~4.807 ms, or 4807 us.
						 */
						timestampDifference = currentTimestamp - lastTimestamp;
						if (timestampDifference/1000 > 4850)
						{
							measurementsToSkip = 2;
						}

						if (measurementsToSkip > 0)
						{
							isDataDirty = 1;
							measurementsToSkip--;
						}
						else
						{
							if (stabilizationMeasurements > 0)
							{
								isDataDirty = 1;
								stabilizationMeasurements--;
							}
							else
							{
								isDataDirty = 0;
							}
						}
						lastTimestamp = currentTimestamp;
					}
				}
			}

			if (!isDataDirty)
			{
				if (!isGyroCalibrated)
				{
					printf("Calibrating gyro\n");
					for (uint8_t axisWalker=0; axisWalker<3; axisWalker++)
					{
						gyroDataAverage* gyroAverage = calibrationOffset[axisWalker];
						if (gyroAverage->sampleCount < GYRO_AVERAGE_SAMPLES)
						{
							gyroAverage->sampleCount++;

							if (gyroAverage->sampleCount == 0)
							{
								gyroAverage->averageValue = angularData[axisWalker];
							}
							else
							{
								gyroAverage->averageValue = gyroAverage->averageValue + (angularData[axisWalker] - gyroAverage->averageValue)/(gyroAverage->sampleCount);
							}
						}
						else if (gyroAverage->sampleCount == GYRO_AVERAGE_SAMPLES)
						{
							isGyroCalibrated = 1;
						}
					}
				}
				else
				{
					double timeStamp = timestampDifference/1000000000.0; //Timestamps are in nanoseconds, gyro output is in rad/s

					for (uint8_t axisWalker=0; axisWalker<3; axisWalker++)
					{
						eulerAngles[axisWalker] += (timeStamp/(57.295779513 * 2))*(angularData[axisWalker] - calibrationOffset[axisWalker]->averageValue);
					}

					printf("%f,%f,%f\n",eulerAngles[0],eulerAngles[1],eulerAngles[2]);
				}
			}

		}
	}
}

