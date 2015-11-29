/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               BvdSim_Registry.h
 ***************************************************************************
 *
 * DESCRIPTION:  BvdSimulation Registry Management Declarations
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    06/02/2009  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef __BvdSim_Registry__
#define __BvdSim_Registry__

# include "k10ntddk.h"
# include "generic_types.h"
# include <string.h>
# include "BasicLib/ConfigDatabase.h"
# include "BasicLib/MemoryBasedConfigDatabase.h"
# include "spid_types.h"

#if defined(WDMSERVICES_EXPORT)
#define WDMSERVICEPUBLIC CSX_MOD_EXPORT
#else
#define WDMSERVICEPUBLIC CSX_MOD_IMPORT
#endif

typedef enum Registry_Value_Type_e {
    Registry_Value_Number,
    Registry_Value_String,
} Registry_Value_Type_t;

class BvdSimRegistry_Value {
public:
    BvdSimRegistry_Value(char *name, UINT_32 value, char *nodeName = NULL)
    : mName(name), mNumberValue(value), mStringValue(NULL), mType(Registry_Value_Number), mNodeName(nodeName) {}

    BvdSimRegistry_Value(char *name, char *value, char *nodeName = NULL)
    : mName(name), mNumberValue(0xffffffff), mStringValue(value), mType(Registry_Value_String), mNodeName(nodeName) {}

    Registry_Value_Type_t getType() const { return mType; }
    char                 *getName() const { return mName; }

    UINT_32        getNumberValue() const { return mNumberValue; }
    char          *getStringValue() const { return mStringValue; }

    char          *getNodeName() const { return mNodeName; }

private:
    Registry_Value_Type_e   mType;
    char                    *mName;
    char                    *mStringValue;
    UINT_32                 mNumberValue;
    char                    *mNodeName;
};

class BvdSimExtraParameters {
public:
    WDMSERVICEPUBLIC BvdSimExtraParameters() {
        mNoParameters = 0;
        mParameters = (BvdSimRegistry_Value **)malloc(sizeof(void *) * 100);
    }
    WDMSERVICEPUBLIC void addParameter(BvdSimRegistry_Value *param) {
        mParameters[mNoParameters] = param;
        mNoParameters++;
        FF_ASSERT(mNoParameters < 100);
    }

    WDMSERVICEPUBLIC BvdSimRegistry_Value *getParameter(UINT_32 index) {
        FF_ASSERT(index < mNoParameters);
        return mParameters[index];
    }

    UINT_32   getNumberOfParameters() { return mNoParameters; }

private:
    UINT_32 mNoParameters;
    BvdSimRegistry_Value **mParameters;
};

typedef BvdSimExtraParameters *PBvdSimExtraParameters;

class BvdSimRegistry_Values {
public:
    WDMSERVICEPUBLIC BvdSimRegistry_Values(const char *DriverName, const char *altDriverName = NULL, const char *DatabaseName = NULL)
    : mDriver(DriverName), mAltDriverName(altDriverName),
      mNumberOfMultiplexors(100), mCMMPoolSize(1024*1024), mNumIrpStackLocationsForVolumes(6),
      mSuppressDcaSupport(0), mDatabaseName(NULL), mDriverRecordSize(-1),
      mRecordSize(-1), mMaxVolumes(8 * 1024), mLoadOnIoctl(0), mMaxEdges(1) {

        UINT_32 len = (UINT_32)strlen(DriverName) + (UINT_32)strlen("Factory") + 1;
        mFactory = (char *)malloc(len);
        strcpy_s(mFactory, len, DriverName);
        strcat_s(mFactory, len, "Factory");

        if (DatabaseName)
        {
            len = (UINT_32)strlen(DatabaseName) + 1;
            mDatabaseName = (char *)malloc(len);
            strcpy_s(mDatabaseName, len, DatabaseName);
        }
    }

    WDMSERVICEPUBLIC ~BvdSimRegistry_Values() {
        if(mFactory != NULL) {
            free(mFactory);
        }
        if(mDatabaseName != NULL) {
            free(mDatabaseName);
        }
    }

    WDMSERVICEPUBLIC void            setFactory(char *s) 
                                     { 
                                         if(mFactory != NULL) 
                                         {
                                             free(mFactory);
                                         }

                                         mFactory = s; 
                                     }

    WDMSERVICEPUBLIC void           setDatabaseName(char *s)                                          
                                    { 
                                        if(mDatabaseName != NULL) 
                                        {
                                            free(mDatabaseName);
                                        }

                                        mDatabaseName = s; 
                                    }

    WDMSERVICEPUBLIC void          setCMMPoolSize(UINT_64 n) { mCMMPoolSize = n; }
    WDMSERVICEPUBLIC void           setMaxVolumes(UINT_32 n) { mMaxVolumes = n; }
    WDMSERVICEPUBLIC void             setMaxEdges(UINT_32 n) { mMaxEdges = n; }
    WDMSERVICEPUBLIC void setNumberOfMultiplexors(UINT_32 n) { mNumberOfMultiplexors = n; }
    WDMSERVICEPUBLIC void setNumIrpStackLocationsForVolumes(UINT_32 n) { mNumIrpStackLocationsForVolumes = n; }
    WDMSERVICEPUBLIC void   setSuppressDcaSupport(UINT_32 n) { mSuppressDcaSupport = n; }
    WDMSERVICEPUBLIC void     setDriverRecordSize(UINT_32 n) { mDriverRecordSize = n; }
    WDMSERVICEPUBLIC void           setRecordSize(UINT_32 n) { mRecordSize = n; } 
    WDMSERVICEPUBLIC void           setLoadOnIoctl(UINT_32 n) { mLoadOnIoctl = n; } 


