#ifndef _EMCPAL_IRP_H_
#define _EMCPAL_IRP_H_
/*!
 * @file EmcPAL_Irp.h
 * @addtogroup emcpal_irps
 * @{
 */

/***************************************************************************
 * Copyright (C) EMC Corporation 2009, 2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!
 * @file EmcPAL_Irp.h
 *
 * @version
 *     7-09 Chris Gould - Created.<br />
 *     9-09 GSE - Extended typedefs
 *
 */


//++
// Include files
//--

#ifdef __cplusplus
extern "C" {
#endif

#include "EmcPAL.h"

//PZPZ
#if !defined(UMODE_ENV) && !defined(ADMIN_ENV) && !defined(SIMMODE_ENV) && defined(ALAMOSA_WINDOWS_ENV)
#include <storport.h>
#endif
#if !defined(UMODE_ENV) && !defined(ADMIN_ENV) && !defined(SIMMODE_ENV) && !defined(ALAMOSA_WINDOWS_ENV)
#include "safe_win_scsi.h"
#endif
#if defined(_NTSRB_) || defined(_NTSTORPORT_) || defined(_NTSCSI_) || defined(SAFE_SCSI_STUFF_AVAILABLE)
#define EMCPAL_SCSI_STUFF_AVAILABLE
#endif

//PZPZ
#define      FLARE_DCA_READ                  0x10

// decide if we truly have a complete 'kernel' case level of underlying
// services available
#if !defined(UMODE_ENV) && !defined(ADMIN_ENV) && !defined(SIMMODE_ENV)
#define EMCPAL_KERNEL_STUFF_AVAILABLE
#endif
#if !defined(UMODE_ENV) && !defined(ADMIN_ENV) && !defined(SIMMODE_ENV) && defined(ALAMOSA_WINDOWS_ENV)
#define EMCPAL_KERNEL_STUFF_AVAILABLE_WIN_ONLY
#endif

/* calling conventions differ between CSX and Windows for IRP cancel callbacks */
#ifdef EMCPAL_USE_CSX_DCALL
#define EMCPAL_IRP_CC CSX_GX_CI_DEFCC
#define EMCPAL_IRP_CC CSX_GX_CI_DEFCC
#else
#define EMCPAL_IRP_CC		/*!< EmcPAL IRP caling convention */
#endif

#ifdef EMCPAL_USE_CSX_DCALL
#define EMCPAL_IRP_MAGIC_FIELD_NAME CSX_P_DCALL_MAGIC_FIELD_NAME
#define EMCPAL_IRP_MAGIC_FIELD_VALUE CSX_P_DCALL_MAGIC_FIELD_VALUE
#else
#define EMCPAL_IRP_MAGIC_FIELD_NAME "Type"
#define EMCPAL_IRP_MAGIC_FIELD_VALUE IO_TYPE_IRP
#endif

#ifdef EMCPAL_USE_CSX_DCALL

#define EMCPAL_IRP_TYPE_CODE_CREATE CSX_P_DCALL_TYPE_CODE_OPEN
#define EMCPAL_IRP_TYPE_CODE_CLOSE  CSX_P_DCALL_TYPE_CODE_CLOSE
#define EMCPAL_IRP_TYPE_CODE_CLEANUP CSX_P_DCALL_TYPE_CODE_USER_DEFINED_3
#define EMCPAL_IRP_TYPE_CODE_READ CSX_P_DCALL_TYPE_CODE_READ
#define EMCPAL_IRP_TYPE_CODE_WRITE CSX_P_DCALL_TYPE_CODE_WRITE
#define EMCPAL_IRP_TYPE_CODE_DEVICE_CONTROL CSX_P_DCALL_TYPE_CODE_IOCTL
#define EMCPAL_IRP_TYPE_CODE_INTERNAL_DEVICE_CONTROL CSX_P_DCALL_TYPE_CODE_USER_DEFINED_2
#define EMCPAL_IRP_TYPE_CODE_SCSI CSX_P_DCALL_TYPE_CODE_USER_DEFINED_4
#define EMCPAL_IRP_TYPE_CODE_CREATE_NAMED_PIPE CSX_P_DCALL_TYPE_CODE_USER_DEFINED_5
#define EMCPAL_IRP_TYPE_CODE_QUERY_INFORMATION CSX_P_DCALL_TYPE_CODE_USER_DEFINED_6
#define EMCPAL_IRP_TYPE_CODE_SET_INFORMATION CSX_P_DCALL_TYPE_CODE_USER_DEFINED_7
#define EMCPAL_IRP_TYPE_CODE_QUERY_EA CSX_P_DCALL_TYPE_CODE_USER_DEFINED_8
#define EMCPAL_IRP_TYPE_CODE_SET_EA CSX_P_DCALL_TYPE_CODE_USER_DEFINED_9
#define EMCPAL_IRP_TYPE_CODE_FLUSH_BUFFERS CSX_P_DCALL_TYPE_CODE_USER_DEFINED_10
#define EMCPAL_IRP_TYPE_CODE_QUERY_VOLUME_INFORMATION CSX_P_DCALL_TYPE_CODE_USER_DEFINED_11
#define EMCPAL_IRP_TYPE_CODE_SET_VOLUME_INFORMATION CSX_P_DCALL_TYPE_CODE_USER_DEFINED_12
#define EMCPAL_IRP_TYPE_CODE_DIRECTORY_CONTROL CSX_P_DCALL_TYPE_CODE_USER_DEFINED_13
#define EMCPAL_IRP_TYPE_CODE_FILE_SYSTEM_CONTROL CSX_P_DCALL_TYPE_CODE_USER_DEFINED_14
#define EMCPAL_IRP_TYPE_CODE_SHUTDOWN CSX_P_DCALL_TYPE_CODE_USER_DEFINED_17     
#define EMCPAL_IRP_TYPE_CODE_LOCK_CONTROL CSX_P_DCALL_TYPE_CODE_USER_DEFINED_18
#define EMCPAL_IRP_TYPE_CODE_CREATE_MAILSLOT CSX_P_DCALL_TYPE_CODE_USER_DEFINED_20
#define EMCPAL_IRP_TYPE_CODE_QUERY_SECURITY CSX_P_DCALL_TYPE_CODE_USER_DEFINED_21
#define EMCPAL_IRP_TYPE_CODE_SET_SECURITY CSX_P_DCALL_TYPE_CODE_USER_DEFINED_22
#define EMCPAL_IRP_TYPE_CODE_POWER CSX_P_DCALL_TYPE_CODE_USER_DEFINED_23
#define EMCPAL_IRP_TYPE_CODE_SYSTEM_CONTROL CSX_P_DCALL_TYPE_CODE_USER_DEFINED_24
#define EMCPAL_IRP_TYPE_CODE_DEVICE_CHANGE CSX_P_DCALL_TYPE_CODE_USER_DEFINED_25
#define EMCPAL_IRP_TYPE_CODE_QUERY_QUOTA CSX_P_DCALL_TYPE_CODE_USER_DEFINED_26
#define EMCPAL_IRP_TYPE_CODE_SET_QUOTA CSX_P_DCALL_TYPE_CODE_USER_DEFINED_27
#define EMCPAL_IRP_TYPE_CODE_PNP CSX_P_DCALL_TYPE_CODE_USER_DEFINED_28
#define EMCPAL_IRP_TYPE_CODE_PNP_POWER CSX_P_DCALL_TYPE_CODE_USER_DEFINED_29

typedef csx_nuint_t EMCPAL_IRP_IOCTL_CODE;

#else

#define EMCPAL_IRP_TYPE_CODE_CREATE IRP_MJ_CREATE							/*!< IRP-type code: create */
#define EMCPAL_IRP_TYPE_CODE_CLOSE IRP_MJ_CLOSE								/*!< IRP-type code: close */
#define EMCPAL_IRP_TYPE_CODE_CLEANUP IRP_MJ_CLEANUP							/*!< IRP-type code: cleanup */
#define EMCPAL_IRP_TYPE_CODE_READ IRP_MJ_READ								/*!< IRP-type code: read */
#define EMCPAL_IRP_TYPE_CODE_WRITE IRP_MJ_WRITE								/*!< IRP-type code: write */
#define EMCPAL_IRP_TYPE_CODE_DEVICE_CONTROL IRP_MJ_DEVICE_CONTROL			/*!< IRP-type code: device control */
#define EMCPAL_IRP_TYPE_CODE_INTERNAL_DEVICE_CONTROL IRP_MJ_INTERNAL_DEVICE_CONTROL		/*!< IRP-type code: internal device control */
#define EMCPAL_IRP_TYPE_CODE_SCSI IRP_MJ_SCSI								/*!< IRP-type code: SCSI */
#define EMCPAL_IRP_TYPE_CODE_CREATE_NAMED_PIPE IRP_MJ_CREATE_NAMED_PIPE		/*!< IRP-type code: create named pipe */
#define EMCPAL_IRP_TYPE_CODE_QUERY_INFORMATION IRP_MJ_QUERY_INFORMATION		/*!< IRP-type code: query info */
#define EMCPAL_IRP_TYPE_CODE_SET_INFORMATION IRP_MJ_SET_INFORMATION			/*!< IRP-type code: set info */
#define EMCPAL_IRP_TYPE_CODE_QUERY_EA IRP_MJ_QUERY_EA						/*!< IRP-type code: query EA */
#define EMCPAL_IRP_TYPE_CODE_SET_EA IRP_MJ_SET_EA							/*!< IRP-type code: set EA */
#define EMCPAL_IRP_TYPE_CODE_FLUSH_BUFFERS IRP_MJ_FLUSH_BUFFERS				/*!< IRP-type code: flush buffers */
#define EMCPAL_IRP_TYPE_CODE_QUERY_VOLUME_INFORMATION IRP_MJ_QUERY_VOLUME_INFORMATION	/*!< IRP-type code: query volume info */
#define EMCPAL_IRP_TYPE_CODE_SET_VOLUME_INFORMATION IRP_MJ_SET_VOLUME_INFORMATION		/*!< IRP-type code: set volume info */
#define EMCPAL_IRP_TYPE_CODE_DIRECTORY_CONTROL IRP_MJ_DIRECTORY_CONTROL		/*!< IRP-type code: directory control */
#define EMCPAL_IRP_TYPE_CODE_FILE_SYSTEM_CONTROL IRP_MJ_FILE_SYSTEM_CONTROL	/*!< IRP-type code: file-system control */
#define EMCPAL_IRP_TYPE_CODE_DEVICE_CONTROL IRP_MJ_DEVICE_CONTROL			/*!< IRP-type code: device control */
#define EMCPAL_IRP_TYPE_CODE_SHUTDOWN IRP_MJ_SHUTDOWN    					/*!< IRP-type code: shutdown */
#define EMCPAL_IRP_TYPE_CODE_LOCK_CONTROL IRP_MJ_LOCK_CONTROL				/*!< IRP-type code: lock control */
#define EMCPAL_IRP_TYPE_CODE_CREATE_MAILSLOT IRP_MJ_CREATE_MAILSLOT			/*!< IRP-type code: create mailslot */
#define EMCPAL_IRP_TYPE_CODE_QUERY_SECURITY IRP_MJ_QUERY_SECURITY			/*!< IRP-type code: query security */
#define EMCPAL_IRP_TYPE_CODE_SET_SECURITY IRP_MJ_SET_SECURITY				/*!< IRP-type code: set security */
#define EMCPAL_IRP_TYPE_CODE_POWER IRP_MJ_POWER								/*!< IRP-type code: power */
#define EMCPAL_IRP_TYPE_CODE_SYSTEM_CONTROL IRP_MJ_SYSTEM_CONTROL			/*!< IRP-type code: system control */
#define EMCPAL_IRP_TYPE_CODE_DEVICE_CHANGE IRP_MJ_DEVICE_CHANGE 			/*!< IRP-type code: device change */
#define EMCPAL_IRP_TYPE_CODE_QUERY_QUOTA IRP_MJ_QUERY_QUOTA 				/*!< IRP-type code: query quota */
#define EMCPAL_IRP_TYPE_CODE_SET_QUOTA IRP_MJ_SET_QUOTA						/*!< IRP-type code: set quota */
#define EMCPAL_IRP_TYPE_CODE_PNP IRP_MJ_PNP									/*!< IRP-type code: PNP (plug & play) */
#define EMCPAL_IRP_TYPE_CODE_PNP_POWER IRP_MJ_PNP_POWER						/*!< IRP-type code: PNP power */

typedef ULONG EMCPAL_IRP_IOCTL_CODE;		/*!< EmcPAL IOCTL code */

#endif

/*!
 * @brief Three way codes equivalence table
  
  <table>
  <tr><th>WDM:</th>								<th>CSX:</th>									<th>EMCPAL:</th></tr>
  <tr><td>IRP_MJ_CREATE</td>					<td>CSX_P_DCALL_TYPE_CODE_OPEN</td>             <td>EMCPAL_IRP_TYPE_CODE_CREATE</td></tr>
  <tr><td>IRP_MJ_CLOSE</td>						<td>CSX_P_DCALL_TYPE_CODE_CLOSE</td>            <td>EMCPAL_IRP_TYPE_CODE_CLOSE</td></tr>
  <tr><td>IRP_MJ_READ</td>						<td>CSX_P_DCALL_TYPE_CODE_READ</td>             <td>EMCPAL_IRP_TYPE_CODE_READ</td></tr>
  <tr><td>IRP_MJ_WRITE</td>						<td>CSX_P_DCALL_TYPE_CODE_WRITE</td>            <td>EMCPAL_IRP_TYPE_CODE_WRITE</td></tr>
  <tr><td>IRP_MJ_DEVICE_CONTROL</td>			<td>CSX_P_DCALL_TYPE_CODE_IOCTL</td>            <td>EMCPAL_IRP_TYPE_CODE_DEVICE_CONTROL</td></tr>
  <tr><td>IRP_MJ_INTERNAL_DEVICE_CONTROL</td>	<td>CSX_P_DCALL_TYPE_CODE_USER_DEFINED_2</td>   <td>EMCPAL_IRP_TYPE_CODE_INTERNAL_DEVICE_CONTROL</td></tr>
  <tr><td>IRP_MJ_CLEANUP</td>					<td>CSX_P_DCALL_TYPE_CODE_USER_DEFINED_3</td>   <td>EMCPAL_IRP_TYPE_CODE_CLEANUP</td></tr>
  <tr><td>IRP_MJ_SCSI</td>						<td>CSX_P_DCALL_TYPE_CODE_USER_DEFINED_4</td>   <td>EMCPAL_IRP_TYPE_CODE_SCSI</td></tr>
  </table>
*/

typedef enum {
    EMCPAL_IRP_TRANSFER_AGNOSTIC,
    EMCPAL_IRP_TRANSFER_BUFFERED,
    EMCPAL_IRP_TRANSFER_DIRECT,
    EMCPAL_IRP_TRANSFER_USER
} EMCPAL_IRP_TRANSFER_METHOD;

//++
// End Includes
//--

#ifdef EMCPAL_USE_CSX_DCALL

typedef struct _EMCPAL_IRP_SCSI_DCALL_PARAMS
{
    struct _SCSI_REQUEST_BLOCK *Srb;
} EMCPAL_IRP_SCSI_DCALL_PARAMS;

typedef csx_dlist_entry_t EMCPAL_IRP_LIST_ENTRY, *PEMCPAL_IRP_LIST_ENTRY;
#define EMCPAL_IRP_LIST_ENTRY_FIELD dcall_user_list_entry_1
#define EMCPAL_IRP_LIST_ENTRY_LINK csx_dlist_field_next

#define EmcpalIrpGetScsiParams(_pIrpStack) ((EMCPAL_IRP_SCSI_DCALL_PARAMS *)(_pIrpStack->level_parameters.user_defined.user_defined_params))
#define EmcpalIrpGetIoctlParams(_pIrpStack) (&(_pIrpStack)->level_parameters.ioctl)
#define EmcpalIrpGetReadParams(_pIrpStack) (&(_pIrpStack)->level_parameters.read)
#define EmcpalIrpGetWriteParams(_pIrpStack) (&(_pIrpStack)->level_parameters.write)
#define EmcpalIrpGetOtherParams(_pIrpStack) (&(_pIrpStack)->level_parameters.user_defined)

#define EmcpalIrpIsCancelInProgress(_pIrp) ((_pIrp)->dcall_cancel_in_progress)

typedef ULONG EMCPAL_IRP_TRANSFER_LENGTH;
typedef csx_uoffset_t EMCPAL_IRP_TRANSFER_OFFSET;

#else /* EMCPAL_USE_CSX_DCALL */

/*! @brief IRP list entry */
typedef LIST_ENTRY EMCPAL_IRP_LIST_ENTRY, *PEMCPAL_IRP_LIST_ENTRY;	/*!< Ptr to IRP list entry */
#define EMCPAL_IRP_LIST_ENTRY_FIELD Tail.Overlay.ListEntry		/*!< IRP list entry field */
#define EMCPAL_IRP_LIST_ENTRY_LINK Flink						/*!< IRP list entry link */

#define EmcpalIrpGetScsiParams(_pIrpStack) (&(_pIrpStack)->Parameters.Scsi)				/*!< IRP get SCSI params */
#define EmcpalIrpGetIoctlParams(_pIrpStack) (&(_pIrpStack)->Parameters.DeviceIoControl)	/*!< IRP get IOCTL params */
#define EmcpalIrpGetReadParams(_pIrpStack) (&(_pIrpStack)->Parameters.Read)				/*!< IRP get read params */
#define EmcpalIrpGetWriteParams(_pIrpStack) (&(_pIrpStack)->Parameters.Write)			/*!< IRP get write params */
#define EmcpalIrpGetOtherParams(_pIrpStack) (&(_pIrpStack)->Parameters.Others)			/*!< IRP get other params */

#define EmcpalIrpIsCancelInProgress(_pIrp) ((_pIrp)->Cancel)							/*!< IRP Is cancel in progress */

typedef ULONG EMCPAL_IRP_TRANSFER_LENGTH;				/*!< IRP transfer length */
typedef ULONGLONG EMCPAL_IRP_TRANSFER_OFFSET;			/*!< IRP transfer offset */

#endif /* EMCPAL_USE_CSX_DCALL */

#ifdef EMCPAL_USE_CSX_DCALL

#define EmcpalIrpGetCurrentStackLocation(_pIrp) csx_p_dcall_get_current_level(_pIrp)
#define EmcpalIrpGetNextStackLocation(_pIrp) csx_p_dcall_get_next_level(_pIrp)
#define EmcpalIrpSkipCurrentIrpStackLocation(_pIrp) csx_p_dcall_goto_previous_level(_pIrp)
#define EmcpalIrpSetNextStackLocation(_pIrp) csx_p_dcall_goto_next_level(_pIrp)
#define EmcpalIrpCopyCurrentIrpStackLocationToNext(_pIrp) csx_p_dcall_copy_current_level_to_next(_pIrp)
#define EmcpalIrpCompleteRequest(_pIrp) csx_p_dcall_complete(_pIrp);
#define EmcpalIrpCompleteRequestDisk(_pIrp) csx_p_dcall_complete(_pIrp);

#else /* EMCPAL_USE_CSX_DCALL */

#define EmcpalIrpGetCurrentStackLocation(_pIrp) IoGetCurrentIrpStackLocation(_pIrp)		/*!< IRP get current stack loation */
#define EmcpalIrpGetNextStackLocation(_pIrp) IoGetNextIrpStackLocation(_pIrp)			/*!< IRP get next stack loation */
#define EmcpalIrpSkipCurrentIrpStackLocation(_pIrp) IoSkipCurrentIrpStackLocation(_pIrp) /*!< IRP skip current stack location */
#define EmcpalIrpSetNextStackLocation(_pIrp) IoSetNextIrpStackLocation(_pIrp)			/*!< IRP set next stack location */
#define EmcpalIrpCopyCurrentIrpStackLocationToNext(_pIrp) IoCopyCurrentIrpStackLocationToNext(_pIrp) /*!< IRP copy current stack location to next */
#define EmcpalIrpCompleteRequest(_pIrp) IoCompleteRequest(_pIrp, IO_NO_INCREMENT)		/*!< IRP  */
#define EmcpalIrpCompleteRequestDisk(_pIrp) IoCompleteRequest(_pIrp, IO_DISK_INCREMENT)	/*!< IRP complete disk request */

#endif /* EMCPAL_USE_CSX_DCALL */

#ifdef PAL_USE_CSX_DCALL

#define EMCPAL_IRP_COMPLETION_STATUS csx_status_e
#define EMCPAL_IRP_STATUS_MORE_PROCESSING_REQUIRED CSX_STATUS_MORE_PROCESSING_REQUIRED
#define EMCPAL_IRP_STATUS_CONTINUE_COMPLETION   CSX_STATUS_CONTINUE_COMPLETION
#define EMCPAL_IRP_STATUS_PENDING CSX_STATUS_PENDING

#else /* PAL_USE_CSX_DCALL */

#define EMCPAL_IRP_COMPLETION_STATUS NTSTATUS
#define EMCPAL_IRP_STATUS_MORE_PROCESSING_REQUIRED STATUS_MORE_PROCESSING_REQUIRED
#define EMCPAL_IRP_STATUS_CONTINUE_COMPLETION STATUS_CONTINUE_COMPLETION
#define EMCPAL_IRP_STATUS_PENDING STATUS_PENDING

#endif /* PAL_USE_CSX_DCALL */

#define EMCPAL_DEVICE_PATH       "\\Device\\"		/*!< Device path */
#define EMCPAL_WIN32_DEVICE_PATH "\\DosDevices\\"	/*!< Win32 Device path */

#ifdef PAL_FULL_IRP_AVAIL

#ifdef EMCPAL_USE_CSX_DCALL
/* this is - and must remain - compatible with CSX csx_p_dcall_completion_function_t */
typedef EMCPAL_STATUS (*PEMCPAL_IRP_COMPLETION_ROUTINE) (PEMCPAL_DEVICE_OBJECT DeviceObject, PEMCPAL_IRP Irp, PVOID Context);
#else
/* this is - and must remain - compatible with NT PIO_COMPLETION_ROUTINE */
typedef EMCPAL_STATUS (*PEMCPAL_IRP_COMPLETION_ROUTINE) (PEMCPAL_DEVICE_OBJECT DeviceObject, PEMCPAL_IRP Irp, PVOID Context);
#endif

CSX_LOCAL CSX_INLINE
VOID
EmcpalIrpSetCompletionRoutine(
    PEMCPAL_IRP pIrp,
    PEMCPAL_IRP_COMPLETION_ROUTINE CompletionRoutine,
    PVOID Context)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_p_dcall_completion_function_set(pIrp, (csx_p_dcall_completion_function_t)CompletionRoutine, Context, CSX_TRUE, CSX_TRUE, CSX_TRUE);
#else
    IoSetCompletionRoutine(pIrp, CompletionRoutine, Context, TRUE, TRUE, TRUE);
#endif
}

