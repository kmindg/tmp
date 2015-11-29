/***************************************************************************
* Copyright (C) EMC Corporation 2010-2014
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!**************************************************************************
* @file fbe_database_util.c
***************************************************************************
*
* @brief
*  This file contains utility function for the database service.
*  
* @version
*   06/27/2014  Ron Proulx  - Created.
*
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_database_private.h"
#include "fbe_database_config_tables.h"
#include "fbe_job_service.h"
#include "fbe/fbe_sector_trace_interface.h"
#include "fbe_cmi.h"

/******************** 
 * LOCAL DEFINITIONS
 ********************/

 
/*!****************************************************************************
 *          fbe_database_util_load_from_persist_read_completion_function()
 ****************************************************************************** 
 * 
 * @brief   Completion function for reading a group of database entries.
 * 
 * @param   read_status - Read status
 * @param   next_entry - Next entry_id expected
 * @param   entries_read - Number of entries in this read data
 * @param   context - Completion context
 * 
 * @return  status - status of the operation.
 *
 * @author
 *  06/27/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_database_util_load_from_persist_read_completion_function(fbe_status_t read_status, 
                                                                            fbe_persist_entry_id_t next_entry, 
                                                                            fbe_u32_t entries_read, 
                                                                            fbe_persist_completion_context_t context)
{
    database_init_read_context_t    * read_context = (database_init_read_context_t    *)context;

    if (read_status != FBE_STATUS_OK)
    {    
        /* log error*/
        if(read_status == FBE_STATUS_IO_FAILED_RETRYABLE) {
            read_context->status = read_status;
            fbe_semaphore_release(&read_context->sem, 0, 1, FALSE);
            return FBE_STATUS_OK;
        }  
    }
    read_context->next_entry_id= next_entry;
    read_context->entries_read = entries_read;
    read_context->status = FBE_STATUS_OK;
    fbe_semaphore_release(&read_context->sem, 0, 1, FALSE);
    return FBE_STATUS_OK;
}
/********************************************************************
 * end fbe_database_util_load_from_persist_read_completion_function()
 ********************************************************************/

/*!****************************************************************************
 *          fbe_database_util_get_validate_entry_from_object_id()
 ****************************************************************************** 
 * 
 * @brief   Using the object id (and possibly edge index) get the associated
 *          validation entry.
 * 
 * @param   validate_table_p - Validation table pointer
 * @param   object_id - Used to generate table index
 * @param   client_index - If valid this is a edge entry
 * @param   out_validate_entry_pp - Address of pointer to entry
 * 
 * @return  status - status of the operation.
 *
 * @author
 *  07/02/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_database_util_get_validate_entry_from_object_id(database_validate_table_t *validate_table_p,
                                                                 fbe_object_id_t object_id,
                                                                 fbe_edge_index_t client_index,
                                                                 database_validate_entry_t **out_validate_entry_pp)
{
    database_validate_entry_t  *entry_start_p = validate_table_p->validate_entry_p;
    fbe_u32_t                   entry_index = 0;

    if(validate_table_p->validate_entry_p == NULL) {
        *out_validate_entry_pp = NULL;
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: Table ptr is invalid, table is either not init'd or destroyed!\n", 
                       __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (client_index != FBE_EDGE_INDEX_INVALID) {
        entry_index = object_id*DATABASE_MAX_EDGE_PER_OBJECT + client_index;
    } else {
        entry_index = object_id;
    }
    if (entry_index < validate_table_p->table_size) {
        *out_validate_entry_pp = &entry_start_p[entry_index];
        return FBE_STATUS_OK;
    }

    *out_validate_entry_pp = NULL;
    database_trace(FBE_TRACE_LEVEL_ERROR, 
                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                   "%s: obj: 0x%x client: %d entry index: %d greater than table size: %d\n", 
                   __FUNCTION__, object_id, client_index, entry_index,
                   validate_table_p->table_size);
    return FBE_STATUS_GENERIC_FAILURE;
}
/***********************************************************
 * end fbe_database_util_get_validate_entry_from_object_id()
 ***********************************************************/

/*!****************************************************************************
 *          fbe_database_util_trace_failing_validate_entry()
 ****************************************************************************** 
 * 
 * @brief   Trace the failing valdiate entry information.
 * 
 * @param   validate_entry_p - The object data 
 * @param   entry_type_string_p - Describes database type
 * 
 * @return  none
 *
 * @author
 *  07/02/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static void fbe_database_util_trace_failing_validate_entry(database_validate_entry_t *validate_entry_p,
                                                           const fbe_u8_t *entry_type_string_p)
{
    /* Trace validate entry data
     */
    database_trace(FBE_TRACE_LEVEL_WARNING, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "validate_database: %s validate entry found: %d\n",
                   entry_type_string_p, validate_entry_p->b_entry_found);

    return;
}
/****************************************************
 * end fbe_database_util_trace_failing_validate_entry()
 ****************************************************/

/*!****************************************************************************
 *          fbe_database_util_trace_failing_header_details()
 ****************************************************************************** 
 * 
 * @brief   Trace the failing header entry information.
 * 
 * @param   header_p - The header data 
 * @param   type_string_p - Describes database type
 * 
 * @return  none
 *
 * @author
 *  07/04/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static void fbe_database_util_trace_failing_header_details(database_table_header_t *header_p,
                                                           const fbe_u8_t *type_string_p)
{
    /* Trace object data
     */
    database_trace(FBE_TRACE_LEVEL_WARNING, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "validate_database: %s header obj: 0x%x entry: 0x%llx state: %d size: %d failed validation\n",
                   type_string_p, header_p->object_id, header_p->entry_id,
                   header_p->state, header_p->version_header.size); 

    return;
}
/****************************************************
 * end fbe_database_util_trace_failing_header_details()
 ****************************************************/

/*!****************************************************************************
 *          fbe_database_util_trace_failing_header_entry()
 ****************************************************************************** 
 * 
 * @brief   Trace the failing header entry information.
 * 
 * @param   disk_header_p - The header data from disk
 * @param   mem_header_p - The header data from in-memory table
 * @param   validate_entry_p - Validate entry pointer
 * 
 * @return  none
 *
 * @author
 *  07/04/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static void fbe_database_util_trace_failing_header_entry(database_table_header_t *disk_header_p,
                                                         database_table_header_t *mem_header_p,
                                                         database_validate_entry_t *validate_entry_p)
{
    /* Trace the on-disk header.
     */
    if (disk_header_p != NULL) {
        fbe_database_util_trace_failing_header_details(disk_header_p, "on-disk");
    }

    /* Trace the in-memory header.
     */
    if (mem_header_p != NULL) {
        fbe_database_util_trace_failing_header_details(mem_header_p, "in-mem");
    }

    /* Trace 
     */
    if (validate_entry_p != NULL) {
        fbe_database_util_trace_failing_validate_entry(validate_entry_p, "header");
    }

    return;
}
/****************************************************
 * end fbe_database_util_trace_failing_header_entry()
 ****************************************************/

/*!****************************************************************************
 *          fbe_database_util_trace_failing_object_details()
 ****************************************************************************** 
 * 
 * @brief   Trace the failing object entry information.
 * 
 * @param   object_p - The object data 
 * @param   type_string_p - Describes database type
 * 
 * @return  none
 *
 * @author
 *  07/02/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static void fbe_database_util_trace_failing_object_details(database_object_entry_t *object_p,
                                                           const fbe_u8_t *type_string_p)
{
    fbe_provision_drive_configuration_t        *pvd_config_p = NULL;
    fbe_database_control_vd_set_configuration_t *vd_config_p = NULL;
    fbe_raid_group_configuration_t             *rg_config_p = NULL;
    fbe_database_lun_configuration_t           *lu_config_p = NULL;

    /* Trace object data
     */
    database_trace(FBE_TRACE_LEVEL_WARNING, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "validate_database: %s object obj: 0x%x entry: 0x%llx state: %d size: %d failed validation\n",
                   type_string_p, object_p->header.object_id, object_p->header.entry_id,
                   object_p->header.state, object_p->header.version_header.size);
    database_trace(FBE_TRACE_LEVEL_WARNING, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "validate_database: %s object db class: %d \n",
                   type_string_p, object_p->db_class_id);

    /* Trace the object type specific information.
     */
    switch(object_p->db_class_id) {
        case DATABASE_CLASS_ID_LUN:
            lu_config_p = &object_p->set_config_union.lu_config;
            database_trace(FBE_TRACE_LEVEL_WARNING, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "validate_database: %s object lu gen: 0x%llx cap: 0x%llx flags: 0x%x\n",
                           type_string_p, lu_config_p->generation_number,
                           lu_config_p->capacity, lu_config_p->config_flags);
            break;

        case DATABASE_CLASS_ID_RAID_START:
        case DATABASE_CLASS_ID_MIRROR:
        case DATABASE_CLASS_ID_STRIPER:
        case DATABASE_CLASS_ID_PARITY:
        case DATABASE_CLASS_ID_RAID_END:
        case DATABASE_CLASS_ID_RAID_GROUP:
            rg_config_p = &object_p->set_config_union.rg_config;
            database_trace(FBE_TRACE_LEVEL_WARNING, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "validate_database: %s object rg gen: 0x%llx cap: 0x%llx raid type: %d update type: %d\n",
                           type_string_p, rg_config_p->generation_number,
                           rg_config_p->capacity, rg_config_p->raid_type,
                           rg_config_p->update_type);
            break;

        case DATABASE_CLASS_ID_VIRTUAL_DRIVE:
            vd_config_p = &object_p->set_config_union.vd_config;
            database_trace(FBE_TRACE_LEVEL_WARNING, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "validate_database: %s object vd gen: 0x%llx cap: 0x%llx mode: %d update type: %d\n",
                           type_string_p, vd_config_p->generation_number,
                           vd_config_p->capacity, vd_config_p->configuration_mode,
                           vd_config_p->update_vd_type);
            break;

        case DATABASE_CLASS_ID_PROVISION_DRIVE:
            pvd_config_p = &object_p->set_config_union.pvd_config;
            database_trace(FBE_TRACE_LEVEL_WARNING, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "validate_database: %s object pvd gen: 0x%llx cap: 0x%llx config type: %d update type: %d\n",
                           type_string_p, pvd_config_p->generation_number,
                           pvd_config_p->configured_capacity, pvd_config_p->config_type,
                           pvd_config_p->update_type);
            break;

        case DATABASE_CLASS_ID_BVD_INTERFACE:
            /* BVD interface does not have a configuration.
             */
            break;

        default:
            database_trace(FBE_TRACE_LEVEL_WARNING, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "validate_database: %s object unexpected db class: %d\n",
                           type_string_p, object_p->db_class_id);
            break;
    }

    return;
}
/****************************************************
 * end fbe_database_util_trace_failing_object_details()
 ****************************************************/

/*!****************************************************************************
 *          fbe_database_util_trace_failing_object_entry()
 ****************************************************************************** 
 * 
 * @brief   Trace the failing object entry information.
 * 
 * @param   disk_object_p - The object data from disk
 * @param   mem_object_p - The object data from in-memory table
 * @param   validate_entry_p - Validate entry pointer
 * 
 * @return  none
 *
 * @author
 *  07/02/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static void fbe_database_util_trace_failing_object_entry(database_object_entry_t *disk_object_p,
                                                         database_object_entry_t *mem_object_p,
                                                         database_validate_entry_t *validate_entry_p)
{
    /* Trace the on-disk object.
     */
    if (disk_object_p != NULL) {
        fbe_database_util_trace_failing_object_details(disk_object_p, "on-disk");
    }

    /* Trace the in-memory object.
     */
    if (mem_object_p != NULL) {
        fbe_database_util_trace_failing_object_details(mem_object_p, "in-mem");
    }

    /* Trace 
     */
    if (validate_entry_p != NULL) {
        fbe_database_util_trace_failing_validate_entry(validate_entry_p, "object");
    }

    return;
}
/****************************************************
 * end fbe_database_util_trace_failing_object_entry()
 ****************************************************/

