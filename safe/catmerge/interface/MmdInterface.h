#ifndef MMDINTERFACE_H
#define MMDINTERFACE_H 0x00000001

/*****************************************************************************
 * Copyright (C) EMC Corp. 2003
 * All rights reserved.
 * Licensed material - Property of EMC Corporation.
 *****************************************************************************/

/*****************************************************************************
 *  MmdInterface.h
 *****************************************************************************
 *
 * DESCRIPTION:
 *		This file describes the interface to the MicroMemory Driver. The
 *		MicroMemory PCI adapter is a memory device with battery backup
 *		to provide a persistent storage area during power loss.
 *
 * NOTES:
 *
 * HISTORY:
 *      5/13/2003	AHutchinson		Created
 *
 *****************************************************************************/

/********************************
 * INCLUDE FILES
 ********************************/
#include "generics.h"
#include "flare_sgl.h"


/********************************
 * LITERAL DEFINITIONS
 ********************************/

//
// User Mode includes additional code within the driver to enable an
// application to issue io transfer ioctls to the driver using paged
// memory from user space. This should only be used for testing purposes.
//
#define MMD_USER_MODE


//
// Device Name Definitions
//
#define MMD_BASE_DEVICE_NAME	   		"Mmd"
#define MMD_NT_DEVICE_NAME				"\\Device\\" MMD_BASE_DEVICE_NAME
#define MMD_WIN32_DEVICE_NAME			"\\DosDevices\\" MMD_BASE_DEVICE_NAME

//
// Driver Interface Revision
//
#define MMD_DRIVER_REVISION		1

//
// IO Control Codes
//
#define MMD_CTL_CODE(code, method)		\
	(EMCPAL_IOCTL_CTL_CODE(EMCPAL_IOCTL_FILE_DEVICE_UNKNOWN, 0x800 + (code), (method), EMCPAL_IOCTL_FILE_ANY_ACCESS))

#define IOCTL_MMD_GET_HARDWARE_INFO		MMD_CTL_CODE(0x1, EMCPAL_IOCTL_METHOD_NEITHER)
#define IOCTL_MMD_SET_CACHE_LED			MMD_CTL_CODE(0x2, EMCPAL_IOCTL_METHOD_NEITHER)
#define IOCTL_MMD_SET_FAULT_LED			MMD_CTL_CODE(0x3, EMCPAL_IOCTL_METHOD_NEITHER)
#define IOCTL_MMD_GET_BATTERY_STATUS	MMD_CTL_CODE(0x4, EMCPAL_IOCTL_METHOD_NEITHER)
#define IOCTL_MMD_GET_ERROR_INFO		MMD_CTL_CODE(0x5, EMCPAL_IOCTL_METHOD_NEITHER)
#define IOCTL_MMD_DISABLE_BATTERY		MMD_CTL_CODE(0x6, EMCPAL_IOCTL_METHOD_NEITHER)
#define IOCTL_MMD_RESET_CARD			MMD_CTL_CODE(0x7, EMCPAL_IOCTL_METHOD_BUFFERED)
#define IOCTL_MMD_ABORT_IO				MMD_CTL_CODE(0x8, EMCPAL_IOCTL_METHOD_NEITHER)
#define IOCTL_MMD_SYNC_TRANSFER			MMD_CTL_CODE(0x9, EMCPAL_IOCTL_METHOD_NEITHER)
#define IOCTL_MMD_UMODE_SYNC_TRANSFER	MMD_CTL_CODE(0xA, EMCPAL_IOCTL_METHOD_NEITHER)
#define IOCTL_MMD_ASYNC_TRANSFER		MMD_CTL_CODE(0xB, EMCPAL_IOCTL_METHOD_NEITHER)
#define IOCTL_MMD_UMODE_ASYNC_TRANSFER	MMD_CTL_CODE(0xC, EMCPAL_IOCTL_METHOD_OUT_DIRECT)
#define IOCTL_MMD_DIAGNOSTIC_WRITE		MMD_CTL_CODE(0xD, EMCPAL_IOCTL_METHOD_BUFFERED)


/********************************
 * MACROS
 ********************************/

/********************************
 * STRUCTURES DEFINITIONS
 ********************************/


/*
 * Ioctl Name:
 *		IOCTL_MMD_SET_WRITE_CACHE_LED
 *
 * Description:
 *      This synchronous ioctl allows a client to set the state of
 *		the adapters cache led (Blue).
 */
typedef enum
{
	MMD_LED_OFF,
	MMD_LED_ON,
	MMD_LED_FLASH_3_5_HZ,
	MMD_LED_FLASH_7_HZ
} MMD_LED_STATE;

typedef struct _MMD_SET_CACHE_LED_IOCTL
{
	MMD_LED_STATE 	ledState;

} MMD_SET_CACHE_LED_IOCTL, *PMMD_SET_CACHE_LED_IOCTL;

/*
 * Ioctl Name:
 *		IOCTL_MMD_SET_FAULT_LED
 *
 * Description:
 *      This synchronous ioctl allows a client to set the state of
 *		the adapters fault led (yellow).
 */
typedef struct _MMD_SET_FAULT_LED_IOCTL
{
	MMD_LED_STATE 	ledState;

} MMD_SET_FAULT_LED_IOCTL, *PMMD_SET_FAULT_LED_IOCTL;


