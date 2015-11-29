/***************************************************************************
* Copyright (C) EMC Corporation 2010-2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!**************************************************************************
* @file fbe_database_fru_signature_process.c
***************************************************************************
*
* @brief
*  This file contains the implementaion of processing fru signature
*
* @version
*    7/18/2012 - Created. Gao Jian
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_database_private.h"
#include "fbe_fru_signature.h"
#include "fbe_private_space_layout.h"
#include "fbe_database_fru_signature_interface.h"
#include "fbe_database_config_tables.h"
#include "fbe_database_drive_connection.h"


/* The following functions are related to read/write and modify fru signature in Database
* *******************************************************************************
*/

/*!**************************************************************
 * @fn fbe_database_write_fru_signature_to_disk
 ****************************************************************
 * @brief
 *  this is a function write fru signature a disk
 *
 * @param[in]   bus, enclosure, slot - indicate which drive do we want to access
 * @param[in]   src_fru_signature - the fru signature to be wrote
 * @param[out]   operation_status - the write operation status return to caller
 *
 * 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no errror.
 *
 * @version
 *  7/19/2012 - Created. Gao Jian.
 *
 ****************************************************************/

fbe_status_t fbe_database_write_fru_signature_to_disk
(fbe_u32_t bus, fbe_u32_t enclosure, fbe_u32_t slot, 
fbe_fru_signature_t* in_fru_signature, fbe_database_fru_signature_IO_operation_status_t* operation_status)
{
    fbe_status_t status;
    database_physical_drive_info_t drive_info;
    fbe_object_id_t pdo_id;
    fbe_lifecycle_state_t pdo_state;
    fbe_database_physical_drive_info_t  pdo_info;
    fbe_u32_t raw_disk_IO_retry_count = 0;

    drive_info.port_number = bus;
    drive_info.enclosure_number = enclosure;
    drive_info.slot_number = slot;
    drive_info.block_geometry = FBE_BLOCK_EDGE_GEOMETRY_INVALID;

    if (in_fru_signature == NULL || operation_status == NULL)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "database write fru signature: invalid param, in_fru_signature == NULL or operation == NULL\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    status = fbe_database_get_pdo_object_id_by_location(bus, enclosure, slot, &pdo_id);
    if (FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "database get pdo: failed to get pdo id %u_%u_%u, status 0x%x\n", 
                     bus, enclosure, slot, status);
        *operation_status = FBE_DATABASE_FRU_SIGNATURE_IO_OPERATION_STATUS_DRIVE_UNACCESSIBLE;
        return status;
    }

    status = fbe_database_generic_get_object_state(pdo_id, &pdo_state, FBE_PACKAGE_ID_PHYSICAL);
    if (FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "database get pdo state: failed to get pdo state, object id: 0x%x, status 0x%x\n", 
                     pdo_id, status);
        *operation_status = FBE_DATABASE_FRU_SIGNATURE_IO_OPERATION_STATUS_DRIVE_UNACCESSIBLE;
        return status;
    }
    //@TODO: is check lifecycle necessary.
    if (pdo_state != FBE_LIFECYCLE_STATE_READY)
    {
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "pdo state: pdo is not in READY state, object id: 0x%x\n", 
                     pdo_id);
        *operation_status = FBE_DATABASE_FRU_SIGNATURE_IO_OPERATION_STATUS_DRIVE_UNACCESSIBLE;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get pdo info before we write signature for later re-check in case that the drive
     * corresponding to this object id changed during the read
     */
    status = fbe_database_get_pdo_info(pdo_id, &pdo_info);
    if (FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "database get pdo info: failed to get pdo info: 0x%x/0x%x\n", 
                     pdo_id, status);
        *operation_status = FBE_DATABASE_FRU_SIGNATURE_IO_OPERATION_STATUS_DRIVE_UNACCESSIBLE;
        return status;
    }

    drive_info.block_geometry = pdo_info.block_geometry;
    status = fbe_database_write_data_on_single_raw_drive((fbe_u8_t*)(in_fru_signature),
                                                         sizeof(fbe_fru_signature_t),
                                                         &drive_info,
                                                         FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_FRU_SIGNATURE);
    while((status != FBE_STATUS_OK) && 
                 (raw_disk_IO_retry_count< FBE_DB_FAILED_RAW_IO_RETRIES))
    {
        if (raw_disk_IO_retry_count > 0)
        {
            /* Wait 1 second if it is not the first retry */
            fbe_thread_delay(FBE_DB_FAILED_RAW_IO_RETRY_INTERVAL);
        }
        raw_disk_IO_retry_count++;
        status = fbe_database_write_data_on_single_raw_drive((fbe_u8_t*)(in_fru_signature),
                                                                                                                          sizeof(fbe_fru_signature_t),
                                                                                                                          &drive_info,
                                                                                                                          FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_FRU_SIGNATURE);
    }
    
    if (FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "database write data to disk: failed to write fru signature, object id: 0x%x, status 0x%x\n", 
                     pdo_id, status);
        *operation_status = FBE_DATABASE_FRU_SIGNATURE_IO_OPERATION_STATUS_DRIVE_UNACCESSIBLE;
        return status;
    } else
        *operation_status = FBE_DATABASE_FRU_SIGNATURE_IO_OPERATION_STATUS_ACCESS_OK;
    
    return status;
}


