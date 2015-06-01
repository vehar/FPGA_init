#include "stdafx.h"
#include "GpioDriver.h"
#include <commctrl.h>
#include <winioctl.h>
//#include <ceddkex.h>
#include "gpio.h"

HANDLE GPIODriver::hGpio = INVALID_HANDLE_VALUE;
 DEVICE_CONTEXT_GPIO *pContext = NULL;

GPIODriver::GPIODriver()
{
	if(hGpio == INVALID_HANDLE_VALUE)
		hGpio = CreateFile(GPIO_DEVICE_NAME, GENERIC_WRITE|GENERIC_READ,0,NULL,OPEN_EXISTING,0,NULL);
	
	if (hGpio == INVALID_HANDLE_VALUE)
	{
		RETAILMSG(1,(TEXT("Failed open gioD: device driver!\r\n")));
	}

	
    // Allocate memory for our handler...
    pContext = (DEVICE_CONTEXT_GPIO*)LocalAlloc(
        LPTR, sizeof(DEVICE_CONTEXT_GPIO)
        );
    if (pContext == NULL)
        {
        DEBUGMSG(1, (L"GPIOOpen LocalAlloc failed\r\n"));
        CloseHandle(hGpio);
        }
    // Save device handle
    pContext->hDevice = hGpio;

}

GPIODriver::~GPIODriver()
{
    DEVICE_CONTEXT_GPIO *pContext = (DEVICE_CONTEXT_GPIO *)hGpio;
    if (pContext != NULL)
    {
        CloseHandle(pContext->hDevice);
        LocalFree(pContext);
    }
}

BOOL GPIODriver::SetGpioMode(DWORD pin, UINT value)
{
	gpioValue inBuf;
	inBuf.pin = pin;
	inBuf.value = (DWORD)value;

	BOOL rc;
	rc = DeviceIoControl(
		hGpio, IOCTL_GPIO_SETMODE,&inBuf,sizeof(inBuf),NULL,NULL,NULL,NULL);
	return rc;
};
//------------------------------------------------------------------------------
//Set GPIO pin's value (set pin as output)
BOOL GPIODriver::SetGpioBit(DWORD pin)
{
	BOOL rc;
	rc = DeviceIoControl(
		hGpio, IOCTL_GPIO_SETBIT,&pin,sizeof(pin),NULL,NULL,NULL,NULL);
	return rc;
};
//------------------------------------------------------------------------------
//Set GPIO pin's value (set pin as input)
BOOL GPIODriver::ClrGpioBit(DWORD pin)
{
	BOOL rc;
	rc = DeviceIoControl(
		hGpio, IOCTL_GPIO_CLRBIT,&pin,sizeof(pin),NULL,NULL,NULL,NULL);
	return rc;
};

DWORD GPIODriver::PullUpGpioBit(DWORD pin)
{
	DWORD   dwValue = 0;
 DeviceIoControl(hGpio, IOCTL_GPIO_PULLUP_ENABLE,&pin,sizeof(pin),&dwValue,sizeof(dwValue),NULL,NULL);
	return dwValue;
};

DWORD GPIODriver::PullDownGpioBit(DWORD pin)
{
	DWORD   dwValue = 0;
 DeviceIoControl(hGpio, IOCTL_GPIO_PULLDOWN_ENABLE,&pin,sizeof(pin),&dwValue,sizeof(dwValue),NULL,NULL);
	return dwValue;
};



DWORD GPIODriver::GetGpioBit(DWORD pin)
{
	DWORD   dwValue = 0;
DeviceIoControl(hGpio, IOCTL_GPIO_GETBIT,&pin,sizeof(pin),&dwValue,sizeof(dwValue),NULL,NULL);
	return dwValue;
};

DWORD GPIODriver::GetGpioMode(DWORD pin)
{
	DWORD   dwValue = 0;
DeviceIoControl(hGpio, IOCTL_GPIO_GETMODE,&pin,sizeof(pin),&dwValue,sizeof(dwValue),NULL,NULL);
	return dwValue;
};
