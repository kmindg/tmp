#pragma once
#include "K10TraceMsg.h"    // For Tracing
#include "k10defs.h"        // For K10_LU_ID
#include "tld.hxx"          // For TLDs
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT

#define TRACE_UTIL_MAX_TRACE_LENGTH (1024)
#define TRACE_UTIL_MAX_CONTEXT_LENGTH (255)
#define TRACE_UTIL_MAX_ROUTINE_NAME_LENGTH (48)

//
//  These variables used to track a class of identical incidents and to
//  decide which particular instances of the incident are worthy of
//  further attention.
//

class TRACE_UTIL_INCIDENT_NOTEWORTHINESS
{
public:
    //
    //
    //  The number of incidents since the object was initialized
    //  or reset which will automatically be deemed noteworthy.  
    //
    csx_u64_t NumInitialNoteworthyIncidents;

    //
    //  Additional incidents between <NumInitialNoteworthyIncidents>
    //  and this incident will be deemed non-noteworthy. 
    //
    csx_u64_t InitialModValue;

    //
    //  The factor by which the mod interval grows each time it is
    //  reached. The incident at which an interval is reached
    //  is deemed noteworthy, and all subsequent incidents 
    //  until the next such interval are deemed non-noteworthy.
    //
    csx_u64_t ModIntervalGrowth;

    //
    //  The maximum value that the mod interval may reach.
    //
    csx_u64_t MaxModInterval;

    //
    //  The number of incidents that have been reported. 
    //
    csx_u64_t LastRecordedIncident;

    //
    //  The instance nunmber of the next noteworthy incident. 
    //
    csx_u64_t NextNoteworthyIncident;

    //
    //  The mod value currently in effect. 
    //
    csx_u64_t CurrentModValue;

    TRACE_UTIL_INCIDENT_NOTEWORTHINESS()
    {
        //
        // Initialize with the following default value
        //
        NumInitialNoteworthyIncidents = 5;
        InitialModValue = 10;
        ModIntervalGrowth = 5;
        MaxModInterval = 100;
        LastRecordedIncident = 0;
        NextNoteworthyIncident = 1;
        CurrentModValue = 0;
    }

};


class CSX_CLASS_EXPORT TraceUtil
{

public:
    //*****************************************************************************
    //.++
    //  FUNCTION:
    //     TraceUtil
    //
    //  SUMMARY:
    //     Class Constructor:
    //
    //  PARAMETERS:
    //     pExistingTraceObject : Pointer to a K10TraceMsg object if the client already has 
    //                            created one.
    //     routineName          : Function\Routine that this object is used for. 
    //     pInputRetVal         : Pointer to a status return failue for the Function\Routine. 
    //
    //  FUNCTIONAL DESCRIPTION:
    //     Contrustor to instantiate the TraceUtil object.
    //  
    //  RETURN VALUE
    //     None
    //
    //  REMARKS
    //     - All traces traced by this object will be preceeded by the string 
    //       pointed to by routineName
    //     - if pInputRetVal is provided, then Function\Routine exits with a status 
    //       different than S_OK will be trace at ERROR_LEVEL.
    //     - If enableElapsedTimeTracing() is called on the object and the function, 
    //       takes longer than the threshold provided, the function exit is traced irrespective 
    //       of status being returned.
    //                            
    //.--
    //*****************************************************************************
    TraceUtil(K10TraceMsg *pExistingTraceObject, const char *routineName, HRESULT *pInputRetVal = NULL);

    //*****************************************************************************
    //.++
    //  FUNCTION:
    //     TraceUtil
    //
    //  SUMMARY:
    //     Class Constructor:
    //
    //  PARAMETERS:
    //     componentName        : Component name. This is the same string that you 
    //                            would use to instantiate a K10TraceMsg object.
    //     routineName          : Function\Routine that this object is used for. 
    //     pInputRetVal         : Pointer to a status return failue for the Function\Routine. 
    //
    //  FUNCTIONAL DESCRIPTION:
    //     Contrustor to instantiate the TraceUtil object.
    //  
    //  RETURN VALUE
    //     None
    //
    //  REMARKS
    //     - All traces traced by this object will be preceeded by the string 
    //       pointed to by routineName
    //     - if pInputRetVal is provided, then Function\Routine exits with a status 
    //       different than S_OK will be trace at ERROR_LEVEL.
    //     - If enableElapsedTimeTracing() is called on the object and the function, 
    //       takes longer than the threshold provided, the function exit is traced irrespective 
    //       of status being returned.
    // 
    //   EXAMPLE
    //     COMPRESS: CompressAdminSingleLunPoll() B6C82B606EA9E012 bFound:0 EXITING with status 0x7165820A. Took 7 msecs.
    //componentName+       routineName          + LUID Context   + Context+                   + ReturnVal + ElapsedTime      
    //.--
    //*****************************************************************************
    TraceUtil(const char * componentName, const char *routineName, HRESULT *pInputRetVal = NULL);


