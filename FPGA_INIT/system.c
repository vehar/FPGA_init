
//***************************************************************************************************
/** @file System.c
 *
 * Copyright  2002-2005 NXP B.V.,
 *
 *	This  source code and any compilation or derivative thereof 
 *  is the proprietary information of NXP B.V.
 *  and is confidential in nature.
 *
 *	Under no circumstances is this software to be exposed to or placed
 *  under an Open Source License of any type without the expressed
 *  written permission of NXP B.V.
 * 
 * 	Module Name		:		System.c
 * 
 * Abstract			:		Device dependant part of the 1761 Host Controller Driver
 *
 *
 * Version			:		3.2.0.0 Host Only
 * Date				:		13/03/07
 *
 * Vesrion			:		3.1.0.0 Host Only
 * Date				:		13/10/06
 *
 * Vesrion			:		3.0.0.0 Host Only
 * Date				:		16/05/06
 * NewFeatures Added: PowerManagement.
 * BugFixes			: 
 *
 *  1. Physical address issue has been fixed.If the client driver comes with physical address,
 *     the driver has been failing in converting the physical address to virtual address.
 *     It has been resolved by directly using the clients virtual address.
 *  2. Memory leak for the StatusPTD has been resolved.
 *  3. When we disconnect and connect a device,it has been detaching and attaching again.
 *     It has been resloved.This is due to the port reset and Getstatus of the device while we disconnect a device.
 *
 * 	Vesrion			:		2.0.0.0 Host Only
 *	Date			:		25/11/05
 * 

 * 	Vesrion			:		1.1 Host Only
 *	Date			:		16/06/05     						
 *     						
 * 	Vesrion			:		1.0 Host Only
 *	Date			:		29/04/05     						 
 * 	Vesrion			:		0.8 Beta
 *	Date			:		27/12/04
 *
 *	Author			:		Praveen,nxp Semiconductors 
 *
 *	Comments		:		Initial Creation
 *
 ****************************************************************************************************/
#define WINCE600_PORTING 

#include <windows.h>
#include <nkintr.h>
#include <ceddk.h>
#include <ddkreg.h>
#include <devload.h>
#include <giisr.h>
#include <phcdddsi.h>

#ifdef WINCE600_PORTING
#include <cebuscfg.h> 
#endif

#include "PCIKit.h"
#include "ISP1761HCDConfig.h"
#include "USB_OET.h"
#include "PowerDbg.h"
#include "HostDeviceConfig.h"
#ifdef PXA_KIT
#include "PXAKit.h"
#endif
#ifdef OMAP35X_KIT
#include "OMAP35xKit.h"
#endif

#define REG_PHYSICAL_PAGE_SIZE TEXT("PhysicalPageSize")

// Amount of memory to use for HCD buffer
#define gcTotalAvailablePhysicalMemory (0x40000) // 256K //(0x20000) // 128K
#define gcHighPriorityPhysicalMemory (gcTotalAvailablePhysicalMemory/4)   // 1/4 as high priority i.e. 16k

#ifdef WINCE600_PORTING
typedef struct {
    DWORD Addr_64Bit:1;
    DWORD Frame_Prog:1;
    DWORD Async_Park:1;
    DWORD Reserved1:1;
    DWORD Isoch_Sched_Threshold:4;
    DWORD EHCI_Ext_Cap_Pointer:8;
    DWORD Reserved2:16;
} HCCP_CAP_Bit;
typedef union {
    volatile HCCP_CAP_Bit   bit;
    volatile DWORD          ul;
} HCCP_CAP;
typedef struct {
    DWORD Cap_ID:8;
    DWORD Next_Cap_Pointer:8;
    DWORD HC_BIOS_Owned:1;
    DWORD Reserved1:7;
    DWORD HC_OS_Owned:1;
    DWORD Reserved2:7;
} USBLEGSUP_Bit;
typedef union {
    volatile USBLEGSUP_Bit  bit;
    volatile DWORD          ul;
} USBLEGSUP;
#endif

#define PHHC_REG_HCCHIPID   0x304
#define PHHC_REG_HCINTR     0x310
#define PHHC_REG_HCINTRENBL 0x314

#define PHHC_REG_HCCHIPID_VALUE   0x00001761

extern BOOL HcdMdd_HSET(LPVOID lpvHcd,PBYTE pBufIn);
extern BOOL HcdMdd_OtgPortControl(LPVOID lpvHcd,BOOL bFlag);

//MPK added on 10/11/2006
#ifdef __cplusplus
extern "C" PUCHAR gpbyPCIPLX9054RegBase;  // PLX
extern "C" ULONG gulPXARegBasePhyAddr;		// Marlon : PXA 
#endif
PUCHAR gpbyPCIPLX9054RegBase;
ULONG gulPXARegBasePhyAddr;		// Marlon : PXA 

typedef struct _SEHCDPdd
{
    LPVOID lpvMemoryObject;
    LPVOID lpvEHCDMddObject;
    PVOID pvVirtualAddress;                        // DMA buffers as seen by the CPU
    DWORD dwPhysicalMemSize;
    PHYSICAL_ADDRESS LogicalAddress;        // DMA buffers as seen by the DMA controller and bus interfaces
    DMA_ADAPTER_OBJECT AdapterObject;
    TCHAR szDriverRegKey[MAX_PATH];
    PUCHAR ioPortBase;
    DWORD dwSysIntr;
    CRITICAL_SECTION csPdd;                     // serializes access to the PDD object
    HANDLE          IsrHandle;
    HANDLE hParentBusHandle;
    HANDLE m_hGPIO;
    DWORD NumOfPorts;
} SEHCDPdd;

#define UnusedParameter(x)  x = x

//**********************************************************************************
/**	
 *	This function is the main dynamic-link library (DLL) entry point for 
 *	the platform dependent driver (PDD) portion of a host controller interface (HCI) driver.
 * 	This function enables the PDD to perform necessary processing at DLL entry time. 
 * 	Generally, this function does little because the HcdPdd_Init function performs most initialization.
 *
 *  @param		:	hinstDLL	-	Handle to a DLL instance. 
 *
 *	@param		:	dwReason	-	Reason code. 
 *
 *	@param		:	lpvReserved	-	Pointer to a system parameter. 
 *
 *  @return		:	TRUE indicates success. FALSE indicates failure
 *
 ***********************************************************************************/