/*!***************************************************************
 * @fn fbe_database_clear_disk_fru_signature
 ****************************************************************
 * @brief
 *  this is a function clear disk fru signature
 *
 * @param[in]   bus, enclosure, slot - indicate which drive to be cleared
 * @param[out]   operation_status - the write operation status return to caller
 * 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no errror.
 *
 * @version
 *  7/25/2012 - Created. Gao Jian.
 *
 ****************************************************************/
fbe_status_t fbe_database_clear_disk_fru_signature
(fbe_u32_t bus, fbe_u32_t enclosure, fbe_u32_t slot, 
fbe_database_fru_signature_IO_operation_status_t* operation_status)
{
    fbe_fru_signature_t zero_fru_signature;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    
    fbe_zero_memory(&zero_fru_signature, sizeof(zero_fru_signature));
    status = fbe_database_write_fru_signature_to_disk
                       (bus, enclosure, slot, &zero_fru_signature, operation_status);
    
    return status;
}

/*!***************************************************************
 * @fn fbe_database_read_fru_signature_from_disk
 ****************************************************************
 * @brief
 *  this is a function read fru signature
 *
 * @param[in]   bus, enclosure, slot - indicate which drive to be cleared
 * @param[out]   out_fru_signature - return the fru signature to caller via it
 * @param[out]   operation_status - the write operation status return to caller
 * 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no errror.
 *
 * @version
 *  7/23/2012 - Created. Gao Jian.
 *
 ****************************************************************/
