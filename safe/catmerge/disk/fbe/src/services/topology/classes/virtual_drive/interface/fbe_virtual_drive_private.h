#ifndef VIRTUAL_DRIVE_PRIVATE_H
#define VIRTUAL_DRIVE_PRIVATE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_virtual_drive_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains private defines for the virtual drive object.
 *  This includes the definitions of the
 *  @ref fbe_virtual_drive_t "virtual_drive" itself, as well as the associated
 *  structures and defines.
 * 
 * @ingroup virtual_drive_class_files
 * 
 * @version
 *   05/20/2009:  Created. RPF
 *
 ***************************************************************************/

#include "fbe/fbe_transport.h"
#include "fbe_block_transport.h"
#include "fbe_mirror_private.h"
#include "fbe_raid_group_object.h"
#include "fbe_raid_group_rebuild.h"
#include "fbe/fbe_virtual_drive.h"
#include "fbe/fbe_physical_drive.h"
#include "fbe_spare.h"
#include "fbe/fbe_port.h"
#include "fbe_metadata.h"

/******************************
 * LITERAL DEFINITIONS.
 ******************************/
/*! @def FBE_VIRTUAL_DRIVE_NUM_EDGES
 *  @brief this is the (1) and only width supported for a virtual drive object
 */
#define FBE_VIRTUAL_DRIVE_NUM_EDGES (FBE_VIRTUAL_DRIVE_WIDTH)

/* Values for getting permission for an I/O operation */
/*! @todo - these are based on the example for the PVD which says the values probably will change 
 */
#define FBE_VIRTUAL_DRIVE_COPY_CPUS_PER_SECOND      500
#define FBE_VIRTUAL_DRIVE_COPY_MBYTES_CONSUMPTION     8

/******************************
 * STRUCTURE DEFINITIONS.
 ******************************/


/* Lifecycle definitions
 * Define the virtual_drive lifecycle's instance.
 */
extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(virtual_drive);

/* These are the lifecycle condition ids for a sas physical drive object. */

/*! @enum fbe_virtual_drive_lifecycle_cond_id_t  
 *  
 *  @brief These are the lifecycle conditions for the virtual_drive object. 
 */
typedef enum fbe_virtual_drive_lifecycle_cond_id_e 
{

    /*! The need replacement condition will be set when we have downstream 
     *  health is broken and we may need to replace the removed/failed drive with
     *  new drive.
     */
    FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_NEED_REPLACEMENT_DRIVE = FBE_LIFECYCLE_COND_FIRST_ID(FBE_CLASS_ID_VIRTUAL_DRIVE),

    /*! Need to evaluate the downstream health.
     */
    FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_EVALUATE_DOWNSTREAM_HEALTH,

    /*! If the virtual drive is in mirror mode it may need to verify the paged 
     *  metadata.
     */
    FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_SETUP_FOR_VERIFY_PAGED_METADATA,

    /*! If the virtual drive is in mirror mode it may need to verify the paged 
     *  metadata.
     */
    FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_VERIFY_PAGED_METADATA,

    /*! Need spare condition will be set to start the swap-in spare drive
     *  when we neeed to perform I/O error triggered proactive copy, user
     *  triggered proactive copy or configuration service triggered equalize
     *  operation.
     */
    FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_NEED_PROACTIVE_SPARE,

    /*! Job service has received a user copy request.  Must start this from 
     *  the monitor context.
     */
    FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_START_USER_COPY,

    /*! Swap in edge condition will swap-in an appropriate edge based on its current 
     *  state of the downstream health.
     */
    FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_SWAP_IN_EDGE,

    /*! Swap out edge condition will swap-out an appropriate edge based on its current 
     *  state of the downstream health.
     */
    FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_SWAP_OUT_EDGE,

    /*! The copy operation completed successfully.
     */
    FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_COPY_COMPLETE,

    /*! Either the source or destination drive has failed.  Abort the copy operation.
     */
    FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_ABORT_COPY,

    /*! The swap operation has completed
     */
    FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_SWAP_OPERATION_COMPLETE,

    /*! Copy operation failed.  Set rebuild checkpoints and rebuild logging etc.
     */
    FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_COPY_FAILED_SET_REBUILD_CHECKPOINT_TO_END_MARKER,

    /*! If required execute eval rebuild logging  */
    FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_EVAL_MARK_NR_IF_NEEDED_COND,

    /*! The virtual drive background operation condition (includes copy). 
     */
    FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_BACKGROUND_MONITOR_OPERATION,

    /*! It check if the copy operation is complete.  Additional work 
     *  (swap-out the source drive etc) to do.
     */
    FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_IS_COPY_COMPLETE,

    /*! This condition is used to allow to set different condition in fail rotary and if required to 
     *  go to activate state.
     */
    FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_FAIL_CHECK_DOWNSTREAM_HEALTH,

    /*! This condition is used to clear the `no spare reported' flag.  This flag was set to 
     *  not continually report `no spare found'.
     */
    FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_CLEAR_NO_SPARE_REPORTED,

    /*! This condition is used to report `copy denied' event.
     */
    FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_REPORT_COPY_DENIED,

	/*! check if VD needs to save power
     */
	FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_CHECK_POWER_SAVINGS,

    FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_LAST /* must be last */
} 
fbe_virtual_drive_lifecycle_cond_id_t;