extern BOOL HcdPdd_DllMain(HANDLE hinstDLL, DWORD dwReason, LPVOID lpvReserved)
{
/*    UnusedParameter(hinstDLL);
    UnusedParameter(dwReason);
    UnusedParameter(lpvReserved);
*/
    return TRUE;
}

//*******************************************************************
/**
/* Get the Registry Configuration from PCI PLX Bridge
 *
 *	@param		:	RegKeyPath
 *	
 *	@param		:	lpdwBaseAddr
 *
 *	@param		:	lpdwAddrLen
 *
 *	@param		:	lpdwIOSpace
 *
 *	@param		:	lpdwSysIntr
 *
 *	@param		:	lpIfcType
 *
 *	@param		:	lpdwBusNumber
 *
 *	@param		:	bInstallIsr
 *
 *	@param		:	IsrDll
 *
 *	@param		:	IsrHandler
 *
 *	@param		:	Irq
 *
 *	@param		:	InstanceIndex
 *
 *	@param		:	OtgSupport
 *
 *	@param		:	HostOnlySupport
 *
 *	@param		:	Plx9054LastOwner
 *
 *	@param		:	Plx9054PhysAddr
 *
 *	@return		:	TRUE for success, FALSE for error
 *   
 ***********************************************************************/
static BOOL
GetRegistryConfig(
    LPCWSTR RegKeyPath,         // IN - driver registry key path
    DWORD * lpdwBaseAddr,       // OUT - base address
    DWORD * lpdwAddrLen,        // OUT - address range
    DWORD * lpdwIOSpace,        // OUT - 1 if base address describes I/O port, 0 otherwise
    DWORD * lpdwSysIntr,        // OUT - system interrupt number
    PINTERFACE_TYPE lpIfcType,  // OUT - interface type (PCIbus, ISAbus, etc)
    DWORD * lpdwBusNumber,      // OUT - bus number, depends on interface type
    BOOL * bInstallIsr,         // OUT - TRUE if ISR Handler found in registry
    LPWSTR IsrDll,              // OUT - Name of ISR Handler dll
    LPWSTR IsrHandler,          // OUT - Name of ISR Handler routine
    DWORD * Irq,                // OUT - IRQ number, used to hook ISR handler
	DWORD * InstanceIndex,		// OUT - InstanceIndex Number.
	DWORD * OtgSupport,     	// OUT - directive to load otg driver or not
	DWORD * HostOnlySupport,    // OUT - directive to suspend host controller or not
    DWORD * NumOfPorts,         // OUT - number of ports the user wants to use
    DWORD * SuspendGpio,        // OUT - GPIO number used as the suspend/wakeup signal
	DWORD * Plx9054LastOwner,	// OUT - am I the last one in the chain to enable PLX9054's interrupt gate
	DWORD * Plx9054PhysAddr
    )
{
    HKEY hKey;
    DWORD dwData;
    DWORD dwSize;
    DWORD dwType;
    BOOL  fRet=FALSE;
    DWORD dwRet;

    const USHORT cDefaultPhcdPortRange = 0x70;
    const ULONG cHTBAMemorySpace = 0;
    const ULONG cHTBAIOSpace = 1;
    
    // Open key
    dwRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,RegKeyPath,0,0,&hKey);
    if (dwRet != ERROR_SUCCESS)
    {
        DEBUGMSG(ZONE_ERROR,(TEXT("!PHCD:GetRegistryConfig Error RegOpenKeyEx(%s) failed %d\r\n"),
                             RegKeyPath, dwRet));
        return FALSE;
    }

    // Read base address, range from registry and determine IOSpace
    dwSize = sizeof(dwData);
    dwRet = RegQueryValueEx(hKey, DEVLOAD_MEMBASE_VALNAME, 0, &dwType, (PUCHAR)&dwData, &dwSize);
    if (dwRet == ERROR_SUCCESS)
    {
        *lpdwIOSpace = cHTBAMemorySpace;
        *lpdwBaseAddr = dwData;
       
          
        dwRet = RegQueryValueEx(hKey, DEVLOAD_MEMLEN_VALNAME, 0, &dwType, (PUCHAR)&dwData, &dwSize);

        if (dwRet == ERROR_SUCCESS)
        {
            *lpdwAddrLen = dwData;
        } else {
            *lpdwAddrLen = cDefaultPhcdPortRange;
        }
    } 
    else
    {
        dwRet = RegQueryValueEx(hKey,DEVLOAD_IOBASE_VALNAME,0,&dwType,(PUCHAR)&dwData,&dwSize);
        
        if (dwRet != ERROR_SUCCESS)
        {
            DEBUGMSG(ZONE_ERROR, (TEXT("!PHCD:GetRegistryConfig Neither %s nor %s were found.Error\r\n"),
                              DEVLOAD_MEMBASE_VALNAME, DEVLOAD_IOBASE_VALNAME, dwRet));
            goto GetRegistryConfig_exit;
        }

        *lpdwIOSpace = cHTBAIOSpace;
        *lpdwBaseAddr = dwData;
       

        dwRet = RegQueryValueEx(hKey,DEVLOAD_IOLEN_VALNAME,0,&dwType,(PUCHAR)&dwData,&dwSize);
        
        if (dwRet == ERROR_SUCCESS)
        {
            *lpdwAddrLen = dwData;
        }
        else
        {
            *lpdwAddrLen = cDefaultPhcdPortRange;
        }  
    }
    

    // Read SysIntr from registry
#ifndef OMAP35X_KIT
    dwSize = sizeof(dwData);
    dwRet = RegQueryValueEx(hKey,DEVLOAD_SYSINTR_VALNAME,0,&dwType,(PUCHAR)&dwData,&dwSize);
    if (dwRet != ERROR_SUCCESS)
    {
        DEBUGMSG(ZONE_ERROR, (TEXT("!PHCD:GetRegistryConfig RegQueryValueEx(%s) failed.Error %d\r\n"),
                              DEVLOAD_SYSINTR_VALNAME, dwRet));
        goto GetRegistryConfig_exit;

    }
    *lpdwSysIntr = dwData;
#else
    *lpdwSysIntr = 0;
