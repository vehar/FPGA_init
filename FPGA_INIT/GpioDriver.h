#pragma once 
#include <windows.h>

//#define GPIO_DEVICE_NAME        L"TWL1:"
#define GPIO_DEVICE_NAME        L"GIO1:"

typedef struct _gpioValue {
	DWORD pin;
	DWORD value;
} gpioValue;


//------------------------------------------------------------------------------
//
//  Define:  GPIO_DIR_xxx/GPIO_INT_xxx
//
#define GPIO_DIR_OUTPUT         (0 << 0)
#define GPIO_DIR_INPUT          (1 << 0)
#define GPIO_INT_LOW_HIGH       (1 << 1)
#define GPIO_INT_HIGH_LOW       (1 << 2)
#define GPIO_INT_LOW            (1 << 3)
#define GPIO_INT_HIGH           (1 << 4)
#define GPIO_DEBOUNCE_ENABLE    (1 << 5)

#define DCLK_PIN 130

class GPIODriver
{
public:

	GPIODriver();
	~GPIODriver();
	//Get GPIO pin's IRQ
	static BOOL SetGpioMode(DWORD pin, UINT value);
	//Set GPIO pin's value (set pin as output)
	static BOOL	SetGpioBit(DWORD pin);
	//Set GPIO pin's value (set pin as input)
	static BOOL ClrGpioBit(DWORD pin);


    static DWORD PullUpGpioBit(DWORD pin);
	static DWORD PullDownGpioBit(DWORD pin);

	static DWORD GetGpioBit(DWORD pin);
	static DWORD GetGpioMode(DWORD pin);

	static HANDLE hGpio;

	//static void FPGA_DCLK(bool x);
};