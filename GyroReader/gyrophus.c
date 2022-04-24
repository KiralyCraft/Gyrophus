/*
 * gyrophus.c
 *
 *  Created on: Apr 24, 2022
 *      Author: KiralyCraft
 */

#include "gyrophus.h"
#include "gyrophus_device_thread.h"
#include <stddef.h>
#include <iio.h>
#include <string.h>
#include <pthread.h>

/*
 * The global iio context used by gyrophus
 */
struct iio_context* iioContext = NULL;
/*
 * Global list of gyrophus devices
 */
struct gyrophus_Device** gyrophusDeviceList;
/*
 * Global count of gyrophus devices
 */
uint8_t gyrophusDeviceCount;

/*
 * The structure that keeps track of the threads
 */
struct _gyrophusThreadEvidence
{
	char* deviceUUID;
	struct _gyrophusDeviceThreadDataExchange* threadDataExchange;
	struct _gyrophusThreadEvidence* nextThreadEvidence;
};

struct _gyrophusThreadEvidence* rootThreadEvidence = NULL;
struct _gyrophusThreadEvidence* lastThreadEvidence = NULL;


struct iio_context* _gyrophus_createContext()
{
	return iio_create_network_context("192.168.0.46");
}

struct gyrophus_Device** gyrophus_listDevices(unsigned int* deviceCount)
{
	if (iioContext == NULL)
	{
		iioContext = _gyrophus_createContext();
	}

	typedef struct gyroList
	{
		uint8_t deviceIndex;
		struct iio_device* theDevice;
		struct gyroList* nextDevice;
	} gyroList;

	gyroList* gyroListRoot = NULL;
	gyroList* currentGyroElement = NULL;
	gyrophusDeviceCount = 0;

	unsigned int iioDeviceCount = iio_context_get_devices_count(iioContext);

	for (uint8_t deviceWalker = 0; deviceWalker < iioDeviceCount; deviceWalker++)
	{
		struct iio_device* currentIIODevice = iio_context_get_device(iioContext,deviceWalker); //TODO check if this is null

		const char* currentIIODeviceName = iio_device_get_name(currentIIODevice);

		if (strstr(currentIIODeviceName,"gyro"))
		{
			gyroList* newGyroElement = malloc(sizeof(gyroList));
			newGyroElement->deviceIndex = deviceWalker;
			newGyroElement->theDevice = currentIIODevice;
			newGyroElement->nextDevice = NULL;

			if (currentGyroElement == NULL)
			{
				gyroListRoot = newGyroElement;
				currentGyroElement = gyroListRoot;
			}
			else
			{
				currentGyroElement->nextDevice = newGyroElement;
				currentGyroElement = newGyroElement;
			}
			gyrophusDeviceCount++;
		}
	}

	gyrophusDeviceList = malloc(sizeof(struct gyrophus_Device*)*gyrophusDeviceCount);

	uint8_t currentGyroListPosition = 0;
	currentGyroElement = gyroListRoot;
	while (currentGyroElement != NULL)
	{
		struct iio_device* theIIODevice = currentGyroElement->theDevice;
		const char* iioDeviceName = iio_device_get_name(theIIODevice);

		gyrophusDeviceList[currentGyroListPosition] = malloc(sizeof(struct gyrophus_Device));

		//TODO actually read generic data into this

		uint8_t deviceNameLength = strlen(iioDeviceName);
		gyrophusDeviceList[currentGyroListPosition]->deviceName = malloc(deviceNameLength+1);
		memcpy(gyrophusDeviceList[currentGyroListPosition]->deviceName,iioDeviceName,deviceNameLength);
		gyrophusDeviceList[currentGyroListPosition]->deviceName[deviceNameLength] = 0;

		gyrophusDeviceList[currentGyroListPosition]->deviceUUID = "shoes";
		gyrophusDeviceList[currentGyroListPosition]->microsecondSamplingTime = 4807;

		for (uint8_t axisWalker = 0; axisWalker < 6; axisWalker++)
		{
			gyrophusDeviceList[currentGyroListPosition]->usableRange[axisWalker] = 32767;
		}
		gyrophusDeviceList[currentGyroListPosition]->iioDevice = theIIODevice;

		currentGyroElement = currentGyroElement->nextDevice;
		free(gyroListRoot);
		gyroListRoot = currentGyroElement;
	}