#endif
    // Read interface type from registry
    dwSize = sizeof(dwData);
    dwRet = RegQueryValueEx(hKey,DEVLOAD_IFCTYPE_VALNAME,0,&dwType,(PUCHAR)&dwData,&dwSize);
    if (dwRet != ERROR_SUCCESS)
    {
        // Default value is PCIBus
        *lpIfcType = PCIBus;
#ifdef OMAP35X_KIT
        *lpIfcType = Internal;
#endif
    }
    else
    {
        *lpIfcType = dwData;
    }

    // Read bus number from registry
    dwSize = sizeof(dwData);
    dwRet = RegQueryValueEx(hKey,DEVLOAD_BUSNUMBER_VALNAME,0,&dwType,(PUCHAR)&dwData,&dwSize);
    if (dwRet != ERROR_SUCCESS)
    {
        // Default value is 0
        *lpdwBusNumber = 0;
    } else {
        *lpdwBusNumber = dwData;
    }

    // Read installable ISR dll name from registry, if it exists
    dwSize = DEVDLL_LEN * sizeof(WCHAR);
    dwRet = RegQueryValueEx(hKey, DEVLOAD_ISRDLL_VALNAME, 0, &dwType, (PUCHAR)IsrDll, &dwSize);
    if (dwRet == ERROR_SUCCESS)
    {
        // ISR dll name found, now look for ISR handler routine name
        dwSize = DEVENTRY_LEN * sizeof(WCHAR);
        dwRet = RegQueryValueEx(hKey, DEVLOAD_ISRHANDLER_VALNAME, 0, &dwType, (PUCHAR)IsrHandler, &dwSize);
        if (dwRet == ERROR_SUCCESS)
        {
            // ISR handler name found, now look for Irq
            dwSize = sizeof(dwData);
            dwRet = RegQueryValueEx(hKey, DEVLOAD_IRQ_VALNAME, 0, &dwType, (PUCHAR)&dwData, &dwSize);
            if (dwRet == ERROR_SUCCESS)
            {
                *Irq = dwData;
                *bInstallIsr = TRUE;
            }
            else
            {
                DEBUGMSG(ZONE_WARNING, (L"!PHCD: Registry values %s and %s found, but no corresponding value %s found\r\n",
                    DEVLOAD_ISRDLL_VALNAME, DEVLOAD_ISRHANDLER_VALNAME, DEVLOAD_IRQ_VALNAME));
            }
        } 
        else
        {
            DEBUGMSG(ZONE_WARNING, (L"!PHCD: Registry value %s found, but no corresponding value %s found\r\n",
                DEVLOAD_ISRDLL_VALNAME, DEVLOAD_ISRHANDLER_VALNAME));
        }
    }

#ifdef OMAP35X_KIT
    // read Irq value which is used as the GPIO line used as the ISP1760 IRQ line
    dwSize = sizeof(dwData);
    dwRet = RegQueryValueEx(hKey, DEVLOAD_IRQ_VALNAME, 0, &dwType, (PUCHAR)&dwData, &dwSize);
    if (dwRet == ERROR_SUCCESS)
    {
        *Irq = dwData;
    }
    else
    {
        DEBUGMSG(ZONE_ERROR, (L"!PHCD: Registry value for Irq not found\r\n"));
        goto GetRegistryConfig_exit;
    }

    // read NumOfPorts value
    dwSize = sizeof(dwData);
    dwRet = RegQueryValueEx(hKey, DEVLOAD_NUMOFPORTS, 0, &dwType, (PUCHAR)&dwData, &dwSize);
    if (dwRet == ERROR_SUCCESS)
    {
        *NumOfPorts = dwData;
    }
    else
    {
        *NumOfPorts = 0;
    }

    // read suspend/wakeup gpio value
    dwSize = sizeof(dwData);
    dwRet = RegQueryValueEx(hKey, DEVLOAD_SUSPENDGPIO, 0, &dwType, (PUCHAR)&dwData, &dwSize);
    if (dwRet == ERROR_SUCCESS)
    {
        *SuspendGpio = dwData;
    }
    else
    {
        *SuspendGpio = 0;
    }
#endif // OMAP35X_KIT

    // Read Instance Index from registry
    dwSize = sizeof(dwData);
    dwRet = RegQueryValueEx(hKey,DEVLOAD_INSTANCEINDEX_VALNAME,0,&dwType,(PUCHAR)&dwData,&dwSize);
    if (dwRet != ERROR_SUCCESS && DEVLOAD_INSTANCEINDEX_VALTYPE == dwType)
    {
        // Default value is 0
        *InstanceIndex = 0;
    }
    else
    {
        *InstanceIndex = dwData;
    }

    // Read Otg option from registry
    dwSize = sizeof(dwData);
    dwRet = RegQueryValueEx(hKey,DEVLOAD_OTG,0,&dwType,(PUCHAR)&dwData,&dwSize);
    if (dwRet != ERROR_SUCCESS && DEVLOAD_OTG_VALTYPE == dwType)
    {
        // Default value is 0
        *OtgSupport = 0;
    }
    else
    {
        *OtgSupport = dwData;
    }

    // Read ALONE option from registry
    dwSize = sizeof(dwData);
    dwRet = RegQueryValueEx(hKey,DEVLOAD_ALONE,0,&dwType,(PUCHAR)&dwData,&dwSize);
    if (dwRet != ERROR_SUCCESS && DEVLOAD_ALONE_VALTYPE == dwType)
    {
        // Default value is 1
        *HostOnlySupport = 1;
    }
    else
    {
        *HostOnlySupport = dwData;
    }
    

    // Read Plx9054 from registry
    dwSize = sizeof(dwData);
    dwRet = RegQueryValueEx(hKey,DEVLOAD_PLX9054_VALNAME,0,&dwType,(PUCHAR)&dwData,&dwSize);
    if (dwRet != ERROR_SUCCESS && DEVLOAD_PLX9054_VALTYPE == dwType)
    {
        // Default value is 0
        *Plx9054LastOwner = 0;
    }
    else
    {
        *Plx9054LastOwner = dwData;
    }

    dwSize = sizeof(dwData);
    dwRet = RegQueryValueEx(hKey,DEVLOAD_PLX9054PORT_VALNAME,0,&dwType,(PUCHAR)&dwData,&dwSize);
    if (dwRet != ERROR_SUCCESS && DEVLOAD_PLX9054PORT_VALTYPE == dwType)
    {
        // Default value is 0
        *Plx9054PhysAddr = 0;
    }
    else
    {
        *Plx9054PhysAddr = dwData;
    }

    fRet = TRUE;

