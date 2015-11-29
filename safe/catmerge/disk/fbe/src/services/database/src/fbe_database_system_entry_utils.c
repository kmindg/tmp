#include "fbe_database_private.h"
#include "fbe_database_system_db_interface.h"
#include "fbe_private_space_layout.h"
#include "fbe_database_system_db_persist_transaction.h"
#include "fbe_database_config_tables.h"
#include "fbe_raid_library.h"


static fbe_status_t database_system_entries_verify(database_config_table_type_t table_type);

static fbe_status_t database_system_db_transaction_persist_entry(fbe_database_system_db_persist_handle_t handle,
                                                     fbe_database_system_db_persist_type_t type,  fbe_lba_t lba, 
                                                     fbe_u8_t *entry, fbe_u32_t size);
static fbe_status_t database_system_db_raw_persist_entry(fbe_database_system_db_persist_type_t type,  fbe_lba_t lba, 
                                                     fbe_u8_t *entry, fbe_u32_t size);

extern fbe_database_service_t fbe_database_service;

/* read system db header from the disk */
fbe_status_t fbe_database_system_db_read_header(database_system_db_header_t *header, fbe_u32_t read_drive_mask)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u64_t read_bytes = 0;
    fbe_lba_t lba;

    if (header == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = database_system_db_get_lba_by_region(FBE_DATABASE_SYSTEM_DB_LAYOUT_HEADER, &lba);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    status = database_system_db_interface_read((fbe_u8_t*)header, lba, sizeof(database_system_db_header_t), 
                                               read_drive_mask, &read_bytes);
    if (status == FBE_STATUS_OK && read_bytes == sizeof (database_system_db_header_t)) {
        return FBE_STATUS_OK;
    } else
        return FBE_STATUS_GENERIC_FAILURE;
}

/*read an object entry from the disk*/
fbe_status_t fbe_database_system_db_read_object_entry(fbe_object_id_t object_id, database_object_entry_t *entry)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u64_t read_bytes = 0;
    fbe_lba_t lba;

    if (entry == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    } else
        fbe_zero_memory(entry, sizeof (database_object_entry_t));

    /*get the disk LBA address and send IO to read it*/
    /*we only read the data part of database_object_entry_t from the disk and 
      zero its queue element*/
    status = database_system_db_get_entry_lba(FBE_DATABASE_SYSTEM_DB_OBJECT_ENTRY, object_id, 0, &lba);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    status = database_system_db_interface_read((fbe_u8_t *)entry, lba, sizeof (database_object_entry_t), 
                                               DATABASE_SYSTEM_RAW_MIRROR_ALL_DB_DRIVE_MASK, &read_bytes);
    if (status == FBE_STATUS_OK && read_bytes == (sizeof (database_object_entry_t))) {
        return FBE_STATUS_OK;
    } else
        return FBE_STATUS_GENERIC_FAILURE;
}

/*read a user entry from the disk*/
fbe_status_t fbe_database_system_db_read_user_entry(fbe_object_id_t object_id, database_user_entry_t *entry)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u64_t read_bytes = 0;
    fbe_lba_t lba;

    if (entry == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /*get the disk LBA address and send IO to read it*/
    status = database_system_db_get_entry_lba(FBE_DATABASE_SYSTEM_DB_USER_ENTRY, object_id, 0, &lba);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: get LBA for object %x failed \n", __FUNCTION__, object_id);
        return status;
    }

    status = database_system_db_interface_read((fbe_u8_t *)entry, lba, sizeof (database_user_entry_t), 
                                               DATABASE_SYSTEM_RAW_MIRROR_ALL_DB_DRIVE_MASK, &read_bytes);
    if (status == FBE_STATUS_OK && read_bytes == sizeof (database_user_entry_t)) {
        return FBE_STATUS_OK;
    }else
        return FBE_STATUS_GENERIC_FAILURE;
}

