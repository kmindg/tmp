#ifndef FBE_JOB_SERVICE_SWAP_H
#define FBE_JOB_SERVICE_SWAP_H

#include "fbe_spare.h"
#include "fbe/fbe_virtual_drive.h"
#include "fbe_database.h"
#include "fbe/fbe_job_service_interface.h"

#define FBE_JOB_SERVICE_DRIVE_SWAP_TIMEOUT 120000 /*ms*/
#define FBE_JOB_SERVICE_DEFAULT_SPARE_OPERATION_CONFIRMATION_ENABLE (FBE_TRUE)
#define FBE_JOB_SERVICE_DRIVE_SWAP_TIMEOUT_SECONDS                  (FBE_JOB_SERVICE_DRIVE_SWAP_TIMEOUT / 1000)
#define FBE_JOB_SERVICE_DEFAULT_SPARE_OPERATION_TIMEOUT_SECONDS     ((FBE_JOB_SERVICE_DRIVE_SWAP_TIMEOUT_SECONDS < 70) ? 60 : (FBE_JOB_SERVICE_DRIVE_SWAP_TIMEOUT_SECONDS - 10))
#define FBE_JOB_SERVICE_MINIMUM_SPARE_OPERATION_TIMEOUT_SECONDS         10
#define FBE_JOB_SERVICE_MAXIMUM_SPARE_OPERATION_TIMEOUT_SECONDS        600

/*!*******************************************************************
 * @struct fbe_job_service_drive_swap_request_s
 *********************************************************************
 * @brief
 *  It defines job service request for FBE_JOB_CONTROL_CODE_DRIVE_SWAP
 *  control code.
 *  Only append members at the end is allowed. DO NOT modify
 *  the existing data members.
 *********************************************************************/
typedef struct fbe_job_service_drive_swap_request_s
{
    fbe_spare_swap_command_t                command_type;           // swap in or swap out commands.
    fbe_object_id_t                         vd_object_id;           // VD object id for the drive which fails.
    fbe_edge_index_t                        swap_edge_index;        // Edge index where we need to swap-in/swap-out.
    fbe_edge_index_t                        mirror_swap_edge_index; // Mirrored edge index of the edge we need to swap-in/swap-out.
    fbe_object_id_t                         orig_pvd_object_id;     // PVD object-id of the original drive.
    fbe_object_id_t                         spare_object_id;        // PVD object-id of the spare drive for swap-in.
    fbe_job_service_error_type_t            status_code;            // Job service error code.
    fbe_spare_internal_error_t              internal_error;         // internal error code  
    fbe_database_transaction_id_t           transaction_id;         // transaction id for interaction with configuration service.
    fbe_u64_t                               job_number;             // indicates the job number.
    fbe_bool_t                              b_operation_confirmation; // Each operation is `confirmed/acknowledged with the virtual drive
    fbe_bool_t                              b_is_proactive_copy;    // True if orginal request was a proactive copy
} fbe_job_service_drive_swap_request_t;

/*!*******************************************************************
 * @struct fbe_job_service_update_spare_library_config_request_s
 *********************************************************************
 * @brief
 *  It defines job service request for FBE_JOB_TYPE_UPDATE_SPARE_SERVICE_CONFIG
 *  control code.
 *********************************************************************/
typedef struct fbe_job_service_update_spare_library_config_request_s
{
    fbe_database_transaction_id_t           transaction_id;         /* transaction id for interaction with configuration service. */
    fbe_u64_t                               job_number;             /* indicates the job number. */
    fbe_bool_t                              b_disable_operation_confirmation; /* Disable the job to object change confirmation/synchronization */
    fbe_u32_t                               operation_timeout_secs; /* How to wait for each job operation to complete (in seconds) */
    fbe_bool_t                              b_set_default;          /* Set all configuration parameters to the default values */

} fbe_job_service_update_spare_library_config_request_t;

/*!****************************************************************************
 * fbe_job_service_drive_swap_request_init()
 ******************************************************************************
 * @brief 
 *   It initializes drive swap request with default values.
 *
 * @param drive_swap_request_p   - Job service swap request pointer.
 ******************************************************************************/
static __forceinline void
fbe_job_service_drive_swap_request_init(fbe_job_service_drive_swap_request_t *drive_swap_request_p)
{
    drive_swap_request_p->command_type = FBE_SPARE_SWAP_INVALID_COMMAND;
    drive_swap_request_p->vd_object_id = FBE_OBJECT_ID_INVALID;
    drive_swap_request_p->swap_edge_index = FBE_EDGE_INDEX_INVALID;
    drive_swap_request_p->mirror_swap_edge_index = FBE_EDGE_INDEX_INVALID;
    drive_swap_request_p->orig_pvd_object_id = FBE_OBJECT_ID_INVALID;
    drive_swap_request_p->spare_object_id = FBE_OBJECT_ID_INVALID;
    drive_swap_request_p->status_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;
    drive_swap_request_p->b_operation_confirmation = FBE_JOB_SERVICE_DEFAULT_SPARE_OPERATION_CONFIRMATION_ENABLE;
    drive_swap_request_p->b_is_proactive_copy = FBE_TRUE;
}