/* Virtual drive flags. */
typedef enum fbe_virtual_drive_flags_e
{
    FBE_VIRTUAL_DRIVE_FLAG_NEED_REPLACEMENT_DRIVE       =   0x00000001,
    FBE_VIRTUAL_DRIVE_FLAG_NEED_PROACTIVE_COPY          =   0x00000002,
    FBE_VIRTUAL_DRIVE_FLAG_DRIVE_FAULT_DETECTED         =   0x00000004,
    FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS     =   0x00000008, /*! @note Indicates virtual drive is processing a swap job. */
    FBE_VIRTUAL_DRIVE_FLAG_SWAP_IN_EDGE                 =   0x00000010,
    FBE_VIRTUAL_DRIVE_FLAG_OPTIMAL_COMPLETE_COPY        =   0x00000020,
    FBE_VIRTUAL_DRIVE_FLAG_UPDATE_CONFIG_MODE           =   0x00000040,
    FBE_VIRTUAL_DRIVE_FLAG_HOLD_IN_FAIL                 =   0x00000080,
    FBE_VIRTUAL_DRIVE_FLAG_REQUEST_ABORT_COPY           =   0x00000100,
    FBE_VIRTUAL_DRIVE_FLAG_ABORT_COPY_REQUEST_DENIED    =   0x00000200,
    FBE_VIRTUAL_DRIVE_FLAG_ATTACH_EDGE_TIMEDOUT         =   0x00000400,
    FBE_VIRTUAL_DRIVE_FLAG_SET_NO_SPARE_REPORTED        =   0x00000800,
    FBE_VIRTUAL_DRIVE_FLAG_SPARE_REQUEST_DENIED         =   0x00001000,
    FBE_VIRTUAL_DRIVE_FLAG_PROACTIVE_REQUEST_DENIED     =   0x00002000,
    FBE_VIRTUAL_DRIVE_FLAG_USER_COPY_DENIED             =   0x00004000,
    FBE_VIRTUAL_DRIVE_FLAG_RAID_GROUP_BROKEN_REPORTED   =   0x00008000,
    FBE_VIRTUAL_DRIVE_FLAG_RAID_GROUP_DEGRADED_REPORTED =   0x00010000,
    FBE_VIRTUAL_DRIVE_FLAG_COPY_IN_PROGRESS_REPORTED    =   0x00020000,
    FBE_VIRTUAL_DRIVE_FLAG_COPY_SOURCE_DRIVE_DEGRADED   =   0x00040000,
    FBE_VIRTUAL_DRIVE_FLAG_COPY_MARK_VERIFY_IN_PROGRESS =   0x00080000,
    FBE_VIRTUAL_DRIVE_FLAG_CONFIRMATION_DISABLED        =   0x00100000,
    FBE_VIRTUAL_DRIVE_FLAG_USER_COPY_STARTED            =   0x00200000,
    FBE_VIRTUAL_DRIVE_FLAG_SWAP_IN_EDGE_COMPLETE        =   0x00400000,
    FBE_VIRTUAL_DRIVE_FLAG_SWAP_OUT_COMPLETE            =   0x00800000,
    FBE_VIRTUAL_DRIVE_FLAG_CHANGE_CONFIG_MODE_COMPLETE  =   0x01000000,
    FBE_VIRTUAL_DRIVE_FLAG_COPY_COMPLETE_SET_CHECKPOINT =   0x02000000,
    FBE_VIRTUAL_DRIVE_FLAG_COPY_FAILED_SET_CHECKPOINT   =   0x04000000,
    FBE_VIRTUAL_DRIVE_FLAG_CLEANUP_SWAP_REQUEST         =   0x08000000,
    FBE_VIRTUAL_DRIVE_FLAG_RAID_GROUP_DENIED_REPORTED   =   0x10000000,

    FBE_VIRTUAL_DRIVE_FLAG_OPERATION_COMPLETE_MASK      =  (FBE_VIRTUAL_DRIVE_FLAG_USER_COPY_STARTED            |
                                                            FBE_VIRTUAL_DRIVE_FLAG_SWAP_IN_EDGE_COMPLETE        |
                                                            FBE_VIRTUAL_DRIVE_FLAG_SWAP_OUT_COMPLETE            |
                                                            FBE_VIRTUAL_DRIVE_FLAG_CHANGE_CONFIG_MODE_COMPLETE  |
                                                            FBE_VIRTUAL_DRIVE_FLAG_COPY_COMPLETE_SET_CHECKPOINT |
                                                            FBE_VIRTUAL_DRIVE_FLAG_COPY_FAILED_SET_CHECKPOINT     )
} fbe_virtual_drive_flags_t;


enum fbe_virtual_drive_constants_e
{
    /*! Virtual drive object maximum non paged records. 
     */
    FBE_VIRTUAL_DRIVE_MAX_NONPAGED_RECORDS      =  1,

    /*! Optimal block size as 512 is considered as worstcase to support the 4K 
     * block size drives.
     */
    FBE_VIRTUAL_DRIVE_OPTIMAL_BLOCK_SIZE        =  512,

    /*! This is the default blocks represented per chunk and must match
     *  what has been set in the metadata.
     */
    FBE_VIRTUAL_DRIVE_CHUNK_SIZE                = 0x800, /* 2048 blocks. */

    /*! This is the number of metadata copies.
     */
    FBE_VIRTUAL_DRIVE_NUMBER_OF_METADATA_COPIES =  1,

    /*! This is the number of seconds between retries of a proactive copy 
     *  request.
     */
    FBE_VIRTUAL_DRIVE_RETRY_PROACTIVE_COPY_TRIGGER_SECONDS  = 30, 

};

/*!****************************************************************************
 *    
 * @struct fbe_virtual_drive_metadata_positions_s
 *  
 * @brief 
 *   This is the definition of the virtual drive metadata positions.
 *   This structure stores an location of the medatadata on drive for the
 *   virtual drive object.
 *   This gets initialized when virtual drive object gets created/initialized
 *   during initialization.
 ******************************************************************************/
typedef struct fbe_virtual_drive_metadata_positions_s
{
    /*! raid paged metadata address. 
     */
    fbe_lba_t           paged_metadata_lba;
    fbe_block_count_t   paged_metadata_block_count;

    /*! If metadata is mirrored this offset is used to calculate mirrored
     *  positions.
     */
    fbe_lba_t           paged_mirror_metadata_offset;
    
    /*! virtual drive object paged metadata size in blocks.
     */
    fbe_lba_t           paged_metadata_capacity; 

    /*! virtual drive non paged metadata address. 
     *@todo These two fields are depricated.
     */
    fbe_lba_t           nonpaged_metadata_lba;
    fbe_block_count_t   nonpaged_metadata_block_count;
} fbe_virtual_drive_metadata_positions_t;

/*! @todo obsolete definations, remove it once metadata calcuation gets 
 *  implemented for virtual drive object. 
 *  Currently no bitmaps.
 */
#define FBE_VIRTUAL_DRIVE_BITMAPS_BLOCKS    (0)
#define FBE_VIRTUAL_DRIVE_BITMAP_SIZE       (FBE_METADATA_BLOCK_SIZE * FBE_VIRTUAL_DRIVE_BITMAPS_BLOCKS)

/*!****************************************************************************
 *    
 * @struct fbe_virtual_drive_t
 *  
 * @brief 
 *   This is the definition of the virtual drive object.
 *   This object represents a virtual drive, which is an object which can
 *   perform block size conversions.
 ******************************************************************************/

typedef struct fbe_virtual_drive_s
{    
    /*! This must be first.  This is the mirror object that we inherit from.
     */
    fbe_mirror_t        spare;

    /*! This determines the configuration mode of the virtual drive object, this
     *  configuration mode details is used during control path and I/O path to 
     *  figure out the current state of the virtual drive object and based on that 
     *  it performs different operation. 
     */
    fbe_virtual_drive_configuration_mode_t configuration_mode;

    /*! This new configuration mode is used to store the new mode in memory 
     * until we commit the configuration and drain I/Os before we change the
     * the configuration mode on both the SPs. 
     */ 
    fbe_virtual_drive_configuration_mode_t new_configuration_mode;

    /*! Generic flags field.
     */
    fbe_virtual_drive_flags_t flags;

    /*! Edge that is being swapped in
     */
    fbe_edge_index_t swap_in_edge_index;

    /*! Edge that is being swapped out
     */
    fbe_edge_index_t swap_out_edge_index;

    /*! Need replacement drive start time.
     */
    fbe_time_t  need_replacement_drive_start_time;

    /*! Drive fault start time.
     */
    fbe_time_t  drive_fault_start_time;

    /*! User can initiate the external copy using fbecli and navi command.
    */
    fbe_spare_swap_command_t    copy_request_type;

    /*! Job status code 
     */
    fbe_u32_t   job_status;

    /*! Need proactive copy start time.
     */
    fbe_time_t  need_proactive_copy_start_time;

    fbe_spare_proactive_health_t proactive_copy_health;

    /*! This field will be used to store the PVD object id of the drive 
        in case of permanent sparing for notifying the admin shim
     */
    fbe_object_id_t     orig_pvd_obj_id;

    /*This flag is used to enable/disable the "Automatic configuration of
    * PVD as Spare" feature.
    */
    fbe_virtual_drive_unused_drive_as_spare_flag_t unused_drive_as_spare;

    /*! Debug flags to allow testing of different paths in the 
     *  virtual drive object.
     */
    fbe_virtual_drive_debug_flags_t debug_flags;   

    /* Lifecycle defines. */
    FBE_LIFECYCLE_DEF_INST_DATA;
    FBE_LIFECYCLE_DEF_INST_COND(FBE_LIFECYCLE_COND_MAX(FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_LAST));
}
fbe_virtual_drive_t;