/*read an edge entry from the disk*/
fbe_status_t fbe_database_system_db_read_edge_entry(fbe_object_id_t object_id, fbe_edge_index_t client_index,
                                                   database_edge_entry_t *entry)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u64_t read_bytes = 0;
    fbe_lba_t lba;

    if (entry == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /*get the disk LBA address and send IO to read it*/
    status = database_system_db_get_entry_lba(FBE_DATABASE_SYSTEM_DB_EDGE_ENTRY, object_id, client_index, &lba);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    status = database_system_db_interface_read((fbe_u8_t *)entry, lba, sizeof (database_edge_entry_t),
                                               DATABASE_SYSTEM_RAW_MIRROR_ALL_DB_DRIVE_MASK, &read_bytes);
    if (status == FBE_STATUS_OK && read_bytes == sizeof (database_edge_entry_t)) {
        return FBE_STATUS_OK;
    } else
        return FBE_STATUS_GENERIC_FAILURE;
}

/*get the LBA which an system database entry will be wrote into*/
fbe_status_t database_system_db_get_entry_lba(fbe_db_system_entry_type_t type, fbe_object_id_t object_id, 
                                              fbe_edge_index_t index, fbe_lba_t *lba)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_lba_t region_start_lba;
    fbe_u32_t system_object_count = 0;

    if (lba == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*get the disk LBA address of the region in raw-3way-mirror, which holds system db entries*/
    status = database_system_db_get_lba_by_region(FBE_DATABASE_SYSTEM_DB_LAYOUT_ENTRY, &region_start_lba);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    /*We assume that the system object count keep fixed. If someone changed the 
      total system object count, we must re-initialize the raw-3way-mirror to distribute
      system entries according to the new layout*/
    database_system_objects_get_reserved_count(&system_object_count);

    if (object_id >= system_object_count || index >= DATABASE_MAX_EDGE_PER_OBJECT) {
        /*we only persist system objects and edges*/
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                       "database_system_db_get_entry_lba: failed to get LBA for entry, obj id: %x, index: %d\n", object_id, index);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*currently we assume each entry occupy a single block*/
    switch(type) {
    case FBE_DATABASE_SYSTEM_DB_OBJECT_ENTRY:
        *lba = region_start_lba + object_id * DATABASE_BLOCK_NUM_PER_SYSTEM_DB_ENTRY;
        break;
    case FBE_DATABASE_SYSTEM_DB_USER_ENTRY:
        *lba = region_start_lba + system_object_count * DATABASE_BLOCK_NUM_PER_SYSTEM_DB_ENTRY + object_id * DATABASE_BLOCK_NUM_PER_SYSTEM_DB_ENTRY;
        break;
    case FBE_DATABASE_SYSTEM_DB_EDGE_ENTRY:
        *lba = region_start_lba + system_object_count * 2 * DATABASE_BLOCK_NUM_PER_SYSTEM_DB_ENTRY + (object_id * DATABASE_MAX_EDGE_PER_OBJECT + index) * DATABASE_BLOCK_NUM_PER_SYSTEM_DB_ENTRY;
        break;
    default:
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

/* persist system db header */
fbe_status_t fbe_database_system_db_persist_header(database_system_db_header_t *header)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u64_t write_bytes = 0;
    fbe_lba_t lba;

    if (header == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = database_system_db_get_lba_by_region(FBE_DATABASE_SYSTEM_DB_LAYOUT_HEADER, &lba);
    if (status != FBE_STATUS_OK) {
        return status;
    }
    status = database_system_db_interface_write((fbe_u8_t*)header, lba, sizeof(database_system_db_header_t), &write_bytes);
    if (status == FBE_STATUS_OK && write_bytes == sizeof (database_system_db_header_t)) {
        return FBE_STATUS_OK;
    } else
        return FBE_STATUS_GENERIC_FAILURE;
}

/*persist an object entry inside a persist transaction*/
fbe_status_t fbe_database_system_db_persist_object_entry(fbe_database_system_db_persist_handle_t handle,
                                                         fbe_database_system_db_persist_type_t type, database_object_entry_t *entry)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_lba_t lba;
    fbe_bool_t	active_side = database_common_cmi_is_active();

    /*only persist in the active side*/
	if (!active_side) {
		return FBE_STATUS_OK;
	}

    if (entry == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*get the lba of the entry we want to write to the disk*/
    status = database_system_db_get_entry_lba(FBE_DATABASE_SYSTEM_DB_OBJECT_ENTRY, entry->header.object_id, 0, &lba);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    /*only persist data part of database_object_entry_t to the disk, the queue element isn't perisit because it is 
      the runtime state*/
    return database_system_db_transaction_persist_entry(handle, type, lba, (fbe_u8_t *)entry, sizeof (database_object_entry_t));

}

/*persist a user entry inside a persist transaction*/
fbe_status_t fbe_database_system_db_persist_user_entry(fbe_database_system_db_persist_handle_t handle,
                                                       fbe_database_system_db_persist_type_t type, database_user_entry_t *entry)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_lba_t lba;
    fbe_bool_t	active_side = database_common_cmi_is_active();

    /*only persist in the active side*/
	if (!active_side) {
		return FBE_STATUS_OK;
	}

    if (entry == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*get the lba of the entry we want to write to the disk*/
    status = database_system_db_get_entry_lba(FBE_DATABASE_SYSTEM_DB_USER_ENTRY, entry->header.object_id, 0, &lba);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    return database_system_db_transaction_persist_entry(handle, type, lba, (fbe_u8_t *)entry, sizeof (database_user_entry_t));
}

/*persist an edge entry inside a persist transaction*/
fbe_status_t fbe_database_system_db_persist_edge_entry(fbe_database_system_db_persist_handle_t handle, 
                                                       fbe_database_system_db_persist_type_t type, database_edge_entry_t *entry)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_lba_t lba;
    fbe_bool_t	active_side = database_common_cmi_is_active();

    /*only persist in the active side*/
	if (!active_side) {
		return FBE_STATUS_OK;
	}

    if (entry == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*get the lba of the entry we want to write to the disk*/
    status = database_system_db_get_entry_lba(FBE_DATABASE_SYSTEM_DB_EDGE_ENTRY, entry->header.object_id, entry->client_index, &lba);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    return database_system_db_transaction_persist_entry(handle, type, lba, (fbe_u8_t *)entry, sizeof (database_edge_entry_t));

}


/*add the persisted data into a transaction table. the transaction is identified by handle. 
  the data will be wrote into the disk when committing the transaction*/
static fbe_status_t database_system_db_transaction_persist_entry(fbe_database_system_db_persist_handle_t handle,
                                                     fbe_database_system_db_persist_type_t type,  fbe_lba_t lba, 
                                                     fbe_u8_t *entry, fbe_u32_t size)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u8_t *zero_data = NULL;


    if (entry == NULL || size == 0) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*currently we assume each entry occupy a single block*/
    switch(type) {
    case FBE_DATABASE_SYSTEM_DB_PERSIST_CREATE:
    case FBE_DATABASE_SYSTEM_DB_PERSIST_UPDATE:
        status = fbe_database_system_db_add_persist_elem(handle, lba, entry, size);
        break;
    case FBE_DATABASE_SYSTEM_DB_PERSIST_DELETE:
        /*for delete, we write zero instead of the orginal entry*/
        zero_data = (fbe_u8_t *)fbe_memory_native_allocate(size);
        if (zero_data == NULL) {
            return FBE_STATUS_GENERIC_FAILURE;
        } else
            fbe_zero_memory(zero_data, size);
        status = fbe_database_system_db_add_persist_elem(handle, lba, zero_data, size);
        if (zero_data != NULL) {
            fbe_memory_native_release(zero_data);
        }
        break;
    default:
        status = FBE_STATUS_GENERIC_FAILURE;
        break;
    }

    return status;
}

/*persist object entry to disk directly without transaction and journal support*/
fbe_status_t fbe_database_system_db_raw_persist_object_entry(fbe_database_system_db_persist_type_t type, database_object_entry_t *entry)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_lba_t lba;

    if (entry == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*get the lba of the entry we want to write to the disk*/
    status = database_system_db_get_entry_lba(FBE_DATABASE_SYSTEM_DB_OBJECT_ENTRY, entry->header.object_id, 0, &lba);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    return database_system_db_raw_persist_entry(type, lba, (fbe_u8_t *)entry, sizeof (database_object_entry_t));

}

/*persist user entry to disk directly without transaction and journal support*/
fbe_status_t fbe_database_system_db_raw_persist_user_entry(fbe_database_system_db_persist_type_t type, database_user_entry_t *entry)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_lba_t lba;

    if (entry == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*get the lba of the entry we want to write to the disk*/
    status = database_system_db_get_entry_lba(FBE_DATABASE_SYSTEM_DB_USER_ENTRY, entry->header.object_id, 0, &lba);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    return database_system_db_raw_persist_entry(type, lba, (fbe_u8_t *)entry, sizeof (database_user_entry_t));
}

/*persist edge entry to disk directly without transaction and journal support*/
fbe_status_t fbe_database_system_db_raw_persist_edge_entry(fbe_database_system_db_persist_type_t type, database_edge_entry_t *entry)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_lba_t lba;

    if (entry == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*get the lba of the entry we want to write to the disk*/
    status = database_system_db_get_entry_lba(FBE_DATABASE_SYSTEM_DB_EDGE_ENTRY, entry->header.object_id, entry->client_index, &lba);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    return database_system_db_raw_persist_entry(type, lba, (fbe_u8_t *)entry, sizeof (database_edge_entry_t));

}

/*raw persist will write entry to disk directly without invloving transaction and journal*/
static fbe_status_t database_system_db_raw_persist_entry(fbe_database_system_db_persist_type_t type,  fbe_lba_t lba, 
                                                     fbe_u8_t *entry, fbe_u32_t size)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u64_t write_bytes = 0;
    fbe_u8_t *zero_data = NULL;


    if (entry == NULL || size == 0) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*currently we assume each entry occupy a single block*/
    switch(type) {
    case FBE_DATABASE_SYSTEM_DB_PERSIST_CREATE:
    case FBE_DATABASE_SYSTEM_DB_PERSIST_UPDATE:
        status = database_system_db_interface_write(entry, lba, size, &write_bytes);
        break;
    case FBE_DATABASE_SYSTEM_DB_PERSIST_DELETE:
        /*for delete, we write zero instead of the orginal entry*/
        zero_data = (fbe_u8_t *)fbe_memory_native_allocate(size);
        if (zero_data == NULL) {
            return FBE_STATUS_GENERIC_FAILURE;
        } else
            fbe_zero_memory(zero_data, size);
        status = database_system_db_interface_write(zero_data, lba, size, &write_bytes);
        if (zero_data != NULL) {
            fbe_memory_native_release(zero_data);
        }
        break;
    default:
        status = FBE_STATUS_GENERIC_FAILURE;
        break;
    }

    if (status == FBE_STATUS_OK && write_bytes == size) {
        return FBE_STATUS_OK;
    } else
        return FBE_STATUS_GENERIC_FAILURE;
}


