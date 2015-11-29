#ifndef __VirtualFlareVolumeStatistics_h__
#define __VirtualFlareVolumeStatistics_h__

//***************************************************************************
// Copyright (C) EMC Corporation 2015
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      VirtualFlareDeviceStatistics.h
//
// Contents:
//      Defines the VirtualFlareDeviceStatistics class. 
//
// Revision History:
//-- 10/09/2015     Liu Yousheng     Initial Version

/* Time of day shifts */
#define TOD_POLL_HOUR_SHIFT       24
#define TOD_POLL_MINUTE_SHIFT     16
#define TOD_POLL_SECOND_SHIFT     8
#define TOD_POLL_HUNDREDTH_SHIFT  0

// All counters should be 64 bit
#define statisticsFormat "llu"

// io size to count
typedef enum {
    IO_SIZE_IDX_LT8K,  // Less than 8K
    IO_SIZE_IDX_EQ8K,  // Equal to 8K
    IO_SIZE_IDX_LE16K, // Less than or equal to 16K
    IO_SIZE_IDX_LE64K, // Less than or equal to 64K
    IO_SIZE_IDX_LE256K,// Less than or equal to 256K
    IO_SIZE_IDX_LE1M,  // Less than or equal to 1M
    IO_SIZE_IDX_MAX,   // Over flow

    IO_SIZE_IDX_NBR,
} IDS_HIST_SIZE_IDX;

// return status be counted
typedef enum {
    IDX_STATUS_SUCCESS,
    IDX_STATUS_QUOTA_EXCEEDED,
    IDX_STATUS_ALERTED,

    IDX_STATUS_OTHER,
    IDX_STATUS_NBR,
} IDX_STATUS_RETURNED;

// type of stripe align write(SAW)
// Offset: Stripe Aligned        Length: Equal to Stripe size        => FSW(Full Stripe Write)
// Offset: Stripe Aligned        Length: Equal to Stripe size * N    => MSW(Multi full Stripe Write)
// Offset: Stripe Aligned        Length: Less than Stripe size       => LSW(Less Stripe Write)
// Offset: NOT Stripe Aligned    Length: Less than Stripe size       => LSW(Less Stripe Write)
// Offset: Stripe Aligned        Length: Greater than Stripe size and not Stripe Aligned          => FSI(Full Stripe Included)
// Offset: NOT Stripe Aligned    Length: Greater than Stripe size and Full Stripe is included     => FSI(Full Stripe Included)
// Offset: NOT Stripe Aligned    Length: Equal to Stripe size        => NSI(No Stripe Included)
// Offset: NOT Stripe Aligned    Length: Greater than Stripe size and NO Full Stripe is included  => NSI(No Stripe Included)
typedef enum {
    SAW_FSW, // Full Stripe Write
    SAW_MSW, // Multiple Full Stripe Write
    SAW_LSW, // Less Stripe size Write
    SAW_FSI, // Full Stripe Included
    SAW_NSI, // No Stripe Included

    SAW_NBR
} STRIPE_ALIGN_WRITE_TYPE;

typedef struct _VirtualFlareVolumeStatisticsStruct {
    UINT_64 mWriteHistogram[IO_SIZE_IDX_NBR]; /* Write Histogram in different size */
    UINT_64 mReadHistogram[IO_SIZE_IDX_NBR]; /* Read Histogram in different size */
    UINT_64 mStatusReturnedCount[IDX_STATUS_NBR]; /* count of return status */
    UINT_64 mSAWTypeCount[SAW_NBR]; /* count of SAW types */
    UINT_64 mThrottleWrCount; /* write request count */
} VirtualFlareVolumeStatisticsStruct, *pVirtualFlareVolumeStatisticsStruct;

// Statistics about VirtualFlareVolume per proxy that can be summed to get
// statistics for the volume.
class VirtualFlareVolumeProxyStatistics {
public:
    friend class VirtualFlareVolumeStatistics;

    VirtualFlareVolumeProxyStatistics() {
        ReInitialize();
    }

    ~VirtualFlareVolumeProxyStatistics() {};

    // ReInitialize the statistics strustcture
    void ReInitialize()  {
        Reset();
    }

    // Reset the statistics strustcture
    void Reset() {
        mStatsBlock.mThrottleWrCount = 0;

        for(int i = 0; i < IO_SIZE_IDX_NBR; i++) {
            mStatsBlock.mReadHistogram[i] = 0;
            mStatsBlock.mWriteHistogram[i] = 0;
        }

        for(int i = 0; i < IDX_STATUS_NBR; i++) {
            mStatsBlock.mStatusReturnedCount[i] = 0;
        }
        for(int i = 0; i < SAW_NBR; i++) {
            mStatsBlock.mSAWTypeCount[i] = 0;
        }
    }