#ifdef EMCPAL_USE_CSX_DCALL

#define EmcpalIrpParamSetReadByteOffset(_pIrpStack)    (EmcpalIrpGetReadParams(_pIrpStack)->io_offset)
#define EmcpalIrpParamSetReadLength(_pIrpStack)        (EmcpalIrpGetReadParams(_pIrpStack)->io_size_requested)

#define EmcpalIrpParamSetWriteByteOffset(_pIrpStack)   (EmcpalIrpGetWriteParams(_pIrpStack)->io_offset)
#define EmcpalIrpParamSetWriteLength(_pIrpStack)       (EmcpalIrpGetWriteParams(_pIrpStack)->io_size_requested)

#define EmcpalIrpParamSetReadKey(_pIrpStack)           (EmcpalIrpGetReadParams(_pIrpStack)->user_context)
#define EmcpalIrpParamSetWriteKey(_pIrpStack)          (EmcpalIrpGetWriteParams(_pIrpStack)->user_context)

#define EmcpalIrpParamSetDcaReadTable(_pIrpStack, _pdca_table)   SET_PDCA_TABLE_IN_IRP((_pIrpStack), _pdca_table)
#define EmcpalIrpParamSetDcaWriteTable(_pIrpStack, _pdca_table)  SET_PDCA_TABLE_IN_IRP((_pIrpStack), _pdca_table)

