#ifndef AUTO_MIGRATOR_H
#define AUTO_MIGRATOR_H

#include <windows.h>

#include "k10defs.h"
#include "K10LayeredDefs.h"
#include "tld.hxx"
#include "csx_ext.h"
#include "AutoMigratorMessages.h"

//  
//  All possible Job states are defined here.
// 
enum AM_JOB_STATE
{
    AM_STATE_NEEDS_DESTINATION = 0,
    AM_STATE_READY,
    AM_STATE_QUEUED,
    AM_STATE_RUNNING,
    AM_STATE_SYNCHRONIZED,
    AM_STATE_COMMITTED,
    AM_STATE_FAILED,
    AM_STATE_CANCELED,

    AM_STATE_INVALID        // Indicates a bug
};

//
//  The Job status provides extra context to the Job state
//  in certain situations.
// 
enum AM_JOB_STATUS
{
    AM_STATUS_OK = 0,
    AM_STATUS_AUTOCOMMIT_FAILED,
    AM_STATUS_SOURCE_FULL,
    AM_STATUS_DEST_FULL,
    AM_STATUS_FAULTED,
    AM_STATUS_SOURCE_OFFLINE,
    AM_STATUS_DEST_OFFLINE,

    AM_STATUS_INVALID       // Indicates a bug
};

//
//  Job Auto option flags used to modify the automatic transitions between Job 
//  states.
//  
//  NOTE: 
//    -Auto Cancel and Auto Delete are not currently supported. 
//    -Auto Start and Auto Commit are optional.
// 
//  EXAMPLE USAGE:
// 
//    Enable only Auto Start on Job creation.
//      ULONG32 autoOptions = AM_AUTO_START
// 
//    Enable both Auto Start and Auto Commit on Job creation.
//      ULONG32 autoOptions = ( AM_AUTO_START | AM_AUTO_COMMIT )
//
#define AM_AUTO_START       (1 << 0)
#define AM_AUTO_COMMIT      (1 << 1) 

//  
//  The values below map Job throttle positions to Migration Session rate 
//  values.
// 
//  [----LOW----|----MED----|----HIGH---|----ASAP---]
//  1         25|26       50|51       75|76         100
//
#define AM_THROTTLE_LOW     25
#define AM_THROTTLE_MED     50
#define AM_THROTTLE_HIGH    75
#define AM_THROTTLE_ASAP    100

//
//  The AM Client Record is an opaque (to the Auto Migrator) field in the Job 
//  record that allows a client to store arbitrary information related to the 
//  Job persistently.
// 
//  EXAMPLE USAGE:
//    
//    MY_OWN_STRUCT* pMyOwnStructPtr = (MY_OWN_STRUCT*) malloc( sizeof(AM_CLIENT_RECORD) );
// 
#define AM_CLIENT_RECORD_SIZE 128

typedef UCHAR AM_CLIENT_RECORD[AM_CLIENT_RECORD_SIZE];
typedef AM_CLIENT_RECORD* PAM_CLIENT_RECORD;

//  
//  The AM Job Info struct is used to return Job information to the client.
// 
typedef struct _AM_JOB_INFO
{
    //  The Job's originator.
    K10_LOGICAL_OBJECT_OWNER    OriginatorID;

    //  Source LUN WWN
    K10_LU_ID                   SrcWWN;

    //  Destination LUN WWN
    K10_LU_ID                   DestWWN; 

    //  Compare LUN - used for Diff copy only. 
    ULONG64                     CompOid;       

    //  Current Job State
    AM_JOB_STATE                State;

    //  Additional status corresponding with the Job State
    AM_JOB_STATUS               Status;

    //  TRUE if the Job is individually paused
    BOOLEAN                     bPaused;             

    //  TRUE if the Job is feature paused
    BOOLEAN                     bFeaturePaused;             

    //  Migration Session percentage complete
    ULONG32                     ProgressPercent;

    //  Job throttle ranging from 1 to 100
    ULONG32                     Throttle;

    //  Client Record data
    AM_CLIENT_RECORD            ClientRecord;

} AM_JOB_INFO, *PAM_JOB_INFO;

