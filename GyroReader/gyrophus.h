/*
 * gyrophus.h
 *
 *  Created on: Apr 24, 2022
 *      Author: KiralyCraft
 */

#ifndef GYROPHUS_H_
#define GYROPHUS_H_

#include <inttypes.h>
#include <stddef.h>

struct gyrophus_Device
{
	/*
	 * The absolute maximum delta corresponding to each axis' maximum change.
	 * -X,X; -Y,Y; -Z,Z
	 */
	int16_t usableRange[6];

	/*
	 * The average value between samples, in microseconds
	 */
	int32_t microsecondSamplingTime;

	/*
	 * Optionally, the device name corresponding to this gyrophus device.
	 * Purely informative.
	 */
	char* deviceName;

	/*
	 * A unique identifier of this gyrophus instance - Hotplug friendly
	 */
	char* deviceUUID;

	/*
	 * Ignore this for now
	 */
	void* iioDevice;
};

/*
 * Returns a list of pointers to gyrophus device instances.
 * This does not open the devices for reading.
 */
struct gyrophus_Device** gyrophus_listDevices(unsigned int* deviceCount);

/*
 * Returns a pointer to a gyrophus device instance.
 * This also opens the sensor for reading.
 */
struct gyrophus_Device* gyrophus_getDevice(char* deviceUUID);

/*
 * Returns 1 if the device is ready and actively sampling real, correct data.
 * Returns 0 otherwise.
 * Returns -1 if no such device exists.
 */
int8_t gyrophus_isDeviceReady(char* deviceUUID);

/*
 * Properly closes the provided gyrophus device. Returns 1 on failure, 0 on success.
 */
int8_t gyrophus_closeDevice(char* deviceUUID);

/*
 * Reads the instant values corresponding to the deviceUUID.
 * If the reading happens between two samples, the last sample value is returned.
 *
 * Values for X,Y,Z are returned as int16_t
 */
void gyrophus_readValues(char* deviceUUID,void* destination,size_t valueSize);

/*
 * Reads the delta values between two samples corresponding to the deviceUUID.
 * If two reads happen at a distance spanning multiple samples, the final cumulative delta is returned between them.
 *
 * Values for X,Y,Z are returned as int16_t
 */
void gyrophus_readDeltaValues(char* deviceUUID,void* destination);

/*
 * Reads the delta values between two samples corresponding to the deviceUUID.
 * If two reads happen at a distance spanning multiple samples, the average between the two reads is returned,
 * as well as the number of samples that have been collected between the two reads.
 *
 * Values for X,Y,Z,T are returned as int16_t (and int64_t respectively)
 */
void gyrophus_readAverageValues(char* deviceUUID,void* destination);

#endif /* GYROPHUS_H_ */
