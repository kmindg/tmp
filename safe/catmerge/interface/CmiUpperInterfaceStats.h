#ifndef __CMI_UPPER_INTERFACE_STATS_
#define __CMI_UPPER_INTERFACE_STATS_

//***************************************************************************
// Copyright (C) EMC Corporation 1998-2005
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      CmiUpperInterfaceStats.h
//
// Contents:
//      definitions used by IOCTL_CMI_GET_STATISTICS (which is defined in 
//      CmiUpperINterface.h). That IOCTL expects a input buffer of type
//      CMI_SP_ID and and output buffer of type CMI_STATISTICS_IOCTL_INFO
//
// Revision History:
//  20-Oct-01   DHarvey     Split out of CmiUpperINterface.h, split up VC stats.
//--


//
// Header files
//
#include "CmiUpperInterface.h"



//++
// Type:
//      CMI_BUNDLING_STATISTICS
//
// Description:  per-Virtual Circuit statistics.
//
//--
struct _CMI_VC_STATISTICS
{
    // How many messages were sent by VC.
	ULONGLONG			TotalMessagesSent; 
	ULONGLONG			TotalMessagesReceived;

    // How many bytes transferred into ring buffers? (CLient
    // data plus CMID headers).
	ULONGLONG			TotalFloatBytesSent;     // excludes retries
	ULONGLONG			TotalFloatBytesReceived; // excludes retries

    // How many bytes of client directed DMA per VC.
	ULONGLONG			TotalFixedBytesSent;     // excludes retries
	ULONGLONG			TotalFixedBytesReceived; // includes retries

    // How many times did each VC stop because of flow control?
	ULONGLONG			TimesFlowControlled;

    // How many times did each VC attempt to force a bundle to send a buffer credit?
    ULONGLONG			ForcedCreditSends;

    // Sum of message response times, in us
    ULONGLONG			CumulativeMessageResponseTime;
    ULONGLONG			CumulativeMessageWaitToSendTime;
    ULONGLONG			CumulativeMessageSendAckTime;

#   ifdef __cplusplus
    VOID                DebugDisplay(ULONG index) const;
#   endif

};
typedef _CMI_VC_STATISTICS CMI_VC_STATISTICS, * PCMI_VC_STATISTICS;

# ifdef __cplusplus
inline CMI_VC_STATISTICS operator -(const CMI_VC_STATISTICS & a, const CMI_VC_STATISTICS & b)
{
    _CMI_VC_STATISTICS result;
    
    result.TotalMessagesSent = a.TotalMessagesSent - b.TotalMessagesSent;
    result.TotalMessagesReceived = a.TotalMessagesReceived - b.TotalMessagesReceived;
    result.TotalFloatBytesSent = a.TotalFloatBytesSent - b.TotalFloatBytesSent;
    result.TotalFloatBytesReceived = a.TotalFloatBytesReceived - b.TotalFloatBytesReceived;
    result.TotalFixedBytesSent = a.TotalFixedBytesSent - b.TotalFixedBytesSent;
    result.TotalFixedBytesReceived = a.TotalFixedBytesReceived - b.TotalFixedBytesReceived;
    result.TimesFlowControlled = a.TimesFlowControlled - b.TimesFlowControlled;
    result.ForcedCreditSends = a.ForcedCreditSends - b.ForcedCreditSends;
    result.CumulativeMessageWaitToSendTime = a.CumulativeMessageWaitToSendTime - b.CumulativeMessageWaitToSendTime;
    result.CumulativeMessageResponseTime = a.CumulativeMessageResponseTime - b.CumulativeMessageResponseTime;
    result.CumulativeMessageSendAckTime = a.CumulativeMessageSendAckTime - b.CumulativeMessageSendAckTime;

    return result;
    
}
# endif 