/*******************************************************************************

  Each Client interface routine is described below in detail. Any return value 
  that is not S_OK should be handled as a failure of the operation by the client. 
  No operation failure will result in the change of a Job's state or status.

*******************************************************************************/

/*
 AutoMigratorCreateJob()

    The AutoMigratorCreateJob routine creates an AutoMigrator Job as a single 
    atomic operation (including any Auto options specified by the Client, such 
    as binding a LUN on their behalf).

 Parameters:

    OriginatorID [in]    
        Client Originator ID

    SrcWWN [in]    
        Source LUN WWN    
        
    pDestTLD [in]     
        TLD with destination LUN details
        (Number and Name tags must be blank)

    pClientRecord [in]   
        Client data to be stored persistently with Job

    Throttle [in]  
        Migration Sync throttle (ranging from 1-100)
        AM_THROTTLE_* values provided in this header file for Client's use

    Options [in]   
        Options that modify the Job behavior. 
        Used with the AM_AUTO_* values defined in this header file.

 Return Values:

    S_OK
        Job and destination (if any) created successfully

    E_INVALIDARG    
        An invalid parameter was specified (TLD, throttle or options)

    AM_LUN_IN_USE    
        Source LUN is already associated with an AM Job

    AM_DEST_CREATE_FAILED
        Destination LUN create operation failed (generic)

    AM_DEST_CREATE_FAILED_...
        Destination LUN create operation failed (for specified reason)
        Refer to the Functional Spec for the list of possibilities

    AM_AUTOSTART_FAILED
        Create internal Migration Session failed (generic)

    AM_AUTOSTART_FAILED_...
        Create internal Migration Session failed (for specified reason)
        Refer to the Functional Spec for the list of possibilities

    AM_INTERNAL_ERROR
        Internal error occurred

 Remarks:

    If a destination TLD is provided, it will create the destination LUN. If 
    AutoStart is enabled in the Options and a destination TLD is provided, it
    will start the session. If an error is returned, no Job or LUN will be created. 

    Note that if pDestTLD is provided, the Number tag must contain the value 0 
    and the Name tag must have no associated data.
*/
extern "C" CSX_MOD_EXPORT 
HRESULT AutoMigratorCreateJob( 
            IN  K10_LOGICAL_OBJECT_OWNER    OriginatorID,
            IN  K10_LU_ID                   SrcWWN,
            IN  TLD                         *pDestTLD,
            IN  PAM_CLIENT_RECORD           pClientRecord,
            IN  ULONG32                     Throttle,
            IN  ULONG32                     Options );

/*
 AutoMigratorAddDestToJob()

    If the client did not supply the destination TLD while creating the Job, 
    this routine allows the client to add an existing LUN by specifying the WWN.

 Parameters:

    SrcWWN [in]    
        Source LUN WWN of an existing Job
        
    DestWWN [in]
        WWN of the LUN to add as a destination
    
 Return Values:

    S_OK
        Destination added successfully

    AM_JOB_NOT_FOUND
        There is no Job associated with the provided source WWN

    AM_INVALID_OPERATION
        This Job is not in the NeedDest state. It already has an associated 
        destination.

    AM_LUN_IN_USE
        Specified Destination LUN is already associated with an AM Job (as 
        source or destination)

    AM_AUTOSTART_FAILED
        Create internal Migration Session failed (generic)

    AM_AUTOSTART_FAILED_...
        Create internal Migration Session failed (for specified reason)
        Refer to the Functional Spec for the list of possibilities

    AM_INTERNAL_ERROR
        Internal error occurred

 Remarks:

    If AutoStart was enabled when the Job was created, the Job will be started
    as a single atomic operation with adding the destination. If an error is
    returned, the Job will remain in the current state and the destination
    will not be added to the Job.
*/
extern "C" CSX_MOD_EXPORT 
HRESULT AutoMigratorAddDestToJob(
            IN  K10_LU_ID   SrcWWN,
            IN  K10_LU_ID   DestWWN );

