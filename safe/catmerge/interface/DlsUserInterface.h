
#ifndef __DLS_USER_INTERFACE_H__
#define __DLS_USER_INTERFACE_H__

//***************************************************************************
// Copyright (C) EMC Corporation 1989-1999, 2009
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      DlsUserInterface.h
//
// Contents:
//      Device names and IOCTLs, and IN/OUT buffers to talk 
//      to the DLS Test Driver. The Test Driver understands
//      but three DLS operations: 
//
//          DLS_OpenLock()
//          DLS_ConvertLock()
//          DLS_CloseLock()
//          DLS_AdminCloseLock()
//          DLS_AdminShutdown()
//
//      User DLS does not understand or support mailbox
//      operations.  
//
// ConstantMacros:
//      DLSDRIVER_NT_DEVICE_NAME 
//      DOS_DLSDRIVER_DEVICE_NAME 
//      IOCTL_DLSDRIVER_OPEN_LOCK
//      IOCTL_DLSDRIVER_CONVERT_LOCK
//      IOCTL_DLSDRIVER_CLOSE_LOCK
//      IOCTL_DLSDRIVER_ADMIN_CLOSE
//      IOCTL_DLSDRIVER_ADMIN_SHUTDOWN 
//
//  Types:
//      DLSDRIVER_OPEN_IN_BUFFER
//      DLSDRIVER_OPEN_OUT_BUFFER
//      DLSDRIVER_CLOSE_IN_BUFFER
//      DLSDRIVER_CLOSE_OUT_BUFFER
//      DLSDRIVER_CONVERT_IN_BUFFER
//      DLSDRIVER_CONVERT_OUT_BUFFER
//      DLSDRIVER_ADMCLOSE_IN_BUFFER
//      DLSDRIVER_ADMCLOSE_OUT_BUFFER
//      DLSDRIVER_ADMIN_SHUTDOWN_IN_BUFFER
//      DLSDRIVER_ADMIN_SHUTDOWN_OUT_BUFFER
//
//
// Revision History:
//      28-Jun-01   CVaidya  Changes to support an admin close lock operation
//                           Code/Comments added to support this new IOCTL
//      05-Feb-99   MWagner  Created.
//
//--

//++
// This pragma turns off warninga about unused inline functions
// in system header files
//--

#ifdef __cplusplus
extern "C" {
#endif

//++
// Include files
//--

//++
// Interface
//--
#include "K10LayeredDefs.h"

//++
// HACK ALERT
//
// Include user_hacks.h to get the definition of NTSTATUS.  This allows the 
// use of DLS_LOCK_NAME, DLS_LOCK_MODE, and DLS_LOCK_HANDLE from 
// DlsTypes.h, but also allows this include file to be 
// included in user space.  It's here solely to tell the compiler to shut up.
//--
#include "k10ntstatus.h"
//++
// end HACK ALERT
//--

#include "DlsTypes.h"
//#include <devioctl.h>

//++
// End Includes
//--



//++
// Constant:
//      DLSDRIVER_NAME
//
// Description:
//      The "full" name of User DLS
//
// Revision History:
//      03-Jun-99   MWagner  Created.
//
//--
#define DLSDRIVER_NAME    "UserDistLockService"
//.End                                                                        

//++
// Constant:
//      DLSDRIVER_ABBREV
//
// Description:
//      The "abbreviated"  name of User DLS
//
// Revision History:
//      03-Jun-99   MWagner  Created.
//
//--
#define DLSDRIVER_ABBREV    "udls"
//.End                                                                        


//++
// Constant:
//      DLSDRIVER_ABBREV
//
// Description:
//      The wide "abbreviated"  name of User DLS
//
// Revision History:
//      03-Jun-99   MWagner  Created.
//
//--
#define DLSDRIVER_ABBREV    "udls"

//.End                                                                        

//++
// Constant:
//      DLSDRIVER_NT_DEVICE_NAME 
//
// Description:
//      The DLS Driver NT Device name
//
// Revision History:
//      03-Jun-99   MWagner  Created.
//
//--
#define DLSDRIVER_NT_DEVICE_NAME    K10_NT_DEVICE_PREFIX DLSDRIVER_NAME 
//.End                                                                        

//++
// Constant:
//      DLSDRIVER_WIN32_DEVICE_NAME   
//
// Description:
//      The DLS Driver Win32 Device name
//
// Revision History:
//      03-Jun-99   MWagner  Created.
//
//--
#define DLSDRIVER_WIN32_DEVICE_NAME      K10_WIN32_DEVICE_PREFIX  DLSDRIVER_ABBREV
//.End                                                                        

//++
// Constant:
//      DLSDRIVER_USER_DEVICE_NAME 
//
// Description:
//      The DLS Driver User Device name
//
// Revision History:
//      17-Feb-99   MWagner  Created.
//
//--
#define DLSDRIVER_USER_DEVICE_NAME      K10_USER_DEVICE_PREFIX  DLSDRIVER_ABBREV
//.End                                                                        

//++++++++
// IOCTLs
//--------

//++
// Macro:
//      DLS_BUILD_IOCTL
//
// Description:
//      Builds a DLS Ioctl code
//
//      winioctl.h or ntddk.h must be included before DlsIUserInterface.h
//      to pick up the CTL_CODE() macro.
//
//      Note that function codes 0-2047 are reserved for Microsoft 
//      Corporation, and 2048-4095 are reserved for customers.
//
// Arguments:
//      ULONG   Ioctl_Value
//
// Return Value
//      XXX:    A Dls Ioctl Code
//
// Revision History:
//      4/8/1999   MWagner    Created.
//
//--
#define DLS_BUILD_IOCTL( IoctlValue )   (     \
                                              \
        EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_K10_DLSDRIVER,   \
                 (IoctlValue) + 2048,         \
                 EMCPAL_IOCTL_METHOD_BUFFERED,             \
                 EMCPAL_IOCTL_FILE_ANY_ACCESS)             \
                                          )
