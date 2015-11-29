#ifndef EMCUTIL_H_
#define EMCUTIL_H_

//***************************************************************************
// Copyright (C) EMC Corporation 2011
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

/*! @file  EmcUTIL.h 
 *  @brief The main header file for the EmcUTIL portion of the platform abstraction layer, included by clients that link against EmcUTIL.
 */

////////////////////////////////////

#include "csx_ext.h"

////////////////////////////////////

//
// select which backend the EmcUTIL APIs use
//
//    WU - Windows User Space APIs
//    WK - Windows Kernel Space APIs
//    CSX - CSX APIs
//
// When the WU or WK backends are selected, there must be no dependency on CSX APIs
//

#if !defined(ALAMOSA_WINDOWS_ENV) && defined(SIMMODE_ENV)

/*! @brief non-Windows case SIMMODE - use CSX per-container registry
 */
#define EMCUTIL_CONFIG_BACKEND_CSX
#define EMCUTIL_REG_IC_ID CSX_IC_ID_SELF

#elif !defined(ALAMOSA_WINDOWS_ENV) && defined(UMODE_ENV)

/*! @brief non-Windows case UMODE - use CSX helper provided single registry
 */
#define EMCUTIL_CONFIG_BACKEND_CSX
#define EMCUTIL_REG_IC_ID CSX_IC_ID_HELPER

#elif !defined(ALAMOSA_WINDOWS_ENV)

/*! @brief non-Windows case KERNEL - use CSX helper provided single registry
 */
#define EMCUTIL_CONFIG_BACKEND_CSX
#define EMCUTIL_REG_IC_ID CSX_IC_ID_HELPER

#elif defined(SIMMODE_ENV)
#include "EmcPAL.h"
/* @brief Windows CLARiiON SIMMODE case, use CSX per-container registry
 */
#define EMCUTIL_CONFIG_BACKEND_CSX
#define EMCUTIL_REG_IC_ID CSX_IC_ID_SELF

#elif defined(UMODE_ENV)
#include <winerror.h>
#include "EmcPAL.h"
/*! @brief Windows CLARiiON UMODE case - use Registry directly
 */
#define EMCUTIL_CONFIG_BACKEND_WU

#else
#include "EmcPAL.h"
/*! @brief Windows CLARiiON KERNEL case - use Registry directly
 */
#define EMCUTIL_CONFIG_BACKEND_WK
#endif

CSX_CDECLS_BEGIN

////////////////////////////////////

// lay some groundwork for Normal/Unicode APIs

#if defined(_UNICODE) || defined(UNICODE)
#define EMCUTIL_UNICODE_IN_USE
#endif

/*! @addtogroup emcutil_misc
 *  @{
 *
 */

/*! @brief EMCUTIL_T macro */
#ifdef EMCUTIL_UNICODE_IN_USE
#define EMCUTIL_T(_string) L ## _string
#else
#define EMCUTIL_T(_string) _string
#endif

////////////////////////////////////

// lay some groundwork to allow use of normal size_t based sizes, or ULONG-style sizes

/*! @brief EMCUTIL Size */
#ifndef EMCUTIL_USE_SIZE_T_STYLE_SIZES
#define EMCUTIL_SIZE_T csx_wulong_t
#else
#define EMCUTIL_SIZE_T csx_size_t
#endif
/*! @brief EMCUTIL Sizeof macro */
#define EMCUTIL_SIZEOF(_t) (EMCUTIL_SIZE_T)sizeof(_t)

////////////////////////////////////

/*! @} END emcutil_misc */


/*! @addtogroup emcutil_status
 *  @{
 *
 */