fbe_status_t fbe_database_read_fru_signature_from_disk
(fbe_u32_t bus, fbe_u32_t enclosure, fbe_u32_t slot,
fbe_fru_signature_t* out_fru_signature, fbe_database_fru_signature_IO_operation_status_t* operation_status)
{
    fbe_status_t status;
    database_physical_drive_info_t drive_info;
    fbe_database_physical_drive_info_t  pdo_info;
    fbe_database_physical_drive_info_t  new_pdo_info;
    fbe_object_id_t pdo_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t new_pdo_id = FBE_OBJECT_ID_INVALID;
    fbe_lifecycle_state_t pdo_state;
    
    fbe_u32_t raw_disk_IO_retry_count = 0;

    drive_info.port_number = bus;
    drive_info.enclosure_number = enclosure;
    drive_info.slot_number = slot;
    drive_info.block_geometry = FBE_BLOCK_EDGE_GEOMETRY_INVALID;
    drive_info.logical_drive_object_id = FBE_OBJECT_ID_INVALID;

    database_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                 "%s: start for %d_%d_%d.\n",
                 __FUNCTION__, bus, enclosure, slot);

    if (out_fru_signature == NULL || operation_status == NULL)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "database read fru signature: invalid param, out_fru_signature == NULL or operation == NULL\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_database_get_pdo_object_id_by_location(bus, enclosure, slot, &pdo_id);
    if (FBE_STATUS_OK != status || pdo_id == FBE_OBJECT_ID_INVALID)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "database get pdo: failed to get pdo id %u_%u_%u, status 0x%x\n", 
                     bus, enclosure, slot, status);
        *operation_status = FBE_DATABASE_FRU_SIGNATURE_IO_OPERATION_STATUS_DRIVE_UNACCESSIBLE;
        return status;
    }    

    status = fbe_database_generic_get_object_state(pdo_id, &pdo_state, FBE_PACKAGE_ID_PHYSICAL);
    if (FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "database get pdo state: failed to get pdo state, object id: 0x%x, status 0x%x\n", 
                     pdo_id, status);
        *operation_status = FBE_DATABASE_FRU_SIGNATURE_IO_OPERATION_STATUS_DRIVE_UNACCESSIBLE;
        return status;
    }

    if (pdo_state != FBE_LIFECYCLE_STATE_READY)
    {
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "pdo state: pdo is not in READY state, object id: 0x%x\n", 
                     pdo_id);
        *operation_status = FBE_DATABASE_FRU_SIGNATURE_IO_OPERATION_STATUS_DRIVE_UNACCESSIBLE;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get pdo info before we read signature for later re-check in case that the drive
     * corresponding to this object id changed during the read
     */
    status = fbe_database_get_pdo_info(pdo_id, &pdo_info);
    if (FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "database get pdo info: failed to get pdo info: 0x%x/0x%x\n", 
                     pdo_id, status);
        *operation_status = FBE_DATABASE_FRU_SIGNATURE_IO_OPERATION_STATUS_DRIVE_UNACCESSIBLE;
        return status;
    }

    /* at least make sure when we get pdo info the drive does not change*/
    if(pdo_info.bus != bus || pdo_info.enclosure != enclosure || pdo_info.slot != slot)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "%s: drive for objid 0x%x has changed to %d_%d_%d.\n", 
                     __FUNCTION__, pdo_id, pdo_info.bus, pdo_info.enclosure, pdo_info.slot);
        *operation_status = FBE_DATABASE_FRU_SIGNATURE_IO_OPERATION_STATUS_DRIVE_UNACCESSIBLE;
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    drive_info.block_geometry = pdo_info.block_geometry;
    status = fbe_database_read_data_from_single_raw_drive((fbe_u8_t*)(out_fru_signature), 
                                                          sizeof(fbe_fru_signature_t), 
                                                          &drive_info, 
                                                          FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_FRU_SIGNATURE);
    while((status != FBE_STATUS_OK)
          && (drive_info.logical_drive_object_id != FBE_OBJECT_ID_INVALID) /*if the object does not exist anymore, no need to retry*/
          && (raw_disk_IO_retry_count< FBE_DB_FAILED_RAW_IO_RETRIES))
    {
        if (raw_disk_IO_retry_count > 0)
        {
            /* Wait 1 second if it is not the first retry */
            fbe_thread_delay(FBE_DB_FAILED_RAW_IO_RETRY_INTERVAL);
        }
        raw_disk_IO_retry_count++;
        drive_info.logical_drive_object_id = FBE_OBJECT_ID_INVALID;
        status = fbe_database_read_data_from_single_raw_drive((fbe_u8_t*)(out_fru_signature), 
                                                              sizeof(fbe_fru_signature_t), 
                                                              &drive_info, 
                                                              FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_FRU_SIGNATURE);
    }
    
    if (FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "database get pdo state: failed to read from raw drive, object id: %d, status 0x%x\n", 
                     pdo_id, status);
        *operation_status = FBE_DATABASE_FRU_SIGNATURE_IO_OPERATION_STATUS_DRIVE_UNACCESSIBLE;
        return status;
    }

    /* re-check that the drive corresponding to the object id is still the original drive*/
    status = fbe_database_get_pdo_object_id_by_location(bus, enclosure, slot, &new_pdo_id);
    if (FBE_STATUS_OK != status || new_pdo_id == FBE_OBJECT_ID_INVALID)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "database get pdo: failed to get pdo id %u_%u_%u, status 0x%x\n", 
                     bus, enclosure, slot, status);
        *operation_status = FBE_DATABASE_FRU_SIGNATURE_IO_OPERATION_STATUS_DRIVE_UNACCESSIBLE;
        return status;
    }

    /*in case a new drive inserted to the same location*/
    if(new_pdo_id != pdo_id)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "%s: object corresponding to %d_%d_%d has changed from 0x%x to 0x%x.\n", 
                     __FUNCTION__, bus, enclosure, slot, pdo_id, new_pdo_id);
        *operation_status = FBE_DATABASE_FRU_SIGNATURE_IO_OPERATION_STATUS_DRIVE_UNACCESSIBLE;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_database_get_pdo_info(new_pdo_id, &new_pdo_info);
    if (FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "database get pdo info: failed to get pdo info: 0x%x/0x%x\n", 
                     new_pdo_id, status);
        *operation_status = FBE_DATABASE_FRU_SIGNATURE_IO_OPERATION_STATUS_DRIVE_UNACCESSIBLE;
        return status;
    }

    /* in case a new drive reuses the same object id*/
    if(new_pdo_info.bus != bus || new_pdo_info.enclosure != enclosure || new_pdo_info.slot != slot)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "%s: drive for objid 0x%x has changed to %d_%d_%d.\n", 
                     __FUNCTION__, new_pdo_id, new_pdo_info.bus, new_pdo_info.enclosure, new_pdo_info.slot);
        *operation_status = FBE_DATABASE_FRU_SIGNATURE_IO_OPERATION_STATUS_DRIVE_UNACCESSIBLE;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(fbe_compare_string(&pdo_info.serial_num[0], 
                            FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE, 
                            &new_pdo_info.serial_num[0], 
                            FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE, 
                            FBE_TRUE))
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "%s: drive for objid 0x%x has changed to %d_%d_%d.\n", 
                     __FUNCTION__, new_pdo_id, pdo_info.bus, pdo_info.enclosure, pdo_info.slot);
        *operation_status = FBE_DATABASE_FRU_SIGNATURE_IO_OPERATION_STATUS_DRIVE_UNACCESSIBLE;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*now we feel relieved that no drive change happened during our reading*/
    
    if (!fbe_compare_string(out_fru_signature->magic_string, 
                            FBE_FRU_SIGNATURE_MAGIC_STRING_SIZE, 
                            FBE_FRU_SIGNATURE_MAGIC_STRING, 
                            FBE_FRU_SIGNATURE_MAGIC_STRING_SIZE, 
                            FBE_FALSE))
    {
        *operation_status = FBE_DATABASE_FRU_SIGNATURE_IO_OPERATION_STATUS_ACCESS_OK;
        return status;
    }
    else
    {
        *operation_status = FBE_DATABASE_FRU_SIGNATURE_IO_OPERATION_STATUS_UNINITIALIZED;
        return status;
    }

    database_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                 "%s: end for %d_%d_%d.\n",
                 __FUNCTION__, bus, enclosure, slot);
    
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_validate_fru_signature
 ****************************************************************
 * @brief
 *  this is a function validate fru signature's version and magic string
 *
 * @param[in]      fru_sig -the fru signature to be validated
 * @param[out]   signature_status - validating result return to caller
 * 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no errror.
 *
 * @version
 *  7/23/2012 - Created. Gao Jian.
 *
 ****************************************************************/
