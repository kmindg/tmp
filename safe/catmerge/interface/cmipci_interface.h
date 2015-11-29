//
//	Copyright (C) EMC Corporation 1998-2005,2010
//	All rights reserved.
//	Licensed material -- property of EMC Corporation
//

//++
// File Name:
//      cmipci_interface.h
//
// Contents:
//      Definitions of IOCTL codes and data structures exported by CMIpci.
//
//--

#ifndef CMIPCI_INTERFACE_H
#define CMIPCI_INTERFACE_H

//
// Test Harness Code Enable Control
//

#define CMIPCI_TEST 1
// When defined, turns on the test harness code in CMIpci




//
// Miscellaneous definitions
//

#define CMIPCI_TIMESTAMP_SCALING_FACTOR                 (10000000)
    // Since the value returned from EmcpalQuerySystemTime represents the number
    // of 100-nanosecond ticks since the start of the year 1601, we must
    // multiply by 10 million to change the granularity to 1 second.

//
// IOCTL control codes
//
#define CMIPCI_CTL_CODE(code, method)		\
	(EMCPAL_IOCTL_CTL_CODE(EMCPAL_IOCTL_FILE_DEVICE_UNKNOWN, 0xD80 + (code), (method), EMCPAL_IOCTL_FILE_ANY_ACCESS))

// Codes 1-3 were used by the cmiscd test harness (didn't want to re-use the offsets)

#define IOCTL_CMIPCI_GASKET_REGISTRATION	CMIPCI_CTL_CODE(4, EMCPAL_IOCTL_METHOD_BUFFERED)
#define IOCTL_CMIPCI_TEST_TRANSPORT_STATS	CMIPCI_CTL_CODE(5, EMCPAL_IOCTL_METHOD_BUFFERED)
#define IOCTL_CMIPCI_TEST_BUS_RESET		CMIPCI_CTL_CODE(6, EMCPAL_IOCTL_METHOD_BUFFERED)
#define IOCTL_CMIPCI_TEST_BAD_XFER		CMIPCI_CTL_CODE(7, EMCPAL_IOCTL_METHOD_BUFFERED)
#define IOCTL_CMIPCI_TEST_SEND_KVTRACE		CMIPCI_CTL_CODE(8, EMCPAL_IOCTL_METHOD_BUFFERED)
#define IOCTL_CMIPCI_TEST_SET_TRACE_MASK        CMIPCI_CTL_CODE(9, EMCPAL_IOCTL_METHOD_BUFFERED)

//
// Structures used by the IOCTL codes
//
//                       
#pragma pack(4)

typedef struct _CMIPCI_TEST_TRANSPORT_STATS
{
	LONG			IODelta;
	LONG			ByteDelta;
	LONG			TimeDelta;
	LONG			OutstandingIOs;
	LONG			PendingIOs;

	LONG			XmitErrorCount;
	LONG			XmitErrorNotificationCount;
	LONG			DMAErrorCount;
	LONG			DMAInsufficientResourcesErrorCount;
	LONG			DMAInvalidParameterErrorCount;
	LONG			InitiatorRingPeerClientErrorCount;
	LONG			TargetRingPeerClientErrorCount;
	LONG			RingCallbackPreemptCount;

	LONG			TestHarnessConnectionState;
	LONG			CommandsRemaining;

} CMIPCI_TEST_TRANSPORT_STATS, *PCMIPCI_TEST_TRANSPORT_STATS;

#define CMIPCI_TEST_TRANSPORT_STATS_SIZE	sizeof(CMIPCI_TEST_TRANSPORT_STATS)


typedef struct _CMIPCI_PEER_INFO
{
	BOOLEAN			DataXferReady;
//	UCHAR			PathId;
//	UCHAR			TargetId;
//	UCHAR			Lun;

} CMIPCI_PEER_INFO, *PCMIPCI_PEER_INFO;

#pragma pack ()

// Define these doorbell values for CmiPci. [Moved from cmipci_global.h]
// AR393917 PING_REQUEST generates PING_RESPONSE from ISR, does not queue DPC.
#define RING_EVENT_DOORBELL         0
#define PING_REQUEST_DOORBELL       1
#define PING_RESPONSE_DOORBELL      2

#endif // CMIPCI_INTERFACE_H

//
// END OF cmipci_interface.h
//