    // translate transfer length to index
    // @param xferLen - transfer length 
    // Returns: the enumeration value to the transfer length
    IDS_HIST_SIZE_IDX GetLengthIndex(ULONG xferLen) const  ;

    // translate status in to its statistics' index
    // @param status - status value
    // Returns: the enumeration value to the status
    IDX_STATUS_RETURNED GetStatusIndex(EMCPAL_STATUS status);

    // Get Stripe Align Write Type of the Io request
    // Notes that all the units are Bytes
    // @param offset - Offset of Io request
    // @param length - Length of Io request
    // @param stripeSize - Stripe Size
    // Returns: the enumeration value of SAW type
    STRIPE_ALIGN_WRITE_TYPE GetStripeAlignWriteIndex(ULONGLONG offset, ULONG length, ULONG stripeSize);

    // translate length index to its type name
    // @param idx - Enumeration value of length type 
    // Returns: the name string to the enumeration value
    const char* GetLengthType(int idx);

    // translate status index in to its type name
    // @param idx - Enumeration value of status type 
    // Returns: the name string to the enumeration value
    const char* GetStatusType(int idx);

    // translate SAW index in to its type name
    // @param idx - Enumeration value of Stripe Align Write Type
    // Returns: the name string to the enumeration value
    const char* GetStripeAlignWriteType(int saw);

    // Read Histogram Statistics value +1
    // @params xferLen - Read request size in Bytes
    void IncReadHistLockHeld(ULONG xferLen)   { mStatsBlock.mReadHistogram[GetLengthIndex(xferLen)]++; }

    // Write Histogram Statistics value +1
    // @params xferLen - Write request size in Bytes
    void IncWriteHistLockHeld(ULONG xferLen)  { mStatsBlock.mWriteHistogram[GetLengthIndex(xferLen)]++; }

    // Count number of status +1
    // @params status - status value returned
    void IncStatusReturnedLockHeld(EMCPAL_STATUS status) {
        mStatsBlock.mStatusReturnedCount[GetStatusIndex(status)]++;
    }

    // corresponding type count of Stripe Align Write +1
    // @params offset - the offset of io request
    // @params length - the length of io request
    // @params stripeSize - stripe size of the raid group
    void IncStripeAlignWriteLockHeld(ULONGLONG offset, ULONG length, ULONG stripeSize) {
        mStatsBlock.mSAWTypeCount[GetStripeAlignWriteIndex(offset, length, stripeSize)]++;
    }

    // count of throttled writes +1
    void IncThrottleWrCountLockHeld() {
        mStatsBlock.mThrottleWrCount++;
    }

    // This function returns the Read Histogram statistics data
    // @params idx - Histogram index
    // Return:  
    //   Histogram statistics data
    UINT_64 GetReadHistogram(IDS_HIST_SIZE_IDX idx) const  {
        return mStatsBlock.mReadHistogram[idx];
    };

    // This function returns the Histogram statistics data
    // @params idx - Histogram index
    // Return:  
    //   Histogram statistics data
    UINT_64 GetWriteHistogram(IDS_HIST_SIZE_IDX idx) const  {
        return mStatsBlock.mWriteHistogram[idx];
    };

    // This function returns the Read Histogram statistics data
    // @params xferLen - transfer length in bytes
    // Return:  
    //   Histogram statistics data
    UINT_64 GetReadHistogram(ULONG xferLen) const  {
        return mStatsBlock.mReadHistogram[GetLengthIndex(xferLen)];
    };

    // This function returns the Histogram statistics data
    // @params xferLen - transfer length in bytes
    // Return:  
    //   Histogram statistics data
    UINT_64 GetWriteHistogram(ULONG xferLen) const  {
        return mStatsBlock.mWriteHistogram[GetLengthIndex(xferLen)];
    };

    // This function returns the status counts
    // @params idx - status index
    // Return:  
    //   The number of status been returned
    UINT_64 GetStatusReturnedCount(IDX_STATUS_RETURNED idx) const  {
        return mStatsBlock.mStatusReturnedCount[idx];
    }
    
    // This function returns the Stripe Align Write Counts
    // @params idx - SAW type index
    // Return:  
    //   The number of its SAW type
    UINT_64 GetStripeAlignWrites(STRIPE_ALIGN_WRITE_TYPE idx) const  {
        return mStatsBlock.mSAWTypeCount[idx];
    }