#define EmcpalIrpParamGetReadByteOffset(_pIrpStack)    (EmcpalIrpGetReadParams(_pIrpStack)->io_offset)
#define EmcpalIrpParamGetReadLength(_pIrpStack)        ((ULONG)EmcpalIrpGetReadParams(_pIrpStack)->io_size_requested)

#define EmcpalIrpParamGetWriteByteOffset(_pIrpStack)   (EmcpalIrpGetWriteParams(_pIrpStack)->io_offset)
#define EmcpalIrpParamGetWriteLength(_pIrpStack)       ((ULONG)EmcpalIrpGetWriteParams(_pIrpStack)->io_size_requested)

#define EmcpalIrpParamGetReadKey(_pIrpStack)           (EmcpalIrpGetReadParams(_pIrpStack)->user_context)
#define EmcpalIrpParamGetWriteKey(_pIrpStack)          (EmcpalIrpGetWriteParams(_pIrpStack)->user_context)

#define EmcpalIrpParamGetDcaReadTable(_pIrpStack)       GET_PDCA_TABLE_FROM_IRP ( _pIrpStack )
#define EmcpalIrpParamGetDcaWriteTable(_pIrpStack)      GET_PDCA_TABLE_FROM_IRP ( _pIrpStack )

/* for now ioctl buffers should be accessed via CX_PREP_CLEANUP */
      