GetRegistryConfig_exit:
    RegCloseKey(hKey);
    return fRet;
}   // GetRegistryConfig



//**************************************************************************
/**	Get the Physical Memory size from the Registry
 *
 *	@param		:		RegKeyPath - Registry key path		
 *
 *	@param		:		lpdwPhyscialMemSize - Base Address
 *
 *	@return		:		TRUE for success, FALSE for error
 *
 ****************************************************************************/

static BOOL
GetRegistryPhysicalMemSize(
    LPCWSTR RegKeyPath,         // IN - driver registry key path
    DWORD * lpdwPhyscialMemSize)       // OUT - base address
{
    HKEY hKey;
    DWORD dwData;
    DWORD dwSize;
    DWORD dwType;
    BOOL  fRet=FALSE;
    DWORD dwRet;
    // Open key
    dwRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,RegKeyPath,0,0,&hKey);
    if (dwRet != ERROR_SUCCESS)
    {
        DEBUGMSG(ZONE_ERROR,(TEXT("!EHCD:GetRegistryConfig RegOpenKeyEx(%s) failed.Error%d\r\n"),
                             RegKeyPath, dwRet));
        return FALSE;
    }

    // Read base address, range from registry and determine IOSpace
    dwSize = sizeof(dwData);
    dwRet = RegQueryValueEx(hKey, REG_PHYSICAL_PAGE_SIZE, 0, &dwType, (PUCHAR)&dwData, &dwSize);
    if (dwRet == ERROR_SUCCESS)
    {
        if (lpdwPhyscialMemSize)
            *lpdwPhyscialMemSize = dwData;
        fRet=TRUE;
    }
    RegCloseKey(hKey);
    return fRet;
}

//**************************************************************************
/**	Verifies if ISP1760 or ISP1761 chip is present
 *
 *  @param      :       Virtual base address pointer to ISP1761 chip
 *
 *	@return		:		TRUE if present, FALSE if absent
 *
 ****************************************************************************/
BOOL IsISP1761Present(DWORD base)
{
    ULONG ulRegData;
    BOOL ret = FALSE;
	PULONG pulPh1761Addr;

    pulPh1761Addr=(PULONG)(base + PHHC_REG_HCCHIPID);
	ulRegData = (ULONG)(*(pulPh1761Addr));		

    DEBUGMSG(ZONE_INIT, (TEXT("\r\nISP1761 USB Chip ID: 0x%08x\r\n"), ulRegData));

    if(PHHC_REG_HCCHIPID_VALUE == (ulRegData & 0x0000FFFF))
        ret = TRUE;

	return  ret;
}

//**************************************************************************
/**	ConfigureEHCICard
 *
 *	@param		:		pPddObject		
 *
 *	@param		:		pioPortBase
 *
 *	@param		:		dwAddrLen
 *
 *	@param		:		dwIOSpace
 *
 *	@param		:		IfcType
 *
 *	@param		:		dwBusNumber
 *
 *	@return		:		TRUE for success, FALSE for error
 *
 ****************************************************************************/ 
BOOL
ConfigureEHCICard(
    SEHCDPdd * pPddObject, // IN - contains PDD reference pointer.
    PUCHAR *pioPortBase,   // IN - contains physical address of register base
                           // OUT- contains virtual address of register base
    DWORD dwAddrLen,
    DWORD dwIOSpace,
    INTERFACE_TYPE IfcType,
    DWORD dwBusNumber
    )
{
    ULONG               inIoSpace = dwIOSpace;
    ULONG               portBase;
    PHYSICAL_ADDRESS    ioPhysicalBase = {0, 0};
    
    portBase = (ULONG)*pioPortBase;
    ioPhysicalBase.LowPart = portBase;

#ifdef PXA25X_DMA
    gulPXARegBasePhyAddr=portBase;
    gulPXARegBasePhyAddr+= 0x1000000;
#endif // PXA25X_DMA

    DEBUGMSG(ZONE_INIT, 
             (TEXT("EHCD: ioPhysicalBase before conversion 0x%X\n"),
              ioPhysicalBase.LowPart));

    DEBUGMSG(ZONE_INIT, 
             (TEXT("EHCD: ioPortBase 0x%X, portBase before conversion 0x%X\r\n"),
              *pioPortBase, portBase));		////////<<<<<<<<< Marlon : For PXA25x DMA support						

#ifdef OMAP35X_KIT
    *pioPortBase = (PUCHAR)MmMapIoSpace(ioPhysicalBase, dwAddrLen, FALSE);
    if (NULL == *pioPortBase)
    {
        DEBUGMSG(ZONE_ERROR, (L"EHCD: Failed MmMapIoSpace\r\n"));
        return FALSE;
    }

    /* try to read the ID register. If unsuccessful, this must be an older rev SOM
     * so bail out gracefully 
     */
    if(!IsISP1761Present((DWORD)*pioPortBase))
    {
        MmUnmapIoSpace(*pioPortBase, dwAddrLen);
        RETAILMSG(1, (L"\r\nWARNING: ISP1761 USB Host Controller not present, driver unloading!\r\n"));
        return FALSE;
    }
#else
    if (!BusTransBusAddrToVirtual(pPddObject->hParentBusHandle, IfcType, dwBusNumber, ioPhysicalBase, dwAddrLen, &inIoSpace, (PPVOID)pioPortBase))
    {
        DEBUGMSG(ZONE_ERROR, (L"EHCD: Failed TransBusAddrToVirtual\r\n"));
        return FALSE;
    }
#endif

    DEBUGMSG(ZONE_INIT, 
             (TEXT("EHCD: ioPhysicalBase 0x%X, IoSpace 0x%X\r\n"),
              ioPhysicalBase.LowPart, inIoSpace));
    DEBUGMSG(ZONE_INIT, 
             (TEXT("EHCD: ioPortBase 0x%X, portBase 0x%X\r\n"),
              *pioPortBase, portBase));

    return TRUE;
}