/*!****************************************************************************
 * fbe_job_service_drive_swap_request_set_swap_command_type()
 ******************************************************************************
 * @brief 
 *   Set the drive swap command type.
 *
 * @param  drive_swap_request_p   - pointer to to the job service swap-request.
 * @param  command_type           - swap command type (swap-in/swap-out).
 *
 ******************************************************************************/
static __forceinline void
fbe_job_service_drive_swap_request_set_swap_command_type(fbe_job_service_drive_swap_request_t *drive_swap_request_p,
                                                         const fbe_spare_swap_command_t command_type)
{
    drive_swap_request_p->command_type = command_type;
}

/*!****************************************************************************
 * fbe_job_service_drive_swap_request_get_swap_command_type()
 ******************************************************************************
 * @brief 
 *   Get the swap command type from the drive swap request.
 *
 * @param  drive_swap_request_p     - pointer to to the job service swap-request.
 * @return request_type_p           - Pointer to the swap command type.
 *
 ******************************************************************************/
static __forceinline void
fbe_job_service_drive_swap_request_get_swap_command_type(fbe_job_service_drive_swap_request_t *const drive_swap_request_p,
                                                         fbe_spare_swap_command_t * command_type_p)
{
    *command_type_p = drive_swap_request_p->command_type;
}

/*!****************************************************************************
 * fbe_job_service_drive_swap_request_set_virtual_drive_object_id()
 ******************************************************************************
 * @brief 
 *   Set the virtual drive object-id in job service swap request.
 *
 * @param drive_swap_request_p  - pointer to to the job service swap-request.
 * @param vd_object_id          - virtual drive object-id.
 *
 ******************************************************************************/
static __forceinline void
fbe_job_service_drive_swap_request_set_virtual_drive_object_id(fbe_job_service_drive_swap_request_t *drive_swap_request_p, 
                                                               const fbe_object_id_t vd_object_id)
{
    drive_swap_request_p->vd_object_id = vd_object_id;
}

/*!****************************************************************************
 * fbe_job_service_drive_swap_request_get_virtual_drive_object_id()
 ******************************************************************************
 * @brief 
 *   Get the virtual drive object-id from the job service swap request.
 *
 * @param drive_swap_request_p  - pointer to to the job service swap-request.
 * @return vd_object_id_p       - Virtual drive object-id pinter.
 *
 ******************************************************************************/
static __forceinline void
fbe_job_service_drive_swap_request_get_virtual_drive_object_id(fbe_job_service_drive_swap_request_t *const drive_swap_request_p,
                                                               fbe_object_id_t * vd_object_id_p)
{
    *vd_object_id_p = drive_swap_request_p->vd_object_id;
}

/*!****************************************************************************
 * fbe_job_service_drive_swap_request_set_edge_index()
 ******************************************************************************
 * @brief 
 *   Set the swap edge index which we want to swap-in/swap-out.
 *
 * @param drive_swap_request_p  - pointer to to the job service swap-request.
 * @param swap_edge_index       - swap edge index.
 *
 ******************************************************************************/
static __forceinline void
fbe_job_service_drive_swap_request_set_swap_edge_index(fbe_job_service_drive_swap_request_t *drive_swap_request_p, 
                                                       const fbe_edge_index_t swap_edge_index)
{
    drive_swap_request_p->swap_edge_index = swap_edge_index;
}

/*!****************************************************************************
 * fbe_job_service_drive_swap_request_get_edge_index()
 ******************************************************************************
 * @brief 
 *   Get the swap edge index (edge index on which we need to perform specific
 *   swap operation) from job service swap request.
 *
 * @param  drive_swap_request_p - pointer to to the job service swap-request.
 * @return swap_edge_index_p    - pointer to the swap edge index.
 *
 ******************************************************************************/
static __forceinline void
fbe_job_service_drive_swap_request_get_swap_edge_index(fbe_job_service_drive_swap_request_t *const drive_swap_request_p,
                                           fbe_edge_index_t * swap_edge_index_p)
{
    *swap_edge_index_p = drive_swap_request_p->swap_edge_index;
}

/*!****************************************************************************
 * fbe_job_service_drive_swap_request_set_mirror_swap_edge_index()
 ******************************************************************************
 * @brief 
 *   Set the mirror swap edge index of edge which we need to swap-in/swap-out.
 *
 * @param drive_swap_request_p      - pointer to to the job service swap-request.
 * @param mirror_swap_edge_index    - mirror swap edge index.
 *
 ******************************************************************************/