/*
 AutoMigratorCreateDestForJob()

    This routine allows the client to create a destination LUN via the input 
    TLD and add it to the Job.

 Parameters:

    SrcWWN [in]    
        Source LUN WWN of an existing job   
        
    pDestTLD [in]     
        TLD with destination LUN details
        (Number and Name tags must be blank)

 Return Values:

    S_OK
        Destination created and added to Job successfully

    AM_JOB_NOT_FOUND
        There is no Job associated with the provided source WWN

    E_INVALIDARG    
        An invalid parameter was specified (TLD, throttle or options)

    AM_INVALID_OPERATION
        This Job is not in the NeedDest state. It already has an associated 
        destination.

    AM_DEST_CREATE_FAILED
        Destination LUN create operation failed (generic)

    AM_DEST_CREATE_FAILED_...
        Destination LUN create operation failed (for specified reason)
        Refer to the Functional Spec for the list of possibilities

    AM_AUTOSTART_FAILED
        Create internal Migration Session failed (generic)

    AM_AUTOSTART_FAILED_...
        Create internal Migration Session failed (for specified reason)
        Refer to the Functional Spec for the list of possibilities

    AM_INTERNAL_ERROR
        Internal error occurred
 
    AM_DEST_CREATE_FAILED_INVALID_PREFERRED_PATH
        Destination LUN create operation failed with invalid preferred
        path when the SP is in the booting phase

 Remarks:

    If AutoStart was enabled when the Job was created, the Job will be
    started as a single atomic operation with creating the destination.
    If an error is returned, the Job will remain in the current state
    and the LUN will not be created.
*/
extern "C" CSX_MOD_EXPORT 
HRESULT AutoMigratorCreateDestForJob(
            IN  K10_LU_ID   SrcWWN,
            IN  TLD         *pDestTLD );


/*
 AutoMigratorAddCompToJob()

    This routine allows the client to add an existing compare LUN by specifying the WWN.
    The compere LUN is used for diff copy of src Lun to dest Lun.

 Parameters:

    SrcWWN [in]    
        Source LUN WWN of an existing Job
        
    CompWWN [in]
        WWN of the LUN to add as a comparsion LUN.
    
 Return Values:

    S_OK
        Destination added successfully

    AM_JOB_NOT_FOUND
        There is no Job associated with the provided source WWN

    AM_INVALID_OPERATION
        This Job is not in the NeedDest state. It already has an associated 
        destination.

    AM_LUN_IN_USE
        Specified Destination LUN is already associated with an AM Job (as 
        source or destination)

    AM_AUTOSTART_FAILED
        Create internal Migration Session failed (generic)

    AM_AUTOSTART_FAILED_...
        Create internal Migration Session failed (for specified reason)
        Refer to the Functional Spec for the list of possibilities

    AM_INTERNAL_ERROR
        Internal error occurred

 Remarks:

    If AutoStart was enabled when the Job was created, the Job will be started
    as a single atomic operation with adding the destination. If an error is
    returned, the Job will remain in the current state and the destination
    will not be added to the Job.
*/
extern "C" CSX_MOD_EXPORT
HRESULT AutoMigratorAddCompToJob(
            IN  K10_LU_ID   SrcWWN,
            IN  ULONG64     CompOid );


/*
 AutoMigratorStartJob()

    This routine starts a single Job in the Ready state. 

 Parameters:

    SrcWWN [in]    
        Source LUN WWN of an existing job    

 Return Values:

    S_OK
        Job started successfully

    AM_JOB_NOT_FOUND
        There is no Job associated with the provided source WWN

    AM_OPERATION_FAILED
        Start internal Migration Session failed (generic)

    AM_OPERATION_...
        Start internal Migration Session failed (for specified reason)
        Refer to the Functional Spec for the list of possibilities

    AM_INVALID_OPERATION
        This Job is not in the Ready state

    AM_INTERNAL_ERROR
        Internal error occurred

 Remarks:

    This operation is only valid if AutoStart is not enabled. This will 
    create an internal Migration session. This can have certain side effects.
    If successful, the Job will transition to the Queued or Running state.
    If an error is returned, the Job will remain in its current state and
    the source and destination LUNs will remain unchanged.
*/

extern "C" CSX_MOD_EXPORT 
HRESULT AutoMigratorStartJob(
            IN  K10_LU_ID   SrcWWN );