#define EmcpalIrpParamSetIoctlCode(_pIrpStack)         (EmcpalIrpGetIoctlParams(_pIrpStack)->ioctl_code)
#define EmcpalIrpParamSetIoctlInputSize(_pIrpStack)    (EmcpalIrpGetIoctlParams(_pIrpStack)->io_in_size_requested)
#define EmcpalIrpParamSetIoctlOutputSize(_pIrpStack)   (EmcpalIrpGetIoctlParams(_pIrpStack)->io_out_size_requested)

#define EmcpalIrpParamGetIoctlCode(_pIrpStack)         (EmcpalIrpGetIoctlParams(_pIrpStack)->ioctl_code)
#define EmcpalIrpParamGetIoctlInputSize(_pIrpStack)    (EmcpalIrpGetIoctlParams(_pIrpStack)->io_in_size_requested)
#define EmcpalIrpParamGetIoctlOutputSize(_pIrpStack)   (EmcpalIrpGetIoctlParams(_pIrpStack)->io_out_size_requested)

#else

#define EmcpalIrpParamSetReadByteOffset(_pIrpStack)    (EmcpalIrpGetReadParams(_pIrpStack)->ByteOffset.QuadPart)
#define EmcpalIrpParamSetReadLength(_pIrpStack)        (EmcpalIrpGetReadParams(_pIrpStack)->Length)

#define EmcpalIrpParamSetWriteByteOffset(_pIrpStack)   (EmcpalIrpGetWriteParams(_pIrpStack)->ByteOffset.QuadPart)
#define EmcpalIrpParamSetWriteLength(_pIrpStack)       (EmcpalIrpGetWriteParams(_pIrpStack)->Length)

#define EmcpalIrpParamSetReadKey(_pIrpStack)           (EmcpalIrpGetReadParams(_pIrpStack)->Key)
#define EmcpalIrpParamSetWriteKey(_pIrpStack)          (EmcpalIrpGetWriteParams(_pIrpStack)->Key)

#define EmcpalIrpParamSetDcaReadTable(_pIrpStack, _pdca_table)   SET_PDCA_TABLE_IN_IRP((_pIrpStack), _pdca_table)
#define EmcpalIrpParamSetDcaWriteTable(_pIrpStack, _pdca_table)  SET_PDCA_TABLE_IN_IRP((_pIrpStack), _pdca_table)

#define EmcpalIrpParamSetIoctlCode(_pIrpStack)         (EmcpalIrpGetIoctlParams(_pIrpStack)->IoControlCode)
#define EmcpalIrpParamSetIoctlInputSize(_pIrpStack)    (EmcpalIrpGetIoctlParams(_pIrpStack)->InputBufferLength)
#define EmcpalIrpParamSetIoctlOutputSize(_pIrpStack)   (EmcpalIrpGetIoctlParams(_pIrpStack)->OutputBufferLength)

#define EmcpalIrpParamGetReadByteOffset(_pIrpStack)    (EmcpalIrpGetReadParams(_pIrpStack)->ByteOffset.QuadPart)
#define EmcpalIrpParamGetReadLength(_pIrpStack)        (EmcpalIrpGetReadParams(_pIrpStack)->Length)

#define EmcpalIrpParamGetWriteByteOffset(_pIrpStack)   (EmcpalIrpGetWriteParams(_pIrpStack)->ByteOffset.QuadPart)
#define EmcpalIrpParamGetWriteLength(_pIrpStack)       (EmcpalIrpGetWriteParams(_pIrpStack)->Length)

#define EmcpalIrpParamGetReadKey(_pIrpStack)           (EmcpalIrpGetReadParams(_pIrpStack)->Key)
#define EmcpalIrpParamGetWriteKey(_pIrpStack)          (EmcpalIrpGetWriteParams(_pIrpStack)->Key)

#define EmcpalIrpParamGetDcaReadTable(_pIrpStack)       GET_PDCA_TABLE_FROM_IRP ( _pIrpStack )
#define EmcpalIrpParamGetDcaWriteTable(_pIrpStack)      GET_PDCA_TABLE_FROM_IRP ( _pIrpStack )

#define EmcpalIrpParamGetIoctlCode(_pIrpStack)         (EmcpalIrpGetIoctlParams(_pIrpStack)->IoControlCode)
#define EmcpalIrpParamGetIoctlInputSize(_pIrpStack)    (EmcpalIrpGetIoctlParams(_pIrpStack)->InputBufferLength)
#define EmcpalIrpParamGetIoctlOutputSize(_pIrpStack)   (EmcpalIrpGetIoctlParams(_pIrpStack)->OutputBufferLength)

#endif

#define EmcpalIrpParamSetDcaReadByteOffset     EmcpalIrpParamSetReadByteOffset
#define EmcpalIrpParamSetDcaReadLength         EmcpalIrpParamSetReadLength

#define EmcpalIrpParamSetDcaWriteByteOffset    EmcpalIrpParamSetWriteByteOffset
#define EmcpalIrpParamSetDcaWriteLength        EmcpalIrpParamSetWriteLength

#define EmcpalIrpParamGetDcaReadByteOffset     EmcpalIrpParamGetReadByteOffset
#define EmcpalIrpParamGetDcaReadLength         EmcpalIrpParamGetReadLength

#define EmcpalIrpParamGetDcaWriteByteOffset    EmcpalIrpParamGetWriteByteOffset
#define EmcpalIrpParamGetDcaWriteLength        EmcpalIrpParamGetWriteLength

#ifdef EMCPAL_USE_CSX_DCALL
#define EmcpalIrpCalculateSize csx_p_dcall_size
#else
#define EmcpalIrpCalculateSize IoSizeOfIrp
#endif

#ifdef EMCPAL_USE_CSX_DCALL
typedef csx_u16_t EMCPAL_IRP_STACK_FLAGS;
#define EMCPAL_IRP_STACK_FLAGS_IS_16_BITS
#else
typedef UCHAR EMCPAL_IRP_STACK_FLAGS;
#define EMCPAL_IRP_STACK_FLAGS_IS_8_BITS
#endif

CSX_LOCAL CSX_INLINE
EMCPAL_IRP_STACK_FLAGS
EmcpalIrpStackGetFlags(
    PEMCPAL_IO_STACK_LOCATION pIrpStack)
{
/* this is needed for the redirector because it likes to try to
 * copy IRP info from one IRP to another */
#ifdef EMCPAL_USE_CSX_DCALL
    return pIrpStack->level_flags;
#else
    return pIrpStack->Flags;
#endif
}

CSX_LOCAL CSX_INLINE
VOID
EmcpalIrpStackSetFlags(
    PEMCPAL_IO_STACK_LOCATION pIrpStack,
    EMCPAL_IRP_STACK_FLAGS Flags)
{
/* this is needed for the redirector because it likes to try to
 * copy IRP info from one IRP to another */
#ifdef EMCPAL_USE_CSX_DCALL
    pIrpStack->level_flags = Flags;
#else
    pIrpStack->Flags = Flags;
#endif
}

