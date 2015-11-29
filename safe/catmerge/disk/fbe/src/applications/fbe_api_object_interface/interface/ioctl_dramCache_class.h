/*********************************************************************
 * Copyright (C) EMC Corporation, 1989 - 2011                    
 * All Rights Reserved.                                          
 * Licensed Material-Property of EMC Corporation.                
 * This software is made available solely pursuant to the terms  
 * of a EMC license agreement which governs its use.             
 *********************************************************************/

/*********************************************************************
 *
 *  Description: 
 *      This file defines the IOCTL DramCache class.
 *
 *  History:
 *      12-May-2010 : inital version
 *
 *********************************************************************/

#ifndef T_IOCTL_DRAMCACHE_CLASS_H
#define T_IOCTL_DRAMCACHE_CLASS_H

#ifndef T_IOCTL_CLASS_H
#include "ioctl_class.h"
#endif

/*********************************************************************
 *          Ioctl DramCache class : bIoctl base class
 *********************************************************************/

class IoctlDramCache : public bIoctl
{
    protected:
        
        // Every object has an idnumber
        unsigned idnumber;
        unsigned idcCount;

        // interface object name
        char  * idname;
  
        // usage 
        char * IoctlDramCacheFuncs; 
        char * usage_format;

    public:
           
        // Cache class objects and working storage
        CacheVolumeStatistics CacheVolStats;
        CacheVolumeStatus VolStatus;    
        CacheVolumeParams VolParams;  
        CacheDriverParams DriverParams;   
        CacheDriverStatus DriverStatus;
        CacheDriverStatisticsExtended CacheDriverStats;
        CacheVolumeAction CacheVolAction;
        IoctlCacheAllocatePseudoControlMemory AllocatePseudoControlMemory; 
        IoctlCacheAllocatePseudoControlMemoryOutput AllocatePseudoControlMemoryOutput;
        IoctlCacheFreePseudoControlMemory FreePseudoControlMemory;
        K10_LU_ID Wwn;

        // Constructor
        IoctlDramCache();
        
        // Destructor
        ~IoctlDramCache(){}

        // Accessor methods
        char   * MyIdNameIs(void);
        unsigned MyIdNumIs(void);
        int      MyCountIs(void);
        void     dumpme(void);
		
        // Select method to send Ioctl to DRAMCache driver
        fbe_status_t select(int index, int argc, char *argv[]); 
       
        // create/destroy volume methods
        fbe_status_t createVol   (int argc, char ** argv);
        fbe_status_t destroyVol  (int argc, char ** argv);
        
        // ------------------------------------------------------------
};

#endif /*T_IOCTL_DRAMCACHE_CLASS_H */