/*!****************************************************************************
 *          fbe_database_util_validate_object_entry()
 ****************************************************************************** 
 * 
 * @brief   Validate the object_entry.
 * 
 * @param   in_table_ptr - In-memory table
 * @param   in_entry_ptr - Entry data read from disk
 * @param   validate_table_p - Pointer to validation table
 * 
 * @return  status - status of the operation.
 *
 * @author
 *  06/27/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_database_util_validate_object_entry(database_table_t *in_table_ptr,
                                                            database_object_entry_t *in_entry_ptr,
                                                            database_validate_table_t *validate_table_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    database_object_entry_t *out_entry_ptr = NULL;
    database_validate_entry_t *validate_entry_p = NULL;
    fbe_u32_t entry_size = 0, local_entry_size = 0;

    /* Get the validate entry from the object id.
     */
    status = fbe_database_util_get_validate_entry_from_object_id(validate_table_p,
                                                                 in_entry_ptr->header.object_id,
                                                                 FBE_EDGE_INDEX_INVALID,
                                                                 &validate_entry_p);
    if (status != FBE_STATUS_OK) {
        fbe_database_util_trace_failing_object_entry(in_entry_ptr, NULL, NULL);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: failed to get validate entry - status: 0x%x\n", 
                       __FUNCTION__, status);
        return status;
    }

    /* Check the size(loaded from disk), if the size is bigger than the size in software, 
     * report error and enter into service mode. The reson for that is: 
     * if the size in disk is bigger than size in software, 
     * the software writes a smaller size of data to disk, 
     * which may erase the data bigger than the software size
    */
    entry_size = in_entry_ptr->header.version_header.size;
    if (entry_size == 0) {
        fbe_database_util_trace_failing_object_entry(in_entry_ptr, NULL, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: obj: 0x%x size is zero\n", 
                       __FUNCTION__, in_entry_ptr->header.object_id);
        return FBE_STATUS_SERVICE_MODE;
    }
    
    if(in_entry_ptr->db_class_id == DATABASE_CLASS_ID_INVALID)
    {
        fbe_database_util_trace_failing_object_entry(in_entry_ptr, NULL, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: the class_id of the object_entry is invalid.\n", 
                       __FUNCTION__);
        return FBE_STATUS_SERVICE_MODE;
    }

    local_entry_size = database_common_object_entry_size(in_entry_ptr->db_class_id);
    if (entry_size > local_entry_size) {
        fbe_database_util_trace_failing_object_entry(in_entry_ptr, NULL, validate_entry_p);
		database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: the on-disk size: %d is larger than software size: %d\n", 
                       __FUNCTION__, entry_size, local_entry_size);
		return FBE_STATUS_SERVICE_MODE;
    }

    /* If there is a database entry on disk and it is not found in-memory
     * something is wrong.
     */
    fbe_spinlock_lock(&in_table_ptr->table_lock);
    status = fbe_database_config_table_get_object_entry_by_id(in_table_ptr,
                                                              in_entry_ptr->header.object_id,
                                                              &out_entry_ptr);
    fbe_spinlock_unlock(&in_table_ptr->table_lock);
    if((status != FBE_STATUS_OK)||(out_entry_ptr == NULL)) 
    {
        fbe_database_util_trace_failing_object_entry(in_entry_ptr, NULL, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: get object entry failed - status: 0x%x\n", 
                       __FUNCTION__, status);
        return FBE_STATUS_SERVICE_MODE;
    }

    /* Make sure the entry was not already marked `found'.
     */
    if (validate_entry_p->b_entry_found) {
        fbe_database_util_trace_failing_object_entry(in_entry_ptr, out_entry_ptr, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: in-memory obj: 0x%x entry id: 0x%llx already found!\n", 
                       __FUNCTION__, out_entry_ptr->header.object_id, out_entry_ptr->header.entry_id);
        return FBE_STATUS_SERVICE_MODE;
    }

    /* Mark the entry found.
     */
    validate_entry_p->b_entry_found = FBE_TRUE;

    /* Simply compare the entries.
     */
    if (out_entry_ptr->header.version_header.size != entry_size) {
        in_entry_ptr->header.version_header.size = out_entry_ptr->header.version_header.size;
    }
    entry_size = FBE_MIN(entry_size, local_entry_size);
    if (!fbe_equal_memory(out_entry_ptr, in_entry_ptr, entry_size)) {
        fbe_database_util_trace_failing_object_entry(in_entry_ptr, out_entry_ptr, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "database validate object: in-memory: obj: 0x%x entry_id: 0x%llx not equal persist: obj: 0x%x entry_id: 0x%llx \n",
                       out_entry_ptr->header.object_id, out_entry_ptr->header.entry_id,
                       in_entry_ptr->header.object_id, in_entry_ptr->header.entry_id);
        return FBE_STATUS_SERVICE_MODE;
    }

    return FBE_STATUS_OK;
}
/****************************************************** 
 * end fbe_database_util_validate_object_entry()
 *****************************************************/

/*!****************************************************************************
 *          fbe_database_util_trace_failing_edge_details()
 ****************************************************************************** 
 * 
 * @brief   Trace the failing edge entry information.
 * 
 * @param   edge_p - The edge data 
 * @param   type_string_p - Describes database type
 * 
 * @return  none
 *
 * @author
 *  07/02/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static void fbe_database_util_trace_failing_edge_details(database_edge_entry_t *edge_p,
                                                         const fbe_u8_t *type_string_p)
{
    /* Trace edge data
     */
    database_trace(FBE_TRACE_LEVEL_WARNING, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "validate_database: %s edge obj: 0x%x entry: 0x%llx state: %d size: %d failed validation \n",
                   type_string_p, edge_p->header.object_id, edge_p->header.entry_id,
                   edge_p->header.state, edge_p->header.version_header.size);
    database_trace(FBE_TRACE_LEVEL_WARNING, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "validate_database: %s edge client index: %d \n",
                   type_string_p, edge_p->client_index);

    /* Trace the edge type specific information.
     */
    database_trace(FBE_TRACE_LEVEL_WARNING, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "validate_database: %s edge server obj: 0x%x cap: 0x%llx offset: 0x%llx\n",
                   type_string_p, edge_p->server_id, edge_p->capacity,
                   edge_p->offset);

    return;
}
/****************************************************
 * end fbe_database_util_trace_failing_edge_details()
 ****************************************************/

/*!****************************************************************************
 *          fbe_database_util_trace_failing_edge_entry()
 ****************************************************************************** 
 * 
 * @brief   Trace the failing edge entry information.
 * 
 * @param   disk_edge_p - The edge data from disk
 * @param   mem_edge_p - The edge data from in-memory table
 * @param   validate_entry_p - Validate entry pointer
 * 
 * @return  none
 *
 * @author
 *  07/02/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static void fbe_database_util_trace_failing_edge_entry(database_edge_entry_t *disk_edge_p,
                                                       database_edge_entry_t *mem_edge_p,
                                                       database_validate_entry_t *validate_entry_p)
{
    /* Trace the on-disk edge.
     */
    if (disk_edge_p != NULL) {
        fbe_database_util_trace_failing_edge_details(disk_edge_p, "on-disk");
    }

    /* Trace the in-memory object.
     */
    if (mem_edge_p != NULL) {
        fbe_database_util_trace_failing_edge_details(mem_edge_p, "in-mem");
    }

    /* Trace 
     */
    if (validate_entry_p != NULL) {
        fbe_database_util_trace_failing_validate_entry(validate_entry_p, "edge");
    }

    return;
}
/****************************************************
 * end fbe_database_util_trace_failing_edge_entry()
 ****************************************************/

/*!****************************************************************************
 *          fbe_database_util_validate_edge_table_entry()
 ****************************************************************************** 
 * 
 * @brief   Validate the edge entry.
 * 
 * @param   in_table_ptr - In-memory table
 * @param   in_entry_ptr - Entry data read from disk
 * @param   validate_table_p - Pointer to validation table
 * 
 * @return  status - status of the operation.
 *
 * @author
 *  06/27/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_database_util_validate_edge_entry(database_table_t *in_table_ptr,
                                                          database_edge_entry_t *in_entry_ptr,
                                                          database_validate_table_t *validate_table_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    database_edge_entry_t *out_entry_ptr = NULL;
    database_validate_entry_t *validate_entry_p = NULL;
    fbe_u32_t entry_size = 0, local_entry_size = 0;

    /* Get the validate entry from the object id.
     */
    status = fbe_database_util_get_validate_entry_from_object_id(validate_table_p,
                                                                 in_entry_ptr->header.object_id,
                                                                 in_entry_ptr->client_index,
                                                                 &validate_entry_p);
    if (status != FBE_STATUS_OK) {
        fbe_database_util_trace_failing_edge_entry(in_entry_ptr, NULL, NULL);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: failed to get validate entry - status: 0x%x\n", 
                       __FUNCTION__, status);
        return status;
    }

    /* Check the size(loaded from disk), if the size is bigger than the size in software, 
     * report error and enter into service mode. The reson for that is: 
     * if the size in disk is bigger than size in software, 
     * the software writes a smaller size of data to disk, 
     * which may erase the data bigger than the software size
    */
    entry_size = in_entry_ptr->header.version_header.size;
    if (entry_size == 0) {
        fbe_database_util_trace_failing_edge_entry(in_entry_ptr, NULL, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: edge entry's size is zero\n", 
                       __FUNCTION__);
        return FBE_STATUS_SERVICE_MODE;
    }
    local_entry_size = database_common_edge_entry_size();
    
	/*If the size value in the entry is bigger than the size of the data structure, enter service mode */
    if (entry_size > local_entry_size) {
        fbe_database_util_trace_failing_edge_entry(in_entry_ptr, NULL, validate_entry_p);
		database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: the on-disk size: %d is greater then softwaqre size: %d\n", 
                       __FUNCTION__, entry_size, local_entry_size); 
		return FBE_STATUS_SERVICE_MODE;
	}

    fbe_spinlock_lock(&in_table_ptr->table_lock);
    status = fbe_database_config_table_get_edge_entry(in_table_ptr,
                                                      in_entry_ptr->header.object_id,
                                                      in_entry_ptr->client_index,
                                                      &out_entry_ptr);
    fbe_spinlock_unlock(&in_table_ptr->table_lock);
    if((status != FBE_STATUS_OK)||(out_entry_ptr == NULL)) {
        fbe_database_util_trace_failing_edge_entry(in_entry_ptr, NULL, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: get edge entry failed - status: 0x%x\n", 
                       __FUNCTION__, status);
        return FBE_STATUS_SERVICE_MODE;
    }

    /* Make sure the entry was not already marked `found'.
     */
    if (validate_entry_p->b_entry_found) {
        fbe_database_util_trace_failing_edge_entry(in_entry_ptr, out_entry_ptr, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: in-memory obj: 0x%x entry id: 0x%llx already found!\n", 
                       __FUNCTION__, out_entry_ptr->header.object_id, out_entry_ptr->header.entry_id);
        return FBE_STATUS_SERVICE_MODE;
    }

    /* Mark the entry found.
     */
    validate_entry_p->b_entry_found = FBE_TRUE;

    /* Simply compare the entries.
     */
    if (out_entry_ptr->header.version_header.size != entry_size) {
        in_entry_ptr->header.version_header.size = out_entry_ptr->header.version_header.size;
    }
    entry_size = FBE_MIN(entry_size, local_entry_size);
    if (!fbe_equal_memory(out_entry_ptr, in_entry_ptr, entry_size)) {
        fbe_database_util_trace_failing_edge_entry(in_entry_ptr, out_entry_ptr, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "database validate edge: in-memory: obj: 0x%x entry_id: 0x%llx not equal persist: obj: 0x%x entry_id: 0x%llx \n",
                       out_entry_ptr->header.object_id, out_entry_ptr->header.entry_id,
                       in_entry_ptr->header.object_id, in_entry_ptr->header.entry_id);
        return FBE_STATUS_SERVICE_MODE;
    }

    return FBE_STATUS_OK;
}
/**************************************************** 
 * end fbe_database_util_validate_edge_entry()
 ***************************************************/

/*!****************************************************************************
 *          fbe_database_util_trace_failing_user_details()
 ****************************************************************************** 
 * 
 * @brief   Trace the failing user entry information.
 * 
 * @param   user_p - The user data 
 * @param   type_string_p - Describes database type
 * 
 * @return  none
 *
 * @author
 *  07/02/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static void fbe_database_util_trace_failing_user_details(database_user_entry_t *user_p,
                                                         const fbe_u8_t *type_string_p)
{
    database_pvd_user_data_t   *pvd_config_p = NULL;
    database_rg_user_data_t    *rg_config_p = NULL;
    database_lu_user_data_t    *lu_config_p = NULL;

    /* Trace user data
     */
    database_trace(FBE_TRACE_LEVEL_WARNING, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "validate_database: %s user obj: 0x%x entry: 0x%llx state: %d size: %d failed validation\n",
                   type_string_p, user_p->header.object_id, user_p->header.entry_id,
                   user_p->header.state, user_p->header.version_header.size);
    database_trace(FBE_TRACE_LEVEL_WARNING, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "validate_database: %s user db class: %d \n",
                   type_string_p, user_p->db_class_id);  

    /* Trace the object type specific information.
     */
    switch(user_p->db_class_id) {
        case DATABASE_CLASS_ID_LUN:
            lu_config_p = &user_p->user_data_union.lu_user_data;
            database_trace(FBE_TRACE_LEVEL_WARNING, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "validate_database: %s user lu number: %d attributes: 0x%x bind time: 0x%llx\n",
                           type_string_p, lu_config_p->lun_number, 
                           lu_config_p->attributes, lu_config_p->bind_time);
            break;

        case DATABASE_CLASS_ID_RAID_START:
        case DATABASE_CLASS_ID_MIRROR:
        case DATABASE_CLASS_ID_STRIPER:
        case DATABASE_CLASS_ID_PARITY:
        case DATABASE_CLASS_ID_RAID_END:
        case DATABASE_CLASS_ID_RAID_GROUP:
        case DATABASE_CLASS_ID_VIRTUAL_DRIVE:
            rg_config_p = &user_p->user_data_union.rg_user_data;
            database_trace(FBE_TRACE_LEVEL_WARNING, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "validate_database: %s user rg number: %d user private: %d\n",
                           type_string_p, rg_config_p->raid_group_number,
                           rg_config_p->user_private);
            break;

        case DATABASE_CLASS_ID_PROVISION_DRIVE:
            pvd_config_p = &user_p->user_data_union.pvd_user_data;
            database_trace(FBE_TRACE_LEVEL_WARNING, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "validate_database: %s user pvd pool id: 0x%x\n",
                           type_string_p, pvd_config_p->pool_id);
            break;

        case DATABASE_CLASS_ID_BVD_INTERFACE:
        default:
            database_trace(FBE_TRACE_LEVEL_WARNING, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "validate_database: %s user unexpected db class: %d\n",
                           type_string_p, user_p->db_class_id);
            break;
    }

    return;
}
/****************************************************
 * end fbe_database_util_trace_failing_user_details()
 ****************************************************/