CSX_LOCAL CSX_INLINE
PEMCPAL_DEVICE_OBJECT
EmcpalIrpStackGetCurrentDeviceObject(
    PEMCPAL_IO_STACK_LOCATION pIrpStack)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return pIrpStack->level_device_handle;
#else
    return pIrpStack->DeviceObject;
#endif
}

CSX_LOCAL CSX_INLINE
PEMCPAL_IRP_COMPLETION_ROUTINE
EmcpalIrpStackGetCompletionRoutine(
    PEMCPAL_IO_STACK_LOCATION pIrpStack)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return (PEMCPAL_IRP_COMPLETION_ROUTINE)pIrpStack->level_completion_function;
#else
    return pIrpStack->CompletionRoutine;
#endif    
}

CSX_LOCAL CSX_INLINE
PVOID
EmcpalIrpStackGetCompletionContext(
    PEMCPAL_IO_STACK_LOCATION pIrpStack)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return pIrpStack->level_completion_context;
#else
    return pIrpStack->Context;
#endif    
}

CSX_LOCAL CSX_INLINE
VOID
EmcpalIrpStackSetCurrentDeviceObject(
    PEMCPAL_IO_STACK_LOCATION pIrpStack,
    PEMCPAL_DEVICE_OBJECT pDeviceObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
    pIrpStack->level_device_handle = pDeviceObject;
#else
    pIrpStack->DeviceObject = pDeviceObject;
#endif
}

CSX_LOCAL CSX_INLINE
VOID
EmcpalIrpStackSetCompletionRoutine(
    PEMCPAL_IO_STACK_LOCATION pIrpStack,
    PEMCPAL_IRP_COMPLETION_ROUTINE CompletionRoutine)
{
#ifdef EMCPAL_USE_CSX_DCALL
    pIrpStack->level_completion_function = (csx_p_dcall_completion_function_t)CompletionRoutine;
#else
    pIrpStack->CompletionRoutine = CompletionRoutine;
#endif    
}

CSX_LOCAL CSX_INLINE
VOID
EmcpalIrpStackSetCompletionContext(
    PEMCPAL_IO_STACK_LOCATION pIrpStack,
    PVOID Context)
{
#ifdef EMCPAL_USE_CSX_DCALL
    pIrpStack->level_completion_context = Context;
#else
    pIrpStack->Context = Context;
#endif    
}

CSX_LOCAL CSX_INLINE
void EmcpalIrpReuse(
    PEMCPAL_IRP pIrp,
    EMCPAL_STATUS status)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_p_dcall_recycle(pIrp, (csx_status_e)status);
#else
    IoReuseIrp(pIrp, status);
#endif
}

CSX_LOCAL CSX_INLINE
PEMCPAL_IRP
EmcpalIrpAllocate(
    UINT_8E StackSize)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return csx_p_dcall_allocate(StackSize);
#else
    return IoAllocateIrp(StackSize, FALSE);
#endif
}

CSX_LOCAL CSX_INLINE
VOID
EmcpalIrpFree(
    PEMCPAL_IRP pIrp)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_p_dcall_free(pIrp);
#else
    IoFreeIrp(pIrp);
#endif
}

CSX_LOCAL CSX_INLINE
BOOL
EmcpalIrpCancel(
    PEMCPAL_IRP pIrp)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return csx_p_dcall_cancel(pIrp);
#else
    return IoCancelIrp(pIrp);
#endif
}

#ifdef EMCPAL_USE_CSX_DCALL
typedef csx_p_dcall_cancel_function_t PEMCPAL_IRP_CANCEL_FUNCTION;
#else
typedef PDRIVER_CANCEL PEMCPAL_IRP_CANCEL_FUNCTION;
#endif

CSX_LOCAL CSX_INLINE
PEMCPAL_IRP_CANCEL_FUNCTION
EmcpalIrpCancelRoutineSet(
    PEMCPAL_IRP pIrp,
    PEMCPAL_IRP_CANCEL_FUNCTION CancelRoutine)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return csx_p_dcall_cancel_function_set(pIrp, CancelRoutine);
#else
    return IoSetCancelRoutine(pIrp, CancelRoutine);
#endif    
}

#ifndef EMCPAL_CASE_NTDDK_EXPOSE
CSX_LOCAL CSX_INLINE
VOID
EmcpalIrpCancelLockAcquire(
    EMCPAL_KIRQL *pFlags)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_p_dcall_cancel_lock_acquire();
    *pFlags = (EMCPAL_KIRQL)0; //PZPZ
#else
    IoAcquireCancelSpinLock(pFlags);
#endif
}

CSX_LOCAL CSX_INLINE
VOID
EmcpalIrpCancelLockRelease(
    EMCPAL_KIRQL Flags)
{
#ifdef EMCPAL_USE_CSX_DCALL
    CSX_UNREFERENCED_PARAMETER(Flags);
    csx_p_dcall_cancel_lock_release();
#else
    IoReleaseCancelSpinLock(Flags);
#endif
}

CSX_LOCAL CSX_INLINE
VOID
EmcpalIrpCancelLockReleaseFromCancelRoutine(
    PEMCPAL_IRP pIrp)
{
    EMCPAL_KIRQL Irql = (EMCPAL_KIRQL)0; //PZPZ
#ifndef EMCPAL_USE_CSX_DCALL
    Irql = pIrp->CancelIrql;
#endif
    EmcpalIrpCancelLockRelease(Irql);
}
#endif /* EMCPAL_CASE_NTDDK_EXPOSE */

CSX_LOCAL CSX_INLINE
VOID
EmcpalIrpStackSetFunction(
    PEMCPAL_IO_STACK_LOCATION pIrpStack,
    UINT_8E MajorFunction,
    UINT_8E MinorFunction)
{
#ifdef EMCPAL_USE_CSX_DCALL
    pIrpStack->dcall_type_code = MajorFunction;
    pIrpStack->dcall_type_sub_code = MinorFunction;	
#else
    pIrpStack->MajorFunction = MajorFunction;
    pIrpStack->MinorFunction = MinorFunction;
#endif		
}

CSX_LOCAL CSX_INLINE
PEMCPAL_IO_STACK_LOCATION
EmcpalIrpGetFirstIrpStackLocation(
    PEMCPAL_IRP pIrp)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return csx_p_dcall_get_first_level(pIrp);
#else
    return (PEMCPAL_IO_STACK_LOCATION)(pIrp + 1) + (pIrp->StackCount - 1);
#endif
}

CSX_LOCAL CSX_INLINE
VOID
EmcpalIrpStackGetFunction(
    PEMCPAL_IO_STACK_LOCATION pIrpStack,
    UINT_8E *pMajorFunction,
    UINT_8E *pMinorFunction)
{
    if (pMajorFunction != NULL)
    {
#ifdef EMCPAL_USE_CSX_DCALL	
        *pMajorFunction = pIrpStack->dcall_type_code;
#else
        *pMajorFunction = pIrpStack->MajorFunction;
#endif
    }

    if (pMinorFunction != NULL)
    {
#ifdef EMCPAL_USE_CSX_DCALL	
        *pMinorFunction = pIrpStack->dcall_type_sub_code;
#else
        *pMinorFunction = pIrpStack->MinorFunction;
#endif		
    }	
}

CSX_LOCAL CSX_INLINE
PEMCPAL_DEVICE_OBJECT
EmcpalDeviceGetRelatedDeviceObject(
    PEMCPAL_FILE_OBJECT pFileObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return csx_p_device_get_handle(pFileObject);
#else
    return IoGetRelatedDeviceObject(pFileObject);
#endif    
}

CSX_LOCAL CSX_INLINE
ULONG
EmcpalDeviceGetReferenceCount(
    PEMCPAL_DEVICE_OBJECT pDeviceObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return csx_p_device_get_open_count(pDeviceObject);
#else
    return pDeviceObject->ReferenceCount;
#endif
}

CSX_LOCAL CSX_INLINE
UINT_8E
EmcpalDeviceGetIrpStackSize(
    PEMCPAL_DEVICE_OBJECT pDeviceObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return csx_p_device_get_dcall_levels_required(pDeviceObject);
#else
    return pDeviceObject->StackSize;
#endif
}