/*!**************************************************************
 * fbe_virtual_drive_lock()
 ****************************************************************
 * @brief 
 *  Function which locks the virtual drive object.
 *
 * @param virtual_drive_p - The virtual drive object pointer.
 * 
 * @return - void
 *
 ****************************************************************/
static __forceinline void
fbe_virtual_drive_lock(fbe_virtual_drive_t *const virtual_drive_p)
{
    fbe_base_object_lock((fbe_base_object_t *)virtual_drive_p);
    return;
}

/*!**************************************************************
 * fbe_virtual_drive_unlock()
 ****************************************************************
 * @brief 
 *  Function which unlocks the virtual drive object lock.
 *
 * @param virtual_drive_p - The virtual drive object pointer.
 * 
 * @return - void
 *
 ****************************************************************/
static __forceinline void
fbe_virtual_drive_unlock(fbe_virtual_drive_t *const virtual_drive_p)
{
    fbe_base_object_unlock((fbe_base_object_t *)virtual_drive_p);
    return;
}

/*!**************************************************************
 * fbe_virtual_drive_init_flags()
 ****************************************************************
 * @brief 
 *  Function which initializes the virtual drive flags.
 *
 * @param virtual_drive_p - The virtual drive object pointer.
 * 
 * @return - void
 *
 ****************************************************************/
static __forceinline void
fbe_virtual_drive_init_flags(fbe_virtual_drive_t *const virtual_drive_p)
{
    virtual_drive_p->flags = 0;
    return;
}

/*!**************************************************************
 * fbe_virtual_drive_set_flag()
 ****************************************************************
 * @brief 
 *  Function which sets virtual drive flag.
 *
 * @param virtual_drive_p - The virtual drive object pointer.
 * @param flag            - Virtual drive flag which user wants to set.
 * 
 * @return - void
 *
 ****************************************************************/
static __forceinline void
fbe_virtual_drive_set_flag(fbe_virtual_drive_t *const virtual_drive_p,
                           const fbe_virtual_drive_flags_t flags)
{
    virtual_drive_p->flags |= flags;
    return;
}

/*!**************************************************************
 * fbe_virtual_drive_get_flags()
 ****************************************************************
 * @brief 
 *  Function which gets virtual drive flag.
 *
 * @param virtual_drive_p - The virtual drive object pointer.
 * @param flag_p          - Pointer to virtual drive flag .
 * 
 * @return - void
 *
 ****************************************************************/
static __forceinline void fbe_virtual_drive_get_flags(fbe_virtual_drive_t *const virtual_drive_p,
                                                      fbe_virtual_drive_flags_t *flags)
{
     *flags = virtual_drive_p->flags;
    return;
}


/*!**************************************************************
 * fbe_virtual_drive_clear_flag()
 ****************************************************************
 * @brief 
 *  Function which clears a virtual drive flag.
 *
 * @param virtual_drive_p - The virtual drive object pointer.
 * @param flag            - Virtual drive flag which user wants to clear.
 * 
 * @return - void
 *
 ****************************************************************/
static __forceinline void
fbe_virtual_drive_clear_flag(fbe_virtual_drive_t *const virtual_drive_p,
                             const fbe_virtual_drive_flags_t flags)
{
    virtual_drive_p->flags &= ~flags;
    return;
}

/*!**************************************************************
 * fbe_virtual_drive_is_flag_set()
 ****************************************************************
 * @brief 
 *  Function verifies whether virtual drive flag is set or not.
 *
 * @param virtual_drive_p - The virtual drive object pointer.
 * @param flag            - Virtual drive flag which user wants verify.
 * 
 * @return - void
 *
 ****************************************************************/
static __forceinline fbe_bool_t
fbe_virtual_drive_is_flag_set(fbe_virtual_drive_t *const virtual_drive_p,
                              const fbe_virtual_drive_flags_t flags)
{
    return((virtual_drive_p->flags & flags) != 0);
}

/*!**************************************************************
 * fbe_virtual_drive_set_swap_in_edge_index()
 ****************************************************************
 * @brief 
 *  Save the edge index that we are swapping in.
 *
 * @param virtual_drive_p - The virtual drive object pointer.
 * @param edge_to_swap - Edge index to swap-in or swap-out
 * 
 * @return - void
 *
 ****************************************************************/
static __forceinline void
fbe_virtual_drive_set_swap_in_edge_index(fbe_virtual_drive_t *const virtual_drive_p,
                                      const fbe_edge_index_t edge_to_swap)
{
    virtual_drive_p->swap_in_edge_index = edge_to_swap;
    return;
}

/*!**************************************************************
 * fbe_virtual_drive_get_swap_in_edge_index()
 ****************************************************************
 * @brief 
 *  Get the edge index that we are swapping in
 *
 * @param virtual_drive_p - The virtual drive object pointer.
 * @param edge_to_swap - Edge index to swap-in or swap-out
 * 
 * @return - void
 *
 ****************************************************************/
static __forceinline fbe_edge_index_t
fbe_virtual_drive_get_swap_in_edge_index(fbe_virtual_drive_t *const virtual_drive_p)
{
    return virtual_drive_p->swap_in_edge_index;
}

/*!**************************************************************
 * fbe_virtual_drive_clear_swap_in_edge_index()
 ****************************************************************
 * @brief 
 *  Clear the edge index that we are swapping in
 *
 * @param virtual_drive_p - The virtual drive object pointer.
 * 
 * @return - void
 *
 ****************************************************************/
static __forceinline void
fbe_virtual_drive_clear_swap_in_edge_index(fbe_virtual_drive_t *const virtual_drive_p)
{
    virtual_drive_p->swap_in_edge_index = FBE_EDGE_INDEX_INVALID;
    return;
}

/*!**************************************************************
 * fbe_virtual_drive_set_swap_out_edge_index()
 ****************************************************************
 * @brief 
 *  Save the edge index that we are swapping out.
 *
 * @param virtual_drive_p - The virtual drive object pointer.
 * @param edge_to_swap - Edge index to swap-out
 * 
 * @return - void
 *
 ****************************************************************/
static __forceinline void
fbe_virtual_drive_set_swap_out_edge_index(fbe_virtual_drive_t *const virtual_drive_p,
                                      const fbe_edge_index_t edge_to_swap)
{
    virtual_drive_p->swap_out_edge_index = edge_to_swap;
    return;
}