/*!****************************************************************************
 *          fbe_database_util_trace_failing_user_entry()
 ****************************************************************************** 
 * 
 * @brief   Trace the failing user entry information.
 * 
 * @param   disk_user_p - The user data from disk
 * @param   mem_user_p - The user data from in-memory table
 * @param   validate_entry_p - Validate entry pointer
 * 
 * @return  none
 *
 * @author
 *  07/02/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static void fbe_database_util_trace_failing_user_entry(database_user_entry_t *disk_user_p,
                                                       database_user_entry_t *mem_user_p,
                                                       database_validate_entry_t *validate_entry_p)
{
    /* Trace the on-disk user entry.
     */
    if (disk_user_p != NULL) {
        fbe_database_util_trace_failing_user_details(disk_user_p, "on-disk");
    }

    /* Trace the in-memory user entry.
     */
    if (mem_user_p != NULL) {
        fbe_database_util_trace_failing_user_details(mem_user_p, "in-mem");
    }

    /* Trace 
     */
    if (validate_entry_p != NULL) {
        fbe_database_util_trace_failing_validate_entry(validate_entry_p, "user");
    }

    return;
}
/****************************************************
 * end fbe_database_util_trace_failing_user_entry()
 ****************************************************/

/*!****************************************************************************
 *          fbe_database_util_validate_user_entry()
 ****************************************************************************** 
 * 
 * @brief   Validate the user entry.
 * 
 * @param   in_table_ptr - In-memory table
 * @param   in_entry_ptr - Entry data read from disk
 * @param   validate_table_p - Pointer to validation table
 * 
 * @return  status - status of the operation.
 *
 * @author
 *  06/27/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_database_util_validate_user_entry(database_table_t *in_table_ptr,
                                                          database_user_entry_t *in_entry_ptr,
                                                          database_validate_table_t *validate_table_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    database_user_entry_t *out_entry_ptr = NULL;
    database_validate_entry_t *validate_entry_p = NULL;
    fbe_u32_t entry_size = 0, local_entry_size = 0;

    /* Get the validate entry from the object id.
     */
    status = fbe_database_util_get_validate_entry_from_object_id(validate_table_p,
                                                                 in_entry_ptr->header.object_id,
                                                                 FBE_EDGE_INDEX_INVALID,
                                                                 &validate_entry_p);
    if (status != FBE_STATUS_OK) {
        fbe_database_util_trace_failing_user_entry(in_entry_ptr, NULL, NULL);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: failed to get validate entry - status: 0x%x\n", 
                       __FUNCTION__, status);
        return status;
    }

    /* Check the size(loaded from disk), if the size is bigger than the size in software, 
     * report error and enter into service mode. The reson for that is: 
     * if the size in disk is bigger than size in software, 
     * the software writes a smaller size of data to disk, 
     * which may erase the data bigger than the software size
    */
    entry_size = in_entry_ptr->header.version_header.size;
    if (entry_size == 0) {
        fbe_database_util_trace_failing_user_entry(in_entry_ptr, NULL, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: user entry's size is zero\n", 
                       __FUNCTION__);
        return FBE_STATUS_SERVICE_MODE;
    }

    if(in_entry_ptr->db_class_id == DATABASE_CLASS_ID_INVALID)
    {
        fbe_database_util_trace_failing_user_entry(in_entry_ptr, NULL, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: the class_id of the user_entry is invalid.\n", 
                       __FUNCTION__);
        return FBE_STATUS_SERVICE_MODE;
    }

    local_entry_size = database_common_user_entry_size(in_entry_ptr->db_class_id);
    if (entry_size > local_entry_size) {
        fbe_database_util_trace_failing_user_entry(in_entry_ptr, NULL, validate_entry_p);
		database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: the entry in software is smaller than the entry in disk or peer, entry degraded mode\n", 
                       __FUNCTION__);
		return FBE_STATUS_SERVICE_MODE;        
    }

    fbe_spinlock_lock(&in_table_ptr->table_lock);
    status = fbe_database_config_table_get_user_entry_by_object_id(in_table_ptr,
                                                          in_entry_ptr->header.object_id,
                                                          &out_entry_ptr);
    fbe_spinlock_unlock(&in_table_ptr->table_lock);
    if((status != FBE_STATUS_OK)||(out_entry_ptr == NULL)) {
        fbe_database_util_trace_failing_user_entry(in_entry_ptr, NULL, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: get user entry failed - status: 0x%x\n", 
                       __FUNCTION__, status);
        return FBE_STATUS_SERVICE_MODE;
    }

    /* Make sure the entry was not already marked `found'.
     */
    if (validate_entry_p->b_entry_found) {
        fbe_database_util_trace_failing_user_entry(in_entry_ptr, out_entry_ptr, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: in-memory obj: 0x%x entry id: 0x%llx already found!\n", 
                       __FUNCTION__, out_entry_ptr->header.object_id, out_entry_ptr->header.entry_id);
        return FBE_STATUS_SERVICE_MODE;
    }

    /* Mark the entry found.
     */
    validate_entry_p->b_entry_found = FBE_TRUE;

    /* Simply compare the entries.
     */
    if (out_entry_ptr->header.version_header.size != entry_size) {
        in_entry_ptr->header.version_header.size = out_entry_ptr->header.version_header.size;
    }
    entry_size = FBE_MIN(entry_size, local_entry_size);
    if (!fbe_equal_memory(out_entry_ptr, in_entry_ptr, entry_size)) {
        fbe_database_util_trace_failing_user_entry(in_entry_ptr, out_entry_ptr, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "database validate user: in-memory: obj: 0x%x entry_id: 0x%llx not equal persist: obj: 0x%x entry_id: 0x%llx \n",
                       out_entry_ptr->header.object_id, out_entry_ptr->header.entry_id,
                       in_entry_ptr->header.object_id, in_entry_ptr->header.entry_id);
        return FBE_STATUS_SERVICE_MODE;
    }

    return FBE_STATUS_OK;
}
/*************************************************** 
 * end fbe_database_util_validate_user_entry()
 ***************************************************/

/*!****************************************************************************
 *          fbe_database_util_trace_failing_global_details()
 ****************************************************************************** 
 * 
 * @brief   Trace the failing global entry information.
 * 
 * @param   global_p - The global data 
 * @param   type_string_p - Describes database type
 * 
 * @return  none
 *
 * @author
 *  07/02/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static void fbe_database_util_trace_failing_global_details(database_global_info_entry_t *global_p,
                                                           const fbe_u8_t *type_string_p)
{
    fbe_system_power_saving_info_t     *power_saving_info_p = NULL;
    fbe_system_spare_config_info_t     *spare_info_p = NULL;
    fbe_system_generation_info_t       *generation_info_p = NULL;
    fbe_system_time_threshold_info_t   *time_threshold_info_p = NULL;
    fbe_system_encryption_info_t       *encryption_info_p = NULL;
    fbe_global_pvd_config_t            *pvd_config_p = NULL;

    /* Trace global data
     */
    database_trace(FBE_TRACE_LEVEL_WARNING, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "validate_database: %s global obj: 0x%x entry: 0x%llx state: %d size: %d failed validation\n",
                   type_string_p, global_p->header.object_id, global_p->header.entry_id,
                   global_p->header.state, global_p->header.version_header.size);
    database_trace(FBE_TRACE_LEVEL_WARNING, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "validate_database: %s global type: %d \n",
                   type_string_p, global_p->type); 

    /* Trace the global type specific information.
     */
    switch(global_p->type) {
        case DATABASE_GLOBAL_INFO_TYPE_SYSTEM_POWER_SAVE:
            power_saving_info_p = &global_p->info_union.power_saving_info;
            database_trace(FBE_TRACE_LEVEL_WARNING, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "validate_database: %s global power save enabled: %d wait up minutes: 0x%llx stats: %d\n",
                           type_string_p, power_saving_info_p->enabled,
                           power_saving_info_p->hibernation_wake_up_time_in_minutes,
                           power_saving_info_p->stats_enabled);
            break;
        case DATABASE_GLOBAL_INFO_TYPE_SYSTEM_SPARE:
            spare_info_p = &global_p->info_union.spare_info;
            database_trace(FBE_TRACE_LEVEL_WARNING, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "validate_database: %s global permanent spare trigger seconds: 0x%llx\n",
                           type_string_p, spare_info_p->permanent_spare_trigger_time);
            break;
        case DATABASE_GLOBAL_INFO_TYPE_SYSTEM_GENERATION:
            generation_info_p = &global_p->info_union.generation_info;
            database_trace(FBE_TRACE_LEVEL_WARNING, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "validate_database: %s global config generation number: 0x%llx\n",
                           type_string_p, generation_info_p->current_generation_number);
            break;
        case DATABASE_GLOBAL_INFO_TYPE_SYSTEM_TIME_THRESHOLD:
            time_threshold_info_p = &global_p->info_union.time_threshold_info;
            database_trace(FBE_TRACE_LEVEL_WARNING, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "validate_database: %s global pvd garbage collection minutes: 0x%llx\n",
                           type_string_p, time_threshold_info_p->time_threshold_in_minutes);
            break;
        case DATABASE_GLOBAL_INFO_TYPE_SYSTEM_ENCRYPTION:
            encryption_info_p = &global_p->info_union.encryption_info;
            database_trace(FBE_TRACE_LEVEL_WARNING, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "validate_database: %s global encryption mode: %d paused: %d\n",
                           type_string_p, encryption_info_p->encryption_mode,
                           encryption_info_p->encryption_paused);
            break;
        case DATABASE_GLOBAL_INFO_TYPE_GLOBAL_PVD_CONFIG:
            pvd_config_p = &global_p->info_union.pvd_config;
            database_trace(FBE_TRACE_LEVEL_WARNING, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "validate_database: %s global pvd user capacity limit: 0x%x\n",
                           type_string_p, pvd_config_p->user_capacity_limit);
            break;
        default:
            database_trace(FBE_TRACE_LEVEL_WARNING, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "validate_database: %s global unexpected type: %d\n",
                           type_string_p, global_p->type);
            break;
    }

    return;
}
/****************************************************
 * end fbe_database_util_trace_failing_global_details()
 ****************************************************/

/*!****************************************************************************
 *          fbe_database_util_trace_failing_global_entry()
 ****************************************************************************** 
 * 
 * @brief   Trace the failing global entry information.
 * 
 * @param   disk_global_p - The global data from disk
 * @param   mem_global_p - The global data from in-memory table
 * @param   validate_entry_p - Validate entry pointer
 * 
 * @return  none
 *
 * @author
 *  07/02/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static void fbe_database_util_trace_failing_global_entry(database_global_info_entry_t *disk_global_p,
                                                         database_global_info_entry_t *mem_global_p,
                                                         database_validate_entry_t *validate_entry_p)
{
    /* Trace the on-disk global entry.
     */
    if (disk_global_p != NULL) {
        fbe_database_util_trace_failing_global_details(disk_global_p, "on-disk");
    }

    /* Trace the in-memory global entry.
     */
    if (mem_global_p != NULL) {
        fbe_database_util_trace_failing_global_details(mem_global_p, "in-mem");
    }

    /* Trace 
     */
    if (validate_entry_p != NULL) {
        fbe_database_util_trace_failing_validate_entry(validate_entry_p, "global");
    }

    return;
}
/****************************************************
 * end fbe_database_util_trace_failing_global_entry()
 ****************************************************/