    // Return the count number of throttled writes
    UINT_64 GetThrottleWrites() const  {
        return mStatsBlock.mThrottleWrCount;
    }

private:
    VirtualFlareVolumeStatisticsStruct mStatsBlock;
};

// translate transfer length to index
// @param xferLen - transfer length value 
// Returns: the enumeration value to the transfer length
inline IDS_HIST_SIZE_IDX VirtualFlareVolumeProxyStatistics::GetLengthIndex(ULONG xferLen) const
{
    int idx = (int)IO_SIZE_IDX_MAX;
    static ULONG const len2Idx[IO_SIZE_IDX_NBR] = {
        1024*8, 1024*8, 1024*16, 1024*64, 1024*256, 1024*1024, (((ULONG)-1LL) >> 1)
    };
    
    if(xferLen < 8*1024) {
        idx = IO_SIZE_IDX_LT8K;
    } else if(xferLen == 8*1024) {
        idx = IO_SIZE_IDX_EQ8K;
    } else {
        for(idx = IO_SIZE_IDX_LE16K; idx < IO_SIZE_IDX_MAX; idx++) {
            if(xferLen <= len2Idx[idx]){
                break;
            }
        }
    }
    return (IDS_HIST_SIZE_IDX)idx;
}

// translate status in to its statistics' index
// @param status - status value
// Returns: the enumeration value to the status
inline IDX_STATUS_RETURNED VirtualFlareVolumeProxyStatistics::GetStatusIndex(EMCPAL_STATUS status)
{

    int idx = (int)IDX_STATUS_SUCCESS;
    static EMCPAL_STATUS const status2Idx[IDX_STATUS_NBR - 1] = {
        EMCPAL_STATUS_SUCCESS, EMCPAL_STATUS_QUOTA_EXCEEDED, EMCPAL_STATUS_ALERTED, 
    };

    for(idx = IDX_STATUS_SUCCESS; idx < IDX_STATUS_OTHER; idx++) {
        if(status2Idx[idx] == status){
            break;
        }
    }
    
    return (IDX_STATUS_RETURNED)idx;
}

// Get Stripe Align Write Type of the Io request
// Notes that all the units are Bytes
// @param offset - Offset of Io request
// @param length - Length of Io request
// @param stripeSize - Stripe Size
// Returns: the enumeration value of SAW
inline STRIPE_ALIGN_WRITE_TYPE VirtualFlareVolumeProxyStatistics::GetStripeAlignWriteIndex(ULONGLONG offset, ULONG length, ULONG stripeSize)
{
    #define STRIPE_ALIGN(x) (0 == (x % stripeSize))
    #define STRIPE_INCLUDE(off, len) \
        ((len >= ((stripeSize * 2) - (off % stripeSize))) || \
        ((len >= stripeSize) && STRIPE_ALIGN(offset)))

    STRIPE_ALIGN_WRITE_TYPE idx;
    if (STRIPE_ALIGN(offset) && length == stripeSize) {
        idx = SAW_FSW;
    }
    else if (STRIPE_ALIGN(offset) && STRIPE_ALIGN(length)) {
        idx = SAW_MSW;
    }
    else if (length < stripeSize) {
        idx = SAW_LSW;
    }
    else if (STRIPE_INCLUDE(offset,length)) {
        idx = SAW_FSI;
    }
    else {
        idx = SAW_NSI;
    }
    return idx;
}

// translate length index to its type name
// @param idx - Enumeration value of length type 
// Returns: the name string to the enumeration value
inline const char* VirtualFlareVolumeProxyStatistics::GetLengthType(int idx)
{
    char* type = "UDEF";
    switch(idx) {
    case IO_SIZE_IDX_LT8K:
        type = "LT8K";
        break;
    case IO_SIZE_IDX_EQ8K:
        type = "EQ8K";
        break;
    case IO_SIZE_IDX_LE16K:
        type = "LE16K";
        break;
    case IO_SIZE_IDX_LE64K:
        type = "LE64K";
        break;
    case IO_SIZE_IDX_LE256K:
        type = "LE256K";
        break;
    case IO_SIZE_IDX_LE1M:
        type = "LE1M";
        break;
    case IO_SIZE_IDX_MAX:
        type = "MAX";
        break;
    };
    return type;
}

