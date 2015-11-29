/**************************************************************************
 * Copyright (C) EMC Corporation 2008-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 **************************************************************************/

/***************************************************************************
 *  fbe_terminator_drive.c
 ***************************************************************************
 *
 *  Description
 *      terminator drive class
 *
 *
 *  History:
 *      06/24/08    guov    Created
 *
 ***************************************************************************/
#include "terminator_drive.h"
#include "terminator_simulated_disk.h"
#include "terminator_fc_drive.h"
#include "terminator_login_data.h"
#include "terminator_sas_io_api.h"
#include "terminator_enclosure.h"
#include "fbe_terminator_device_registry.h"
#include "fbe_terminator.h"
#include "terminator_board.h"
#include "terminator_virtual_phy.h"
#include "terminator_port.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe_simulated_drive.h"

/************* 
 * Literals
 *************/
#define FBE_TERMINATOR_DRIVE_DISK_STRING_LENGTH            16
#define FBE_TERMINATOR_DRIVE_DISK_BYTES_PER_CHUNK_INFO      2   /*! @note The provision drive chunk info size could change. */
#define FBE_TERMINATOR_DRIVE_DISK_CHUNK_SIZE            0x800
#define FBE_TERMINATOR_DRIVE_DISK_PAGED_METADATA_CAPACITY (FBE_TERMINATOR_DRIVE_DISK_CHUNK_SIZE * 2)

/************************ 
 * GLOBALS
 ************************/
terminator_simulated_drive_type_t terminator_simulated_drive_type = TERMINATOR_DEFAULT_SIMULATED_DRIVE_TYPE;
static terminator_drive_array_t   terminator_drive_array;
static fbe_terminator_drive_debug_flags_t terminator_drive_default_debug_flags = FBE_TERMINATOR_DRIVE_DEBUG_FLAG_NONE;

extern terminator_sas_drive_vpd_inquiry_page_0xf3_t default_drive_vpd_inquiry_f3_table;
extern terminator_sas_drive_vpd_inquiry_page_0xb2_t default_drive_vpd_inquiry_b2_table;
extern terminator_sas_drive_inq_data_t              default_drive_inquiry_table;
extern fbe_u8_t                                     default_mode_page_10[TERMINATOR_SCSI_MODE_PAGE_10_BYTE_SIZE];
extern fbe_u8_t                                     default_log_page_31[TERMINATOR_SCSI_LOG_PAGE_31_BYTE_SIZE];

/* local functions */
static fbe_status_t drive_fill_in_login_data(terminator_drive_t * self);

static int terminator_simulated_drive_class_initialized = 0; /* SAFEBUG - need to track this */

/*
 */
fbe_status_t terminator_simulated_drive_class_init(void)
{
    fbe_spinlock_init(&terminator_drive_array.drive_array_spinlock);
    terminator_drive_array.num_simulated_drives = 0;
    if (terminator_simulated_drive_type == TERMINATOR_SIMULATED_DRIVE_TYPE_LOCAL_FILE)
    {
        terminator_simulated_disk_local_file_init();
    }
    CSX_ASSERT_H_RDC(terminator_simulated_drive_class_initialized == 0);
    terminator_simulated_drive_class_initialized = 1;
    return FBE_STATUS_OK;
}

/*
 */
fbe_status_t terminator_simulated_drive_class_destroy(void)
{
    /* Since no memory is associated wiht this we log an error if there are
     * still drives in the array but we continue and always return success.
     */
    if (terminator_simulated_drive_class_initialized == 0) { goto skip_stuff; } /* SAFEBUG - prevent invalid work */
    //fbe_spinlock_lock(&terminator_drive_array.drive_array_spinlock); /* SAFEBUG - pointless */
    if (terminator_drive_array.num_simulated_drives != 0)
    {
        /* In case if drives are pulled and never reinserted then that drives 
         * are still remains in the array so remove that drives from array.
         */
        free_pulled_drive();

#ifndef ALAMOSA_WINDOWS_ENV
        /* cannot understand how we can cleanly terminate without doing this - something is amiss here
         */
        {
            fbe_u32_t drive_array_index;
            terminator_drive_t * leftover_drive = NULL;
            for (drive_array_index = 0; drive_array_index < FBE_SIMULATED_DRIVE_MAX_NUMBER_OF_DRIVES; drive_array_index++)
            {
                leftover_drive = terminator_drive_array.drive_array[drive_array_index];
                if (NULL != leftover_drive)
                {
                    drive_free(leftover_drive);
                }
            }
        }
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE */

        if (terminator_drive_array.num_simulated_drives != 0)
        {
            terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                             "%s Terminator drive array has %d entries. \n",
                             __FUNCTION__, terminator_drive_array.num_simulated_drives);
        }
    }
    //fbe_spinlock_unlock(&terminator_drive_array.drive_array_spinlock); /* SAFEBUG - pointless */

    fbe_spinlock_destroy(&terminator_drive_array.drive_array_spinlock);
    terminator_drive_array.num_simulated_drives = 0;
    if (terminator_simulated_drive_type == TERMINATOR_SIMULATED_DRIVE_TYPE_LOCAL_FILE)
    {
        terminator_simulated_disk_local_file_destroy();
    }
    terminator_simulated_drive_class_initialized = 0;

skip_stuff:;
    return FBE_STATUS_OK;
}

static fbe_status_t terminator_simulated_drive_set_debug_flags_for_range(fbe_u32_t first_term_drive_index, 
                                                                  fbe_u32_t last_term_drive_index,
                                                                  fbe_terminator_drive_debug_flags_t terminator_drive_debug_flags)
{
    fbe_u32_t   term_drive_array_index;
    fbe_bool_t  b_term_drive_found = FBE_FALSE;

    if (((first_term_drive_index != FBE_TERMINATOR_DRIVE_SELECT_ALL_DRIVES)   &&
         (first_term_drive_index >= FBE_SIMULATED_DRIVE_MAX_NUMBER_OF_DRIVES)    ) ||
        ((last_term_drive_index  != FBE_TERMINATOR_DRIVE_SELECT_NO_DRIVES)    &&
         (last_term_drive_index  > FBE_SIMULATED_DRIVE_MAX_NUMBER_OF_DRIVES)     )    )
    {
        terminator_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                         "%s First: %d or last: %d term drive index is larger than max: %d\n",
                         __FUNCTION__, first_term_drive_index, last_term_drive_index, (FBE_SIMULATED_DRIVE_MAX_NUMBER_OF_DRIVES - 1));
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Take the array lock so that it doesn't change.  Independant of the 
     * option walk the entire array.
     */
    fbe_spinlock_lock(&terminator_drive_array.drive_array_spinlock);
    for (term_drive_array_index = 0; term_drive_array_index < FBE_SIMULATED_DRIVE_MAX_NUMBER_OF_DRIVES; term_drive_array_index++)
    {
        /*! @note We take the journal lock to prevent the drive from being 
         *        deleted
         */
        if (terminator_drive_array.drive_array[term_drive_array_index] != NULL)
        {
            fbe_spinlock_lock(&terminator_drive_array.drive_array[term_drive_array_index]->journal_lock);
            if (first_term_drive_index == FBE_TERMINATOR_DRIVE_SELECT_ALL_DRIVES)
            {
                b_term_drive_found = FBE_TRUE;
                terminator_drive_array.drive_array[term_drive_array_index]->drive_debug_flags = terminator_drive_debug_flags;
            }
            else if ((last_term_drive_index  == FBE_TERMINATOR_DRIVE_SELECT_NO_DRIVES) &&
                     (term_drive_array_index == first_term_drive_index)                   )
            {
                b_term_drive_found = FBE_TRUE;
                terminator_drive_array.drive_array[term_drive_array_index]->drive_debug_flags = terminator_drive_debug_flags;
                fbe_spinlock_unlock(&terminator_drive_array.drive_array[term_drive_array_index]->journal_lock);
                break;
            }
            else if ((term_drive_array_index >= first_term_drive_index) &&
                     (term_drive_array_index <= last_term_drive_index)     )
            {
                b_term_drive_found = FBE_TRUE;
                terminator_drive_array.drive_array[term_drive_array_index]->drive_debug_flags = terminator_drive_debug_flags;
            }
            fbe_spinlock_unlock(&terminator_drive_array.drive_array[term_drive_array_index]->journal_lock);
        }
    } /* end for all drives */
    
    /* Set the global flags if applicable
     */
    if (first_term_drive_index == FBE_TERMINATOR_DRIVE_SELECT_ALL_DRIVES)
    {
        terminator_drive_default_debug_flags = terminator_drive_debug_flags;
    }

    /* If not found report a warning but it's not a failure
     */
    if (b_term_drive_found == FBE_FALSE)
    {
        terminator_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                         "%s First: %d last: %d term drive index not found in array with: %d drives\n",
                         __FUNCTION__, first_term_drive_index, last_term_drive_index, terminator_drive_array.num_simulated_drives);
    }
    fbe_spinlock_unlock(&terminator_drive_array.drive_array_spinlock);

    return FBE_STATUS_OK;
}

static fbe_status_t terminator_simulated_drive_set_debug_flags_for_bes(fbe_u32_t backend_bus_number, 
                                                                fbe_u32_t encl_number, 
                                                                fbe_u32_t slot_number,
                                                                fbe_terminator_drive_debug_flags_t terminator_drive_debug_flags)
{
    fbe_u32_t   term_drive_array_index;
    fbe_bool_t  b_term_drive_found = FBE_FALSE;

    if ((backend_bus_number >= FBE_SIMULATED_DRIVE_MAX_NUMBER_OF_DRIVES) ||
        (encl_number        >= FBE_SIMULATED_DRIVE_MAX_NUMBER_OF_DRIVES) ||
        (slot_number        >= FBE_SIMULATED_DRIVE_MAX_NUMBER_OF_DRIVES)    )
    {
        terminator_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                         "%s %d_%d_%d: is larger than max: %d\n",
                         __FUNCTION__, backend_bus_number, encl_number, slot_number, (FBE_SIMULATED_DRIVE_MAX_NUMBER_OF_DRIVES - 1));
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Take the array lock so that it doesn't change.  Independant of the 
     * option walk the entire array.
     */
    fbe_spinlock_lock(&terminator_drive_array.drive_array_spinlock);
    for (term_drive_array_index = 0; term_drive_array_index < FBE_SIMULATED_DRIVE_MAX_NUMBER_OF_DRIVES; term_drive_array_index++)
    {
        /*! @note We take the journal lock to prevent the drive from being 
         *        deleted
         */
        if (terminator_drive_array.drive_array[term_drive_array_index] != NULL)
        {
            fbe_spinlock_lock(&terminator_drive_array.drive_array[term_drive_array_index]->journal_lock);
            if ((terminator_drive_array.drive_array[term_drive_array_index]->backend_number == backend_bus_number) &&
                (terminator_drive_array.drive_array[term_drive_array_index]->encl_number    == encl_number)        &&
                (terminator_drive_array.drive_array[term_drive_array_index]->slot_number    == slot_number)           )
            {
                b_term_drive_found = FBE_TRUE;
                terminator_drive_array.drive_array[term_drive_array_index]->drive_debug_flags = terminator_drive_debug_flags;
                fbe_spinlock_unlock(&terminator_drive_array.drive_array[term_drive_array_index]->journal_lock);
                break;
            }
            fbe_spinlock_unlock(&terminator_drive_array.drive_array[term_drive_array_index]->journal_lock);
        }
    } /* end for all drives */

    /* If not found report a warning but it's not a failure
     */
    if (b_term_drive_found == FBE_FALSE)
    {
        terminator_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                         "%s %d_%d_%d: not found in array with: %d drives\n",
                         __FUNCTION__,  backend_bus_number, encl_number, slot_number, terminator_drive_array.num_simulated_drives);
    }
    fbe_spinlock_unlock(&terminator_drive_array.drive_array_spinlock);

    return FBE_STATUS_OK;
}