/*
 * Ioctl Name:
 *		IOCTL_MMD_GET_BATTERY_STATUS
 *
 * Description:
 *      This synchronous ioctl allows a client to get the current
 *		battery status of each battery.
 */
typedef enum
{
	MMD_BATTERY_OK,
	MMD_BATTERY_FAILED,
	MMD_BATTERY_DISABLED,
	MMD_BATTERY_DISABLED_AND_FAILED
} MMD_BATTERY_STATUS;

typedef struct _MMD_GET_BATTERY_STATUS_IOCTL
{
	MMD_BATTERY_STATUS 	batteryStatus_1;
	MMD_BATTERY_STATUS 	batteryStatus_2;
	UINT_16E			batteryCharge_1;
	UINT_16E			batteryCharge_2;
} MMD_GET_BATTERY_STATUS_IOCTL, *PMMD_GET_BATTERY_STATUS_IOCTL;



/*
 * Ioctl Name:
 *		IOCTL_MMD_GET_ERROR_INFO
 *
 * Description:
 *      This synchronous ioctl retreives error information from
 *		the adapters CSR registers.
 */
#define MMD_ERROR_LOG_COUNT 2

typedef enum
{
	MMD_EDC_NONE_DEFAULT,
	MMD_EDC_NONE,
	MMD_EDC_DETECT_ERRORS,
	MMD_EDC_DETECT_AND_CORRECT_ERRORS
} MMD_EDC_MODE;

typedef struct _MMD_GET_ERROR_INFO_IOCTL
{
	MMD_EDC_MODE	edcMode;
	UINT_32E		errorCount;
	UINT_64			errorDataLog[MMD_ERROR_LOG_COUNT];
	UINT_64			errorInfoLog[MMD_ERROR_LOG_COUNT];

} MMD_GET_ERROR_INFO_IOCTL, *PMMD_GET_ERROR_INFO_IOCTL;



/*
 * Ioctl Name:
 *		IOCTL_MMD_GET_HARDWARE_INFO
 *
 * Description:
 *      This synchronous ioctl retreives general information about
 *		the adapter.
 */
#define MMD_MAX_CARD_NAME_LEN 	64

typedef struct _MMD_GET_HARDWARE_INFO_IOCTL
{
	UINT_32E						driverRevision;
	UCHAR 							cardName[MMD_MAX_CARD_NAME_LEN+1];
	UINT_32E						cardRevision;
	UINT_32E						cardPhysicalSize;
	UINT_32E						cardVirtualSize;
	BOOLEAN							passedPost;
	BOOLEAN							memValid;
	EMCPAL_LARGE_INTEGER					lastInitTime;

    struct {
    	MMD_LED_STATE				powerLed;
    	MMD_LED_STATE				faultLed;
    	MMD_LED_STATE				cacheLed;
    } ledStatus;

    MMD_GET_BATTERY_STATUS_IOCTL 	batteryStatus;

} MMD_GET_HARDWARE_INFO_IOCTL, *PMMD_GET_HARDWARE_INFO_IOCTL;



/*
 * Ioctl Name:
 *		IOCTL_MMD_ASYNC_TRANSFER
 *		IOCTL_MMD_SYNC_TRANSFER
 *
 * Description:
 *      Thess synchronous/asynchronous ioctls are used to
 *		tranfer data to or from the adapters RAM. The value
 *		of transferType determines whether the operation is
 *		a read/write.
 */
typedef enum {
	MMD_IO_READ,
	MMD_IO_WRITE
} MMD_IO_TYPE;

typedef enum {
	MMD_IO_STATUS_SUCCESS,
	MMD_IO_STATUS_ECC_ERROR,
	MMD_IO_STATUS_HW_ERROR,
} MMD_IO_STATUS;

typedef struct _MMD_IO_TRANSFER_IOCTL
{
	MMD_IO_TYPE		transferType;
	SGL				*pHostSgl;
	SGL				*pAdapSgl;
	MMD_IO_STATUS	status;
	PVOID			transferHandle;

} MMD_IO_TRANSFER_IOCTL, *PMMD_IO_TRANSFER_IOCTL;



/*
 * Ioctl Name:
 *		IOCTL_MMD_RESET_CARD
 *
 * Description:
 *      This asynchronous ioctl will abort all pending async
 *		transfer requests and start to reset the adapter.
 *		The adapters RAM will be initialized with zeros.
 */
typedef enum {
	MMD_RST_STATUS_SUCCESS,
	MMD_RST_STATUS_HW_ERR
} MMD_RESET_STATUS;

typedef struct _MMD_RESET_CARD_IOCTL
{
	MMD_RESET_STATUS	status;

} MMD_RESET_CARD_IOCTL, *PMMD_RESET_CARD_IOCTL;



/*
 * Ioctl Name:
 *		IOCTL_MMD_DIAGNOSTIC_WRITE
 *
 * Description:
 *      This asynchronous ioctl allows a client to write an 64 bit
 *		value to any 64 bit aligned address on the adapter and
 *		insert either a single or multi bit error. The actual bits
 *		to switch are specified by the BitsToFlip fields.
 */

typedef struct _MMD_DIAGNOSTIC_WRITE_IOCTL
{
	UINT_32E	address;
	UINT_64		data;
	UINT_64		dataBitsToFlip;
   	UINT_32E	chkBitsToFlip;

} MMD_DIAGNOSTIC_WRITE_IOCTL, *PMMD_DIAGNOSTIC_WRITE_IOCTL;


#endif // #ifndef MMDINTERFACE_H