    //*****************************************************************************
    //.++
    //  FUNCTION:
    //     TraceUtil
    //
    //  SUMMARY:
    //     Class Constructor:
    //
    //  PARAMETERS:
    //     componentName        : Component name. This is the same string that you 
    //                            would use to instantiate a K10TraceMsg object.
    //     routineName          : Function\Routine that this object is used for.
    //     pInputNW             : Point to the noteworthiness data
    //     pInputRetVal         : Pointer to a status return failue for the Function\Routine. 
    //
    //  FUNCTIONAL DESCRIPTION:
    //     Contrustor to instantiate the TraceUtil object.
    //  
    //  RETURN VALUE
    //     None
    //
    //  REMARKS
    //     - All traces traced by this object will be preceeded by the string 
    //       pointed to by routineName
    //     - if pInputRetVal is provided, then Function\Routine exits with a status 
    //       different than S_OK will be trace at ERROR_LEVEL.
    //     - If enableElapsedTimeTracing() is called on the object and the function, 
    //       takes longer than the threshold provided, the function exit is traced irrespective 
    //       of status being returned.
    // 
    //   EXAMPLE
    //     COMPRESS: CompressAdminSingleLunPoll() B6C82B606EA9E012 bFound:0 EXITING with status 0x7165820A. Took 7 msecs.
    //componentName+       routineName          + LUID Context   + Context+                   + ReturnVal + ElapsedTime      
    //.--
    //*****************************************************************************

    TraceUtil(const char * componentName, const char *routineName, TRACE_UTIL_INCIDENT_NOTEWORTHINESS *pInputNW, HRESULT *pInputRetVal = NULL);

    //*****************************************************************************
    //.++
    //  FUNCTION:
    //     ~TraceUtil
    //
    //  SUMMARY:
    //     Destructor
    //
    //  PARAMETERS:
    //
    //  FUNCTIONAL DESCRIPTION:
    //  
    //  RETURN VALUE
    //
    //  REMARKS
    //     
    //
    //.--
    //*****************************************************************************
    ~TraceUtil(void);

    //*****************************************************************************
    //.++
    //  FUNCTION:
    //     setContext
    //
    //  SUMMARY:
    //     Prepends str to all traces
    //
    //  PARAMETERS:
    //     str : Pointer to the string to add as context to traces.
    //
    //  FUNCTIONAL DESCRIPTION:
    //     See Summary
    //  
    //  RETURN VALUE
    //     None
    //  REMARKS
    //     Note that repeated calls to this function are additive. For e.g. setContext(str2) 
    //     followed by setContext(str1) will result in both str1 and str2 being traced.
    //  
    //  EXAMPLE
    //     COMPRESS: CompressAdminSingleLunPoll() bFound:1 Found AM Dest 6006016009702800:7E2109128CA9E011
    //componentName+ routineName                + context+ Msg                         
    //.--
    //*****************************************************************************
    void setContext(const char *str);

    //*****************************************************************************
    //.++
    //  FUNCTION:
    //     setContext
    //
    //  SUMMARY:
    //     Prepends the 2nd half of the LU ID to all traces
    //
    //  PARAMETERS:
    //     luId : LU Id to prepend to the traces.
    //
    //  FUNCTIONAL DESCRIPTION:
    //     See Summary
    //  
    //  RETURN VALUE
    //
    //  REMARKS
    //
    //  EXAMPLE
    //     COMPRESS: CompressAdminSingleLunPoll() B6C82B606EA9E012 Failed to allocate TLD TAG_KEY     
    //componentName+ routineName                + luId           + Msg                         
    //.--
    //*****************************************************************************
    void setContext(const K10_LU_ID &luId);