/*This function is used to verify the system db persist logic, raw-3way-mirror and 
  raw-3way-mirror based reboot*/
fbe_status_t system_db_rw_verify(void)
{
    fbe_status_t status;
    fbe_object_id_t vd2_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VD_2;
    fbe_object_id_t spare_pvd2_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_2_SPARE;
    database_object_entry_t object_entry;
    database_edge_entry_t edge_entry;

    /* NOTE: we must do check the entries before other tests . So we put it in the beginning */
    /* test persistence of the system db entries in boot path */
    status =  database_system_entries_verify(DATABASE_CONFIG_TABLE_USER);
    if (status != FBE_STATUS_OK)
        return status;
    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "%s: check user entries OK\n", __FUNCTION__);
    status = database_system_entries_verify(DATABASE_CONFIG_TABLE_OBJECT);
    if (status != FBE_STATUS_OK)
        return status;
    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "%s: check object entries OK\n", __FUNCTION__);
    status = database_system_entries_verify(DATABASE_CONFIG_TABLE_EDGE);
    if (status != FBE_STATUS_OK)
        return status;
    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "%s: check edge entries OK\n", __FUNCTION__);

    fbe_zero_memory(&object_entry, sizeof (database_object_entry_t));
    /*verify read/write/delete object*/
    status = fbe_database_system_db_read_object_entry(spare_pvd2_id, &object_entry);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "system_db_rw_verify: read spare error %d \n", status);
        return status;
    }

    
    if (object_entry.header.object_id == spare_pvd2_id) {
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "system_db_rw_verify: spare used");
    } else {
        object_entry.header.state = DATABASE_CONFIG_ENTRY_VALID;
        object_entry.header.object_id = spare_pvd2_id;
        status = fbe_database_system_db_raw_persist_object_entry(FBE_DATABASE_SYSTEM_DB_PERSIST_UPDATE, &object_entry);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "system_db_rw_verify: persist error");
            return status;
        }

        fbe_zero_memory(&object_entry, sizeof (database_object_entry_t));
        status = fbe_database_system_db_read_object_entry(spare_pvd2_id, &object_entry);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "system_db_rw_verify: read error");
            return status;
        }
        if (object_entry.header.state != DATABASE_CONFIG_ENTRY_VALID || 
            object_entry.header.object_id != spare_pvd2_id) {
            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "system_db_rw_verify: write/read not consistent");
            return FBE_STATUS_GENERIC_FAILURE;
        } else
            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "system_db_rw_verify: read/write object consistent");
    }
    
    /*now verify the entry delete*/
    status = fbe_database_system_db_raw_persist_object_entry(FBE_DATABASE_SYSTEM_DB_PERSIST_DELETE, &object_entry);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "system_db_rw_verify: delete error");
        return status;
    }

    fbe_zero_memory(&object_entry, sizeof (database_object_entry_t));

    status = fbe_database_system_db_read_object_entry(spare_pvd2_id, &object_entry);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "system_db_rw_verify: read spare error %d \n", status);
        return status;
    }

    if (object_entry.header.state == DATABASE_CONFIG_ENTRY_VALID || 
        object_entry.header.object_id == spare_pvd2_id) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "system_db_rw_verify: delete not persist \n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

 
    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "system_db_rw_verify: start verify edge entry");

    /*test edge entry persist commit*/
    edge_entry.header.object_id = vd2_id;
    edge_entry.header.state = DATABASE_CONFIG_ENTRY_VALID;
    edge_entry.client_index = 1;
    edge_entry.server_id = spare_pvd2_id;
    
    status = fbe_database_system_db_raw_persist_edge_entry(FBE_DATABASE_SYSTEM_DB_PERSIST_CREATE, &edge_entry);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "system_db_rw_verify: create edge error \n");
        return status;
    }

    fbe_zero_memory(&edge_entry, sizeof (database_edge_entry_t));
    status = fbe_database_system_db_read_edge_entry(vd2_id, 1, &edge_entry);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "system_db_rw_verify: read edge error \n");
        return status;
    }

    if (edge_entry.server_id != spare_pvd2_id) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "system_db_rw_verify: persist commit not consistent \n");
        return FBE_STATUS_GENERIC_FAILURE;
    }


    /*delete the entry we just created for test*/
    /*test edge entry persist commit*/
    edge_entry.header.object_id = vd2_id;
    edge_entry.header.state = DATABASE_CONFIG_ENTRY_VALID;
    edge_entry.client_index = 1;
    edge_entry.server_id = spare_pvd2_id;
    
    status = fbe_database_system_db_raw_persist_edge_entry(FBE_DATABASE_SYSTEM_DB_PERSIST_DELETE, &edge_entry);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "system_db_rw_verify: create edge error \n");
        return status;
    }

    fbe_zero_memory(&edge_entry, sizeof (database_edge_entry_t));
    status = fbe_database_system_db_read_edge_entry(vd2_id, 1, &edge_entry);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "system_db_rw_verify: read edge error \n");
        return status;
    }

    if (edge_entry.server_id != 0 || edge_entry.header.object_id != 0 ||
        edge_entry.header.state != DATABASE_CONFIG_ENTRY_INVALID) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "system_db_rw_verify: persist commit not consistent \n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

