//
//	Copyright (C) Data General Corporation 1998
//	All rights reserved.
//	Licensed material -- property of Data General Corporation
//

//++
// File Name:
//      cmiscd_interface.h
//
// Contents:
//      Definitions of IOCTL codes and data structures exported by CMIscd.
//
//--

#ifndef CMISCD_INTERFACE_H
#define CMISCD_INTERFACE_H

#include "EmcPAL_Ioctl.h"

//
// Miscellaneous definitions
//
#define CMISCD_TIMESTAMP_SCALING_FACTOR                 (10000000)
    // Since the value returned from EmcpalQuerySystemTime represents the number
    // of 100-nanosecond ticks since the start of the year 1601, we must
    // multiply by 10 million to change the granularity to 1 second.
#define CMISCD_NUM_BYTES_PER_MB                         (1048576)
    // A megabyte (MB) is 2 to the 20th power bytes.

//
// IOCTL control codes
//
#define CMISCD_CTL_CODE(code, method)		\
	(EMCPAL_IOCTL_CTL_CODE(EMCPAL_IOCTL_FILE_DEVICE_UNKNOWN, 0xD00 + (code), (method), EMCPAL_IOCTL_FILE_ANY_ACCESS))


// Codes 1-3 were used by the test harness, since moved to another file in services/interface.

// Reread registry information IOCTL for rebootless enablers.
#define IOCTL_CMISCD_REREAD_REGISTRY	CMISCD_CTL_CODE(4, EMCPAL_IOCTL_METHOD_BUFFERED)

//
// Structures used by the IOCTL codes
//
//                       
#pragma pack(4)

typedef struct _CMISCD_TEST_TRANSPORT_STATS
{
    LONG						IODelta;
    LONG						ByteDelta;
	LONG						TimeDelta;
	LONG						OutstandingIOs;

} CMISCD_TEST_TRANSPORT_STATS, *PCMISCD_TEST_TRANSPORT_STATS;

#define CMISCD_TEST_TRANSPORT_STATS_SIZE	sizeof(CMISCD_TEST_TRANSPORT_STATS)

#pragma pack ()


#endif // CMISCD_INTERFACE_H

//
// END OF cmiscd_interface.h
//
