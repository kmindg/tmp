//***************************************************************************
// Copyright (C) EMC Corporation 2012-2015
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name: libpmp.h
//      
// Contents: Defines and structures used to interface with the PMP module.
//     
//
//--

#ifndef __LIBPMP_H__
#define __LIBPMP_H__

#include "csx_ext.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * types, enumerations and macros
 */

/*
 * An opaque handle created by libpmpRegister.
 */
typedef const void *libpmp_handle_t;

#ifndef _STDINT_H
/* 
 * lxpmp (a stand alone Linux (non-CSX) program) uses native stdint.h, so we need to 
 * define these types if this file is included without lxpmp
 */
typedef unsigned int   uint32_t;
typedef unsigned short uint16_t;
typedef unsigned long  uint64_t;
#endif /* _STDINT_H */

#define LIBPMP_HANDLE_IS_VALID(_h) ( (_h) != (libpmp_handle_t)NULL )
#define LIBPMP_INVALID_HANDLE() ((libpmp_handle_t) NULL)

/*
 * Default size of the PMP Partition on a BeachComber
 * The Partition is 2GB + overhead for the PMP header plus
 * space for each of the registered users.   Which at this
 * time is 7506432 bytes.
 */
#define PMP_PARTITION_SIZE_DEFAULT 2154990080
#define PMP_PARTITION_OVERHEAD ((UINT_64) PMP_PARTITION_SIZE_DEFAULT - ((UINT_64) 2 * (UINT_64) (1024*1024*1024)))
/*
 * Status codes that can be returned by libpmp
 */
typedef enum {
  libpmp_status_success = 0,
  libpmp_status_not_initialized,
  libpmp_status_pmp_not_enabled,
  libpmp_status_pmp_not_supported,
  libpmp_status_unknown_client,
  libpmp_status_already_registered,
  libpmp_status_not_registered,
  libpmp_status_no_resources,
  libpmp_status_service_unavailable,
  libpmp_status_invalid_argument,
  libpmp_status_nonexistent_sgl,
  libpmp_status_cannot_open,
  libpmp_status_cannot_map,
  libpmp_status_already_open,
  libpmp_status_cannot_unmap,
  libpmp_status_sgl_too_large,
  libpmp_status_sgl_already_installed,
  libpmp_status_exceeded_sgl_limit,
  libpmp_status_sgl_overlaps,
  libpmp_status_too_many_users,
  libpmp_status_system_error,
  libpmp_status_would_exceed_capacity,
  libpmp_status_corrupt_structure,
  libpmp_status_sgl_exceeds_reservation
} libpmp_status_t, *plibpmp_status_t;

#define LIBPMP_SUCCESS( _s ) ( libpmp_status_success            == _s || \
                               libpmp_status_already_registered == _s )

/*
 * Legitimate clients of PMP.
 */
#define LIBPMP_BASE_CLIENT 500
typedef enum {
  libpmp_client_mcc     = LIBPMP_BASE_CLIENT,
  libpmp_client_pramfs,
  libpmp_client_tool,
  libpmp_client_test,

  /* Add more as needed */
  
  libpmp_client_max
} libpmp_client_id;

CSX_MOD_EXPORT
libpmp_status_t libpmpInit( libpmp_handle_t    *handle /* output */,
                            libpmp_client_id    id,
                            const char         *client_name );

CSX_MOD_EXPORT
libpmp_status_t libpmpRegister( libpmp_handle_t     handle,
                                uint16_t            app_revision);

CSX_MOD_EXPORT
libpmp_status_t libpmpUnregister( libpmp_handle_t handle );

typedef enum {
  libpmpFlagImageRestored            = 0x00000001,
  libpmpFlagImageValid               = 0x00000002,
  libpmpFlagNvDeviceFailure          = 0x00000004,
  libpmpFlagInvalidSglSize           = 0x00000008,
  libpmpFlagDmaFailed                = 0x00000010,
  libpmpFlagNvDeviceCapacityExceeded = 0x00000020,
  libpmpFlagNvDeviceZeroed           = 0x00000040,
  libpmpFlagNoSglFound               = 0x00000080,
  libpmpFlagRestoreFailed            = 0x00000100,
  libpmpFlagMemoryPersistenceProblem = 0x00000200,
  libpmpFlagInvalidPmpRequest        = 0x00000400,
  libpmpFlagSglAddressAlignmentError = 0x00000800,
  libpmpFlagSglAddressRangeError     = 0x00001000,
  libpmpFlagSglLengthRangeError      = 0x00002000,
  libpmpFlagSglLengthMultiple        = 0x00004000,
  libpmpFlagAll                      = 0xFFFFFFFF
} libpmp_flags_t;

