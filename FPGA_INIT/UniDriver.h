#pragma once

#include <windows.h>
#include "driverHelper.h"


#define FILE_DEVICE_HAL 0x00000101
#define FILE_DEVICE_HAL	0x00000101
#define METHOD_BUFFERED          0
#define FILE_ANY_ACCESS          0

#define TIM_INT_OFFSET           9

#define CTL_CODE(DeviceType, Function, Method, Access ) (((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method))
#define IOCTL_HAL_REQUEST_SYSINTR CTL_CODE(FILE_DEVICE_HAL, 38, METHOD_BUFFERED, FILE_ANY_ACCESS)



class UniDriver  //general memory interface
{
	static HANDLE hDrv;
	static HANDLE Open();	
	
public:

	UniDriver();
	~UniDriver();

  
   static DWORD ReadReg(RWRegData_t* readData);
   static DWORD WriteReg(RWRegData_t* writeData);
   static void  DmaRead(WORD addr, WORD size);
   static HANDLE GetHDrv() {return hDrv;};
};