// .End DLS_BUILD_IOCTL

//++
// Enum:
//      DLSDRIVER_OP
//
// Description:
//      The Dls Operations Implemented by the User Dls
//
// Members:
//
//      DlsDriverLockOpOpen     =  1    DistLockServiceOpenLock()
//      DlsDriverLockOpClose    =  2    DistLockServiceCloseLock()
//      DlsDriverLockOpConvert  =  3    DistLockServiceConvertLock()
//      DlsDriverLockOpAdmClose =  4    DistLockServiceAdmCloseLock()
//      DlsDriverAdminShutdown  =  5    DistLockServiceAdminShutdown()
//
// Revision History:
//      6/25/2001  CVaidya    Added an OP to support admin close operation
//      4/8/1999   MWagner    Created. 
//
//--
typedef enum _DLSDRIVER_OP
{
        DlsDriverLockOpOpen     =  1,
        DlsDriverLockOpClose    =  2,
        DlsDriverLockOpConvert  =  3,
        DlsDriverLockOpAdmClose =  4,
        DlsDriverAdminShutdown  =  5
} DLSDRIVER_OP, *PDLSDRIVER_OP;
//.End                                                                        

//++
// Constants:
//      IOCTL_DLSDRIVER_OPEN_LOCK     calls Dls_OpenLock()   
//      IOCTL_DLSDRIVER_CLOSE_LOCK    calls Dls_CloseLock()   
//      IOCTL_DLSDRIVER_CONVERT_LOCK  calls Dls_ConvertLock()   
//      IOCTL_DLSDRIVER_ADMIN_CLOSE   calls Dls_AdmCloseLock()
//      IOCTL_DLSDRIVER_ADMIN_SHUTDOWN calls Dls_AdminShutdown()
//
// Revision History:
//      6/25/2001  CVaidya    Added an IOCTL to support admin close operation
//      4/8/1999   MWagner    Created.
//
//--
#define IOCTL_DLSDRIVER_OPEN_LOCK      DLS_BUILD_IOCTL(DlsDriverLockOpOpen) 
#define IOCTL_DLSDRIVER_CLOSE_LOCK     DLS_BUILD_IOCTL(DlsDriverLockOpClose) 
#define IOCTL_DLSDRIVER_CONVERT_LOCK   DLS_BUILD_IOCTL(DlsDriverLockOpConvert) 
#define IOCTL_DLSDRIVER_ADMIN_CLOSE    DLS_BUILD_IOCTL(DlsDriverLockOpAdmClose)
#define IOCTL_DLSDRIVER_ADMIN_SHUTDOWN DLS_BUILD_IOCTL(DlsDriverAdminShutdown)

// .End DLS_IOCTLS