/*!**************************************************************
 * fbe_virtual_drive_get_swap_out_edge_index()
 ****************************************************************
 * @brief 
 *  Get the edge index that we are swapping out
 *
 * @param virtual_drive_p - The virtual drive object pointer.
 * @param edge_to_swap - Edge index to swap-in or swap-out
 * 
 * @return - void
 *
 ****************************************************************/
static __forceinline fbe_edge_index_t
fbe_virtual_drive_get_swap_out_edge_index(fbe_virtual_drive_t *const virtual_drive_p)
{
    return virtual_drive_p->swap_out_edge_index;
}

/*!**************************************************************
 * fbe_virtual_drive_clear_swap_out_edge_index()
 ****************************************************************
 * @brief 
 *  Clear the edge index that we are swapping out
 *
 * @param virtual_drive_p - The virtual drive object pointer.
 * 
 * @return - void
 *
 ****************************************************************/
static __forceinline void
fbe_virtual_drive_clear_swap_out_edge_index(fbe_virtual_drive_t *const virtual_drive_p)
{
    virtual_drive_p->swap_out_edge_index = FBE_EDGE_INDEX_INVALID;
    return;
}

/*!**************************************************************
 * fbe_virtual_drive_set_chunk_size()
 ****************************************************************
 * @brief 
 *  Set the chunk size of the virtual drive object.
 *
 * @param  virtual_drive_p - Virtual drive object pointer. 
 * @param chunk_size       - The chunk size.
 *
 * @return - void
 *
 ****************************************************************/
static __forceinline void 
fbe_virtual_drive_set_chunk_size(fbe_virtual_drive_t * virtual_drive_p,
                                 fbe_chunk_size_t  chunk_size)
{
    fbe_raid_group_set_chunk_size ((fbe_raid_group_t *) virtual_drive_p, chunk_size);
    return;

}

/*!*********************************************************************
 * fbe_virtual_drive_set_user_copy_type()
************************************************************************
 * @brief Set the user (external) copy type, currently we only supports external
 * Proactive Copy request.
 *
 * @param virtual_drive_p - Virtual drive object pointer.
 * @param copy_request_type - Requeested type of copy (Proactive Copy).
 *
 **********************************************************************/
static __forceinline void
fbe_virtual_drive_set_copy_request_type(fbe_virtual_drive_t *virtual_drive_p, 
                                        fbe_spare_swap_command_t copy_request_type)
{
    virtual_drive_p->copy_request_type = copy_request_type;
    return;
}

/*!**********************************************************************
 * fbe_virtual_drive_get_copy_request_type()
 ************************************************************************
 * @brief Get the virtual drive  copy (Ex:Proactive Copy) request type.
 *
 * @param virtual_drive_p - Virtual drive object pointer.
 * @param copy_request_type_p - Pointer to the copy request.
 *
 * @return - *user_copy_type: Pointer to theuser copy type.
 *
  ***********************************************************************/
static __forceinline void
fbe_virtual_drive_get_copy_request_type(fbe_virtual_drive_t *const virtual_drive_p,
                                        fbe_spare_swap_command_t *copy_request_type_p)
{
    *copy_request_type_p =  virtual_drive_p->copy_request_type;
    return;
}

/*!*********************************************************************
 * fbe_virtual_drive_set_job_status()
************************************************************************
 * @brief Set the job status in the virtual drive
 *
 * @param virtual_drive_p - Virtual drive object pointer.
 * @param job_status - The job status
 *
 **********************************************************************/
static __forceinline void fbe_virtual_drive_set_job_status(fbe_virtual_drive_t *virtual_drive_p, 
                                                           fbe_u32_t job_status)
{
    virtual_drive_p->job_status = job_status;
    return;
}

/*!**********************************************************************
 * fbe_virtual_drive_get_job_status()
 ************************************************************************
 * @brief Get the job status from the virtual drive
 *
 * @param virtual_drive_p - Virtual drive object pointer.
 * @param job_status_p - Pointer to job status to populate
 *
 * @return none
 *
  ***********************************************************************/
static __forceinline void fbe_virtual_drive_get_job_status(fbe_virtual_drive_t *const virtual_drive_p,
                                                           fbe_u32_t *job_status_p)
{
    *job_status_p =  virtual_drive_p->job_status;
    return;
}

static __forceinline fbe_bool_t fbe_virtual_drive_is_debug_flag_set(fbe_virtual_drive_t *virtual_drive_p,
                                                          fbe_virtual_drive_debug_flags_t debug_flags)
{
    if (virtual_drive_p == NULL)
    {
        return FBE_FALSE;
    }
    return ((virtual_drive_p->debug_flags & debug_flags) == debug_flags);
}

static __forceinline void fbe_virtual_drive_init_debug_flags(fbe_virtual_drive_t *virtual_drive_p,
                                                   fbe_virtual_drive_debug_flags_t debug_flags)
{
    virtual_drive_p->debug_flags = debug_flags;
    return;
}
static __forceinline void fbe_virtual_drive_get_debug_flags(fbe_virtual_drive_t *virtual_drive_p,
                                                  fbe_virtual_drive_debug_flags_t *debug_flags_p)
{
    *debug_flags_p = virtual_drive_p->debug_flags;
}
static __forceinline void fbe_virtual_drive_set_debug_flags(fbe_virtual_drive_t *virtual_drive_p,
                                                  fbe_virtual_drive_debug_flags_t debug_flags)
{
    virtual_drive_p->debug_flags = debug_flags;
    return;
}
static __forceinline void fbe_virtual_drive_set_debug_flag(fbe_virtual_drive_t *virtual_drive_p,
                                                fbe_virtual_drive_debug_flags_t debug_flags)
{
    virtual_drive_p->debug_flags |= debug_flags;
}

/*!**************************************************************
 * fbe_virtual_drive_init_orig_pvd_object_id()
 ****************************************************************
 * @brief 
 *  Function which initializes the pvd object id.
 *
 * @param virtual_drive_p - The virtual drive object pointer.
 * 
 * @return - void
 *
 ****************************************************************/
static __forceinline void
fbe_virtual_drive_init_orig_pvd_object_id(fbe_virtual_drive_t *const virtual_drive_p)
{
    virtual_drive_p->orig_pvd_obj_id = FBE_OBJECT_ID_INVALID;
    return;
}

/*!**************************************************************
 * fbe_virtual_drive_set_orig_pvd_object_id()
 ****************************************************************
 * @brief 
 *  Function which initializes the pvd object id.
 *
 * @param virtual_drive_p - The virtual drive object pointer.
 * 
 * @return - void
 *
 ****************************************************************/
static __forceinline void
fbe_virtual_drive_set_orig_pvd_object_id(fbe_virtual_drive_t *const virtual_drive_p, 
                                         fbe_object_id_t pvd_object_id)
{
    virtual_drive_p->orig_pvd_obj_id = pvd_object_id;
    return;
}

/*!**************************************************************
 * fbe_virtual_drive_get_orig_pvd_object_id()
 ****************************************************************
 * @brief 
 *  Function which initializes the pvd object id.
 *
 * @param virtual_drive_p - The virtual drive object pointer.
 * 
 * @return - void
 *
 ****************************************************************/