/*!***************************************************************************
 *          terminator_simulated_drive_set_debug_flags()
 *****************************************************************************
 *
 * @brief   Set the terminator drive debug flags according to select type.
 * 
 * @param   drive_select_type - Method to select drive type (all, fru_index for b/e/s)
 * @param   first_term_drive_index - FBE_TERMINATOR_DRIVE_SELECT_ALL_DRIVES -
 *              Select all drives. Otherwise select type defines this value.
 * @param   last_term_drive_index - Last index of terminator drive array to select
 * @param   backend_bus_number - Backend bus number
 * @param   encl_number - encl_number
 * @param   slot_number - slot number
 * @param   terminator_drive_debug_flags - Terminator debug flags to set
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  10/18/2011  Ron Proulx  - Created
 *
 *********************************************************************/
fbe_status_t terminator_simulated_drive_set_debug_flags(fbe_terminator_drive_select_type_t drive_select_type,
                                                        fbe_u32_t first_term_drive_index,
                                                        fbe_u32_t last_term_drive_index,
                                                        fbe_u32_t backend_bus_number,
                                                        fbe_u32_t encl_number,
                                                        fbe_u32_t slot_number,
                                                        fbe_terminator_drive_debug_flags_t terminator_drive_debug_flags)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /* First validate the parameters.  All traces are warnings since failing to 
     * set the debug flags is not fatal.  First validate the select type.  Only 
     * (1) bit can be set.
     */
    if ((drive_select_type != FBE_TERMINATOR_DRIVE_SELECT_TYPE_ALL_DRIVES)       &&
        (drive_select_type != FBE_TERMINATOR_DRIVE_SELECT_TYPE_TERM_DRIVE_INDEX) &&
        (drive_select_type != FBE_TERMINATOR_DRIVE_SELECT_TYPE_BUS_ENCL_SLOT)    &&
        (drive_select_type != FBE_TERMINATOR_DRIVE_SELECT_TYPE_TERM_DRIVE_RANGE)    )
    {
        terminator_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                         "%s Invalid drive select: 0x%x \n",
                         __FUNCTION__, drive_select_type);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now validate the debug flags.
     */
    if (terminator_drive_debug_flags > FBE_TERMINATOR_DRIVE_DEBUG_MAX_FLAGS)
    {
        terminator_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                         "%s Invalid debug flags: 0x%x \n",
                         __FUNCTION__, terminator_drive_debug_flags);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Switch based on the select type.
     */
    switch(drive_select_type)
    {
        case FBE_TERMINATOR_DRIVE_SELECT_TYPE_ALL_DRIVES:
            /* If all drives is selected we expected first terminator drive to be
             * `all drives'.
             */
            if (first_term_drive_index != FBE_TERMINATOR_DRIVE_SELECT_ALL_DRIVES)
            {
                terminator_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s First term drive index: %d not expected\n",
                                 __FUNCTION__, first_term_drive_index);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* Invoke method to set debug flags for a range of drives
             */
            status = terminator_simulated_drive_set_debug_flags_for_range(first_term_drive_index, 
                                                                          last_term_drive_index,
                                                                          terminator_drive_debug_flags);
            break;

        case FBE_TERMINATOR_DRIVE_SELECT_TYPE_TERM_DRIVE_INDEX:
            /* Select the drive at the specified index
             */
            if (first_term_drive_index >= FBE_SIMULATED_DRIVE_MAX_NUMBER_OF_DRIVES)
            {
                terminator_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s First term drive index: %d larger than max: %d\n",
                                 __FUNCTION__, first_term_drive_index, (FBE_SIMULATED_DRIVE_MAX_NUMBER_OF_DRIVES - 1));
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* Invoke method to set debug flags for a range of drives
             */
            status = terminator_simulated_drive_set_debug_flags_for_range(first_term_drive_index, 
                                                                          FBE_TERMINATOR_DRIVE_SELECT_NO_DRIVES,
                                                                          terminator_drive_debug_flags);
            break;

        case FBE_TERMINATOR_DRIVE_SELECT_TYPE_BUS_ENCL_SLOT:
            /* Select terminator drive at the specified bus/encl/slot
             */
            if ((backend_bus_number > FBE_SIMULATED_DRIVE_MAX_NUMBER_OF_DRIVES) ||
                (encl_number        > FBE_SIMULATED_DRIVE_MAX_NUMBER_OF_DRIVES) ||
                (slot_number        > FBE_SIMULATED_DRIVE_MAX_NUMBER_OF_DRIVES)    )
            {
                terminator_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s First %d_%d_%d larger than max: %d\n",
                                 __FUNCTION__, backend_bus_number, encl_number, slot_number, FBE_SIMULATED_DRIVE_MAX_NUMBER_OF_DRIVES);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* Invoke method to set debug flags for a b/e/s
             */
            status = terminator_simulated_drive_set_debug_flags_for_bes(backend_bus_number, 
                                                                        encl_number, 
                                                                        slot_number,
                                                                        terminator_drive_debug_flags);
            break;

        case FBE_TERMINATOR_DRIVE_SELECT_TYPE_TERM_DRIVE_RANGE:
            /* Select the drives specified by the range of indexes specified.
             */
            if ((first_term_drive_index >= FBE_SIMULATED_DRIVE_MAX_NUMBER_OF_DRIVES) ||
                (last_term_drive_index  >= FBE_SIMULATED_DRIVE_MAX_NUMBER_OF_DRIVES)    )
            {
                terminator_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s First: %d or last: %d term drive index is larger than max: %d\n",
                                 __FUNCTION__, first_term_drive_index, last_term_drive_index, (FBE_SIMULATED_DRIVE_MAX_NUMBER_OF_DRIVES - 1));
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* Invoke method to set debug flags for a range of drives
             */
            status = terminator_simulated_drive_set_debug_flags_for_range(first_term_drive_index, 
                                                                          last_term_drive_index,
                                                                          terminator_drive_debug_flags);
            break;

        default:
            /* It's IMPOSSIBLE to get here since we have checked for the four valid drive select type
             * at the beginning of this function.
             */
            terminator_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                             "%s Unexpected term drive select value: 0x%x\n",
                             __FUNCTION__, drive_select_type);
            return FBE_STATUS_GENERIC_FAILURE;

    } /* end of switch on drive select type */

    /* Always return the execution status
     */
    return status;
}

#if defined(UMODE_ENV) || defined(SIMMODE_ENV)
/*! @note All of the client methods return FBE_STATUS_PENDING !!! 
 *        Since they will be completed asynchronously.
 */
fbe_status_t terminator_simulated_drive_client_read(terminator_drive_t * drive,
                                                    fbe_lba_t lba,
                                                    fbe_block_count_t block_count,
                                                    fbe_block_size_t block_size,
                                                    fbe_u8_t * data_buffer,
                                                    void * context)
{
    terminator_sas_drive_info_t *info;

    info = (terminator_sas_drive_info_t *)base_component_get_attributes(&drive->base);

    simulated_drive_read(info->drive_serial_number, 
						data_buffer, 
						lba, 
						(fbe_u32_t)(block_count * block_size),
						context);

    /*! @note Must return pending*/
    return FBE_STATUS_PENDING;
}

fbe_status_t terminator_simulated_drive_client_write(terminator_drive_t * drive,
                                                     fbe_lba_t lba,
                                                     fbe_block_count_t block_count,
                                                     fbe_block_size_t block_size,
                                                     fbe_u8_t * data_buffer,
                                                     void * context)
{
    terminator_sas_drive_info_t *info;

    info = (terminator_sas_drive_info_t *)base_component_get_attributes(&drive->base);
    simulated_drive_write(info->drive_serial_number, 
						data_buffer, 
						lba, 
						(fbe_u32_t)(block_count * block_size),
						context);

    /*! @note Must return pending*/
    return FBE_STATUS_PENDING;
}

fbe_status_t terminator_simulated_drive_client_write_zero_pattern(terminator_drive_t * drive,
                                                                  fbe_lba_t lba,
                                                                  fbe_block_count_t block_count,
                                                                  fbe_block_size_t block_size,
                                                                  fbe_u8_t * data_buffer,
                                                                  void * context)
{
    terminator_sas_drive_info_t *info;
    fbe_u32_t bytes;

    info = (terminator_sas_drive_info_t *)base_component_get_attributes(&drive->base);

    if((block_count * block_size) > FBE_U32_MAX)
    {
       /* we are crossing 32bit limit here, not expected here
         */
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* The size of the buffer (by definition of a SCSI WRITE SAME) is the
     * block size.  The repeat_count (last parameter) is the simply the 
     * number of blocks to write with the write same buffer. 
     */
    bytes = simulated_drive_write_same(info->drive_serial_number, 
										data_buffer, 
										(fbe_lba_t)lba,
										(fbe_u32_t)block_size,
										(fbe_u32_t)block_count,
										context);

    /*! @note Must return pending*/
    return FBE_STATUS_PENDING;
}

fbe_status_t terminator_simulated_drive_destroy(terminator_drive_t * drive)
{
    terminator_sas_drive_info_t *info;
    fbe_bool_t b_destroyed;

    info = (terminator_sas_drive_info_t *)base_component_get_attributes(&drive->base);
    b_destroyed = simulated_drive_remove(info->drive_serial_number);

    /*! @note Must return pending*/
    return (b_destroyed) ? FBE_STATUS_OK : FBE_STATUS_GENERIC_FAILURE;
}
#endif // UMODE_ENV || SIMMODE_ENV

terminator_drive_t * drive_new(void)
{
    terminator_drive_t * new_drive = NULL;
    fbe_u32_t       drive_array_index;

    fbe_spinlock_lock(&terminator_drive_array.drive_array_spinlock);
    for (drive_array_index = 0; drive_array_index < FBE_SIMULATED_DRIVE_MAX_NUMBER_OF_DRIVES; drive_array_index++)
    {
        if (terminator_drive_array.drive_array[drive_array_index] == NULL)
        {
            break;
        }
    }
    if (drive_array_index >= FBE_SIMULATED_DRIVE_MAX_NUMBER_OF_DRIVES)
    {
        terminator_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s Unexpected simulated drive count: %d\n", 
                         __FUNCTION__, terminator_drive_array.num_simulated_drives);
        fbe_spinlock_unlock(&terminator_drive_array.drive_array_spinlock);
        return NULL;
    }

    new_drive = (terminator_drive_t *)fbe_terminator_allocate_memory(sizeof(terminator_drive_t));
    if (new_drive == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s Failed to allocate memory simulated drive count: %d at line %d.\n",
                         __FUNCTION__,
                         terminator_drive_array.num_simulated_drives,
                         __LINE__);

        fbe_spinlock_unlock(&terminator_drive_array.drive_array_spinlock);

        return NULL;
    }

    fbe_zero_memory(new_drive, sizeof(terminator_drive_t));

    base_component_init(&new_drive->base);
    base_component_set_component_type(&new_drive->base, TERMINATOR_COMPONENT_TYPE_DRIVE);
    base_component_set_reset_function(&new_drive->base, terminator_miniport_device_reset_common_action);
    drive_clear_reset_count(new_drive);
    drive_clear_error_count(new_drive);

	new_drive->drive_handle = FBE_TERMINATOR_DRIVE_HANDLE_INVALID; /* Invalid handle */
	fbe_zero_memory(new_drive->drive_identity,FBE_TERMINATOR_DRIVE_IDENTITY_SIZE);

    /* Initialize journal lock and queue */
    fbe_spinlock_init(&new_drive->journal_lock);
    fbe_queue_init(&new_drive->journal_queue_head);
    new_drive->journal_num_records = 0;
    new_drive->journal_blocks_allocated = 0;

    new_drive->drive_debug_flags = terminator_drive_default_debug_flags;
    terminator_drive_array.drive_array[drive_array_index] = new_drive;
    terminator_drive_array.num_simulated_drives++;
    fbe_spinlock_unlock(&terminator_drive_array.drive_array_spinlock);
    return new_drive;
}

