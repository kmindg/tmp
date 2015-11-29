//***************************************************************************
// Copyright (C) EMC Corporation 2002,2009-2013
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name: CacheStatsOBS.h
//
// Contents:
//    Implements MCC-OBS integration for KittyHawk VNXe platform.
//    This class is a main class that interfaces with OBS framework to expose
//    SP Cache statistic data via OBS.
//    The OBS consumers/clients can access SP Cache using namespace path.
//
//    The Family root for SPCache statistics is: "blockCache", and different type of stats
//    data are grouped under the following namespace path.
//
//    For CacheDriverStatistics data
//          blockCache.global - sub-Family to get Driver statistics
//
//    For CacheVolumeStatistics
//          blockCache.flu  - SET to get Volume/Lun statistics
//
//    This class creates all statistics Counters/Facts fields which is part of the OBS framework.
//    All Counters/Facts fields must be created using the OBS macro.
//
//   Important Notes:
//     - If any news statistics data/params are added for CacheVolumeStatistics and CacheDriverStatistics
//       those new stats data/params must be defined as either Counter or Fact field in CacheStatsOBS.cpp file.
//
// Revision History:
//    05/08/2013 Inititial Revision
//--
#ifdef C4_INTEGRATED

#ifndef __CacheStatsOBS_h_
#define __CacheStatsOBS_h_

#include "BasicLib/BasicLockedObject.h"
#include "Cache/CacheDriverStatistics.h"
#include "Cache/CacheVolumeStatistics.h"
#include "Cache/ReplacementParams.h"

// OBS freamework header file
#include "stats.hxx"

//-----------------------------------------------------------------------
// Note: OBS requires that Lookup() and EnumerateAndCallFunction() function
//        must be implemented if the statistics is implemented as a SET.
//-----------------------------------------------------------------------

//----------------------------------
// For Volume statistics
//----------------------------------

// CacheHistogramLookup()
//       - Lookup() for specific volume histogram. 
//
//    @param  STD::string name - string of histogram bucket to be looked up for 
//    @param  Stat::SetLookupData info - class from OBS framework, maintains info about how to lookup set data
//    @param  STD::string& errorMsg  - string of error message to be returned to the OBS framework. 
//    @return Stat::Addr   - class from OBS framework, represents the address of an Element.
Stat::Addr CacheHistogramLookup(STD::string name, Stat::SetLookupData info, STD::string& errorMsg);

// CacheHistogramEnumerateAndCallFunction()
//    - The Volume histogram is implemented as a Set-inside-Set.
//    - When OBS consumer requests a Volume statistics, the Volume histogram stats data will
//      also be returned. This funciton is also automatically invoked when the 
//      "CacheVolumeEnumerateAndCallFunction()" function is done.     
//
//    @param  Stat::BaseSet::EnumParams& params - class from Stats framework, encapsulates all the
//                                                prameters associated with a specific stats like name, visibility,etc.
void  CacheHistogramEnumerateAndCallFunction(const Stat::BaseSet::EnumParams& params);

// CacheHistogramOBS class
//   - This class interfaces with OBS framework. The class must be derived from Stat::StructMeterBase which
//     is an OBS based class. This is a requirement by OBS framework.
//     This class is used to handle the Volume Histogram statistics data.
class CacheHistogramOBS : public Stat::StructMeterBase {

  public:
    CacheHistogramOBS(); 
    ~CacheHistogramOBS(); 

    // Assign a CacheVolumeOBS pointer to data member here. 
    void    SetVolumeOBSPtr (CacheVolumeOBS *volPtr) { mVolOBSPtr = volPtr; } 

    // A fetching method called by OBS framework to get Volume histogram stats data
    //    @param  volHistStats - volume histogram stats data to be filled in and returned to OBS framework  
    void    GetCacheHistogramStats(CacheVolumeHistogramStruct &volHistStats);

    // Set Histogram bucket number.
    //     @param num - histogram number to be set
    void    SetHistBucketNum(UINT_32 num)  { mHistBucketNum = num; };

    // Get Histogram bucket number.
    UINT_32 GetHistBucketNum()             { return (mHistBucketNum); };

    // A pointer to the CacheVolumeOBS object where this histogram belongs to
    // Use this pointer to reference back to the CacheVolumeOBS to get statistic data 
    // being held in that object.
    CacheVolumeOBS			*mVolOBSPtr;

  private:
    // Histogram bucket/slot number 
    UINT_32				mHistBucketNum;
};

// CacheVolumeLookup()
//   - allow OBS consumer to retrieve statistic data for a specific Volume/Luns based on FLU number.
//     The namespace path can be specified as  "blockCache.flu.(nn)" where nn is a FLU number
//
//    @param  STD::string name - string of histogram bucket to be looked up for 
//    @param  Stat::SetLookupData info - class from OBS framework, maintains info about how to lookup set data
//    @param  STD::string& errorMsg  - string of error message to be returned to the OBS framework. 
//    @return Stat::Addr   - class from OBS framework, represents the address of an Element.
Stat::Addr CacheVolumeLookup(STD::string name, Stat::SetLookupData info, STD::string& errorMsg);

