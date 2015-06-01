#pragma once

#include "CustLang.h"
#ifdef WINCE
	#define InitFolder L"\\FlashDisk\\Programs\\InitWinSmart\\"
#else
	#define InitFolder L"E:\\work\\projects\\InitWinSmartBoth\\"
#endif
#define DesktopBmp L"InitDesktop.bmp"
#define FPGAFileName L"FPGA.rbf"


#define DesktopBmpCMD ( InitFolder DesktopBmp )
#define FPGAFileCMD ( InitFolder FPGAFileName )


#define MainRectX		100
#define MainRectY		110
#define MainRectWidth   415
#define MainRectHeight  245

#define textOffsetX      15

#define textYStart		 15
#define textOffsetY		 74

#define prgBarOffsetX    30
#define prgBarOffsetY    28

#define prgBarWidth     320
#define prgBarHeight     30