/*!****************************************************************************
 *          fbe_database_util_validate_global_entry()
 ****************************************************************************** 
 * 
 * @brief   Validate the global entry.
 * 
 * @param   in_table_ptr - In-memory table
 * @param   in_entry_ptr - Entry data read from disk
 * @param   validate_table_p - Pointer to validation table
 * 
 * @return  status - status of the operation.
 *
 * @author
 *  06/27/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_database_util_validate_global_entry(database_table_t *in_table_ptr, 
                                                            database_global_info_entry_t *in_entry_ptr,
                                                            database_validate_table_t *validate_table_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    database_global_info_entry_t *out_entry_ptr = NULL;
    database_validate_entry_t *validate_entry_p = NULL;
    fbe_u32_t entry_size = 0, local_entry_size = 0;

    /* Get the validate entry from the type which is used to index
     * into the table.
     */
    status = fbe_database_util_get_validate_entry_from_object_id(validate_table_p,
                                                                 in_entry_ptr->type,
                                                                 FBE_EDGE_INDEX_INVALID,
                                                                 &validate_entry_p);
    if (status != FBE_STATUS_OK) {
        fbe_database_util_trace_failing_global_entry(in_entry_ptr, NULL, NULL);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: failed to get validate entry - status: 0x%x\n", 
                       __FUNCTION__, status);
        return status;
    }

    /* Check the size(loaded from disk), if the size is bigger than the size in software, 
     * report error and enter into service mode. The reson for that is: 
     * if the size in disk is bigger than size in software, 
     * the software writes a smaller size of data to disk, 
     * which may erase the data bigger than the software size
    */
    entry_size = in_entry_ptr->header.version_header.size;
    /* can't check zero, because the first global info entry is a NULL entry */
    /*
    if (entry_size == 0) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: entry's size is zero\n", 
                       __FUNCTION__);
        status = FBE_STATUS_GENERIC_FAILURE;
        return status;
    }
    */
    
    local_entry_size = database_common_global_info_entry_size(in_entry_ptr->type);
    if (entry_size > local_entry_size) {
        fbe_database_util_trace_failing_global_entry(in_entry_ptr, NULL, validate_entry_p);
		database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: on-disk size: %d is greater than software size: %d enter service mode\n", 
                       __FUNCTION__, entry_size, local_entry_size);
		return FBE_STATUS_SERVICE_MODE;  
    }

    status = fbe_database_config_table_get_global_info_entry(in_table_ptr, 
                                                             in_entry_ptr->type, 
                                                             &out_entry_ptr);
    if((status != FBE_STATUS_OK)||(out_entry_ptr == NULL)) {
        fbe_database_util_trace_failing_global_entry(in_entry_ptr, NULL, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: get global info entry failed - status: 0x%x\n", 
                       __FUNCTION__, status);
        return FBE_STATUS_SERVICE_MODE; 
    }

    /* Make sure the entry was not already marked `found'.
     */
    if (validate_entry_p->b_entry_found) {
        fbe_database_util_trace_failing_global_entry(in_entry_ptr, out_entry_ptr, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: in-memory obj: 0x%x entry id: 0x%llx already found!\n", 
                       __FUNCTION__, out_entry_ptr->header.object_id, out_entry_ptr->header.entry_id);
        return FBE_STATUS_SERVICE_MODE;
    }

    /* Mark the entry found.
     */
    validate_entry_p->b_entry_found = FBE_TRUE;

    /* Simply compare the entries.
     */
    if (out_entry_ptr->header.version_header.size != entry_size) {
        in_entry_ptr->header.version_header.size = out_entry_ptr->header.version_header.size;
    }
    entry_size = FBE_MIN(entry_size, local_entry_size);
    if (!fbe_equal_memory(out_entry_ptr, in_entry_ptr, entry_size)) {
        fbe_database_util_trace_failing_global_entry(in_entry_ptr, out_entry_ptr, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "database validate global: in-memory: obj: 0x%x entry_id: 0x%llx not equal persist: obj: 0x%x entry_id: 0x%llx \n",
                       out_entry_ptr->header.object_id, out_entry_ptr->header.entry_id,
                       in_entry_ptr->header.object_id, in_entry_ptr->header.entry_id);
        return FBE_STATUS_SERVICE_MODE;
    }

    return FBE_STATUS_OK;
}
/***************************************************** 
 * end fbe_database_util_validate_global_entry()
 *****************************************************/

/*!****************************************************************************
 *          fbe_database_util_trace_failing_system_spare_details()
 ****************************************************************************** 
 * 
 * @brief   Trace the failing system spare entry information.
 * 
 * @param   system_spare_p - The system spare data 
 * @param   type_string_p - Describes database type
 * 
 * @return  none
 *
 * @author
 *  07/03/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static void fbe_database_util_trace_failing_system_spare_details(database_system_spare_entry_t *system_spare_p,
                                                                 const fbe_u8_t *type_string_p)
{
    /* Trace system spare data
     */
    database_trace(FBE_TRACE_LEVEL_WARNING, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "validate_database: %s system spare obj: 0x%x entry: 0x%llx state: %d size: %d failed validation\n",
                   type_string_p, system_spare_p->header.object_id, system_spare_p->header.entry_id,
                   system_spare_p->header.state, system_spare_p->header.version_header.size);
    database_trace(FBE_TRACE_LEVEL_WARNING, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "validate_database: %s system spare spare obj: 0x%x \n",
                   type_string_p, system_spare_p->spare_drive_id);

    return;
}
/************************************************************
 * end fbe_database_util_trace_failing_system_spare_details()
 ************************************************************/

/*!****************************************************************************
 *          fbe_database_util_trace_failing_system_spare_entry()
 ****************************************************************************** 
 * 
 * @brief   Trace the failing system spare entry information.
 * 
 * @param   disk_system_spare_p - The system spare data from disk
 * @param   mem_system_spare_p - The system spare data from in-memory table
 * @param   validate_entry_p - Validate entry pointer
 * 
 * @return  none
 *
 * @author
 *  07/03/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static void fbe_database_util_trace_failing_system_spare_entry(database_system_spare_entry_t *disk_system_spare_p,
                                                               database_system_spare_entry_t *mem_system_spare_p,
                                                               database_validate_entry_t *validate_entry_p)
{
    /* Trace the on-disk system spare entry.
     */
    if (disk_system_spare_p != NULL) {
        fbe_database_util_trace_failing_system_spare_details(disk_system_spare_p, "on-disk");
    }

    /* Trace the in-memory system spare entry.
     */
    if (mem_system_spare_p != NULL) {
        fbe_database_util_trace_failing_system_spare_details(mem_system_spare_p, "in-mem");
    }

    /* Trace 
     */
    if (validate_entry_p != NULL) {
        fbe_database_util_trace_failing_validate_entry(validate_entry_p, "system spare");
    }

    return;
}
/**********************************************************
 * end fbe_database_util_trace_failing_system_spare_entry()
 *********************************************************/

/*!****************************************************************************
 *          fbe_database_util_validate_system_spare_entry()
 ****************************************************************************** 
 * 
 * @brief   Validate the system spare entry.
 * 
 * @param   in_table_ptr - In-memory table
 * @param   in_entry_ptr - Entry data read from disk
 * @param   validate_table_p - Pointer to validation table
 * 
 * @return  status - status of the operation.
 *
 * @author
 *  06/27/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_database_util_validate_system_spare_entry(database_table_t *in_table_ptr, 
                                                                  database_system_spare_entry_t *in_entry_ptr,
                                                                  database_validate_table_t *validate_table_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    database_system_spare_entry_t *out_entry_ptr = NULL;
    database_validate_entry_t *validate_entry_p = NULL;
    fbe_u32_t entry_size = 0;

    /* Get the validate entry from the object id.
     */
    status = fbe_database_util_get_validate_entry_from_object_id(validate_table_p,
                                                                 in_entry_ptr->header.object_id,
                                                                 FBE_EDGE_INDEX_INVALID,
                                                                 &validate_entry_p);
    if (status != FBE_STATUS_OK) {
        fbe_database_util_trace_failing_system_spare_entry(in_entry_ptr, NULL, NULL);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: failed to get validate entry - status: 0x%x\n", 
                       __FUNCTION__, status);
        return status;
    }

    /* Check the size(loaded from disk), if the size is bigger than the size in software, 
     * report error and enter into service mode. The reson for that is: 
     * if the size in disk is bigger than size in software, 
     * the software writes a smaller size of data to disk, 
     * which may erase the data bigger than the software size
    */
    entry_size = in_entry_ptr->header.version_header.size;
    if (entry_size == 0) {
        fbe_database_util_trace_failing_system_spare_entry(in_entry_ptr, NULL, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: entry's size is zero\n", 
                       __FUNCTION__);
        return FBE_STATUS_SERVICE_MODE;
    }

    status = fbe_database_config_table_get_system_spare_entry(in_table_ptr, 
                                                             in_entry_ptr->header.object_id, 
                                                             &out_entry_ptr);
    if((status != FBE_STATUS_OK)||(out_entry_ptr == NULL)) {
        fbe_database_util_trace_failing_system_spare_entry(in_entry_ptr, NULL, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: get system_spare entry failed - status: 0x%x\n", 
                       __FUNCTION__, status);
        return FBE_STATUS_SERVICE_MODE;
    }

    /* Make sure the entry was not already marked `found'.
     */
    if (validate_entry_p->b_entry_found) {
        fbe_database_util_trace_failing_system_spare_entry(in_entry_ptr, out_entry_ptr, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: in-memory obj: 0x%x entry id: 0x%llx already found!\n", 
                       __FUNCTION__, out_entry_ptr->header.object_id, out_entry_ptr->header.entry_id);
        return FBE_STATUS_SERVICE_MODE;
    }

    /* Mark the entry found.
     */
    validate_entry_p->b_entry_found = FBE_TRUE;

    /* Simply compare the entries.
     */
    if (out_entry_ptr->header.version_header.size != entry_size) {
        in_entry_ptr->header.version_header.size = out_entry_ptr->header.version_header.size;
    }
    if (!fbe_equal_memory(out_entry_ptr, in_entry_ptr, entry_size)) {
        fbe_database_util_trace_failing_system_spare_entry(in_entry_ptr, out_entry_ptr, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "database validate system spare: in-memory: obj: 0x%x entry_id: 0x%llx not equal persist: obj: 0x%x entry_id: 0x%llx \n",
                       out_entry_ptr->header.object_id, out_entry_ptr->header.entry_id,
                       in_entry_ptr->header.object_id, in_entry_ptr->header.entry_id);
        return FBE_STATUS_SERVICE_MODE;
    }

    return FBE_STATUS_OK;
}
/***************************************************** 
 * end fbe_database_util_validate_system_spare_entry()
 *****************************************************/