//+++++++++++++++
// IOCTL_BUFFERS
//
// n.b. "Dls_CurrentVersion" is included in DLS_Lock_Types_Exported.h
//
//       Buffer structure prefix names- All objections to these have been
//       voiced by Paul McGrath, and duly noted.  At any rate, here's how
//       the prefix for buffer members was generated:
//
//           d(o = Open, cl = Close, cn = Convert)(i = Input, o = Output)b
//
//       So the input buffer for a DlsDriver Convert has members that all
//       start with dncib.
//---------------

#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack(4)
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */
//++
// Type:
//      DLSDRIVER_OPEN_IN_BUFFER
//
// Description:
//      The input buffer for an OPEN IOCTL
//
// Members:
//      ULONG               doibVersion                dlsCurrentVersion
//      DLS_LOCK_NAME       doibName                   DLS_Lock_Types_Exported.h
//      ULONG               doibFailFastDelayInMinutes User DLS Client specified 
//                                                     Executioner timeout delay
// Revision History:
//      05-Apr-99   MWagner  Created.
//--
typedef struct _DLSDRIVER_OPEN_IN_BUFFER
{
        ULONG                 doibVersion;
        DLS_LOCK_NAME         doibName;
        ULONG                 doibFailFastDelayInMinutes;
} DLSDRIVER_OPEN_IN_BUFFER, *PDLSDRIVER_OPEN_IN_BUFFER;
//.End                                                                        

//++
// Type:
//      DLSDRIVER_OPEN_OUT_BUFFER
//
// Description:
//      The output buffer for an OPEN IOCTL
//
// Members:
//      ULONG                 doobVersion                DLS_Current_Version
//      UDLS_LOCK_HANDLE      doobLockHandle             the handle to be used
//                                                       in subsequent lock
//                                                       operations
//      ULONG                 doobStatus                 0x0 is success
//
// Revision History:
//      05-Apr-99   MWagner  Created.
//
//--
typedef struct _DLSDRIVER_OPEN_OUT_BUFFER
{
        ULONG               doobVersion;
        UDLS_LOCK_HANDLE    doobLockHandle;  
        ULONG               doobStatus;
} DLSDRIVER_OPEN_OUT_BUFFER, *PDLSDRIVER_OPEN_OUT_BUFFER;
//.End                                                                        
 
//++
// Type:
//      DLSDRIVER_CONVERT_IN_BUFFER
//
// Description:
//      The input buffer for a CONVERT IOCTL
//
//      DLS_LOCK_MODE is defined in DLS_Lock_Types_Exported.h
//
// Members:
//      ULONG            dcnibVersion        dlsCurrentVersion
//      UDLS_LOCK_HANDLE dcnibLockHandle     see DLS_Lock_Types_Exported.h
//      DLS_LOCK_MODE    dcnibLockMode       see DLS_Lock_Types_Exported.h
//
// Revision History:
//      05-Apr-99   MWagner  Created.
//
//--
typedef struct _DLSDRIVER_CONVERT_IN_BUFFER
{
        ULONG                 dcnibVersion;
        UDLS_LOCK_HANDLE      dcnibLockHandle;  
        DLS_LOCK_MODE         dcnibLockMode;  
} DLSDRIVER_CONVERT_IN_BUFFER, *PDLSDRIVER_CONVERT_IN_BUFFER;
//.End                                                                        


//++
// Type:
//      DLSDRIVER_CONVERT_OUT_BUFFER
//
// Description:
//      The output buffer for a CONVERT IOCTL
//
//
// Members:
//      ULONG           dcnobVersion        dlsCurrentVersion
//      DLS_LOCK_MODE   dcnobLockMode       see DLS_Lock_Types_Exported.h
//                                          for version 1, this mode should
//                                          always be the same as the requested
//                                          mode, for later versions this may
//                                          not be true
//      ULONG           dcnobStatus         0x0 is success
//      BOOLEAN         dcnobPeerWritten    if peer had the write lock before
//
// Revision History:
//      05-Apr-99   MWagner  Created.
//
//--
typedef struct _DLSDRIVER_CONVERT_OUT_BUFFER
{
        ULONG               dcnobVersion;
        DLS_LOCK_MODE       dcnobLockMode;  
        ULONG               dcnobStatus;
        BOOLEAN             dcnobPeerWritten;
} DLSDRIVER_CONVERT_OUT_BUFFER, *PDLSDRIVER_CONVERT_OUT_BUFFER;
//.End                                                                        