//*********************************************************************
/** 
 *	Configure and initialize EHCI card
 *
 *	@param			:		pPddObject
 *
 *	@param			:		szDriverRegKey
 *
 *  @return			:		TRUE if card could be located and configured,
 *							otherwise FALSE
 *
 ************************************************************************/
 
static BOOL 
InitializeEHCI(
    SEHCDPdd * pPddObject,    // IN - Pointer to PDD structure
    LPCWSTR szDriverRegKey)   // IN - Pointer to active registry key string
{
    PUCHAR ioPortBase = NULL;
    DWORD dwAddrLen;
    DWORD dwIOSpace;
   
    DWORD	dwSysIntr;
    INTERFACE_TYPE IfcType;
    DWORD	dwBusNumber;
    WCHAR	IsrDll[DEVDLL_LEN];
    WCHAR	IsrHandler[DEVENTRY_LEN];
    DWORD	Irq = -1;

    DWORD	InstanceIndex;
	DWORD   HostOnlySupport;
	DWORD   Plx9054LastOwner;
	DWORD	Plx9054PhysAddr;
	PUCHAR	pbyPLX9054RegBase = NULL;
	DWORD	OtgSupport;
        
    BOOL InstallIsr = FALSE;
    BOOL fResult = FALSE;
    LPVOID pobMem = NULL;
    LPVOID pobEHCD = NULL;
    DWORD PhysAddr;
    DWORD dwHPPhysicalMemSize;
    DWORD realIrq;
    DWORD NumOfPorts;
    DWORD SuspendGpio;
    
   
#ifdef WINCE600_PORTING 
   PCI_SLOT_NUMBER Slot;
   HKEY    hKey;
   DDKPCIINFO dpi;
   HCCP_CAP Hccparam;
   DWORD dwOffset, dwLength, dwCount;
   HANDLE hBus;
   USBLEGSUP Usblegsup;


    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,szDriverRegKey,0,0,&hKey)!= ERROR_SUCCESS) {
        DEBUGMSG(1,(TEXT("InitializeEHCI:GetRegistryConfig RegOpenKeyEx(%s) failed\r\n"),
                             szDriverRegKey));
       
      
       
        return FALSE;
    }
    
    
    dpi.cbSize=sizeof(dpi);
#endif
    
    

	if (!GetRegistryConfig( szDriverRegKey, 
							&PhysAddr,&dwAddrLen,&dwIOSpace,&dwSysIntr,&IfcType,&dwBusNumber, 
							&InstallIsr, IsrDll, IsrHandler, &Irq, 
							&InstanceIndex,
							&OtgSupport,
							&HostOnlySupport,
							&NumOfPorts,
                            &SuspendGpio,
							&Plx9054LastOwner,&Plx9054PhysAddr)) {
        RETAILMSG(1,(TEXT("!PHCD: Error reading registry settings\r\n")));
        return FALSE;
    }
    

#ifdef PLX9054

	pbyPLX9054RegBase = PCIKit_GetPLX9054RegBase(pPddObject->hParentBusHandle,
													Plx9054PhysAddr,
													PLX9054_REGSZ,
													1/*dwIOSpace*/,//Praveen 
													IfcType,
													dwBusNumber);
													


	gpbyPCIPLX9054RegBase=pbyPLX9054RegBase;

	if(!Plx9054LastOwner || HostOnlySupport) 
 		PreInit_ISP1761WinCEKit(pbyPLX9054RegBase);


#endif
		


    DEBUGMSG(ZONE_INIT,(TEXT("PHCD: Read config from registry: Base Address: 0x%X, Length: 0x%X, I/O Port: %s, SysIntr: 0x%X, Interface Type: %u, Bus Number: %u\r\n"),
                        PhysAddr, dwAddrLen, dwIOSpace ? L"YES" : L"NO", dwSysIntr, IfcType, dwBusNumber));

	DEBUGMSG(ZONE_INIT,(TEXT("PLX Base Address %x\n"),Plx9054PhysAddr));

    ioPortBase = (PUCHAR)PhysAddr;
    
    if (!(fResult = ConfigureEHCICard(pPddObject, &ioPortBase, dwAddrLen, dwIOSpace,IfcType, dwBusNumber)))
    {
        goto InitializeEHCI_Error;
    }

    

    DEBUGMSG(ZONE_INIT, (TEXT("HCD:Install ISR %x \n"),InstallIsr));
		
  	  if (InstallIsr)
  	  {  

 	  	
      	// Install ISR handler

		pPddObject->IsrHandle =LoadIntChainHandler(IsrDll, IsrHandler, (BYTE)Irq);	

        if (!pPddObject->IsrHandle)
        {
            DEBUGMSG(ZONE_ERROR, (L"EHCD: Couldn't install ISR handler.Error\r\n"));
     
        }
        else
        {
            GIISR_INFO Info;
            PHYSICAL_ADDRESS PortAddress = {PhysAddr, 0};
     
            if (!BusTransBusAddrToStatic(pPddObject->hParentBusHandle,IfcType,dwBusNumber,PortAddress, dwAddrLen, &dwIOSpace, &(PVOID)PhysAddr))
            {
                DEBUGMSG(ZONE_ERROR, (L"EHCD: Failed TransBusAddrToStatic\r\n"));
                return FALSE;
            }


            /* the following code has been used when we share 
          	   the interrupt line between different drivers.
          	   Here we share interrupt line between HC and DC */
                
            // Set up ISR handler
            Info.SysIntr = dwSysIntr;
            Info.CheckPort = FALSE; //TRUE;
            Info.PortIsIO = (dwIOSpace) ? TRUE : FALSE;
            Info.UseMaskReg = FALSE; //TRUE;
          // 	Info.PortAddr = PhysAddr +PHHC_REG_HCINTR; // HC Interrupt register
          // 	Info.PortAddr = PhysAddr +0x68; //0x94  0xA9 HC Interrupt register
            Info.PortSize = sizeof(DWORD);
          //  Info.MaskAddr = PhysAddr +PHHC_REG_HCINTRENBL; // HC Interrupt enable register
          //  Info.MaskAddr = PhysAddr +0x94; // HC Interrupt enable register  
            
            if (!KernelLibIoControl(pPddObject->IsrHandle, IOCTL_GIISR_INFO, &Info, sizeof(Info), NULL, 0, NULL))
            {
                DEBUGMSG(ZONE_ERROR, (L"EHCD: KernelLibIoControl call failed.\r\n"));
            }
        }
    }