fbe_status_t fbe_database_validate_fru_signature
(fbe_fru_signature_t* fru_sig, fbe_database_fru_signature_validation_status_t* signature_status)
{
    fbe_bool_t magic_string_match = FBE_FALSE;
    fbe_bool_t version_match = FBE_FALSE;

    if (fru_sig == NULL)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "database validate fru signature: invalid param, fru_sig == NULL");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Check the magic string of fru signature */
    if (!fbe_compare_string(fru_sig->magic_string,
                                                    FBE_FRU_SIGNATURE_MAGIC_STRING_SIZE,
                                                    FBE_FRU_SIGNATURE_MAGIC_STRING,
                                                    FBE_FRU_SIGNATURE_MAGIC_STRING_SIZE, 
                                                    FBE_FALSE))
    {
        magic_string_match = FBE_TRUE;
    }
    else
    {
        magic_string_match = FBE_FALSE;
    }

    /* Check the version of fru signature */
    if (fru_sig->version == FBE_FRU_SIGNATURE_VERSION)
    {
        version_match = FBE_TRUE;
    }
    else
    {
        version_match = FBE_FALSE;
    }

    if ((FBE_FALSE == magic_string_match) && (FBE_FALSE == version_match))
    {
        *signature_status = 
            FBE_DATABASE_FRU_SIGNATURE_VALIDATION_STATUS_MAGIC_STRING_AND_VERSION_ERROR;
    }
    else if (FBE_FALSE == magic_string_match)
    {
        *signature_status = 
            FBE_DATABASE_FRU_SIGNATURE_VALIDATION_STATUS_MAGIC_STRING_ERROR;            
    }
    else if (FBE_FALSE == version_match)
    {
        *signature_status =
            FBE_DATABASE_FRU_SIGNATURE_VALIDATION_STATUS_VERSION_ERROR;
    }
    else
    {
        *signature_status = 
            FBE_DATABASE_FRU_SIGNATURE_VALIDATION_STATUS_OK;
    }
    
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_stamp_fru_signature_to_system_drives
 ****************************************************************
 * @brief
 *  this is a function write fru signature all system drives
 *  An assumption of this function, the first system drive locates in bus 0, enc 0, slot 0
 *  and, other system drives follow the first system drive close orderly. 
 *  All system drives are in the bus 0 enc 0.
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no errror.
 *
 * @version
 *  7/23/2012 - Created. Gao Jian.
 *
 ****************************************************************/