/*!****************************************************************************
 *          fbe_database_util_validate_entries_from_persist()
 ****************************************************************************** 
 * 
 * @brief   For the entries just read, validate them.
 * 
 * @param   read_buffer - Read buffer
 * @param   elements_read - Number of entries read
 * @param   table_type - Database table type
 * @param   validate_table_p - Pointer to validation table
 * 
 * @return  status - status of the operation.
 *
 * @author
 *  06/27/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_database_util_validate_entries_from_persist(fbe_u8_t *read_buffer, 
                                                                    fbe_u32_t elements_read, 
                                                                    database_config_table_type_t table_type,
                                                                    database_validate_table_t *validate_table_p)

{
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u8_t *                      current_data = read_buffer;
    fbe_u32_t                       elements = 0;
    fbe_persist_user_header_t      *user_header = NULL;
    database_table_header_t        *database_table_header = NULL;
    database_table_t               *table_ptr = NULL;
    database_object_entry_t        *object_entry_ptr = NULL;
    database_edge_entry_t          *edge_entry_ptr = NULL;
    database_user_entry_t          *user_entry_ptr = NULL;
    database_global_info_entry_t   *global_info_entry_ptr = NULL;
    database_system_spare_entry_t  *system_spare_entry_ptr = NULL;

    status = fbe_database_get_service_table_ptr(&table_ptr, table_type);
    if ((status != FBE_STATUS_OK)||(table_ptr == NULL)) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: get table type: %d failed - status: 0x%x\n", 
                       __FUNCTION__, table_type, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*let's go over the read buffer and copy the data to service table */
    for (elements = 0; elements < elements_read; elements++) {
        user_header = (fbe_persist_user_header_t *)current_data;
        current_data += sizeof(fbe_persist_user_header_t);
        database_table_header = (database_table_header_t *)current_data;

        /*! If both the persist header and database headers are 0, then this
         *  is not a valid entry.
         *
         *  @note Not clear if holes in the database are expected.  If not
         *  We could simply note that all the entries have been read.
         */
        if ((user_header->entry_id == 0)           &&
            (database_table_header->entry_id == 0)    ) {
            /* Skip any unused entries.
             */
            database_trace (FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: element: %d is empty\n",
                            __FUNCTION__, elements);
            current_data += (FBE_PERSIST_USER_DATA_BYTES_PER_ENTRY - sizeof(fbe_persist_user_header_t));
            continue;
        }

        /* Sanity check the entry id */
        if (user_header->entry_id != database_table_header->entry_id) {
            /* We expect persist entry id to be the same as the entry id stored in the database table.
            * If not, log an ERROR and return FBE_STATUS_SERVICE_MODE.
            */ 
            fbe_database_util_trace_failing_header_entry(database_table_header, NULL, NULL);
            database_trace (FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: PersistId 0x%llx != TableId 0x%llx. We have a corrupted database! QUIT booting\n",
                            __FUNCTION__,
                            (unsigned long long)user_header->entry_id,
                            (unsigned long long)database_table_header->entry_id);
            return FBE_STATUS_SERVICE_MODE;
        }

        /* we expect a valid entry */
        if (database_table_header->state != DATABASE_CONFIG_ENTRY_VALID) {
            fbe_database_util_trace_failing_header_entry(database_table_header, NULL, NULL);
            database_trace (FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: element: %d Entry id 0x%llx has unexpected state 0x%x. Skip it!\n",
                            __FUNCTION__, elements,
                            (unsigned long long)database_table_header->entry_id,
                            database_table_header->state);
            return FBE_STATUS_SERVICE_MODE;
        }

        /* Validate the entry.
         */
        switch(table_type) {
        case DATABASE_CONFIG_TABLE_OBJECT:
            object_entry_ptr = (database_object_entry_t *)current_data;
            status = fbe_database_util_validate_object_entry(table_ptr, object_entry_ptr, validate_table_p);
            break;
        case DATABASE_CONFIG_TABLE_EDGE:
            edge_entry_ptr = (database_edge_entry_t *)current_data;
            status = fbe_database_util_validate_edge_entry(table_ptr, edge_entry_ptr, validate_table_p);
            break;
        case DATABASE_CONFIG_TABLE_USER:
            user_entry_ptr = (database_user_entry_t *)current_data;
            status = fbe_database_util_validate_user_entry(table_ptr, user_entry_ptr, validate_table_p);
            break;
        case DATABASE_CONFIG_TABLE_GLOBAL_INFO:
            global_info_entry_ptr = (database_global_info_entry_t *)current_data;
            status = fbe_database_util_validate_global_entry(table_ptr, global_info_entry_ptr, validate_table_p);
            break;
        case DATABASE_CONFIG_TABLE_SYSTEM_SPARE:
            system_spare_entry_ptr = (database_system_spare_entry_t *)current_data;
            status = fbe_database_util_validate_system_spare_entry(table_ptr, system_spare_entry_ptr, validate_table_p);
            break;

        default:
            database_trace (FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: Table type:%d is not supported!\n",
                            __FUNCTION__,
                            table_type);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
        }
        if (status != FBE_STATUS_OK) {
            return status;
        }

        /*go to next entry*/
        current_data += (FBE_PERSIST_USER_DATA_BYTES_PER_ENTRY - sizeof(fbe_persist_user_header_t));
    }
    return FBE_STATUS_OK;
}
/*******************************************************
 * end fbe_database_util_validate_entries_from_persist()
 *******************************************************/

/*!****************************************************************************
 *          fbe_database_util_get_persist_entry_info()
 ****************************************************************************** 
 * 
 * @brief   Get persist entry info.
 * 
 * @param   in_entry_ptr - Entry data from in-memory table
 * @param   b_is_entry_in_use_p - Determines if entry is marlked in-use
 * 
 * @return  status - status of the operation.
 *
 * @author
 *  06/27/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_database_util_get_persist_entry_info(database_table_header_t *in_entry_ptr,
                                                      fbe_bool_t *b_is_entry_in_use_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_persist_control_get_entry_info_t    persist_entry_info;

    /* By default the entry is not in-use.*/
    *b_is_entry_in_use_p = FBE_FALSE;

    /*! @note Since the table is indexed by object id, there can be `holes'
     *        or `blank' / unused entries.  Skip any entries that are not valid.
     */
    if (in_entry_ptr->state != DATABASE_CONFIG_ENTRY_VALID) {
        return FBE_STATUS_OK;
    }

    /* Validate that the table entry is marked `in-use' in the bitmap.
     */
    persist_entry_info.entry_id = in_entry_ptr->entry_id;
    persist_entry_info.b_does_entry_exist = FBE_FALSE;
    status = fbe_database_send_packet_to_service(FBE_PERSIST_CONTROL_CODE_GET_ENTRY_INFO,
                                                 &persist_entry_info, 
                                                 sizeof(fbe_persist_control_get_entry_info_t),
                                                 FBE_SERVICE_ID_PERSIST,
                                                 NULL, /* no sg list*/
                                                 0,    /* no sg list*/
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: failed to get entry info for 0x%llx - status: 0x%x\n", 
                       __FUNCTION__, in_entry_ptr->entry_id, status);
        return status;
    }

    *b_is_entry_in_use_p = persist_entry_info.b_does_entry_exist;
    
    return FBE_STATUS_OK;
}
/************************************************ 
 * end fbe_database_util_get_persist_entry_info()
 ************************************************/

/*!****************************************************************************
 *          fbe_database_util_get_class_id_from_object_id()
 ****************************************************************************** 
 * 
 * @brief   Get the class id of the associated object.
 * 
 * @param   object_id - Object id to determine class for
 * @param   class_id_p - Pointer to class id to update
 * 
 * @return  status - status of the operation.
 *
 * @author
 *  07/03/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_database_util_get_class_id_from_object_id(fbe_object_id_t object_id,
                                                                  fbe_class_id_t *class_id_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_base_object_mgmt_get_class_id_t get_class_id;

    /* Validate the class of the object.
     */
    *class_id_p = FBE_CLASS_ID_INVALID;
    status = fbe_database_send_packet_to_service(FBE_BASE_OBJECT_CONTROL_CODE_GET_CLASS_ID,
                                                 &get_class_id, 
                                                 sizeof(fbe_base_object_mgmt_get_class_id_t),
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 NULL, /* no sg list*/
                                                 0,    /* no sg list*/
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: failed to get class id of obj: 0x%x - status: 0x%x\n", 
                       __FUNCTION__, object_id, status);
        return status;
    }

    *class_id_p = get_class_id.class_id;

    return FBE_STATUS_OK;
}
/********************************************************** 
 * end fbe_database_util_get_class_id_from_object_id()
 **********************************************************/

/*!****************************************************************************
 *          fbe_database_util_validate_object_entry_from_table()
 ****************************************************************************** 
 * 
 * @brief   Validate the object_entry.
 * 
 * @param   in_entry_ptr - Entry data from in-memory table
 * @param   validate_table_p - Pointer to validation table
 * 
 * @return  status - status of the operation.
 *
 * @author
 *  07/02/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_database_util_validate_object_entry_from_table(database_object_entry_t *in_entry_ptr,
                                                                       database_validate_table_t *validate_table_p)
{
    fbe_status_t                status = FBE_STATUS_GENERIC_FAILURE;
    database_validate_entry_t  *validate_entry_p = NULL;
    fbe_bool_t                  b_is_entry_in_use = FBE_FALSE;
    fbe_class_id_t              expected_class_id = FBE_CLASS_ID_INVALID;
    fbe_class_id_t              class_id_of_object = FBE_CLASS_ID_INVALID;

    /*! @note Since the table is indexed by object id, there can be `holes'
     *        or `blank' / unused entries.  Skip any entries that are not valid.
     */
    if (in_entry_ptr->header.state != DATABASE_CONFIG_ENTRY_VALID) {
        return FBE_STATUS_OK;
    }

    /* Special case is object is the first entry in the table or it is a 
     * special entry like the BVD interface that does not exist on-disk 
     * (entry id of: FBE_PERSIST_SECTOR_START_ENTRY_ID).  Skip these cases.
     */
    if (in_entry_ptr->header.entry_id == FBE_PERSIST_SECTOR_START_ENTRY_ID) {
        return FBE_STATUS_OK;
    }

    /* Get the validate entry from the object id.
     */
    expected_class_id = database_common_map_class_id_database_to_fbe(in_entry_ptr->db_class_id);
    status = fbe_database_util_get_validate_entry_from_object_id(validate_table_p,
                                                                 in_entry_ptr->header.object_id,
                                                                 FBE_EDGE_INDEX_INVALID,
                                                                 &validate_entry_p);
    if (status != FBE_STATUS_OK) {
        fbe_database_util_trace_failing_object_entry(NULL, in_entry_ptr, NULL);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: failed to get validate entry - status: 0x%x\n", 
                       __FUNCTION__, status);
        return status;
    }

    /* Validate that the table entry is marked `in-use' in the bitmap.
     */
    status = fbe_database_util_get_persist_entry_info((database_table_header_t *)in_entry_ptr, 
                                                      &b_is_entry_in_use);
    if (status != FBE_STATUS_OK) {
        fbe_database_util_trace_failing_object_entry(NULL, in_entry_ptr, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: failed to get persist entry info - status: 0x%x\n", 
                       __FUNCTION__, status);
        return status;
    }

    /* If the entry was not found on disk report the failure.
     */
    if (!validate_entry_p->b_entry_found) {
        fbe_database_util_trace_failing_object_entry(NULL, in_entry_ptr, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: obj: 0x%x entry_id: 0x%llx in-use: %d was not on-disk\n", 
                       __FUNCTION__, in_entry_ptr->header.object_id,
                       in_entry_ptr->header.entry_id, b_is_entry_in_use);
        return FBE_STATUS_SERVICE_MODE;
    }

    /* If the entry is not marked `in-use' report the error.
     */
    if (!b_is_entry_in_use) {
        fbe_database_util_trace_failing_object_entry(NULL, in_entry_ptr, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: obj: 0x%x entry_id: 0x%llx in-use: %d was not in-use\n", 
                       __FUNCTION__, in_entry_ptr->header.object_id,
                       in_entry_ptr->header.entry_id, b_is_entry_in_use);
        return FBE_STATUS_SERVICE_MODE;
    }

    /* Validate the class of the object.
     */
    status = fbe_database_util_get_class_id_from_object_id(in_entry_ptr->header.object_id, &class_id_of_object);
    if (status != FBE_STATUS_OK) {
        fbe_database_util_trace_failing_object_entry(NULL, in_entry_ptr, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: failed to get class id of obj: 0x%x - status: 0x%x\n", 
                       __FUNCTION__, in_entry_ptr->header.object_id, status);
        return status;
    }
    if (class_id_of_object != expected_class_id) {
        fbe_database_util_trace_failing_object_entry(NULL, in_entry_ptr, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: actual class id: %d did not match expected: %d\n", 
                       __FUNCTION__, class_id_of_object, expected_class_id);
        return FBE_STATUS_SERVICE_MODE;
    }

    return FBE_STATUS_OK;
}
/********************************************************** 
 * end fbe_database_util_validate_object_entry_from_table()
 **********************************************************/