	*deviceCount = gyrophusDeviceCount;
	return gyrophusDeviceList;
}

struct _gyrophusDeviceThreadDataExchange* _gyrophus_getThreadDataExchange(char* deviceUUID)
{
	struct _gyrophusThreadEvidence* threadChecker = rootThreadEvidence;

	if (rootThreadEvidence != NULL)
	{
		while (threadChecker != NULL)
		{
			if (strcmp(deviceUUID,threadChecker->deviceUUID) == 0)
			{
				return threadChecker->threadDataExchange;
			}
			threadChecker = threadChecker->nextThreadEvidence;
		}
	}
	return NULL;
}

struct gyrophus_Device* gyrophus_getDevice(char* deviceUUID)
{
	for (uint8_t deviceWalker = 0; deviceWalker < gyrophusDeviceCount; deviceWalker++)
	{
		if (strcmp(gyrophusDeviceList[deviceWalker]->deviceUUID,deviceUUID) == 0)
		{
			uint8_t foundExistingThread = 0;
			if (_gyrophus_getThreadDataExchange(deviceUUID) != NULL)
			{
				foundExistingThread = 1;
			}

			//If the device doesn't already have a processing thread
			if (!foundExistingThread)
			{
				struct _gyrophusThreadEvidence* newThreadEvidence = malloc(sizeof(struct _gyrophusThreadEvidence));

				uint8_t deviceUUIDLength = strlen(deviceUUID);
				newThreadEvidence->deviceUUID = malloc(deviceUUIDLength+1);
				memcpy(newThreadEvidence->deviceUUID,deviceUUID,deviceUUIDLength);
				newThreadEvidence->deviceUUID[deviceUUIDLength] = 0;

				newThreadEvidence->nextThreadEvidence = NULL;

				newThreadEvidence->threadDataExchange = malloc(sizeof(struct _gyrophusDeviceThreadDataExchange));
				struct _gyrophusDeviceThreadDataExchange* threadDataExchange = newThreadEvidence->threadDataExchange;

				///INIT THREAD CONTEXT
				threadDataExchange->shouldStop = 0;
				threadDataExchange->isDeviceReady = 0;
				threadDataExchange->iioDevice = gyrophusDeviceList[deviceWalker]->iioDevice;
				threadDataExchange->theMutex = malloc(sizeof(pthread_mutex_t));
				pthread_mutex_init(threadDataExchange->theMutex,NULL);
				//////////////////////
				pthread_t threadReference;
				pthread_create(&threadReference, NULL, &_gyrophus_thread_process, threadDataExchange);

				if (lastThreadEvidence == NULL)
				{
					rootThreadEvidence = newThreadEvidence;
					lastThreadEvidence = newThreadEvidence;
				}
				else
				{
					lastThreadEvidence->nextThreadEvidence = newThreadEvidence;
					lastThreadEvidence = newThreadEvidence;
				}
			}

			return gyrophusDeviceList[deviceWalker];
		}
	}
	return NULL;
}


int8_t gyrophus_isDeviceReady(char* deviceUUID)
{
	struct _gyrophusDeviceThreadDataExchange* deviceData = _gyrophus_getThreadDataExchange(deviceUUID);
	if (deviceData!=NULL)
	{
		return deviceData->isDeviceReady;
	}
	else
	{
		return -1;
	}
}


int8_t gyrophus_closeDevice(char* deviceUUID)
{
	struct _gyrophusDeviceThreadDataExchange* deviceData = _gyrophus_getThreadDataExchange(deviceUUID);
	if (deviceData!=NULL)
	{
		deviceData->shouldStop = 1;
		return 1;
	}
	else
	{
		return 0;
	}
}


void gyrophus_readValues(char* deviceUUID,void* destination,size_t valueSize)
{
	struct _gyrophusDeviceThreadDataExchange* deviceData = _gyrophus_getThreadDataExchange(deviceUUID);
	if (deviceData != NULL)
	{
		uint8_t reportedValueCount = deviceData->reportedValueCount;
		pthread_mutex_lock(deviceData->theMutex);
		memcpy(destination,deviceData->readValues,valueSize);
		pthread_mutex_unlock(deviceData->theMutex);
	}
}