static fbe_bool_t database_system_compare_object_entry(database_object_entry_t * ptr1, database_object_entry_t *ptr2)
{
    if (ptr1 == NULL || ptr2 == NULL) {
        return FBE_FALSE;
    }

    if (ptr1->header.version_header.size != ptr2->header.version_header.size) {
            database_trace(FBE_TRACE_LEVEL_ERROR,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: version_header size in memory is not equal with size read from disk\n",
                           __FUNCTION__);
            database_trace(FBE_TRACE_LEVEL_ERROR,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: version_header size is %d, size read from disk is %d\n",
                           __FUNCTION__, ptr1->header.version_header.size, ptr2->header.version_header.size);
        return FBE_FALSE;
    }

    if (ptr1->header.state == DATABASE_CONFIG_ENTRY_VALID && ptr1->header.version_header.size == 0) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: version header size in memory is zero\n",
                       __FUNCTION__);
        return FBE_FALSE;
    }

    if ((ptr1->header.object_id == ptr2->header.object_id) &&
        (ptr1->db_class_id == ptr2->db_class_id)) {
        return FBE_TRUE;
    } else {
        return FBE_FALSE;
    }

}

static fbe_bool_t database_system_compare_user_entry(database_user_entry_t * ptr1, database_user_entry_t *ptr2)
{
    if (ptr1 == NULL || ptr2 == NULL) {
        return FBE_FALSE;
    }

    if (ptr1->header.version_header.size != ptr2->header.version_header.size) {
            database_trace(FBE_TRACE_LEVEL_ERROR,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: version_header size in memory is not equal with size read from disk\n",
                           __FUNCTION__);
            database_trace(FBE_TRACE_LEVEL_ERROR,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: version_header size is %d, size read from disk is %d\n",
                           __FUNCTION__, ptr1->header.version_header.size, ptr2->header.version_header.size);
        return FBE_FALSE;
    }

    if (ptr1->header.state == DATABASE_CONFIG_ENTRY_VALID && ptr1->header.version_header.size == 0) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: version header size in memory is zero\n",
                       __FUNCTION__);
        return FBE_FALSE;
    }

    if ((ptr1->header.object_id == ptr2->header.object_id) &&
        (ptr1->db_class_id == ptr2->db_class_id)) {
        return FBE_TRUE;
    } else {
        return FBE_FALSE;
    }

}