/*!****************************************************************************
 *          fbe_database_util_validate_edge_entry_from_table()
 ****************************************************************************** 
 * 
 * @brief   Validate the edge entry.
 * 
 * @param   in_entry_ptr - Entry data from in-memory table
 * @param   validate_table_p - Pointer to validation table
 * 
 * @return  status - status of the operation.
 *
 * @author
 *  07/03/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_database_util_validate_edge_entry_from_table(database_edge_entry_t *in_entry_ptr,
                                                                     database_validate_table_t *validate_table_p)
{
    fbe_status_t                status = FBE_STATUS_GENERIC_FAILURE;
    database_validate_entry_t  *validate_entry_p = NULL;
    fbe_bool_t                  b_is_entry_in_use = FBE_FALSE;
    fbe_class_id_t              client_class_id = FBE_CLASS_ID_INVALID;
    fbe_class_id_t              server_class_id = FBE_CLASS_ID_INVALID;
    fbe_class_id_t              expected_server_class_id = FBE_CLASS_ID_INVALID;
    fbe_bool_t                  b_server_class_id_valid = FBE_FALSE;

    /*! @note Since the table is indexed by object id, there can be `holes'
     *        or `blank' / unused entries.  Skip any entries that are not valid.
     */
    if (in_entry_ptr->header.state != DATABASE_CONFIG_ENTRY_VALID) {
        return FBE_STATUS_OK;
    }

    /* Special case is object is the first entry in the table or it is a 
     * special entry like the BVD interface that does not exist on-disk 
     * (entry id of: FBE_PERSIST_SECTOR_START_ENTRY_ID).  Skip these cases.
     */
    if (in_entry_ptr->header.entry_id == FBE_PERSIST_SECTOR_START_ENTRY_ID) {
        return FBE_STATUS_OK;
    }

    /* Get the validate entry from the object id.
     */
    status = fbe_database_util_get_validate_entry_from_object_id(validate_table_p,
                                                                 in_entry_ptr->header.object_id,
                                                                 in_entry_ptr->client_index,
                                                                 &validate_entry_p);
    if (status != FBE_STATUS_OK) {
        fbe_database_util_trace_failing_edge_entry(NULL, in_entry_ptr, NULL);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: failed to get validate entry - status: 0x%x\n", 
                       __FUNCTION__, status);
        return status;
    }

    /* Validate that the table entry is marked `in-use' in the bitmap.
     */
    status = fbe_database_util_get_persist_entry_info((database_table_header_t *)in_entry_ptr, 
                                                      &b_is_entry_in_use);
    if (status != FBE_STATUS_OK) {
        fbe_database_util_trace_failing_edge_entry(NULL, in_entry_ptr, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: failed to get persist entry info - status: 0x%x\n", 
                       __FUNCTION__, status);
        return status;
    }

    /* If the entry was not found on disk report the failure.
     */
    if (!validate_entry_p->b_entry_found) {
        fbe_database_util_trace_failing_edge_entry(NULL, in_entry_ptr, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: obj: 0x%x entry_id: 0x%llx in-use: %d was not on-disk\n", 
                       __FUNCTION__, in_entry_ptr->header.object_id,
                       in_entry_ptr->header.entry_id, b_is_entry_in_use);
        return FBE_STATUS_SERVICE_MODE;
    }

    /* If the entry is not marked `in-use' report the error.
     */
    if (!b_is_entry_in_use) {
        fbe_database_util_trace_failing_edge_entry(NULL, in_entry_ptr, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: obj: 0x%x entry_id: 0x%llx in-use: %d was not in-use\n", 
                       __FUNCTION__, in_entry_ptr->header.object_id,
                       in_entry_ptr->header.entry_id, b_is_entry_in_use);
        return FBE_STATUS_SERVICE_MODE;
    }

    /* Validate that the class id of the server is correct for the class id of
     * the client.
     */
    status = fbe_database_util_get_class_id_from_object_id(in_entry_ptr->header.object_id, &client_class_id);
    if (status != FBE_STATUS_OK) {
        fbe_database_util_trace_failing_edge_entry(NULL, in_entry_ptr, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: failed to get class id of obj: 0x%x - status: 0x%x\n", 
                       __FUNCTION__, in_entry_ptr->header.object_id, status);
        return status;
    }
    status = fbe_database_util_get_class_id_from_object_id(in_entry_ptr->header.object_id, &server_class_id);
    if (status != FBE_STATUS_OK) {
        fbe_database_util_trace_failing_edge_entry(NULL, in_entry_ptr, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: failed to get class id of obj: 0x%x - status: 0x%x\n", 
                       __FUNCTION__, in_entry_ptr->header.object_id, status);
        return status;
    }
    switch(client_class_id) {
        case FBE_CLASS_ID_BVD_INTERFACE:
            expected_server_class_id = FBE_CLASS_ID_LUN;
            if (server_class_id == expected_server_class_id) {
                b_server_class_id_valid = FBE_TRUE;
            }
            break;
        case FBE_CLASS_ID_LUN:
            /* The server id should be one of the following.
             */
            expected_server_class_id = FBE_CLASS_ID_RAID_GROUP;
            switch(server_class_id) {
                case FBE_CLASS_ID_RAID_GROUP:
                case FBE_CLASS_ID_MIRROR:
                case FBE_CLASS_ID_STRIPER:
                case FBE_CLASS_ID_PARITY:
                    b_server_class_id_valid = FBE_TRUE;
                    break;
                default:
                    break;
            }
            break;
        case FBE_CLASS_ID_RAID_GROUP:
        case FBE_CLASS_ID_MIRROR:
        case FBE_CLASS_ID_STRIPER:
        case FBE_CLASS_ID_PARITY:
            /*! @note This code does not process system raid groups therefore
             *        we know the server for a raid group should be a virtual
             *        drive.
             */
            expected_server_class_id = FBE_CLASS_ID_VIRTUAL_DRIVE;
            if (server_class_id == expected_server_class_id) {
                b_server_class_id_valid = FBE_TRUE;
            }
            break;
        case FBE_CLASS_ID_VIRTUAL_DRIVE:
            expected_server_class_id = FBE_CLASS_ID_PROVISION_DRIVE;
            if (server_class_id == expected_server_class_id) {
                b_server_class_id_valid = FBE_TRUE;
            }
            break;
        case FBE_CLASS_ID_PROVISION_DRIVE:
        default:
            /* Provision drive edges are discovered.*/
            fbe_database_util_trace_failing_edge_entry(NULL, in_entry_ptr, validate_entry_p);
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: unexpected client class id: %d server class id: %d \n", 
                       __FUNCTION__, client_class_id, server_class_id);
            return FBE_STATUS_SERVICE_MODE;
    }
    if (!b_server_class_id_valid) {
        fbe_database_util_trace_failing_edge_entry(NULL, in_entry_ptr, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: client class id: %d server class id: %d did not match expected: %d\n", 
                       __FUNCTION__, client_class_id, server_class_id, expected_server_class_id);
        return FBE_STATUS_SERVICE_MODE;
    }

    return FBE_STATUS_OK;
}
/********************************************************** 
 * end fbe_database_util_validate_edge_entry_from_table()
 **********************************************************/

/*!****************************************************************************
 *          fbe_database_util_validate_user_entry_from_table()
 ****************************************************************************** 
 * 
 * @brief   Validate the user entry.
 * 
 * @param   in_entry_ptr - Entry data from in-memory table
 * @param   validate_table_p - Pointer to validation table
 * 
 * @return  status - status of the operation.
 *
 * @author
 *  07/03/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_database_util_validate_user_entry_from_table(database_user_entry_t *in_entry_ptr,
                                                                     database_validate_table_t *validate_table_p)
{
    fbe_status_t                status = FBE_STATUS_GENERIC_FAILURE;
    database_validate_entry_t  *validate_entry_p = NULL;
    fbe_bool_t                  b_is_entry_in_use = FBE_FALSE;
    fbe_class_id_t              expected_class_id = FBE_CLASS_ID_INVALID;
    fbe_class_id_t              class_id_of_object = FBE_CLASS_ID_INVALID;

    /*! @note Since the table is indexed by object id, there can be `holes'
     *        or `blank' / unused entries.  Skip any entries that are not valid.
     */
    if (in_entry_ptr->header.state != DATABASE_CONFIG_ENTRY_VALID) {
        return FBE_STATUS_OK;
    }

    /* Special case is object is the first entry in the table or it is a 
     * special entry like the BVD interface that does not exist on-disk 
     * (entry id of: FBE_PERSIST_SECTOR_START_ENTRY_ID).  Skip these cases.
     */
    if (in_entry_ptr->header.entry_id == FBE_PERSIST_SECTOR_START_ENTRY_ID) {
        return FBE_STATUS_OK;
    }

    /* Get the validate entry from the object id.
     */
    expected_class_id = database_common_map_class_id_database_to_fbe(in_entry_ptr->db_class_id);
    status = fbe_database_util_get_validate_entry_from_object_id(validate_table_p,
                                                                 in_entry_ptr->header.object_id,
                                                                 FBE_EDGE_INDEX_INVALID,
                                                                 &validate_entry_p);
    if (status != FBE_STATUS_OK) {
        fbe_database_util_trace_failing_user_entry(NULL, in_entry_ptr, NULL);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: failed to get validate entry - status: 0x%x\n", 
                       __FUNCTION__, status);
        return status;
    }

    /* Validate that the table entry is marked `in-use' in the bitmap.
     */
    status = fbe_database_util_get_persist_entry_info((database_table_header_t *)in_entry_ptr, 
                                                      &b_is_entry_in_use);
    if (status != FBE_STATUS_OK) {
        fbe_database_util_trace_failing_user_entry(NULL, in_entry_ptr, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: failed to get persist entry info - status: 0x%x\n", 
                       __FUNCTION__, status);
        return status;
    }

    /* If the entry was not found on disk report the failure.
     */
    if (!validate_entry_p->b_entry_found) {
        fbe_database_util_trace_failing_user_entry(NULL, in_entry_ptr, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: obj: 0x%x entry_id: 0x%llx in-use: %d was not on-disk\n", 
                       __FUNCTION__, in_entry_ptr->header.object_id,
                       in_entry_ptr->header.entry_id, b_is_entry_in_use);
        return FBE_STATUS_SERVICE_MODE;
    }

    /* If the entry is not marked `in-use' report the error.
     */
    if (!b_is_entry_in_use) {
        fbe_database_util_trace_failing_user_entry(NULL, in_entry_ptr, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: obj: 0x%x entry_id: 0x%llx in-use: %d was not in-use\n", 
                       __FUNCTION__, in_entry_ptr->header.object_id,
                       in_entry_ptr->header.entry_id, b_is_entry_in_use);
        return FBE_STATUS_SERVICE_MODE;
    }

    /* Validate the class of the object.
     */
    status = fbe_database_util_get_class_id_from_object_id(in_entry_ptr->header.object_id, &class_id_of_object);
    if (status != FBE_STATUS_OK) {
        fbe_database_util_trace_failing_user_entry(NULL, in_entry_ptr, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: failed to get class id of obj: 0x%x - status: 0x%x\n", 
                       __FUNCTION__, in_entry_ptr->header.object_id, status);
        return status;
    }
    if (class_id_of_object != expected_class_id) {
        fbe_database_util_trace_failing_user_entry(NULL, in_entry_ptr, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: actual class id: %d did not match expected: %d\n", 
                       __FUNCTION__, class_id_of_object, expected_class_id);
        return FBE_STATUS_SERVICE_MODE;
    }

    return FBE_STATUS_OK;
}
/********************************************************** 
 * end fbe_database_util_validate_user_entry_from_table()
 **********************************************************/

/*!****************************************************************************
 *          fbe_database_util_validate_global_entry_from_table()
 ****************************************************************************** 
 * 
 * @brief   Validate the global info entry.
 * 
 * @param   in_entry_ptr - Entry data from in-memory table
 * @param   validate_table_p - Pointer to validation table
 * 
 * @return  status - status of the operation.
 *
 * @author
 *  07/03/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_database_util_validate_global_entry_from_table(database_global_info_entry_t *in_entry_ptr,
                                                                       database_validate_table_t *validate_table_p)
{
    fbe_status_t                status = FBE_STATUS_GENERIC_FAILURE;
    database_validate_entry_t  *validate_entry_p = NULL;
    fbe_bool_t                  b_is_entry_in_use = FBE_FALSE;

    /*! @note Since the table is indexed by object id, there can be `holes'
     *        or `blank' / unused entries.  Skip any entries that are not valid.
     */
    if (in_entry_ptr->header.state != DATABASE_CONFIG_ENTRY_VALID) {
        return FBE_STATUS_OK;
    }

    /* Special case is object is the first entry in the table or it is a 
     * special entry like the BVD interface that does not exist on-disk 
     * (entry id of: FBE_PERSIST_SECTOR_START_ENTRY_ID).  Skip these cases.
     */
    if (in_entry_ptr->header.entry_id == FBE_PERSIST_SECTOR_START_ENTRY_ID) {
        return FBE_STATUS_OK;
    }

    /* Get the validate entry from the type which is used to index
     * into the table.
     */
    status = fbe_database_util_get_validate_entry_from_object_id(validate_table_p,
                                                                 in_entry_ptr->type,
                                                                 FBE_EDGE_INDEX_INVALID,
                                                                 &validate_entry_p);
    if (status != FBE_STATUS_OK) {
        fbe_database_util_trace_failing_global_entry(NULL, in_entry_ptr, NULL);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: failed to get validate entry - status: 0x%x\n", 
                       __FUNCTION__, status);
        return status;
    }

    /* Validate that the table entry is marked `in-use' in the bitmap.
     */
    status = fbe_database_util_get_persist_entry_info((database_table_header_t *)in_entry_ptr, 
                                                      &b_is_entry_in_use);
    if (status != FBE_STATUS_OK) {
        fbe_database_util_trace_failing_global_entry(NULL, in_entry_ptr, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: failed to get persist entry info - status: 0x%x\n", 
                       __FUNCTION__, status);
        return status;
    }

    /* If the entry was not found on disk report the failure.
     */
    if (!validate_entry_p->b_entry_found) {
        fbe_database_util_trace_failing_global_entry(NULL, in_entry_ptr, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: obj: 0x%x entry_id: 0x%llx in-use: %d was not on-disk\n", 
                       __FUNCTION__, in_entry_ptr->header.object_id,
                       in_entry_ptr->header.entry_id, b_is_entry_in_use);
        return FBE_STATUS_SERVICE_MODE;
    }

    /* If the entry is not marked `in-use' report the error.
     */
    if (!b_is_entry_in_use) {
        fbe_database_util_trace_failing_global_entry(NULL, in_entry_ptr, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: obj: 0x%x entry_id: 0x%llx in-use: %d was not in-use\n", 
                       __FUNCTION__, in_entry_ptr->header.object_id,
                       in_entry_ptr->header.entry_id, b_is_entry_in_use);
        return FBE_STATUS_SERVICE_MODE;
    }

    return FBE_STATUS_OK;
}
/********************************************************** 
 * end fbe_database_util_validate_global_entry_from_table()
 **********************************************************/

