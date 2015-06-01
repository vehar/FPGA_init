// UseDriverSampleCons.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <commctrl.h>
#include <iostream>
#include <iomanip>
#include "driverHelper.h"
#include "UniDriver.h"

using namespace std;

UniDriver gmi;

void ExtBusCs3Init();
void SetCs3Value(DWORD val, DWORD addr);
DWORD GetCs3Value(DWORD addr);

int _tmain(int argc, _TCHAR* argv[])
{
	cout<<"Started ..."<<endl;
	ExtBusCs3Init();

	bool exit = false;
    DWORD addr = 0x0;
	DWORD value = 0xff;
	while(!exit)
	{
		addr+=2;
		//SetCs3Value(value, addr);
		GetCs3Value(addr);
		if(addr>0x4000) addr = 0;
	}
	return 0;
}

void SetCs3Value(DWORD val, DWORD addr)
{
	RWRegData_t regData = {GPMC_CS3_BASE, 0, 0};
	regData.value = val;
	regData.offset = addr;
	gmi.WriteReg(&regData);	
}

DWORD GetCs3Value(DWORD addr)
{
	RWRegData_t regData = {GPMC_CS3_BASE, 0, 0};
	regData.offset = addr;
	return gmi.ReadReg(&regData);	
}

void ExtBusCs3Init()
{
    RWRegData_t regData = {GPMC_BASE_ADDR, offsetof(GPMC_REGS_t, GPMC_CONFIGDATA[3].GPMC_CONFIG1), 0};
	gmi.ReadReg(&regData);
	regData.value = 0;
	regData.value |= 1<<12;//16 bit
	regData.value |= 1<<4;//latency x2, если убрать будет в 2-ва раза быстрее
    gmi.WriteReg(&regData);

	
/*
	regData.offset = offsetof(GPMC_REGS_t, GPMC_CONFIGDATA[3].GPMC_CONFIG2);
	gmi.ReadReg(&regData);
	regData.value = 0;
	regData.value |= 1;
    regData.value |= 0xf<<16;
	gmi.WriteReg(&regData);

	regData.offset = offsetof(GPMC_REGS_t, GPMC_CONFIGDATA[3].GPMC_CONFIG3);
	gmi.ReadReg(&regData);
	regData.value = 0;
	gmi.WriteReg(&regData);

	regData.offset = offsetof(GPMC_REGS_t, GPMC_CONFIGDATA[3].GPMC_CONFIG4);
	gmi.ReadReg(&regData);
	regData.value = 0;
//	gmi.WriteReg(&regData);

	regData.offset = offsetof(GPMC_REGS_t, GPMC_CONFIGDATA[3].GPMC_CONFIG5);
	gmi.ReadReg(&regData);
// 	gmi.WriteReg(&regData);

/*
	regData.offset = offsetof(GPMC_REGS_t, GPMC_CONFIG);
	gmi.ReadReg(&regData);
	//regData.value = 0;
	regData.value |= (0x3);
	gmi.WriteReg(&regData);
*/
}



