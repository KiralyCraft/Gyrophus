/*
 * gyrophus_device_thread.h
 *
 *  Created on: Apr 24, 2022
 *      Author: KiralyCraft
 */

#ifndef GYROPHUS_DEVICE_THREAD_H_
#define GYROPHUS_DEVICE_THREAD_H_

#include <iio.h>

struct _gyrophusDeviceThreadDataExchange
{
	/*
	 * A signal which the thread checks serving as a stop condition
	 */
	uint8_t shouldStop;

	/*
	 * 1 if the device is actively providing useful data, 0 otherwise
	 */
	uint8_t isDeviceReady;

	/*
	 * The values read from the sensor.
	 */
	void* readValues;

	/*
	 * How many distinct channels are read
	 */
	int8_t reportedValueCount;

	/*
	 * A reference to the IIO device which this thread will be messing with
	 */
	struct iio_device* iioDevice;

	/*
	 * Synchronization
	 */
	pthread_mutex_t* theMutex;
};

/*
 * The thread processing Gyro events.
 * The argument is expected to be a _gyrophusDeviceThreadDataExchange.
 * The return value is not used.
 */
void* _gyrophus_thread_process(void* threadInitialData);

#endif /* GYROPHUS_DEVICE_THREAD_H_ */