static __forceinline void
fbe_virtual_drive_get_orig_pvd_object_id(fbe_virtual_drive_t *const virtual_drive_p,
                                         fbe_object_id_t* pvd_object_id_p)
{
    *pvd_object_id_p = virtual_drive_p->orig_pvd_obj_id;
    return;
}

/*!**************************************************************
 * fbe_virtual_drive_set_flag()
 ****************************************************************
 * @brief 
 *  Function which sets virtual drive flag.
 *
 * @param virtual_drive_p - The virtual drive object pointer.
 * @param unused_drive_as_spare_flag - unused drive as spare which user wants to set.
 * 
 * @return - void
 *
 ****************************************************************/
static __forceinline void
fbe_virtual_drive_set_unused_drive_as_spare(fbe_virtual_drive_t *const virtual_drive_p,
                                            const fbe_virtual_drive_unused_drive_as_spare_flag_t unused_drive_as_spare_flag)
{
    virtual_drive_p->unused_drive_as_spare = unused_drive_as_spare_flag;
    return;
}

/*!**************************************************************
 * fbe_virtual_drive_get_flag()
 ****************************************************************
 * @brief 
 *  Function which gets virtual drive flag.
 *
 * @param virtual_drive_p - The virtual drive object pointer.
 * @param unused_drive_as_spare_flag - Pointer to unused drive as spare .
 * 
 * @return - void
 *
 ****************************************************************/
static __forceinline void
fbe_virtual_drive_get_unused_drive_as_spare(fbe_virtual_drive_t *const virtual_drive_p,
                                            fbe_virtual_drive_unused_drive_as_spare_flag_t * unused_drive_as_spare_flag_p)
{
     *unused_drive_as_spare_flag_p = virtual_drive_p->unused_drive_as_spare;
    return;
}
/*!**************************************************************
 * fbe_virtual_drive_init_unused_drive_as_spare_flag()
 ****************************************************************
 * @brief 
 *  Function which inits virtual drive flag.
 *
 * @param virtual_drive_p - The virtual drive object pointer.
 * 
 * 
 * @return - void
 *
 ****************************************************************/
static __forceinline void
fbe_virtual_drive_init_unused_drive_as_spare_flag(fbe_virtual_drive_t * virtual_drive_p, fbe_bool_t unused_drive_as_spare_flag)                                                    
{
    virtual_drive_p->unused_drive_as_spare.unused_drive_as_spare_flag = unused_drive_as_spare_flag;
     return;
}


/* Imports for visibility.
 */

/* fbe_virtual_drive_main.c */
fbe_status_t fbe_virtual_drive_metadata_io_entry(fbe_object_handle_t object_handle, 
                                                 fbe_packet_t * packet_p);
fbe_status_t fbe_virtual_drive_init(fbe_virtual_drive_t * const virtual_drive_p);
fbe_status_t fbe_virtual_drive_destroy( fbe_object_handle_t object_handle);
fbe_base_config_downstream_health_state_t fbe_virtual_drive_verify_downstream_health (fbe_virtual_drive_t * virtual_drive_p);
fbe_status_t fbe_virtual_drive_set_condition_based_on_downstream_health(fbe_virtual_drive_t *virtual_drive_p,
                                                                        fbe_base_config_downstream_health_state_t downstream_health_state,
                                                                        const char *caller);
static fbe_status_t fbe_virtual_drive_init_members(fbe_virtual_drive_t * const virtual_drive_p);
fbe_status_t fbe_virtual_drive_set_configuration_mode(fbe_virtual_drive_t * virtual_drive_p,
                                                      fbe_virtual_drive_configuration_mode_t configuration_mode);
fbe_status_t fbe_virtual_drive_get_configuration_mode(fbe_virtual_drive_t * virtual_drive_p,
                                                      fbe_virtual_drive_configuration_mode_t * configuration_mode_p);
fbe_status_t fbe_virtual_drive_get_new_configuration_mode(fbe_virtual_drive_t * virtual_drive_p,
                                                          fbe_virtual_drive_configuration_mode_t * new_configuration_mode_p);
fbe_status_t fbe_virtual_drive_set_new_configuration_mode(fbe_virtual_drive_t * virtual_drive_p,
                                                          fbe_virtual_drive_configuration_mode_t new_configuration_mode);
fbe_status_t fbe_virtual_drive_set_default_offset(fbe_virtual_drive_t * virtual_drive_p,
                                                  fbe_lba_t  new_default_offset);
fbe_bool_t fbe_virtual_drive_is_pass_thru_configuration_mode_set(fbe_virtual_drive_t * virtual_drive_p);
fbe_bool_t fbe_virtual_drive_is_mirror_configuration_mode_set(fbe_virtual_drive_t * virtual_drive_p);
void fbe_virtual_drive_get_default_debug_flags(fbe_virtual_drive_debug_flags_t *debug_flags_p);
fbe_status_t fbe_virtual_drive_set_default_debug_flags(fbe_virtual_drive_debug_flags_t new_default_debug_flags);
void fbe_virtual_drive_get_default_raid_group_debug_flags(fbe_raid_group_debug_flags_t *debug_flags_p);
fbe_status_t fbe_virtual_drive_set_default_raid_group_debug_flags(fbe_raid_group_debug_flags_t new_default_raid_group_debug_flags);
void fbe_virtual_drive_get_default_raid_library_debug_flags(fbe_raid_library_debug_flags_t *debug_flags_p);
fbe_status_t fbe_virtual_drive_set_default_raid_library_debug_flags(fbe_raid_library_debug_flags_t new_default_raid_library_debug_flags);
fbe_status_t fbe_virtual_drive_get_primary_edge_index(fbe_virtual_drive_t *virtual_drive_p,
                                                      fbe_edge_index_t *primary_edge_index_p);
fbe_status_t fbe_virtual_drive_get_secondary_edge_index(fbe_virtual_drive_t *virtual_drive_p,
                                                        fbe_edge_index_t *secondary_edge_index_p);
fbe_bool_t fbe_virtual_drive_is_downstream_drive_broken(fbe_virtual_drive_t *virtual_drive_p);
fbe_status_t fbe_virtual_drive_check_hook(fbe_virtual_drive_t *virtual_drive_p,
                                          fbe_u32_t state,
                                          fbe_u32_t substate,
                                          fbe_u64_t val2,
                                          fbe_scheduler_hook_status_t *status);
fbe_status_t fbe_virtual_drive_parent_mark_needs_rebuild_done_completion(fbe_packet_t *packet_p,
                                                     fbe_packet_completion_context_t in_context);
fbe_status_t fbe_virtual_drive_parent_mark_needs_rebuild_done_set_checkpoint_to_end_marker(fbe_virtual_drive_t *virtual_drive_p,
                                                                     fbe_packet_t *packet_p);