    //*****************************************************************************
    //.++
    //  FUNCTION:
    //     trace
    //
    //  SUMMARY:
    //     Generic printf style trace statement to trace at ERROR_LEVEL
    //
    //  PARAMETERS:
    //     fmt
    //
    //  FUNCTIONAL DESCRIPTION:
    //     See Summary
    //  
    //  RETURN VALUE
    //
    //  REMARKS
    //     None
    //.--
    //*****************************************************************************
    void trace(const char *fmt, ...) __attribute__((format(__printf_func__,2,3)));

    //*****************************************************************************
    //.++
    //  FUNCTION:
    //     trace
    //
    //  SUMMARY:
    //     Generic printf style trace statement to trace at the specified level
    //
    //  PARAMETERS:
    //     traceLevel : Trace level to trace at.
    //     fmt        : printf style format specifier.
    //
    //  FUNCTIONAL DESCRIPTION:
    //     See Summary
    //  
    //  RETURN VALUE
    //
    //  REMARKS
    //     None
    //.--
    //*****************************************************************************
    void trace(K10_TRACE_LEVEL traceLevel, const char *fmt, ...) __attribute__((format(__printf_func__,3,4)));

    //*****************************************************************************
    //.++
    //  FUNCTION:
    //     traceReturnVal
    //
    //  SUMMARY:
    //     Generic prinft style trace statement followed by the return value 
    //     provided in the constructor
    //
    //  PARAMETERS:
    //     fmt       : printf style format specifier.
    //
    //  FUNCTIONAL DESCRIPTION:
    //     See Summary
    //  
    //  RETURN VALUE
    //
    //  REMARKS
    //     
    //.--
    //*****************************************************************************
    void traceReturnVal(const char *fmt, ...) __attribute__((format(__printf_func__,2,3)));

    //*****************************************************************************
    //.++
    //  FUNCTION:
    //     traceBug
    //
    //  SUMMARY:
    //     Function to trace printf style statement for an unexpected event
    //
    //  PARAMETERS:
    //     fmt
    //
    //  FUNCTIONAL DESCRIPTION:
    //     Function adds a "Bug" before the provided trace msg. The entire trace 
    //     will be traced at error level.
    //  
    //  RETURN VALUE
    //
    //  REMARKS
    //     
    //.--
    //*****************************************************************************
    void traceBug(const char *fmt, ...) __attribute__((format(__printf_func__,2,3)));

    //*****************************************************************************
    //.++
    //  FUNCTION:
    //     enableElapsedTimeTracing
    //
    //  SUMMARY:
    //     Function to enable the elasped time in the function from the time
    //     the object was instantiated to the time the object goes out of scope.
    //     Note the trace is generated only if the time taken is longer than 
    //     the threshold.
    //
    //  PARAMETERS:
    //     timeThresholdMSecs : Threshold in milliseconds. A trace will be generated
    //     only if the elapsed time is longer than this threshold value.
    //     Valid values for timeThresholdMSec are >= 0
    //
    //  FUNCTIONAL DESCRIPTION:
    //     See Summary
    //  
    //  RETURN VALUE
    //
    //  REMARKS
    //     
    //.--
    //*****************************************************************************
    void enableElapsedTimeTracing(csx_u64_t timeThresholdMSecs = 0);

    //*****************************************************************************
    //.++
    //  FUNCTION:
    //     traceK10Exception
    //
    //  SUMMARY:
    //     Function to trace the various pieces of information in a K10Exception object
    //
    //  PARAMETERS:
    //     ke  : K10 Exception object to be traced.
    //
    //  FUNCTIONAL DESCRIPTION:
    //     See Summary
    //  
    //  RETURN VALUE
    //
    //  REMARKS
    //     None
    //.--
    //*****************************************************************************
    void traceK10Exception(K10Exception ke);

    //*****************************************************************************
    //.++
    //  FUNCTION:
    //     traceTldAllocationFailure
    //
    //  SUMMARY:
    //     Function to trace TLD allocation failure
    //
    //  PARAMETERS:
    //     tag  : Tag for the TLD that the allocation failure happened for.
    //
    //  FUNCTIONAL DESCRIPTION:
    //     See Summary
    //  
    //  RETURN VALUE
    //
    //  REMARKS
    //     None
    //.--
    //*****************************************************************************
    //void traceTldAllocationFailure(TLDtag tag);

    //*****************************************************************************
    //.++
    //  FUNCTION:
    //     traceWWN
    //
    //  SUMMARY:
    //     Function to traces msg followed by the luId
    //
    //  PARAMETERS:
    //     msg   : Pointer to a NULL terminated msg.
    //     luId  : LU ID
    //
    //  FUNCTIONAL DESCRIPTION:
    //     See Summary
    //  
    //  RETURN VALUE
    //
    //  REMARKS
    //     None
    //.--
    //*****************************************************************************
    void traceWWN(const char *msg, K10_LU_ID luId);