static __forceinline void
fbe_job_service_drive_swap_request_set_mirror_swap_edge_index(fbe_job_service_drive_swap_request_t *drive_swap_request_p, 
                                                              const fbe_edge_index_t mirror_swap_edge_index)
{
    drive_swap_request_p->mirror_swap_edge_index = mirror_swap_edge_index;
}

/*!****************************************************************************
 * fbe_job_service_drive_swap_request_get_mirror_swap_edge_index()
 ******************************************************************************
 * @brief 
 *   Get the mirrorswap edge index (mirror edge index of the swap operation)
 *   from job service swap request.
 *
 * @param  drive_swap_request_p     - pointer to to the job service swap-request.
 * @return mirror_swap_edge_index_p - pointer to the mirror swap edge index.
 *
 ******************************************************************************/
static __forceinline void
fbe_job_service_drive_swap_request_get_mirror_swap_edge_index(fbe_job_service_drive_swap_request_t *const drive_swap_request_p,
                                                              fbe_edge_index_t * mirror_swap_edge_index_p)
{
    *mirror_swap_edge_index_p = drive_swap_request_p->mirror_swap_edge_index;
}

/*!****************************************************************************
 * fbe_job_service_drive_swap_request_set_spare_object_id()
 ******************************************************************************
 * @brief 
 *   Set the spare object-id in job service swap-request.
 *
 * @param drive_swap_request_p  - pointer to to the job service swap-request.
 * @param spare_object_id       - selected spare object-id.
 *
 ******************************************************************************/
static __forceinline void
fbe_job_service_drive_swap_request_set_spare_object_id(fbe_job_service_drive_swap_request_t *drive_swap_request_p, 
                                                       const fbe_object_id_t spare_object_id)
{
    drive_swap_request_p->spare_object_id = spare_object_id;
}

/*!****************************************************************************
 * fbe_job_service_drive_swap_request_get_spare_object_id()
 ******************************************************************************
 * @brief 
 *   Get the spare object-id from job service swap request.
 *
 * @param drive_swap_request_p  - pointer to to the job service swap-request.
 * @return spare_object_id_p    - pointer to the spare object-id.
 *
 ******************************************************************************/
static __forceinline void
fbe_job_service_drive_swap_request_get_spare_object_id(fbe_job_service_drive_swap_request_t *const drive_swap_request_p,
                                                       fbe_object_id_t * spare_object_id_p)
{
    *spare_object_id_p = drive_swap_request_p->spare_object_id;
}

/*!****************************************************************************
 * fbe_job_service_drive_swap_request_set_original_pvd_object_id()
 ******************************************************************************
 * @brief 
 *   Set the spare object-id in job service swap-request.
 *
 * @param drive_swap_request_p  - pointer to to the job service swap-request.
 * @param spare_object_id       - selected spare object-id.
 *
 ******************************************************************************/
static __forceinline void
fbe_job_service_drive_swap_request_set_original_pvd_object_id(fbe_job_service_drive_swap_request_t *drive_swap_request_p, 
                                                              const fbe_object_id_t orig_pvd_object_id)
{
    drive_swap_request_p->orig_pvd_object_id = orig_pvd_object_id;
}

/*!****************************************************************************
 * fbe_job_service_drive_swap_request_get_original_pvd_object_id()
 ******************************************************************************
 * @brief 
 *   Get the spare object-id from job service swap request.
 *
 * @param drive_swap_request_p  - pointer to to the job service swap-request.
 * @return spare_object_id_p    - pointer to the spare object-id.
 *
 ******************************************************************************/
static __forceinline void
fbe_job_service_drive_swap_request_get_original_pvd_object_id(fbe_job_service_drive_swap_request_t *const drive_swap_request_p,
                                                              fbe_object_id_t * orig_pvd_object_id_p)
{
    *orig_pvd_object_id_p = drive_swap_request_p->orig_pvd_object_id;
}

/*!****************************************************************************
 * fbe_job_service_drive_swap_request_set_status()
 ******************************************************************************
 * @brief 
 *   Set the Drive swap request status.
 *
 * @param  drive_swap_request_p - pointer to to the job service swap-request.
 * @param  status_code          - Drive swap request job status.
 *
 ******************************************************************************/
static __forceinline void
fbe_job_service_drive_swap_request_set_status(fbe_job_service_drive_swap_request_t *drive_swap_request_p,
                                              const fbe_job_service_error_type_t status_code)
{
    drive_swap_request_p->status_code = status_code;
}

/*!****************************************************************************
 * fbe_job_service_drive_swap_request_get_swap_status()
 ******************************************************************************
 * @brief 
 *   Get the swap status from the drive swap request.
 *
 * @param  drive_swap_request_p - pointer to to the job service swap-request.
 * @return status_code_p        - Pointer to the drive swap request status.
 *
 ******************************************************************************/
