#ifndef __CMI_COMMON_INTERFACE__
#define __CMI_COMMON_INTERFACE__

//***************************************************************************
// Copyright (C) Data General Corporation 1989-1998
// All rights reserved.
// Licensed material -- property of Data General Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************
# include "flare_sgl.h"

//++
// File Name:
//      CmiCommonInterface.h
//
// Contents:
//      Definitions of the exported IOCTL codes and data structures
//		that are shared by both the upper-level interfaces of the CMI
//		driver (defined in CmiUpperInterface.h) and the lower-level
//		interfaces (defined in CmiTransportClassInterface.h).
//
// Revision History:
//	19-Feb-98   Goudreau	Created.
//	29-Mar-99	Goudreau	Split SGL type into physical and virtual flavors.
//
//--

//
// Exported basic data types
//

//++
// Type:
//		CMI_PHYSICAL_SG_ELEMENT
//
// Description:
//		An element in a Physical Scatter/Gather list as used by CMI.
//		A Scatter/Gather List itself is just an array of SGEs, terminated
//		by an entry whose <length> field is 0.
//
//		If we ever move to true 64-bit support, the type of the <address>
//		field should change to PHYSICAL_ADDRESS, and the type of the <length>
//		field should probably change to ULONGLONG.
//--
#pragma pack(4)
typedef struct _CMI_PHYSICAL_SG_ELEMENT
{
	SGL_LENGTH		length;
	SGL_PADDR		PhysAddress;

}	CMI_PHYSICAL_SG_ELEMENT, *PCMI_PHYSICAL_SG_ELEMENT, *CMI_PHYSICAL_SGL;
#pragma pack()
//.End

//++
// Type:
//      CMI_VERSION
//
// Description:
//      Distinguishes between CMI Versions when necessary for legacy support.
typedef UCHAR CMI_VERSION;

# define CMI_VERSION_CMIPD  1
# define CMI_VERSION_CMID   2
# define CMI_VERSION_CMID_UNIFIED  3


// The maximum number of CMI ports we'll open in the CMIxxx drivers
#define CMI_MAXIMUM_NUMBER_OF_CMI_PORTS		10


#endif // __CMI_COMMON_INTERFACE__