/*!****************************************************************************
 *          fbe_database_util_validate_system_spare_entry_from_table()
 ****************************************************************************** 
 * 
 * @brief   Validate the system spare info.
 * 
 * @param   in_entry_ptr - Entry data from in-memory table
 * @param   validate_table_p - Pointer to validation table
 * 
 * @return  status - status of the operation.
 *
 * @author
 *  07/02/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_database_util_validate_system_spare_entry_from_table(database_system_spare_entry_t *in_entry_ptr,
                                                                             database_validate_table_t *validate_table_p)
{
    fbe_status_t                status = FBE_STATUS_GENERIC_FAILURE;
    database_validate_entry_t  *validate_entry_p = NULL;
    fbe_bool_t                  b_is_entry_in_use = FBE_FALSE;

    /*! @note Since the table is indexed by object id, there can be `holes'
     *        or `blank' / unused entries.  Skip any entries that are not valid.
     */
    if (in_entry_ptr->header.state != DATABASE_CONFIG_ENTRY_VALID) {
        return FBE_STATUS_OK;
    }

    /* Special case is object is the first entry in the table or it is a 
     * special entry like the BVD interface that does not exist on-disk 
     * (entry id of: FBE_PERSIST_SECTOR_START_ENTRY_ID).  Skip these cases.
     */
    if (in_entry_ptr->header.entry_id == FBE_PERSIST_SECTOR_START_ENTRY_ID) {
        return FBE_STATUS_OK;
    }

    /* Get the validate entry from the object id.
     */
    status = fbe_database_util_get_validate_entry_from_object_id(validate_table_p,
                                                                 in_entry_ptr->header.object_id,
                                                                 FBE_EDGE_INDEX_INVALID,
                                                                 &validate_entry_p);
    if (status != FBE_STATUS_OK) {
        fbe_database_util_trace_failing_system_spare_entry(NULL, in_entry_ptr, NULL);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: failed to get validate entry - status: 0x%x\n", 
                       __FUNCTION__, status);
        return status;
    }

    /* Validate that the table entry is marked `in-use' in the bitmap.
     */
    status = fbe_database_util_get_persist_entry_info((database_table_header_t *)in_entry_ptr, 
                                                      &b_is_entry_in_use);
    if (status != FBE_STATUS_OK) {
        fbe_database_util_trace_failing_system_spare_entry(NULL, in_entry_ptr, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: failed to get persist entry info - status: 0x%x\n", 
                       __FUNCTION__, status);
        return status;
    }

    /* If the entry was not found on disk report the failure.
     */
    if (!validate_entry_p->b_entry_found) {
        fbe_database_util_trace_failing_system_spare_entry(NULL, in_entry_ptr, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: obj: 0x%x entry_id: 0x%llx in-use: %d was not on-disk\n", 
                       __FUNCTION__, in_entry_ptr->header.object_id,
                       in_entry_ptr->header.entry_id, b_is_entry_in_use);
        return FBE_STATUS_SERVICE_MODE;
    }

    /* If the entry is not marked `in-use' report the error.
     */
    if (!b_is_entry_in_use) {
        fbe_database_util_trace_failing_system_spare_entry(NULL, in_entry_ptr, validate_entry_p);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: obj: 0x%x entry_id: 0x%llx in-use: %d was not in-use\n", 
                       __FUNCTION__, in_entry_ptr->header.object_id,
                       in_entry_ptr->header.entry_id, b_is_entry_in_use);
        return FBE_STATUS_SERVICE_MODE;
    }

    return FBE_STATUS_OK;
}
/**************************************************************** 
 * end fbe_database_util_validate_system_spare_entry_from_table()
 ****************************************************************/

/*!****************************************************************************
 *          fbe_database_util_validate_entries_from_table()
 ****************************************************************************** 
 * 
 * @brief   Validate that for each in-memory entry we located a corresponding
 *          on-disk entry.
 * 
 * @param   table_type - Database table type
 * @param   validate_table_p - Pointer to validation table
 * 
 * @return  status - status of the operation.
 *
 * @author
 *  07/02/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_database_util_validate_entries_from_table(database_config_table_type_t table_type,
                                                                  database_validate_table_t *validate_table_p)

{
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    database_table_header_t        *header_entry_ptr = NULL;
    database_table_t               *table_ptr = NULL;
    database_object_entry_t        *object_entry_ptr = NULL;
    database_edge_entry_t          *edge_entry_ptr = NULL;
    database_user_entry_t          *user_entry_ptr = NULL;
    database_global_info_entry_t   *global_info_entry_ptr = NULL;
    database_system_spare_entry_t  *system_spare_entry_ptr = NULL;
    fbe_u32_t                       entry_index;

    status = fbe_database_get_service_table_ptr(&table_ptr, table_type);
    if ((status != FBE_STATUS_OK)||(table_ptr == NULL)) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: get table type: %d failed - status: 0x%x\n", 
                       __FUNCTION__, table_type, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate that each in-memory entry was also located on-disk.
     */
    header_entry_ptr = (database_table_header_t *)table_ptr->table_content.object_entry_ptr;
    for (entry_index = 0; entry_index < table_ptr->table_size; entry_index++) {
        /* Although we could skip objects here we keep the logic simple by
         * letting each of the entry handlers skip in-memory only / special
         * objects.
         */
        switch(table_type) {
        case DATABASE_CONFIG_TABLE_OBJECT:
            object_entry_ptr = (database_object_entry_t *)header_entry_ptr;
            status = fbe_database_util_validate_object_entry_from_table(object_entry_ptr, validate_table_p);
            object_entry_ptr++;
            break;
        case DATABASE_CONFIG_TABLE_EDGE:
            edge_entry_ptr = (database_edge_entry_t *)header_entry_ptr;
            status = fbe_database_util_validate_edge_entry_from_table(edge_entry_ptr, validate_table_p);
            edge_entry_ptr++;
            break;
        case DATABASE_CONFIG_TABLE_USER:
            user_entry_ptr = (database_user_entry_t *)header_entry_ptr;
            status = fbe_database_util_validate_user_entry_from_table(user_entry_ptr, validate_table_p);
            user_entry_ptr++;
            break;
        case DATABASE_CONFIG_TABLE_GLOBAL_INFO:
            global_info_entry_ptr = (database_global_info_entry_t *)header_entry_ptr;
            status = fbe_database_util_validate_global_entry_from_table(global_info_entry_ptr, validate_table_p);
            global_info_entry_ptr++;
            break;
        case DATABASE_CONFIG_TABLE_SYSTEM_SPARE:
            system_spare_entry_ptr = (database_system_spare_entry_t *)header_entry_ptr;
            status = fbe_database_util_validate_system_spare_entry_from_table(system_spare_entry_ptr, validate_table_p);
            system_spare_entry_ptr++;
            break;

        default:
            database_trace (FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: Table type:%d is not supported!\n",
                            __FUNCTION__,
                            table_type);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (status != FBE_STATUS_OK) {
            return status;
        }
        /* We have already incremented the entry pointer.*/
    }

    return FBE_STATUS_OK;
}
/*******************************************************
 * end fbe_database_util_validate_entries_from_table()
 *******************************************************/

/***************************************************************************************************
    fbe_database_util_validate_table_from_persist()
*****************************************************************************************************
*
* @brief
*  This function reads from persist service and validates the data on disk and then compares the 
*  data to the in-memory value 
* 
* @param    table_type - Table type to validate 
*
* @return
*  fbe_status_t
*
*****************************************************************************************************/
static fbe_status_t fbe_database_util_validate_table_from_persist(database_config_table_type_t table_type)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_status_t                    wait_status = FBE_STATUS_OK;
    fbe_u8_t                       *read_buffer;
    database_init_read_context_t   *read_context;
    fbe_u32_t                       total_entries_read = 0;
    fbe_persist_sector_type_t       persist_sector_type;
    database_table_t               *table_ptr = NULL;
    database_validate_table_t       validate_table;

    /* Allocate our validation database for this table.
     */
    fbe_zero_memory(&validate_table, sizeof(database_validate_table_t));
    status = fbe_database_get_service_table_ptr(&table_ptr, table_type);
    if ((status != FBE_STATUS_OK)||(table_ptr == NULL)) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: get table type: %d failed - status: 0x%x\n", 
                       __FUNCTION__, table_type, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    validate_table.table_size = table_ptr->table_size;
    validate_table.validate_entry_p = (database_validate_entry_t *)fbe_memory_native_allocate(sizeof(database_validate_entry_t) * validate_table.table_size);
    if (validate_table.validate_entry_p == NULL) {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: alloc failed: %d bytes for type: %d size: 0x%x\n", 
                       __FUNCTION__, (int)(sizeof(database_validate_entry_t) * table_ptr->table_size),
                       table_type, table_ptr->table_size);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_zero_memory(validate_table.validate_entry_p, (sizeof(database_validate_entry_t) * validate_table.table_size));

    /* Allocate the read buffer.
     */
    read_context = (database_init_read_context_t *)fbe_memory_native_allocate(sizeof(database_init_read_context_t));
    if (read_context == NULL) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Can't allocate read_context\n",
            __FUNCTION__);
        fbe_memory_native_release(validate_table.validate_entry_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    read_buffer = (fbe_u8_t *)fbe_memory_ex_allocate(FBE_PERSIST_USER_DATA_READ_BUFFER_SIZE);
    if (read_buffer == NULL) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Can't allocate read_buffer\n",
            __FUNCTION__);
        fbe_memory_native_release(validate_table.validate_entry_p);
        fbe_memory_native_release(read_context);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    database_trace (FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: Load Table 0x%x\n",
                    __FUNCTION__, table_type);

    /* look up the sector type */
    fbe_semaphore_init(&read_context->sem, 0, 1);
    persist_sector_type = database_common_map_table_type_to_persist_sector(table_type);
    if (persist_sector_type != FBE_PERSIST_SECTOR_TYPE_INVALID) {
        /*start to read from the first one*/
        read_context->next_entry_id = FBE_PERSIST_SECTOR_START_ENTRY_ID;
        do {
            status = fbe_database_persist_read_sector(persist_sector_type,
                read_buffer,
                FBE_PERSIST_USER_DATA_READ_BUFFER_SIZE,
                read_context->next_entry_id,
                fbe_database_util_load_from_persist_read_completion_function,
                (fbe_persist_completion_context_t)read_context);
            if (status == FBE_STATUS_GENERIC_FAILURE)
            {
                /* log the error */
                database_trace (FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "Read from sector: %d failed! next_entry 0x%llX\n",
                                persist_sector_type,
                                (unsigned long long)read_context->next_entry_id );
                fbe_semaphore_destroy(&read_context->sem);
                fbe_memory_native_release(validate_table.validate_entry_p);
                fbe_memory_native_release(read_context);
                fbe_memory_ex_release(read_buffer);
                return status;
            }

            wait_status = fbe_semaphore_wait_ms(&read_context->sem, 30000);
            if(wait_status == FBE_STATUS_TIMEOUT)
            {
                /* log the error */
                database_trace (FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "Read from sector: %d TIMEOUT! next_entry 0x%llX\n",
                                persist_sector_type,
                                (unsigned long long)read_context->next_entry_id );
                fbe_memory_native_release(validate_table.validate_entry_p);
                fbe_memory_native_release(read_context);
                fbe_memory_ex_release(read_buffer);
                return status;
            }
            if (read_context->status == FBE_STATUS_IO_FAILED_RETRYABLE) 
            {
                database_trace (FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "Read from sector: %d Retryable error next_entry 0x%llX\n",
                                persist_sector_type,
                                (unsigned long long)read_context->next_entry_id );
                fbe_semaphore_destroy(&read_context->sem);
                fbe_memory_native_release(validate_table.validate_entry_p);
                fbe_memory_native_release(read_context);
                fbe_memory_ex_release(read_buffer);
                return FBE_STATUS_IO_FAILED_RETRYABLE;
            }
            if (read_context->next_entry_id != FBE_PERSIST_NO_MORE_ENTRIES_TO_READ) {
                database_trace (FBE_TRACE_LEVEL_DEBUG_LOW,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "Read %d entries from sector: %d up to entry 0x%llX\n",
                                read_context->entries_read,
                                persist_sector_type,
                                (unsigned long long)(read_context->next_entry_id-1));
            }else{
                database_trace (FBE_TRACE_LEVEL_DEBUG_LOW,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "Read %d entries from sector: %d up to sector end\n",
                                read_context->entries_read,
                                persist_sector_type);
            }

            /*now let's copy the read buffer content to the service table */
            status = fbe_database_util_validate_entries_from_persist(read_buffer, read_context->entries_read, 
                                                                     table_type, &validate_table);
            if (status != FBE_STATUS_OK) {
                /* Error happened when loading the database, quit and hault the system in this state*/
                fbe_semaphore_destroy(&read_context->sem);
                fbe_memory_native_release(validate_table.validate_entry_p);
                fbe_memory_native_release(read_context);
                fbe_memory_ex_release(read_buffer);
                return status;
            }

            /*increment only AFTER copying*/
            total_entries_read+= read_context->entries_read;
        } while (read_context->next_entry_id != FBE_PERSIST_NO_MORE_ENTRIES_TO_READ);
    }

    /* Now for all in-memory entries, make sure there was a corresponding disk-entry.
     */
    status = fbe_database_util_validate_entries_from_table(table_type, &validate_table);

    /* Cleanup */
    fbe_semaphore_destroy(&read_context->sem);
    fbe_memory_native_release(validate_table.validate_entry_p);
    fbe_memory_native_release(read_context);
    fbe_memory_ex_release(read_buffer);

    return status;
}
/***************************************************** 
 * end fbe_database_util_validate_table_from_persist()
 *****************************************************/

