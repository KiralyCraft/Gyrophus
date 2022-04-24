/*
 * test.c
 *
 *  Created on: Apr 24, 2022
 *      Author: KiralyCraft
 */
#include "gyrophus.h"

int main()
{
	int devCount = 0;
	struct gyrophus_Device** theDevices = gyrophus_listDevices(&devCount);
	printf("%d\n",devCount);
	struct gyrophus_Device* theDevice = gyrophus_getDevice(theDevices[0]->deviceUUID);

	int16_t* theData = malloc(sizeof(int16_t)*3);
	while(1)
	{
		if (gyrophus_isDeviceReady("shoes"))
		{
			gyrophus_readValues("shoes",theData,sizeof(int16_t)*3);
			printf("%d - %d - %d\n",theData[0],theData[1],theData[2]);

			usleep(theDevice->microsecondSamplingTime);
		}
		else
		{
			printf("Not ready\n");
		}
	}
}