/*
 AutoMigratorPauseJob()

    This routine pauses a single Job in the Running, Queued, Synchronized, or 
    Failed state. 

 Parameters:

    SrcWWN [in]    
        Source LUN WWN of an existing job    

 Return Values:

    S_OK
        Job paused successfully

    AM_JOB_NOT_FOUND
        There is no Job associated with the provided source WWN

    AM_OPERATION_FAILED
        Pause internal Migration Session failed (generic)

    AM_OPERATION_FAILED_...
        Pause internal Migration Session failed (for specified reason)
        Refer to the Functional Spec for the list of possibilities

    AM_INVALID_OPERATION
        This Job is not in a Queued, Running, Failed, or Synchronized state

    AM_INTERNAL_ERROR
        Internal error occurred

 Remarks:

    The behavior of a Job in the Synchronized or Failed state will be 
    unaffected. This operation will return S_OK on Jobs that are already paused. 
    This operation will not change the state of the Job.
*/
extern "C" CSX_MOD_EXPORT 
HRESULT AutoMigratorPauseJob(
            IN  K10_LU_ID   SrcWWN );

/*
 AutoMigratorResumeJob()

    This routine resumes a Paused Job.

 Parameters:

    SrcWWN [in]    
        Source LUN WWN of an existing job   

 Return Values:

    S_OK
        Job resumed successfully

    AM_JOB_NOT_FOUND
        There is no Job associated with the provided source WWN

    AM_OPERATION_FAILED
        Resume internal Migration Session failed (generic)

    AM_OPERATION_FAILED_...
        Resume internal Migration Session failed (for specified reason)
        Refer to the Functional Spec for the list of possibilities

    AM_INVALID_OPERATION
        This Job is not in a Queued, Running, Failed, or Synchronized state

    AM_INTERNAL_ERROR
        Internal error occurred

 Remarks:

    This operation will succeed on Jobs that are not paused. This operation 
    will not change the state of the Job.
*/
extern "C" CSX_MOD_EXPORT 
HRESULT AutoMigratorResumeJob(
            IN  K10_LU_ID   SrcWWN );

/*
 AutoMigratorCommitJob()

    This routine allows the client to commit a Job that is in the Synchronized 
    state.

 Parameters:

    SrcWWN [in]    
        Source LUN WWN of an existing job    

 Return Values:

    S_OK
        Job committed successfully

    AM_JOB_NOT_FOUND
        There is no Job associated with the provided source WWN

    AM_INVALID_OPERATION
        This Job is not in the Synchronized state or AutoCommit is enabled

    AM_OPERATION_FAILED
        Commit internal Migration Session failed (generic)

    AM_OPERATION_FAILED_...
        Commit internal Migration Session failed (for specified reason)
        Refer to the Functional Spec for the list of possibilities

    AM_INTERNAL_ERROR
        Internal error occurred

 Remarks:

    This operation is only valid if AutoCommit is not enabled. This will 
    complete the internal Migration session by moving the WWN and certain 
    metadata of the source LUN to the destination LUN and destroying the source 
    LUN. If successful, the Job will transition to the Committed state. If an 
    error is returned, the Job will remain in its current state and the source 
    and destination LUNs will remain unchanged.
*/
extern "C" CSX_MOD_EXPORT 
HRESULT AutoMigratorCommitJob(
            IN  K10_LU_ID   SrcWWN );

/*
 AutoMigratorCancelJob()

    This routine cancels a Job that is not yet in the Committed state.  

 Parameters:

    SrcWWN [in]    
        Source LUN WWN of an existing job    

 Return Values:

    S_OK
        Job canceled successfully

    AM_JOB_NOT_FOUND
        There is no Job associated with the provided source WWN

    AM_INVALID_OPERATION
        This Job is already committed

    AM_OPERATION_FAILED
        Cancel internal Migration Session failed (generic)

    AM_OPERATION_FAILED_SYNCHRONIZED
        Cancel internal Migration Session failed because the Job is in the 
        Synchronized state and must be committed

    AM_OPERATION_FAILED_...
        Cancel internal Migration Session failed (for specified reason)
        Refer to the Functional Spec for the list of possibilities

    AM_INTERNAL_ERROR
        Internal error occurred

 Remarks:

    If successful, the Job will transition to the Canceled state, the 
    internal Migration Session (if any) will be destroyed, and the destination 
    LUN (if any) will be destroyed. If failed, nothing will be destroyed.
*/
extern "C" CSX_MOD_EXPORT 
HRESULT AutoMigratorCancelJob(
            IN  K10_LU_ID   SrcWWN );

