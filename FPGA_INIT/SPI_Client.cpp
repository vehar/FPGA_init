// SPI_Client.cpp : Defines the entry point for the console application.
//
//#include "stdafx.h"
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h> 
#include <string.h>
#include "locale.h"
#include <winbase.h>
#include <WinIoCtl.h>
#include <initguid.h>
//#include "spi.h"
//#include "omap35xx_mcspi.h"


#include <winuser.h>   
#include <winuserm.h>
//#include <pm.h>

//#include "UniDriver.h"
//#include "driverHelper.h"

#include "gpio.h"
//#include "omap35xx_gpio.h" 

//#include <commctrl.h>

#include "GpioDriver.h"
#include "FPGA/FPGA.h"
#include "ExtBusFunc.h"

//#define DEBUG 
#define DCLK_PIN 130

//void ExtBusCs3Init();
//void SetCs3Value(DWORD val, DWORD addr);
//DWORD GetCs3Value(DWORD addr);
//DWORD ReadReg(RWRegData_t* readData);
//DWORD WriteReg(RWRegData_t* writeData);

int                SetupExtBus();
DWORD		      err;

unsigned char   Value;
bool            Initialized = false;

int     Check_StatusConf (void);
int     FPGA_Programing(void);
int     FPGA_Blast(void);
void    ProgramByte( int one_byte );

int     FPGA_DBUS_TEST(void);
int     FPGA_ABUS_TEST(void);


char FPGA_rbf_Filename []={"YAFFS_PART1/rdm.rbf"};
    
    
int _tmain(int argc, _TCHAR* argv[])
{
GPIODriver *gpio=new GPIODriver();

//hCons = GetStdHandle(STD_OUTPUT_HANDLE);
//SetConsoleTextAttribute(hCons, $0A);
//system("color 0A");
printf("FPGA_init start_2-4-15\n");

/*FpgaProgramming_START*/
//=========================================================
/*
#define FPGA_DCLKPIO     130
#define FPGA_DATA0PIO    135
#define FPGA_NSTATUSPIO  134
#define FPGA_CDONEPIO    133
#define FPGA_NCONFIGPIO  131
*/
/*
gpio->SetGpioMode(DCLK_PIN,         GPIO_DIR_OUTPUT);
gpio->SetGpioMode(FPGA_DATA0PIO,    GPIO_DIR_OUTPUT);
gpio->SetGpioMode(FPGA_NSTATUSPIO,  GPIO_DIR_INPUT);
gpio->SetGpioMode(FPGA_CDONEPIO,    GPIO_DIR_INPUT);
gpio->SetGpioMode(FPGA_NCONFIGPIO,  GPIO_DIR_OUTPUT);
*/
FPGA_Init();
FPGA_Prg_Init();


	static char programmed = 0;
	int result;

		result = FPGA_Blast(); 

		//SendMessage(MainW->GetHWnd(), WM_MYMSG_FPGAPRG, result, 0);
		//PrgFPGATestAddr->Show();
		 printf("FPGA_Programing resultn %i\n",result);
        
		result = FPGA_DBUS_TEST();
		//SendMessage(MainW->GetHWnd(), WM_MYMSG_FPGAD TST, result, 0);
		//PrgFPGATestData->Show();
		printf("FPGA_DBUS_TEST resultn %i\n",result);
        
		result = FPGA_ABUS_TEST();
		//SendMessage(MainW->GetHWnd(), WM_MYMSG_FPGAATST, result, 0);
		printf("FPGA_ABUS_TEST resultn %i\n",result);
		programmed = 1;

/*FpgaProgramming_END*/
//=========================================================


Sleep(10000);
return 0; 
}


#define INIT_CYCLE			50		/* extra DCLK cycles while initialization is in progress */
#define RECONF_COUNT_MAX	3		/* #auto-reconfiguration when error found */
#define	CHECK_EVERY_X_BYTE  10240	/* check NSTATUS pin for error for every X bytes */
									/* DO NOT put '0' */
#define CLOCK_X_CYCLE		2		/* clock another 'x' cycles during user mode (not necessary, 
									   for debug purpose only) */

