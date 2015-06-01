/*!--------------------------------------------------------------------------
 * \file	hal_parameters.h
 * \brief	Declarations of HAL string parameter items.
 *
 *
 * File: hal_parameters.h
 *
 * Purpose:
 *
 *	LoCE's HAL code is designed to be very extensible. This includes
 *	the run-time resolution of serveral configuration options. One of
 *	the ways in which the HAL resolves its many options is through the
 *	parsing of a parameter string. This string is passed to the HAL from
 *	the bootloader via a section of reserved memory. This file contains
 *	the declaration of functions and objects used in the parsing of the
 *	HAL parameter string.
 *
 * Copyright (c) 2002-2005 Logic Product Development
 *
 * NOTICE:
 *	This file contains source code, ideas, techniques, and information
 *	(the Information) which are Proprietary and Confidential Information
 *	of Logic Product Development, Inc.  This Information may not be used
 *	by or disclosed to any third party except under written license, and
 *	shall be subject to the limitations prescribed under license.
 *
 *-------------------------------------------------------------------------*/
#ifndef _HAL_PARAMETERS_H_
#define _HAL_PARAMETERS_H_


/* The maximum length an individual token may be. */
#define HAL_STR_MAX_TOKEN_LEN           (32)

/* This version string should be the first token in the hal parameter
 * string. It will be checked before the string is parsed. If the versions
 * do not match, a warning will output to the user. This should help manage
 * conflicts between differing versions of LoCE and which ever bootloader
 * loaded it. */
#define HAL_STR_VERSION                 "0_1"


/* The following is a list of the valid tokens which may be included in
 * the hal parameter string. */

/* The token delimeter is used to mark the beginning and end of each
 * and every individual token. Note, the two definitions are used to
 * make the routines parsing the HAL string easier. */
#define HAL_STR_DELIMITER               ":"
#define HAL_STR_DELIMITER_              ':'

/* The following tokens are used to define which HAL-level drivers are used
 * by the base HAL code. Inside each HAL-level driver interface structure is
 * a member called "driver_name." Each "driver_name" member is initialized
 * to one of the macros below. By defining the strings in this manner, only
 * this file needs to be kept up-to-date.
 */

/* Alternative way to start KITL. */
#define HAL_STR_KITL                    "kitl"
#define HAL_STR_KITL_IP_ADDR            "ip_addr"

/* Denotes the HAL-level Ethernet debug driver to use. */
#define HAL_STR_DBG_ENET                "dbg_enet"
#define HAL_STR_DBG_ENET_91C111         "91C111"
#define HAL_STR_DBG_ENET_LAN911X        "LAN911X"
#define HAL_STR_DBG_ENET_NULL           "null"

/* Denotes the HAL-level Ethernet debug driver base address to use. */
#define HAL_STR_DBG_ENET_BASE           "dbg_enet_base"
#define HAL_STR_DBG_ENET_IRQ            "dbg_enet_irq"

/* Denotes the HAL-level debug serial driver to use. */
#define HAL_STR_DBG_SERIAL              "dbg_serial"
#define HAL_STR_DBG_SERIAL_16C550       "16C550"
#define HAL_STR_DBG_SERIAL_7727_SCIF    "7727_scif"
#define HAL_STR_DBG_SERIAL_7760_SCIF    "7760_scif"
#define HAL_STR_DBG_SERIAL_7750_SCIF    "7750_scif"
#define HAL_STR_DBG_SERIAL_A400         "A400_UART"
#define HAL_STR_DBG_SERIAL_A404         "A404_UART"
#define HAL_STR_DBG_SERIAL_PXA270       "PXA270_FFUART"
#define HAL_STR_DBG_SERIAL_PXA320       "PXA320_FFUART"
#define HAL_STR_DBG_SERIAL_IMX31        "IMX31_UART"
#define HAL_STR_DBG_SERIAL_NULL         "null"

/* Denotes the HAL-level real time clock driver to use. */
#define HAL_STR_RTC                     "rtc"
#define HAL_STR_RTC_7727_INT            "rtc_7727_int"
#define HAL_STR_RTC_7760_INT            "rtc_7760_int"
#define HAL_STR_RTC_7750_INT            "rtc_7750_int"
#define HAL_STR_RTC_TIBQ4802            "rtc_tibq4802"
#define HAL_STR_RTC_A400_INT            "rtc_a400_int"
#define HAL_STR_RTC_A404_INT            "rtc_a404_int"
#define HAL_STR_RTC_PXA270              "rtc_pxa270"
#define HAL_STR_RTC_PXA320              "rtc_pxa320"
#define HAL_STR_RTC_IMX31               "rtc_imx31"
#define HAL_STR_RTC_IMX31_PMIC          "rtc_imx31_pmic"
#define HAL_STR_RTC_NULL                "rtc_null"

/* Denotes the string used to force HIVE registry clear */
#define HAL_STR_CLEAN_SYS_HIVE          "clean_syshive"
#define HAL_STR_CLEAN_USR_HIVE          "clean_usrhive"

/* Denotes the HAL-level real time clock driver base address to use. */
#define HAL_STR_RTC_TIBQ4802_BASE       "rtc_tibq4802_base"

/* Denotes the HAL-level watch dog timer driver to use. */
#define HAL_STR_WDT                     "wdt"
#define HAL_STR_WDT_7727_INT            "wdt_7727_int"
#define HAL_STR_WDT_7760_INT            "wdt_7760_int"
#define HAL_STR_WDT_7750_INT            "wdt_7750_int"
#define HAL_STR_WDT_MEM_MAP             "wdt_mem_map"
#define HAL_STR_WDT_NULL                "wdt_null"

/* Denotes the HAL-level real time clock driver base address to use. */
#define HAL_STR_WDT_MEM_MAP_BASE        "wdt_mem_map_base"

/* Denotes whether or not to share the debug Ethernet connection
 * with applications. */
#define HAL_STR_SHARE_ETH               "share_eth"

/* Denotes the size of this Card Engine's SDRAM */
#define HAL_STR_MBYTES_OF_SDRAM         "mbytes_sdram"

/* Denotes the value to set the kernel variable dwDefaultThreadQuantum to. */
#define HAL_STR_THREAD_QUANTUM          "quantum"

#ifdef _ARM_
#if CARD_ENGINE_IMX31_10
/* Denotes what available coprocessor(s) to activate */
#define HAL_STR_COPROCESSORS            "coproc"
#endif
#endif // _ARM_

/* Denotes whether the debug leds should be on or off */
#define HAL_STR_DEBUG_LEDS              "dbg_leds"

/* Forward declarations of public HAL parameter functions. */

unsigned char *hal_param_get_value(unsigned char *key, unsigned int *len);
void hal_param_resolve_globals(void);

//#ifdef _ARM_
unsigned long
oem_addr_table_map_phys_to_virt(unsigned long phys_addr);
//#endif /* _ARM_ */

#endif /* closes #ifndef _HAL_PARAMETERS_H_ */