#if defined(EMCUTIL_CONFIG_BACKEND_WU)
#include <winerror.h>
typedef csx_wulong_t EMCUTIL_STATUS;								/*!< EmcUTIL Status type */
#define EMCUTIL_STATUS_OK 0											/*!< Status: Success */
#define EMCUTIL_STATUS_UNSUCCESSFUL ERROR_GEN_FAILURE				/*!< Status: Failure */
#define EMCUTIL_STATUS_NAME_NOT_FOUND ERROR_FILE_NOT_FOUND			/*!< Status: Name not found */
#define EMCUTIL_STATUS_NO_RESOURCES ERROR_NO_SYSTEM_RESOURCES		/*!< Status: Insufficient resources */
#define EMCUTIL_STATUS_INVALID_PARAMETER ERROR_INVALID_PARAMETER	/*!< Status: Invalid parameter */
#define EMCUTIL_STATUS_MORE_DATA ERROR_MORE_DATA					/*!< Status: More data available */
#define EMCUTIL_STATUS_NO_MORE_ENTRIES ERROR_NO_MORE_ITEMS			/*!< Status: No more entries */
#define EMCUTIL_STATUS_INTERNAL_ERROR ERROR_GEN_FAILURE				/*!< Status: Internal error */
#define EMCUTIL_STATUS_NOT_IMPLEMENTED ERROR_CALL_NOT_IMPLEMENTED	/*!< Status: Not implemented */
#define EMCUTIL_STATUS_ACCESS_DENIED ERROR_ACCESS_DENIED			/*!< Status: Access denied */
#define EMCUTIL_STATUS_ALREADY_EXISTS ERROR_ALREADY_EXISTS			/*!< Status: Name already exists */
#define EMCUTIL_STATUS_INVALID_STATE  ERROR_BAD_ENVIRONMENT			/*!< Status: Invalid state */
#define EMCUTIL_STATUS_DEVICE_DOES_NOT_EXIST ERROR_DEV_NOT_EXIST	/*!< Status: Device does not exist */
#define EMCUTIL_STATUS_TIMEOUT ((EMCUTIL_STATUS)258)				/*!< Status: Timeout */

#elif defined(EMCUTIL_CONFIG_BACKEND_WK)
typedef NTSTATUS EMCUTIL_STATUS;									/*!< EmcUTIL Status type */
#define EMCUTIL_STATUS_OK 0											/*!< Status: Success */
#define EMCUTIL_STATUS_UNSUCCESSFUL STATUS_UNSUCCESSFUL				/*!< Status: Failure */
#define EMCUTIL_STATUS_NAME_NOT_FOUND STATUS_OBJECT_NAME_NOT_FOUND	/*!< Status: Name not found */
#define EMCUTIL_STATUS_NO_RESOURCES STATUS_INSUFFICIENT_RESOURCES	/*!< Status: Insufficient resources */
#define EMCUTIL_STATUS_INVALID_PARAMETER STATUS_INVALID_PARAMETER	/*!< Status: Invalid parameter */
#define EMCUTIL_STATUS_MORE_DATA STATUS_BUFFER_OVERFLOW				/*!< Status: More data available */
#define EMCUTIL_STATUS_NO_MORE_ENTRIES STATUS_NO_MORE_ENTRIES		/*!< Status: No more entries */
#define EMCUTIL_STATUS_INTERNAL_ERROR STATUS_INTERNAL_ERROR			/*!< Status: Internal error */
#define EMCUTIL_STATUS_NOT_IMPLEMENTED STATUS_NOT_IMPLEMENTED		/*!< Status: Not implemented */
#define EMCUTIL_STATUS_ACCESS_DENIED STATUS_ACCESS_DENIED			/*!< Status: Access denied */
#define EMCUTIL_STATUS_ALREADY_EXISTS STATUS_OBJECT_NAME_EXISTS		/*!< Status: Name already exists */
#define EMCUTIL_STATUS_INVALID_STATE  STATUS_INVALID_DEVICE_STATE   /*!< Status: Invalid state */
#define EMCUTIL_STATUS_DEVICE_DOES_NOT_EXIST STATUS_DEVICE_DOES_NOT_EXIST	/*!< Status: Device does not exist */
#define EMCUTIL_STATUS_TIMEOUT STATUS_TIMEOUT						/*!< Status: Timeout */

