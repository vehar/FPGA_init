#include <windows.h>
#include "InitWin.h"
#include <commctrl.h>

#include "custwnd.h"
#include "custimage.h"
#include "CustProgrBar.h"
#include <Tlhelp32.h>

//#include <commctrl.h>
//#include "pxa320_lib.h"

#ifdef WINCE
	#include "UniDriver.h"
	#include "ClockMgr.h"
	#include "GPIO.h"
	#include "FPGA.h"
	#include "I2C.h"
#endif

#include "resource.h"
#include "CustErrorList.h"
#include "CustMsgBox.h"
#include "KB_Keys.h"
#include "CeCustOperations.h"
#include "CustTools.h"
#include "CustThread.h"
#include "CustDbgOutput.h"
#include "CustLang.h"
#include "InitWinSmartDict.h"

//#define MAX_LOADSTRING		100


#define WM_MYMSG_FPGAPRG  (WM_USER + 1)
#define WM_MYMSG_FPGADTST (WM_USER + 2)
#define WM_MYMSG_FPGAATST (WM_USER + 3)

// Global Variables:
HINSTANCE			g_hInst;			// current instance
BOOL			InitInstance(HINSTANCE, int);

CustWnd *MainW;
CustProgrBar *PrgProgrBar, *PrgFPGATestAddr, *PrgFPGATestData;
CustThread *AdditionalThread;

HBRUSH BrWorkPlace;//,Br2;
//HPEN PnYellow;
HANDLE  hBPWallPaper;
RECT rcWorkPlace;
BLENDFUNCTION alpha;
unsigned char first;
char Success;
char Ok, Error;
HFONT hFont, hPrgFont;
//LOGFONT lplf = {-13, 0, 0, 0, 700, 0, 0, 0, 1, 0, 2, 1, 49, L"Courier New"};

//const LOGFONT PrgBarLogFont  = {-21, 0, 0, 0, 700, 0, 0, 0, 1, 0, 2, CLEARTYPE_QUALITY, 18, L"Times New Roman"};
//const LOGFONT MessageLogFont = {-21, 0, 0, 0, 700, 0, 0, 0, 1, 0, 2, CLEARTYPE_QUALITY/*DEFAULT_QUALITY*/, 18, L"Times New Roman"};

const LOGFONT PrgBarLogFont  = {-21, 0, 0, 0, 700, 0, 0, 0, 1, 0, 2, CLEARTYPE_QUALITY, 18, L"Times New Roman"};

const LOGFONT MessageLogFont = {-24, 0, 0, 0, 400, 0, 0, 0, 1, 0, 2, CLEARTYPE_QUALITY, 18, L"Times New Roman"}; 
//const LOGFONT MessageLogFont = {-21, 0, 0, 0, 700, 0, 0, 0, 1, 0, 2, CLEARTYPE_QUALITY, 21, L"Times New Roman"};


unsigned char CanExit; 
unsigned char Value;
bool Initialized = false;
bool NowBlackScreen = false;

//void I2CAScanConf();

LRESULT CALLBACK    MainWProc(HWND, UINT, WPARAM, LPARAM);
VOID CALLBACK TimerProc(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime);

DWORD ThreadProc(LPVOID lpParameter);
int FPGA_Programing();
int FPGA_DBUS_TEST();
int FPGA_ABUS_TEST();
char Step;
bool ActivateCustCon;
bool ActivateWinMob;

HDC hDcWallpaper;
HANDLE hOldWallpaper;



#ifdef WINCE

int APIENTRY _tWinMain(HINSTANCE hInstance,          
					   HINSTANCE hPrevInstance,
					   LPTSTR    lpCmdLine,
					   int       nCmdShow)
#else
 int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#endif
//int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MSG msg;	
	
	Value = 0;
	//	InitializeCriticalSection(&critsect);
	ActivateCustCon = false;
	ActivateWinMob = false;
	CustLang::LoadCurLang();
	CustMsgBox::SetInstances(hInstance, GetDesktopWindow());
    SetCursorPos(750,500);

#ifdef WINCE
	FPGA_Init();