EMCPAL_LOCAL EMCPAL_INLINE
VOID
EmcpalDeviceSetIrpStackSize(
    PEMCPAL_DEVICE_OBJECT pDeviceObject,
    UINT_8E StackSize)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_p_device_set_dcall_levels_required(pDeviceObject, StackSize);
#else
    pDeviceObject->StackSize = StackSize;
#endif
}

CSX_LOCAL CSX_INLINE
PVOID
EmcpalDeviceGetExtension(
    PEMCPAL_DEVICE_OBJECT pDeviceObject)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return csx_p_device_get_private_data(pDeviceObject);
#else
    return pDeviceObject->DeviceExtension;
#endif
}


CSX_LOCAL CSX_INLINE
VOID
EmcpalIrpPrepareRWBufferedIoBuffer(
    PEMCPAL_IRP  pIrp,
    PVOID IoBuffer)
{
#ifdef EMCPAL_USE_CSX_DCALL
    PEMCPAL_IO_STACK_LOCATION pIrpStack = EmcpalIrpGetNextStackLocation(pIrp);
    /* Read */
    EmcpalIrpGetReadParams(pIrpStack)->io_buffer = IoBuffer; /* CSXBUFFCOMPAT */
    pIrp->dcall_io_buffer = IoBuffer;
    pIrp->dcall_buffer_list = NULL;
#else
    pIrp->AssociatedIrp.SystemBuffer = IoBuffer;
    pIrp->MdlAddress = NULL;
#endif	
}

/* Buffer must be nonpaged but does not need
 * to be contiguous.  Works for read and write
 * requests
 */
CSX_LOCAL CSX_INLINE
VOID
EmcpalIrpPrepareRWDirectIoBuffer(
    PEMCPAL_IRP  pIrp,
    PVOID IoBuffer,
    ULONG IoSize)
{
#ifdef EMCPAL_USE_CSX_DCALL
    PEMCPAL_MDL Mdl;

    Mdl = csx_p_dcall_buffer_allocate(IoBuffer, IoSize);
    /* ASSERT Mdl != NULL */
    pIrp->dcall_buffer_list = Mdl;
    pIrp->dcall_io_buffer = NULL;
    csx_p_dcall_buffer_set_nonpaged(Mdl);
#else
    PMDL Mdl;

    Mdl = IoAllocateMdl(IoBuffer, IoSize, FALSE, FALSE, NULL);
    /* ASSERT Mdl != NULL */
    pIrp->MdlAddress = Mdl;
    pIrp->AssociatedIrp.SystemBuffer = NULL;
    MmBuildMdlForNonPagedPool(Mdl);
#endif	
}

CSX_LOCAL CSX_INLINE
VOID
EmcpalIrpPrepareRWIoBuffer(
    PEMCPAL_FILE_OBJECT pFileObject,
    PEMCPAL_IRP pIrp,
    PVOID IoBuffer,
    ULONG IoSize)
{
#ifdef EMCPAL_USE_CSX_DCALL
    PEMCPAL_DEVICE_OBJECT pDeviceObject = EmcpalDeviceGetRelatedDeviceObject(pFileObject);
    /* ASSERT PirpStack->MajorFunction == READ OR WRITE */
    if (csx_p_device_io_style_is_buffered(pDeviceObject)) {
        EmcpalIrpPrepareRWBufferedIoBuffer(pIrp, IoBuffer);
    } else if (csx_p_device_io_style_is_direct(pDeviceObject)) {
        EmcpalIrpPrepareRWDirectIoBuffer(pIrp, IoBuffer, IoSize);
    } else {
      CSX_PANIC();
    }
#else
    PEMCPAL_DEVICE_OBJECT pDeviceObject = EmcpalDeviceGetRelatedDeviceObject(pFileObject);
    /* ASSERT PirpStack->MajorFunction == READ OR WRITE */
    if (pDeviceObject->Flags & DO_BUFFERED_IO) {
        EmcpalIrpPrepareRWBufferedIoBuffer(pIrp, IoBuffer);
    } else if (pDeviceObject->Flags & DO_DIRECT_IO) {
        EmcpalIrpPrepareRWDirectIoBuffer(pIrp, IoBuffer, IoSize);
    } else {
        /* This should always use buffered or direct I/O.  We assume that any devices
           that use neither have a private contract and call either of the two functions
            below directly.
         */
        /* print or assert */
    }
#endif    
}

/* buffered I/O */
CSX_LOCAL CSX_INLINE
PVOID
EmcpalIrpRetrieveRWBufferedIoBuffer(
    PEMCPAL_IRP pIrp)
{
    PVOID IoBuffer;
#ifdef EMCPAL_USE_CSX_DCALL
    IoBuffer = pIrp->dcall_io_buffer;
#else
    IoBuffer = pIrp->AssociatedIrp.SystemBuffer;
#endif /* EMCPAL_USE_CSX_DCALL */
    return IoBuffer;
}

#ifdef EMCPAL_USE_CSX_DCALL
#define EmcpalIrpNormalPagePriority 0
#define EmcpalIrpHighPagePriority 0
#else
#define EmcpalIrpNormalPagePriority NormalPagePriority
#define EmcpalIrpHighPagePriority HighPagePriority
#endif

CSX_LOCAL CSX_INLINE
PVOID
EmcpalIrpRetrieveRWDirectIoBuffer(
    PEMCPAL_IRP pIrp,
    int priority)
{
    PVOID IoBuffer;
#ifdef EMCPAL_USE_CSX_DCALL
    if (pIrp->dcall_buffer_list != NULL) {
        IoBuffer = csx_p_dcall_buffer_get_mapped_address(pIrp->dcall_buffer_list);
    } else {
        IoBuffer = NULL;
    }
#else
    if (pIrp->MdlAddress != NULL) {
        IoBuffer = MmGetSystemAddressForMdlSafe(pIrp->MdlAddress, (MM_PAGE_PRIORITY) priority);
    } else {
        IoBuffer = NULL;
    }
#endif /* EMCPAL_USE_CSX_DCALL */
    return IoBuffer;
}

CSX_LOCAL CSX_INLINE
PVOID
EmcpalIrpRetrieveRWIoBuffer(
    PEMCPAL_IRP pIrp)
{
#ifdef EMCPAL_USE_CSX_DCALL
    PEMCPAL_IO_STACK_LOCATION pIrpStack = EmcpalIrpGetCurrentStackLocation(pIrp);
    /* ASSERT PirpStack->MajorFunction == READ OR WRITE */
    if (csx_p_device_io_style_is_buffered(pIrpStack->level_device_handle)) {
        return EmcpalIrpRetrieveRWBufferedIoBuffer(pIrp);
    } else if (csx_p_device_io_style_is_direct(pIrpStack->level_device_handle)) {
        return EmcpalIrpRetrieveRWDirectIoBuffer(pIrp, EmcpalIrpNormalPagePriority);
    } else {
        /* This should always use buffered or direct I/O.  We assume that any devices
           that use neither have a private contract and call either of the two functions
            below directly.
         */
        /* print or assert */
        return NULL;
    }
#else
    PEMCPAL_IO_STACK_LOCATION pIrpStack = EmcpalIrpGetCurrentStackLocation(pIrp);
    /* ASSERT PirpStack->MajorFunction == READ OR WRITE */
    if (pIrpStack->DeviceObject->Flags & DO_BUFFERED_IO) {
        return EmcpalIrpRetrieveRWBufferedIoBuffer(pIrp);
    } else if (pIrpStack->DeviceObject->Flags & DO_DIRECT_IO) {
        return EmcpalIrpRetrieveRWDirectIoBuffer(pIrp, EmcpalIrpNormalPagePriority);
    } else {
        /* This should always use buffered or direct I/O.  We assume that any devices
           that use neither have a private contract and call either of the two functions
            below directly.
         */
        /* print or assert */
        return NULL;
    }
#endif    
}	

/* Buffer must be nonpaged but does not need
 * to be contiguous
 */
CSX_LOCAL CSX_INLINE
VOID
EmcpalIrpFinishIoBuffer(
    PEMCPAL_IRP	pIrp)
{
#ifndef EMCPAL_USE_CSX_DCALL
    if (pIrp->MdlAddress != NULL) {
        IoFreeMdl(pIrp->MdlAddress);
        pIrp->MdlAddress = NULL;
    }
#else
    if (pIrp->dcall_buffer_list != NULL) {
        csx_p_dcall_buffer_free(pIrp->dcall_buffer_list);
        pIrp->dcall_buffer_list = NULL;
    }
#endif /* EMCPAL_USE_CSX_DCALL */
}