#elif defined(EMCUTIL_CONFIG_BACKEND_CSX)
typedef csx_status_e EMCUTIL_STATUS;								/*!< EmcUTIL Status type */
#define EMCUTIL_STATUS_OK CSX_STATUS_SUCCESS						/*!< Status: Success */
#define EMCUTIL_STATUS_UNSUCCESSFUL CSX_STATUS_GENERAL_FAILURE		/*!< Status: Failure */
#define EMCUTIL_STATUS_NAME_NOT_FOUND CSX_STATUS_OBJECT_NOT_FOUND	/*!< Status: Name not found */
#define EMCUTIL_STATUS_NO_RESOURCES CSX_STATUS_NO_RESOURCES			/*!< Status: Insufficient resources */
#define EMCUTIL_STATUS_INVALID_PARAMETER CSX_STATUS_INVALID_PARAMETER /*!< Status: Invalid parameter */
#define EMCUTIL_STATUS_MORE_DATA CSX_STATUS_BUFFER_OVERFLOW			/*!< Status: More data available */
#define EMCUTIL_STATUS_NO_MORE_ENTRIES CSX_STATUS_OBJECT_NOT_FOUND	/*!< Status: No more entries */
#define EMCUTIL_STATUS_INTERNAL_ERROR CSX_STATUS_PROTOCOL_ERROR		/*!< Status: Internal error */
#define EMCUTIL_STATUS_NOT_IMPLEMENTED CSX_STATUS_NOT_IMPLEMENTED	/*!< Status: Not implemented */
#define EMCUTIL_STATUS_ACCESS_DENIED CSX_STATUS_NO_PERMISSIONS		/*!< Status: Access denied */
#define EMCUTIL_STATUS_ALREADY_EXISTS CSX_STATUS_OBJECT_ALREADY_EXISTS /*!< Status: Name already exists */
#define EMCUTIL_STATUS_INVALID_STATE  CSX_STATUS_INVALID_STATE /*!< Status: Invalid state */
#define EMCUTIL_STATUS_DEVICE_DOES_NOT_EXIST EMCPAL_STATUS_DEVICE_DOES_NOT_EXIST	/*!< Status: Device does not exist */
#define EMCUTIL_STATUS_TIMEOUT CSX_STATUS_TIMEOUT					/*!< Status: Timeout */

#else
#error "Error: bad EMCUTIL_CONFIG configuration..."
#endif

/*! @brief EMCUTIL Failure test */
#define EMCUTIL_FAILURE(_s) ((_s) != EMCUTIL_STATUS_OK)
/*! @brief EMCUTIL Success test */
#define EMCUTIL_SUCCESS(_s) ((_s) == EMCUTIL_STATUS_OK)

/*! @} END emcutil_status */


////////////////////////////////////

// some groundwork for the API interfaces

#define EMCUTIL_API   CSX_GLOBAL		/*!< EMCUTIL API */
#define EMCUTIL_LOCAL CSX_LOCAL			/*!< EMCUTIL Local */
#define EMCUTIL_CC    __cdecl			/*!< EMCUTIL CC */

CSX_CDECLS_END

////////////////////////////////////

// include the major API interfaces

#include "EmcUTIL_Device.h"
#include "EmcUTIL_Network.h"
#include "EmcUTIL_Misc.h"
#include "EmcUTIL_Registry.h"
#include "EmcUTIL_EventLog.h"
#include "EmcUTIL_SelfTest.h"
#include "EmcUTIL_Time.h"

////////////////////////////////////

#if defined(__cplusplus) && defined(UMODE_ENV)

#ifndef ALAMOSA_WINDOWS_ENV
#define EMCUTIL_DECLARE_THREAD_IMPERSONATION_GUARD \
     ImpersonationGuard impersonationGuard_;       \
     static_cast< void >( impersonationGuard_ );

class ImpersonationGuard 
{
  public:
    ImpersonationGuard()
    {
      CSX_CALL_ENTER_STATIC();
    }

    ~ImpersonationGuard() throw()
    {
      CSX_CALL_LEAVE_STATIC();    
    }

  private:
    ImpersonationGuard(const ImpersonationGuard& ref);
    ImpersonationGuard& operator= (const ImpersonationGuard& ref);

  private:
    CSX_CALL_CONTEXT_STATIC;
};

#else
#  define EMCUTIL_DECLARE_THREAD_IMPERSONATION_GUARD
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - thread context issues */

#endif // defined(__cplusplus) && defined(UMODE_ENV)

////////////////////////////////////

#ifdef ALAMOSA_WINDOWS_ENV
#define EMCUTIL_MAX_PATH MAX_PATH
#define EMCUTIL_MAX_FILENAME MAXIMUM_FILENAME_LENGTH
#else
#if (defined(UMODE_ENV) || defined(SIMMODE_ENV)) && !defined(SAFE_NOSTDINC_IN_USE) && !defined(SAFE_AVOID_PULLING_IN_C_RUNTIMES)
#include <limits.h>
#define EMCUTIL_MAX_PATH PATH_MAX
#define EMCUTIL_MAX_FILENAME NAME_MAX
#else
#define EMCUTIL_MAX_PATH 260
#define EMCUTIL_MAX_FILENAME 255
#endif
#endif