#ifdef WINCE600_PORTING
    
    DEBUGMSG(1,(TEXT("EHCD: Read config from registry: Base Address: 0x%X, Length: 0x%X, I/O Port: %s, SysIntr: 0x%X, Interface Type: %u, Bus Number: %u\r\n"),
                        PhysAddr, dwAddrLen, dwIOSpace ? L"YES" : L"NO", dwSysIntr,IfcType, dwBusNumber));

    // Ask pre-OS software to release the control of the EHCI hardware.
    if (IfcType == PCIBus) 
    {
    	if ( DDKReg_GetPciInfo(hKey, &dpi ) == ERROR_SUCCESS)
    		 { 
    		 	Slot.u.AsULONG = 0;
          Slot.u.bits.DeviceNumber = dpi.dwDeviceNumber;
          Slot.u.bits.FunctionNumber = dpi.dwFunctionNumber;
          Hccparam.ul = READ_REGISTER_ULONG((PULONG)(ioPortBase + 0x314));
          dwOffset = Hccparam.bit.EHCI_Ext_Cap_Pointer;
          
           if (dwOffset)
           	 {
                ASSERT(dwOffset>=0x40); // EHCI spec 2.2.4
                hBus = CreateBusAccessHandle(szDriverRegKey);
    						 if (hBus)
    						 	 {
    						 	  while (dwOffset)
    						 	   {
    						 	  	dwLength = GetDeviceConfigurationData(hBus, PCI_WHICHSPACE_CONFIG, dwBusNumber, Slot.u.AsULONG, dwOffset, sizeof(Usblegsup), &Usblegsup);
    						 	  	if (dwLength == sizeof(Usblegsup))
    						 	  		 {
    						 	  		if (Usblegsup.bit.Cap_ID == 1)
    						 	  			 {
    						 	  			Usblegsup.bit.HC_OS_Owned = 1;
    						 	  			 dwLength = SetDeviceConfigurationData(hBus, PCI_WHICHSPACE_CONFIG, dwBusNumber, Slot.u.AsULONG, dwOffset, sizeof(Usblegsup), &Usblegsup);
    						 	  			  break;
    						 	  			   } 
    						 	  			   else
    						 	  			   {
    						 	  			   	 dwOffset = Usblegsup.bit.Next_Cap_Pointer;
    						 	  			   	  continue;
    						 	  			   	}
                        } 
                        else
                        	 {
                        	 dwOffset = 0;
                        }
                    }
            if (dwOffset && dwLength == sizeof(Usblegsup))
            	 {
                        // Wait up to 1 sec for pre-OS software to release the EHCI hardware.
                        dwCount = 0;
                        
                        while (dwLength == sizeof(Usblegsup) && dwCount < 1000) {
                        	 dwLength = GetDeviceConfigurationData(hBus, PCI_WHICHSPACE_CONFIG, dwBusNumber, Slot.u.AsULONG, dwOffset, sizeof(Usblegsup), &Usblegsup);
                        	 if (Usblegsup.bit.HC_OS_Owned == 1 && Usblegsup.bit.HC_BIOS_Owned == 0) {
                        	 	 break;
                            }
                             dwCount++;
                            Sleep(1);
                              }
                    }
                }
                 if (hBus) {
                    CloseBusAccessHandle(hBus);
                        }
            }
        }
    }
    
#endif
    
    // The PDD can supply a buffer of contiguous physical memory here, or can let the 
    // MDD try to allocate the memory from system RAM.  We will use the HalAllocateCommonBuffer()
    // API to allocate the memory and bus controller physical addresses and pass this information
    // into the MDD.
    
    if (GetRegistryPhysicalMemSize(szDriverRegKey,&pPddObject->dwPhysicalMemSize))
    {
        // A quarter for High priority Memory.
        dwHPPhysicalMemSize = pPddObject->dwPhysicalMemSize/4;
        // Align with page size.        
        pPddObject->dwPhysicalMemSize = (pPddObject->dwPhysicalMemSize + PAGE_SIZE -1) & ~(PAGE_SIZE -1);
        dwHPPhysicalMemSize = ((dwHPPhysicalMemSize +  PAGE_SIZE -1) & ~(PAGE_SIZE -1));
    }
    else 
        pPddObject->dwPhysicalMemSize=0;
    
    if (pPddObject->dwPhysicalMemSize<gcTotalAvailablePhysicalMemory)   // Setup Minimun requirement.
    { 
        pPddObject->dwPhysicalMemSize = gcTotalAvailablePhysicalMemory;
        dwHPPhysicalMemSize = gcHighPriorityPhysicalMemory;
    }

    pPddObject->AdapterObject.ObjectSize = sizeof(DMA_ADAPTER_OBJECT);
    pPddObject->AdapterObject.InterfaceType = IfcType;
    pPddObject->AdapterObject.BusNumber = dwBusNumber;
    
    if ((pPddObject->pvVirtualAddress = HalAllocateCommonBuffer(&pPddObject->AdapterObject, pPddObject->dwPhysicalMemSize, &pPddObject->LogicalAddress, FALSE)) == NULL)
   	{
        goto InitializeEHCI_Error;
    }

    //MPK on 17/11/2006
    DEBUGMSG(ZONE_INIT,(TEXT("HaAllocCommonBuffer Virt. Address %x \n"),pPddObject->pvVirtualAddress));
    DEBUGMSG(ZONE_INIT,(TEXT("HaAllocCommonBuffer:Logical Addr LowPart %x HighPart %x \n"),(PUCHAR) pPddObject->LogicalAddress.LowPart,
    						(PUCHAR) pPddObject->LogicalAddress.HighPart));
    if (!(pobMem = HcdMdd_CreateMemoryObject(pPddObject->dwPhysicalMemSize, dwHPPhysicalMemSize, (PUCHAR) pPddObject->pvVirtualAddress, (PUCHAR) pPddObject->LogicalAddress.LowPart)))
    {
    	goto InitializeEHCI_Error;
    }

