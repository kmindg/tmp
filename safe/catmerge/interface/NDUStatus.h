//
// Copyright (C) 2000-2005	Data General Corporation
//
//
//
//	Revision History
//	----------------
//	?? ??  00	K. Owen		Initial version.
//	02 Oct 00	B. Yang		Task 1955 - log and trace inhibitor, Add constructor and destructor.
//	25 Jun 03	C. Hopkins	Added pragmas to prevent compiler warnings
//	24 Oct 05	R. Hicks	DIMS 126457 - allow to check BIOS/POST upgrade failures
//

#ifndef _NDUStatus_h
#define _NDUStatus_h

#include <windows.h>
#include "CaptivePointer.h"
#include "K10TraceMsg.h"
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT

// registry values used to record programmable's update revision

#define BIOSREV_VALUE   "UpdateBIOSRev"		// name of BIOS registry value
#define POSTREV_VALUE   "UpdatePOSTRev"		// name of POST registry value

// programmable names

#define BIOS_NAME       "BIOS"				// name of BIOS programmable
#define POST_NAME       "POST"				// name of POST programmable

// Registry setting to indicate if IO was intentionally inhibited.

#define INHIBIT_VALUE   "InhibitIO"

class CSX_CLASS_EXPORT NDUStatus
{
public:
	
	// Constructor and destructor

	NDUStatus() {	
			mpProc = NULL; // Initialization needed in non-Unicode
				       // case since mpProc is not a
				       // CaptivePointer in this variant.
			INIT_K10TRACE(mnpK, mpProc, GLOBALMANAGEMENT_STRING);
		}

	~NDUStatus() {}

    //      Returns true if the reboot driver has inhibited driver loading,
    //      false in normal operation.

    BOOL    InDegradedMode( void );

    //      Used by NDU to reset the reboot driver's boot count.

    void    ResetBootCount( void );

    //      Used by check scripts to report status to NDU prior to unquiescing
    //      the SP.  The caller is expected to have logged any problems prior
    //      to invoking this method.
    //
    //      AppName is the K10App name of the caller.  Set InhibitIO to true
    //      when the SP should not be allowed to service I/O requests.

    void    ReportStatus( char *AppName, BOOL InhibitIO );

    //      Used by NDU to determine check script status.

    BOOL    IOInhibited( void );

    //      Used by NDU to clear the check script status.

    void    ClearStatus( void );

    //      Used by NDU to determine the unquiesced status.

    BOOL    SPIsUnquiesced( void );

    //      Used by NDU to set the unquiesced status.

    void    SetUnquiescedStatus( BOOL Unquiesced );

    //      Used by NDULoadBios to record the revision of the programmable image 
    //      being installed

    int     SetUpdateRev( const char *pUpdateType, ULONG MajorRev, ULONG MinorRev );

    //      Used by NDU to get a programmables's update revision from registry

    int     GetUpdateRev(const char *pUpdateType, ULONG &ulMajorRev, ULONG &ulMinorRev);

    //      Used by NDU to get a programmable's current revision from Flare

    void    GetCurrentRev(const char *pProgName, ULONG &ulMajorRev, ULONG &ulMinorRev);

private:

#pragma warning(disable:4251)
	NPtr<K10TraceMsg>		mnpK;
#pragma warning(default:4251)

	char*					mpProc;

};

#endif // _NDUStatus_h