//++
// Type:
//      DLSDRIVER_CLOSE_IN_BUFFER
//
// Description:
//      The input buffer for an CLOSE IOCTL
//
// Members:
//      ULONG                     dclibVersion        dlsCurrentVersion
//      UDLS_LOCK_HANDLE          dclibLockHandle     see DLS_Lock_Types_Exported.h
//
// Revision History:
//      05-Apr-99   MWagner  Created.
//
//--
typedef struct _DLSDRIVER_CLOSE_IN_BUFFER
{
    ULONG               dclibVersion;
    UDLS_LOCK_HANDLE    dclibLockHandle;  
} DLSDRIVER_CLOSE_IN_BUFFER, *PDLSDRIVER_CLOSE_IN_BUFFER;
//.End                                                                        
 

//++
// Type:
//      DLSDRIVER_CLOSE_OUT_BUFFER
//
// Description:
//      The output buffer for an CLOSE IOCTL
//
// Members:
//      ULONG           dclobVersion        dlsCurrentVersion
//      ULONG           dclobStatus         0x0 is success
//
// Revision History:
//      05-Apr-99   MWagner  Created.
//
//--
typedef struct _DLSDRIVER_CLOSE_OUT_BUFFER
{
    ULONG               dclobVersion;
    ULONG               dclobStatus;
} DLSDRIVER_CLOSE_OUT_BUFFER, *PDLSDRIVER_CLOSE_OUT_BUFFER;
//.End

//++
// Type:
//      DLSDRIVER_ADMCLOSE_IN_BUFFER
//
// Description:
//      The input buffer for an ADMIN CLOSE IOCTL
//
// Members:
//      ULONG                     dacibVersion        dlsCurrentVersion
//      DLS_LOCK_NAME             dacibName           see DlsTypes.h
//
// Revision History:
//      25-Jun-01   CVaidya  Created.
//
//--
typedef struct _DLSDRIVER_ADMCLOSE_IN_BUFFER
{
    ULONG               dacibVersion;
    DLS_LOCK_NAME       dacibName;  
} DLSDRIVER_ADMCLOSE_IN_BUFFER, *PDLSDRIVER_ADMCLOSE_IN_BUFFER;
//.End                                                                        
 
//++
// Type:
//      DLSDRIVER_ADMCLOSE_OUT_BUFFER
//
// Description:
//      The output buffer for an ADMIN CLOSE IOCTL
//
// Members:
//      ULONG           dacobVersion        dlsCurrentVersion
//      ULONG           dacobStatus         0x0 is success
//
// Revision History:
//      25-Jun-01   CVaidya  Created.
//
//--
typedef struct _DLSDRIVER_ADMCLOSE_OUT_BUFFER
{
    ULONG               dacobVersion;
    ULONG               dacobStatus;
} DLSDRIVER_ADMCLOSE_OUT_BUFFER, *PDLSDRIVER_ADMCLOSE_OUT_BUFFER;
//.End                    

//++
// Type:
//      DLSDRIVER_ADMIN_SHUTDOWN_IN_BUFFER
//
// Description:
//      The input buffer for an SHUTDOWN IOCTL
//
// Members:
//      ULONG                     dasibVersion        dlsCurrentVersion
//
// Revision History:
//      25-Jun-01   CVaidya  Created.
//
//--
typedef struct _DLSDRIVER_ADMIN_SHUTDOWN_IN_BUFFER
{
    ULONG               dasibVersion;  
} DLSDRIVER_ADMIN_SHUTDOWN_IN_BUFFER, *PDLSDRIVER_ADMIN_SHUTDOWN_IN_BUFFER;
//.End                                                                        
 
//++
// Type:
//      DLSDRIVER_ADMIN_SHUTDOWN_OUT_BUFFER
//
// Description:
//      The output buffer for an SHUTDOWN IOCTL
//
// Members:
//      ULONG           dasobVersion        dlsCurrentVersion
//      ULONG           dasobStatus         0x0 is success
//
// Revision History:
//      25-Jun-01   CVaidya  Created.
//
//--
typedef struct _DLSDRIVER_ADMIN_SHUTDOWN_OUT_BUFFER
{
    ULONG               dasobVersion;
    ULONG               dasobStatus;
} DLSDRIVER_ADMIN_SHUTDOWN_OUT_BUFFER, *PDLSDRIVER_ADMIN_SHUTDOWN_OUT_BUFFER;
//.End                    
#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack()
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */

#ifdef __cplusplus
};
#endif
 
#endif // __DLS_USER_INTERFACE_H__