CSX_LOCAL CSX_INLINE
VOID
EmcpalIrpGetStatus(
    PEMCPAL_IRP pIrp,
    EMCPAL_STATUS *pStatus,
    ULONG	  *pIoSizeActual)
{
#ifdef EMCPAL_USE_CSX_DCALL
    // Assert dcall->dcall_status_ptr != NULL	
#endif
    if (pStatus != NULL)
    {
#ifdef EMCPAL_USE_CSX_DCALL
        *pStatus = pIrp->dcall_status_ptr->dcall_status_actual;
#else
        *pStatus = pIrp->IoStatus.Status;
#endif
    }
    if (pIoSizeActual != NULL) {
#ifdef EMCPAL_USE_CSX_DCALL
        *pIoSizeActual = (ULONG)pIrp->dcall_status_ptr->io_size_actual;
#else
        *pIoSizeActual = (ULONG)pIrp->IoStatus.Information;
#endif
    }
}


CSX_LOCAL CSX_INLINE
VOID
EmcpalIrpSetBadStatus(
    PEMCPAL_IRP pIrp,
    EMCPAL_STATUS Status)
{
    // ASSERT !NT_SUCCESS(status)
    // ASSERT dcall_status_ptr not null    
#ifdef EMCPAL_USE_CSX_DCALL 
    pIrp->dcall_status_ptr->dcall_status_actual = (csx_status_e)Status;
    pIrp->dcall_status_ptr->io_size_actual = 0;
#else
    pIrp->IoStatus.Status = Status;
    pIrp->IoStatus.Information = 0;
#endif
}

CSX_LOCAL CSX_INLINE
VOID
EmcpalIrpSetGoodStatus(
    PEMCPAL_IRP pIrp,
    EMCPAL_STATUS Status,
    ULONG IoSizeActual
    )
{
    // ASSERT dcall_status_ptr not null
    // ASSERT NT_SUCCESS(status)
#ifdef EMCPAL_USE_CSX_DCALL 
    pIrp->dcall_status_ptr->dcall_status_actual = (csx_status_e)Status;
    pIrp->dcall_status_ptr->io_size_actual = IoSizeActual;
#else
    pIrp->IoStatus.Status = Status;
    pIrp->IoStatus.Information = IoSizeActual;
#endif
}


CSX_LOCAL CSX_INLINE
VOID
EmcpalIrpSetStatus(
    PEMCPAL_IRP pIrp,
    EMCPAL_STATUS Status)
{
#ifdef EMCPAL_USE_CSX_DCALL
    pIrp->dcall_status_ptr->dcall_status_actual = (csx_status_e)Status;
#else
    pIrp->IoStatus.Status = Status;
#endif
}

CSX_LOCAL CSX_INLINE
VOID
EmcpalIrpSetInformation(
    PEMCPAL_IRP pIrp,
    ULONG Information)
{
#ifdef EMCPAL_USE_CSX_DCALL
    pIrp->dcall_status_ptr->io_size_actual = Information;
#else
    pIrp->IoStatus.Information = Information;
#endif
}

CSX_LOCAL CSX_INLINE
ULONG
EmcpalIrpGetInformation(
    PEMCPAL_IRP pIrp)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return (ULONG)(pIrp->dcall_status_ptr->io_size_actual);
#else
    return (ULONG)(pIrp->IoStatus.Information);
#endif
}

EMCPAL_API
VOID
EmcpalIrpDecodeIoctl(
    PEMCPAL_IRP pIrp,
    PEMCPAL_IO_STACK_LOCATION pIrpStack,
    ULONG *IoctlCode,
    PVOID *InputBuffer,
    ULONG *InputSize,
    PVOID *OutputBuffer,
    ULONG *OutputSize);

EMCPAL_API
EMCPAL_STATUS
EmcpalIrpValidateIoctl(
    PEMCPAL_IRP pIrp,
    PEMCPAL_IO_STACK_LOCATION pIrpStack,
    ULONG InputSize,
    ULONG OutputSize,
    const TEXT *Function);

EMCPAL_API
VOID
EmcpalIrpBuildLocalIoctl(
    PEMCPAL_IRP pIrp,
    LONG IoctlCode,
    PVOID IoctlBuffer,
    LONG InputSize,
    LONG OutputSize);

EMCPAL_API
EMCPAL_STATUS
EmcpalDeviceLocalIoctl(
    PEMCPAL_CLIENT pPalClient,
    PEMCPAL_FILE_OBJECT pFileObject,
    LONG IoctlCode,
    PVOID IoctlBuffer,
    LONG InputSize,
    LONG OutputSize,
    ULONG * ActualSize);

EMCPAL_API
EMCPAL_STATUS
EmcpalDeviceIoctl(
    PEMCPAL_CLIENT       pPalClient,
    PEMCPAL_FILE_OBJECT 	pFileObject,
    ULONG				IoctlCode,
    PVOID 				InputBuffer,
    ULONG 				InputSize,
    PVOID 				OutputBuffer,
    ULONG 				OutputSize,
    ULONG 		*ActualSize);

EMCPAL_API
EMCPAL_STATUS
EmcpalDeviceRead(
    PEMCPAL_CLIENT       pPalClient,
	PEMCPAL_FILE_OBJECT 	pFileObject,
	PVOID 				Buffer,
	ULONG 				IoSize,
    UINT_64E            ByteOffset,
    ULONG               Key,
	ULONG 		        *ActualSize);

EMCPAL_API
EMCPAL_STATUS
EmcpalDeviceWrite(
    PEMCPAL_CLIENT       pPalClient,
	PEMCPAL_FILE_OBJECT 	pFileObject,
	PVOID 				Buffer,
	ULONG 				IoSize,
    UINT_64E            ByteOffset,
    ULONG               Key,
	ULONG 		        *ActualSize);

EMCPAL_API
EMCPAL_STATUS
EmcpalDeviceOpen(
    PEMCPAL_CLIENT pPalClient,
    TEXT *DeviceName,
    PEMCPAL_FILE_OBJECT *pPalFileObject);

EMCPAL_API
EMCPAL_STATUS
EmcpalDeviceClose(
    PEMCPAL_FILE_OBJECT pPalFileObject);

CSX_LOCAL CSX_INLINE
VOID
EmcpalIrpInitialize(
    PEMCPAL_IRP    pIrp,
    USHORT  IrpSize,
    UINT_8E StackSize)
{
#ifdef EMCPAL_USE_CSX_DCALL    
    csx_p_dcall_initialize(pIrp, IrpSize, CSX_P_DCALL_FLAGS_USER_ALLOCATED, StackSize);
#else
    IoInitializeIrp(pIrp, IrpSize, StackSize);
    pIrp->IoStatus.Information = 0;
#endif
}

CSX_LOCAL CSX_INLINE
EMCPAL_STATUS
EmcpalIrpSend(
    PEMCPAL_FILE_OBJECT pFileObject,
    PEMCPAL_IRP pIrp)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return csx_p_dcall_make(pFileObject, pIrp);
#else
    PEMCPAL_DEVICE_OBJECT DeviceObject = IoGetRelatedDeviceObject(pFileObject);
    return IoCallDriver(DeviceObject, pIrp);
#endif
}

CSX_LOCAL CSX_INLINE
BOOL
EmcpalIrpPendingReturned(
    PEMCPAL_IRP    pIrp)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return pIrp->dcall_pending_returned;
#else
    return pIrp->PendingReturned;
#endif    
}

CSX_LOCAL CSX_INLINE
VOID
EmcpalIrpMarkPending(
    PEMCPAL_IRP    pIrp)
{
#ifdef EMCPAL_USE_CSX_DCALL
    csx_p_dcall_mark_pending(pIrp);
#else
    IoMarkIrpPending(pIrp);
#endif
}

