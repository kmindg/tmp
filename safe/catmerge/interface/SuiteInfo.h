
//++
//
//  File Name:
//      SuiteInfo.h
//
//  Contents:
//      SuiteInfo class definition.
//
//  Revision History:
//
//      ??-???-??   Karl Owen   Implemented.
//      08-Nov-01   Karl Owen   Commented.
//
//--

#ifndef _SuiteInfo_H
#define _SuiteInfo_H

#include "K10TraceMsg.h"
#include "attributes.h"
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT

//++
//
//  Class:
//      Dependency
//
//  Description:
//      A container class which represents NDU dependencies.
//
//  Revision History:
//      08-Nov-01   Karl Owen   Changed from struct to class.
//
//--

class CSX_CLASS_EXPORT Dependency
{
public:
                Dependency( char * InitialName = NULL, char * InitialRevision = NULL );
                ~Dependency();
    char        *GetName();
    char        *GetRevision();
    BOOL        IsPermit();
    BOOL        IsSatisfied();
    Dependency  *GetNext();
    void        SetName( const char * NewName );
    void        SetRevision( const char * NewRevision );
    void        SetSatisfied( BOOL NewSatisfied );
    void        SetNext( Dependency * NewNext );

private:
    char        *name;
    char        *revision;
    BOOL         satisfied;
    Dependency  *next;
};

//++
//
//  Class:
//      AdminLibEntry
//
//  Description:
//      A container class which represents admin libraries.
//
//  Revision History:
//      08-Nov-01   Karl Owen   Changed from struct to class.
//
//--

class CSX_CLASS_EXPORT AdminLibEntry
{
public:
                    AdminLibEntry();
                    ~AdminLibEntry();
    BOOL            IsEnabled( K10TraceMsg * trace );
    long            GetCompatibilityLevel( short & Level, K10TraceMsg * trace );
    void            GetTargetString( char * TargetBuffer, ULONG BufferSize );

    char            *Name;
    int             DBID;
    AdminLibEntry   *Next;
};

//++
//
//  Class:
//      ProcessEntry
//
//  Description:
//      A container class which represents proccesses.
//
//  Revision History:
//      31-Jan-02   Brandon Myers   Created.
//
//--

class CSX_CLASS_EXPORT ProcessEntry
{
public:
                    ProcessEntry();
                    ~ProcessEntry();

    char            *Name;
    ProcessEntry    *Next;
};

//++
//
//  Class:
//      SuiteInfo
//
//  Description:
//      A container class which describes an NDU suite.  Many of the
//      attributes are extracted from the TOC.txt file, while others
//      are calculated (or maintained by external classes).
//
//  Revision History:
//      08-Nov-01   Karl Owen   Commented.
//      23-May-02   Karl Owen   Added display_revision member.
//      05-Aug-02   Karl Owen   Added display_name member.
//
//--

class CSX_CLASS_EXPORT SuiteInfo
{
public:

            //  Class constructor.
            //
            //  path  - Path to a file.
            //  isUPF - If true, <path> points to an NDU UPF, and TOC.txt
            //          must be extracted from the UPF.  If false, <path>
            //          points directly to the directory where the TOC.txt
            //          can be found.

            SuiteInfo( char *path, BOOL isUPF = false, BOOL inProgress = false );

            //  Class destructor.

            ~SuiteInfo( void );

            //  Returns the number of objects in the chain beginning with
            //  <this>.

    int     Count( void );

    int     HasName( char * Name );
    BOOL    CanUpgrade( void );
    void    GetSyncFlags( AttributeTable & GlobalAttributes, BOOL & RebootRequired, BOOL & QuiesceRequired, BOOL & CacheDisableRequired, BOOL & TerminateSelf, BOOL & RefreshConfiguration );
    void    GetOperationFlags( AttributeTable & GlobalAttributes, BOOL InitialInstall, BOOL & RebootRequired, BOOL & QuiesceRequired, BOOL & CacheDisableRequired, BOOL & TerminateSelf, BOOL & RefreshConfiguration, int Opcode );
    DWORD   GetMaximumCacheSize( BOOL UseReducedCacheSize );
    BOOL    GetUseReducedCacheSize();
    csx_s32_t   GetCacheRevision();
    BOOL    GetRefreshConfigurationRequired();
    BOOL    GetCausesConfigurationUpdate();

    //  The following data members could be considered private, but this is
    //  a container class and we expect other classes to reach in directly
    //  to the members.

// private:
    char            *name;
    char            *revision;
    char            *display_name;
    char            *display_revision;
    char            *vendor;
    char            *description;
    char            *license;
    char            *psmname;
    char            *Operation;

    BOOL            NDUInProgress;

    BOOL            commit_required;
    BOOL            suite_complete;
    BOOL            upgrade_reboot_required;
    BOOL            install_reboot_required;
    BOOL            uninstall_reboot_required;
    BOOL            post_activate_required;
    BOOL            has_post_activate_script;
    BOOL            system_software;
    BOOL            disruptive;
    BOOL            revert_available;
    BOOL            active;
    BOOL            installable;
    BOOL            uninstallable;
    BOOL            hidden;
    BOOL            terminate_self;
    BOOL            quiesce_required;
    BOOL            cache_disable_required;
    BOOL            transient;
    BOOL            ephemeral;

    int             generation;

    BOOL            error_already_installed;
    BOOL            error_dependency_not_met;
    BOOL            error_breaks_dependency;
    BOOL            error_permit_not_met;
    BOOL            error_breaks_permit;    
    BOOL            error_duplicate;
    BOOL            error_commit_required;
    BOOL            error_downgrade_denied;
    BOOL            error_invalid_upf;
    BOOL            error_ephemeral_upgrade_denied;
    BOOL            error_reimage_required;

    AttributeTable  Attributes;

    Dependency      *dependencies;
    Dependency      *GenerationDependencies;
    Dependency      *AntiDependencies;
    Dependency      *AttributeDependencies;
    Dependency      *AttributeIncompatibles;
    AdminLibEntry   *CommitAdminLibs;         // the list of the admin libraries that support COMMIT opcode
    AdminLibEntry   *RefreshConfigAdminLibs;  // the list of the admin libraries that support REFRESH_CONFIGURATION opcode
    ProcessEntry    *Processes;
    ProcessEntry    *Obsoleted;
    Dependency      *Implies;
    Dependency      *Predecessor;
    Dependency      *FirmwareBinaries;
    SuiteInfo       *next;

    //  Used to detect memory leaks.

    static int  AllocCount;

private:
    K10TraceMsg     *trace;
    void            Populate( char *path );
    void            AddXmlData( char *path );
    BOOL            IsRebootRequired( AttributeTable & GlobalAttributes, char * OverrideAttribute, BOOL DefaultValue );

    BOOL            refresh_configuration_required;
    BOOL            causes_configuration_update;
    BOOL            use_reduced_cache_size;
};

#endif // _SuiteInfo_H