int FPGA_Blast()
{
	bool        IsError = false;
	long int    counter = 0;
	int	        program_done = 0;		            	/* programming process (configuration and initialization) done = 1*/
	int	        configuration_count = 0;	/* # reprogramming after error */	
	int			confdone_ok = 1;			/* CONF_DONE pin. '1' - error */
	int			nstatus = 0;				/* NSTATUS pin. '0' - error */
    long int	i = 0;						/* standard counter variable */
	int			one_byte = 0;				/* the byte to program */

	FILE * ptrFile = fopen(FPGA_rbf_Filename, "rb" );// Open programming file as READ and in BINARY 

	  if (ptrFile == NULL)
	  { 
		  printf("fopen error\n");
		  printf("FPGA config file '%s' not found\n" ,FPGA_rbf_Filename);
		  return -1;
	  }
	/* Get file size */
	int seek_position = fseek(ptrFile, 0, SEEK_END );
		if (seek_position != NULL) 
        { 
            printf("fseek error = %i \n",seek_position );
            return -2;
        }
	int	file_size	= ftell( ptrFile );
	fprintf( stdout, "Info: Programming file size: %ld\n", file_size );//181945


//PINS_INIT---------------------------------------------------------------------------
	FPGA_Prg_Init();
//PINS_INIT---------------------------------------------------------------------------

//------------------------------------------------------------------------
fprintf( stdout, "-- Start configuration process...\n" );
		
	while ( !program_done && (configuration_count < RECONF_COUNT_MAX) )// Start configuration 
	{	
		fseek(ptrFile, 0, SEEK_SET );// Reset file pointer and parallel port registers 

		// Drive a transition of 0 to 1 to NCONFIG to indicate start of configuration 
	FPGA_DCLK(0);
	FPGA_NCONFIG(0);
    while((FPGA_GETNSTATUS()==1)&&(counter++<100000));
	if(counter>=1000)
    {
          printf("No STATUS 1->0\n"); 
          return -1;
	}
    
	FPGA_NCONFIG(1);
    while((FPGA_GETNSTATUS()==0)&&(counter++<100000));
	if(counter>=1000)
    {
          printf("No STATUS 0->1\n");  
          return -1;
	}    

    Check_StatusConf();

		// Loop through every single byte 
		for ( i = 0; i < file_size; i++ )
		{
			one_byte = fgetc(ptrFile);

			// Progaram a byte 
			ProgramByte( one_byte );

			// Check for error through NSTATUS for every 10KB programmed and the last byte 
			if ( !(i % CHECK_EVERY_X_BYTE) || (i == file_size - 1) )
			{
				nstatus = FPGA_GETNSTATUS();

				if ( !nstatus )
				{
					printf("Conf error = %d\n",configuration_count-1);

					program_done = 0;
					break;
				} 
				else 
					program_done = 1;
			}
			//For debug
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
            int percent = file_size/320;
            if((i>=percent) && ((i%percent) == 0))
                {
                    printf("*");
                }     
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%           
		}//end for
		//после конфигурирования пропадает изображение на єкране!
	
  configuration_count++;
		if ( !program_done )
		{			
			printf("Attempt- %i",configuration_count);
			continue;
		}

		//Configuration end 
		// Check CONF_DONE that indicates end of configuration 
		confdone_ok = FPGA_GETCDONE();

//#warning strange behavior - confdone_ok must be 0 if Ok
//похоже, что FPGA_GETCDONE не работает или пин не сконфигурирован!

		if ( !confdone_ok ) //Error
		{
			printf("Error: Configuration done CONF_DONE is %i\n", confdone_ok  ); 
			program_done = 0;
			printf("configuration_count is %i\n",configuration_count);	
			if ( configuration_count == RECONF_COUNT_MAX )
				break;
		}
	
		// if contain error during configuration, restart configuration 
		if ( !program_done )
			continue;

		 program_done = 1; 

		//Start initialization 
		// Clock another extra DCLK cycles while initialization is in progress
		 //  through internal oscillator or driving clock cycles into CLKUSR pin 
		// These extra DCLK cycles do not initialize the device into USER MODE 
		// It is not required to drive extra DCLK cycles at the end of
		//   configuration													   
		// The purpose of driving extra DCLK cycles here is to insert some delay
		 //  while waiting for the initialization of the device to complete before
		 //  checking the CONFDONE and NSTATUS signals at the end of whole 
		 //  configuration cycle 											       
		for ( i = 0; i < INIT_CYCLE; i++ )
		{
			FPGA_DCLK(0);
			FPGA_DCLK(1);
		}
		// Initialization end 
          
		nstatus = FPGA_GETNSTATUS();
		confdone_ok = FPGA_GETCDONE();

		if ( !nstatus || confdone_ok )
		{
			printf("Error: Initialization finish but contains error: NSTATUS is %s and CONF_DONE is %s. Exiting...", (nstatus?"HIGH":"LOW"), (confdone_ok?"LOW":"HIGH") );
			program_done = 0; 
			configuration_count = RECONF_COUNT_MAX; // No reconfiguration 
		}
	}

	//Sleep(100);//for gebug
	// Add another 'x' clock cycles while the device is in user mode.
	// This is not necessary and optional. Only used for debugging purposes 
	if ( CLOCK_X_CYCLE > 0 )
	{
		printf( "Info: Clock another %d cycles in while device is in user mode...\n", CLOCK_X_CYCLE );
		for ( i = 0; i < CLOCK_X_CYCLE; i++ )
		{
			FPGA_DCLK(0);
			FPGA_DCLK(1);
		}
	}

	if ( !program_done )
	{
		printf( "\nError: Configuration not successful! Error encountered...\n" );
	}

	fprintf( stdout, "\nInfo: Configuration successful!\n" );
//------------------------------------------------------------------------

return 0;
}

