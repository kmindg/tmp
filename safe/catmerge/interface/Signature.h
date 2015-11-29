// Copyright (C) 2010	EMC Corporation
//++
//
//  File Name:
//      Signature.h
//
//  Contents:
//      Signature class definition.
//
//  Revision History:
//
//      ??-???-??   Karl Owen   Implemented.
//      28-Nov-01   Abhi Commented.
//      18-Apr-08   Victor Kan  Added IPv6 support.  DIMS 195580.
//      20-Feb-09   Victor Kan  Added SNMP support.  DIMS 197085.
//      03-Apr-09   D Hesi      Modified LoadSNMPSettings() from private to public method for 
//                              SNMP persistence support.  DIMS 197085.
//      30-Apr-10   P. Caldwell  Added Speed/Duplex members & methods.  AR 323083.
//
//--

#ifndef _Signature_h
#define _Signature_h

#include "netidparamsDef.h"
//#include <windef.h>

#include <map>
#include <string>

// Note:  DUPLEX_MODE is also defined in NetworkAdminTypes.h.  Keep these two in sync.
#ifndef NETWORK_DUPLEX_MODE
#define NETWORK_DUPLEX_MODE		// So it doesn't get defined twice.

// enum copied from cm_environ_exports.h / MGMT_PORT_DUPLEX_MODE
enum DUPLEX_MODE
{
    DUPLEX_MODE_HALF               = 0,
    DUPLEX_MODE_FULL               = 1,
    DUPLEX_MODE_UNSPECIFIED        = 252,  //  For Write;  
    DUPLEX_MODE_INVALID            = 253,
    DUPLEX_MODE_INDETERMIN         = 254,  //  For Read.
    DUPLEX_MODE_NA                 = 255
};

#endif

//++
//
//  Class:
//      Signature
//
//  Description:
//      This class is used both to save the signature of an SP in PSM and to
//      also save and restore network settings if the signature changes.
//
//  Revision History:
//      28-Nov-01   Abhi  Commented.
//      18-Apr-08   Victor Kan  Use the new MGMT_PORT_DATA structure
//                              exported by admin to manage IPv4 and IPv6
//                              data together.
//
//--

#pragma warning(disable:4251) //  Use of stl in an exported class.

#ifndef ALAMOSA_WINDOWS_ENV
typedef std::map<std::string,DWORD> community_name_map;
#else
typedef __declspec(dllexport) std::map<std::string,DWORD> community_name_map;
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT */

class CSX_CLASS_EXPORT Signature
{
public:
                    Signature( K10TraceMsg *trace );
                    ~Signature( void );
    unsigned long   Get( unsigned long engine, BOOL & ForceRestore );
    int             RestoreNetworkRegistry( BOOL RestoreHostName );
    int             Set( unsigned long engine,
                                    unsigned long sig );
    int             LoadNetworkRegistry();
    void            Dump();

    void            SetNetwork( PMGMT_PORT_DATA mgmt_port_cfg );
    void            SetRequestedDuplex( DUPLEX_MODE Mode );
    void            SetRequestedSpeed( unsigned long RequestedSpeed );
    DUPLEX_MODE     GetRequestedDuplex( );
    unsigned long   GetRequestedSpeed( );
    DWORD           RestoreSNMPSettings();
    DWORD           LoadSNMPSettings();

private:
    unsigned long   SPSignature;

    MGMT_PORT_DATA  ManagementPortConfig;
    // This is the new data type used for combined IPv4 and IPv6 admin reads,
    // so we might as well use it to hold the same data for our Signature
    // object's management port network config data.

    DWORD              SNMPServiceStartValue;

    community_name_map SNMPCommunities;

    DWORD              ConfigAndManageService( char* service_name, DWORD start_value );
    DWORD              SNMPCommunityMapFromRegistry( community_name_map& map );
    unsigned long      RequestedSpeed;
    DUPLEX_MODE        RequestedDuplex;
    K10TraceMsg        *Trace;
};

#endif // _Signature_h