// CacheVolumeEnumerateAndCallFunction()
//   - allow OBS consumer to retrieve statistic data all Volume/Luns.
//     The namespace path can be specified as  "blockCache.flu" 
//
//    @param  Stat::BaseSet::EnumParams& params - class from Stats framework, encapsulates all the
//                                                prameters associated with a specific stats like name, visibility,etc.
void  CacheVolumeEnumerateAndCallFunction(const Stat::BaseSet::EnumParams& params);

// CacheVolumeOBS class
//   - This class interfaces with OBS framework. The class must be derived from Stat::StructMeterBase which
//     is an OBS based class. This is a requirement by OBS framework.
//     This class provides volume/LUN statistic to the OBS framework 
//     There is one CacheVolumeOBS object created to be associated with each Volume/Lun created.
//     When the LUN is deleted, this object is also deleted.
class CacheVolumeOBS : public Stat::StructMeterBase {

 public:
    CacheVolumeOBS(CacheVolume *volume);
    ~CacheVolumeOBS();

    // A fetching method called by OBS framework to get Volume stats data
    //    @param  cacheVolStats - volume stats data to be filled in and returned to OBS framework  
    void    GetCacheVolumeStats (CacheVolumeStatisticsStruct &cacheVolStats);

    // A pointer to the Volume that this VolumeOBS object is associated to.
    CacheVolume                        *mVolumePtr;

    // A cache volume stat data to be retrieved when the fetching function is called.
    CacheVolumeStatistics               mSumVolumeStats;

    // An array of CacheHistogramOBS object.
    CacheHistogramOBS                   mHistogramStats[ADM_HIST_SIZE+1];

};


//----------------------------------
// For Driver statistics
//----------------------------------

// CacheReplacementTrackLookup()
//   - MCC does not allow the lookup for a specific replacement track.
//     The implementation will return "Not Allow" message to consumer.
//
//    @param  STD::string name - string of histogram bucket to be looked up for 
//    @param  Stat::SetLookupData info - class from OBS framework, maintains info about how to lookup set data
//    @param  STD::string& errorMsg  - string of error message to be returned to the OBS framework. 
//    @return Stat::Addr   - class from OBS framework, represents the address of an Element.
Stat::Addr CacheReplacementTrackLookup(STD::string name, Stat::SetLookupData info, STD::string& errorMsg);

// CacheReplacementTrackEnumAndCall()
//   - This is for Driver extended Replacement track statistics data
//     The namespace path is "blockCache.global.replacementTrack" 
//    @param  Stat::BaseSet::EnumParams& params - class from Stats framework, encapsulates all the
//                                                prameters associated with a specific stats like name, visibility,etc.
void CacheReplacementTrackEnumAndCall(const Stat::BaseSet::EnumParams& params);

// CacheReplacementTrackOBS class
//   - This class interfaces with OBS framework. The class must be derived from Stat::StructMeterBase which
//     is an OBS based class. This is a requirement by OBS framework.
//     This class provides Driver statistic for Extended Replacement Track to the OBS framework 
class CacheReplacementTrackOBS : public Stat::StructMeterBase {

  public:
    CacheReplacementTrackOBS(); 
    ~CacheReplacementTrackOBS(); 

    // A fetching method called by OBS framework to get Driver stats data
    //    @param  CacheReplacementTrackStatsStruct - replacement track stats data to be filled in and returned to OBS framework
    void    GetCacheReplacementTrackStats (CacheReplacementTrackStatsStruct &cacheRepTrack);

    // A method to help sum the ReplacementTrack stats data from all cores.
    //     @param trackNum       - replacement track number
    //     @param cacheRepTrack  - replacement track stats data
    //     @param drvStatsExt    - driver statistics data which contains replacement track data in it
    void    SumReplacementTrackStatistics( int trackNum, CacheReplacementTrackStatsStruct *cacheRepTrack,
                                           CacheDriverStatisticsExtended  *drvStatsExt);

    // Set Replacement Track number.
    //     @param num - replacement track number to be set
    void    SetReplacementTrackNumber(UINT_32 num)  { mRepTrackNumber = num; };

    // Get Replacement Track number.
    INT_32  GetReplacementTrackNumber()             { return (mRepTrackNumber); };

  private:
    // Replacement Track number used to pass it between EnumerateAndCallFunction() and fetching method.
    INT_32 			      mRepTrackNumber; 
};

// CacheDriverOBS class
//   - This class interfaces with OBS framework. The class must be derived from Stat::StructMeterBase which
//     is an OBS based class. This is a requirement by OBS framework.
//     This class provides Driver statistic to the OBS framework 
class CacheDriverOBS : public BasicLockedObject, public Stat::StructMeterBase {

 public:
    CacheDriverOBS();
    ~CacheDriverOBS();

    // Functions required for OBS framework, need to be friend here 
    friend void  CacheDriverAtomicFetch(Stat::ReqInstance*);

    // A fetching method called by OBS framework to get Driver stats data
    //    @param  cacheDriverStats - driver stats data to be filled in and returned to OBS framework  
    void    GetCacheDriverStats (CacheDriverStatisticsStruct &cacheDriverStats);

    // Array of Replacment Track to hold its data, so OBS fetching function can access to it.
    CacheReplacementTrackOBS  mReplaceTrackStatData[MaxReplacementTrackPerPool];

};

#endif //__CacheStatsOBS_h_
#endif // C4_INTEGRATED
