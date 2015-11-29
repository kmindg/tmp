/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               IOSectorManagement.h
 ***************************************************************************
 *
 * DESCRIPTION:  Definitions for the BVDSim IO Sector Management Facility
 *
 *               This class implements a singleton/factory pattern.
 *               This factory provides methods for managing the life cycle of
 *               IOBuffer, IOSectorIdentity, IOReference and IOtaskId instances.
 *
 *               Contained within this facotry is the master Database of 
 *               IOSectorIdentity instances used to track IO's within 
 *               the BVDSimulation Environment
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    01/27/2010  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef _IOSECTORMANAGEMENT_
#define _IOSECTORMANAGEMENT_

#ifdef __cplusplus

# include "generic_types.h"

# define SECTORSIZE 512
# define FIBRE_SECTORSIZE 520
# define SGL_SIZE 8192

typedef enum IOTaskType_e {
    IOTask_Type_512 = 0x0,
    IOTask_Type_520 = 0x1,
} IOTaskType_t;

typedef enum IOTaskSP_e {
    IOSTASK_SP_UNKNOWN      = 0x2,
    IOTASK_SP_A             = 0x0,
    IOTASK_SP_B             = 0x1,
} IOTASKSP_T;

typedef enum IOSectorPattern_e
{
    SectorPatternRepeating      = 0,
    SectorPatternEmpty          = 1,
    SectorPatternIncrementing   = 2,
    SectorPatternUnknown        = 3
} IOSectorPattern_t;

typedef enum IOTaskOp_e
{
    IOTask_OP_SglRead = 0x0,
    IOTask_OP_SglWrite = 0x1,
} IOTaskOp_t;

class IOTaskId;
class IOBuffer;
class IOReference;
class IOSectorIdentity;
class IOSectorIdentity_ID;

#if defined(WDMSERVICES_EXPORT)
#define WDMSERVICEPUBLIC CSX_MOD_EXPORT
#else
#define WDMSERVICEPUBLIC CSX_MOD_IMPORT
#endif


/*
 * IOSector_DM performs DataManagement.
 * It is opaque inside the IOSectorManement.
 * 
 * The class is declared here so that the other IO* classes
 * can specify that is a a friend
 */
class IOSector_DM;

/*
 * The IOSectorManagement is a singleton factory
 *
 * As a singleton, this class is implemented entirely using static methods
 * rather than exposing a getInstance() static method. 
 *
 * Note static methods can readily become global entrypoints in a DLL.
 *   1) Constructors for the IOTask, IOBuffer, IOSectorIdentity and IOReferences are
 *      of the form ClassNameFactory
 *   2) Destructors for IOTask, IOBuffer, IOSectorIdentity and IOReferences are
 *      of the form destroy(instance)
 */



class IOSectorManagement
{
public:
    static WDMSERVICEPUBLIC void initialize(UINT_32 currentSP, bool skipSyncDelay);

    static WDMSERVICEPUBLIC void incrementIOGenerationNumber();

    static WDMSERVICEPUBLIC UINT_32 getIOGenerationNumber();

    static WDMSERVICEPUBLIC IOTaskId *IOTaskIdFactory(IOTaskType_t taskType = IOTask_Type_512, 
                                                      char *name = NULL, 
                                                      bool chkSumNeeded = FALSE,
                                                      IOTASKSP_T SP = IOSTASK_SP_UNKNOWN);


    static WDMSERVICEPUBLIC IOBuffer *IOBufferFactory(UINT_64 Len);
    static WDMSERVICEPUBLIC IOBuffer *IOBufferFactory(UINT_64 Len, UINT_8 *buffer);

    static WDMSERVICEPUBLIC IOSectorIdentity *IOSectorIdentityFactory(IOSectorIdentity_ID sectorIdentityID,
                                                                      IOSectorIdentity *Parent,
                                                                      bool duplicateAllowed);
    static WDMSERVICEPUBLIC IOSectorIdentity *IOSectorIdentityFactory(IOTaskId *taskID,
                                                                      IOSectorPattern_t Pattern,
                                                                      UINT_32 Seed,
                                                                      IOSectorIdentity *Parent = NULL);

    static WDMSERVICEPUBLIC IOReference *IOReferenceFactory(IOTaskId *taskID,
                                                            IOSectorPattern_t Pattern,
                                                            UINT_32 Seed,
                                                            UINT_64 NoSectors,
                                                            IOBuffer *iob = NULL);
    static WDMSERVICEPUBLIC IOReference *IOReferenceFactory(IOTaskId *taskID, IOBuffer *iob);

    static WDMSERVICEPUBLIC void destroy(IOBuffer *iob);
    static WDMSERVICEPUBLIC void destroy(IOSectorIdentity *ios);
    static WDMSERVICEPUBLIC void destroy(IOReference *ior);
    static WDMSERVICEPUBLIC void destroy(IOTaskId *iot);

    static WDMSERVICEPUBLIC IOSectorIdentity *findIOSectorIdentity(IOSectorIdentity_ID ID);
    static WDMSERVICEPUBLIC IOTaskId *findIOTaskId(UINT_64 ID);

private:
    friend class IOTaskId;
    friend class IOBuffer;
    friend class IOSectorIdentity;
    friend class IOReference;

    static UINT_32 getCurrentSP();
    static UINT_32 getNext_IOTaskID();
    static UINT_32 getNext_IOSectorIdentityID();

    virtual void run();
};

#endif // __cplusplus
#endif // _IOSECTORMANAGEMENT_