    //*****************************************************************************
    //.++
    //  FUNCTION:
    //     clearContext
    //
    //  SUMMARY:
    //     Function to clear the context.
    //
    //  PARAMETERS:
    //     None
    //
    //  FUNCTIONAL DESCRIPTION:
    //     See Summary
    //  
    //  RETURN VALUE
    //     None
    //
    //  REMARKS
    //     None
    //.--
    //*****************************************************************************
    void clearContext();

    //*****************************************************************************
    //.++
    //  FUNCTION:
    //     stripClassName
    //
    //  SUMMARY:
    //     Utility function to strip off the class name from routine name.
    //
    //  PARAMETERS:
    //     functionName
    //
    //  FUNCTIONAL DESCRIPTION:
    //     See Summary
    //  
    //  RETURN VALUE
    //     A pointer to the routine name.
    //
    //  REMARKS
    //     None
    //.--
    //*****************************************************************************
    static const char * stripClassName(const char *  functionName);


    //*****************************************************************************
    //.++
    //  FUNCTION:
    //      initializeIncidentNoteworthiness
    //
    //  SUMMARY:
    //      Initializes a noteworthiness object.
    //
    //  PARAMETERS:
    //      pNoteworthiness
    //          Client provides storage (opaque to client)
    //      NumInitialNoteworthyIncidents
    //          The number of incidents since the object was initialized
    //           or reset which will automatically be deemed noteworthy.  
    //      ModIntervalGrowth
    //          Additional incidents between <NumInitialNoteworthyIncidents> and
    //           this incident will be deemed non-noteworthy. 
    //      ModIntervalGrowth
    //          The factor by which the mod interval grows each time it is
    //           reached. The incident at which an interval is reached
    //           is deemed noteworthy, and all subsequent incidents 
    //           until the next such interval are deemed non-noteworthy.
    //      MaxModInterval
    //          The maximum value that the mod interval may reach.
    //
    //  RETURN VALUE
    //      None
    //.--
    //*****************************************************************************
    void initializeIncidentNoteworthiness (
        TRACE_UTIL_INCIDENT_NOTEWORTHINESS  *pNoteworthiness,
        ULONG64                             NumInitialNoteworthyIncidents,
        ULONG64                             InitialModValue,
        ULONG64                             ModIntervalGrowth,
        ULONG64                             MaxModInterval);

    //*****************************************************************************
    //.++
    //  FUNCTION:
    //      recordIncident
    //        Records a new incident in a noteworthiness object and returns
    //        information about that incident.
    //
    //  PARAMETERS:
    //      pNoteworthiness
    //          Address of a previously-initialized noteworthiness object.
    //      pIsNoteworthy
    //          Address of a flag that will be set to TRUE if this particular
    //           incident is noteworthy, else FALSE.
    //      pGapUntilNextNoteworthyIncident
    //          Will be filled in with the number of additional non-noteworthy incidents
    //           that must be recorded until one is deemed noteworthy.  
    //
    //  RETURN VALUE
    //      ULONG64
    //           The instance number (since the last time the object
    //           was reset) of the incident that was recorded.
    //.--
    //*****************************************************************************
    ULONG64 recordIncident (
        TRACE_UTIL_INCIDENT_NOTEWORTHINESS  *pNoteworthiness,
        bool                                *pIsNoteworthy,
        csx_u64_t                           *pGapUntilNextNoteworthyIncident);

private:
    K10TraceMsg    *pK10Trace;
    HRESULT        *pReturnValue;
    csx_u64_t       startTime;
    bool            elapsedTimeTracingEnabled;
    csx_u64_t       timeThresholdMSecs;
    bool            isKtraceCaptive;
    bool            isNoteworthy;
    bool            isIncidentNoteworthyValid;
    csx_u64_t       GapUntilNext;

    char RoutineNameBuffer[TRACE_UTIL_MAX_ROUTINE_NAME_LENGTH];
    char ContextBuffer[TRACE_UTIL_MAX_CONTEXT_LENGTH];
    void vTrace(K10_TRACE_LEVEL traceLevel, const char * fmt, va_list args);
    void setupRoutineTracing(const char *routineName, HRESULT *pInputRetVal);
    void initializeContext();
};