fbe_status_t drive_free(terminator_drive_t * drive)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       drive_array_index;
    fbe_bool_t   b_drive_found = FBE_FALSE;

    /* Destroy the journal
     */
    status = drive_journal_destroy(drive);
    if (status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s Failed to destroy journal with status: 0x%x\n",
                         __FUNCTION__, status);
    }

    fbe_spinlock_lock(&terminator_drive_array.drive_array_spinlock);
    for (drive_array_index = 0; drive_array_index < FBE_SIMULATED_DRIVE_MAX_NUMBER_OF_DRIVES; drive_array_index++)
    {
        if (terminator_drive_array.drive_array[drive_array_index] == drive)
        {
            b_drive_found = FBE_TRUE;
            break;
        }
    }
    if (b_drive_found == FBE_FALSE)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s Failed to locate 0x%p in drive array.\n",
                         __FUNCTION__, drive);
    }
    else
    {
        terminator_drive_array.drive_array[drive_array_index] = NULL;
        terminator_drive_array.num_simulated_drives--;
    }
    fbe_spinlock_unlock(&terminator_drive_array.drive_array_spinlock);

#if defined(UMODE_ENV) || defined(SIMMODE_ENV)
    if (terminator_simulated_drive_type == TERMINATOR_SIMULATED_DRIVE_TYPE_REMOTE_MEMORY)
    {
        terminator_simulated_drive_destroy(drive);
    }
#endif // UMODE_ENV || SIMMODE_ENV

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * free_pulled_drive()
 ****************************************************************
 * @brief
 *  In case if drives are pulled and never reinserted then that 
 *  drives are still remains in the terminator drive array so
 *  remove that drives from array.
 *
 * @param void
 *
 * @return void
 *
 * @version
 *  02/13/12 - Created. 
 *
 ****************************************************************/
fbe_status_t free_pulled_drive(void)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       drive_array_index;

    fbe_spinlock_lock(&terminator_drive_array.drive_array_spinlock);
    
    for (drive_array_index = 0; drive_array_index < FBE_SIMULATED_DRIVE_MAX_NUMBER_OF_DRIVES; drive_array_index++)
    {
        /* Check drive pulled status.
         */
        if(terminator_drive_array.is_drive_pulled[drive_array_index])
        {
            /* Destroy the journal
            */
            status = drive_journal_destroy(terminator_drive_array.drive_array[drive_array_index]);

            if (status != FBE_STATUS_OK)
            {
                terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Failed to destroy journal with status: 0x%x\n",
                                __FUNCTION__, status);
            }
            else
            {
                terminator_drive_array.drive_array[drive_array_index] = NULL;
                terminator_drive_array.num_simulated_drives--;
                terminator_drive_array.is_drive_pulled[drive_array_index] = FBE_FALSE;
            }
        }
    }
    
    fbe_spinlock_unlock(&terminator_drive_array.drive_array_spinlock);
    
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * terminator_simulated_drive_set_drive_pulled_flag_status()
 ****************************************************************
 * @brief
 *  This function set is_drive_pulled flag status of specified drive.
 *
 * @param drive - Pointer to terminator_drive_t
 * @param drive_pulled_flag_status - FBE_TRUE if drive is pulled.
                FBE_FALSE if drive is reinserted.
 *
 * @return 
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 *  @version
 *  02/13/12 - Created. 
 *
 ****************************************************************/
fbe_status_t terminator_simulated_drive_set_drive_pulled_flag_status(terminator_drive_t * drive, fbe_bool_t drive_pulled_flag_status)
{
    fbe_u32_t       drive_array_index;
    fbe_bool_t      b_drive_found = FBE_FALSE;

    fbe_spinlock_lock(&terminator_drive_array.drive_array_spinlock);
    for (drive_array_index = 0; drive_array_index < FBE_SIMULATED_DRIVE_MAX_NUMBER_OF_DRIVES; drive_array_index++)
    {
        if (terminator_drive_array.drive_array[drive_array_index] == drive)
        {
            b_drive_found = FBE_TRUE;
            break;
        }
    }
    if (b_drive_found == FBE_FALSE)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s Failed to locate 0x%p in drive array.\n",
                         __FUNCTION__, drive);
    }
    else
    {
        terminator_drive_array.is_drive_pulled[drive_array_index] = drive_pulled_flag_status;
    }
    fbe_spinlock_unlock(&terminator_drive_array.drive_array_spinlock);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * set_terminator_simulated_drive_flag_to_drive_pulled()
 ****************************************************************
 * @brief
 *  This function set is_drive_pulled flag status of terminator
 *  drive array to FBE_TRUE.
 *
 * @param self - Pointer to base_component_t
 *
 * @return 
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 *  @version
 *  02/13/12 - Created. 
 *
 ****************************************************************/
fbe_status_t set_terminator_simulated_drive_flag_to_drive_pulled(base_component_t * self)
{
    fbe_bool_t is_drive_pulled = FBE_TRUE;
    return terminator_simulated_drive_set_drive_pulled_flag_status((terminator_drive_t*)self, is_drive_pulled);
}

/*!**************************************************************
 * set_terminator_simulated_drive_flag_to_drive_reinserted()
 ****************************************************************
 * @brief
 *  This function set is_drive_pulled flag status of terminator
 *  drive array to FBE_FALSE.
 *
 * @param self - Pointer to base_component_t
 *
 * @return 
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 *  @version
 *  02/13/12 - Created. 
 *
 ****************************************************************/
fbe_status_t set_terminator_simulated_drive_flag_to_drive_reinserted(base_component_t * self)
{
    fbe_bool_t is_drive_pulled = FBE_FALSE;
    return terminator_simulated_drive_set_drive_pulled_flag_status((terminator_drive_t*)self, is_drive_pulled);
}

/* allocates and initializes a structure based on type of drive */
fbe_status_t terminator_drive_initialize_protocol_specific_data(terminator_drive_t * drive_handle)
{
    void * attributes = NULL;
    /* obtain protocol */
    fbe_drive_type_t drive_type = drive_get_protocol(drive_handle);
    /* allocate memory for attributes - this depends on type of drive */
    switch(drive_type)
    {
        case FBE_DRIVE_TYPE_SAS:
        case FBE_DRIVE_TYPE_SAS_FLASH_HE:
        case FBE_DRIVE_TYPE_SAS_FLASH_ME:
        case FBE_DRIVE_TYPE_SAS_FLASH_LE:
        case FBE_DRIVE_TYPE_SAS_FLASH_RI:
            if((attributes = allocate_sas_drive_info()) == NULL)
            {
                return FBE_STATUS_GENERIC_FAILURE;
            }
            break;
        case FBE_DRIVE_TYPE_SATA:
        case FBE_DRIVE_TYPE_SATA_FLASH_HE:
            if((attributes = allocate_sata_drive_info()) == NULL)
            {
                return FBE_STATUS_GENERIC_FAILURE;
            }
            break;
        case FBE_DRIVE_TYPE_FIBRE:
            if((attributes = allocate_fc_drive_info()) == NULL)
            {
                return FBE_STATUS_GENERIC_FAILURE;
            }
            break; 
        default:
            /* fibre and other drives are not supported now */
            return FBE_STATUS_GENERIC_FAILURE;
    }
    base_component_assign_attributes(&drive_handle->base, attributes);
    return FBE_STATUS_OK;
}

terminator_sas_drive_info_t * allocate_sas_drive_info()
{
    terminator_sas_drive_info_t   *new_info   = NULL;
    fbe_cpd_shim_callback_login_t *login_data = NULL;

    new_info = (terminator_sas_drive_info_t *)fbe_terminator_allocate_memory(sizeof(terminator_sas_drive_info_t));
    if (!new_info)
    {
        return NULL;
    }

    new_info->drive_type = FBE_SAS_DRIVE_INVALID;

    login_data = sas_drive_info_get_login_data(new_info);
    cpd_shim_callback_login_init(login_data);
    cpd_shim_callback_login_set_device_type(login_data, FBE_PMC_SHIM_DEVICE_TYPE_SSP);

    fbe_zero_memory(&new_info->drive_serial_number, sizeof(new_info->drive_serial_number));
    new_info->backend_number = -1;
    new_info->encl_number = -1;
    new_info->slot_number    = INVALID_SLOT_NUMBER;
    new_info->drive_state = TERMINATOR_SAS_DRIVE_STATE_OK;
    new_info->miniport_sas_device_table_index = INVALID_TMSDT_INDEX;
    fbe_zero_memory(&new_info->product_id, sizeof(new_info->product_id));
   
    fbe_zero_memory(new_info->inquiry, sizeof(new_info->inquiry));
    fbe_zero_memory(new_info->vpd_inquiry_f3, sizeof(new_info->vpd_inquiry_f3));    
    fbe_zero_memory(new_info->mode_page, sizeof(new_info->mode_page));
    fbe_zero_memory(new_info->log_page_31, sizeof(new_info->log_page_31));

    return new_info;
}

terminator_sas_drive_info_t * sas_drive_info_new(fbe_terminator_sas_drive_info_t *sas_drive_info)
{
    fbe_cpd_shim_callback_login_t *login_data = NULL;
    terminator_sas_drive_info_t   *new_info   = (terminator_sas_drive_info_t *)fbe_terminator_allocate_memory(sizeof(terminator_sas_drive_info_t));
    if (!new_info)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s failed to allocate memory for sas drive info at line %d.\n",
                         __FUNCTION__,
                         __LINE__);

        return NULL;
    }

    new_info->drive_type = sas_drive_info->drive_type;

    login_data = sas_drive_info_get_login_data(new_info);
    cpd_shim_callback_login_init(login_data);
    cpd_shim_callback_login_set_device_address(login_data, sas_drive_info->sas_address);
    cpd_shim_callback_login_set_device_type(login_data, FBE_PMC_SHIM_DEVICE_TYPE_SSP);

    fbe_zero_memory(&new_info->drive_serial_number, sizeof(new_info->drive_serial_number));
    memcpy(new_info->drive_serial_number, sas_drive_info->drive_serial_number, sizeof(sas_drive_info->drive_serial_number));
    new_info->backend_number = sas_drive_info->backend_number;
    new_info->encl_number    = sas_drive_info->encl_number;

    /* slot_number will be set explicitly in sas_enclosure_insert_sas_drive()
     */
    //new_info->slot_number = sas_drive_info->slot_number;

    new_info->drive_state = TERMINATOR_SAS_DRIVE_STATE_OK;

    new_info->miniport_sas_device_table_index = INVALID_TMSDT_INDEX;

    fbe_zero_memory(&new_info->product_id, sizeof(new_info->product_id));
    memcpy(new_info->product_id, sas_drive_info->product_id, sizeof(sas_drive_info->product_id));

    fbe_zero_memory(new_info->inquiry, sizeof(new_info->inquiry));
    fbe_zero_memory(new_info->vpd_inquiry_f3, sizeof(new_info->vpd_inquiry_f3));    
    fbe_zero_memory(new_info->mode_page, sizeof(new_info->mode_page));
    fbe_zero_memory(new_info->log_page_31, sizeof(new_info->log_page_31));

    return new_info;
}