CSX_LOCAL CSX_INLINE
VOID
EmcpalIrpDecodeReadRequest(
    PEMCPAL_IRP pIrp,
    PEMCPAL_IO_STACK_LOCATION pIrpStack,
    ULONG *Length,
    UINT_64E *ByteOffset,
    ULONG *Key)
{
    ULONG    myLength;
    UINT_64E myOffset;
    ULONG    myKey;

    // assert function is EMCPAL_IRP_TYPE_CODE_READ
    myLength = (ULONG)EmcpalIrpParamGetReadLength(pIrpStack);
    myOffset = (UINT_64E)EmcpalIrpParamGetReadByteOffset(pIrpStack);
    myKey = (ULONG)EmcpalIrpParamGetReadKey(pIrpStack);

    if (Length)
    {
        *Length = myLength;
    }

    if (ByteOffset)
    {
        *ByteOffset = myOffset;
    }

    if (Key)
    {
        *Key = myKey;
    }
}

CSX_LOCAL CSX_INLINE
VOID
EmcpalIrpDecodeWriteRequest(
    PEMCPAL_IRP pIrp,
    PEMCPAL_IO_STACK_LOCATION pIrpStack,
    ULONG *Length,
    UINT_64E *ByteOffset,
    ULONG *Key)
{
    ULONG    myLength;
    UINT_64E myOffset;
    ULONG    myKey;

    // assert function is IRP_MJ_WRITE
    myLength = (ULONG)EmcpalIrpParamGetWriteLength(pIrpStack);
    myOffset = (UINT_64E)EmcpalIrpParamGetWriteByteOffset(pIrpStack);
    myKey = (ULONG)EmcpalIrpParamGetWriteKey(pIrpStack);

    if (Length)
    {
        *Length = myLength;
    }

    if (ByteOffset)
    {
        *ByteOffset = myOffset;
    }

    if (Key)
    {
        *Key = myKey;
    }
}


CSX_LOCAL CSX_INLINE
VOID
EmcpalIrpBuildReadRequest(
    PEMCPAL_IRP pIrp,
    ULONG Length,
    UINT_64E ByteOffset,
    ULONG Key)
{
    PEMCPAL_IO_STACK_LOCATION pIrpStack = EmcpalIrpGetNextStackLocation(pIrp);
    EmcpalIrpStackSetFunction(pIrpStack, EMCPAL_IRP_TYPE_CODE_READ, 0);    
    EmcpalIrpParamSetReadLength(pIrpStack) = Length; 
    EmcpalIrpParamSetReadByteOffset(pIrpStack) = ByteOffset;
    EmcpalIrpParamSetReadKey(pIrpStack) = Key;
}

CSX_LOCAL CSX_INLINE
VOID
EmcpalIrpBuildWriteRequest(
    PEMCPAL_IRP pIrp,
    ULONG Length,
    UINT_64E ByteOffset,
    ULONG Key)
{
    PEMCPAL_IO_STACK_LOCATION pIrpStack = EmcpalIrpGetNextStackLocation(pIrp);
    EmcpalIrpStackSetFunction(pIrpStack, EMCPAL_IRP_TYPE_CODE_WRITE, 0);    
    EmcpalIrpParamSetWriteLength(pIrpStack) = Length; 
    EmcpalIrpParamSetWriteByteOffset(pIrpStack) = ByteOffset;
    EmcpalIrpParamSetWriteKey(pIrpStack) = Key;
}

#ifndef EMCPAL_NO_FLARE_IOCTLS

EMCPAL_API
VOID
EmcpalIrpBuildDcaRequest(
    BOOL                IsRead,
    PEMCPAL_IRP				pIrp,
    ULONG				Length,
    UINT_64E 			ByteOffset,
    PVOID				pDcaTable,
    PVOID				Context);

CSX_LOCAL CSX_INLINE
PVOID
EmcpalIrpGetDcaContext(
    PEMCPAL_IRP 		pIrp)
{
#ifdef EMCPAL_USE_CSX_DCALL
    return pIrp->dcall_io_buffer;
#else /* EMCPAL_USE_CSX_DCALL */
    return pIrp->AssociatedIrp.SystemBuffer;
#endif /* EMCPAL_USE_CSX_DCALL */
}

EMCPAL_API
VOID
EmcpalIrpDecodeDcaRequest(
    PEMCPAL_IRP 		pIrp,
    ULONG 		*Length,
    UINT_64E 	*ByteOffset,
    PVOID		*pDcaTable);

#ifndef EMCPAL_USE_CSX_DCALL
typedef struct {
    EMCPAL_SYNC_EVENT event;
    IO_STATUS_BLOCK    irpStatus;
} EMCPAL_INT_IRP_PEND_INFO, *PEMCPAL_INT_IRP_PEND_INFO;

CSX_LOCAL CSX_INLINE
#ifndef EMCPAL_USE_CSX_DCALL
NTSTATUS
#else
EMCPAL_STATUS
#endif
EmcpalIntIrpSendAndPendCompletionCallback(
    PEMCPAL_DEVICE_OBJECT pDeviceObject,
    PEMCPAL_IRP pIrp,
    PVOID context)
{
    PEMCPAL_INT_IRP_PEND_INFO pendInfo = (PEMCPAL_INT_IRP_PEND_INFO) context;
    PEMCPAL_SYNC_EVENT pEvent = (PEMCPAL_SYNC_EVENT) context;

    pendInfo->irpStatus = pIrp->IoStatus;

    if (EmcpalIrpPendingReturned(pIrp))
    {
        EmcpalIrpMarkPending(pIrp);
        EmcpalSyncEventSet(pEvent);
    }

    /* Will windows clean up after us here? do we need to free the MDL and such if one exists? */

    return EMCPAL_STATUS_CONTINUE_COMPLETION;
}
#endif /* EMCPAL_USE_CSX_DCALL */

#endif /* EMCPAL_NO_FLARE_IOCTLS */

CSX_LOCAL CSX_INLINE
EMCPAL_STATUS
EmcpalIrpSendAndPend(
    PEMCPAL_CLIENT pPalClient,
    PEMCPAL_FILE_OBJECT pFileObject,
    PEMCPAL_IRP pIrp,
    ULONG *Information,
    EMCPAL_TIMEOUT_MSECS timeout)
{
    EMCPAL_STATUS status;

#ifdef EMCPAL_USE_CSX_DCALL
    csx_p_dcall_status_t dcallStatus;

    status = csx_p_dcall_make_and_timedpend(
                EmcpalClientGetCsxModuleContext(pPalClient),
                pFileObject,
                pIrp,
                &dcallStatus,
                (csx_timeout_msecs_t)timeout);

    if (CSX_SUCCESS(status))
    {
        if (Information)
        {
            *Information = (ULONG)dcallStatus.io_size_actual;
        }
    }
    return status;
#else /* EMCPAL_USE_CSX_DCALL */
    EMCPAL_INT_IRP_PEND_INFO pendInfo;
    PEMCPAL_DEVICE_OBJECT pDeviceObject = IoGetRelatedDeviceObject(pFileObject);    


    EmcpalSyncEventCreate(pPalClient, &pendInfo.event, "pal_irp_evt", FALSE);

    pendInfo.irpStatus.Information = 0;
    pendInfo.irpStatus.Status = STATUS_SUCCESS;

    EmcpalIrpSetCompletionRoutine(pIrp, EmcpalIntIrpSendAndPendCompletionCallback, &pendInfo);

    status = IoCallDriver(pDeviceObject, pIrp);

    if (status == STATUS_PENDING)
    {
        status = EmcpalSyncEventWait(&pendInfo.event, timeout); 
        if (status == STATUS_TIMEOUT)
        {
            /* if we timeout we need to panic here */
            /* CX_PREP_TODO */            
        } 
        else
        {
            /* assert success */
        }
        status = pendInfo.irpStatus.Status;
    }

    if (Information)
    {
        *Information = (ULONG)pendInfo.irpStatus.Information;
    }

    EmcpalSyncEventDestroy(&pendInfo.event);
    
    return status;     
#endif /* EMCPAL_USE_CSX_DCALL */
}

#endif // PAL_FULL_IRP_AVAIL

#ifdef PAL_FULL_IRP_AVAIL
#include "EmcPAL_Irp_Extensions.h"
#endif

#ifdef __cplusplus
}
#endif

/*!
 * @} end group emcpal_irps
 * @} end file EmcPAL_Irp.h
 */
#endif /* _EMCPAL_IRP_H_ */