#ifdef OMAP35X_KIT
    PostInit_ISP1761OMAP35xKit(pPddObject->m_hGPIO, Irq, &realIrq, SuspendGpio);
    if (!KernelIoControl( 
            IOCTL_HAL_REQUEST_SYSINTR, 
            &realIrq, 
            sizeof(realIrq),
            &dwSysIntr, 
            sizeof(dwSysIntr), 
            NULL) ) 
    {
        DEBUGMSG(ZONE_ERROR, (L"EHCD: IOCTL_HAL_REQUEST_SYSINTR Failed.\r\n"));
        goto InitializeEHCI_Error;
    }

    pPddObject->NumOfPorts = NumOfPorts;
#endif

    if (!(pobEHCD = HcdMdd_CreateHcdObject(pPddObject, pobMem, szDriverRegKey, ioPortBase, dwSysIntr, NumOfPorts)))

    {
        goto InitializeEHCI_Error;
    }

#ifdef PLX9054	
    if(Plx9054LastOwner)
        PostInit_ISP1761WinCEKit(pbyPLX9054RegBase);
#endif


    pPddObject->lpvMemoryObject = pobMem;
    pPddObject->lpvEHCDMddObject = pobEHCD;
    _tcsncpy(pPddObject->szDriverRegKey, szDriverRegKey, MAX_PATH);
    pPddObject->ioPortBase = ioPortBase;
    pPddObject->dwSysIntr = dwSysIntr;

    return TRUE;

InitializeEHCI_Error:
#ifdef OMAP35X_KIT
    if(0 != dwSysIntr)
    {
        KernelIoControl( 
            IOCTL_HAL_RELEASE_SYSINTR, 
            &dwSysIntr, 
            sizeof(dwSysIntr), 
            NULL, 
            0,
            NULL);
    }
#endif

    if (pPddObject->IsrHandle)
    {
        FreeIntChainHandler(pPddObject->IsrHandle);
        pPddObject->IsrHandle = NULL;
    }
    
    if (pobEHCD)
        HcdMdd_DestroyHcdObject(pobEHCD);
    if (pobMem)
        HcdMdd_DestroyMemoryObject(pobMem);
    if(pPddObject->pvVirtualAddress)
        HalFreeCommonBuffer(&pPddObject->AdapterObject,gcTotalAvailablePhysicalMemory, pPddObject->LogicalAddress, pPddObject->pvVirtualAddress, FALSE);
     
    pPddObject->lpvMemoryObject = NULL;
    pPddObject->lpvEHCDMddObject = NULL;
    pPddObject->pvVirtualAddress = NULL;

    return FALSE;
    
}

//**************************************************************************
/** 
 *	PDD Entry point - called at system init to detect and configure EHCI card.
 *   
 *	@param			:	dwContext
 *
 *	@return			:	pointer to PDD specific data structure, or NULL if error.
 *
 ****************************************************************************/
extern DWORD 
HcdPdd_Init(
    DWORD dwContext)  // IN - Pointer to context value. For device.exe, this is a string 
                      //      indicating our active registry key.
{
    SEHCDPdd *  pPddObject = malloc(sizeof(SEHCDPdd));
    BOOL        fRet = FALSE;

    if (pPddObject)
    {
		#ifdef PXA_KIT
			Init_ISP176xWinCEKit();
		#endif

        #ifdef OMAP35X_KIT
            pPddObject->m_hGPIO = PreInit_ISP1761OMAP35xKit();
        #endif

        pPddObject->pvVirtualAddress = NULL;
        InitializeCriticalSection(&pPddObject->csPdd);
        pPddObject->IsrHandle = NULL;
        pPddObject->hParentBusHandle = CreateBusAccessHandle((LPCWSTR)g_dwContext); 
        
        fRet = InitializeEHCI(pPddObject, (LPCWSTR)dwContext);

        if(!fRet)
        {
            if (pPddObject->hParentBusHandle)
                CloseBusAccessHandle(pPddObject->hParentBusHandle);
            
            DeleteCriticalSection(&pPddObject->csPdd);
            free(pPddObject);
            pPddObject = NULL;
        }
    }

    return (DWORD)pPddObject;
}

//***************************************************************************
/** 
 *  Check power required by specific device configuration and return whether it
 *  can be supported on this platform.  For CEPC, this is trivial, just limit to
 *  the 500mA requirement of USB.  For battery powered devices, this could be 
 *  more sophisticated, taking into account current battery status or other info.
 *
 *	@param			:		bPort	
 *
 *	@param			:		dwCfgPower
 *
 *	@param			:		dwTotalPower
 *
 *	@return			:		TRUE if configuration can be supported, FALSE if not.
 *
 *****************************************************************************/
extern BOOL HcdPdd_CheckConfigPower(
    UCHAR bPort,         // IN - Port number
    DWORD dwCfgPower,    // IN - Power required by configuration
    DWORD dwTotalPower)  // IN - Total power currently in use on port
{
    return ((dwCfgPower + dwTotalPower) > 500) ? FALSE : TRUE;
}


//***************************************************************************
/**	
 *	This function executes when the system boots.
 *	This function must call the HcdMdd_PowerUp function to enable 
 *	the model device driver (MDD) to restore state information.
 *	The host controller interface (HCI) memory area, as passed to 
 *	the HcdMdd_CreateHcdObject function, must be preserved across a power cycle.
 *	No system calls can be made from within this function.
 *
 *	@param			:		hDeviceContext
 *
 *	@return			:		Nothing
 *
 *****************************************************************************/
extern void HcdPdd_PowerUp(DWORD hDeviceContext)
{
    SEHCDPdd * pPddObject = (SEHCDPdd *)hDeviceContext;
    HcdMdd_PowerUp(pPddObject->lpvEHCDMddObject);

    return;
}

//***************************************************************************
/**
 *	This function executes when the system is turned off.
 *	This function must call the HcdMdd_PowerDown function to enable 
 *	the model device driver (MDD) to save state information.
 *	Then, any processing specific to the platform is performed, such as removing 
 *	power from the host controller hardware.
 *	No system calls can be made from within this function.
 *
 *	@param		:		hDeviceContext
 *
 *	@return		:		Nothing
 *
 *****************************************************************************/

