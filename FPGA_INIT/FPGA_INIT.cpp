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

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winbase.h>
#include <WinIoCtl.h>
#include <initguid.h>
#include <winuser.h>   
#include <winuserm.h>
#include <time.h>
#include <mmsystem.h> //Include file for Multimedia API's 


#include "gpio.h"
#include "GpioDriver.h"
#include "FPGA/FPGA.h"
#include "ExtBusFunc.h"



//#define DEBUG 

#define INIT_CYCLE			50		/* extra DCLK cycles while initialization is in progress */
#define RECONF_COUNT_MAX	3		/* auto-reconfiguration when error found */
#define	CHECK_EVERY_X_BYTE  1024	/* check NSTATUS pin for error for every X bytes */
									/* DO NOT put '0' */
#define CLOCK_X_CYCLE		0		/* clock another 'x' cycles during user mode (not necessary, 
									   for debug purpose only) */

//#define NO_CONFIG				//skip fpga configure for debug
 #define VISUALISE_PROGRESS		//visualise configuration process with stars *
 #define CRUTCH				    //после конфигурирования пропадает изображение на єкране - костыль решает

  
  char FPGA_rbf_Filename []={"YAFFS_PART1/rdm.rbf"};// path to configuration file 

unsigned char   Value;

int     SetupExtBus(void);
int     Check_StatusConf (void);
int     FPGA_Programing(void);
int     FPGA_Blast(void);
void    ProgramByte(int one_byte );

int     FPGA_DBUS_TEST(void);
int     FPGA_ABUS_TEST(void);
   
    
int _tmain(int argc, _TCHAR* argv[])
{
    GPIODriver *gpio=new GPIODriver();

	int BLAST_result = 0;
	int DBUS_TEST_result = 0;
	int ABUS_TEST_result = 0;

	int error = 0;
 
printf(" FPGA_INIT created on <%s, %s> started\n",__DATE__, __TIME__);

/*FpgaProgramming_START*/
//=========================================================
		BLAST_result = FPGA_Blast(); 
        printf("FPGA_Programing %s ", (BLAST_result?"ERR = ":"OK\n")); 
		if(BLAST_result){printf("%d\n",BLAST_result); error++;}

		DBUS_TEST_result = FPGA_DBUS_TEST();
		printf("FPGA_DBUS_TEST %s ", (DBUS_TEST_result?"ERR = ":"OK\n"));
		if(DBUS_TEST_result){printf("%d\n",DBUS_TEST_result); error++;}
        
		ABUS_TEST_result = FPGA_ABUS_TEST();
		printf("FPGA_ABUS_TEST %s ", (ABUS_TEST_result?"ERR = ":"OK\n"));
		if(ABUS_TEST_result){printf("%d\n",ABUS_TEST_result); error++;}

//=========================================================
/*FpgaProgramming_END*/

printf("Exiting...\n\n");
if(error)
{
	PlaySound (TEXT("YAFFS_PART1\\Error.WAV"), NULL, SND_SYNC);
	Sleep(10000);
}
else 
{
	PlaySound (TEXT("YAFFS_PART1\\Sucsess.WAV"), NULL, SND_SYNC);
	Sleep(3000);
}
return 0; 
}