/* edge will be changed dynamically, so we don't check it */
static fbe_bool_t database_system_compare_edge_entry(database_edge_entry_t * ptr1, database_edge_entry_t *ptr2)
{
    if (ptr1 == NULL || ptr2 == NULL) {
        return FBE_FALSE;
    }

    if (ptr1->header.version_header.size != ptr2->header.version_header.size){
            database_trace(FBE_TRACE_LEVEL_ERROR,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: version_header size in memory is not equal with size read from disk\n",
                           __FUNCTION__);
            database_trace(FBE_TRACE_LEVEL_ERROR,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: version_header size is %d, size read from disk is %d\n",
                           __FUNCTION__, ptr1->header.version_header.size, ptr2->header.version_header.size);
        return FBE_FALSE;
    }

    if (ptr1->header.state == DATABASE_CONFIG_ENTRY_VALID && ptr1->header.version_header.size == 0) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: version header size in memory is zero\n",
                       __FUNCTION__);
        return FBE_FALSE;
    }

    if ((ptr1->header.object_id == ptr2->header.object_id) &&
        (ptr1->server_id == ptr2->server_id) &&
        (ptr1->client_index == ptr2->client_index)) {
        return FBE_TRUE;
    } else {
        return FBE_FALSE;
    }

    return FBE_TRUE;
}