fbe_cpd_shim_callback_login_t * sas_drive_info_get_login_data(terminator_sas_drive_info_t * self)
{
    return (&self->login_data);
}

void sas_drive_set_drive_type(terminator_drive_t * self, fbe_sas_drive_type_t type)
{
    terminator_sas_drive_info_t * info = NULL;
    terminator_sas_drive_inq_data_t *inq_data = NULL;
    terminator_sas_drive_vpd_inquiry_page_0xf3_t *vpd_inq_f3 = NULL;
    terminator_sas_drive_vpd_inquiry_page_0xb2_t *vpd_inq_b2 = NULL;
    terminator_sas_drive_vpd_inquiry_page_0xc0_t *vpd_inq_c0 = NULL;

    fbe_status_t status;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return;
    }
    info->drive_type = type;

    /* initialize drive type specific data. 
    */
    status = sas_drive_get_default_inq_data(info->drive_type, &inq_data);  
    if (status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s: inquiry for drive_type[%d] not found\n", __FUNCTION__, type);
        return;
    }

    status = sas_drive_get_default_vpd_inq_f3_data(info->drive_type, &vpd_inq_f3);  
    if (status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s: vpd inquiry f3 for drive_type[%d] not found\n", __FUNCTION__, type);
        return;
    }

    status = sas_drive_get_default_vpd_inq_b2_data(info->drive_type, &vpd_inq_b2);  
    if (status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s: vpd inquiry b2 for drive_type[%d] not found\n", __FUNCTION__, type);
        return;
    }

    status = sas_drive_get_default_vpd_inq_c0_data(info->drive_type, &vpd_inq_c0);  
    if (status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s: vpd inquiry c0 for drive_type[%d] not found\n", __FUNCTION__, type);
        return;
    }

    fbe_copy_memory(info->inquiry, inq_data->drive_inquiry, sizeof(info->inquiry));
    fbe_copy_memory(info->vpd_inquiry_f3, vpd_inq_f3->data, sizeof(info->vpd_inquiry_f3));    
    fbe_copy_memory(info->mode_page, default_mode_page_10, sizeof(info->mode_page));  
    fbe_copy_memory(info->vpd_inquiry_b2, vpd_inq_b2->data, sizeof(info->vpd_inquiry_b2));  
    fbe_copy_memory(info->vpd_inquiry_c0, vpd_inq_c0->data, sizeof(info->vpd_inquiry_c0));  
    fbe_copy_memory(info->log_page_31, default_log_page_31, sizeof(info->log_page_31)); 
}

fbe_sas_drive_type_t sas_drive_get_drive_type(terminator_drive_t * self)
{
    terminator_sas_drive_info_t * info = NULL;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return FBE_SAS_DRIVE_INVALID;
    }
    return (info->drive_type);
}

void sas_drive_set_slot_number(terminator_drive_t * self, fbe_u32_t slot_number)
{
    terminator_sas_drive_info_t * info = NULL;

    self->slot_number = slot_number;
    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return;
    }
    info->slot_number = slot_number;
}
fbe_u32_t sas_drive_get_slot_number(terminator_drive_t * self)
{
    terminator_sas_drive_info_t * info = NULL;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return INVALID_SLOT_NUMBER;
    }
    return (info->slot_number);
}

fbe_sas_address_t sas_drive_get_sas_address(terminator_drive_t * self)
{
    terminator_sas_drive_info_t * info = NULL;
    fbe_cpd_shim_callback_login_t * login_data = NULL;
    fbe_sas_address_t address = FBE_SAS_ADDRESS_INVALID;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return address;
    }
    login_data = sas_drive_info_get_login_data(info);
    address = cpd_shim_callback_login_get_device_address(login_data);
    return address;
}

void sas_drive_set_sas_address(terminator_drive_t * self, fbe_sas_address_t address)
{
    terminator_sas_drive_info_t * info = NULL;
    fbe_cpd_shim_callback_login_t * login_data = NULL;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return;
    }
    login_data = sas_drive_info_get_login_data(info);
    cpd_shim_callback_login_set_device_address(login_data, address);
    return;
}

fbe_status_t sas_drive_call_io_api(terminator_drive_t * self, fbe_terminator_io_t * terminator_io)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    if(terminator_io->payload != NULL) {
        switch(drive_get_protocol(self)){
            case FBE_DRIVE_TYPE_SAS:
            case FBE_DRIVE_TYPE_SAS_FLASH_HE:
            case FBE_DRIVE_TYPE_SAS_FLASH_ME:
            case FBE_DRIVE_TYPE_SAS_FLASH_LE:
            case FBE_DRIVE_TYPE_SAS_FLASH_RI:
                status = fbe_terminator_sas_drive_payload (terminator_io, self);
                break;
            case FBE_DRIVE_TYPE_SATA:
            case FBE_DRIVE_TYPE_SATA_FLASH_HE:
                status = fbe_terminator_sata_drive_payload (terminator_io, self);
                break;
            default:
                break;
        }
        return status;
   }else{
        /* TODO: should log an error trace here */
        return FBE_STATUS_GENERIC_FAILURE;
    }
}
/* fbe_u8_t *serial_number is the buffer, we should copy the serial number to */
fbe_status_t sas_drive_info_get_serial_number(terminator_sas_drive_info_t * self, fbe_u8_t *serial_number)
{
    if (self == NULL)
        return FBE_STATUS_GENERIC_FAILURE;
    fbe_copy_memory(serial_number, &self->drive_serial_number, sizeof(self->drive_serial_number));
    return FBE_STATUS_OK;
}
fbe_status_t sas_drive_info_set_serial_number(terminator_sas_drive_info_t * self, fbe_u8_t *serial_number)
{
    if (self == NULL)
        return FBE_STATUS_GENERIC_FAILURE;
    fbe_copy_memory(&self->drive_serial_number, serial_number, sizeof(self->drive_serial_number));
    return FBE_STATUS_OK;
}

fbe_status_t sas_drive_info_get_product_id(terminator_sas_drive_info_t * self, fbe_u8_t *product_id)
{
    if (self == NULL)
        return FBE_STATUS_GENERIC_FAILURE;
    fbe_copy_memory(product_id, &self->product_id, sizeof(self->product_id));
    return FBE_STATUS_OK;
}
fbe_status_t sas_drive_info_set_product_id(terminator_sas_drive_info_t * self, fbe_u8_t *product_id)
{
    if (self == NULL)
        return FBE_STATUS_GENERIC_FAILURE;
    fbe_copy_memory(&self->product_id, product_id, sizeof(self->product_id));
    return FBE_STATUS_OK;
}

fbe_u8_t * sas_drive_get_serial_number(terminator_drive_t * self)
{
    terminator_sas_drive_info_t * info = NULL;
    if(self == NULL)
    {
        return NULL;
    }
    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return NULL;
    }
    return info->drive_serial_number;
}

void sas_drive_set_serial_number(terminator_drive_t * self, const fbe_u8_t * serial_number)
{
    terminator_sas_drive_info_t * info = NULL;
    if(self == NULL || serial_number == NULL)
    {
        return;
    }
    info = base_component_get_attributes(&self->base);
    if(info != NULL)
    {
        fbe_zero_memory(info->drive_serial_number, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE);
        memcpy(info->drive_serial_number, serial_number, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE - 1);
    }
}

fbe_u8_t * sas_drive_get_product_id(terminator_drive_t * self)
{
    terminator_sas_drive_info_t * info = NULL;
    if(self == NULL)
    {
        return NULL;
    }
    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return NULL;
    }
    return info->product_id;
}

fbe_status_t sas_drive_verify_product_id(fbe_u8_t *product_id)
{
    const fbe_u8_t * sas_drive_product_id_valid_substrings[] = {
        "CLAR",
        "NEO"
    };

    size_t sas_drive_product_id_valid_substrings_num = TERMINATOR_ARRAY_SIZE(sas_drive_product_id_valid_substrings);
    size_t i = 0;

    if (NULL == product_id)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for (i = 0; i < sas_drive_product_id_valid_substrings_num; ++i)
    {
        if (strstr(product_id + TERMINATOR_SCSI_INQUIRY_PRODUCT_ID_LEADING_OFFSET,
                   sas_drive_product_id_valid_substrings[i]))
        {
            return FBE_STATUS_OK;
        }
    }

    return FBE_STATUS_GENERIC_FAILURE;
}

void sas_drive_set_product_id(terminator_drive_t * self, const fbe_u8_t * product_id)
{
    terminator_sas_drive_info_t * info = NULL;
    if(self == NULL || product_id == NULL)
    {
        return;
    }
    info = base_component_get_attributes(&self->base);
    if(info != NULL)
    {
        fbe_zero_memory(info->product_id, sizeof(info->product_id));
        memcpy(info->product_id, product_id, sizeof(info->product_id) - 1);
    }
}


fbe_u8_t * sas_drive_get_inquiry(terminator_drive_t * self)
{
    terminator_sas_drive_info_t * info = NULL;
    if(self == NULL)
    {
        return NULL;
    }
    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return NULL;
    }
    return info->inquiry;
}


fbe_u8_t * sas_drive_get_vpd_inquiry_f3(terminator_drive_t * self, fbe_u32_t * size)
{
    terminator_sas_drive_info_t * info = NULL;
    if(self == NULL)
    {
        return NULL;
    }
    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return NULL;
    }
    *size = sizeof(info->vpd_inquiry_f3);
    return info->vpd_inquiry_f3;
}

fbe_u8_t * sas_drive_get_vpd_inquiry_b2(terminator_drive_t * self, fbe_u32_t * size)
{
    terminator_sas_drive_info_t * info = NULL;
    if(self == NULL)
    {
        return NULL;
    }
    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return NULL;
    }
    *size = sizeof(info->vpd_inquiry_b2);
    return info->vpd_inquiry_b2;
}

/*!***************************************************************************
 *          sas_drive_get_vpd_inquiry_c0 ()
 *****************************************************************************
 *
 * @brief   Get the drives's VPD Inquiry C0 page
 * 
 * @param   self - pointer to the drive
 * @param   size - size of the log page 31 buffer
 *
 * @return  fbe_u8_t * - pointer to the log page buffer of the drive
 *
 * @author
 *  08/25/2015  Wayne Garrett - Created
 *
 *********************************************************************/