/*
 AutoMigratorDeleteJob()

    This routine deletes a Job that is in a Committed or Canceled state. 

 Parameters:

    SrcWWN [in]    
        Source LUN WWN of an existing job    

 Return Values:

    S_OK
        Job deleted successfully

    AM_JOB_NOT_FOUND
        There is no Job associated with the provided source WWN

    AM_INVALID_OPERATION
        This Job is not in the Committed or Canceled states

    AM_INTERNAL_ERROR
        Internal error occurred

 Remarks:

    If successful, the Job will no longer exist. If an error is returned, the 
    Job will remain unchanged.
*/
extern "C" CSX_MOD_EXPORT 
HRESULT AutoMigratorDeleteJob(
            IN  K10_LU_ID   SrcWWN );

/*
 AutoMigratorGetJobInfo()

    This routine returns the details of a Job in the form of an AM_JOB_INFO 
    struct.

 Parameters:

    SrcWWN [in]    
        Source LUN WWN of an existing job

    JobInfo [out]
        Job information container

 Return Values:

    S_OK
        Job info returned successfully

    AUTO_MIGRATOR_JOB_NOT_FOUND
        There is no Job associated with the provided source WWN

    AM_INTERNAL_ERROR
        Internal error occurred

 Remarks:

    The Job will remain unchanged.
*/
extern "C" CSX_MOD_EXPORT 
HRESULT AutoMigratorGetJobInfo(
            IN  K10_LU_ID       SrcWWN,
            OUT PAM_JOB_INFO    pJobInfo );

/*
 AutoMigratorThrottleJob()

    This routine changes the throttle of a Job.

 Parameters:

    SrcWWN [in]    
        Source LUN WWN of an existing job

    Throttle [in]
        New throttle value

 Return Values:

    S_OK    
        Throttle set successfully

    E_INVALIDARG    
        Throttle value is not in the correct range

    AM_JOB_NOT_FOUND
        There is no Job associated with the provided source WWN

    AM_OPERATION_FAILED
        Throttle internal Migration Session failed (generic)

    AM_OPERATION_FAILED_...
        Throttle internal Migration Session failed (for specified reason)
        Refer to the Functional Spec for the list of possibilities

    AM_INTERNAL_ERROR
        Internal error occurred

 Remarks:

    The throttle may be changed any time a Job exists. However, it affects only 
    the rate of data copy while the Job is running. The Job will remain in the 
    same state. If an error is returned, the throttle will remain unchanged.
*/
extern "C" CSX_MOD_EXPORT 
HRESULT AutoMigratorThrottleJob(
            IN  K10_LU_ID   SrcWWN,
            IN  ULONG32     Throttle );

/*
 AutoMigratorResetJob()

    This routine resets a Canceled Job.

 Parameters:

    SrcWWN [in]    
        Source LUN WWN of an existing job

 Return Values:

    S_OK
        Job reset successfully

    AM_JOB_NOT_FOUND
        There is no Job associated with the provided source WWN

    AM_INVALID_OPERATION
        This Job is not in the Canceled state

    AM_INTERNAL_ERROR
        Internal error occurred

 Remarks:

     If successful, the Job will transition to the NeedDest state. If an error 
     is returned, the Job will remain in the current state.
*/
extern "C" CSX_MOD_EXPORT 
HRESULT AutoMigratorResetJob(
            IN  K10_LU_ID   SrcWWN );

/*
 AutoMigratorSetClientRecord()

    This routine sets the client record associated with a Job.

 Parameters:

    SrcWWN [in]    
        Source LUN WWN of an existing job

    pClientRecord [in]
        Pointer to the new Client Record.

 Return Values:

    S_OK
        Client record set successfully

    AM_JOB_NOT_FOUND
        There is no Job associated with the provided source WWN

    AM_INTERNAL_ERROR
        Internal error occurred

 Remarks:

     The Job will remain in the same state. If an error is returned, the client 
     record will remain unchanged.
*/
extern "C" CSX_MOD_EXPORT 
HRESULT AutoMigratorSetClientRecord(
            IN  K10_LU_ID           SrcWWN,
            IN  PAM_CLIENT_RECORD   pClientRecord );

