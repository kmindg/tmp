/*******************************************************************************
 * Copyright (C) EMC Corporation, 1998 - 2010
 * All rights reserved.
 * Licensed material - Property of EMC Corporation
 ******************************************************************************/

/*******************************************************************************
 * RedirectorVolumeStatistics.h
 *
 * This header file defines the exported IoRedirector Volume Statistics
 *
 ******************************************************************************/

#if !defined (RedirectorVolumeStatistics_H)
#define RedirectorVolumeStatistics_H

# include "BasicVolumeDriver/BasicVolumeParams.h"

// Define IoRedirector specific returned BVD Statistics
class RedirectorVolumeStatistics : public BasicVolumeStatistics {

public:
    // The total number of Read/Writes received from drivers above that were serviced on
    // this SP since boot.
    ULONGLONG              totalLocalIOServicedLocally;

    // The total number of Read/Writes received from drivers above that were forwarded to
    // the peer SP since boot. 
	ULONGLONG              totalLocalIORedirected;

    // The total number of Read/Writes sent to the drivers below due to peer I/O since SP boot
    ULONGLONG              totalPeerIOServiced;

    // The difference between the number of peer IRPs received and the number of local IRPs received
    LONG                   remoteToLocalIoDelta;
};

#endif /* !defined (RedirectorVolumeStatistics_H) */