#define LIBPMP_USER_FLAG_IS_ERROR( _f )       \
( _f & ( libpmpFlagNvDeviceFailure          | \
         libpmpFlagInvalidSglSize           | \
         libpmpFlagDmaFailed                | \
         libpmpFlagNvDeviceCapacityExceeded | \
         libpmpFlagNoSglFound               | \
         libpmpFlagRestoreFailed            | \
         libpmpFlagMemoryPersistenceProblem | \
         libpmpFlagInvalidPmpRequest        | \
         libpmpFlagSglAddressAlignmentError | \
         libpmpFlagSglAddressRangeError     | \
         libpmpFlagSglLengthRangeError      | \
         libpmpFlagSglLengthMultiple ) )

CSX_MOD_EXPORT
libpmp_status_t libpmpGetAndClearUserFlags ( libpmp_handle_t handle, uint32_t  operflagmap,
                                             uint32_t *operflags, uint16_t *appflags);
CSX_MOD_EXPORT
libpmp_status_t libpmpClearUserFlags   ( libpmp_handle_t handle );
CSX_MOD_EXPORT
libpmp_status_t libpmpClientIsEnabled  ( libpmp_handle_t handle, csx_bool_t *enabled );
CSX_MOD_EXPORT
libpmp_status_t libpmpGetDeviceCapacity( libpmp_handle_t h, uint32_t *c );
CSX_MOD_EXPORT
libpmp_status_t libpmpGetAvailableSgls ( libpmp_handle_t h, uint32_t *a );
CSX_MOD_EXPORT
libpmp_status_t libpmpGetMaxSglSize    ( libpmp_handle_t h, uint64_t *m );
CSX_MOD_EXPORT
libpmp_status_t libpmpGetClientUsage   ( libpmp_handle_t h, uint64_t *m );
CSX_MOD_EXPORT
libpmp_status_t libpmpIsSglInstalled( libpmp_handle_t handle, csx_bool_t* installed);
CSX_MOD_EXPORT
libpmp_status_t libpmpClientEnable     ( libpmp_handle_t h );
CSX_MOD_EXPORT
libpmp_status_t libpmpClientDisable    ( libpmp_handle_t h );
CSX_MOD_EXPORT
libpmp_status_t libpmpGetRevision      ( libpmp_handle_t h, uint16_t *r );
CSX_MOD_EXPORT
libpmp_status_t libpmpGetFirstErrorSgl ( libpmp_handle_t h, uint32_t *s );
CSX_MOD_EXPORT
libpmp_status_t libpmpGetFirstErrorAdr ( libpmp_handle_t h, uint64_t *a );
CSX_MOD_EXPORT
libpmp_status_t libpmpAddSgl           ( libpmp_handle_t h,
                                         uint64_t       sgllength,
                                         uint64_t       sglSourceAddr,
                                         uint32_t       *sglEntryNumber );
CSX_MOD_EXPORT
libpmp_status_t libpmpDelSglByAddr     ( libpmp_handle_t h,
                                         uint64_t       sglSourceAddr );
CSX_MOD_EXPORT
libpmp_status_t libpmpDelSglByEntry    ( libpmp_handle_t h,
                                         uint32_t       sglEntryNumber );
CSX_MOD_EXPORT
libpmp_status_t libpmpDelSglAll        ( libpmp_handle_t h );

CSX_MOD_EXPORT
libpmp_status_t libpmpGlobalGetPmpEnable  ( libpmp_handle_t handle, csx_bool_t *avail );
CSX_MOD_EXPORT
int libpmp_safe_to_discard( void );
CSX_MOD_EXPORT
libpmp_status_t pmplib_debug_dump_single_user_structure(libpmp_handle_t handle, libpmp_client_id clientid);
CSX_MOD_EXPORT
unsigned int libpmp_client_id_to_index( libpmp_client_id id );
CSX_MOD_EXPORT
libpmp_status_t pmplib_debug_get_single_user_flags( libpmp_handle_t handle, libpmp_client_id clientid,
                                                    uint32_t *operflags, uint16_t *appflags );
CSX_MOD_EXPORT
libpmp_status_t libpmp_debug_clear_usr_flag( libpmp_handle_t, libpmp_client_id );
  
