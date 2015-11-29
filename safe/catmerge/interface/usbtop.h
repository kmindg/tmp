#ifndef __USBTOP_KERNEL_INTERFACE__
#define __USBTOP_KERNEL_INTERFACE__
//***************************************************************************
// Copyright (C) EMC Corporation 1989-2007
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//		usbtop.h
//
// Contents:
//		The USBTOP device driver's kernel space interface.
//
// Revision History:
//--

#include "k10ntddk.h"
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT

#ifdef _USBTOP_
#define USBTOPAPI CSX_MOD_EXPORT
#else
#define USBTOPAPI CSX_MOD_IMPORT
#endif

// Exported Interface Functions 

USBTOPAPI EMCPAL_STATUS UsbTopGetSerialPortForSlot(UINT32 IoSlotNumber, csx_pchar_t * ReturnString);

// Return Values
#define USBTOP_SUCCESS                          0  //Goodness
#define USBTOP_INVALID_ARGUMENTS                1  //Self-explanitory
#define USBTOP_INVALID_SLOT_ERROR               2  //The slot number passed in is invalid for the platform
#define USBTOP_GENERIC_USB_ERROR                3  //Hit an error traversing USB, may indicate HW not there
#define USBTOP_GENERIC_REGISTRY_ERROR           4  //Information isn't in the registry yet, may be worth a retry
#define USBTOP_CONVERTER_DRIVER_ERROR           5  //Unused for now


#endif //__USBTOP_KERNEL_INTERFACE__