fbe_status_t fbe_database_stamp_fru_signature_to_system_drives()
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t system_drive_number = 0;
    fbe_u32_t i = 0;
    fbe_u32_t chassis_wwn_seed;
    fbe_fru_signature_t fru_sig;
    fbe_database_fru_signature_IO_operation_status_t operation_status;
    fbe_u32_t write_error_count = 0;
    
    /*Get wwn seed from midplane prom*/
    status = fbe_database_get_board_resume_prom_wwnseed_with_retry(&chassis_wwn_seed);
    if (FBE_STATUS_OK != status)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: Failed to read PROM wwn_seed\n",
                    __FUNCTION__);

        return status;
    }

    /* initialize some attributes of system drives, based on the assumptions about system drives */
    fru_sig.bus = 0;
    fru_sig.enclosure = 0;
    fru_sig.system_wwn_seed = chassis_wwn_seed;
    fru_sig.version = FBE_FRU_SIGNATURE_VERSION;
    fbe_copy_memory(fru_sig.magic_string, 
                                           FBE_FRU_SIGNATURE_MAGIC_STRING, 
                                           FBE_FRU_SIGNATURE_MAGIC_STRING_SIZE);
    
    system_drive_number = fbe_private_space_layout_get_number_of_system_drives();
    for (i = 0 ; i < system_drive_number; i++)
    {
        fru_sig.slot = i;
        status = fbe_database_write_fru_signature_to_disk(0, 
                                                                                                                 0, 
                                                                                                                  i, 
                                                                                                                 &fru_sig, 
                                                                                                                 &operation_status);
        if (FBE_STATUS_OK != status || 
            operation_status != FBE_DATABASE_FRU_SIGNATURE_IO_OPERATION_STATUS_ACCESS_OK)
        {
            database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Failed to write fru signature to disk bus %d, enc %d, slot %d\n",
                        __FUNCTION__, fru_sig.bus, fru_sig.enclosure, fru_sig.slot);
            write_error_count++;
        }
    }

    if (0 == write_error_count)
        /* wirte OK */
        return FBE_STATUS_OK;
    else
        return FBE_STATUS_GENERIC_FAILURE;
}

/*!**************************************************************************************************
@fn fbe_database_get_homewrecker_fru_signature()
*****************************************************************************************************

* @brief
*  Encapsulate fru signature read for Homewrecker. 
*  
* @param out_fru_descriptor- output fru signature for read
* @param 
* @param 
* 
* @return
*  
*
*****************************************************************************************************/
fbe_status_t  
fbe_database_get_homewrecker_fru_signature(fbe_fru_signature_t* fru_sig, 
                                                            fbe_homewrecker_signature_report_t *info,
                                                            int bus, int enclosure, int slot)
{    
    fbe_status_t    status = FBE_STATUS_INVALID;
    fbe_database_fru_signature_IO_operation_status_t operation_status;
    if (fru_sig == NULL || info == NULL)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, 
                      __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }    
    status = fbe_database_read_fru_signature_from_disk(bus, enclosure, slot, fru_sig, &operation_status);
    if (FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                     "Homewrecker bus:%d encl:%d slot:%d signature read failed\n", 
                     bus, enclosure, slot);
    }
    info->io_status = operation_status;

    return status;
}