fbe_status_t fbe_virtual_drive_get_checkpoint_for_parent_raid_group(fbe_virtual_drive_t *virtual_drive_p, 
                                                                    fbe_raid_group_get_checkpoint_info_t *rg_server_obj_info_p,
                                                                    fbe_virtual_drive_configuration_mode_t configuration_mode,
                                                                    fbe_bool_t b_is_edge_swapped,
                                                                    fbe_bool_t b_is_copy_complete);
fbe_status_t fbe_virtual_drive_get_checkpoint_for_virtual_drive(fbe_virtual_drive_t *virtual_drive_p, 
                                                                fbe_raid_group_get_checkpoint_info_t *rg_server_obj_info_p,
                                                                fbe_virtual_drive_configuration_mode_t configuration_mode,
                                                                fbe_bool_t b_is_edge_swapped,
                                                                fbe_bool_t b_is_copy_complete);
fbe_status_t fbe_virtual_drive_set_swapped_bit_completion(fbe_packet_t *packet_p,
                                                          fbe_packet_completion_context_t in_context);
fbe_status_t fbe_virtual_drive_set_swapped_bit_set_checkpoint_to_end_marker(fbe_virtual_drive_t *virtual_drive_p,
                                                                            fbe_packet_t *packet_p);
fbe_status_t fbe_virtual_drive_parent_mark_needs_rebuild_done_get_edge_to_set_to_end_marker(fbe_virtual_drive_t *virtual_drive_p,
                                                                                            fbe_edge_index_t *edge_index_to_set_p);

/* fbe_virtual_drive_monitor.c */
fbe_status_t fbe_virtual_drive_monitor(fbe_virtual_drive_t * const virtual_drive_p, 
                                     fbe_packet_t * mgmt_packet_p);
fbe_status_t fbe_virtual_drive_monitor_load_verify(void);

fbe_status_t virtual_drive_need_permanent_spare_cond_completion(fbe_packet_t * packet_p,
                                                                fbe_packet_completion_context_t context);

fbe_status_t virtual_drive_need_proactive_spare_cond_completion(fbe_packet_t * packet_p,
                                                                fbe_packet_completion_context_t context);

fbe_status_t fbe_virtual_drive_quiesce_io_if_needed(fbe_virtual_drive_t *virtual_drive_p);

/* fbe_virtual_drive_usurper.c */
fbe_status_t fbe_virtual_drive_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);

/* fbe_virtual_drive_executor.c
 */
fbe_status_t fbe_virtual_drive_block_transport_entry(fbe_transport_entry_context_t context, fbe_packet_t * packet);

/* fbe_virtual_drive_event.c */
fbe_status_t fbe_virtual_drive_event_entry(fbe_object_handle_t object_handle, 
                                            fbe_event_type_t event_type,
                                            fbe_event_context_t event_context);
void fbe_virtual_drive_trace(fbe_virtual_drive_t *virtual_drive_p,
                             fbe_trace_level_t trace_level,
                             fbe_virtual_drive_debug_flags_t trace_debug_flags,
                             fbe_char_t * string_p, ...) __attribute__((format(__printf_func__,4,5)));
fbe_status_t fbe_virtual_drive_get_medic_action_priority(fbe_virtual_drive_t* virtual_drive_p,
                                                         fbe_medic_action_priority_t *medic_action_priority);
fbe_status_t fbe_virtual_drive_attribute_changed(fbe_virtual_drive_t *virtual_drive_p,  
                                                 fbe_block_edge_t *edge_p);

/* fbe_virtual_drive_swap.c */
fbe_status_t fbe_virtual_drive_swap_get_need_replacement_drive_start_time(fbe_virtual_drive_t *virtual_drive_p,
                                                                          fbe_time_t *need_replacement_drive_start_time_p);

fbe_status_t fbe_virtual_drive_swap_set_need_replacement_drive_start_time(fbe_virtual_drive_t *virtual_drive_p);

fbe_status_t fbe_virtual_drive_swap_init_need_replacement_drive_start_time(fbe_virtual_drive_t *virtual_drive_p);

fbe_status_t fbe_virtual_drive_swap_clear_need_replacement_drive_start_time(fbe_virtual_drive_t *virtual_drive_p);

fbe_status_t fbe_virtual_drive_swap_reset_need_replacement_drive_start_time(fbe_virtual_drive_t *virtual_drive_p);

fbe_status_t fbe_virtual_drive_swap_init_drive_fault_start_time(fbe_virtual_drive_t *virtual_drive_p);

fbe_status_t fbe_virtual_drive_swap_init_need_proactive_copy_start_time(fbe_virtual_drive_t *virtual_drive_p);

fbe_status_t fbe_virtual_drive_swap_set_need_proactive_copy_start_time(fbe_virtual_drive_t *virtual_drive_p);

fbe_status_t fbe_virtual_drive_swap_clear_need_proactive_copy_start_time(fbe_virtual_drive_t *virtual_drive_p);

fbe_status_t fbe_virtual_drive_swap_reset_need_proactive_copy_start_time(fbe_virtual_drive_t *virtual_drive_p);

fbe_status_t fbe_virtual_drive_swap_check_if_permanent_spare_needed(fbe_virtual_drive_t * virtual_drive_p, 
                                                                    fbe_bool_t * is_permanent_sparing_required_p);

fbe_status_t fbe_virtual_drive_swap_validate_permanent_spare_request(fbe_virtual_drive_t *virtual_drive_p,
                                                  fbe_virtual_drive_swap_in_validation_t *virtual_drive_swap_in_validation_p);

fbe_status_t fbe_virtual_drive_swap_validate_proactive_copy_request(fbe_virtual_drive_t *virtual_drive_p,
                                                  fbe_virtual_drive_swap_in_validation_t *virtual_drive_swap_in_validation_p);

fbe_status_t fbe_virtual_drive_can_we_initiate_replacement_request(fbe_virtual_drive_t * virtual_drive_p,
                                                                   fbe_bool_t * can_initiate_p);

fbe_status_t fbe_virtual_drive_check_and_send_notification(fbe_virtual_drive_t * virtual_drive_p);   

fbe_status_t fbe_virtual_drive_swap_check_if_permanent_sparing_can_start(fbe_virtual_drive_t * virtual_drive_p,
                                                                         fbe_bool_t * can_permanent_sparing_start_b_p);

fbe_status_t fbe_virtual_drive_swap_check_if_swap_out_can_start(fbe_virtual_drive_t * virtual_drive_p,
                                                                fbe_bool_t *b_can_swap_out_start_p);

fbe_status_t fbe_virtual_drive_swap_check_if_proactive_spare_needed(fbe_virtual_drive_t * virtual_drive_p,
                                                                    fbe_bool_t * is_ps_swap_in_required_b,
                                                                    fbe_edge_index_t * proactive_spare_edge_to_swap_in_p);

fbe_status_t fbe_virtual_drive_swap_check_if_user_copy_is_allowed(fbe_virtual_drive_t *virtual_drive_p,
                                                     fbe_virtual_drive_swap_in_validation_t *virtual_drive_swap_in_validation_p);

fbe_status_t fbe_virtual_drive_swap_is_proactive_spare_swapped_in(fbe_virtual_drive_t * virtual_drive_p,
                                                                  fbe_bool_t * is_proactive_spare_swapped_in_p);