static fbe_status_t
database_system_entries_verify(database_config_table_type_t table_type)
{
    database_table_t *  table_ptr = NULL;
    fbe_u32_t           objectid, index;
    fbe_status_t        status;
    database_object_entry_t *   object_entry_ptr = NULL;
    database_edge_entry_t *     edge_entry_ptr = NULL;
    database_user_entry_t *     user_entry_ptr = NULL;
    database_object_entry_t     object_entry;
    database_edge_entry_t       edge_entry;
    database_user_entry_t       user_entry;
    fbe_bool_t                  is_equal = FBE_FALSE;
    fbe_u32_t                   system_object_count = 0;

    status = fbe_database_get_service_table_ptr(&table_ptr, table_type);
    if ((status != FBE_STATUS_OK) || table_ptr == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "%s: fbe_database_get_service_table_ptr failed, status = 0x%x\n",
                      __FUNCTION__, status);
        return status;
    }

    database_system_objects_get_reserved_count(&system_object_count);
    for (objectid= 0; objectid < system_object_count; objectid++) {
        switch (table_type) {
        case DATABASE_CONFIG_TABLE_OBJECT:
                status = fbe_database_config_table_get_object_entry_by_id(table_ptr,
                                                                                 objectid,
                                                                                 &object_entry_ptr);
                if (status != FBE_STATUS_OK) {
                    return status;
                }
                status = fbe_database_system_db_read_object_entry(objectid, &object_entry);
                if (status != FBE_STATUS_OK) {
                    return status;
                }
                is_equal = database_system_compare_object_entry(object_entry_ptr, &object_entry);
                if (is_equal != FBE_TRUE) {
                    database_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: object number=0x%x, table_type = %d \n",
                            __FUNCTION__, objectid, table_type);
                    status = FBE_STATUS_GENERIC_FAILURE;
                }
                break;
        case DATABASE_CONFIG_TABLE_USER:
                status = fbe_database_config_table_get_user_entry_by_object_id(table_ptr,
                                                                               objectid,
                                                                               &user_entry_ptr);
                if (status != FBE_STATUS_OK) {
                    return status;
                }
                status = fbe_database_system_db_read_user_entry(objectid, &user_entry);
                if (status != FBE_STATUS_OK) {
                    return status;
                }
                is_equal = database_system_compare_user_entry(user_entry_ptr, &user_entry);
                if (is_equal != FBE_TRUE) {
                    database_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: object number=0x%x, table_type = %d \n",
                            __FUNCTION__, objectid, table_type);
                    status = FBE_STATUS_GENERIC_FAILURE;
                }
                break;
        case DATABASE_CONFIG_TABLE_EDGE:
                for (index = 0; index < DATABASE_MAX_EDGE_PER_OBJECT; index++) {
                    status = fbe_database_config_table_get_edge_entry(table_ptr,
                                                                      objectid,
                                                                      index,
                                                                      &edge_entry_ptr);
                    if (status != FBE_STATUS_OK) {
                        return status;
                    }
                    status = fbe_database_system_db_read_edge_entry(objectid, index, &edge_entry);
                    if (status != FBE_STATUS_OK) {
                        return status;
                    }
                    is_equal = database_system_compare_edge_entry(edge_entry_ptr, &edge_entry);
                    if (is_equal != FBE_TRUE) {
                        database_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: object number=0x%x, client_index = 0x%x, table_type = %d \n",
                                __FUNCTION__, objectid, index, table_type);
                        status = FBE_STATUS_GENERIC_FAILURE;
                        break;
                    }
                }
                break;

        default:
            database_trace(FBE_TRACE_LEVEL_WARNING,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: Table type:%d is not supported!\n",
                           __FUNCTION__,
                           table_type);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
        }
        if (is_equal != FBE_TRUE) {
            database_trace(FBE_TRACE_LEVEL_ERROR,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: \n",
                           __FUNCTION__);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
        }
        /* goto next entry */
    }
    return status;
}