fbe_u8_t * sas_drive_get_vpd_inquiry_c0(terminator_drive_t * self, fbe_u32_t * size)
{
    terminator_sas_drive_info_t * info = NULL;
    if(self == NULL)
    {
        return NULL;
    }
    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return NULL;
    }
    *size = sizeof(info->vpd_inquiry_c0);
    return info->vpd_inquiry_c0;
}


fbe_u8_t * sas_drive_get_mode_page(terminator_drive_t * self, fbe_u32_t * size)
{
    terminator_sas_drive_info_t * info = NULL;
    if(self == NULL)
    {
        return NULL;
    }
    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return NULL;
    }
    *size = sizeof(info->mode_page);
    return info->mode_page;
}

/*!***************************************************************************
 *          sas_drive_get_log_page_31 ()
 *****************************************************************************
 *
 * @brief   Get the log page 31 data for the specified drive 
 * 
 * @param   self - pointer to the drive
 * @param   size - size of the log page 31 buffer
 *
 * @return  fbe_u8_t * - pointer to the log page buffer of the drive
 *
 * @author
 *  07/23/2015  Deanna Heng - Created
 *
 *********************************************************************/
fbe_u8_t * sas_drive_get_log_page_31(terminator_drive_t * self, fbe_u32_t * size)
{
    terminator_sas_drive_info_t * info = NULL;
    if(self == NULL)
    {
        return NULL;
    }
    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return NULL;
    }
    *size = sizeof(info->log_page_31);
    return info->log_page_31;
}

/*!***************************************************************************
 *          sas_drive_set_log_page_31 ()
 *****************************************************************************
 *
 * @brief   Set the log page 31 data for the specified drive 
 * 
 * @param   self - pointer to the drive
 * @param   log_page_31 - pointer the the log page data
 * @param   page_size - size of the log page 31 buffer
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  07/23/2015  Deanna Heng - Created
 *
 *********************************************************************/
fbe_status_t sas_drive_set_log_page_31(terminator_drive_t * self, fbe_u8_t * log_page_31, fbe_u32_t page_size)
{
    terminator_sas_drive_info_t * info = NULL;

    if(self == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    info = base_component_get_attributes(&self->base);
    if (info != NULL)
    {
        if (sizeof(info->log_page_31) <= TERMINATOR_SCSI_LOG_PAGE_31_BYTE_SIZE)
        {
             fbe_zero_memory(info->log_page_31, sizeof(info->log_page_31)); 
             fbe_copy_memory(&info->log_page_31, log_page_31, page_size);
             return FBE_STATUS_OK;
        } 
        else
        {
            terminator_trace(FBE_TRACE_LEVEL_ERROR, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s log page 31 size 0x%x > max size 0x%x\n", 
                             __FUNCTION__, (fbe_u32_t)sizeof(info->log_page_31), TERMINATOR_SCSI_LOG_PAGE_31_BYTE_SIZE);
        }
    }

    return FBE_STATUS_GENERIC_FAILURE;
}

void sas_drive_set_backend_number(terminator_drive_t * self, fbe_u32_t backend_number)
{
    terminator_sas_drive_info_t * info = NULL;
    if(self == NULL)
    {
        return;
    }
    self->backend_number = backend_number;
    info = base_component_get_attributes(&self->base);
    if(info != NULL)
    {
        info->backend_number = backend_number;
    }
    return;
}
void sas_drive_set_enclosure_number(terminator_drive_t * self, fbe_u32_t encl_number)
{
    terminator_sas_drive_info_t * info = NULL;
    if(self == NULL)
    {
        return;
    }
    self->encl_number = encl_number;
    info = base_component_get_attributes(&self->base);
    if(info != NULL)
    {
        info->encl_number = encl_number;
    }
    return;
}

fbe_u32_t sas_drive_get_backend_number(terminator_drive_t * self)
{
    terminator_sas_drive_info_t * info = NULL;
    if(self == NULL)
    {
        return -1;
    }
    info = base_component_get_attributes(&self->base);
    if(info == NULL)
    {
        return -1;
    }
    return info->backend_number;
}

fbe_u32_t sas_drive_get_enclosure_number(terminator_drive_t * self)
{
    terminator_sas_drive_info_t * info = NULL;
    if(self == NULL)
    {
        return -1;
    }
    info = base_component_get_attributes(&self->base);
    if(info == NULL)
    {
        return -1;
    }
    return info->encl_number;
}
fbe_status_t sas_drive_get_state(terminator_drive_t * self, terminator_sas_drive_state_t * drive_state)
{
    terminator_sas_drive_info_t * info = NULL;

    if (self == NULL){
        return FBE_STATUS_GENERIC_FAILURE;
    }

    info = base_component_get_attributes(&self->base);

    * drive_state = info->drive_state;
    return FBE_STATUS_OK;
}
fbe_status_t sas_drive_set_state(terminator_drive_t * self, terminator_sas_drive_state_t  drive_state)
{
    terminator_sas_drive_info_t * info = NULL;

    if (self == NULL){
        return FBE_STATUS_GENERIC_FAILURE;
    }

    info = base_component_get_attributes(&self->base);

    info->drive_state = drive_state;

    return FBE_STATUS_OK;
}

void drive_clear_reset_count(terminator_drive_t * self)
{
    self->reset_count = 0;
}
void drive_increment_reset_count(terminator_drive_t * self)
{
    self->reset_count++;
}
void drive_clear_error_count(terminator_drive_t * self)
{
    self->error_count = 0;
}
void drive_increment_error_count(terminator_drive_t * self)
{
    self->error_count++;
}
fbe_u32_t drive_get_error_count(terminator_drive_t * self)
{
    return self->error_count;
}

fbe_lba_t drive_get_capacity(terminator_drive_t * self)
{
    return self->capacity;
}
void drive_set_capacity(terminator_drive_t * self, fbe_lba_t capacity)
{
    if (self != NULL)
    {
        self->capacity = capacity;
    }
}

fbe_block_size_t drive_get_block_size(terminator_drive_t * self)
{
    return self->block_size;
}

void drive_set_block_size(terminator_drive_t * self, fbe_block_size_t block_size)
{
    if (self != NULL)
    {
        self->block_size = block_size;
    }
}

fbe_u32_t drive_get_maximum_transfer_bytes(terminator_drive_t * self)
{
    return(self->maximum_transfer_bytes);
}

void drive_set_maximum_transfer_bytes(terminator_drive_t * self, fbe_u32_t maximum_transfer_bytes)
{
    if (self != NULL)
    {
        self->maximum_transfer_bytes = maximum_transfer_bytes;
    }
}

fbe_drive_type_t drive_get_protocol(terminator_drive_t * self)
{
    return self->drive_protocol;
}
void drive_set_protocol(terminator_drive_t * self, fbe_drive_type_t protocol)
{
    self->drive_protocol = protocol;
    return;
}

static fbe_status_t sas_drive_activate(terminator_drive_t * self)
{
    fbe_status_t status;
    base_component_t *parent;
    fbe_u32_t port_number;
    fbe_u32_t slot_number;
    ses_stat_elem_array_dev_slot_struct drive_slot_stat;
    ses_stat_elem_exp_phy_struct exp_phy_stat;
    base_component_t *virtual_phy = NULL;
    fbe_u8_t phy_id;
    fbe_sas_enclosure_type_t encl_type;
    fbe_bool_t generate_drive_login = FBE_FALSE;
    fbe_terminator_device_ptr_t port_handle;
    fbe_u8_t drive_slot_insert_count = 0;

    /* We support the SAS drives only */
    terminator_sas_drive_info_t * attributes = base_component_get_attributes(&self->base);
    if(attributes == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    port_number = attributes->backend_number;
    terminator_get_port_ptr(self, &port_handle);

    /* fill in login data - should we move this into insertion logic??? */
    status = drive_fill_in_login_data(self);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* 2) set the drive insert-bit in enclosure */
    /* obtain parent of device */
    base_component_get_parent(&self->base, &parent);

    /* make sure this is an enclosure */
    if (base_component_get_component_type(parent)!=TERMINATOR_COMPONENT_TYPE_ENCLOSURE)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // Get the related virtual phy  of the enclosure
    virtual_phy =
        base_component_get_child_by_type(parent, TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY);
    if(virtual_phy == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Lock the port configuration so no one else can change it while we modify configuration */
    terminator_lock_port(port_handle);

    slot_number = attributes->slot_number;
    // update if needed
    status = terminator_update_drive_parent_device((fbe_terminator_device_ptr_t*) &virtual_phy, &slot_number);
    if (status != FBE_STATUS_OK)
    {
		terminator_unlock_port(port_handle);
        KvTrace ("%s terminator_update_drive_parent_device failed!\n", __FUNCTION__);
        return status;
    }
    // Get the current ESES status of the drive slot & set the status code to OK
    status = sas_virtual_phy_get_drive_eses_status((terminator_virtual_phy_t *)virtual_phy,
        slot_number, &drive_slot_stat);
    if (status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s sas_virtual_phy_get_drive_eses_status failed!\n", __FUNCTION__);
        //fbe_mutex_unlock(&matching_port->update_mutex);
        terminator_unlock_port(port_handle);
        return status;
    }
    drive_slot_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    status = sas_virtual_phy_set_drive_eses_status((terminator_virtual_phy_t *)virtual_phy,
        slot_number, drive_slot_stat);
    if (status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s sas_virtual_phy_set_drive_eses_status failed!\n", __FUNCTION__);
        terminator_unlock_port(port_handle);
        return status;
    }

    // Increment the drive slot insert count
    status = terminator_get_drive_slot_insert_count((terminator_virtual_phy_t *)virtual_phy,
                                                    slot_number,
                                                    &drive_slot_insert_count);
    if(status != FBE_STATUS_OK) {
		terminator_unlock_port(port_handle);
		terminator_trace(FBE_TRACE_LEVEL_ERROR, 
						FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
						"%s terminator_get_drive_slot_insert_count failed!\n", __FUNCTION__);
        return(status);         
    }                           

    drive_slot_insert_count++;

    status = terminator_set_drive_slot_insert_count((terminator_virtual_phy_t *)virtual_phy,
                                                    slot_number,
                                                    drive_slot_insert_count);
	
    if(status != FBE_STATUS_OK) {
		terminator_unlock_port(port_handle);
		terminator_trace(FBE_TRACE_LEVEL_ERROR, 
						FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
						"%s terminator_set_drive_slot_insert_count failed!\n", __FUNCTION__);
        return(status);         
    }                           


    status = terminator_virtual_phy_get_enclosure_type(virtual_phy, &encl_type);
    if (status != FBE_STATUS_OK)
    {
		terminator_unlock_port(port_handle);
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s terminator_virtual_phy_get_enclosure_type failed!\n", __FUNCTION__);
        return status;
    }

    // Get the corresponding PHY status
    status = sas_virtual_phy_get_drive_slot_to_phy_mapping(slot_number, &phy_id, encl_type);
    if (status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s sas_virtual_phy_get_drive_slot_to_phy_mapping failed!\n", __FUNCTION__);
        terminator_unlock_port(port_handle);
        return status;
    }
    status = sas_virtual_phy_get_phy_eses_status((terminator_virtual_phy_t *)virtual_phy, phy_id, &exp_phy_stat);
    if (status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s set sas_virtual_phy_get_phy_eses_status failed!\n", __FUNCTION__);
        terminator_unlock_port(port_handle);
        return status;
    }

    // Login is generated only if the below conditions are statisfied.
    if((!drive_slot_stat.dev_off) &&
       (exp_phy_stat.cmn_stat.elem_stat_code == SES_STAT_CODE_OK))
    {
        //Set the "Phy Ready" bit and generate login for the drive
        exp_phy_stat.phy_rdy = FBE_TRUE;
        status = sas_virtual_phy_set_phy_eses_status((terminator_virtual_phy_t *)virtual_phy, phy_id, exp_phy_stat);
        if (status != FBE_STATUS_OK)
        {
            terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s set sas_virtual_phy_set_phy_eses_status failed!\n", __FUNCTION__);
            terminator_unlock_port(port_handle);
            return status;
        }
        generate_drive_login = FBE_TRUE;
    }

    terminator_unlock_port(port_handle);
    /* 3) set the login_pending flag, terminator api will wake up the mini_port_api thread to process the login */
    if(generate_drive_login)
    {
        status = terminator_set_device_login_pending(self);
        if (status != FBE_STATUS_OK)
        {
            //terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s set_login_pending failed on device 0x%X!\n", __FUNCTION__, self);
        }
    }

    status = fbe_terminator_miniport_api_device_state_change_notify(port_number);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "Drive activation failed\n");
        return status;
    }

    return status;

}