// translate SAW index in to its type name
// @param idx - Enumeration value of Stripe Align Write Type
// Returns: the name string to the enumeration value
inline const char* VirtualFlareVolumeProxyStatistics::GetStripeAlignWriteType(int saw)
{
    static char *const idx2Type[SAW_NBR] = {
        "FSW", "MSW", "LSW", "FSI", "NSI"
    };

    if(saw >= SAW_FSW && saw < SAW_NBR) {
        return idx2Type[saw];
    }
    return "UDEF";
}

// translate status index in to its type name
// @param idx - Enumeration value of status type 
// Returns: the name string to the enumeration value
inline const char* VirtualFlareVolumeProxyStatistics::GetStatusType(int idx)
{
    static char *const idx2Type[IDX_STATUS_NBR] = {
        "STATUS_SUCCESS", "STATUS_QUOTA_EXCEEDED", "STATUS_ALERTED", "STATUS_OTHER"
    };

    if(idx >= IDX_STATUS_SUCCESS && idx <= IDX_STATUS_OTHER) {
        return idx2Type[idx];
    }
    return "Unknown";
}

// VirtualFlareVolumeStatistics to be summed per proxy, accessible via BVD/admin
// Changes revlock with admin (DRAMVirtualFlareCom and MMan/MManMP_Redirector/LUNObjects.cpp)
class VirtualFlareVolumeStatistics : public BasicVolumeStatistics, public VirtualFlareVolumeProxyStatistics {
public:
    static const int StatsMaxCPUs = MAX_CORES;

    VirtualFlareVolumeStatistics()  { 
        Reset();
    }
    ~VirtualFlareVolumeStatistics() {};

    // Reset the statistics strustcture
    void Reset() { EmcpalZeroMemory(this, sizeof(*this)); }

    // fill analyzer TOD timestamp 
    // @param stamp - timestamp to fill
    void SetTimestamp(UINT_32 stamp)    { PerformanceTimestamp = stamp; }

    // return timestamp 
    UINT_32 GetPerformanceTimestamp() { return PerformanceTimestamp; }

    // fill analyzer TOD timestamp with current system time
    void SetTodTimestamp() { SetTimestamp(TodTimestamp()); }

    // Time of day
    // return TOD timestamp of current system time
    unsigned int TodTimestamp()
    {
        EMCUTIL_TIME_FIELDS   NtTimeFields;
    
        EmcutilGetSystemTimeFields(&NtTimeFields);
        EmcutilConvertTimeFieldsToLocalTime(&NtTimeFields);
    
        return ((NtTimeFields.Hour << TOD_POLL_HOUR_SHIFT) | 
                (NtTimeFields.Minute << TOD_POLL_MINUTE_SHIFT) | 
                (NtTimeFields.Second << TOD_POLL_SECOND_SHIFT) | 
                ((NtTimeFields.Milliseconds / 10) << TOD_POLL_HUNDREDTH_SHIFT));
    }

    // Operator += will add VirtualFlareVolumeProxyStatistics into VirtualFlareVolumeStatistics.
    VirtualFlareVolumeStatistics & operator += (const VirtualFlareVolumeProxyStatistics * right) {
        for (int idx = 0; idx < IO_SIZE_IDX_NBR; idx++) {
            mStatsBlock.mWriteHistogram[idx] += right->mStatsBlock.mWriteHistogram[idx];
            mStatsBlock.mReadHistogram[idx] += right->mStatsBlock.mReadHistogram[idx];
        }

        for (int idx = 0; idx < IDX_STATUS_NBR; idx++) {
            mStatsBlock.mStatusReturnedCount[idx] += right->mStatsBlock.mStatusReturnedCount[idx];
        }
        
        for (int idx = 0; idx < SAW_NBR; idx++) {
            mStatsBlock.mSAWTypeCount[idx] += right->mStatsBlock.mSAWTypeCount[idx];
        }

        mStatsBlock.mThrottleWrCount += right->mStatsBlock.mThrottleWrCount;

        return *this;
    };

    // DebugDisplay 
    // @param flags : 0 = fields, 1 = csv
    void DebugDisplay(ULONG flags = 0, FILE* fileHandle = stdout);

    // Display the CSV header.
    void DebugDisplayHeading(FILE* fileHandle = stdout);
    
    UINT_32 PerformanceTimestamp; /* navi analyzer TOD timestamp */

};

// Defined so that the class is processor cache line aligned.
class CSX_ALIGN_N(64) VirtualFlareVolumeProxyStatisticsLocked :
    public VirtualFlareVolumeProxyStatistics, public BasicLockedObject {
};

#endif  // __VirtualFlareVolumeStatistics_h__

