
//++
//
//  File Name:
//      Status.h
//
//  Contents:
//      Status class definition.
//
//  Revision History:
//
//      ??-???-??   Karl Owen   Implemented.
//      15-Nov-01   Karl Owen   Commented.
//
//---

#include <windows.h>

#include "K10TraceMsg.h"

#ifndef _Status_h
#define _Status_h

#include "NDUXML.h"
#include "ToC.h"
#include "tld.hxx"
#include "K10Tags.h"
#include "k10defs.h"
#include "ndu_toc_common.h"
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT

CSX_MOD_EXPORT char * GetProgressStateString( int cur );

//++
//
//  Class:
//      HistoryEntry
//
//  Description:
//      This class implements the most basic element of a status history.
//
//  Revision History:
//      22-Aug-02   Brandon Myers  Created.
//
//--

class CSX_CLASS_EXPORT HistoryEntry
{
public:
    HistoryEntry( void );
    ~HistoryEntry( void );
    void    IncrementCompleted();
    void    FinishStep();
    void    SetLastUpdateTime();
    void    AddXml( NDUXmlOut & XmlFile );
    BOOL    LoadXml( NDUXmlIn & XmlFile );

    int            Order;
    int            Completed;
    int            Total;
    int            Progress;
    HistoryEntry * Next;

private:
    time_t         LastUpdateTime;
};

//++
//
//  Class:
//      PackageEntry
//
//  Description:
//      This class implements a highly simplified version of the SuiteInfo class.
//
//  Revision History:
//      23-Aug-02   Brandon Myers  Created.
//
//--

class CSX_CLASS_EXPORT PackageEntry
{
public:
    PackageEntry( void );
    ~PackageEntry( void );
    void    AddXml( NDUXmlOut & XmlFile );
    BOOL    LoadXml( NDUXmlIn & XmlFile );

    char *         Name;
    char *         Revision;
    int            Generation;
    bool           Transient;
    PackageEntry * Next;
};

//++
//
//  Class:
//      StatusImpl
//
//  Description:
//      This class manages the persistent status of the NDU
//      subsystem.  It is an opaque container to rest of NDU.
//
//  Revision History:
//      15-Nov-01   Karl Owen   Commented.
//		29-Jun-12	Eric Yao	Make it as implementation of Status Singleton class. see AR 494931
//
//--

class CSX_CLASS_EXPORT StatusImpl
{
public:
            StatusImpl( void );
            ~StatusImpl( void );

            //  The status class maintains three values:
            //
            //  op      - Current (or most recent) operation
            //  pend    - Non zero if an NDU is currently in progress
            //  cur     - Current phase (if an NDU is in progress),
            //            result of most recent NDU if not.

            //  Get the values.

    void    get( int *op, int *pend, int *cur, bool NonBlocking=false, bool NonCache=true );

            //  Set the values.

    void    set( int op, int pend, int cur, bool flush=true );

    int     SetNDUState( int Progress, UINT_16E TargetBundleCompatLevel, UINT_16E PrimarySPCompatLevel=CURRENT_FLARE_COMMIT_LEVEL );
    int     DisablePowerSaving( UINT_16E TargetBundleCompatLevel );
    int     EnablePowerSaving();

    void    AddStatusXml( NDUXmlOut & XmlFile );
    void    AddHistoryXml( NDUXmlOut & XmlFile );
    void    AddPackagesXml( NDUXmlOut & XmlFile );

    BOOL    LoadStatusXml( NDUXmlIn & XmlFile );
    BOOL    LoadHistoryXml( NDUXmlIn & XmlFile );
    BOOL    LoadPackagesXml( NDUXmlIn & XmlFile );
    int     LoadHistory();
    int     LoadPackages();

    BOOL    NDUInProgress( void );
    BOOL    NDUInProgressNonBlocking( void );
    BOOL    LocalMaster( void );

    void    SetPackages( Entry * List );
    BOOL    IsTransient();
    void    Estimate( int Operation, int RebootRequired, int QuiesceRequired, int Degraded,
                      int ThisSPOnly, Entry * Install, Entry * Deactivate, Entry * Activate,
                      Entry * Uninstall ); // Obsolete interface.
    void    Estimate( int Operation, int RebootRequired, int QuiesceRequired, int Degraded,
                      int ThisSPOnly, int DMPDelay, int Disruptive,
                      Entry * Install, Entry * Deactivate, Entry * Activate, Entry * Uninstall );  // Another obsolete interface.
    void    Estimate( int Operation, int RebootRequired, int QuiesceRequired, int DisableCacheRequired, int Degraded,
                      int ThisSPOnly, int DMPDelay, int Disruptive,
                      Entry * Install, Entry * Deactivate, Entry * Activate, Entry * Uninstall );
    TLD *   GenerateHistoryInfo( BOOL AdminObjects, TLD *pOut );
    int     GetCurrentSubstep( int Progress );
    int     GetNDUCounter();
    void    IncrementNDUCounter();
    int     GetCurrentArrayTime(void);
    int     GetGlobalCommonAttributesAsInteger( char * Key );
    int     SetGlobalCommonAttributesAsInteger( char * Key, int Data );
    bool    GetGlobalCommonAttributesAsBool( char * Key );
    int     SetGlobalCommonAttributesAsBool( char * Key, bool Data );
    int     GetNDUStartTime(void);
    int     GetNDUFinishTime(void);
    char *  GetOperationString( int op );
    char *  GetPendingString( int pend );
    UINT_16E GetCommitLevel();
    void    SetSpCollectRequired( ULONG engine );