static fbe_status_t sata_drive_activate(terminator_drive_t * self)
{
    fbe_status_t status;
    base_component_t *parent;
    fbe_u32_t port_number;
    ses_stat_elem_array_dev_slot_struct drive_slot_stat;
    ses_stat_elem_exp_phy_struct        exp_phy_stat;
    base_component_t                    *virtual_phy = NULL;
    fbe_u8_t                            phy_id;
    fbe_sas_enclosure_type_t            encl_type;
    fbe_u32_t                           slot_number;
    fbe_terminator_device_ptr_t port_handle;

    terminator_sata_drive_info_t * attributes = base_component_get_attributes(&self->base);
    if(attributes == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    port_number = attributes->backend_number;
    slot_number = attributes->slot_number;
    terminator_get_port_ptr(self, &port_handle);

    /* fill in login data - should we move this into insertion logic??? */
    status = drive_fill_in_login_data(self);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* inform enclosure to update the drive and phy status */
    base_component_get_parent(&self->base, &parent);

    /* make sure this is an enclosure */
    if (base_component_get_component_type(parent)!=TERMINATOR_COMPONENT_TYPE_ENCLOSURE)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // Get the related virtual phy  of the enclosure
    virtual_phy =
        base_component_get_child_by_type(parent, TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY);
    if(virtual_phy == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    // update if needed
    status = terminator_update_drive_parent_device((fbe_terminator_device_ptr_t*)&virtual_phy, &slot_number);
    if (status != FBE_STATUS_OK)
    {
        KvTrace ("%s terminator_update_drive_parent_device failed!\n", __FUNCTION__);
        return status;
    }

    /* Lock the port configuration so no one else can change it while we modify configuration */
    //fbe_mutex_lock(&matching_port->update_mutex);
    if((status = terminator_lock_port(port_handle)) != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s: terminator_lock_port() failed for port %d\n", __FUNCTION__, port_number);
    }

    fbe_zero_memory(&drive_slot_stat, sizeof(ses_stat_elem_array_dev_slot_struct));
    drive_slot_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    status = sas_virtual_phy_set_drive_eses_status((terminator_virtual_phy_t *)virtual_phy,
        slot_number, drive_slot_stat);
    if (status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s set enclosure_drive_status failed!\n", __FUNCTION__);
        //fbe_mutex_unlock(&matching_port->update_mutex);
        terminator_unlock_port(port_handle);
        return status;
    }

    fbe_zero_memory(&exp_phy_stat, sizeof(ses_stat_elem_exp_phy_struct));
    exp_phy_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    exp_phy_stat.link_rdy = 0x1;
    exp_phy_stat.phy_rdy = 0x1;

    status = terminator_virtual_phy_get_enclosure_type(virtual_phy, &encl_type);
    if (status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s terminator_virtual_phy_get_enclosure_type failed!\n", __FUNCTION__);
        //fbe_mutex_unlock(&matching_port->update_mutex);
        terminator_unlock_port(port_handle);
        return status;
    }

    status = sas_virtual_phy_get_drive_slot_to_phy_mapping(slot_number, &phy_id, encl_type);
    if (status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s sas_virtual_phy_get_drive_slot_to_phy_mapping failed!\n", __FUNCTION__);
        //fbe_mutex_unlock(&matching_port->update_mutex);
        terminator_unlock_port(port_handle);
        return status;
    }
    status = sas_virtual_phy_set_phy_eses_status((terminator_virtual_phy_t *)virtual_phy, phy_id, exp_phy_stat);
    if (status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s set enclosure_drive_status failed!\n", __FUNCTION__);
        //fbe_mutex_unlock(&matching_port->update_mutex);
        terminator_unlock_port(port_handle);
        return status;
    }

    //fbe_mutex_unlock(&matching_port->update_mutex);
    terminator_unlock_port(port_handle);
    /* mark as login pending */
    status = terminator_set_device_login_pending(self);
    if (status != FBE_STATUS_OK)
    {
        //terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s set_login_pending failed on device 0x%X!\n", __FUNCTION__, self);
    }

    status = fbe_terminator_miniport_api_device_state_change_notify(port_number);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "Drive activation failed\n");
        return status;
    }

    return status;
}

fbe_status_t drive_activate(terminator_drive_t * self)
{
    fbe_drive_type_t drive_type;
    drive_type = drive_get_protocol(self);
    switch(drive_type)
    {
        case FBE_DRIVE_TYPE_SAS:
        case FBE_DRIVE_TYPE_SAS_FLASH_HE:
        case FBE_DRIVE_TYPE_SAS_FLASH_ME:
        case FBE_DRIVE_TYPE_SAS_FLASH_LE:
        case FBE_DRIVE_TYPE_SAS_FLASH_RI:
            return sas_drive_activate(self);
        case FBE_DRIVE_TYPE_SATA:
        case FBE_DRIVE_TYPE_SATA_FLASH_HE:
            return sata_drive_activate(self);
        default:
            /* Only SAS and SATA drives are supported */
            return FBE_STATUS_GENERIC_FAILURE;
    }
}

static fbe_status_t sata_drive_fill_in_login_data(terminator_drive_t * self)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    base_component_t * parent = NULL;
    terminator_sata_drive_info_t *attributes = NULL;
    fbe_cpd_shim_callback_login_t * login_data = NULL;
    fbe_u32_t           maximum_transfer_bytes = 0;
    //fbe_u64_t                       drive_id;

    terminator_sas_enclosure_info_t *enclosure_attributes = NULL;
    fbe_cpd_shim_callback_login_t * enclosure_login_data = NULL;
    fbe_miniport_device_id_t cpd_device_id = CPD_DEVICE_ID_INVALID;
    fbe_miniport_device_id_t parent_device_id = CPD_DEVICE_ID_INVALID;
    fbe_sas_address_t parent_device_address = FBE_SAS_ADDRESS_INVALID;
    fbe_u8_t            enclosure_chain_depth = 0;
    fbe_u8_t            phy_number = 0;
    fbe_u32_t           slot_number;

    /* first get the parent device */
    base_component_get_parent(&self->base, &parent);
    if (parent == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    slot_number = sata_drive_get_slot_number(self);
    switch(base_component_get_component_type(parent))
    {
    case TERMINATOR_COMPONENT_TYPE_ENCLOSURE:
        attributes = base_component_get_attributes(&self->base);
        if (attributes == NULL)
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }
        login_data = sata_drive_info_get_login_data(attributes);
        if (login_data == NULL)
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* drive_id generation and assignment has moved to drive creation function */
        /* drive_id = base_generate_device_id(); */
        //drive_id = base_component_get_id(&self->base);
        //cpd_shim_callback_login_set_device_id(login_data, drive_id);
        /* base_component_assign_id(&self->base, drive_id); */

        enclosure_attributes = base_component_get_attributes(parent);
        enclosure_login_data = sas_enclosure_info_get_login_data(enclosure_attributes);
        parent_device_id = cpd_shim_callback_login_get_device_id(enclosure_login_data);
        parent_device_address = cpd_shim_callback_login_get_device_address(enclosure_login_data);
        enclosure_chain_depth = cpd_shim_callback_login_get_enclosure_chain_depth(enclosure_login_data) + 1;
        status = sas_enclosure_calculate_sas_device_phy_number(&phy_number,(terminator_enclosure_t *)parent, slot_number);
    if (status != FBE_STATUS_OK) {
        return status;
    }

        /* Use the new device id format */
        terminator_add_device_to_miniport_sas_device_table(self);
        terminator_get_cpd_device_id_from_miniport(self, &cpd_device_id);
        cpd_shim_callback_login_set_device_id(login_data, cpd_device_id);
        terminator_get_maximum_transfer_bytes_from_miniport(self, &maximum_transfer_bytes);
        drive_set_maximum_transfer_bytes(self, maximum_transfer_bytes);
        cpd_shim_callback_login_set_parent_device_id(login_data, parent_device_id);
        cpd_shim_callback_login_set_parent_device_address(login_data, parent_device_address);
        cpd_shim_callback_login_set_enclosure_chain_depth(login_data, enclosure_chain_depth);
        cpd_shim_callback_login_set_phy_number(login_data, phy_number);
        status = FBE_STATUS_OK;
        break;
    default:
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    return status;

}

static fbe_status_t sas_drive_fill_in_login_data(terminator_drive_t * self)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    base_component_t * parent = NULL;
    terminator_sas_drive_info_t *attributes = NULL;
    fbe_cpd_shim_callback_login_t * login_data = NULL;
    //fbe_terminator_api_device_handle_t      drive_id;

    terminator_sas_enclosure_info_t *enclosure_attributes = NULL;
    fbe_cpd_shim_callback_login_t * enclosure_login_data = NULL;
    fbe_sas_address_t parent_device_address = FBE_SAS_ADDRESS_INVALID;
    fbe_u8_t            enclosure_chain_depth = 0;
    fbe_u8_t            phy_number = 0;
    fbe_u32_t           slot_number;
    fbe_miniport_device_id_t cpd_device_id;
    fbe_miniport_device_id_t parent_cpd_device_id;
    fbe_u32_t           maximum_transfer_bytes = 0;

    /* first get the parent device */
    base_component_get_parent(&self->base, &parent);
    if (parent == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    slot_number = sas_drive_get_slot_number(self);
    switch(base_component_get_component_type(parent))
    {
    case TERMINATOR_COMPONENT_TYPE_ENCLOSURE:
        attributes = base_component_get_attributes(&self->base);
        if (attributes == NULL)
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }
        login_data = sas_drive_info_get_login_data(attributes);
        if (login_data == NULL)
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* drive_id generation and assignment has moved to drive creation function */
        /* drive_id = base_generate_device_id(); */
//      drive_id = base_component_get_id(&self->base);
//      cpd_shim_callback_login_set_device_id(login_data, drive_id);
        /* base_component_assign_id(&self->base, drive_id); */

        enclosure_attributes = base_component_get_attributes(parent);
        enclosure_login_data = sas_enclosure_info_get_login_data(enclosure_attributes);
        parent_cpd_device_id = cpd_shim_callback_login_get_device_id(enclosure_login_data);
        parent_device_address = cpd_shim_callback_login_get_device_address(enclosure_login_data);
        enclosure_chain_depth = cpd_shim_callback_login_get_enclosure_chain_depth(enclosure_login_data) + 1; /*should we increment it?*/
        /* Use the new device id format */
        terminator_add_device_to_miniport_sas_device_table(self);
        terminator_get_cpd_device_id_from_miniport(self, &cpd_device_id);
        cpd_shim_callback_login_set_device_id(login_data, cpd_device_id);
        terminator_get_maximum_transfer_bytes_from_miniport(self, &maximum_transfer_bytes);
        drive_set_maximum_transfer_bytes(self, maximum_transfer_bytes);
        parent_cpd_device_id = cpd_shim_callback_login_get_device_id(enclosure_login_data);
        status = sas_enclosure_calculate_sas_device_phy_number(&phy_number, (terminator_enclosure_t *)parent, slot_number);
    if (status != FBE_STATUS_OK) {
        return status;
    }
        cpd_shim_callback_login_set_parent_device_id(login_data, parent_cpd_device_id);
        cpd_shim_callback_login_set_parent_device_address(login_data, parent_device_address);
        cpd_shim_callback_login_set_enclosure_chain_depth(login_data, enclosure_chain_depth);
        cpd_shim_callback_login_set_phy_number(login_data, phy_number);
        status = FBE_STATUS_OK;
        break;
    default:
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}