/* read shared expected memory info from the disk */
fbe_status_t fbe_database_system_db_read_semv_info(fbe_database_emv_t* semv_info, fbe_u32_t read_drive_mask)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u64_t read_bytes = 0;
    fbe_lba_t lba;

    if (NULL == semv_info) 
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    status = database_system_db_get_lba_by_region(FBE_DATABASE_SYSTEM_DB_LAYOUT_SEMV_INFO, &lba);
    if (FBE_STATUS_OK != status) 
    {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get semv info lba\n",
                       __FUNCTION__);
        return status;
    }

    status = database_system_db_interface_read((fbe_u8_t*)semv_info, lba, sizeof(fbe_database_emv_t), 
                                               read_drive_mask, &read_bytes);
    
    if (status == FBE_STATUS_OK && read_bytes == sizeof (fbe_database_emv_t)) {
        return FBE_STATUS_OK;
    } 
    else
    {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to read semv info from raw-mirror region on db disks\n",
                       __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
}

/* persist shared expected memory info onto disks*/
fbe_status_t fbe_database_system_db_persist_semv_info(fbe_database_emv_t* semv_info)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u64_t write_bytes = 0;
    fbe_lba_t lba;

    if (NULL == semv_info) 
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    status = database_system_db_get_lba_by_region(FBE_DATABASE_SYSTEM_DB_LAYOUT_SEMV_INFO, &lba);
    if (FBE_STATUS_OK != status) 
    {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get semv info lba\n",
                       __FUNCTION__);
        return status;
    }
    
    status = database_system_db_interface_write((fbe_u8_t*)semv_info, lba, sizeof(fbe_database_emv_t), &write_bytes);
    if (status == FBE_STATUS_OK && write_bytes == sizeof (fbe_database_emv_t)) 
    {
        return FBE_STATUS_OK;
    } 
    else
    {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to persist semv info onto raw-mirror region on db disks\n",
                       __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
}

