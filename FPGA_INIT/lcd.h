/*
================================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
================================================================================
*/

#ifndef __LCD_H__
#define __LCD_H__

#ifdef __cplusplus
extern "C" {
#endif


//------------------------------------------------------------------------------
//  Defines/Enums

//
//  TV Out display controller settings
//

#define NTSC_WIDTH      720
#define NTSC_HEIGHT     480

#define PAL_WIDTH       720
#define PAL_HEIGHT      576

#define TVOUT_COMPOSITE (0x0A)
#define TVOUT_SVIDEO    (0x0D)

//------------------------------------------------------------------------------
//
//  LCD PDD Functions
//
//  LCD Physical Layer Interace
//

//------------------------------------------------------------------------------
//
//  LCD PDD Functions
//
//  LCD Physical Layer Interace
//
//EF - ported begin
struct _lcdPanlelInfo{
    UCHAR* pName;
    DWORD DEFAULT_PIXELTYPE;
    DWORD LCD_WIDTH;
    DWORD LCD_HEIGHT;
	DWORD LCD_HSW;
	DWORD LCD_HFP;
	DWORD LCD_HBP;
	DWORD LCD_VSW;
	DWORD LCD_VFP;
	DWORD LCD_VBP;
	DWORD LCD_LOGCLKDIV;
	DWORD LCD_PIXCLKDIV;
	DWORD LCD_LOADMODE;
	DWORD LCD_POLFREQ;
	DWORD LCD_DEFAULT_COLOR;
	DWORD LCD_TRANS_COLOR;
	DWORD TV_DEFAULT_COLOR;
	DWORD TV_TRANS_COLOR;
};

typedef struct _lcdPanlelInfo lcdPanlelInfo;

BOOL 
LcdPdd_InitPowerLevel();   

//EF - ported end

BOOL
LcdPdd_LCD_Initialize(
    OMAP_DSS_REGS         *pDSSRegs,
    OMAP_DISPC_REGS       *pDispRegs,
    OMAP_RFBI_REGS        *pRfbiRegs,
    OMAP_VENC_REGS        *pVencRegs,
    OMAP_PRCM_DSS_CM_REGS *pPrcmDssCM,
	BOOL				   skipLcdInit
    );
        
BOOL
LcdPdd_TV_Initialize(
    OMAP_DSS_REGS       *pDSSRegs,
    OMAP_DISPC_REGS     *pDispRegs,
    OMAP_RFBI_REGS      *pRfbiRegs,
    OMAP_VENC_REGS      *pVencRegs
    );


BOOL
LcdPdd_GetMemory(
    DWORD   *pVideoMemLen,
    DWORD   *pVideoMemAddr
    );

BOOL
LcdPdd_LCD_GetMode(
    DWORD   *pPixelFormat,
    DWORD   *pWidth,
    DWORD   *pHeight,
    DWORD   *pPixelClock
    );

BOOL
LcdPdd_TV_GetMode(
    DWORD   *pWidth,
    DWORD   *pHeight,
    DWORD   *pMode
    );


BOOL
LcdPdd_SetPowerLevel(
    DWORD   dwPowerLevel
    );

BOOL
LcdPdd_DVI_Select(
    BOOL bEnable
    );

BOOL
LcdPdd_DVI_Enabled(void);

BOOL LcdInitGpio(void);

DWORD
LcdPdd_Get_PixClkDiv(void);

#ifdef __cplusplus
}
#endif

#endif __LCD_H__