    BOOL    SyncTime(void);
    BOOL    AddEvent(char * Type, char * SubType, char * Description);
    
private:
    int     SaveHistory();
    void    DeleteHistory();
    int     SavePackages();
    void    DeletePackages();
    void    AddOpenEntry( int Total, int Progres );
    int     SetNDUCounter( int NDUOperationCounter );
    int     SetNDUStartTime( char SPID );
    int     SetNDUFinishTime(void);
    void    InitializeNDUData( char SPID );

    int     operation;
    int     pending;
    int     status;
    int     Initializing;
    PackageEntry * Packages;
    HistoryEntry * Open;
    HistoryEntry * Closed;

    K10TraceMsg  * trace;

	//NoCachedStatus flag to determine whether we have initialized the op, pend, cur value from psm or not
	bool	NoCachedStatus;
};

//++
//
//  Class:
//      Status
//
//  Description:
//      This class manages the persistent status of the NDU
//      subsystem.  It is an opaque container to rest of NDU.
//
//  Revision History:
//      15-Nov-01   Karl Owen   Commented.
//		29-Jun-12	Eric Yao	Make Status as a Singleton class. see AR 494931
//
//--

class CSX_CLASS_EXPORT Status
{
public:
            Status( void );
            ~Status( void );

            //  The status class maintains three values:
            //
            //  op      - Current (or most recent) operation
            //  pend    - Non zero if an NDU is currently in progress
            //  cur     - Current phase (if an NDU is in progress),
            //            result of most recent NDU if not.

            //  Get the values.

    void    get( int *op, int *pend, int *cur, bool NonBlocking=false, bool NonCache=true);

            //  Set the values.

    void    set( int op, int pend, int cur, bool flush = true );

    int     SetNDUState( int Progress, UINT_16E TargetBundleCompatLevel, UINT_16E PrimarySPCompatLevel=CURRENT_FLARE_COMMIT_LEVEL );
    int     DisablePowerSaving( UINT_16E TargetBundleCompatLevel );
    int     EnablePowerSaving();

    void    AddStatusXml( NDUXmlOut & XmlFile );
    void    AddHistoryXml( NDUXmlOut & XmlFile );
    void    AddPackagesXml( NDUXmlOut & XmlFile );

    BOOL    LoadStatusXml( NDUXmlIn & XmlFile );
    BOOL    LoadHistoryXml( NDUXmlIn & XmlFile );
    BOOL    LoadPackagesXml( NDUXmlIn & XmlFile );
    int     LoadHistory();
    int     LoadPackages();

    BOOL    NDUInProgress( void );
    BOOL    NDUInProgressNonBlocking( void );
    BOOL    LocalMaster( void );

    void    SetPackages( Entry * List );
    BOOL    IsTransient();
    void    Estimate( int Operation, int RebootRequired, int QuiesceRequired, int Degraded,
                      int ThisSPOnly, Entry * Install, Entry * Deactivate, Entry * Activate,
                      Entry * Uninstall ); // Obsolete interface.
    void    Estimate( int Operation, int RebootRequired, int QuiesceRequired, int Degraded,
                      int ThisSPOnly, int DMPDelay, int Disruptive,
                      Entry * Install, Entry * Deactivate, Entry * Activate, Entry * Uninstall );  // Another obsolete interface.
    void    Estimate( int Operation, int RebootRequired, int QuiesceRequired, int DisableCacheRequired, int Degraded,
                      int ThisSPOnly, int DMPDelay, int Disruptive,
                      Entry * Install, Entry * Deactivate, Entry * Activate, Entry * Uninstall );
    TLD *   GenerateHistoryInfo( BOOL AdminObjects, TLD *pOut );
    int     GetCurrentSubstep( int Progress );
    int     GetNDUCounter();
    void    IncrementNDUCounter();
    int     GetCurrentArrayTime(void);
    int     GetGlobalCommonAttributesAsInteger( char * Key );
    int     SetGlobalCommonAttributesAsInteger( char * Key, int Data );
    bool    GetGlobalCommonAttributesAsBool( char * Key );
    int     SetGlobalCommonAttributesAsBool( char * Key, bool Data );
    int     GetNDUStartTime(void);
    int     GetNDUFinishTime(void);
    char *  GetOperationString( int op );
    char *  GetPendingString( int pend );
    UINT_16E GetCommitLevel();
    void    SetSpCollectRequired( ULONG engine );

    BOOL    SyncTime(void);
    BOOL    AddEvent(char * Type, char * SubType, char * Description);
    
private:
    //  Don't try to share the implementation class among all instances, as
    //  there are many complicated locking issues that will need to be handled
    //  first.  See AR 529832.
    StatusImpl *	pImpl;
};

#endif // _Status_h
