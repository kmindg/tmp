/***************************************************************************
 * Copyright (C) EMC Corporation 2011-2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               AbstractStreamConfig.h
 ***************************************************************************
 *
 * DESCRIPTION: Abstract Class for stream configs used for BvdSimIO tests.    
 *       
 *       
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    03/10/2011 Komal Padia Initial Version
 *
 **************************************************************************/
#ifndef __ABSTRACTSTREAMCONFIG__
#define __ABSTRACTSTREAMCONFIG__

#include "generic_types.h"
#include "BvdSimIoTypes.h"
#include "BvdSimFileIo.h"
#include "simulation/DeviceConstructor.h"
#include "IOTaskId.h"
#include "DeviceConstructor.h"

class ConcurrentIoOperation; 
class ListOfIoOperations; 
class AbstractStreamConfig; 

class ConcurrentIoIterator {
public:
    virtual ~ConcurrentIoIterator() {};
    
    virtual ConcurrentIoOperation* next() = 0; 

    virtual PBvdSimFileIo_File getFile() = 0;

    virtual AbstractStreamConfig* getStreamConfig() = 0; 

    virtual PIOTaskId getTaskID() = 0; 

    virtual UINT_32 getDeviceIndex() = 0;

    virtual UINT_32 getRGIndex() = 0;

    virtual void* getControlMemoryAddress() const { return 0; }

    virtual void* getDataMemoryAddress() const { return 0; }

}; 

class AbstractStreamConfig {
public:

    virtual ~AbstractStreamConfig() {}; 

    virtual UINT_32 getStartBlock() = 0;

    virtual UINT_32 getEndBlock() = 0; 

    virtual UINT_32 getBlocksToTransfer()  = 0;

    virtual UINT_32 getStrideInBlocks() const { return 0; }

    virtual UINT_32 getIterations()  = 0;

    virtual BvdSimIo_stream_type_t getStreamType()  = 0;
 
    virtual BvdSimIo_type_t getReadType()  = 0;

    virtual BvdSimIo_type_t getWriteType()  = 0; 

    virtual BvdSimIo_type_t getZeroFillType()  = 0; 

    virtual PIOTaskId getTaskID() = 0; 

    virtual UINT_32 getPostCreateSleep() = 0;  

    virtual IoStyle_t GetIoStyle() = 0; 

    virtual bool IsSectorSizeRandom() = 0; 

    virtual ConcurrentIoIterator* GetIoIterator(PDeviceConstructor deviceFactory, UINT_32 deviceIndex, UINT_32 RGIndex, UINT_32 thdId, UINT_32 totalThds) = 0;
};

#endif