////////////////////////////////////

// get last error from system CRT - only has meaning for actual system-supplied standard CRT APIs like malloc() etc...
#ifdef ALAMOSA_WINDOWS_ENV
#define EmcutilLastCrtError() GetLastError()
#else
#define EmcutilLastCrtError() errno
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT */

////////////////////////////////////

//define EMCUTIL_SYS_ERROR_SUCCESS              EMCUTIL_STATUS_OK
//define EMCUTIL_SYS_ERROR_ACCESS_DENIED        EMCUTIL_STATUS_ACCESS_DENIED
//define EMCUTIL_SYS_ERROR_NO_SYSTEM_RESOURCES  EMCUTIL_STATUS_NO_RESOURCES
//define EMCUTIL_SYS_ERROR_INVALID_PARAMETER    EMCUTIL_STATUS_INVALID_PARAMETER
//define EMCUTIL_SYS_ERROR_ALREADY_EXISTS       EMCUTIL_STATUS_ALREADY_EXISTS
//define EMCUTIL_SYS_ERROR_GEN_FAILURE          EMCUTIL_STATUS_UNSUCCESSFUL
//define EMCUTIL_SYS_ERROR_FILE_NOT_FOUND       EMCUTIL_STATUS_NAME_NOT_FOUND
//define EMCUTIL_SYS_ERROR_MORE_DATA            EMCUTIL_STATUS_MORE_DATA
//define EMCUTIL_SYS_ERROR_CALL_NOT_IMPLEMENTED EMCUTIL_STATUS_NOT_IMPLEMENTED

#ifdef ALAMOSA_WINDOWS_ENV

#define EMCUTIL_SYS_ERROR_BAD_ARGUMENTS        ERROR_BAD_ARGUMENTS
#define EMCUTIL_SYS_ERROR_BAD_COMMAND          ERROR_BAD_COMMAND
#define EMCUTIL_SYS_ERROR_BAD_ENVIRONMENT      ERROR_BAD_ENVIRONMENT
#define EMCUTIL_SYS_ERROR_CANTREAD             ERROR_CANTREAD
#define EMCUTIL_SYS_ERROR_DISK_CORRUPT         ERROR_DISK_CORRUPT
#define EMCUTIL_SYS_ERROR_DISK_FULL            ERROR_DISK_FULL
#define EMCUTIL_SYS_ERROR_HANDLE_EOF           ERROR_HANDLE_EOF
#define EMCUTIL_SYS_ERROR_INSUFFICIENT_BUFFER  ERROR_INSUFFICIENT_BUFFER
#define EMCUTIL_SYS_ERROR_INVALID_BLOCK_LENGTH ERROR_INVALID_BLOCK_LENGTH
#define EMCUTIL_SYS_ERROR_INVALID_DATA         ERROR_INVALID_DATA
#define EMCUTIL_SYS_ERROR_INVALID_HANDLE       ERROR_INVALID_HANDLE
#define EMCUTIL_SYS_ERROR_INVALID_NAME         ERROR_INVALID_NAME
#define EMCUTIL_SYS_ERROR_IO_DEVICE            ERROR_IO_DEVICE
#define EMCUTIL_SYS_ERROR_LOGON_FAILURE        ERROR_LOGON_FAILURE
#define EMCUTIL_SYS_ERROR_MR_MID_NOT_FOUND     ERROR_MR_MID_NOT_FOUND
#define EMCUTIL_SYS_ERROR_NO_MORE_ITEMS        ERROR_NO_MORE_ITEMS
#define EMCUTIL_SYS_ERROR_NOT_ENOUGH_MEMORY    ERROR_NOT_ENOUGH_MEMORY
#define EMCUTIL_SYS_ERROR_NOT_FOUND            ERROR_NOT_FOUND
#define EMCUTIL_SYS_ERROR_NOT_SUPPORTED        ERROR_NOT_SUPPORTED
#define EMCUTIL_SYS_ERROR_OPEN_FAILED          ERROR_OPEN_FAILED
#define EMCUTIL_SYS_ERROR_OUTOFMEMORY          ERROR_OUTOFMEMORY
#define EMCUTIL_SYS_ERROR_PATH_NOT_FOUND       ERROR_PATH_NOT_FOUND
#define EMCUTIL_SYS_ERROR_PIPE_NOT_CONNECTED   ERROR_PIPE_NOT_CONNECTED