static __forceinline void
fbe_job_service_drive_swap_request_get_status(fbe_job_service_drive_swap_request_t *const drive_swap_request_p,
                                              fbe_job_service_error_type_t * status_code_p)
{
    *status_code_p = drive_swap_request_p->status_code;
}

/*!****************************************************************************
 * fbe_job_service_drive_swap_request_set_internal_error()
 ******************************************************************************
 * @brief 
 *   Set the Drive swap internal error code.
 *
 * @param  drive_swap_request_p - pointer to to the job service swap-request.
 * @param  internal_error - Internal (a.k.a unexpected) error code
 *
 ******************************************************************************/
static __forceinline void
fbe_job_service_drive_swap_request_set_internal_error(fbe_job_service_drive_swap_request_t *drive_swap_request_p,
                                              const  fbe_spare_internal_error_t internal_error)
{
    drive_swap_request_p->internal_error = internal_error;
}

/*!****************************************************************************
 * fbe_job_service_drive_swap_request_get_internal_error()
 ******************************************************************************
 * @brief 
 *   Get the swap status from the drive swap request.
 *
 * @param  drive_swap_request_p - pointer to to the job service swap-request.
 * @return internal_error_p       - Pointer to internal error code
 *
 ******************************************************************************/
static __forceinline void
fbe_job_service_drive_swap_request_get_internal_error(fbe_job_service_drive_swap_request_t *const drive_swap_request_p,
                                              fbe_spare_internal_error_t *internal_error_p)
{
    *internal_error_p = drive_swap_request_p->internal_error;
}

/*!****************************************************************************
 * fbe_job_service_drive_swap_request_set_confirmation_enable()
 ******************************************************************************
 * @brief 
 *   Enable or disable the operation confirmation (i.e. job will either wait
 *   for virtual drive monitor or not)
 *
 * @param  drive_swap_request_p - pointer to to the job service swap-request.
 * @param  b_operation_confirmation_enable -
 * 
 * @return  none
 *
 ******************************************************************************/
static __forceinline void
fbe_job_service_drive_swap_request_set_confirmation_enable(fbe_job_service_drive_swap_request_t *drive_swap_request_p,
                                              const fbe_bool_t b_operation_confirmation_enable)
{
    drive_swap_request_p->b_operation_confirmation = b_operation_confirmation_enable;
}

/*!****************************************************************************
 * fbe_job_service_drive_swap_request_get_confirmation_enable()
 ******************************************************************************
 * @brief 
 *   Determine if operation confirmation is enabled or not (fro this request)
 *
 * @param  drive_swap_request_p - pointer to to the job service swap-request.
 * @param   b_operation_confirmation_enable_p - Pointer to operation confirmation
 *          enable to update.
 * 
 * @return  none
 *
 ******************************************************************************/
static __forceinline void
fbe_job_service_drive_swap_request_get_confirmation_enable(fbe_job_service_drive_swap_request_t *const drive_swap_request_p,
                                              fbe_bool_t *b_operation_confirmation_enable_p)
{
    *b_operation_confirmation_enable_p = drive_swap_request_p->b_operation_confirmation;
}

/*!****************************************************************************
 * fbe_job_service_drive_swap_request_set_is_proactive_copy()
 ******************************************************************************
 * @brief 
 *      Set to True if proactive copy (default) otherwise set to False (user copy)
 *
 * @param  drive_swap_request_p - pointer to to the job service swap-request.
 * @param  b_is_proactive_copy -
 * 
 * @return  none
 *
 ******************************************************************************/
static __forceinline void
fbe_job_service_drive_swap_request_set_is_proactive_copy(fbe_job_service_drive_swap_request_t *drive_swap_request_p,
                                                         const fbe_bool_t b_is_proactive_copy)
{
    drive_swap_request_p->b_is_proactive_copy = b_is_proactive_copy;
}

/*!****************************************************************************
 * fbe_job_service_drive_swap_request_get_is_proactive_copy()
 ******************************************************************************
 * @brief 
 *   Determine if the original request was a proactive copy operation or not.
 *
 * @param  drive_swap_request_p - pointer to to the job service swap-request.
 * @param  b_is_proactive_copy_p - Pointer to `proactive copy' or not value.
 * 
 * @return  none
 *
 ******************************************************************************/
static __forceinline void
fbe_job_service_drive_swap_request_get_is_proactive_copy(fbe_job_service_drive_swap_request_t *const drive_swap_request_p,
                                                         fbe_bool_t *b_is_proactive_copy_p)
{
    *b_is_proactive_copy_p = drive_swap_request_p->b_is_proactive_copy;
}

#endif /* FBE_JOB_SERVICE_SWAP_H */