#endif
	CanExit = 0;
	
	if (!InitInstance(hInstance, nCmdShow)) 
	{
		return FALSE;
	}

	InvalidateRect(MainW->GetHWnd(),0,0);
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		if ((msg.message==WM_KEYDOWN)||(msg.message==WM_KEYUP))
			msg.hwnd = MainW->GetHWnd();
		//	if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
		//	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		//	}
		if(ActivateCustCon)
		{
			int result = CustMsgBox::Show(ListExplorerDict::GetString(DICT_PCCON), ListExplorerDict::GetString(DICT_QACTIVATEPCCON), MB_YESNO);
            if(result)
			{
				SetMdrIdNames();
				SwitchConnectionType(CustomConn);
				SaveRegistry();
				CpuReset();
			}
			ActivateCustCon = false;
		}

		if(ActivateWinMob)
		{
			int result = CustMsgBox::Show(ListExplorerDict::GetString(DICT_PCCON), ListExplorerDict::GetString(DICT_QACTIVATEACTSYNC), MB_YESNO);
			if(result)
			{
				SwitchConnectionType(WinMobileConn);
				SaveRegistry();
				CpuReset();
			}
			ActivateWinMob = false;
		}
		if(CanExit)
		{
			if(CanExit==1)
			{
				//for(int i=0;i<3;i++)
					//Sleep(1000);
			}
			//SendMessage(MainW->GetHWnd(),WM_DESTROY,0,0);
		}
	}
    delete PrgProgrBar;
	delete PrgFPGATestAddr;
	delete PrgFPGATestData;
	delete MainW;
	delete AdditionalThread;
	return (int) msg.wParam;
}



BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	g_hInst = hInstance; // Store instance handle in our global variable
#ifndef WINCE
	SetHInstance(g_hInst);
#endif    
//	WCHAR xx[33];
#ifdef WINCE
	MainW = new CustWnd(L"MainW", MainWProc, WS_VISIBLE , CW_USEDEFAULT, 0, 640, 480, NULL, NULL, hInstance, NULL);
#else
	MainW = new CustWnd(L"MainW", MainWProc, WS_OVERLAPPED | WS_SYSMENU  , CW_USEDEFAULT, 0, 640, 480, NULL, NULL, hInstance, NULL);
#endif
	CustMsgBox::SetInstances(hInstance, MainW->GetHWnd());
	if (!MainW->GetHWnd())
	{
		MessageBox(GetDesktopWindow(),L"Error creating main window",L"Error",MB_OK);
		return FALSE;
	}
	//
	if (!MainW->GetHWnd())
	{
		return FALSE;
	}

	rcWorkPlace.left     =	MainRectX;
	rcWorkPlace.top		 =	MainRectY;
	rcWorkPlace.right	 =  rcWorkPlace.left+MainRectWidth;
	rcWorkPlace.bottom   =  rcWorkPlace.top+MainRectHeight;

	//SetTimer(MainW->GetHWnd(), 0x1001, 100, (TIMERPROC)TimerProc);

	PrgProgrBar = new CustProgrBar(hInstance, MainW->GetHWnd(), rcWorkPlace.left + prgBarOffsetX, rcWorkPlace.top + (textYStart + prgBarOffsetY), prgBarWidth, prgBarHeight);

	PrgProgrBar->SetBorderWidth(1);
	PrgProgrBar->SetColor(RGB(200,0,0), PB_BorderColorI);
	PrgProgrBar->SetColor(RGB(140,140,140), PB_BackroundColorI);
	PrgProgrBar->SetColor(RGB(255,255,255), PB_TextColorI);
	PrgProgrBar->SetColor(RGB(28,22,220), PB_BarColorI);
	hPrgFont = CreateFontIndirect(&PrgBarLogFont);
	PrgProgrBar->SetTextFont(hPrgFont);
	
	PrgFPGATestAddr = new CustProgrBar(hInstance, MainW->GetHWnd(), rcWorkPlace.left + prgBarOffsetX, rcWorkPlace.top + textYStart + textOffsetY+prgBarOffsetY , prgBarWidth, prgBarHeight);

	PrgFPGATestAddr->SetBorderWidth(1);
	PrgFPGATestAddr->SetColor(RGB(200,0,0), PB_BorderColorI);
	PrgFPGATestAddr->SetColor(RGB(140,140,140), PB_BackroundColorI);
	PrgFPGATestAddr->SetColor(RGB(255,255,255), PB_TextColorI);
	PrgFPGATestAddr->SetColor(RGB(28,22,220), PB_BarColorI);
	PrgFPGATestAddr->SetTextFont(hPrgFont);

	
	PrgFPGATestData = new CustProgrBar(hInstance, MainW->GetHWnd(), rcWorkPlace.left + prgBarOffsetX, rcWorkPlace.top + textYStart + textOffsetY *2+prgBarOffsetY, prgBarWidth, prgBarHeight);
	

	PrgFPGATestData->SetBorderWidth(1);
	PrgFPGATestData->SetColor(RGB(200,0,0), PB_BorderColorI);
	PrgFPGATestData->SetColor(RGB(140,140,140), PB_BackroundColorI);
	PrgFPGATestData->SetColor(RGB(255,255,255), PB_TextColorI);
	PrgFPGATestData->SetColor(RGB(28,22,220), PB_BarColorI);
	PrgFPGATestData->SetTextFont(hPrgFont);
	Initialized = true;
	ShowWindow(MainW->GetHWnd(), SW_SHOW);
	UpdateWindow(MainW->GetHWnd());   
	return TRUE;
}