    WDMSERVICEPUBLIC const char *      getDriverName() { return mDriver; }
    WDMSERVICEPUBLIC const char *   getAltDriverName() { return mAltDriverName; }
    WDMSERVICEPUBLIC char *               getFactory() { return mFactory; }
    WDMSERVICEPUBLIC UINT_64          getCMMPoolSize() { return mCMMPoolSize; }
    WDMSERVICEPUBLIC UINT_32           getMaxVolumes() { return mMaxVolumes; }
    WDMSERVICEPUBLIC UINT_32             getMaxEdges() { return mMaxEdges; }
    WDMSERVICEPUBLIC UINT_32 getNumberOfMultiplexors() { return mNumberOfMultiplexors; }
    WDMSERVICEPUBLIC UINT_32 getNumIrpStackLocationsForVolumes() { return mNumIrpStackLocationsForVolumes; }
    WDMSERVICEPUBLIC UINT_32   getSuppressDcaSupport() { return mSuppressDcaSupport; }
    WDMSERVICEPUBLIC char *          getDatabaseName() { return mDatabaseName; }
    WDMSERVICEPUBLIC UINT_32     getDriverRecordSize() { return mDriverRecordSize; }
    WDMSERVICEPUBLIC UINT_32           getRecordSize() { return mRecordSize; } 
    WDMSERVICEPUBLIC UINT_32          getLoadOnIoctl() { return mLoadOnIoctl; } 


private:
    const char *mDriver;         // Root Node value "DisplayName"
    const char  *mAltDriverName; // Trace Node value "Trace"
    char *mFactory;              // Factory Node key "Factory";
    UINT_64 mCMMPoolSize;      // value "CMMPoolSize"
    UINT_32 mMaxVolumes;           // value "MaxVolumes"
    UINT_32 mMaxEdges;              // value "MaxEdges"
    UINT_32 mNumberOfMultiplexors; // value "NumberOfMultiplexors"
    UINT_32 mNumIrpStackLocationsForVolumes;   // value "NumIrpStackLocationsForVolumes"
    UINT_32 mSuppressDcaSupport;   // value "SuppressDcaSupport" bit mask
    char *mDatabaseName;         // value "DatabaseName"
    UINT_32 mDriverRecordSize;     // value "RecordSize"
    UINT_32 mRecordSize;           // value "RecordSize"
    UINT_32 mLoadOnIoctl;
};

typedef BvdSimRegistry_Values *PBvdSimRegistry_Values;

class BvdSim_Registry_Data {
public:
    WDMSERVICEPUBLIC BvdSim_Registry_Data(const char *DriverName, const char *altDriverName = NULL,
                         PBvdSimRegistry_Values BvdRegistryData = NULL,
                         PBvdSimExtraParameters extraParameters = NULL);
    WDMSERVICEPUBLIC const char *getDriverName();
    WDMSERVICEPUBLIC const char *getAltDriverName();
    WDMSERVICEPUBLIC BvdSimRegistry_Values *getBvdRegistryData();
    WDMSERVICEPUBLIC BvdSimExtraParameters *getExtraPameters();
    WDMSERVICEPUBLIC void addParameter(BvdSimRegistry_Value *param);

private:
    const char *mDriverName;
    const char *mAltDriverName;
    BvdSimRegistry_Values *mBvdRegistryData;
    BvdSimExtraParameters *mExtraParameters;
};

typedef BvdSim_Registry_Data *PBvdSim_Registry_Data;

class BvdSim_Registry;

class BvdSim_Registry {
public:
    static WDMSERVICEPUBLIC MemoryBasedDatabaseKey *GetRegistry();

    static WDMSERVICEPUBLIC void Init_Driver_Info(PBvdSim_Registry_Data registryData);

    static WDMSERVICEPUBLIC MemoryBasedDatabaseKey *Get_Driver_Info(const char * RegistryPath);

    static WDMSERVICEPUBLIC void registerSpId(UINT_32 Sp);
    static WDMSERVICEPUBLIC const char *getSpIdName();
    static WDMSERVICEPUBLIC UINT_32 getSpId();
    static WDMSERVICEPUBLIC void registerStackSize(UINT_32 StackSize);
    
    static WDMSERVICEPUBLIC PSPID getPSPID();
    static WDMSERVICEPUBLIC UINT_32 getStackSize();
private:
    static void InitBVDInfo(MemoryBasedDatabaseKey *parameterKey, BvdSimRegistry_Values *values);
    static void InitExtraParams(MemoryBasedDatabaseKey *parameterKey, BvdSimExtraParameters *extraParams);
    static MemoryBasedDatabaseKey* FindOrAllocateNode(MemoryBasedDatabaseKey *parameterKey, char * path);

    BvdSim_Registry();
    ~BvdSim_Registry();
};

#endif