/* read the system object operation flags from disk */
fbe_status_t fbe_database_read_system_object_recreate_flags(fbe_database_system_object_recreate_flags_t *system_object_op_flags, fbe_u32_t read_drive_mask)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u64_t read_bytes = 0;
    fbe_lba_t lba;

    if (NULL == system_object_op_flags) 
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    status = database_system_db_get_lba_by_region(FBE_DATABASE_SYSTEM_DB_LAYOUT_SYSTEM_OBJECT_OP_FLAGS, &lba);
    if (FBE_STATUS_OK != status) 
    {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get system object operation flags lba\n",
                       __FUNCTION__);
        return status;
    }

    status = database_system_db_interface_read((fbe_u8_t*)system_object_op_flags, lba, sizeof(fbe_database_system_object_recreate_flags_t), 
                                               read_drive_mask, &read_bytes);
    
    if (status == FBE_STATUS_OK && read_bytes == sizeof (fbe_database_system_object_recreate_flags_t)) {
        return FBE_STATUS_OK;
    } 
    else
    {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to read system object operation flags from raw-mirror region on db disks\n",
                       __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

}

/* write the system object operation flags to disk */
fbe_status_t fbe_database_persist_system_object_recreate_flags(fbe_database_system_object_recreate_flags_t *system_object_op_flags)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u64_t write_bytes = 0;
    fbe_lba_t lba;

    if (NULL == system_object_op_flags) 
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    status = database_system_db_get_lba_by_region(FBE_DATABASE_SYSTEM_DB_LAYOUT_SYSTEM_OBJECT_OP_FLAGS, &lba);
    if (FBE_STATUS_OK != status) 
    {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get system object operation flags lba\n",
                       __FUNCTION__);
        return status;
    }
    
    status = database_system_db_interface_write((fbe_u8_t*)system_object_op_flags, lba, sizeof(fbe_database_system_object_recreate_flags_t), &write_bytes);
    if (status == FBE_STATUS_OK && write_bytes == sizeof (fbe_database_system_object_recreate_flags_t)) 
    {
        return FBE_STATUS_OK;
    } 
    else
    {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to persist system object operation flags onto raw-mirror region on db disks\n",
                       __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    

}