fbe_status_t fbe_virtual_drive_initiate_swap_operation(fbe_virtual_drive_t * virtual_drive_p,
                                                       fbe_packet_t * packet_p,
                                                       fbe_edge_index_t  swap_edge_index,
                                                       fbe_edge_index_t  mirror_swap_edge_index,
                                                       fbe_spare_swap_command_t swap_command_type);

fbe_status_t fbe_virtual_drive_swap_change_configuration_mode(fbe_virtual_drive_t * virtual_drive_p);

fbe_status_t fbe_virtual_drive_swap_completion_cleanup(fbe_virtual_drive_t *virtual_drive_p,
                                                       fbe_packet_t *packet_p);

fbe_status_t fbe_virtual_drive_handle_swap_request_completion(fbe_virtual_drive_t *virtual_drive_p,
                                                      fbe_packet_t *packet_p);

fbe_status_t fbe_virtual_drive_swap_get_permanent_spare_edge_index(fbe_virtual_drive_t * virtual_drive_p,
                                                                   fbe_edge_index_t * permanent_spare_edge_index_p);


fbe_status_t fbe_virtual_drive_swap_get_swap_out_edge_index(fbe_virtual_drive_t * virtual_drive_p,
                                                            fbe_edge_index_t * edge_index_to_swap_out_p);

fbe_status_t fbe_virtual_drive_swap_find_broken_edge(fbe_virtual_drive_t *virtual_drive_p,
                                                     fbe_edge_index_t *broken_edge_index_p);

fbe_bool_t fbe_virtual_drive_swap_is_primary_edge_healthy(fbe_virtual_drive_t *virtual_drive_p);

fbe_bool_t fbe_virtual_drive_swap_is_primary_edge_broken(fbe_virtual_drive_t *virtual_drive_p);

fbe_bool_t fbe_virtual_drive_swap_is_secondary_edge_broken(fbe_virtual_drive_t *virtual_drive_p);

fbe_bool_t fbe_virtual_drive_swap_is_secondary_edge_healthy(fbe_virtual_drive_t *virtual_drive_p);

fbe_status_t fbe_virtual_drive_swap_validate_swap_out_request(fbe_virtual_drive_t *virtual_drive_p,
                                                              fbe_virtual_drive_swap_out_validation_t *virtual_drive_swap_out_validation_p);

fbe_status_t fbe_virtual_drive_swap_send_event_to_mark_needs_rebuild_completion(fbe_event_t * event_p,
                                                                                fbe_event_completion_context_t context);

fbe_status_t fbe_virtual_drive_send_swap_notification(fbe_virtual_drive_t* virtual_drive_p,
                                                      fbe_virtual_drive_configuration_mode_t configuration_mode);

fbe_status_t fbe_vitual_drive_get_server_object_id_for_edge(fbe_virtual_drive_t* virtual_drive_p,
                                                            fbe_edge_index_t edge_index,
                                                            fbe_object_id_t * object_id_p);

fbe_status_t fbe_virtual_drive_swap_ask_user_copy_permission(fbe_virtual_drive_t * virtual_drive_p,
                                                             fbe_packet_t * packet_p);

fbe_status_t fbe_virtual_drive_swap_log_permanent_spare_denied(fbe_virtual_drive_t *virtual_drive_p,
                                                               fbe_packet_t *packet_p);

fbe_status_t fbe_virtual_drive_swap_log_proactive_spare_denied(fbe_virtual_drive_t *virtual_drive_p,
                                                               fbe_packet_t *packet_p);

fbe_status_t fbe_virtual_drive_swap_log_user_copy_denied(fbe_virtual_drive_t *virtual_drive_p,
                                                         fbe_packet_t *packet_p);

fbe_status_t fbe_virtual_drive_swap_save_original_pvd_object_id(fbe_virtual_drive_t *virtual_drive_p);

fbe_status_t fbe_virtual_drive_swap_set_degraded_needs_rebuild_if_needed(fbe_virtual_drive_t *virtual_drive_p,
                                                                         fbe_packet_t *packet_p,
                                                                         fbe_base_config_downstream_health_state_t downstream_health_state);

fbe_status_t fbe_virtual_drive_swap_ask_copy_abort_go_degraded_permission(fbe_virtual_drive_t *virtual_drive_p,
                                                                          fbe_packet_t *packet_p);

fbe_bool_t fbe_virtual_drive_swap_set_copy_complete_in_progress_if_allowed(fbe_virtual_drive_t *virtual_drive_p);

fbe_bool_t fbe_virtual_drive_swap_set_abort_copy_in_progress_if_allowed(fbe_virtual_drive_t *virtual_drive_p);

fbe_status_t fbe_virtual_drive_swap_check_if_proactive_copy_can_start(fbe_virtual_drive_t *virtual_drive_p,
                                                                      fbe_bool_t *can_proactive_copy_start_b_p);

fbe_status_t fbe_virtual_drive_can_we_initiate_proactive_copy_request(fbe_virtual_drive_t *virtual_drive_p,
                                                                      fbe_bool_t *b_can_initiate_p);

fbe_status_t fbe_virtual_drive_set_swap_in_command(fbe_virtual_drive_t *virtual_drive_p);

fbe_status_t fbe_virtual_drive_set_swap_out_command(fbe_virtual_drive_t *virtual_drive_p);

fbe_status_t fbe_virtual_drive_handle_config_change(fbe_virtual_drive_t *virtual_drive_p);

/* fbe_virtual_drive_class.c */
fbe_status_t  fbe_class_virtual_drive_get_metadata_positions(fbe_lba_t edge_capacity, 
                                                             fbe_virtual_drive_metadata_positions_t * virtual_drive_metadata_positions);

fbe_status_t fbe_virtual_drive_class_control_entry(fbe_packet_t * packet);
fbe_status_t fbe_virtual_drive_get_permanent_spare_trigger_time(fbe_u64_t * permanent_spare_trigger_time_p);
void fbe_virtual_drive_initialize_default_system_spare_config(void);
void fbe_virtual_drive_teardown_default_system_spare_config(void);
fbe_status_t fbe_virtual_drive_class_set_resource_priority(fbe_packet_t * packet_p);



/* fbe_virtual_drive_metadata.c */
fbe_status_t fbe_virtual_drive_nonpaged_metadata_init(fbe_virtual_drive_t * virtual_drive, fbe_packet_t * packet);

fbe_status_t fbe_virtual_drive_get_metadata_positions(fbe_virtual_drive_t *  virtual_drive_p,
                                                      fbe_virtual_drive_metadata_positions_t * virtual_drive_metadata_position_p);

fbe_status_t fbe_virtual_drive_metadata_write_default_nonpaged_metadata(fbe_virtual_drive_t *virtual_drive_p,
                                                                        fbe_packet_t *packet_p);

fbe_status_t fbe_virtual_drive_metadata_is_mark_nr_required(fbe_virtual_drive_t *virtual_drive_p,
                                                            fbe_bool_t *b_is_mark_nr_required_p);