/*!****************************************************************************
 *          fbe_database_validate_database()
 ****************************************************************************** 
 * 
 * @brief   Read the on-disk database and compare it to the in-memory database.
 *          If there are any descrepencies trace an error and report the failure.
 * 
 * @param   database_p - Pointer to database service
 * @param   validate_caller - Caller (reason) for validation
 * @param   failure_action - What action to take if teh validation fails
 * @param   validation_status_p - Address of validation status to update
 * 
 * @return  status - status of the operation.
 *
 * @author
 *  06/27/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_database_validate_database(fbe_database_service_t *database_p,
                                                   fbe_database_validate_request_type_t validate_caller,  
                                                   fbe_database_validate_failure_action_t failure_action, 
                                                   fbe_status_t *validation_status_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    database_config_table_type_t    table_type;

    /* By default the validation request succeeded.
     */
    *validation_status_p = FBE_STATUS_OK;
    database_trace (FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: start - caller: %d action: %d debug flags: 0x%x sep version: 0x%llx\n",
                    __FUNCTION__, validate_caller, failure_action,
                    database_p->db_debug_flags,
                    database_p->system_db_header.persisted_sep_version);

    /* For table that needs to be validated, validate it.
     */
    for(table_type = DATABASE_CONFIG_TABLE_INVALID+1; table_type < DATABASE_CONFIG_TABLE_LAST; table_type ++) {
        status = fbe_database_util_validate_table_from_persist(table_type);
        if (status != FBE_STATUS_OK) {
            database_trace (FBE_TRACE_LEVEL_WARNING,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: validation of table: %d failed - status: 0x%x\n",
                            __FUNCTION__, table_type, status);

            /* Populate the validation status and change the return status as required.
             */
            *validation_status_p = status;
            if (status == FBE_STATUS_SERVICE_MODE) {
                status = FBE_STATUS_OK;
            }
            break;
        }
    }

    database_trace (FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: complete - status: 0x%x validation status: 0x%x\n",
                    __FUNCTION__, status, *validation_status_p);

    return status;
}
/***************************************
 * end fbe_database_validate_database()
 ***************************************/

/*!****************************************************************************
 *          fbe_database_is_validation_allowed()
 ****************************************************************************** 
 * 
 * @brief   If enable for the request type and the SP and database are healthy.
 * 
 * @param   validate_caller - The caller.  Determines if we perform validation not
 * @param   failure_action - What action to take when there is a validation failure
 * @param   validation_status_p - Address of validation status to update
 * 
 * @return  bool
 *
 * @author
 *  07/03/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_bool_t fbe_database_is_validation_allowed(fbe_database_validate_request_type_t validate_caller,
                                              fbe_database_validate_not_allowed_reason_t *not_allowed_reason_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_bool_t                  b_validation_allowed = FBE_FALSE;
    fbe_database_service_t     *database_p = fbe_database_get_database_service();
    fbe_cmi_sp_state_t          sp_state;
    fbe_database_debug_flags_t  debug_flags;
    fbe_sector_trace_is_external_build_t  b_is_external_build;

    /* By default the validation is not allowed.
     */
    *not_allowed_reason_p = FBE_DATABASE_VALIDATE_NOT_ALLOWED_REASON_NOT_ENABLED;

    /* Validate the SP state.
     */
    status = fbe_cmi_get_current_sp_state(&sp_state);
    if (status != FBE_STATUS_OK) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s: failed to get CMI state - status: 0x%x\n",
                        __FUNCTION__, status);
        *not_allowed_reason_p = FBE_DATABASE_VALIDATE_NOT_ALLOWED_REASON_UNEXPECTED_ERROR;
        return FBE_FALSE;
    }
    if ((sp_state != FBE_CMI_STATE_ACTIVE)  &&
        (sp_state != FBE_CMI_STATE_PASSIVE)    ) {
        *not_allowed_reason_p = FBE_DATABASE_VALIDATE_NOT_ALLOWED_REASON_DEGRADED;
        return FBE_FALSE;
    }

    /* Get the database state.
     */
    if (database_p->state != FBE_DATABASE_STATE_READY) {
        *not_allowed_reason_p = FBE_DATABASE_VALIDATE_NOT_ALLOWED_REASON_NOT_READY;
        return FBE_FALSE;
    }

    /* Validate that it is allowed for the build type.
     */
    b_is_external_build.b_is_external_build = fbe_sector_trace_is_external_build();
    /*! @todo REMOVE
	status = fbe_database_send_packet_to_service(FBE_SECTOR_TRACE_CONTROL_CODE_IS_EXTERNAL_BUILD,
                                                 &b_is_external_build, 
                                                 sizeof(fbe_sector_trace_is_external_build_t),
                                                 FBE_SERVICE_ID_SECTOR_TRACE,
                                                 NULL,
                                                 0,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s: failed to get build type - status: 0x%x\n",
                        __FUNCTION__, status);
        *not_allowed_reason_p = FBE_DATABASE_VALIDATE_NOT_ALLOWED_REASON_UNEXPECTED_ERROR;
        return FBE_FALSE;
    }
    */
    if (b_is_external_build.b_is_external_build) {
        *not_allowed_reason_p = FBE_DATABASE_VALIDATE_NOT_ALLOWED_REASON_EXTERNAL_BUILD;
        return FBE_FALSE;
    }

    /* By default the validation was not executed.
     */
    fbe_database_get_debug_flags(database_p, &debug_flags);

    /* Based on the debug flags and the request type validate the database
     * or not.
     */
    switch(validate_caller) {
        case FBE_DATABASE_VALIDATE_REQUEST_TYPE_USER:
            /* The user has requested the validation.  Do it.
             */
            b_validation_allowed = FBE_TRUE;
            break;

        case FBE_DATABASE_VALIDATE_REQUEST_TYPE_PEER_LOST:
            /* Only if enabled.
             */
            if (fbe_database_is_debug_flag_set(database_p, FBE_DATABASE_DEBUG_FLAG_VALIDATE_DATABASE_ON_PEER_LOSS)) {
                b_validation_allowed = FBE_TRUE;
            }
            break;

        case FBE_DATABASE_VALIDATE_REQUEST_TYPE_SP_COLLECT:
            /* Only if enabled.
             */
            if (fbe_database_is_debug_flag_set(database_p, FBE_DATABASE_DEBUG_FLAG_VALIDATE_DATABASE_ON_SP_COLLECT)) {
                b_validation_allowed = FBE_TRUE;
            }
            break;

        case FBE_DATABASE_VALIDATE_REQUEST_TYPE_DESTROY:
            /* Only if enabled.
             */
            if (fbe_database_is_debug_flag_set(database_p, FBE_DATABASE_DEBUG_FLAG_VALIDATE_DATABASE_ON_DESTROY)) {
                b_validation_allowed = FBE_TRUE;
            }
            break;

        default:
            /* All other callers we don't validate.
             */
            break;
    }

    /* If enabled issue the validation.
     */
    if (b_validation_allowed) {
        *not_allowed_reason_p = FBE_DATABASE_VALIDATE_NOT_ALLOWED_REASON_NOT_ENABLED;
    }

    return b_validation_allowed;
}
/*************************************************** 
 * end fbe_database_is_validation_allowed()
 **************************************************/

/*!****************************************************************************
 *          fbe_database_validate_database_if_enabled
 ****************************************************************************** 
 * 
 * @brief   If enable for the request type validate database.
 * 
 * @param   validate_caller - The caller.  Determines if we perform validation not
 * @param   failure_action - What action to take when there is a validation failure
 * @param   validation_status_p - Address of validation status to update
 * 
 * @return  status - status of the operation.
 *
 * @author
 *  06/27/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_database_validate_database_if_enabled(fbe_database_validate_request_type_t validate_caller,
                                                       fbe_database_validate_failure_action_t failure_action,
                                                       fbe_status_t *validation_status_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_bool_t                  b_issue_validate = FBE_FALSE;
    fbe_database_service_t     *database_p = fbe_database_get_database_service();
    fbe_database_debug_flags_t  debug_flags;

    /* By default the validation was not executed.
     */
    fbe_database_get_debug_flags(database_p, &debug_flags);
    *validation_status_p = FBE_STATUS_NO_ACTION;

    /* Validate that the action is supported.
     */
    switch (failure_action) {
        case FBE_DATABASE_VALIDATE_FAILURE_ACTION_ERROR_TRACE:
        case FBE_DATABASE_VALIDATE_FAILURE_ACTION_PANIC_SP:
            /* Currently on these actions are supported.
             */
            break;
        case FBE_DATABASE_VALIDATE_FAILURE_ACTION_NONE:
        case FBE_DATABASE_VALIDATE_FAILURE_ACTION_CORRECT_ENTRY:
        default:
            database_trace (FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: caller: %d failure action: %d not supported.\n",
                            __FUNCTION__, validate_caller, failure_action);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Based on the debug flags and the request type validate the database
     * or not.
     */
    switch(validate_caller) {
        case FBE_DATABASE_VALIDATE_REQUEST_TYPE_USER:
            /* The user has requested the validation.  Do it.
             */
            b_issue_validate = FBE_TRUE;
            break;

        case FBE_DATABASE_VALIDATE_REQUEST_TYPE_PEER_LOST:
            /* Only if enabled.
             */
            if (fbe_database_is_debug_flag_set(database_p, FBE_DATABASE_DEBUG_FLAG_VALIDATE_DATABASE_ON_PEER_LOSS)) {
                b_issue_validate = FBE_TRUE;
            }
            break;

        case FBE_DATABASE_VALIDATE_REQUEST_TYPE_SP_COLLECT:
            /* Only if enabled.
             */
            if (fbe_database_is_debug_flag_set(database_p, FBE_DATABASE_DEBUG_FLAG_VALIDATE_DATABASE_ON_SP_COLLECT)) {
                b_issue_validate = FBE_TRUE;
            }
            break;

        case FBE_DATABASE_VALIDATE_REQUEST_TYPE_DESTROY:
            /* Only if enabled.
             */
            if (fbe_database_is_debug_flag_set(database_p, FBE_DATABASE_DEBUG_FLAG_VALIDATE_DATABASE_ON_DESTROY)) {
                b_issue_validate = FBE_TRUE;
            }
            break;

        default:
            /* All other callers we don't validate.
             */
            break;
    }

    /* If enabled issue the validation.
     */
    if (b_issue_validate) {
        status =  fbe_database_validate_database(database_p,
                                                 validate_caller,
                                                 failure_action,
                                                 validation_status_p);
    } else {
        database_trace(FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: caller: %d failure action: %d debug flags: 0x%x not allowed\n",
                       __FUNCTION__, validate_caller, failure_action, debug_flags);
    }

    return status;
}
/*************************************************** 
 * end fbe_database_validate_database_if_enabled()
 **************************************************/

/*!****************************************************************************
 *          fbe_database_start_validate_database_job()
 ****************************************************************************** 
 * 
 * @brief   Start the `validate database' job.
 * 
 * @param   validate_caller - The caller.  Determines if we perform validation not
 * @param   failure_action - What action to take when there is a validation failure
 * 
 * @return  status - status of the operation.
 *
 * @author
 *  06/27/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_database_start_validate_database_job(fbe_database_validate_request_type_t validate_caller,
                                                      fbe_database_validate_failure_action_t failure_action)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_job_service_validate_database_t validate_database_job;

    /* Populate the request.
     */
    fbe_zero_memory(&validate_database_job, sizeof(fbe_job_service_validate_database_t));
    validate_database_job.validate_caller = validate_caller;
    validate_database_job.failure_action = failure_action;

    /* Trace the validate database request.
     */
    database_trace(FBE_TRACE_LEVEL_INFO, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "start validate database job - caller: %d failure action: %d\n", 
                   validate_caller, failure_action);

    /* Start the job.
     */                                 
    status = fbe_database_send_packet_to_service(FBE_JOB_CONTROL_CODE_VALIDATE_DATABASE,
                                                 &validate_database_job,
                                                 sizeof(fbe_job_service_validate_database_t),
                                                 FBE_SERVICE_ID_JOB_SERVICE,
                                                 NULL,  /* no sg list*/
                                                 0,  /* no sg list*/
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_WARNING,
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                       "%s: caller: %d failure action: %d - failed status: 0x%x\n", 
                       __FUNCTION__, validate_caller, failure_action, status);
    }

    return status;
}
/************************************************
 * end fbe_database_start_validate_database_job()
 ************************************************/

/******************************* 
 * end file fbe_database_util.c
 *******************************/