#if 0
typedef void *libpmpCommitCallback ( libpmp_handle_t h, void *arg1, void *arg2 );
libpmp_status_t libpmpCommit( libpmp_handle_t h, libpmpCommitCallback c);
#endif
CSX_MOD_EXPORT
const char *libpmpErrorString( libpmp_status_t s );
CSX_MOD_EXPORT
libpmp_status_t libpmpGlobalSetMpApproveDisableRequest( libpmp_handle_t handle );
CSX_MOD_EXPORT
libpmp_status_t libpmp_get_mp_status( libpmp_handle_t handle, uint32_t *mp_status );
CSX_MOD_EXPORT 
libpmp_status_t libpmp_set_mp_status( libpmp_handle_t handle, uint32_t mp_status );


/*
 * Internal structures NEED TO MOVE THIS OUT OF THIS HEADER FILE
 */
#define LIBPMP_NUMBER_OF_CLIENTS (libpmp_client_max - LIBPMP_BASE_CLIENT)
#define LIBPMP_CLIENT_ID_INVALID ((libpmp_client_id)(-1))

#define PMP_CLIENT_NAME_LENGTH (12)
/*
 * This structure is cast to the libpmp_handle_t
 */
typedef struct {
  /* File descriptor for PMP partition device */ 
  #define LIBPMPC_CLIENT_FD_UNINIT ((csx_p_file_t)NULL)
  csx_p_file_t         libpmpc_client_fd;
  libpmp_client_id     libpmpc_client_type;
  unsigned int         libpmpc_client_index;
  char                 libpmpc_client_name[PMP_CLIENT_NAME_LENGTH];
  struct lxpmp_hdr    *libpmpc_lxpmp_hdr;
  struct lxpmp_user   *libpmpc_lxuser_hdr;
} libpmp_client_t;

#include "lxpmp.h" 

#if !defined(SIMMODE_ENV)
#define libpmpLog(...) {                                              \
        static char _buffer[128];                                     \
        csx_p_stdio_snprintf(_buffer, 128, __VA_ARGS__);              \
        pmp_log(_buffer);                                             \
        csx_p_stdio_snprintf(_buffer, 128, "LIBPMP: " __VA_ARGS__);    \
        EmcpalDbgPrint(_buffer);                                      \
    }
#else  
/* 
 * SIMMODE_ENV 
 */
#define PMP_A_FILENAME "PMP.A"
#define PMP_B_FILENAME "PMP.B"


// Set the size of the partition that PMP will use to preserve Persistent
// Memory.   Note that this size is in bytes will be impacted by the number
// of bytes that libpmp needs to reserve for internal data structures.  At
// last check this overhead was approximately 7506432 bytes.
//
// 4K for PMP Header
// @365 4K blocks for each client (5 clients)
// Total comes to 7506432 bytes of overhead.
//
// @param pmpPartitionSize - requested size of the PMP partition in Bytes.
// Returns:
//   void
CSX_MOD_EXPORT
void setPmpPartitionSizeBytes(unsigned long long pmpPartitionSizeBytes);

// Get the size of the partition that PMP will use to preserve Persistent
// Memory.   Note that this size is in bytes will be impacted by the number
// of bytes that libpmp needs to reserve for internal data structures.  At
// last check this overhead was approximately 7506432 bytes.
//
// Returns:
//   size of the PMP partition in bytes
CSX_MOD_EXPORT
unsigned long long getPmpPartitionSizeBytes(void);

typedef void (*PMPLOGGER_CALLBACK)(void* context, char* buffer);

extern PMPLOGGER_CALLBACK  PMPLOGGERFUNC;
extern void*       PMPLOGGERCONTEXT;

CSX_MOD_EXPORT
void setTraceLogger(void* context,PMPLOGGER_CALLBACK logger);

#define libpmpLog(...) {                                              \
        if(PMPLOGGERFUNC != 0) {                                      \
            static char _buffer[128];                                 \
            csx_p_stdio_snprintf(_buffer, 128, __VA_ARGS__);          \
            (*PMPLOGGERFUNC)(PMPLOGGERCONTEXT, _buffer);              \
        } else {                                                      \
            mut_printf(MUT_LOG_HIGH, __VA_ARGS__);                    \
        }                                                             \
    }

#endif /* SIMMODE_ENV */

extern unsigned long long  PMP_PARTITION_SIZE;
#define PMP_PARTITION_NAME_LEN 80

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /*  __LIBPMP_H__ */