/*
 AutoMigratorFeaturePause()

    This routine 'feature pauses' all of a client's Jobs.

 Parameters:

    OriginatorID [in]    
        Client's K10 originator ID

 Return Values:

    S_OK
        Feature Pause set successfully

    AM_OPERATION_FAILED
        Set feature pause failed (generic)

    AM_OPERATION_FAILED_...
        Set feature pause failed (for specified reason)
        Refer to the Functional Spec for the list of possibilities

    AM_INTERNAL_ERROR
        Internal error occurred

 Remarks:

     This operation will succeed when the client is already feature paused.
*/
extern "C" CSX_MOD_EXPORT 
HRESULT AutoMigratorFeaturePause(
            IN  K10_LOGICAL_OBJECT_OWNER    OriginatorID );

/*
 AutoMigratorFeatureResume()

    This routine removes the 'feature pause' for a client.

 Parameters:

    OriginatorID [in]    
        Client's K10 originator ID

 Return Values:

    S_OK
        Removed feature pause successfully

    AM_OPERATION_FAILED
        Remove feature pause failed

    AM_OPERATION_FAILED_...
        Remove feature pause failed (for specified reason)
        Refer to the Functional Spec for the list of possibilities

    AM_INTERNAL_ERROR
        Internal error occurred

 Remarks:

     This operation will succeed when the client is already feature paused.
*/
extern "C" CSX_MOD_EXPORT 
HRESULT AutoMigratorFeatureResume(
            IN  K10_LOGICAL_OBJECT_OWNER    OriginatorID );

/*
 AutoMigratorReportAllJobs()

    This routine returns the Job info for each of a client's Jobs 
    or as many as will fit in the provided buffer. 

 Parameters:

    OriginatorID [in]
        Client originator ID

    BufferSize [in]
        Client allocated buffer size     
        Size of pBuffer in bytes or 0 if pBuffer is NULL

    pBuffer [in]
        Buffer to return Job info    
        Buffer of size BufferSize or NULL

    pBuffer [out]
        Job info for as many Jobs as possible
        Array with JobsInBuffer elements or NULL

    JobsInBuffer [out]
        Number of Jobs returned in the buffer
        0-(Max Jobs)

    TotalJobs [out]
        Total number of Jobs
        0-(Max Jobs)

 Return Values:

    S_OK
        Job info for all Jobs returned successfully

    AM_INTERNAL_ERROR
        Internal error occurred

 Remarks:

     This operation returns the number of Jobs in the buffer and also the total 
     number of Jobs. If these do not match, the client may increase the size 
     of the buffer and reissue it to obtain information on all of its Jobs.
*/
extern "C" CSX_MOD_EXPORT 
HRESULT AutoMigratorReportAllJobs(
            IN      K10_LOGICAL_OBJECT_OWNER    OriginatorID,
            IN      ULONG32                     BufferSize,
            IN OUT  PAM_JOB_INFO                pBuffer,
            OUT     ULONG32                     &JobsInBuffer,
            OUT     ULONG32                     &TotalJobs );

/*
 AutoMigratorFindSrcWWN()

    This routine returns the source LUN WWN of an existing Job identified 
    by its destination LUN WWN. 

 Parameters:

    DestWWN [in]    
        Valid LUN WWN of an existing Job

    SrcWWN  [out]
        Valid LUN WWN

 Return Values:

    S_OK
        Source WWN returned successfully

    AM_JOB_NOT_FOUND
        There is no Job associated with the provided destination WWN

    AM_INTERNAL_ERROR
        Internal error occurred

 Remarks:

     The Job will remain unchanged.
*/
extern "C" CSX_MOD_EXPORT 
HRESULT AutoMigratorFindSrcWWN(
            IN      K10_LU_ID   DestWWN,
            OUT     K10_LU_ID   &SrcWWN );

#endif