int Check_StatusConf (void)
{
int		nstatus = FPGA_GETNSTATUS();
int		confdone = FPGA_GETCDONE();

		if ( !nstatus || confdone )
		{
        printf("NStatus = %d - normal is 1", nstatus);//normal state must be 1
        printf("CDONE = %d - normal is 0 \n",confdone);//normal state must be 0
            return 1;
		}        
return 0;
}

//Descriptions:	Dump to parallel port bit by bit, from least significant
//				bit to most significant bit. A positive edge clock pulse	
//				is also asserted.
void ProgramByte( int one_byte )
{
	int	bit = 0;
	int i = 0;

	//write from LSb to MSb  
	for ( i = 0; i < 8; i++ )
	{
		bit = one_byte >> i;
		bit = bit & 0x1;
		
	//Dump to DATA0 and insert a positive edge pulse at the same time  
	  FPGA_DATA0(bit);
	  FPGA_DCLK(1);
	  FPGA_DCLK(0);
	}
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
		//InvalidateRect(MainW->GetHWnd(),0,0);
		FPGA_Write(17 ,(unsigned short)1<<i);
		FPGA_Write(127,(unsigned short)~1<<i);
		temp = FPGA_Read(17);
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
//		InvalidateRect(MainW->GetHWnd(),0,0);
		FPGA_Read( 1 << i );  
		temp = FPGA_Read(18);
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

//------------------------------------------------------------------------------

int FPGA_Programing()
{
#ifdef WINCE
int result = 0;

#define FPGA_DCLKPIO     130
#define FPGA_DATA0PIO    135
#define FPGA_NSTATUSPIO  134
#define FPGA_CDONEPIO    133
#define FPGA_NCONFIGPIO  131

	bool IsError = false;
	int counter = 0;

	FPGA_Prg_Init();

	FPGA_DCLK(0);
	FPGA_NCONFIG(0);
/*
while((FPGA_GETNSTATUS()==1)&&(counter++<1000));
	if(counter>=1000)
	{
          printf("No FPGA_GETNSTATUS 1\n");
          return -1;
	}
*/
	FPGA_NCONFIG(1);

 /*while((FPGA_GETNSTATUS()==0)&&(counter++<1000));
	if(counter>=1000)
	{
          printf("No FPGA_GETNSTATUS 0\n");
          return -1;
	}
	*/

printf("NStatus = %d ",((int)FPGA_GETNSTATUS()));
printf("CDONE = %d\n",((int)FPGA_GETCDONE()));
	HANDLE hFile; 
	hFile = CreateFile (TEXT("YAFFS_PART1/RDM-35_FPGA.rbf"), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL); 
	//hFile = CreateFile (FPGAFileCMD, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);                 

	if (hFile == INVALID_HANDLE_VALUE)	
	{
		printf("RDM-35_FPGA.rbf not found\n");
		return -2;
	}
	else
	{
		//cout<<"Ok"<<endl;
		printf("Ok\n");
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
				//InvalidateRect(MainW->GetHWnd(), NULL, 0);
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