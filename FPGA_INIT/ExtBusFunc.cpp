#include "stdafx.h"
#include <windows.h>
#include "UniDriver.h"

UniDriver bus;

void SetDataValue(DWORD val, DWORD addr)
{
	RWRegData_t regData = {GPMC_DATA_BASE, 0, 0};
	regData.value = val;
	regData.offset = addr;
	bus.WriteReg(&regData);	
}

DWORD GetDataValue(DWORD addr)
{
	RWRegData_t regData = {GPMC_DATA_BASE, 0, 0};
	regData.offset = addr;
	return bus.ReadReg(&regData);	
}


void SetCs1Value(DWORD val, DWORD addr)
{
	RWRegData_t regData = {GPMC_CS1_BASE, 0, 0};
	regData.value = val;
	regData.offset = addr;
	bus.WriteReg(&regData);	
}

DWORD GetCs1Value(DWORD addr)
{
	RWRegData_t regData = {GPMC_CS1_BASE, 0, 0};
	regData.offset = addr;
	return bus.ReadReg(&regData);	
}


DWORD GetRegVal(int i)
{
    RWRegData_t regData = {GPMC_BASE_ADDR, offsetof(GPMC_REGS_t, GPMC_CONFIGDATA[i].GPMC_CONFIG7), 0};
	return bus.ReadReg(&regData);	
}

void ExtBusCs1Init(void) 
{ 
    RWRegData_t regData = {GPMC_BASE_ADDR, offsetof(GPMC_REGS_t, GPMC_CONFIGDATA[1].GPMC_CONFIG1), 0};
	bus.ReadReg(&regData);
	regData.value = 0;
	regData.value |= 1<<12;//16 bit
	regData.value |= 1<<4;//latency x2, если убрать будет в 2-ва раза быстрее
    bus.WriteReg(&regData); 

	////////////////////////////////////////////
	RWRegData_t regData_timing = {GPMC_BASE_ADDR, offsetof(GPMC_REGS_t, GPMC_CONFIGDATA[1].GPMC_CONFIG2), 0};
	bus.ReadReg(&regData_timing);
	regData_timing.value = 0;
	regData_timing.value |= 0x001F1F02;//default - 0x00101001 - settings fot ext bus timings
    bus.WriteReg(&regData_timing); 
}