//===================================================================================================================
//===================================================================================================================
int FPGA_Blast()
{
	long int    counter = 0;
	int	        program_done = 0;		    /* programming process (configuration and initialization) done = 1*/
	int	        configuration_count = 0;	/* # reprogramming after error */	
	int			confdone_ok = 1;			/* CONF_DONE pin. '1' - error */
	int			nstatus = 0;				/* NSTATUS pin. '0' - error */
	int			one_byte = 0;				/* the byte to program */

	FILE * ptrFile = fopen(FPGA_rbf_Filename, "rb" );// Open programming file as READ and in BINARY 

	  if (ptrFile == NULL)
	  { 
		  printf("fopen error\n");
		  printf("FPGA config file '%s' not found\n" ,FPGA_rbf_Filename);
		  return 1;
	  }
	/* Get file size */
	int seek_position = fseek(ptrFile, 0, SEEK_END );
		if (seek_position != NULL) 
        { 
            printf("fseek error = %i \n",seek_position );
            return 2;
        }
	int	file_size	= ftell( ptrFile );
	fprintf( stdout, "Info: Programming file size: %4.3f kB\n", (float)file_size/1000 );//181945

    ExtBusCs1Init();//init ExtBus for TESTS functions
    FPGA_Prg_Init();//init GPIO
    
    fprintf( stdout, "- Start configuration process...\n" );

//------------------------------------------------------------------------
#ifndef NO_CONFIG		
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
          return 3;
	}
    
	FPGA_NCONFIG(1);
    while((FPGA_GETNSTATUS()==0)&&(counter++<100000));
	if(counter>=1000)
    {
          printf("No STATUS 0->1\n");  
          return 4;
	}    

    Check_StatusConf();
	
		for (long int i = 0; i < file_size; i++ )// Loop through every single byte 
		{
			one_byte = fgetc(ptrFile);	 
			ProgramByte( one_byte );// Progaram a byte

			// Check for error through NSTATUS for every 10KB programmed and the last byte 
			if ( !(i % CHECK_EVERY_X_BYTE) || (i == file_size - 1) )
			{
				nstatus = FPGA_GETNSTATUS();
				if ( !nstatus )
				{
					printf("\nConf error\n");
					program_done = 0;
					break;
				} 
				else 
					program_done = 1;
			}
#ifdef VISUALISE_PROGRESS
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
            int percent = file_size/320;
            if((i >= percent) && ((i % percent) == 0))
                {
                    printf("*");
                }     
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%  
#endif        
		}//end for
		//после конфигурирования пропадает изображение на єкране!

FPGA_DATA0(0);//after configuration must be in 0
FPGA_DCLK(0);//after configuration must be in 0


  configuration_count++;
		if ( !program_done )
		{			
			printf("Attempt: %i from %i\n",configuration_count, RECONF_COUNT_MAX);
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
		for (long int i = 0; i < INIT_CYCLE; i++ )
		{
			FPGA_DCLK(0);
			FPGA_DCLK(1);
		}
		// Initialization end 
          
		nstatus = FPGA_GETNSTATUS();
		confdone_ok = FPGA_GETCDONE();

		
        if(Check_StatusConf() == 1)
		{
			printf("Error: Initialization finish but contains error\n");
			program_done = 0; 
			configuration_count = RECONF_COUNT_MAX; // No reconfiguration 
		}
	}

	// Add another 'x' clock cycles while the device is in user mode.
	// This is not necessary and optional. Only used for debugging purposes 
	if ( CLOCK_X_CYCLE > 0 )
	{
		printf( "Info: Clock another %d cycles in while device is in user mode...\n", CLOCK_X_CYCLE );
		for (int i = 0; i < CLOCK_X_CYCLE; i++ )
		{
			FPGA_DCLK(0);
			FPGA_DCLK(1);
		}
	}
    
  #ifdef CRUTCH
    FPGA_Write(96 ,0); //AScanDrawMode
    FPGA_Write(97 ,0); //Если не произвести эту запись (AScanEn) - изображение на экране не появится!
  #endif

#endif //NO_CONFIG end    
	fprintf( stdout, "Info: Configuration successful!\n" );
//------------------------------------------------------------------------

return 0;
}

int Check_StatusConf (void)
{
int		nstatus = FPGA_GETNSTATUS();
int		confdone = FPGA_GETCDONE();

		if ( !nstatus || confdone )
		{
			 printf("NStatus = %d - normal is 1\n", nstatus);//normal state must be 1
			 printf("CDONE   = %d - normal is 0\n",confdone);//normal state must be 0
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
	int Ok = 0;
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
		if(temp != 1<<i) Ok |= 1<<i;
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
	int Ok = 0;

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
		if(temp != 1<<i) Ok |= 1<<i;
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