static fbe_status_t drive_fill_in_login_data(terminator_drive_t * self)
{
    fbe_drive_type_t drive_type = drive_get_protocol(self);
    switch(drive_type)
    {
        case FBE_DRIVE_TYPE_SAS:
        case FBE_DRIVE_TYPE_SAS_FLASH_HE:
        case FBE_DRIVE_TYPE_SAS_FLASH_ME:
        case FBE_DRIVE_TYPE_SAS_FLASH_LE:
        case FBE_DRIVE_TYPE_SAS_FLASH_RI:
            return sas_drive_fill_in_login_data(self);
        case FBE_DRIVE_TYPE_SATA:
        case FBE_DRIVE_TYPE_SATA_FLASH_HE:
            return sata_drive_fill_in_login_data(self);
        default:
            /* only SAS and SATA drives are supported */
            return FBE_STATUS_GENERIC_FAILURE;
    }
}

/* This function is not used by anyone currently. */
fbe_status_t terminator_drive_complete_forced_creation(terminator_drive_t *self)
{
    fbe_u8_t         drive_name[TERMINATOR_DISK_FILE_NAME_LEN] = {0};
    fbe_drive_type_t drive_type = drive_get_protocol(self);
    void            *attributes = base_component_get_attributes(&self->base);

    int count = 0;

    if (NULL == attributes)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Create the disk file name by convention of
     *  "disk<Port number>_<Enclosure number>_<Slot number>"
     */
    switch (drive_type)
    {
    case FBE_DRIVE_TYPE_SAS:
    case FBE_DRIVE_TYPE_SAS_FLASH_HE:
    case FBE_DRIVE_TYPE_SAS_FLASH_ME:
    case FBE_DRIVE_TYPE_SAS_FLASH_LE:
    case FBE_DRIVE_TYPE_SAS_FLASH_RI:
        /* Create the disk file name.
        * The disk naming convention ( and thus the corresponding file name) is
        * "disk<Port number>_<Enclosure_number>_<slot_number>"
        */
        fbe_zero_memory(drive_name, sizeof(drive_name));
        count = _snprintf(drive_name, sizeof(drive_name), TERMINATOR_DISK_NAME_FORMAT_STRING,
                ((terminator_sas_drive_info_t *)attributes)->backend_number,
                ((terminator_sas_drive_info_t *)attributes)->encl_number,
                ((terminator_sas_drive_info_t *)attributes)->slot_number);
        if (( count < 0 ) || (sizeof(drive_name) == count )) {
             
            return FBE_STATUS_GENERIC_FAILURE;
        }
        break;
    case FBE_DRIVE_TYPE_SATA:
    case FBE_DRIVE_TYPE_SATA_FLASH_HE:
        /* Create the disk file name.
        * The disk naming convention ( and thus the corresponding file name) is
        * "disk<Port number>_<Enclosure_number>_<slot_number>"
        */
        fbe_zero_memory(drive_name, sizeof(drive_name));
        count = _snprintf(drive_name, sizeof(drive_name), TERMINATOR_DISK_NAME_FORMAT_STRING,
                ((terminator_sata_drive_info_t *)attributes)->backend_number,
                ((terminator_sata_drive_info_t *)attributes)->encl_number,
                ((terminator_sata_drive_info_t *)attributes)->slot_number);
        if (( count < 0 ) || (sizeof(drive_name) == count )) {
             
            return FBE_STATUS_GENERIC_FAILURE;
        }
        break;
    default:
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* fill in login data - Should we move this into the insertion logic??? */
    return drive_fill_in_login_data(self);
}

static fbe_bool_t terminator_drive_is_system_drive(terminator_drive_t *drive)
{
    if ((drive->backend_number <= TERMINATOR_SIMULATED_DISK_MAX_DATABASE_DRIVE_BUS)  &&
        (drive->encl_number    <= TERMINATOR_SIMULATED_DISK_MAX_DATABASE_DRIVE_ENCL) &&
        (drive->slot_number    <= TERMINATOR_SIMULATED_DISK_MAX_DATABASE_DRIVE_SLOT)    )
    {
        return FBE_TRUE;
    }

    return FBE_FALSE;
}

/*!****************************************************************************
 *          terminator_trace_request()
 ******************************************************************************
 * @brief
 * This function is used to enhance the debugging functionality for terminator
 * simulated disk memory trace.  It wraps over the "terminator_trace" and prints
 * additional debugging information using trace level FBE_TRACE_LEVEL_INFO. 
 * Caller should pass the debug flag based on the functionality. 
 * 
 * @param   drive  - pointer to terminator drive object
 * @param   trace_level         - base object trace level
 * @param   message_id          - base object trace message id
 * @param   debug_flag          - caller specified debug flag
 * @param   fmt                 - pointer to formatted string
 *
 * @return  void
 * 
 * @note    Max capacity to print string format is 188 characters
 *
 * @author
 *  10/17/2011  Ron Proulx  - Created from provision drive code.
 *
 ******************************************************************************/
static void terminator_drive_trace_request(terminator_drive_t *drive, 
                                     fbe_trace_level_t trace_level,
                                     fbe_u32_t message_id,
                                     fbe_terminator_drive_debug_flags_t debug_flag,
                                     const fbe_u8_t * fmt, ...)
{
    va_list     argList;
    fbe_u8_t    header[FBE_TERMINATOR_DRIVE_DISK_STRING_LENGTH] = {0};

    /* Currently we don't take any lock.  Determine if we should trace or not.
     */
    if ((drive->drive_debug_flags & debug_flag) != 0)
    {
        /*! @note By default tracing of system drives is disabled. 
         */
        if ( ((drive->drive_debug_flags & FBE_TERMINATOR_DRIVE_DEBUG_FLAG_TRACE_SYSTEM_DRIVES) != 0) ||
            !terminator_drive_is_system_drive(drive)                                                    )
        {
            if (fmt != NULL)
            {
                csx_p_snprintf(header, FBE_TERMINATOR_DRIVE_DISK_STRING_LENGTH, 
                               "TERM:%d_%d_%d", 
                               drive->backend_number, drive->encl_number, drive->slot_number);
 
                va_start(argList, fmt);
                fbe_trace_report_w_header(FBE_COMPONENT_TYPE_SERVICE,
                                          FBE_SERVICE_ID_TERMINATOR,
                                          trace_level,
                                          header,
                                          fmt, 
                                          argList);
                va_end(argList);
            }
        }
    }

    return;
}
static fbe_u8_t *terminator_drive_get_first_block_buffer(fbe_sg_element_t *pre_sg_p, fbe_sg_element_t *sg_p)    
{
    /* Simply return the address field of the sg.
     */
    if ((pre_sg_p != NULL) && (pre_sg_p->address != NULL))
    {
        return(fbe_sg_element_address(pre_sg_p));
    }
    if (sg_p == NULL)
    {
        return NULL;
    }
    return(fbe_sg_element_address(sg_p));
}

static fbe_u8_t *terminator_drive_get_last_block_buffer(fbe_sg_element_t *sg_p, fbe_sg_element_t *post_sg_p)
{
    fbe_sg_element_t   *sgl_p;
    fbe_sg_element_t   *prev_sgl_p;
    fbe_u8_t           *buffer_p;

    /* Find the terminator
     */
    if ((post_sg_p != NULL) && (post_sg_p->address != NULL))
    {
        sgl_p = post_sg_p;
    }
    else
    {
        sgl_p = sg_p;
    }
    if (sgl_p == NULL)
    {
        return NULL;
    }

    /* Locate the terminator.
     */
    while (fbe_sg_element_count(sgl_p) != 0)
    {
        prev_sgl_p = sgl_p;
        sgl_p++;
    }
    if ((prev_sgl_p == NULL)                                               ||
        ((fbe_sg_element_count(prev_sgl_p) % FBE_BE_BYTES_PER_BLOCK) != 0)    )
    {
        return(NULL);
    }

    /* Get the address of the last block.
     */
    buffer_p = fbe_sg_element_address(prev_sgl_p) + fbe_sg_element_count(prev_sgl_p);
    buffer_p -= FBE_BE_BYTES_PER_BLOCK;
    return(buffer_p);
}

static fbe_status_t terminator_drive_trace_dump_sector(terminator_drive_t *drive, fbe_terminator_io_t *terminator_io,
                                                       fbe_sector_t *sector_p)
{
    fbe_u32_t   number_of_chunk_info;
    fbe_u32_t   byte_16_count = 0;
    fbe_u32_t   word_16_to_trace = 4;
    fbe_u64_t  *data_ptr = (fbe_u64_t *)sector_p;

    word_16_to_trace = (sizeof(fbe_sector_t) / 16);

    /* Only trace the words necessary.
     */
    number_of_chunk_info = (fbe_u32_t)(drive->capacity / FBE_TERMINATOR_DRIVE_DISK_CHUNK_SIZE);
    number_of_chunk_info = (number_of_chunk_info * FBE_TERMINATOR_DRIVE_DISK_BYTES_PER_CHUNK_INFO) / 16;
    word_16_to_trace = FBE_MIN(number_of_chunk_info, word_16_to_trace);

    /* We only dump (1) sector per request so there is no need to display
     * the lba again.
     */
    for ( byte_16_count = 0; byte_16_count < word_16_to_trace; byte_16_count += 4)
    {
        terminator_drive_trace_request(drive, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                       FBE_TERMINATOR_DRIVE_DEBUG_FLAG_TRACE_DATA,
                                       "tio: %p qw:%02d-%02d %016llx %016llx %016llx %016llx\n",
                                       terminator_io,
                                       byte_16_count, (byte_16_count + 3),
                                       data_ptr[byte_16_count], data_ptr[byte_16_count + 1], 
                                       data_ptr[byte_16_count + 2], data_ptr[byte_16_count + 3]); 
    }

    /* Always trace the metadata.
     */
    terminator_drive_trace_request(drive, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                   FBE_TERMINATOR_DRIVE_DEBUG_FLAG_TRACE_DATA,
                                   "tio: %p crc: 0x%04x ts: 0x%04x ws: 0x%04x lba: 0x%04x\n",
                                   terminator_io,
                                   sector_p->crc, sector_p->time_stamp, sector_p->write_stamp, sector_p->lba_stamp);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * terminator_drive_trace_data()
 ****************************************************************
 * @brief
 *  Log info about data for a terminator io.
 * 
 * @param drive - Pointer to terminator drive object
 * @param terminator_io - Pointer to I/O with buffer     
 * 
 * @return None
 * 
 ****************************************************************/
static void terminator_drive_trace_data(terminator_drive_t *drive, fbe_terminator_io_t *terminator_io)
{
    fbe_sg_element_t   *sg_p;    
    fbe_sg_element_t   *pre_sg_p;    
    fbe_sg_element_t   *post_sg_p;
    fbe_u64_t          *data_ptr = NULL;

    /* This method expects data.
     */
    if (terminator_io->payload == NULL)
    {
        terminator_drive_trace_request(drive, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                         FBE_TERMINATOR_DRIVE_DEBUG_FLAG_TRACE_DATA,
                         "tio: %p  l:%010llx b:%08llx op: %d end blk_size: %d no payload !\n",
                         terminator_io, terminator_io->lba, terminator_io->block_count, 
                         terminator_io->opcode, terminator_io->block_size);
        return;
    }

    /* Walk thru all the (3) sg lists
     */
    fbe_payload_ex_get_sg_list(terminator_io->payload, &sg_p, NULL);
    fbe_payload_ex_get_pre_sg_list(terminator_io->payload, &pre_sg_p);
    fbe_payload_ex_get_post_sg_list(terminator_io->payload, &post_sg_p);

    /* This method expects data.
     */
    data_ptr = (fbe_u64_t *)terminator_drive_get_first_block_buffer(pre_sg_p, sg_p);
    if (data_ptr == NULL)
    {
        /* This method expects data.
         */
        terminator_drive_trace_request(drive, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                         FBE_TERMINATOR_DRIVE_DEBUG_FLAG_TRACE_DATA,
                         "tio: %p l:%010llx b:%08llx end   blk_size: %d no data to trace\n",
                         terminator_io, terminator_io->lba, terminator_io->block_count, 
                         terminator_io->block_size);
        return;
    }
    else if (((drive->drive_debug_flags & FBE_TERMINATOR_DRIVE_DEBUG_FLAG_METADATA_WRITE_DUMP) != 0)       &&
             (terminator_io->opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE)                           &&
             (terminator_io->lba == (drive->capacity - FBE_TERMINATOR_DRIVE_DISK_PAGED_METADATA_CAPACITY))    )
    {
        /* This is a metadata write trace more data.
         */
        terminator_drive_trace_dump_sector(drive, terminator_io, (fbe_sector_t *)data_ptr);
    }
    else
    {
        /* Else dump the first and last 16 bytes of the first and last blocks
         * of the request.  Since stripped mirrors use a different block
         * size, assert if that is the case.
         * Note!! The `:' matters so that we can screen for fru and lba etc!!
         */
        terminator_drive_trace_request(drive, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                         FBE_TERMINATOR_DRIVE_DEBUG_FLAG_TRACE_DATA,
                         "tio: %p l:%010llx %016llx %016llx %016llx %016llx\n",
                         terminator_io, terminator_io->lba,
                         data_ptr[0], data_ptr[1], 
                         data_ptr[FBE_BE_BYTES_PER_BLOCK/sizeof(fbe_u64_t) - 2], 
                         data_ptr[FBE_BE_BYTES_PER_BLOCK/sizeof(fbe_u64_t) - 1]);
    }

    /* Now dump the last block.
     */
    if (terminator_io->block_count > 1)
    {
        data_ptr = (fbe_u64_t *)terminator_drive_get_last_block_buffer(sg_p, post_sg_p);
        if (data_ptr == NULL)
        {
            /* This method expects data.
             */
            terminator_drive_trace_request(drive, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                     FBE_TERMINATOR_DRIVE_DEBUG_FLAG_TRACE_DATA,
                     "tio: %p l:%010llx b:%08llx end   blk_size: %d no last to trace\n",
                     terminator_io, terminator_io->lba, terminator_io->block_count, 
                     terminator_io->block_size);
        }
        else
        {
            terminator_drive_trace_request(drive, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                     FBE_TERMINATOR_DRIVE_DEBUG_FLAG_TRACE_DATA,
                     "tio: %p l:%010llx %016llx %016llx %016llx %016llx\n",
                     terminator_io, (terminator_io->lba + terminator_io->block_count - 1),
                     data_ptr[0], data_ptr[1], 
                     data_ptr[FBE_BE_BYTES_PER_BLOCK/sizeof(fbe_u64_t) - 2], 
                     data_ptr[FBE_BE_BYTES_PER_BLOCK/sizeof(fbe_u64_t) - 1]);
        }
    }

    return;
}

void terminator_drive_trace_read(terminator_drive_t *drive, fbe_terminator_io_t *terminator_io, fbe_bool_t b_start)
{
    fbe_payload_ex_t *payload_p = NULL;
    fbe_key_handle_t encryption_handle;
    terminator_key_info_t *key_info = NULL;
    payload_p = (fbe_payload_ex_t *)terminator_io->payload;

    /* Check if we want to decrypt the data. If so decrypt it */
    fbe_payload_ex_get_key_handle(payload_p, &encryption_handle);
    key_info = (terminator_key_info_t *)(fbe_ptrhld_t)encryption_handle;
    /* Trace the I/O if enabled.
     */
    if (drive->drive_debug_flags != 0)
    {
        /* Only trace the request completion (and data) if `trace end' is set.
         */
        if (b_start == FBE_FALSE)
        {
            if ((drive->drive_debug_flags & FBE_TERMINATOR_DRIVE_DEBUG_FLAG_TRACE_END) != 0)
            {
                terminator_drive_trace_request(drive, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                         (FBE_TERMINATOR_DRIVE_DEBUG_FLAG_ALL_TRAFFIC | FBE_TERMINATOR_DRIVE_DEBUG_FLAG_READ_TRAFFIC),
                         "tio: %p l:%010llx b:%08llx op: %d end  hndl: 0x%llx %s\n",
                         terminator_io, terminator_io->lba, terminator_io->block_count,
                         terminator_io->opcode, encryption_handle, (key_info) ? ((fbe_char_t*)&key_info->keys[0]) : "None");

                /* If this is the completion and data tracing is enabled, trace the data.
                 */
                if ((drive->drive_debug_flags & FBE_TERMINATOR_DRIVE_DEBUG_FLAG_TRACE_DATA) != 0)
                {
                    terminator_drive_trace_data(drive, terminator_io);
                }
            }
        }
        else
        {
            terminator_drive_trace_request(drive, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                             (FBE_TERMINATOR_DRIVE_DEBUG_FLAG_ALL_TRAFFIC | FBE_TERMINATOR_DRIVE_DEBUG_FLAG_READ_TRAFFIC),
                             "tio: %p l:%010llx b:%08llx op: %d start blk_size: %d\n",
                             terminator_io, terminator_io->lba, terminator_io->block_count,
                             terminator_io->opcode, terminator_io->block_size);
        }
    }
    return;
}

void terminator_drive_trace_write_zero_pattern(terminator_drive_t *drive, fbe_terminator_io_t *terminator_io, fbe_bool_t b_start)
{
    /* Trace the I/O if enabled.
     */
    if (drive->drive_debug_flags != 0)
    {
        /* Only trace the request completion (and data) if `trace end' is set.
         */
        if (b_start == FBE_FALSE)
        {
            if ((drive->drive_debug_flags & FBE_TERMINATOR_DRIVE_DEBUG_FLAG_TRACE_END) != 0)
            {
                terminator_drive_trace_request(drive, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                         (FBE_TERMINATOR_DRIVE_DEBUG_FLAG_ALL_TRAFFIC | FBE_TERMINATOR_DRIVE_DEBUG_FLAG_ZERO_TRAFFIC),
                         "tio: %p l:%010llx b:%08llx op: %d end  blk_size: %d\n",
                         terminator_io, terminator_io->lba, terminator_io->block_count,
                         terminator_io->opcode, terminator_io->block_size);
            }
        }
        else
        {
            terminator_drive_trace_request(drive, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                             (FBE_TERMINATOR_DRIVE_DEBUG_FLAG_ALL_TRAFFIC | FBE_TERMINATOR_DRIVE_DEBUG_FLAG_ZERO_TRAFFIC),
                             "tio: %p l:%010llx b:%08llx op: %d start blk_size: %d\n",
                             terminator_io, terminator_io->lba, terminator_io->block_count,
                             terminator_io->opcode, terminator_io->block_size);
        }
    }
    return;
}

void terminator_drive_trace_write(terminator_drive_t *drive, fbe_terminator_io_t *terminator_io, fbe_bool_t b_start)
{
    fbe_payload_ex_t *payload_p = NULL;
    fbe_key_handle_t encryption_handle;
    terminator_key_info_t *key_info = NULL;
    payload_p = (fbe_payload_ex_t *)terminator_io->payload;

    /* Check if we want to decrypt the data. If so decrypt it */
    fbe_payload_ex_get_key_handle(payload_p, &encryption_handle);
    key_info = (terminator_key_info_t *)(fbe_ptrhld_t)encryption_handle;
    /* Trace the I/O if enabled.
     */
    if (drive->drive_debug_flags != 0)
    {
        /* Only trace the request completion (and data) if `trace end' is set.
         */
        if (b_start == FBE_FALSE)
        {
            if ((drive->drive_debug_flags & FBE_TERMINATOR_DRIVE_DEBUG_FLAG_TRACE_END) != 0)
            {
                terminator_drive_trace_request(drive, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                         (FBE_TERMINATOR_DRIVE_DEBUG_FLAG_ALL_TRAFFIC | FBE_TERMINATOR_DRIVE_DEBUG_FLAG_WRITE_TRAFFIC),
                         "tio: %p l:%010llx b:%08llx op: %d end  hndl: 0x%llx %s\n",
                         terminator_io, terminator_io->lba, terminator_io->block_count,
                         terminator_io->opcode, encryption_handle, (key_info) ? ((fbe_char_t*)&key_info->keys[0]) : "None");

                /* If this is the completion and data tracing is enabled, trace the data.
                 */
                if ((drive->drive_debug_flags & FBE_TERMINATOR_DRIVE_DEBUG_FLAG_TRACE_DATA) != 0)
                {
                    terminator_drive_trace_data(drive, terminator_io);
                }
            }
        }
        else
        {
            terminator_drive_trace_request(drive, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                             (FBE_TERMINATOR_DRIVE_DEBUG_FLAG_ALL_TRAFFIC | FBE_TERMINATOR_DRIVE_DEBUG_FLAG_WRITE_TRAFFIC),
                             "tio: %p l:%010llx b:%08llx op: %d start blk_size: %d\n",
                             terminator_io, terminator_io->lba, terminator_io->block_count,
                             terminator_io->opcode, terminator_io->block_size);
        }
    }
    return;
}