extern void HcdPdd_PowerDown(DWORD hDeviceContext)
{
    SEHCDPdd * pPddObject = (SEHCDPdd *)hDeviceContext;

    HcdMdd_PowerDown(pPddObject->lpvEHCDMddObject);

    return;
}

//***************************************************************************
/**	
 *	This function is called if the host controller interface driver is unloaded.
 *	As part of processing this function, the HcdMdd_DestroyHcdObject function 
 *	executes to enable the model device driver (MDD) to clean up internal data structures.
 *
 *	@param		:		hDeviceContext
 *
 *	@return		:		TRUE if configuration can be supported, FALSE if not.
 *
 *****************************************************************************/
 
extern BOOL HcdPdd_Deinit(DWORD hDeviceContext)
{
    SEHCDPdd * pPddObject = (SEHCDPdd *)hDeviceContext;

#ifdef OMAP35X_KIT
    if(0 != pPddObject->dwSysIntr)
    {
        KernelIoControl( 
            IOCTL_HAL_RELEASE_SYSINTR, 
            &(pPddObject->dwSysIntr), 
            sizeof(pPddObject->dwSysIntr), 
            NULL, 
            0,
            NULL);
    }

    DeInit_ISP1761OMAP35xKit(pPddObject->m_hGPIO);
#endif

    if(pPddObject->lpvEHCDMddObject)
        HcdMdd_DestroyHcdObject(pPddObject->lpvEHCDMddObject);
    if(pPddObject->lpvMemoryObject)
        HcdMdd_DestroyMemoryObject(pPddObject->lpvMemoryObject);
    if(pPddObject->pvVirtualAddress)
        HalFreeCommonBuffer(&pPddObject->AdapterObject, pPddObject->dwPhysicalMemSize, pPddObject->LogicalAddress, pPddObject->pvVirtualAddress, FALSE);

    if (pPddObject->IsrHandle)
    {
        FreeIntChainHandler(pPddObject->IsrHandle);
        pPddObject->IsrHandle = NULL;
    }
    
    if (pPddObject->hParentBusHandle)
        CloseBusAccessHandle(pPddObject->hParentBusHandle);
    
    free(pPddObject);
    return TRUE;
    
}

//***************************************************************************
/**
*****************************************************************************/
extern DWORD HcdPdd_Open(DWORD hDeviceContext, DWORD AccessCode,
        DWORD ShareMode)
{
    UnusedParameter(hDeviceContext);
    UnusedParameter(AccessCode);
    UnusedParameter(ShareMode);

    return 1; // we can be opened, but only once!
}

//***************************************************************************
/**
 *	This function is the standard stream interface driver close routine. 
 *	It is not currently implemented.
 *
 *****************************************************************************/
extern BOOL HcdPdd_Close(DWORD hOpenContext)
{
    UnusedParameter(hOpenContext);

    return TRUE;
}

//***************************************************************************
/**
 *	This function is the standard stream interface driver read routine. 
 *	It currently is not implemented.
 *
 *****************************************************************************/
extern DWORD HcdPdd_Read(DWORD hOpenContext, LPVOID pBuffer, DWORD Count)
{
    UnusedParameter(hOpenContext);
    UnusedParameter(pBuffer);
    UnusedParameter(Count);

    return (DWORD)-1; // an error occured
}

//***************************************************************************
/**	
 *	This function is the standard stream interface driver write routine.
 *	It currently is not implemented.
 *
 *****************************************************************************/
extern DWORD HcdPdd_Write(DWORD hOpenContext, LPCVOID pSourceBytes,
        DWORD NumberOfBytes)
{
    UnusedParameter(hOpenContext);
    UnusedParameter(pSourceBytes);
    UnusedParameter(NumberOfBytes);

    return (DWORD)-1;
}

//***************************************************************************
/** 
 *	This function is the standard stream interface driver seek routine.
 *	It currently is not implemented.
 *
 *****************************************************************************/
extern DWORD HcdPdd_Seek(DWORD hOpenContext, LONG Amount, DWORD Type)
{
    UnusedParameter(hOpenContext);
    UnusedParameter(Amount);
    UnusedParameter(Type);

    return (DWORD)-1;
}

//***************************************************************************
/**
 *	This function is the standard stream interface driver IOCTL routine
 *	It currently is not implemented.
 *
 *****************************************************************************/
extern BOOL HcdPdd_IOControl(DWORD hOpenContext, DWORD dwCode, PBYTE pBufIn,
        DWORD dwLenIn, PBYTE pBufOut, DWORD dwLenOut, PDWORD pdwActualOut)
{
#ifdef TEST_HSET	
   BOOL bReturn; // Marlon : fix HSTEST compile error
#endif   
   SEHCDPdd * pPddObject = (SEHCDPdd *)hOpenContext;
	
	switch (dwCode) 
	{
#ifdef TEST_HSET	
	case IOCTL_HSET_DEBUG:
		bReturn=HcdMdd_HSET((LPVOID)(pPddObject->lpvEHCDMddObject),pBufIn);
		return (!bReturn);/*1 is considered as Failure and 0 is considered as pass*/
#endif
	case IOCTL_POWERDOWN:
		HcdMdd_PowerDown((LPVOID)(pPddObject->lpvEHCDMddObject));
		return TRUE;
	case IOCTL_POWERUP:
	    HcdMdd_PowerUp((LPVOID)(pPddObject->lpvEHCDMddObject));
		return TRUE;

	case IOCTL_HOST_TO_DEVICE:
		HcdMdd_OtgPortControl((LPVOID)(pPddObject->lpvEHCDMddObject),TRUE);
		return TRUE;
	case IOCTL_DEVICE_TO_HOST:
		HcdMdd_OtgPortControl((LPVOID)(pPddObject->lpvEHCDMddObject),FALSE);
		return TRUE;	
	

	default:
		break;
	}
    UnusedParameter(dwCode);
    UnusedParameter(dwLenIn);
    UnusedParameter(pBufOut);
    UnusedParameter(dwLenOut);
    UnusedParameter(pdwActualOut);

    return FALSE;
}


//***************************************************************************
/**	
 *	Manage WinCE suspend/resume events
 *	This gets called by the MDD's IST when it detects a power resume.
 *	By default it has nothing to do.
 *****************************************************************************/
extern void HcdPdd_InitiatePowerUp (DWORD hDeviceContext)
{
    return;
}