fbe_status_t fbe_virtual_drive_metadata_determine_nonpaged_flags(fbe_virtual_drive_t *virtual_drive_p,
                                                                 fbe_raid_group_np_metadata_flags_t *set_np_flags_p,
                                                                 fbe_bool_t b_modify_edge_swapped,
                                                                 fbe_bool_t b_set_clear_edged_swapped,
                                                                 fbe_bool_t b_modify_mark_nr_required,
                                                                 fbe_bool_t b_set_clear_mark_nr_required,
                                                                 fbe_bool_t b_modify_source_failed,
                                                                 fbe_bool_t b_set_clear_source_failed,
                                                                 fbe_bool_t b_modify_degraded_needs_rebuild,
                                                                 fbe_bool_t b_set_clear_degraded_needs_rebuild);

fbe_status_t fbe_virtual_drive_metadata_write_nonpaged_flags(fbe_virtual_drive_t *virtual_drive_p,
                                                             fbe_packet_t *packet_p,
                                                             fbe_raid_group_np_metadata_flags_t md_flags_to_write);

fbe_status_t fbe_virtual_drive_metadata_clear_edge_swapped(fbe_virtual_drive_t *virtual_drive_p,
                                                           fbe_packet_t *packet_p);

fbe_status_t fbe_virtual_drive_metadata_set_edge_swapped(fbe_virtual_drive_t *virtual_drive_p,
                                                         fbe_packet_t *packet_p);

fbe_status_t fbe_virtual_drive_set_swap_operation_start_nonpaged_flags(fbe_virtual_drive_t *virtual_drive_p,
                                                                       fbe_packet_t *packet_p);
                                        
fbe_status_t fbe_virtual_drive_metadata_is_edge_swapped(fbe_virtual_drive_t *virtual_drive_p,
                                                        fbe_bool_t *b_edge_swapped_p);                                 

fbe_status_t fbe_virtual_drive_metadata_clear_no_spare_been_reported(fbe_virtual_drive_t *virtual_drive_p,
                                                                     fbe_packet_t *packet_p);

fbe_status_t fbe_virtual_drive_metadata_set_no_spare_been_reported(fbe_virtual_drive_t *virtual_drive_p,
                                                                   fbe_packet_t *packet_p);

fbe_status_t fbe_virtual_drive_metadata_has_no_spare_been_reported(fbe_virtual_drive_t *virtual_drive_p,
                                                                   fbe_bool_t *b_has_been_reported_p);

fbe_status_t fbe_virtual_drive_metadata_clear_source_failed(fbe_virtual_drive_t *virtual_drive_p,
                                                            fbe_packet_t *packet_p);

fbe_status_t fbe_virtual_drive_metadata_set_source_failed(fbe_virtual_drive_t *virtual_drive_p,
                                                          fbe_packet_t *packet_p);

fbe_status_t fbe_virtual_drive_metadata_is_source_failed(fbe_virtual_drive_t *virtual_drive_p,
                                                         fbe_bool_t *b_is_source_failed_p);

fbe_status_t fbe_virtual_drive_metadata_set_degraded_needs_rebuild(fbe_virtual_drive_t *virtual_drive_p,
                                                          fbe_packet_t *packet_p);

fbe_status_t fbe_virtual_drive_metadata_is_degraded_needs_rebuild(fbe_virtual_drive_t *virtual_drive_p,
                                                                  fbe_bool_t *b_is_degraded_needs_rebuild_p);

fbe_status_t fbe_virtual_drive_metadata_set_mark_nr_required(fbe_virtual_drive_t *virtual_drive_p,
                                                             fbe_packet_t *packet_p);

fbe_status_t fbe_virtual_drive_metadata_set_checkpoint_to_end_marker_completion(
                                    fbe_packet_t*                   packet_p,
                                    fbe_packet_completion_context_t context_p);

/* fbe_virtual_drive_copy.c */
fbe_status_t fbe_virtual_drive_rebuild_process_io_completion(fbe_packet_t * packet_p,
                                                             fbe_packet_completion_context_t * context);

fbe_status_t fbe_virtual_drive_copy_send_event_to_check_lba(fbe_virtual_drive_t *virtual_drive_p,
                                                         fbe_raid_group_rebuild_context_t *rebuild_context_p,
                                                         fbe_packet_t *packet_p);

fbe_status_t fbe_virtual_drive_copy_send_io_request(fbe_virtual_drive_t *virtual_drive_p,
                                                    fbe_packet_t *packet_p,
                                                    fbe_bool_t b_queue_to_block_transport,
                                                    fbe_lba_t start_lba,
                                                    fbe_block_count_t block_count);

fbe_status_t fbe_virtual_drive_start_user_copy(fbe_virtual_drive_t *virtual_drive_p);

fbe_status_t fbe_virtual_drive_send_request_to_mark_consumed(fbe_virtual_drive_t * virtual_drive_p, fbe_packet_t * packet_p);

void fbe_virtual_drive_set_unused_as_spare_flag(fbe_bool_t flag);

fbe_status_t fbe_virtual_drive_logging_log_all_copies_complete(fbe_virtual_drive_t *virtual_drive_p,
                                                               fbe_packet_t *in_packet_p, 
                                                               fbe_u32_t in_position);

fbe_status_t fbe_virtual_drive_copy_handle_processing_on_edge_state_change(fbe_virtual_drive_t *virtual_drive_p,
                                    fbe_u32_t                                   in_position,
                                    fbe_path_state_t                            in_path_state,
                                    fbe_base_config_downstream_health_state_t   in_downstream_health);

fbe_status_t fbe_virtual_drive_copy_generate_notifications(fbe_virtual_drive_t *virtual_drive_p,
                                                           fbe_packet_t *packet_p);

fbe_status_t fbe_virtual_drive_copy_validate_swapped_out_edge_index(fbe_virtual_drive_t *virtual_drive_p);

fbe_status_t fbe_virtual_drive_copy_swap_out_validate_nonpaged_metadata(fbe_virtual_drive_t *virtual_drive_p,
                                                                        fbe_edge_index_t edge_index_to_swap_out);

fbe_bool_t fbe_virtual_drive_is_copy_complete(fbe_virtual_drive_t *virtual_drive_p);

fbe_status_t fbe_virtual_drive_copy_complete_set_checkpoint_to_end_marker(fbe_virtual_drive_t *virtual_drive_p,
                                                                          fbe_packet_t *packet_p);

fbe_status_t fbe_virtual_drive_dest_drive_failed_set_checkpoint_to_end_marker(fbe_virtual_drive_t *virtual_drive_p,
                                                                              fbe_packet_t *packet_p);

fbe_status_t fbe_virtual_drive_copy_failed_set_rebuild_checkpoint_to_end_marker(fbe_virtual_drive_t *virtual_drive_p,
                                                                                fbe_packet_t *packet_p);
fbe_status_t fbe_virtual_drive_usurper_get_control_buffer(fbe_virtual_drive_t *virtual_drive_p,
                                                          fbe_packet_t* in_packet_p,
                                                          fbe_payload_control_buffer_length_t in_buffer_length,
                                                          fbe_payload_control_buffer_t* out_buffer_pp);

#endif /* VIRTUAL_DRIVE_PRIVATE_H */

/*******************************
 * end fbe_virtual_drive_private.h
 *******************************/
