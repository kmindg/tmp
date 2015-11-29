#ifndef __VirtualFlareVolumeStatisticsDebugDisplay__
#define __VirtualFlareVolumeStatisticsDebugDisplay__

//***************************************************************************
// Copyright (C) EMC Corporation 2015
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      VirtualFlareVolumeStatisticsDebugDisplay.h
//
// Contents:
//
//  Separate header to display the VirtualFlare driver and VirtualFlare Volume statistics 
//
//  Display function can be used according to the environment in which data will be
//  displyed
//
// Revision History:
//-- 11/02/2015     Liu Yousheng     Initial Version

#ifdef I_AM_DEBUG_EXTENSION
#include "csx_ext_dbgext.h"
#define DISPLAY csx_dbg_ext_print 
#elif defined(_WDBGEXTS_) && defined(KDEXT_64BIT)
#define DISPLAY(...) nt_dprintf(__VA_ARGS__)
// ENHANCE: Need to have mut_printf for mut tests but not sure how to do it.
// as mut_printf will take firt argument as level.
//#elif defined(MUT_API) 
//#define DISPLAY mut_printf
#else
#define DISPLAY(...) fprintf(fileHandle, __VA_ARGS__)
#endif 

void VirtualFlareVolumeStatistics::DebugDisplayHeading(FILE* fileHandle)
{
    DISPLAY(" Timestamp, ");

    for(int i = 0; i < IO_SIZE_IDX_NBR; i++) {
         DISPLAY("Read %s, ", GetLengthType(i));
    }
    for(int i = 0; i < IO_SIZE_IDX_NBR; i++) {
            DISPLAY("Write %s, ", GetLengthType(i));
    }
    for (int idx = SAW_FSW; idx < SAW_NBR; idx++) {
            DISPLAY("%s, ", GetStripeAlignWriteType(idx));
    }
    for (int idx = IDX_STATUS_SUCCESS; idx < IDX_STATUS_NBR; idx++) {
            DISPLAY("%s", GetStatusType(idx));
    }
    DISPLAY("Throttle Writes");

    DISPLAY("\n");
    return;
}


void VirtualFlareVolumeStatistics::DebugDisplay(ULONG flags, FILE* fileHandle)
{
    if (flags == 1) { // CSV

        DISPLAY("%02d:%02d:%02d.%02d, ", 
                (UCHAR)(PerformanceTimestamp >> TOD_POLL_HOUR_SHIFT),
                (UCHAR)(PerformanceTimestamp >> TOD_POLL_MINUTE_SHIFT),
                (UCHAR)(PerformanceTimestamp >> TOD_POLL_SECOND_SHIFT),
                (UCHAR)(PerformanceTimestamp >> TOD_POLL_HUNDREDTH_SHIFT));

        for(int i = 0; i < IO_SIZE_IDX_NBR; i++) {
             DISPLAY("%" statisticsFormat", ", mStatsBlock.mReadHistogram[i]);
        }
        for(int i = 0; i < IO_SIZE_IDX_NBR; i++) {
                DISPLAY("%" statisticsFormat", ", mStatsBlock.mWriteHistogram[i]);
        }
        for (int idx = SAW_FSW; idx < SAW_NBR; idx++) {
                DISPLAY("%" statisticsFormat", ", mStatsBlock.mSAWTypeCount[idx]);
        }
        for (int idx = IDX_STATUS_SUCCESS; idx < IDX_STATUS_NBR; idx++) {
                DISPLAY("%" statisticsFormat", ", mStatsBlock.mStatusReturnedCount[idx]);
        }
        DISPLAY("%" statisticsFormat"\n", mStatsBlock.mThrottleWrCount);

        return;
    }

    DISPLAY("Timestamp ");
    DISPLAY("%02d:%02d:%02d.%02d\n", 
            (UCHAR)(PerformanceTimestamp >> TOD_POLL_HOUR_SHIFT),
            (UCHAR)(PerformanceTimestamp >> TOD_POLL_MINUTE_SHIFT),
            (UCHAR)(PerformanceTimestamp >> TOD_POLL_SECOND_SHIFT),
            (UCHAR)(PerformanceTimestamp >> TOD_POLL_HUNDREDTH_SHIFT));
    DISPLAY("\n\tReadHistogram\n");
    for(int i = 0; i < IO_SIZE_IDX_NBR; i++) {
        if (mStatsBlock.mReadHistogram[i])
            DISPLAY("\t%s\t         : %" statisticsFormat"\n", GetLengthType(i), mStatsBlock.mReadHistogram[i]);
    }
    DISPLAY("\n\tWriteHistogram\n");
    for(int i = 0; i < IO_SIZE_IDX_NBR; i++) {
        if (mStatsBlock.mWriteHistogram[i])
            DISPLAY("\t%s\t         : %" statisticsFormat"\n", GetLengthType(i), mStatsBlock.mWriteHistogram[i]);
    }

    DISPLAY("\n\tstripe align write\n");
    for (int idx = SAW_FSW; idx < SAW_NBR; idx++) {
        if (mStatsBlock.mSAWTypeCount[idx])
            DISPLAY("\t%s\t         : %" statisticsFormat"\n", GetStripeAlignWriteType(idx), mStatsBlock.mSAWTypeCount[idx]);
    }
    
    DISPLAY("\n\tStatus Count\n");
    for (int idx = IDX_STATUS_SUCCESS; idx < IDX_STATUS_NBR; idx++) {
        if (mStatsBlock.mStatusReturnedCount[idx])
            DISPLAY("\t%s\t         : %" statisticsFormat"\n", GetStatusType(idx), mStatsBlock.mStatusReturnedCount[idx]);
    }

    DISPLAY("\n\tThrottle Write Count : %" statisticsFormat"\n", mStatsBlock.mThrottleWrCount);

    DISPLAY("\n");
}


#endif  // __VirtualFlareVolumeStatisticsDebugDisplay__