//++
// Type:
//      CMI_BUNDLING_STATISTICS
//
// Description: Keeps per-SP statistics.
//
//--
struct _CMI_BUNDLING_STATISTICS
{
    // Stats about bundle sends.
    ULONGLONG			messagesSent;      // total client messages.
    ULONGLONG			bundlesSent;       // total bundles created
    ULONGLONG			bytesSent;         // total client bytes
    ULONGLONG			bundlesCompressed;  // total number of bundles compressed into one dataphase.
    ULONGLONG           fixedDataBundlesCompressed; // subset of above carring some fixed data
    ULONGLONG           bundlesAckedOutOfOrder; // Sender got ACK out of order
    ULONGLONG           bundlesResentSlowAck;   // Tried a different path due to slow response
    ULONGLONG           bundlesAckedByReceivedBundle; // received bundle beat SCSI ACK
    ULONGLONG           bundleResponseTimeCumulativeAckedByRecievedBundle; // for each bundlesAckedByReceivedBundle
    ULONGLONG           bundleResponseTimeCumulativeNormalAck;      // for each bundlesSent

	// Indexed by the number of bundles already in-flight when a bundle
    // is formed, and then incremented.  Gives a view of how many bundles
    // were in transit at one time. If the index is out of range, the
    // highest element is incremented.
    ULONGLONG			bundlesInFlight[CMI_HISTOGRAM_BUCKETS]; 

	// Indexed by the number of messages in the bundle when a bundle
    // is formed, and then incremented. If the index is out of range, the
    // highest element is incremented.
	ULONGLONG			bundlesByNumMessages[CMI_HISTOGRAM_BUCKETS];

    // Stats on the reason a bundle composition filled. Sum these, and
    // subtract from totalBundles to get bundles composed that didn't
    // fill.
	ULONGLONG			bundlesFilledSenderSgl; // The SGL filled up
	ULONGLONG			bundlesFilledRAD;       // The bundle header filled.
	ULONGLONG			bundlesFilledMessages;  // Out of message descriptors.
	ULONGLONG			bundlesFilledControlOps;// The control header filled.
	ULONGLONG			bundlesFilledMaxBytes;  // The total bytes in the bundle was too large.

    // How many times were bundles sent on each link (sum minus
    // totalBundles == # retries).
	ULONGLONG			bundleTransmissions[CMI_GATE_COUNT/*Per link*/];
	ULONGLONG			bundleFailures[CMI_GATE_COUNT/*Per link*/];

    // Stats about bundle reception
    ULONGLONG           bundlesReceived;                // excludes retries
    ULONGLONG           bytesReceived;                  // excludes retries
    ULONGLONG			messagesReceived;               // total client messages.
    ULONGLONG           bundlesReceivedOutOfOrder;      // excludes retries
    ULONGLONG           bundlesDuplicatesReceived;      // Were ACKed.
    ULONGLONG           bundlesReceivedOutOfWindow;     // Were NAKed.
    ULONGLONG           extraDataPhaseOnReceive;        // Partial transfers on receive
;

    // Per-VC stats.
	CMI_VC_STATISTICS   vcStats[CMI_MAX_CONDUITS]; 


#   ifdef __cplusplus
    VOID                DebugDisplay() const;
#   endif

};

typedef _CMI_BUNDLING_STATISTICS CMI_BUNDLING_STATISTICS, *PCMI_BUNDLING_STATISTICS;