#else

#define EMCUTIL_SYS_ERROR_ARTIFICAL_BASE       11000
#define EMCUTIL_SYS_ERROR_BAD_ARGUMENTS        ((DWORD)CSX_STATUS_INVALID_ARGUMENTS)
#define EMCUTIL_SYS_ERROR_BAD_COMMAND          ((DWORD)CSX_STATUS_INVALID_COMMAND)
#define EMCUTIL_SYS_ERROR_BAD_ENVIRONMENT      (EMCUTIL_SYS_ERROR_ARTIFICAL_BASE+120)
#define EMCUTIL_SYS_ERROR_CANTREAD             (EMCUTIL_SYS_ERROR_ARTIFICAL_BASE+3)
#define EMCUTIL_SYS_ERROR_DISK_CORRUPT         ((DWORD)EMCPAL_STATUS_DISK_CORRUPT_ERROR)
#define EMCUTIL_SYS_ERROR_DISK_FULL            ((DWORD)EMCPAL_STATUS_DISK_FULL)
#define EMCUTIL_SYS_ERROR_HANDLE_EOF           ((DWORD)CSX_STATUS_END_OF_FILE) /* non-1-1 */
#define EMCUTIL_SYS_ERROR_INSUFFICIENT_BUFFER  ((DWORD)CSX_STATUS_BUFFER_TOO_SMALL)
#define EMCUTIL_SYS_ERROR_INVALID_BLOCK_LENGTH (EMCUTIL_SYS_ERROR_ARTIFICAL_BASE+6)
#define EMCUTIL_SYS_ERROR_INVALID_DATA         (EMCUTIL_SYS_ERROR_ARTIFICAL_BASE+204)
#define EMCUTIL_SYS_ERROR_INVALID_HANDLE       ((DWORD)EMCPAL_STATUS_PORT_DISCONNECTED)
#define EMCUTIL_SYS_ERROR_INVALID_NAME         ((DWORD)EMCPAL_STATUS_OBJECT_NAME_INVALID)
#define EMCUTIL_SYS_ERROR_IO_DEVICE            ((DWORD)EMCPAL_STATUS_DEVICE_PROTOCOL_ERROR)
#define EMCUTIL_SYS_ERROR_LOGON_FAILURE        (EMCUTIL_SYS_ERROR_ARTIFICAL_BASE+104)
#define EMCUTIL_SYS_ERROR_MR_MID_NOT_FOUND     (EMCUTIL_SYS_ERROR_ARTIFICAL_BASE+12)
#define EMCUTIL_SYS_ERROR_NO_MORE_ITEMS        ((DWORD)CSX_STATUS_END_OF_FILE) /* non-1-1 */
#define EMCUTIL_SYS_ERROR_NOT_ENOUGH_MEMORY    ((DWORD)CSX_STATUS_NO_RESOURCES) /* non-1-1 */
#define EMCUTIL_SYS_ERROR_NOT_FOUND            ((DWORD)EMCPAL_STATUS_NOT_FOUND)
#define EMCUTIL_SYS_ERROR_NOT_SUPPORTED        (EMCUTIL_SYS_ERROR_ARTIFICAL_BASE+135)
#define EMCUTIL_SYS_ERROR_OPEN_FAILED          (EMCUTIL_SYS_ERROR_ARTIFICAL_BASE+134)
#define EMCUTIL_SYS_ERROR_OUTOFMEMORY          ((DWORD)CSX_STATUS_NO_RESOURCES) /* non-1-1 */
#define EMCUTIL_SYS_ERROR_PATH_NOT_FOUND       ((DWORD)EMCPAL_STATUS_OBJECT_PATH_NOT_FOUND)
#define EMCUTIL_SYS_ERROR_PIPE_NOT_CONNECTED   (EMCUTIL_SYS_ERROR_ARTIFICAL_BASE+17)

#endif /* ALAMOSA_WINDOWS_ENV - STDPORT - accomodate Win32 error code usage in system */

////////////////////////////////////

//++
//.End file EmcUTIL.h
//--

#endif                                     /* EMCUTIL_H_ */