//{-15, 0, 0, 0, 700, 0, 0, 0, 1, 0, 2, 1, 34, L"Tahoma"}; 
//{-16, 0, 0, 0, 400, 0, 0, 0, 1, 0, 2, 1, 49, L"Courier New"};
//{18,7,0,800,1,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,0};



LRESULT CALLBACK MainWProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	//int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hDc, hDcWorkPlace;
	//RECT ClientRect;
	HANDLE hOldWorkPlace, hOldFont/*, hOldPen*/;
	//HANDLE hBPTemp;
//	WCHAR WStrTemp[50];
	//WCHAR WStrTemp1[30];
	switch (message) 
	{
	case WM_CREATE: 

		first   = 1;
		Success = -33;
		Ok      = 1;
		Error   = 0;
		Step    = 0;
//		PnYellow        = CreatePen(PS_SOLID, 1, RGB(255,255,0));
		BrWorkPlace     = CreateSolidBrush(RGB(155,150,158));
		//BrWorkPlace     = CreateSolidBrush(RGB(50,50,255));

		
		hBPWallPaper =(HANDLE) SHLoadDIBitmap(DesktopBmpCMD);
	
		alpha.AlphaFormat=0;
		alpha.BlendFlags=0;
		alpha.BlendOp=0;
		alpha.SourceConstantAlpha=240;
		InitAlphaBlendFunc();
		//CreateThread(NULL ,NULL, ThreadProc, 0, 0, NULL );
		AdditionalThread = new CustThread(ThreadProc,NULL);
		hFont = CreateFontIndirect(&MessageLogFont);
		break;
	case WM_KEYDOWN:
		switch(wParam)
		{
		case FPKey_F1:
			ActivateCustCon = true;
			break;
// 		case FPKey_F2:
// 			ActivateWinMob = true;
// 			break;
		default:
			if(Error>0)
			{
				CanExit=1;
			}
		}
		break;
	case WM_KEYUP:
		switch(wParam)
		{
		case FPKey_F1:
            ActivateCustCon = false;
			break;
// 		case FPKey_F2:
// 			ActivateWinMob = false;
// 			break;
		}
		break;

	case WM_MYMSG_FPGAPRG:
		Success = wParam;
		Value = 100;
		InvalidateRect(hWnd, NULL, 0);
		break;
	case WM_MYMSG_FPGADTST:
		Success = wParam;
		Value = 100;
		InvalidateRect(hWnd, NULL, 0);
		break;
	case WM_MYMSG_FPGAATST:
		Success = wParam;
		Value = 100;
		InvalidateRect(hWnd, NULL, 0);
		break;
	case WM_ERASEBKGND:
		//first = 1;
		return 1;
	case WM_PAINT:
		
		hDc = BeginPaint(hWnd, &ps);
		hOldFont = SelectObject(hDc, hFont);
		if(first)
		{
			first = 0;
			hDcWallpaper  = CreateCompatibleDC(hDc);
			
			hDcWorkPlace		 =  CreateCompatibleDC(hDc);
			HBITMAP hBPWorkPlace =  CreateCompatibleBitmap(hDc,640,480);

			//SelectObject(hDc, Pn1);
			hOldWallpaper = SelectObject(hDcWallpaper, hBPWallPaper);
			
			hOldWorkPlace = SelectObject(hDcWorkPlace, hBPWorkPlace);


			FillRect   (hDcWorkPlace, &rcWorkPlace, BrWorkPlace);
			BitBlt     (hDc, 0, 0, 640, 480, hDcWallpaper, 0, 0, SRCCOPY);

			if( !AlphaBlend(hDc, MainRectX, MainRectY, MainRectWidth, MainRectHeight, hDcWorkPlace, MainRectX, MainRectY, MainRectWidth, MainRectHeight,alpha))
			{
				MessageBox(hWnd,L"alpha error",L"error",MB_OK);
			}  

		
			SelectObject(hDcWorkPlace, hOldWorkPlace);

			DeleteObject(hBPWorkPlace);
			
			DeleteDC(hDcWorkPlace);
		}

//hOldPen = SelectObject(hDc,PnYellow);
//		SelectObject(hDc,Br2);
		SetBkMode(hDc,TRANSPARENT);
		SetTextColor(hDc,RGB(0,0,80));
		switch(Step)
		{
		case 0:
			ExtTextOut (hDc, rcWorkPlace.left+textOffsetX, rcWorkPlace.top+textYStart, ETO_OPAQUE, NULL, ListExplorerDict::GetString(DICT_FPGALOADING), wcslen(ListExplorerDict::GetString(DICT_FPGALOADING)), NULL);
			PrgProgrBar->SetPercent(Value);
			if((Value==100) && (Success != -33))
			{ 
				SetTextColor(hDc,RGB(255,255,0));
				if((Value == 100) && (Success == 1))
				{
					ExtTextOut (hDc, PrgProgrBar->GetWidth() + PrgProgrBar->GetX()+ 10, PrgProgrBar->GetY()+3, ETO_OPAQUE, NULL,  ListExplorerDict::GetString(DICT_SUCCESS), wcslen(ListExplorerDict::GetString(DICT_SUCCESS)), NULL);
				}
				else
				{
					SetTextColor(hDc,RGB(210,0,0));
					switch(Success)
					{
					case -1:		
						ExtTextOut (hDc, PrgProgrBar->GetWidth() + PrgProgrBar->GetX()+ 10, PrgProgrBar->GetY()+3, ETO_OPAQUE, NULL, ListExplorerDict::GetString(DICT_FPGANOTRESPOND), wcslen(ListExplorerDict::GetString(DICT_FPGANOTRESPOND)), NULL);
						break;
					case -2:
						ExtTextOut (hDc, PrgProgrBar->GetWidth() + PrgProgrBar->GetX()+ 10, PrgProgrBar->GetY()+3, ETO_OPAQUE, NULL, ListExplorerDict::GetString(DICT_FPGAFILEMISSING), wcslen(ListExplorerDict::GetString(DICT_FPGAFILEMISSING)), NULL);
						break;
					default:
						ExtTextOut (hDc, PrgProgrBar->GetWidth() + PrgProgrBar->GetX()+ 10, PrgProgrBar->GetY()+15, ETO_OPAQUE, NULL, ListExplorerDict::GetString(DICT_FPGAERROR), wcslen(ListExplorerDict::GetString(DICT_FPGAERROR)), NULL);
					}
					ExtTextOut (hDc, PrgProgrBar->GetWidth() + PrgProgrBar->GetX()+ 10, PrgProgrBar->GetY()+3, ETO_OPAQUE, NULL, ListExplorerDict::GetString(DICT_FPGAFAILED), wcslen(ListExplorerDict::GetString(DICT_FPGAFAILED)), NULL);
					Error =1;
				}
				Success = -33;
				Step = 1;
				InvalidateRect(hWnd,0,0);
			}
			break;

		case 1:
			ExtTextOut (hDc, rcWorkPlace.left + textOffsetX, rcWorkPlace.top + textYStart+ textOffsetY, ETO_OPAQUE, NULL,  ListExplorerDict::GetString(DICT_FPGADATABUSTEST), wcslen(ListExplorerDict::GetString(DICT_FPGADATABUSTEST)), NULL);
			PrgFPGATestAddr->SetPercent(Value);

			if( (Value == 100)&&(Success != -33) )
			{
				SetTextColor(hDc,RGB(255,255,0));
				if((Value == 100) && (Success==1))
				{
					ExtTextOut (hDc, PrgFPGATestAddr->GetWidth() + PrgFPGATestAddr->GetX()+ 10, PrgFPGATestAddr->GetY()+3, ETO_OPAQUE, NULL, ListExplorerDict::GetString(DICT_SUCCESS), wcslen(ListExplorerDict::GetString(DICT_SUCCESS)), NULL);
				}
				else
				{
					SetTextColor(hDc,RGB(210,0,0));
					ExtTextOut (hDc, PrgFPGATestAddr->GetWidth() + PrgFPGATestAddr->GetX()+ 10, PrgFPGATestAddr->GetY()+3, ETO_OPAQUE, NULL, ListExplorerDict::GetString(DICT_FPGAFAILED), wcslen(ListExplorerDict::GetString(DICT_FPGAFAILED)), NULL);
					Error = 1;
				}
				Success = -33;
				Step = 2;
				//InvalidateRect(hWnd,0,0);

			}
			//ExtTextOut (hDc, rcWorkPlace.left+10, rcWorkPlace.top+110, ETO_OPAQUE, NULL, L"Testing Address Bus...", wcslen(L"Testing Address Bus..."), NULL);

			break;

		case 2:
			ExtTextOut (hDc, rcWorkPlace.left+textOffsetX, rcWorkPlace.top+textYStart+ textOffsetY*Step, ETO_OPAQUE, NULL, ListExplorerDict::GetString(DICT_FPGAADDRBUSTEST), wcslen(ListExplorerDict::GetString(DICT_FPGAADDRBUSTEST)), NULL);
			
			PrgFPGATestData->SetPercent(Value);

			if((Value==100)&&(Success!=-33))
			{
				SetTextColor(hDc,RGB(255,255,0));
				PrgFPGATestData->SetPercent(100);
				if((Value == 100) && (Success==1))
				{
					ExtTextOut (hDc, PrgFPGATestData->GetWidth() + PrgFPGATestData->GetX()+ 10, PrgFPGATestData->GetY()+3, ETO_OPAQUE, NULL, ListExplorerDict::GetString(DICT_SUCCESS), wcslen(ListExplorerDict::GetString(DICT_SUCCESS)), NULL);
				}
				else
				{
					SetTextColor(hDc,RGB(210,0,0));
					ExtTextOut (hDc, PrgFPGATestData->GetWidth() + PrgFPGATestData->GetX()+ 10, PrgFPGATestData->GetY()+3, ETO_OPAQUE, NULL,ListExplorerDict::GetString(DICT_FPGAFAILED), wcslen(ListExplorerDict::GetString(DICT_FPGAFAILED)), NULL);
					Error =1;
				}
				Step = 3;
			}	
			break;
		}
		if(Step==3)
		{
			
			if ( Error == 0 ) CanExit=1;
			else
			{
				//if( LoadString(g_hInst, IDS_Pres2Continue, WStrTemp, sizeof(WStrTemp)>>1))	
				ExtTextOut (hDc, rcWorkPlace.left+10, rcWorkPlace.bottom - 20, ETO_OPAQUE, NULL, ListExplorerDict::GetString(DICT_PRESSTOCONTINUE), wcslen(ListExplorerDict::GetString(DICT_PRESSTOCONTINUE)), NULL);
			}
		}

		if(NowBlackScreen) 
		{
			BitBlt (hDc, 0, 0, 640, 480, hDcWallpaper, 0, 0, SRCCOPY);
			SelectObject(hDcWallpaper, hOldWallpaper);
			DeleteDC(hDcWallpaper);
		}
		
		SelectObject(hDc, hOldFont);
		EndPaint(hWnd, &ps);
		//return 0;
		break;
	case WM_DESTROY:
		DeleteHandle((HANDLE*)&hBPWallPaper);
		DeleteHandle((HANDLE*)&hFont);
		DeleteHandle((HANDLE*)&hPrgFont);
		DeleteHandle((HANDLE*)&BrWorkPlace);
		//DeleteObject(hBPWallPaper);
		//		DeleteObject(Pn1);
		//		DeleteObject(Br1);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}


DWORD ThreadProc(LPVOID lpParameter)
{
	static char programmed = 0;
	int result;
	//for(result = 0;result<1;result++)
	while(!Initialized) Sleep(5); 
	PrgProgrBar->Show();
	//Sleep(2000);
	if(!programmed)
	{
		result = FPGA_Programing();
		SendMessage(MainW->GetHWnd(), WM_MYMSG_FPGAPRG, result, 0);
		//Step = 1;
		while(Step !=1) Sleep(50);
		//FPGA_Init();
		PrgFPGATestAddr->Show();
		result = FPGA_DBUS_TEST();
		SendMessage(MainW->GetHWnd(), WM_MYMSG_FPGADTST, result, 0);
		while(Step !=2) Sleep(50);	
		PrgFPGATestData->Show();
		result = FPGA_ABUS_TEST();
		SendMessage(MainW->GetHWnd(), WM_MYMSG_FPGAATST, result, 0);
		programmed = 1;

		while(!CanExit) Sleep(5);
		Sleep(1000);
		PrgProgrBar->Hide();
		PrgFPGATestAddr->Hide();
		PrgFPGATestData->Hide();
		NowBlackScreen = true;
		InvalidateRect(MainW->GetHWnd(),0,0);
		Sleep(50);
		SendMessage(MainW->GetHWnd(),WM_DESTROY,0,0);
	}
	return 1;
}

VOID CALLBACK TimerProc(
						HWND hWnd, // handle of window for timer messages
						UINT uMsg,    // WM_TIMER message
						UINT idEvent, // timer identifier
						DWORD dwTime  // current system time
						)
{
	static char programmed = 1;
	if(Value<100)
		Value++;
	InvalidateRect(hWnd, NULL, 0);

	if(!programmed)
	{
		FPGA_Programing();
		programmed = 1;
	}
	return;
}


int FPGA_Programing()
{
#ifdef WINCE
int result = 0;
//#define FPGA_DCLKPIO     GPIO59 //rdm35
#define FPGA_DCLKPIO		GPIO27 
#define FPGA_DATA0PIO		GPIO2
#define FPGA_NSTATUSPIO		GPIO54
#define FPGA_NCONFIGPIO     GPIO5
#define FPGA_CDONEPIO       GPIO6
	bool IsError = false;
	int counter = 0;
	volatile u32 FPGAPins[]={5, FPGA_DCLKPIO, FPGA_DATA0PIO,FPGA_NSTATUSPIO, FPGA_NCONFIGPIO, FPGA_CDONEPIO};
	const u8 FPGAPins_Af1[]={5,0,0,0,0,0};
	const u8 FPGAPins_PP1[]={5,3,3,3,3,3};
	GPIOConfig(FPGAPins,FPGAPins_Af1,FPGAPins_PP1, IsError);   
	GPIO_REGS *GPIO = (GPIO_REGS*) malloc (sizeof(GPIO_REGS));
	FPGA_Prg_Init();

	FPGA_DCLK(0);
	FPGA_NCONFIG(0);
	while(FPGA_GETNSTATUS()==1);
	FPGA_NCONFIG(1);

	while((FPGA_GETNSTATUS()==0)&&(counter++<1000));
	if(counter>=1000)
		return -1;

	//	cout<<"NStatus "<<((int)FPGA_GETNSTATUS())<<endl;
	//	cout<<"CDONE "<<((int)FPGA_GETCDONE())<<endl;
	HANDLE hFile;
	hFile = CreateFile (FPGAFileCMD, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);                 

	if (hFile == INVALID_HANDLE_VALUE)	
	{
		return -2;
	}
	else
	{
		//cout<<"Ok"<<endl;
		DWORD nr = 32000;
//		DWORD count;
		DWORD size = 0;
		DWORD estimatedSize = 0;
		u8 *buf = new u8[nr];//(u8*) malloc (sizeof(u8)*nr);
		DWORD dwBytesRead= 0;
//		int i,j;
		int Value1;
		SetFilePointer (hFile, 0, NULL, FILE_BEGIN); 
		while(1)
		{
			ReadFile (hFile, buf, nr, &dwBytesRead, NULL);
			estimatedSize +=dwBytesRead;
			if(nr!=dwBytesRead) 
				break;
		}
		//TimeCheck();
		
		SetFilePointer (hFile, 0, NULL, FILE_BEGIN) ; 
		size = 0;
		while(size < estimatedSize)
		{	 
			ReadFile (hFile, buf, nr, &dwBytesRead, NULL);
			size+=dwBytesRead;
			
			if(FPGA_Write_Prg_Buf((unsigned short*)buf, (unsigned short)dwBytesRead))
			{
				Value1 = (100 * size) / estimatedSize;
				if(Value1 >= 100) 
				{
					Value = 99;
					result  = true;
				}
				else
				{
					Value = Value1;
				}
				InvalidateRect(MainW->GetHWnd(), NULL, 0);
				if(dwBytesRead != nr) 
				{
					size = estimatedSize;
				};
			}
			else
			{
				size = estimatedSize; 
				break;
			}
		}
		delete []buf;
		CloseHandle (hFile);
		return result;
	}
	
#else
	for(int i=0;i<100;i++)
	{
		Sleep(20);
		Value = i;
		InvalidateRect(MainW->GetHWnd(), NULL, 0);
	}
	return 1;
#endif
	return -3;
}


int FPGA_DBUS_TEST()
{
	char Ok = 1;
#ifdef WINCE
	unsigned short temp;
	int Value1;
	for(int i=0;i<15;i++)
	{

		Value1 =(int) (100.0 * (float) i/14);
		if(i==14) Value1 = 99;
		if(Value1 >= 100) 
		{
			Value = 99;
		}
		else
		{
			Value = Value1;
		}
		InvalidateRect(MainW->GetHWnd(),0,0);
		FPGA_Write(61 ,(unsigned short)1<<i);
		FPGA_Write(127,(unsigned short)~1<<i);
		temp = FPGA_Read(61);
		if(temp != 1<<i)
			Ok ++;
	}
#else
	for(int i=0;i<100;i++)
	{
		Sleep(20);
		Value = i;
		InvalidateRect(MainW->GetHWnd(), NULL, 0);
	}
	return 1;

#endif
	return Ok;
}

int FPGA_ABUS_TEST()
{
	char Ok = 1;

#ifdef WINCE
	unsigned short temp;
	int Value1;
	for(int i=0;i<7;i++)
	{
		Value1 =(int) (100.0 * (float) i/6);
		if(i==6) Value1 = 99;
		if(Value1 >= 100)
		{
          Value = 99;
		}
		else
		{
		  Value = Value1;
		}
		InvalidateRect(MainW->GetHWnd(),0,0);
		FPGA_Read( 1 << i );  
		temp = FPGA_Read(62);
		if(temp != 1<<i) 
			Ok ++;
	}
#else
	for(int i=0;i<100;i++)
	{
		Sleep(20);
		Value = i;
		InvalidateRect(MainW->GetHWnd(), NULL, 0);
	}
	return 1;
#endif

	return Ok;
}