# ifdef __cplusplus
inline CMI_BUNDLING_STATISTICS operator -(const CMI_BUNDLING_STATISTICS a, const CMI_BUNDLING_STATISTICS b)
{
    CMI_BUNDLING_STATISTICS result;
    
    ULONG       i;

    result.messagesSent = a.messagesSent - b.messagesSent;
    result.bundlesSent = a.bundlesSent - b.bundlesSent;
    result.bytesSent = a.bytesSent - b.bytesSent;
    result.bundlesCompressed = a.bundlesCompressed - b.bundlesCompressed;
    result.fixedDataBundlesCompressed = a.fixedDataBundlesCompressed - b.fixedDataBundlesCompressed;
    result.bundlesAckedOutOfOrder = a.bundlesAckedOutOfOrder - b.bundlesAckedOutOfOrder;
    result.bundlesResentSlowAck = a.bundlesResentSlowAck - b.bundlesResentSlowAck;
    result.bundlesAckedByReceivedBundle = a.bundlesAckedByReceivedBundle - b.bundlesAckedByReceivedBundle;
    result.bundleResponseTimeCumulativeAckedByRecievedBundle = a.bundleResponseTimeCumulativeAckedByRecievedBundle - b.bundleResponseTimeCumulativeAckedByRecievedBundle;
    result.bundleResponseTimeCumulativeNormalAck = a.bundleResponseTimeCumulativeNormalAck - b.bundleResponseTimeCumulativeNormalAck;

    for(i = 0; i < sizeof(result.bundlesInFlight)/sizeof(result.bundlesInFlight[0]); i ++) {
        result.bundlesInFlight[i] = a.bundlesInFlight[i] - b.bundlesInFlight[i];
    }
    for(i = 0; i < sizeof(result.bundlesByNumMessages)/sizeof(result.bundlesByNumMessages[0]); i ++) {
        result.bundlesByNumMessages[i] = a.bundlesByNumMessages[i] - b.bundlesByNumMessages[i];
    }

    result.bundlesFilledSenderSgl = a.bundlesFilledSenderSgl - b.bundlesFilledSenderSgl;
    result.bundlesFilledRAD = a.bundlesFilledRAD - b.bundlesFilledRAD;
    result.bundlesFilledMessages = a.bundlesFilledMessages - b.bundlesFilledMessages;
    result.bundlesFilledControlOps = a.bundlesFilledControlOps - b.bundlesFilledControlOps;
    result.bundlesFilledMaxBytes = a.bundlesFilledMaxBytes - b.bundlesFilledMaxBytes;

    for(i = 0; i < sizeof(result.bundleTransmissions)/sizeof(result.bundleTransmissions[0]); i ++) {
        result.bundleTransmissions[i] = a.bundleTransmissions[i] - b.bundleTransmissions[i];
    }
    for(i = 0; i < sizeof(result.bundleFailures)/sizeof(result.bundleFailures[0]); i ++) {
        result.bundleFailures[i] = a.bundleFailures[i] - b.bundleFailures[i];
    }
    result.bundlesReceived = a.bundlesReceived - b.bundlesReceived;
    result.bytesReceived = a.bytesReceived - b.bytesReceived; 
    result.messagesReceived = a.messagesReceived - b.messagesReceived;
    result.bundlesReceivedOutOfOrder = a.bundlesReceivedOutOfOrder - b.bundlesReceivedOutOfOrder;
    result.bundlesDuplicatesReceived = a.bundlesDuplicatesReceived - b.bundlesDuplicatesReceived;
    result.bundlesReceivedOutOfWindow = a.bundlesReceivedOutOfWindow - b.bundlesReceivedOutOfWindow;
    result.extraDataPhaseOnReceive = a.extraDataPhaseOnReceive - b.extraDataPhaseOnReceive;
    
    for(i = 0; i < sizeof(result.vcStats)/sizeof(result.vcStats[0]); i ++) {
        result.vcStats[i] = a.vcStats[i] - b.vcStats[i];
    }
    return result;
    
}

# endif 

//++
// Type:
//      CMI_STATISTICS_IOCTL_INFO
//
// Description: Structure used by:
//          IOCTL_CMI_GET_STATISTICS
//
// Members:
//    spId      - Specifies the SP the stats are for.
//    bundleStats - copy of the per SP statistics.
//--
typedef struct _CMI_STATISTICS_IOCTL_INFO
{
    CMI_SP_ID                spId;          // INPUT/OUTPUT
    CMI_BUNDLING_STATISTICS  bundleStats;   // OUTPUT

} CMI_STATISTICS_IOCTL_INFO, *PCMI_STATISTICS_IOCTL_INFO;
//.End


#endif 
