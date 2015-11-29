#include "csx_ext.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_winddk.h"
#include "fbe_base_object.h"
#include "fbe_base_service.h"
#include "fbe/fbe_drive_configuration_interface.h"
#include "fbe_service_manager.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe_drive_configuration_service_private.h"
#include "fbe_drive_configuration_service_download_private.h"


/* GLOBALS */
fbe_drive_configuration_service_t    drive_configuration_service;
fbe_bool_t                           service_initialized = FBE_FALSE;


fbe_drive_configuration_port_record_t   default_port_record_table[] =
{
    {FBE_PORT_TYPE_FIBRE,
     1000, 2, 101, 0,/*IO stat*/
     1000, 2, 101, 0 /*enlcosure stat*/
     },

    {FBE_PORT_TYPE_SAS_LSI,
     1000, 2, 101, 0,/*IO stat*/
     1000, 2, 101, 0 /*enlcosure stat*/
     },

    {FBE_PORT_TYPE_SAS_PMC,
     1000, 2, 101, 0,/*IO stat*/
     1000, 2, 101, 0 /*enlcosure stat*/
     },

    {FBE_PORT_TYPE_INVALID, 0, 0, 0, 0, 0, 0, 0, 0}

};

/* Declare our service methods */
fbe_status_t fbe_drive_configuration_control_entry(fbe_packet_t * packet); 
fbe_service_methods_t fbe_drive_configuration_service_methods = {FBE_SERVICE_ID_DRIVE_CONFIGURATION, fbe_drive_configuration_control_entry};

/*local functions*/
static void fbe_drive_configuration_init_dieh(void);
static void fbe_drive_configuration_init_dieh_table(fbe_u32_t table);
static fbe_bool_t fbe_drive_configuration_sn_is_in_range(const fbe_u8_t *drive_sn, fbe_drive_configuration_drive_info_t *table_entry);
static fbe_bool_t fbe_drive_configuration_string_match(fbe_u8_t *table_entry_pid, fbe_u8_t *registration_info_pid, fbe_u32_t max_length);
static fbe_status_t fbe_drive_configuration_control_add_record(fbe_packet_t *packet);
static fbe_status_t fbe_drive_configuration_add_record(fbe_drive_configuration_record_t *new_record, fbe_drive_configuration_handle_t *handle);
static fbe_status_t fbe_drive_configuration_get_free_drive_table_entry(fbe_u32_t table_index, fbe_u32_t *entry_index);
static fbe_status_t fbe_drive_configuration_get_free_port_table_entry(fbe_u32_t *entry_index);
static fbe_status_t fbe_drive_configuration_get_handle_index(fbe_drive_configuration_registration_info_t *registration_info, fbe_drive_configuration_handle_t *handle);
static fbe_status_t fbe_drive_configuration_control_change_drive_thresholds(fbe_packet_t *packet);
static fbe_status_t fbe_drive_configuration_control_get_handles_list(fbe_packet_t *packet);
static fbe_status_t fbe_drive_configuration_control_get_port_handles_list(fbe_packet_t *packet);
static fbe_status_t fbe_drive_configuration_change_drive_threshold(fbe_drive_configuration_control_change_thresholds_t * change_threshold);
static fbe_status_t fbe_drive_configuration_control_get_drive_record(fbe_packet_t *packet);
static fbe_status_t fbe_drive_configuration_control_get_port_record(fbe_packet_t *packet);
static fbe_status_t fbe_drive_configuration_validate_drive_handle(fbe_drive_configuration_handle_t handle);
static fbe_status_t fbe_drive_configuration_validate_port_handle(fbe_drive_configuration_handle_t handle);
static fbe_status_t fbe_drive_configuration_start_table_update(fbe_packet_t *packet);
static fbe_status_t fbe_drive_configuration_end_table_update(fbe_packet_t *packet);
static fbe_status_t drive_configuration_send_update_to_all_drives(void);
static fbe_status_t drive_configuration_get_all_drive_objects_of_type(fbe_topology_mgmt_get_all_drive_of_type_t *get_type);
static fbe_status_t drive_configuration_get_all_objects_of_type_completion (fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_drive_configuration_send_port_table_change_usurper(fbe_object_id_t object_id);
static fbe_status_t fbe_drive_configuration_send_drive_table_change_usurper(fbe_object_id_t object_id);
static fbe_status_t fbe_drive_configuration_send_table_change_usurper_completion (fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_drive_configuration_get_port_hanlde(fbe_drive_configuration_port_info_t *registration_info, fbe_drive_configuration_handle_t *handle);
static fbe_status_t fbe_drive_configuration_validate_port_handle(fbe_drive_configuration_handle_t handle);
static fbe_status_t fbe_drive_configuration_control_add_port_record(fbe_packet_t *packet);
static fbe_status_t fbe_drive_configuration_control_change_port_threshold(fbe_packet_t *packet);
static fbe_status_t drive_configuration_send_update_to_all_ports(void);
static fbe_status_t fbe_drive_configuration_add_port_record(fbe_drive_configuration_port_record_t *new_record, fbe_drive_configuration_handle_t *handle);
static fbe_status_t fbe_drive_configuration_change_port_threshold(fbe_drive_configuration_control_change_port_threshold_t * change_threshold);
static fbe_status_t fbe_drive_configuration_control_dieh_force_clear_update(fbe_packet_t *packet);
static fbe_status_t fbe_drive_configuration_control_dieh_get_status(fbe_packet_t *packet);

static fbe_status_t fbe_drive_configuration_control_reset_queuing_table(fbe_packet_t *packet);
static fbe_status_t fbe_drive_configuration_control_add_queuing_table_entry(fbe_packet_t *packet);
static fbe_status_t fbe_drive_configuration_control_activate_queuing_table(fbe_packet_t *packet);
static void fbe_drive_configuration_init_queuing_table(void);
static fbe_status_t fbe_drive_configuration_add_queuing_table_entry(fbe_drive_configuration_queuing_record_t * add_entry);
static fbe_status_t fbe_drive_configuration_get_free_queuing_table_entry(fbe_u32_t * entry_index);
static fbe_status_t fbe_drive_configuration_activate_queuing_table(void);
static fbe_status_t fbe_drive_configuration_send_queuing_change_usurper(fbe_object_id_t object_id, fbe_packet_t *packet);


/********************************************************************************************************************************************************/



static fbe_status_t 
fbe_drive_configuration_init(fbe_packet_t * packet)
{
    
    drive_configuration_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s: entry\n", __FUNCTION__);

    fbe_base_service_set_service_id((fbe_base_service_t *) &drive_configuration_service, FBE_SERVICE_ID_DRIVE_CONFIGURATION);
    
    fbe_base_service_init((fbe_base_service_t *) &drive_configuration_service);
    fbe_spinlock_init(&drive_configuration_service.threshold_table_lock);

    /* Initialize DIEH. */
    drive_configuration_service.active_table_index = 0;
    fbe_drive_configuration_init_dieh();

    /* Initialize download. */
    fbe_drive_configuration_service_initialize_download();

    /* Initialize enhanced queuing timer table. */
    fbe_spinlock_init(&drive_configuration_service.queuing_table_lock);
    fbe_drive_configuration_init_queuing_table();

    /* Initialize tunable parameters */
    fbe_drive_configuration_init_paramters();


    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    service_initialized = FBE_TRUE;

    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_drive_configuration_destroy(fbe_packet_t * packet)
{
    fbe_u32_t   table_index, entry_index;

    drive_configuration_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s: entry\n", __FUNCTION__);

    /*verify all objects deregistered from the table*/
    fbe_spinlock_lock (&drive_configuration_service.threshold_table_lock);

    for (table_index = 0; table_index < DCS_THRESHOLD_TABLES; table_index++)
    {
        for (entry_index = 0; entry_index < MAX_THRESHOLD_TABLE_RECORDS; entry_index++) {
            if (drive_configuration_service.threshold_record_tables[table_index][entry_index].ref_count != 0){
                 drive_configuration_trace(FBE_TRACE_LEVEL_WARNING, 
                                           FBE_TRACE_MESSAGE_ID_INFO,
                                           "%s: tbl:%d idx:%d has ref count:%d, some PDO did not un-register\n", __FUNCTION__,
                                           table_index, entry_index, drive_configuration_service.threshold_record_tables[table_index][entry_index].ref_count);
    
            }
        }
    }
    fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);

    fbe_spinlock_destroy(&drive_configuration_service.threshold_table_lock);
    fbe_spinlock_destroy(&drive_configuration_service.queuing_table_lock);

    fbe_drive_configuration_service_destroy_download();
    
    fbe_base_service_destroy((fbe_base_service_t *) &drive_configuration_service);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_drive_configuration_control_entry(fbe_packet_t * packet)
{    
    fbe_status_t status;
    fbe_payload_control_operation_opcode_t control_code;

    control_code = fbe_transport_get_control_code(packet);
    if(control_code == FBE_BASE_SERVICE_CONTROL_CODE_INIT) {
        status = fbe_drive_configuration_init(packet);
        return status;
    }

    if(!fbe_base_service_is_initialized((fbe_base_service_t *) &drive_configuration_service)){
        fbe_transport_set_status(packet, FBE_STATUS_NOT_INITIALIZED, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_NOT_INITIALIZED;
    }

    switch(control_code) {
        case FBE_DRIVE_CONFIGURATION_CONTROL_ADD_RECORD:
            status = fbe_drive_configuration_control_add_record( packet);
            break;
        case FBE_DRIVE_CONFIGURATION_CONTROL_CHANGE_THRESHOLDS:
            status = fbe_drive_configuration_control_change_drive_thresholds( packet);
            break;
        case FBE_DRIVE_CONFIGURATION_CONTROL_GET_HANDLES_LIST:
            status = fbe_drive_configuration_control_get_handles_list( packet);
            break;
        case FBE_DRIVE_CONFIGURATION_CONTROL_GET_DRIVE_RECORD:
            status = fbe_drive_configuration_control_get_drive_record( packet);
            break;
        case FBE_BASE_SERVICE_CONTROL_CODE_DESTROY:
            status = fbe_drive_configuration_destroy( packet);
            break;
        case FBE_DRIVE_CONFIGURATION_CONTROL_START_TABLE_UPDATE:
            status = fbe_drive_configuration_start_table_update( packet);
            break;
        case FBE_DRIVE_CONFIGURATION_CONTROL_END_TABLE_UPDATE:
            status = fbe_drive_configuration_end_table_update( packet);
            break;
        case FBE_DRIVE_CONFIGURATION_CONTROL_ADD_PORT_RECORD:
            status = fbe_drive_configuration_control_add_port_record( packet);
            break;
        case FBE_DRIVE_CONFIGURATION_CONTROL_CHANGE_PORT_THRESHOLD:
            status = fbe_drive_configuration_control_change_port_threshold( packet);
            break;
        case FBE_DRIVE_CONFIGURATION_CONTROL_GET_PORT_HANDLES_LIST:
            status = fbe_drive_configuration_control_get_port_handles_list( packet);
            break;
        case FBE_DRIVE_CONFIGURATION_CONTROL_GET_PORT_RECORD:
            status = fbe_drive_configuration_control_get_port_record( packet);
            break;
        case FBE_DRIVE_CONFIGURATION_CONTROL_DOWNLOAD_FIRMWARE:
            status = fbe_drive_configuration_control_download_firmware( packet);
            break;
        case FBE_DRIVE_CONFIGURATION_CONTROL_ABORT_DOWNLOAD:
            status = fbe_drive_configuration_control_abort_download( packet);
            break;
        case FBE_DRIVE_CONFIGURATION_CONTROL_GET_DOWNLOAD_PROCESS:
            status = fbe_drive_configuration_control_get_download_process_info( packet);
            break;
        case FBE_DRIVE_CONFIGURATION_CONTROL_GET_DOWNLOAD_DRIVE:
            status = fbe_drive_configuration_control_get_download_drive_info( packet);
            break;
        case FBE_DRIVE_CONFIGURATION_CONTROL_GET_DOWNLOAD_MAX_DRIVE_COUNT:
            status = fbe_drive_configuration_control_get_max_dl_count( packet);
            break;
        case FBE_DRIVE_CONFIGURATION_CONTROL_GET_ALL_DOWNLOAD_DRIVES:
            status = fbe_drive_configuration_control_get_all_download_drives_info(packet);
            break;
        case FBE_DRIVE_CONFIGURATION_CONTROL_DIEH_FORCE_CLEAR_UPDATE:
            status = fbe_drive_configuration_control_dieh_force_clear_update(packet);
            break;
        case FBE_DRIVE_CONFIGURATION_CONTROL_DIEH_GET_STATUS:
            status = fbe_drive_configuration_control_dieh_get_status(packet);
            break;
        case FBE_DRIVE_CONFIGURATION_CONTROL_GET_TUNABLE_PARAMETERS:
            status = fbe_drive_configuration_control_get_parameters(packet);
            break;
        case FBE_DRIVE_CONFIGURATION_CONTROL_SET_TUNABLE_PARAMETERS:
            status = fbe_drive_configuration_control_set_parameters(packet);
            break;            
        case FBE_DRIVE_CONFIGURATION_CONTROL_GET_MODE_PAGE_OVERRIDES:
            status = fbe_drive_configuration_control_get_mode_page_overrides(packet);
            break;
        case FBE_DRIVE_CONFIGURATION_CONTROL_SET_MODE_PAGE_BYTE:
            status = fbe_drive_configuration_control_set_mode_page_byte(packet);
            break;
        case FBE_DRIVE_CONFIGURATION_CONTROL_MODE_PAGE_ADDL_OVERRIDE_CLEAR:
            status = fbe_drive_configuration_control_mp_addl_override_clear(packet);
            break;
        case FBE_DRIVE_CONFIGURATION_CONTROL_RESET_QUEUING_TABLE:
            status = fbe_drive_configuration_control_reset_queuing_table(packet);
            break;
        case FBE_DRIVE_CONFIGURATION_CONTROL_ADD_QUEUING_TABLE_ENTRY:
            status = fbe_drive_configuration_control_add_queuing_table_entry(packet);
            break;
        case FBE_DRIVE_CONFIGURATION_CONTROL_ACTIVATE_QUEUING_TABLE:
            status = fbe_drive_configuration_control_activate_queuing_table(packet);
            break;

        default:
            status = fbe_base_service_control_entry((fbe_base_service_t*)&drive_configuration_service, packet);
            break;
    }

    return status;
}

fbe_status_t fbe_drive_configuration_register_drive (fbe_drive_configuration_registration_info_t *registration_info,
                                                     fbe_drive_configuration_handle_t *handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t    active = 0;

    fbe_spinlock_lock (&drive_configuration_service.threshold_table_lock);

    /*to start with, we get a handle*/
    status = fbe_drive_configuration_get_handle_index(registration_info, handle);  /*todo: rename this.  misleading.  not dealing with a handle, it's the active table index*/

    if (status != FBE_STATUS_OK || *handle == FBE_DRIVE_CONFIGURATION_SERVICE_INVALID_HANLDLE) 
    {
        drive_configuration_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: unable to register. no match found\n", __FUNCTION__);
        fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    active = dcs_get_active_table_index();
    drive_configuration_service.threshold_record_tables[active][*handle].ref_count++;

    fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);

    /*encode table index in handle*/
    *handle |= (active << DCS_TABLE_INDEX_SHIFT);

    return FBE_STATUS_OK;

}

/* Note: Caller responsible for guarding threshold table */
static fbe_status_t fbe_drive_configuration_get_handle_index(fbe_drive_configuration_registration_info_t *registration_info,
                                                             fbe_drive_configuration_handle_t *handle)
{
    fbe_s32_t                               curren_handle = FBE_DRIVE_CONFIGURATION_SERVICE_INVALID_HANLDLE;
    fbe_s32_t                               table_counter = 0;
    fbe_drive_configuration_drive_info_t *  table_entry;
    fbe_u32_t                               max_matches = 0;
    fbe_u32_t                               current_matches = 0;
    fbe_bool_t                              value_mismatch = FBE_FALSE;
    fbe_u32_t                               active = 0;
    
    *handle = FBE_DRIVE_CONFIGURATION_SERVICE_INVALID_HANLDLE;

    if (!service_initialized){
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Service uninitialized\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    active = dcs_get_active_table_index();    

    /*let's find a match for the parameters that drive has
    the main concept is to do a match by this hirarchy: CLASS->Vendor->Product ID->REV->Serail number*/
    for (table_counter = 0; table_counter < MAX_THRESHOLD_TABLE_RECORDS; table_counter++) {
        table_entry = &drive_configuration_service.threshold_record_tables[active][table_counter].threshold_record.drive_info;
        current_matches = 0;/*we always start with 0*/
        value_mismatch = FBE_FALSE;

        if (table_entry->drive_type == registration_info->drive_type) {
            curren_handle = table_counter;
            current_matches ++;
            if (table_entry->drive_vendor == registration_info->drive_vendor) {
                curren_handle = table_counter;
                current_matches ++;
                if (fbe_drive_configuration_string_match(table_entry->part_number, registration_info->part_number, FBE_SCSI_INQUIRY_PART_NUMBER_SIZE)) {
                    curren_handle = table_counter;
                    current_matches ++;
                    if ( fbe_drive_configuration_string_match(table_entry->fw_revision, registration_info->fw_revision, FBE_SCSI_INQUIRY_REVISION_SIZE)) {
                        curren_handle = table_counter;
                        current_matches ++;
                        if (fbe_drive_configuration_sn_is_in_range(registration_info->serial_number, table_entry)) {
                            *handle = (fbe_drive_configuration_handle_t)table_counter;
                            break;/*a match at this level is perfect and we don't need to search more*/
                        }else if (*registration_info->serial_number != 0 && *table_entry->serial_number_start != 0) {
                            value_mismatch = FBE_TRUE;
                        }
                    }else if (*table_entry->fw_revision != 0 && *registration_info->fw_revision != 0) {
                        value_mismatch = FBE_TRUE;
                    }
                }else if (*table_entry->part_number != 0 && *registration_info->part_number != 0) {
                    value_mismatch = FBE_TRUE;
                }
            }else if (registration_info->drive_vendor != FBE_DRIVE_VENDOR_INVALID && table_entry->drive_vendor != FBE_DRIVE_VENDOR_INVALID) {
                value_mismatch = FBE_TRUE;
            }
        }

        /*we want the entry in the table that had the most matches to our drive*/
        if (current_matches > max_matches && !value_mismatch) {
            *handle = (fbe_drive_configuration_handle_t)curren_handle;
            max_matches = current_matches;
        }
    }

    if (curren_handle == FBE_DRIVE_CONFIGURATION_SERVICE_INVALID_HANLDLE) {    /* this is expected at startup, unless we load a default configuration. */
        drive_configuration_trace(FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: Can't match a threshold record with the drive information\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_drive_configuration_init_fbe_stat
 ****************************************************************
 * @brief
 *  Initalize a default fbe_stat record.
 * 
 ****************************************************************/
static void fbe_drive_configuration_init_fbe_stat(fbe_stat_t * fbe_stat, fbe_u64_t interval, fbe_u64_t weight)
{
    fbe_stat->interval = interval;
    fbe_stat->weight = weight;
    fbe_stat->threshold = 0;
    fbe_stat->control_flag = FBE_STATS_CNTRL_FLAG_DEFAULT;  /* obsolete. replaced by .actions */
    fbe_stat->actions.num = 0;
}

/*!**************************************************************
 * @fn fbe_drive_configuration_init_fbe_stat_add_action
 ****************************************************************
 * @brief
 *  Adds an fbe_stat action to a previously initialized fbe_stat
 *  record.  This function can only be called after calling
 *  fbe_drive_configuration_init_fbe_stat().
 * 
 * @param  fbe_stat - ptr to fbe_stat record.
 * @param  flag - action to add
 * @param  ratio - ratio to trigger the action
 * @param  reactivate_ratio - ratio to reactivate an action if this action
 *                           was already taken.
 * 
 * @return none.
 *
 * @version
 *   09/29/2015:  Wayne Garrett - Created.
 *
 ****************************************************************/
static void fbe_drive_configuration_init_fbe_stat_add_action(fbe_stat_t * fbe_stat, fbe_stat_action_flags_t flag, fbe_u32_t ratio, fbe_u32_t reactivate_ratio)
{
    fbe_u32_t i=fbe_stat->actions.num;
    fbe_stat->actions.entry[i].flag = flag;         /* flag that represents action.*/
    fbe_stat->actions.entry[i].ratio = ratio;        /* ratio to execute action*/
    fbe_stat->actions.entry[i].reactivate_ratio = reactivate_ratio;  /* low watermark to reactivate ratio*/    
    fbe_stat->actions.num++;
}

/*!**************************************************************
 * @fn fbe_drive_configuration_init_fbe_stat
 ****************************************************************
 * @brief
 *  Initalize the default DIEH records.
 * 
 ****************************************************************/
static void fbe_drive_configuration_init_dieh(void)
{
    fbe_u32_t       entry = 0;
    fbe_u32_t       active = 0;
    fbe_u32_t       table = 0;
    fbe_stat_t *    fbe_stat_p;

    fbe_spinlock_lock (&drive_configuration_service.threshold_table_lock);


    for (table = 0; table < DCS_THRESHOLD_TABLES; table++)
    {  
        fbe_drive_configuration_init_dieh_table(table);
    }

    /*clean the port statistics table*/
    for (entry = 0; entry < MAX_PORT_ENTRIES; entry++) {
        drive_configuration_service.threshold_port_record_table[entry].threshold_record.port_info.port_type = FBE_PORT_TYPE_INVALID;
        drive_configuration_service.threshold_port_record_table[entry].updated = FBE_FALSE;
        drive_configuration_service.threshold_port_record_table[entry].ref_count = 0;
    }

    /*now copy from the default tables*/
    entry = 0;
    active = dcs_get_active_table_index();
    
    drive_configuration_service.threshold_record_tables[active][0].threshold_record.drive_info.drive_type = FBE_DRIVE_TYPE_SAS;
    drive_configuration_service.threshold_record_tables[active][0].threshold_record.drive_info.drive_vendor = FBE_DRIVE_VENDOR_INVALID;
    drive_configuration_service.threshold_record_tables[active][0].threshold_record.drive_info.part_number[0] = '\0';
    drive_configuration_service.threshold_record_tables[active][0].threshold_record.drive_info.fw_revision[0] = '\0';
    drive_configuration_service.threshold_record_tables[active][0].threshold_record.drive_info.serial_number_start[0] = '\0';
    drive_configuration_service.threshold_record_tables[active][0].threshold_record.drive_info.serial_number_end[0] = '\0';

    /* Cummulative bucket */
    fbe_stat_p = &drive_configuration_service.threshold_record_tables[active][0].threshold_record.threshold_info.io_stat;
    fbe_drive_configuration_init_fbe_stat(fbe_stat_p, 1080000, 3333);
    fbe_drive_configuration_init_fbe_stat_add_action(fbe_stat_p, FBE_STAT_ACTION_FLAG_FLAG_RESET,       50,  10);
    fbe_drive_configuration_init_fbe_stat_add_action(fbe_stat_p, FBE_STAT_ACTION_FLAG_FLAG_END_OF_LIFE, 84,  FBE_U32_MAX);
    fbe_drive_configuration_init_fbe_stat_add_action(fbe_stat_p, FBE_STAT_ACTION_FLAG_FLAG_FAIL,        100, FBE_U32_MAX);

    /* Recovered Bucket */
    fbe_stat_p = &drive_configuration_service.threshold_record_tables[active][0].threshold_record.threshold_info.recovered_stat;
    fbe_drive_configuration_init_fbe_stat(fbe_stat_p, 1080000, 3600);
    fbe_drive_configuration_init_fbe_stat_add_action(fbe_stat_p, FBE_STAT_ACTION_FLAG_FLAG_RESET,       50,  10);
    fbe_drive_configuration_init_fbe_stat_add_action(fbe_stat_p, FBE_STAT_ACTION_FLAG_FLAG_END_OF_LIFE, 84,  FBE_U32_MAX);
    fbe_drive_configuration_init_fbe_stat_add_action(fbe_stat_p, FBE_STAT_ACTION_FLAG_FLAG_FAIL,        100, FBE_U32_MAX);

    /* Media Bucket */
    fbe_stat_p = &drive_configuration_service.threshold_record_tables[active][0].threshold_record.threshold_info.media_stat;
    fbe_drive_configuration_init_fbe_stat(fbe_stat_p, 1800000, 18000);
    fbe_drive_configuration_init_fbe_stat_add_action(fbe_stat_p, FBE_STAT_ACTION_FLAG_FLAG_RESET,       30,  5);
    fbe_drive_configuration_init_fbe_stat_add_action(fbe_stat_p, FBE_STAT_ACTION_FLAG_FLAG_END_OF_LIFE, 50,  FBE_U32_MAX);
    fbe_drive_configuration_init_fbe_stat_add_action(fbe_stat_p, FBE_STAT_ACTION_FLAG_FLAG_FAIL,        100, FBE_U32_MAX);

    /* Hardware Bucket */
    fbe_stat_p = &drive_configuration_service.threshold_record_tables[active][0].threshold_record.threshold_info.hardware_stat;
    fbe_drive_configuration_init_fbe_stat(fbe_stat_p, 1000000, 36000);
    fbe_drive_configuration_init_fbe_stat_add_action(fbe_stat_p, FBE_STAT_ACTION_FLAG_FLAG_RESET,       50,  10);
    fbe_drive_configuration_init_fbe_stat_add_action(fbe_stat_p, FBE_STAT_ACTION_FLAG_FLAG_END_OF_LIFE, 89,  FBE_U32_MAX);
    fbe_drive_configuration_init_fbe_stat_add_action(fbe_stat_p, FBE_STAT_ACTION_FLAG_FLAG_FAIL,        100, FBE_U32_MAX);

    /* Link Bucket */
    fbe_stat_p = &drive_configuration_service.threshold_record_tables[active][0].threshold_record.threshold_info.link_stat;
    fbe_drive_configuration_init_fbe_stat(fbe_stat_p, 1000, 13);
    fbe_drive_configuration_init_fbe_stat_add_action(fbe_stat_p, FBE_STAT_ACTION_FLAG_FLAG_RESET,       50,  10);
    fbe_drive_configuration_init_fbe_stat_add_action(fbe_stat_p, FBE_STAT_ACTION_FLAG_FLAG_FAIL,        100, FBE_U32_MAX);

    /* Health Check Bucket */
    fbe_stat_p = &drive_configuration_service.threshold_record_tables[active][0].threshold_record.threshold_info.healthCheck_stat;
    fbe_drive_configuration_init_fbe_stat(fbe_stat_p, 200000, 150000);
    fbe_drive_configuration_init_fbe_stat_add_action(fbe_stat_p, FBE_STAT_ACTION_FLAG_FLAG_FAIL,        100, FBE_U32_MAX);

    /* Data Bucket */
    fbe_stat_p = &drive_configuration_service.threshold_record_tables[active][0].threshold_record.threshold_info.data_stat;
    fbe_drive_configuration_init_fbe_stat(fbe_stat_p, 1000000, 10000);
    fbe_drive_configuration_init_fbe_stat_add_action(fbe_stat_p, FBE_STAT_ACTION_FLAG_FLAG_RESET,       50,  10);
    fbe_drive_configuration_init_fbe_stat_add_action(fbe_stat_p, FBE_STAT_ACTION_FLAG_FLAG_END_OF_LIFE, 89,  FBE_U32_MAX);
    fbe_drive_configuration_init_fbe_stat_add_action(fbe_stat_p, FBE_STAT_ACTION_FLAG_FLAG_FAIL,        100, FBE_U32_MAX);


    drive_configuration_service.threshold_record_tables[active][0].threshold_record.threshold_info.error_burst_delta = 100;/*delta between errors to indicate error burst*/
    drive_configuration_service.threshold_record_tables[active][0].threshold_record.threshold_info.error_burst_weight_reduce = 20;/*percent of weight reduce for error burst*/
    drive_configuration_service.threshold_record_tables[active][0].threshold_record.threshold_info.recovery_tag_reduce = 30;/* percent of interval to reduce tag after recovery */
    drive_configuration_service.threshold_record_tables[active][0].threshold_record.threshold_info.record_flags = FBE_STATS_FLAG_RELIABLY_CHALLENGED;

    entry = 0;
    while(default_port_record_table[entry].port_info.port_type != FBE_PORT_TYPE_INVALID ) {
        fbe_copy_memory(&drive_configuration_service.threshold_port_record_table[entry].threshold_record,
                        &default_port_record_table[entry],
                        sizeof(fbe_drive_configuration_port_record_t));

        drive_configuration_service.threshold_port_record_table[entry].updated = FBE_TRUE;/*make sure if we pull all drives it would not be removed*/
        entry ++;
    }

    /*clean teh tbale that tells us this drive type got change notification*/
    for (entry = 0; entry < NUMBER_OF_DRIVE_TYPES; entry++) {
        drive_configuration_service.drive_type_update_sent[entry] = FBE_FALSE;;
    }

    /*clean teh tbale that tells us this port type got change notification*/
    for (entry = 0; entry < NUMBER_OF_PORT_TYPES; entry++) {
        drive_configuration_service.port_type_update_sent[entry] = FBE_FALSE;;
    }

    fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);

}

/*!**************************************************************
 * @fn fbe_drive_configuration_init_dieh_table
 ****************************************************************
 * @brief
 *  Initalize/clear a DIEH record table.
 * 
 * @param  table - table index to init.
 * 
 * @return none.
 *
 * @version
 *   10/04/2012:  Wayne Garrett - Created.
 *
 ****************************************************************/
static void fbe_drive_configuration_init_dieh_table(fbe_u32_t table)
{
    fbe_u32_t entry;

    /*first clear*/    
    fbe_zero_memory (&drive_configuration_service.threshold_record_tables[table], (sizeof(drive_configuration_table_drive_entry_t) * MAX_THRESHOLD_TABLE_RECORDS));

    for (entry = 0; entry < MAX_THRESHOLD_TABLE_RECORDS; entry++) {
        drive_configuration_service.threshold_record_tables[table][entry].threshold_record.drive_info.drive_type = FBE_DRIVE_TYPE_INVALID;
        drive_configuration_service.threshold_record_tables[table][entry].updated = FBE_FALSE;
    }

}

static fbe_bool_t fbe_drive_configuration_sn_is_in_range(const fbe_u8_t *drive_sn, fbe_drive_configuration_drive_info_t *table_entry)
{
    fbe_s32_t       start_range = 0;
    fbe_s32_t       end_range = 0;

    start_range = fbe_compare_string((fbe_u8_t *)drive_sn, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE, table_entry->serial_number_start, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE, FALSE);
    end_range = fbe_compare_string((fbe_u8_t *)drive_sn, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE, table_entry->serial_number_end, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE, FALSE);

    if (start_range >= 0 && end_range <=0) {
        return FBE_TRUE;
    }else{
        return FBE_FALSE;
    }
    
}

static fbe_bool_t fbe_drive_configuration_string_match(fbe_u8_t *str1, fbe_u8_t *str2, fbe_u32_t max_length)
{
    fbe_u32_t       length = 0;
    fbe_u8_t *      temp_str = str1;

    /*since the strings may contain more garbage after the null termination, we need to find the exact length of the
    string, but do it safely*/
    while (*temp_str && length <= max_length) {
        temp_str++;
        length++;
    }

    if (length == 0 && *str2 != 0) {
        return FBE_FALSE;
    }

    return fbe_equal_memory(str1, str2, length);
}

fbe_status_t fbe_drive_configuration_get_threshold_info(fbe_drive_configuration_handle_t handle, fbe_drive_threshold_and_exceptions_t  *threshold_rec)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t           table_index, entry_index;


    if (threshold_rec == NULL ) {
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: data return pointer is NULL\n", __FUNCTION__, handle);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    fbe_spinlock_lock (&drive_configuration_service.threshold_table_lock);
    
    table_index = dcs_handle_to_table_index(handle);
    entry_index = dcs_handle_to_entry_index(handle);

    /* this should only be called for an active table handle */
    if (table_index != dcs_get_active_table_index())
    {
        drive_configuration_trace(FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: failure. handle 0x%x is not active\n", __FUNCTION__, handle);
        fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);
        return FBE_STATUS_GENERIC_FAILURE;
    }   

    status  = fbe_drive_configuration_validate_drive_handle(handle);
    if (status != FBE_STATUS_OK) 
    {
        fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);
        return status;
    }    

    fbe_copy_memory(&threshold_rec->threshold_info,
                    &drive_configuration_service.threshold_record_tables[table_index][entry_index].threshold_record.threshold_info,
                    sizeof (threshold_rec->threshold_info));

    fbe_copy_memory(&threshold_rec->category_exception_codes,
                    &drive_configuration_service.threshold_record_tables[table_index][entry_index].threshold_record.category_exception_codes,
                    sizeof (threshold_rec->category_exception_codes));

    fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);

    return FBE_STATUS_OK;
}


fbe_status_t fbe_drive_configuration_get_threshold_info_ptr(fbe_drive_configuration_handle_t handle, fbe_drive_configuration_record_t **threshold_rec_pp)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t           table_index, entry_index;


    if (threshold_rec_pp == NULL ) {
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: data return pointer is NULL\n", __FUNCTION__, handle);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (handle == FBE_DRIVE_CONFIGURATION_SERVICE_INVALID_HANLDLE) {
        /* not tracing this since it's expected if DIEH is disabled. Let caller decide what to do. */
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *threshold_rec_pp = NULL;

    fbe_spinlock_lock (&drive_configuration_service.threshold_table_lock);
    
    table_index = dcs_handle_to_table_index(handle);
    entry_index = dcs_handle_to_entry_index(handle);

    /* this should only be called for an active table handle */
    if (table_index != dcs_get_active_table_index())
    {
        drive_configuration_trace(FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: failure. handle 0x%x is not active\n", __FUNCTION__, handle);
        fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);
        return FBE_STATUS_GENERIC_FAILURE;
    }   

    status  = fbe_drive_configuration_validate_drive_handle(handle);
    if (status != FBE_STATUS_OK) 
    {
        fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);
        return status;
    }    

    *threshold_rec_pp = &drive_configuration_service.threshold_record_tables[table_index][entry_index].threshold_record;

    fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);

    return FBE_STATUS_OK;
}


static fbe_status_t fbe_drive_configuration_control_add_record(fbe_packet_t *packet)
{
    fbe_payload_control_buffer_length_t                 len = 0;
    fbe_payload_ex_t *                                  payload = NULL;
    fbe_payload_control_operation_t *                   control_operation = NULL;
    fbe_drive_configuration_control_add_record_t *      add_record = NULL;
    fbe_status_t                                        status = FBE_STATUS_GENERIC_FAILURE;

    drive_configuration_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s: entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);   

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if(len != sizeof(fbe_drive_configuration_control_add_record_t)){
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Invalid buffer_len \n", __FUNCTION__);

        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &add_record); 
    if(add_record == NULL){
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: fbe_payload_control_get_buffer fail\n", __FUNCTION__);

        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*we can't do that before the start update was done*/
    if (drive_configuration_service.update_started != FBE_TRUE) {
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: start_update was not called\n", __FUNCTION__);

        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (!((add_record->new_record.drive_info.drive_type > FBE_DRIVE_TYPE_INVALID) && 
        (add_record->new_record.drive_info.drive_type < FBE_DRIVE_TYPE_LAST))) {
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Invalid drive type %d\n", __FUNCTION__, add_record->new_record.drive_info.drive_type);

        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_drive_configuration_add_record(&add_record->new_record, &add_record->handle);
    
    fbe_payload_control_set_status (control_operation, (status == FBE_STATUS_OK ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE) );
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

static fbe_status_t fbe_drive_configuration_add_record(fbe_drive_configuration_record_t *new_record, fbe_drive_configuration_handle_t *handle)
{   
    fbe_u32_t               table_entry = 0, entry_index = 0;
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_bool_t              entry_identical = FBE_FALSE;
    fbe_u32_t               updating = 0;
    
    if (handle == NULL || new_record == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_spinlock_lock (&drive_configuration_service.threshold_table_lock);

    updating = dcs_get_updating_table_index();

    /*any chance we already have an entry there which has an identical drive information?*/
    for (entry_index = 0; entry_index < MAX_THRESHOLD_TABLE_RECORDS; entry_index++) {
        /*check if the drive details are identical
        In this case we would just copy the threshold data(regardless if it changed or not), but all PDOs would not need a new handle*/
        entry_identical = fbe_equal_memory(&drive_configuration_service.threshold_record_tables[updating][entry_index].threshold_record.drive_info,
                                           &new_record->drive_info,
                                           sizeof(fbe_drive_configuration_drive_info_t));

        if (entry_identical) {
            drive_configuration_service.threshold_record_tables[updating][entry_index].updated = FBE_TRUE;/*since the user sent it, its an entry we want to keep*/
            /*now we have to copy the threshold data (we copy everything to make it simpler)*/
            fbe_copy_memory(&drive_configuration_service.threshold_record_tables[updating][entry_index].threshold_record,
                            new_record,
                            sizeof(fbe_drive_configuration_record_t));

            *handle = entry_index | (updating << DCS_TABLE_INDEX_SHIFT);
            fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);
            return FBE_STATUS_OK;
        }

    }

    /*search for a free place since this record did not exist before*/
    status  = fbe_drive_configuration_get_free_drive_table_entry(updating, &table_entry);
    if (FBE_STATUS_INSUFFICIENT_RESOURCES == status) {
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Unable to get free entry in error threshold table\n", __FUNCTION__);

        fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*and copy the data to it*/
    fbe_copy_memory (&drive_configuration_service.threshold_record_tables[updating][table_entry].threshold_record,
                     new_record,
                     sizeof (fbe_drive_configuration_record_t));

    fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);

    /* encode table index into handle */
    *handle = table_entry | (updating << DCS_TABLE_INDEX_SHIFT);

    /*we also mark this table entry as updated. This way we know it's a new/changed record*/
    drive_configuration_service.threshold_record_tables[updating][table_entry].updated = FBE_TRUE;

    return FBE_STATUS_OK;
}

/*NOTE: Caller responsible for guarding threshold table*/
static fbe_status_t fbe_drive_configuration_get_free_drive_table_entry(fbe_u32_t table_index, fbe_u32_t *entry_index)
{
    fbe_u32_t       entry = 0;

    /*look for a free entry*/
    for (entry = 0; entry < MAX_THRESHOLD_TABLE_RECORDS; entry++) {
        if (drive_configuration_service.threshold_record_tables[table_index][entry].threshold_record.drive_info.drive_type == FBE_DRIVE_TYPE_INVALID){
            *entry_index = entry;
            return FBE_STATUS_OK;
        }
    }

    /*if we got here it's bad, we have no places left*/
    return FBE_STATUS_INSUFFICIENT_RESOURCES;
}

static fbe_status_t fbe_drive_configuration_control_change_drive_thresholds(fbe_packet_t *packet)
{
    fbe_payload_control_buffer_length_t                     len = 0;
    fbe_payload_ex_t *                                      payload = NULL;
    fbe_payload_control_operation_t *                       control_operation = NULL;
    fbe_drive_configuration_control_change_thresholds_t *   change_threshold = NULL;
    fbe_status_t                                            status = FBE_STATUS_GENERIC_FAILURE;

    drive_configuration_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s: entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);   

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if(len != sizeof(fbe_drive_configuration_control_change_thresholds_t)){
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Invalid buffer_len \n", __FUNCTION__);

        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &change_threshold); 
    if(change_threshold == NULL){
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: fbe_payload_control_get_buffer fail\n", __FUNCTION__);

        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_drive_configuration_change_drive_threshold(change_threshold);

    fbe_payload_control_set_status (control_operation, (status == FBE_STATUS_OK ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE) );
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

static fbe_status_t fbe_drive_configuration_control_get_handles_list(fbe_packet_t *packet)
{
    fbe_payload_control_buffer_length_t                     len = 0;
    fbe_payload_ex_t *                                      payload = NULL;
    fbe_payload_control_operation_t *                       control_operation = NULL;
    fbe_drive_configuration_control_get_handles_list_t *    get_handles = NULL;
    fbe_s32_t                                               entry = 0;
    fbe_u32_t                                               total_handles = 0;
    fbe_u32_t                                               active;

    drive_configuration_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s: entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);   

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if(len != sizeof(fbe_drive_configuration_control_get_handles_list_t)){
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Invalid buffer_len. expected:%d actual:%d \n", __FUNCTION__, sizeof(fbe_drive_configuration_control_get_handles_list_t), len);

        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &get_handles); 
    if(get_handles == NULL){
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: fbe_payload_control_get_buffer fail\n", __FUNCTION__);

        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

   /*now let's populate the list*/

    fbe_spinlock_lock (&drive_configuration_service.threshold_table_lock);

    active = dcs_get_active_table_index();
    for (entry = 0; entry < MAX_THRESHOLD_TABLE_RECORDS; entry++) {
        if (drive_configuration_service.threshold_record_tables[active][entry].threshold_record.drive_info.drive_type != FBE_DRIVE_TYPE_INVALID){
            get_handles->handles_list[total_handles] = (fbe_drive_configuration_handle_t)entry | active << DCS_TABLE_INDEX_SHIFT;
            total_handles ++;
        }
    }
    fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);

    get_handles->total_count = total_handles;

    fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

static fbe_status_t fbe_drive_configuration_change_drive_threshold(fbe_drive_configuration_control_change_thresholds_t * change_threshold)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t       table_index, entry_index;                    

    /*and change the record*/
    fbe_spinlock_lock (&drive_configuration_service.threshold_table_lock);

    table_index = dcs_handle_to_table_index(change_threshold->handle);
    entry_index = dcs_handle_to_entry_index(change_threshold->handle);


    /* this should only be called for an active table handle */
    if (table_index != dcs_get_active_table_index())
    {
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: failure. handle 0x%x is not active\n", __FUNCTION__, change_threshold->handle);
        fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);
        return FBE_STATUS_GENERIC_FAILURE;
    }   


    status  = fbe_drive_configuration_validate_drive_handle(change_threshold->handle);
    if (status != FBE_STATUS_OK) {
        fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);
        return status;
    }


    fbe_copy_memory (&drive_configuration_service.threshold_record_tables[table_index][entry_index].threshold_record.threshold_info,
                     &change_threshold->new_threshold,
                     sizeof (fbe_drive_stat_t));

    fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);

    return FBE_STATUS_OK;
}

static fbe_status_t fbe_drive_configuration_control_get_drive_record(fbe_packet_t *packet)
{
    fbe_payload_control_buffer_length_t                     len = 0;
    fbe_payload_ex_t *                                      payload = NULL;
    fbe_payload_control_operation_t *                       control_operation = NULL;
    fbe_drive_configuration_control_get_drive_record_t *    get_record = NULL;
    fbe_status_t                                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                                               table_index, entry_index;
    
    drive_configuration_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s: entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);   

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if(len != sizeof(fbe_drive_configuration_control_get_drive_record_t)){
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Invalid buffer_len \n", __FUNCTION__);

        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &get_record); 
    if(get_record == NULL){
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: fbe_payload_control_get_buffer fail\n", __FUNCTION__);

        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    fbe_spinlock_lock (&drive_configuration_service.threshold_table_lock);

    table_index = dcs_handle_to_table_index(get_record->handle);
    entry_index = dcs_handle_to_entry_index(get_record->handle);

    /* this should only be called for an active table handle */
    if (table_index != dcs_get_active_table_index())
    {
        drive_configuration_trace(FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: failure. handle 0x%x is not active\n", __FUNCTION__, get_record->handle);
        fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);

        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }   

    /*make sure the handle is valid*/
    status = fbe_drive_configuration_validate_drive_handle(get_record->handle);
    if (status != FBE_STATUS_OK) {
        drive_configuration_trace(FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Invalid handle: 0x%x\n", __FUNCTION__, get_record->handle);
        fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);

        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

   /*now let's populate the list*/    
    fbe_copy_memory(&get_record->record,
                    &drive_configuration_service.threshold_record_tables[table_index][entry_index].threshold_record,
                    sizeof(fbe_drive_configuration_record_t));

    fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);
    
    fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

fbe_status_t fbe_drive_configuration_get_scsi_exception_action (fbe_drive_configuration_record_t *threshold_rec,
                                                                fbe_payload_cdb_operation_t *cdb_operation, 
                                                                fbe_payload_cdb_fis_io_status_t *io_status,
                                                                fbe_payload_cdb_fis_error_t *type_and_action)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                                   exception_index = 0;
    fbe_drive_config_scsi_sense_code_t          error_exception;
    fbe_u8_t *                                  sense_buffer = NULL;
    fbe_scsi_sense_key_t                        sense_key = FBE_SCSI_SENSE_KEY_NO_SENSE; 
    fbe_scsi_additional_sense_code_t            asc = FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO; 
    fbe_scsi_additional_sense_code_qualifier_t  ascq = FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO;
    fbe_bool_t                                  lba_is_valid = FBE_FALSE;
    fbe_lba_t                                   bad_lba = 0;

        
    if (NULL == threshold_rec || NULL == cdb_operation || NULL == io_status || NULL == type_and_action) {
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Invalid pointers passed in\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;  
    }                
                
    fbe_payload_cdb_operation_get_sense_buffer(cdb_operation, &sense_buffer);
    
    /* Get sense code data */
    status = fbe_payload_cdb_parse_sense_data(sense_buffer, &sense_key, &asc, &ascq, &lba_is_valid, &bad_lba);

    if (FBE_STATUS_OK == status) 
    {
        /*check if we need to recommand on an exception action for the drive
         * This means for this kind of drive we recommand a different action than
         * the drive would usually map
         */
        for (exception_index = 0; exception_index < MAX_EXCEPTION_CODES; exception_index ++) {
            error_exception = threshold_rec->category_exception_codes[exception_index].scsi_fis_union.scsi_code;
            if (error_exception.sense_key == sense_key) {
                if ((error_exception.asc_range_start <= asc) && (error_exception.asc_range_end >= asc)) {
                    if ((error_exception.ascq_range_start <= ascq) && (error_exception.ascq_range_end >= ascq)) {
                        /*we have a match*/
                        *io_status = threshold_rec->category_exception_codes[exception_index].status;
                        *type_and_action = threshold_rec->category_exception_codes[exception_index].type_and_action;
                        return FBE_STATUS_OK;
                    }
                }
            }
        }
    }

    *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_OK;
    *type_and_action = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NO_ERROR;
    return FBE_STATUS_CONTINUE;/*if we had no match we would let the user he can go on to the hard coded mapping*/
}


static fbe_status_t fbe_drive_configuration_validate_drive_handle(fbe_drive_configuration_handle_t handle)
{
    fbe_u32_t table_index, entry_index;

    table_index = dcs_handle_to_table_index(handle);
    entry_index = dcs_handle_to_entry_index(handle);

    if (handle == FBE_DRIVE_CONFIGURATION_SERVICE_INVALID_HANLDLE ||
        entry_index < 0 || entry_index >= MAX_THRESHOLD_TABLE_RECORDS ) {
        drive_configuration_trace(FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Invalid handle: %d\n", __FUNCTION__, handle);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (drive_configuration_service.threshold_record_tables[table_index][entry_index].threshold_record.drive_info.drive_type == FBE_DRIVE_TYPE_INVALID) {
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Handle points to an empty entry. handle: %d\n", __FUNCTION__, handle);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;

}


fbe_status_t fbe_drive_configuration_get_fis_exception_action (fbe_drive_configuration_handle_t handle,
                                                                fbe_payload_fis_operation_t *fis_operation, 
                                                                fbe_payload_cdb_fis_io_status_t *io_status,
                                                                fbe_payload_cdb_fis_error_flags_t *type_and_action)
{

    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                       exception_index = 0;
    fbe_drive_config_fis_code_t     error_exception;
    fbe_u32_t                       table_index, entry_index;
    
    if (io_status == NULL || type_and_action == NULL ) {
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Invalid pointers passed in\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;

    }
            
    fbe_spinlock_lock (&drive_configuration_service.threshold_table_lock);

    table_index = dcs_handle_to_table_index(handle);
    entry_index = dcs_handle_to_entry_index(handle);

    /* this should only be called for an active table handle */
    if (table_index != dcs_get_active_table_index())
    {
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: failure. handle 0x%x is not active\n", __FUNCTION__, handle);
        fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);
        return FBE_STATUS_GENERIC_FAILURE;
    }  
            
    status  = fbe_drive_configuration_validate_drive_handle(handle);
    if (status != FBE_STATUS_OK) {
        fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);
        return status;
    }
    
    for (exception_index = 0; exception_index < MAX_EXCEPTION_CODES; exception_index ++) {
        error_exception = drive_configuration_service.threshold_record_tables[table_index][entry_index].threshold_record.category_exception_codes[exception_index].scsi_fis_union.fis_code;
        /*TODO - add here logic to find expections, return FBE_STATUS_OK if you had a match*/
    }

    fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);
    
    *io_status = FBE_PAYLOAD_CDB_FIS_IO_STATUS_OK;
    *type_and_action = FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_NO_ERROR;

    return FBE_STATUS_CONTINUE;/*if we had no match we would let the user he can go on to the hard coded mapping*/

}

static fbe_status_t fbe_drive_configuration_start_table_update(fbe_packet_t *packet)
{
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_ex_t *                      payload = NULL;
    fbe_payload_control_operation_t *       control_operation = NULL;
    fbe_u32_t                               entry = 0;
    fbe_u32_t                               updating = 0;
    
    drive_configuration_trace(FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s: entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);   

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if(len != 0){
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Invalid buffer_len \n", __FUNCTION__);

        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* if previous update hasn't completed yet, then fail the request since we
       are probably still waiting for drives to unregister from the old table index*/
    if (drive_configuration_service.update_started)
    {
        drive_configuration_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: DIEH Failure. start_update is already in progress. \n", __FUNCTION__);

        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_payload_control_set_status_qualifier (control_operation, FBE_DCS_DIEH_STATUS_FAILED_UPDATE_IN_PROGRESS);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    fbe_spinlock_lock (&drive_configuration_service.threshold_table_lock);

    updating = dcs_get_updating_table_index();

    fbe_drive_configuration_init_dieh_table(updating);   /*first clear table*/

    /*all entried are marked as not updated, this way, we know to delete them if they were not updated
    when we are at the end of the process (fbe_drive_configuration_end_table_update)*/
    for (entry = 0; entry < MAX_THRESHOLD_TABLE_RECORDS; entry++) {
        drive_configuration_service.threshold_record_tables[updating][entry].updated = FBE_FALSE;
    }

    /*clean teh tbale that tells us this drive type got change notification*/
    for (entry = 0; entry < NUMBER_OF_DRIVE_TYPES; entry++) {
        drive_configuration_service.drive_type_update_sent[entry] = FBE_FALSE;;
    }

    /*clean teh tbale that tells us this drive type got change notification*/
    for (entry = 0; entry < NUMBER_OF_PORT_TYPES; entry++) {
        drive_configuration_service.port_type_update_sent[entry] = FBE_FALSE;;
    }

    drive_configuration_service.update_started = FBE_TRUE;

    fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);
    
    fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

static fbe_status_t fbe_drive_configuration_end_table_update(fbe_packet_t *packet)
{
    fbe_payload_control_buffer_length_t                     len = 0;
    fbe_payload_ex_t *                                      payload = NULL;
    fbe_payload_control_operation_t *                       control_operation = NULL;
    fbe_status_t                                            status = FBE_STATUS_GENERIC_FAILURE;
    
    drive_configuration_trace(FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s: entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);   

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if(len != 0){
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Invalid buffer_len \n", __FUNCTION__);

        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_spinlock_lock (&drive_configuration_service.threshold_table_lock);

    dcs_activate_new_table();
    drive_configuration_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "DIEH: active table changed. idx:%d\n", drive_configuration_service.active_table_index);

    fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);

    /* tell all drives to unregister and reregister with new table records.  After all 
      drives unregister the update_started flag will be cleared, indicating another
      update can be performed. */
    drive_configuration_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "DIEH: send_update_to_all_drives\n");
    status = drive_configuration_send_update_to_all_drives();

    /* It's possible that no cleints have registered with old table.  If that's the case then we need to clear the update flag */
    if ( dcs_is_all_unregistered_from_old_table() ) 
    {
        drive_configuration_service.update_started = FBE_FALSE;

        drive_configuration_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "DIEH - update complete(line:%d). all unregistered for old table idx: %d \n", __LINE__, dcs_get_updating_table_index());
    }
    
    fbe_payload_control_set_status (control_operation, (status == FBE_STATUS_OK) ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;

}

static fbe_status_t drive_configuration_send_update_to_all_drives(void)
{
    fbe_topology_control_get_physical_drive_objects_t * pDriveObjects = NULL;
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t       object_idx = 0;

    pDriveObjects = (fbe_topology_control_get_physical_drive_objects_t *)fbe_memory_native_allocate(sizeof(fbe_topology_control_get_physical_drive_objects_t));
    if (pDriveObjects == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Send update to all drives regardless if a record was updated for it.  This is the easiest solution */

    status = fbe_drive_configuration_get_physical_drive_objects(pDriveObjects);
    if (status != FBE_STATUS_OK) {
        fbe_memory_native_release(pDriveObjects);
        return status;
    }
     
    /*now, for each of them, we send the update usurper command*/
    for (object_idx = 0; object_idx < pDriveObjects->number_of_objects; object_idx++) {
        status = fbe_drive_configuration_send_drive_table_change_usurper(pDriveObjects->pdo_list[object_idx]);
        if (status != FBE_STATUS_OK) {
            fbe_memory_native_release(pDriveObjects);
            return status;
        }
    }
    
    fbe_memory_native_release(pDriveObjects);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_drive_configuration_unregister_drive (fbe_drive_configuration_handle_t handle)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t           table_index, entry_index;

    status = fbe_drive_configuration_validate_drive_handle(handle);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    /*decrement ref count*/
    fbe_spinlock_lock (&drive_configuration_service.threshold_table_lock);

    table_index = dcs_handle_to_table_index(handle);
    entry_index = dcs_handle_to_entry_index(handle);

    if (drive_configuration_service.threshold_record_tables[table_index][entry_index].ref_count > 0)  
    {
        drive_configuration_service.threshold_record_tables[table_index][entry_index].ref_count--;
    }
    else
    {
        drive_configuration_trace(FBE_TRACE_LEVEL_WARNING,FBE_TRACE_MESSAGE_ID_INFO,
                                  "DIEH unreg_drive ref count already 0.  Expected only if force cleared \n");
    }

    /* If drives are unregistering because of a table update, then check if all drives have unregistered.   When they
       have then clear the flag which indicates a table update is no longer in progress.  This will allow
       subsequent updates to be preformed safely. */
    if (drive_configuration_service.update_started)
    {
        if ( dcs_is_all_unregistered_from_old_table() ) 
        {
            drive_configuration_service.update_started = FBE_FALSE;

            drive_configuration_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                      "DIEH - update complete(line:%d). all unregistered for old table idx: %d \n", __LINE__, dcs_get_updating_table_index());
        }
    }
        
    fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);
    
    return FBE_STATUS_OK;
}

fbe_status_t fbe_drive_configuration_did_drive_handle_change(fbe_drive_configuration_registration_info_t *registration_info,
                                                       fbe_drive_configuration_handle_t handle,
                                                       fbe_bool_t *changed)
{

    fbe_drive_configuration_handle_t        temp_handle = FBE_DRIVE_CONFIGURATION_SERVICE_INVALID_HANLDLE;
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                               active;


    fbe_spinlock_lock (&drive_configuration_service.threshold_table_lock);

    /* If handle doesn't have the active table index then config has changed.  */
    active = dcs_get_active_table_index();
    if ( dcs_handle_to_table_index(handle) != active )
    {
        *changed = FBE_TRUE;
        fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);
        return FBE_STATUS_OK;
    }

    status = fbe_drive_configuration_validate_drive_handle(handle);
    if (status != FBE_STATUS_OK) {
        fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);
        return status;
    }

    if (changed == NULL) {
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: NULL pointer passed in\n", __FUNCTION__);
        fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*first we get a handle to the registration info passed in*/
    status = fbe_drive_configuration_get_handle_index(registration_info, &temp_handle);
    if (status == FBE_STATUS_GENERIC_FAILURE) {
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: NULL pointer passed in\n", __FUNCTION__);

        fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);

    /*encode table index in handle*/
    temp_handle |= (active << DCS_TABLE_INDEX_SHIFT);

    if (handle == temp_handle) {
        *changed  = FBE_FALSE;/*this is the same entry, the object has to do nothing*/
    }else {
        *changed  = FBE_TRUE;/*the object would need to de register and re-register*/
    }

    return FBE_STATUS_OK;
}

static fbe_status_t drive_configuration_get_all_drive_objects_of_type(fbe_topology_mgmt_get_all_drive_of_type_t *get_objects)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *                      packet = NULL;
    fbe_semaphore_t                     sem;
    fbe_payload_ex_t *                      payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;

    fbe_semaphore_init(&sem, 0, 1);

    /* Allocate packet.*/
    packet = (fbe_packet_t *) fbe_memory_native_allocate (sizeof (fbe_packet_t));

    if (packet == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    fbe_transport_initialize_packet(packet);
    payload = fbe_transport_get_payload_ex (packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);
    fbe_payload_control_build_operation (control_operation,
                                         FBE_TOPOLOGY_CONTROL_CODE_GET_ALL_DRIVES_OF_TYPE,
                                         get_objects,
                                         sizeof(fbe_topology_mgmt_get_all_drive_of_type_t));

    fbe_transport_set_completion_function (packet, drive_configuration_get_all_objects_of_type_completion,  &sem);
    

    /* Set packet address.*/
    fbe_transport_set_address(packet,
                              FBE_PACKAGE_ID_PHYSICAL,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              FBE_OBJECT_ID_INVALID); 
    
    /* Mark packet with the attribute, either traverse or not */
    fbe_transport_set_packet_attr(packet, FBE_PACKET_FLAG_NO_ATTRIB);

    status = fbe_service_manager_send_control_packet(packet);

    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
        /* Only trace when it is an error we do not expect.*/
         drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s error in sending packet, status:%d\n", __FUNCTION__, status); 

         fbe_semaphore_destroy(&sem);
         fbe_transport_destroy_packet(packet);
         fbe_memory_native_release(packet);
         return status;

    }

    /* Wait for the callback.  Our completion function is always called.*/
    fbe_semaphore_wait(&sem, NULL);

    /* Save the status for returning.*/
    status = fbe_transport_get_status_code(packet);
    fbe_payload_control_get_status(control_operation, &control_status);

    fbe_semaphore_destroy(&sem);
    fbe_transport_destroy_packet(packet);
    fbe_memory_native_release(packet);

    return ((status == FBE_STATUS_OK && control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) ? FBE_STATUS_OK :FBE_STATUS_GENERIC_FAILURE) ;


}


static fbe_status_t drive_configuration_get_all_objects_of_type_completion (fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
    fbe_semaphore_release(sem, 0, 1, FALSE);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_drive_configuration_send_drive_table_change_usurper(fbe_object_id_t object_id)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *                      packet = NULL;
    fbe_semaphore_t                     sem;
    fbe_payload_ex_t *                      payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;

    fbe_semaphore_init(&sem, 0, 1);

    /* Allocate packet.*/
    packet = (fbe_packet_t *) fbe_memory_native_allocate (sizeof (fbe_packet_t));

    if (packet == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    fbe_transport_initialize_packet(packet);
    payload = fbe_transport_get_payload_ex (packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);
    fbe_payload_control_build_operation (control_operation,
                                         FBE_PHYSICAL_DRIVE_CONTROL_CODE_STAT_CONFIG_TABLE_CHANGED,
                                         NULL,
                                         0);

    fbe_transport_set_completion_function (packet, fbe_drive_configuration_send_table_change_usurper_completion,  &sem);
    

    /* Set packet address.*/
    fbe_transport_set_address(packet,
                              FBE_PACKAGE_ID_PHYSICAL,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              object_id); 
    
    /* Mark packet with the attribute, either traverse or not */
    fbe_transport_set_packet_attr(packet, FBE_PACKET_FLAG_NO_ATTRIB);

    status = fbe_service_manager_send_control_packet(packet);

    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
        /* Only trace when it is an error we do not expect.*/
         drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s error in sending packet, status:%d\n", __FUNCTION__, status); 

         fbe_semaphore_destroy(&sem);
         fbe_transport_destroy_packet(packet);
         fbe_memory_native_release(packet);
         return status;

    }

    /* Wait for the callback.  Our completion function is always called.*/
    fbe_semaphore_wait(&sem, NULL);

    /* Save the status for returning.*/
    status = fbe_transport_get_status_code(packet);
    fbe_payload_control_get_status(control_operation, &control_status);

    fbe_semaphore_destroy(&sem);
    fbe_transport_destroy_packet(packet);
    fbe_memory_native_release(packet);

    return ((status == FBE_STATUS_OK && control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) ? FBE_STATUS_OK :FBE_STATUS_GENERIC_FAILURE) ;

}

static fbe_status_t fbe_drive_configuration_send_table_change_usurper_completion (fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
    fbe_semaphore_release(sem, 0, 1, FALSE);
    return FBE_STATUS_OK;
}


fbe_status_t fbe_drive_configuration_register_port (fbe_drive_configuration_port_info_t *registration_info,
                                                    fbe_drive_configuration_handle_t *handle)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;

    /*to start with, we get a hndle*/
    status = fbe_drive_configuration_get_port_hanlde(registration_info, handle);
    if (status != FBE_STATUS_OK) {
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: unalble to get handle, can't register\n", __FUNCTION__);

        return status;
    }

    drive_configuration_service.threshold_port_record_table[*handle].ref_count++;

    return FBE_STATUS_OK;

}

fbe_status_t fbe_drive_configuration_unregister_port (fbe_drive_configuration_handle_t handle)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;

    status = fbe_drive_configuration_validate_port_handle(handle);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    /*decrement ref count*/
    fbe_spinlock_lock (&drive_configuration_service.threshold_table_lock);
    drive_configuration_service.threshold_port_record_table[handle].ref_count--;

    /*check if this is a record that needs to be removed(no one registered to it anymore and it was not recently updated)*/
    if (drive_configuration_service.threshold_port_record_table[handle].ref_count == 0 && 
        drive_configuration_service.threshold_port_record_table[handle].updated == FBE_FALSE) {

        /*we remove this entry since eveyone deregistered from it and it did not get new updated*/
        drive_configuration_service.threshold_port_record_table[handle].threshold_record.port_info.port_type = FBE_PORT_TYPE_INVALID;
        drive_configuration_service.threshold_port_record_table[handle].updated = FBE_FALSE;
    }
    
    fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);

    

    return FBE_STATUS_OK;

}

static fbe_status_t fbe_drive_configuration_get_port_hanlde(fbe_drive_configuration_port_info_t *registration_info,
                                                            fbe_drive_configuration_handle_t *handle)
{
    fbe_s32_t                               curren_handle = FBE_DRIVE_CONFIGURATION_SERVICE_INVALID_HANLDLE;
    fbe_s32_t                               table_counter = 0;
    fbe_drive_configuration_port_record_t * table_entry;
    
    if (!service_initialized){
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Service uninitialized\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_spinlock_lock (&drive_configuration_service.threshold_table_lock);

    /*let's find a match for the parameters that drive has
    the main concept is to do a match by this hirarchy: CLASS->Vendor->Product ID->REV->Serail number*/
    for (table_counter = 0; table_counter < MAX_PORT_ENTRIES; table_counter++) {
        table_entry = &drive_configuration_service.threshold_port_record_table[table_counter].threshold_record;

        if (table_entry->port_info.port_type == registration_info->port_type) {
            curren_handle = table_counter;
            *handle = (fbe_drive_configuration_handle_t)table_counter;
        }
    }

    fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);

    if (curren_handle == FBE_DRIVE_CONFIGURATION_SERVICE_INVALID_HANLDLE) {
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: Can't match a threshold record with the drive information\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

static fbe_status_t fbe_drive_configuration_validate_port_handle(fbe_drive_configuration_handle_t handle)
{
    if (handle < 0 || handle >= MAX_PORT_ENTRIES ) {
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Invalid handle: %d\n", __FUNCTION__, handle);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (drive_configuration_service.threshold_port_record_table[handle].threshold_record.port_info.port_type == FBE_PORT_TYPE_INVALID) {
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Handle points to an empty entry\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;

}

static fbe_status_t fbe_drive_configuration_control_add_port_record(fbe_packet_t *packet)
{

    fbe_payload_control_buffer_length_t                 len = 0;
    fbe_payload_ex_t *                                  payload = NULL;
    fbe_payload_control_operation_t *                   control_operation = NULL;
    fbe_drive_configuration_control_add_port_record_t * add_record = NULL;
    fbe_status_t                                        status = FBE_STATUS_GENERIC_FAILURE;

    drive_configuration_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s: entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);   

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if(len != sizeof(fbe_drive_configuration_control_add_port_record_t)){
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Invalid buffer_len \n", __FUNCTION__);

        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &add_record); 
    if(add_record == NULL){
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: fbe_payload_control_get_buffer fail\n", __FUNCTION__);

        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*we can't do that before the start update was done*/
    if (drive_configuration_service.update_started != FBE_TRUE) {
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: start_update was not called\n", __FUNCTION__);

        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_drive_configuration_add_port_record(&add_record->new_record, &add_record->handle);
    
    fbe_payload_control_set_status (control_operation, (status == FBE_STATUS_OK ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE) );
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

static fbe_status_t fbe_drive_configuration_add_port_record(fbe_drive_configuration_port_record_t *new_record, fbe_drive_configuration_handle_t *handle)
{
    fbe_u32_t               table_entry = 0, entry_index = 0;
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_bool_t              entry_identical = FBE_FALSE;
    
    if (handle == NULL || new_record == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_spinlock_lock (&drive_configuration_service.threshold_table_lock);

    /*any chance we already have an entry there which has an identical drive information?*/
    for (entry_index = 0; entry_index < MAX_PORT_ENTRIES; entry_index++) {
        /*check if the drive details are identical
        In this case we would just copy the threshold data(regardless if it changed or not), but all PDOs would not need a new handle*/
        entry_identical = fbe_equal_memory(&drive_configuration_service.threshold_port_record_table[entry_index].threshold_record.port_info,
                                           &new_record->port_info,
                                           sizeof(fbe_drive_configuration_port_info_t));

        if (entry_identical) {
            drive_configuration_service.threshold_port_record_table[entry_index].updated = FBE_TRUE;/*since the user sent it, its an entry we want to keep*/
            /*now we have to copy the threshold data (we copy everything to make it simpler)*/
            fbe_copy_memory(&drive_configuration_service.threshold_port_record_table[entry_index].threshold_record,
                            new_record,
                            sizeof(fbe_drive_configuration_port_record_t));

            *handle = entry_index;
            fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);
            return FBE_STATUS_OK;
        }

    }

    fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);

    /*search for a free place sine this record did not exist before*/
    status  = fbe_drive_configuration_get_free_port_table_entry(&table_entry);
    if (FBE_STATUS_INSUFFICIENT_RESOURCES == status) {
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Unable to get free entry in error threshold table\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*and copy the data to it*/
    fbe_spinlock_lock (&drive_configuration_service.threshold_table_lock);

    fbe_copy_memory (&drive_configuration_service.threshold_port_record_table[table_entry].threshold_record,
                     new_record,
                     sizeof (fbe_drive_configuration_port_record_t));

    fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);

    *handle = table_entry;

    /*we also mark this table entry as updated. This way, we know it's a record that it's a new/changed record*/
    drive_configuration_service.threshold_port_record_table[table_entry].updated = FBE_TRUE;

    return FBE_STATUS_OK;

}

static fbe_status_t fbe_drive_configuration_control_change_port_threshold(fbe_packet_t *packet)
{
    fbe_payload_control_buffer_length_t                         len = 0;
    fbe_payload_ex_t *                                          payload = NULL;
    fbe_payload_control_operation_t *                           control_operation = NULL;
    fbe_drive_configuration_control_change_port_threshold_t *   change_threshold = NULL;
    fbe_status_t                                                status = FBE_STATUS_GENERIC_FAILURE;

    drive_configuration_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s: entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);   

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if(len != sizeof(fbe_drive_configuration_control_change_port_threshold_t)){
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Invalid buffer_len \n", __FUNCTION__);

        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &change_threshold); 
    if(change_threshold == NULL){
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: fbe_payload_control_get_buffer fail\n", __FUNCTION__);

        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_drive_configuration_change_port_threshold(change_threshold);

    fbe_payload_control_set_status (control_operation, (status == FBE_STATUS_OK ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE) );
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

fbe_status_t fbe_drive_configuration_get_port_threshold_info (fbe_drive_configuration_handle_t handle, fbe_port_stat_t  *threshold_info)
{                                                    
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;

    status  = fbe_drive_configuration_validate_port_handle(handle);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    if (threshold_info == NULL ) {
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: data return pointer is NULL\n", __FUNCTION__, handle);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_spinlock_lock (&drive_configuration_service.threshold_table_lock);
    fbe_copy_memory(threshold_info,
                    &drive_configuration_service.threshold_port_record_table[handle].threshold_record.threshold_info,
                    sizeof (fbe_port_stat_t));

    fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);

    return FBE_STATUS_OK;

}

static fbe_status_t fbe_drive_configuration_control_get_port_handles_list(fbe_packet_t *packet)
{
    fbe_payload_control_buffer_length_t                     len = 0;
    fbe_payload_ex_t *                                      payload = NULL;
    fbe_payload_control_operation_t *                       control_operation = NULL;
    fbe_drive_configuration_control_get_handles_list_t *    get_handles = NULL;
    fbe_s32_t                                               entry = 0;
    fbe_u32_t                                               total_handles = 0;

    drive_configuration_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s: entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);   

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if(len != sizeof(fbe_drive_configuration_control_get_handles_list_t)){
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Invalid buffer_len \n", __FUNCTION__);

        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &get_handles); 
    if(get_handles == NULL){
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: fbe_payload_control_get_buffer fail\n", __FUNCTION__);

        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

   /*now let's populate the list*/
    fbe_spinlock_lock (&drive_configuration_service.threshold_table_lock);
    for (entry = 0; entry < MAX_PORT_ENTRIES; entry++) {
        if (drive_configuration_service.threshold_port_record_table[entry].threshold_record.port_info.port_type != FBE_PORT_TYPE_INVALID){
            get_handles->handles_list[total_handles] = (fbe_drive_configuration_handle_t)entry;
            total_handles ++;
        }
    }
    fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);

    get_handles->total_count = total_handles;

    fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;

}

static fbe_status_t fbe_drive_configuration_control_get_port_record(fbe_packet_t *packet)
{
    fbe_payload_control_buffer_length_t                     len = 0;
    fbe_payload_ex_t *                                      payload = NULL;
    fbe_payload_control_operation_t *                       control_operation = NULL;
    fbe_drive_configuration_control_get_port_record_t *     get_record = NULL;
    fbe_status_t                                            status = FBE_STATUS_GENERIC_FAILURE;
    
    drive_configuration_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s: entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);   

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if(len != sizeof(fbe_drive_configuration_control_get_port_record_t)){
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Invalid buffer_len \n", __FUNCTION__);

        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &get_record); 
    if(get_record == NULL){
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: fbe_payload_control_get_buffer fail\n", __FUNCTION__);

        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*first we make sure the handle is valid*/
    status = fbe_drive_configuration_validate_port_handle(get_record->handle);
    if (status != FBE_STATUS_OK) {
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Invalid handle: %d\n", __FUNCTION__, get_record->handle);

        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

   /*now let's populate the list*/
    fbe_spinlock_lock (&drive_configuration_service.threshold_table_lock);

    fbe_copy_memory(&get_record->record,
                    &drive_configuration_service.threshold_port_record_table[get_record->handle].threshold_record,
                    sizeof(fbe_drive_configuration_port_record_t));

    fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);
    
    fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

static fbe_status_t fbe_drive_configuration_get_free_port_table_entry(fbe_u32_t *entry_index)
{
    fbe_u32_t       entry = 0;

    fbe_spinlock_lock (&drive_configuration_service.threshold_table_lock);

    /*look for a free entry*/
    for (entry = 0; entry < MAX_PORT_ENTRIES; entry++) {
        if (drive_configuration_service.threshold_port_record_table[entry].threshold_record.port_info.port_type == FBE_PORT_TYPE_INVALID){
            fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);
            *entry_index = entry;
            return FBE_STATUS_OK;
        }
    }

    fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);

    /*if we got here it's bad, we have no places left*/
    return FBE_STATUS_INSUFFICIENT_RESOURCES;
}

static fbe_status_t fbe_drive_configuration_change_port_threshold(fbe_drive_configuration_control_change_port_threshold_t * change_threshold)
{
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;

    status  = fbe_drive_configuration_validate_port_handle(change_threshold->handle);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    /*and change the record*/
    fbe_spinlock_lock (&drive_configuration_service.threshold_table_lock);

    fbe_copy_memory (&drive_configuration_service.threshold_port_record_table[change_threshold->handle].threshold_record.threshold_info,
                     &change_threshold->new_threshold,
                     sizeof (fbe_port_stat_t));

    fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_drive_configuration_control_dieh_force_clear_update
 ****************************************************************
 * @brief
 *  Clears the DCS DIEH update state.  This should be used in
 *  cases where start_update is called but end_update is not, due
 *  to detecting errors.  This will clear the update state to allow
 *  subsequent updates to be processed.
 * 
 * @param packet - usurper packet
 * 
 * @return status - The status of the operation.
 *
 * @version
 *   10/04/2012:  Wayne Garrett - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_drive_configuration_control_dieh_force_clear_update(fbe_packet_t *packet)
{
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;

    drive_configuration_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s: entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);   

    fbe_spinlock_lock (&drive_configuration_service.threshold_table_lock);

    /* easiest way to clear is simply clear the start_update flag */
    drive_configuration_service.update_started = FBE_FALSE;

    fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);

    fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_drive_configuration_control_dieh_get_status
 ****************************************************************
 * @brief
 *  Gets the status of the DIEH table update.
 * 
 * @param packet - usurper packet.
 * 
 * @return status - The status of the operation.
 *
 * @version
 *   10/04/2012:  Wayne Garrett - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_drive_configuration_control_dieh_get_status(fbe_packet_t *packet)
{
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_u32_t                           len;
    fbe_bool_t *                        is_updating = NULL;

    drive_configuration_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s: entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);   



    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if(len != sizeof(fbe_bool_t)){
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Invalid buffer_len. actual:%d expected:%d\n", __FUNCTION__, len, sizeof(fbe_bool_t));

        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &is_updating); 
    if(is_updating == NULL){
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: fbe_payload_control_get_buffer fail\n", __FUNCTION__);

        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_spinlock_lock (&drive_configuration_service.threshold_table_lock);

    *is_updating = drive_configuration_service.update_started;

    fbe_spinlock_unlock (&drive_configuration_service.threshold_table_lock);

    fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}


/*!**************************************************************
 * @fn fbe_drive_configuration_init_queuing_table
 ****************************************************************
 * @brief
 *  This function initializes enhanced queuing timer table.
 * 
 * @param packet - None.
 * 
 * @return status - The status of the operation.
 *
 * @version
 *  08/22/2012  Lili Chen  - Created.
 *
 ****************************************************************/
static void fbe_drive_configuration_init_queuing_table(void)
{
    fbe_u32_t       entry = 0;

    fbe_spinlock_lock (&drive_configuration_service.queuing_table_lock);

    /* Clean the table */
    for (entry = 0; entry < MAX_QUEUING_TABLE_ENTRIES; entry++) {
        drive_configuration_service.queuing_table[entry].drive_info.drive_type = FBE_DRIVE_TYPE_INVALID;
        drive_configuration_service.queuing_table[entry].lpq_timer = ENHANCED_QUEUING_TIMER_MS_INVALID;
        drive_configuration_service.queuing_table[entry].hpq_timer = ENHANCED_QUEUING_TIMER_MS_INVALID;
    }

    fbe_spinlock_unlock (&drive_configuration_service.queuing_table_lock);
}

/*!**************************************************************
 * @fn fbe_drive_configuration_control_reset_queuing_table
 ****************************************************************
 * @brief
 *  This function processes usurper to reset enhanced queuing timer table.
 * 
 * @param packet - usurper packet.
 * 
 * @return status - The status of the operation.
 *
 * @version
 *  08/22/2012  Lili Chen  - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_drive_configuration_control_reset_queuing_table(fbe_packet_t *packet)
{
    fbe_payload_ex_t *                                  payload = NULL;
    fbe_payload_control_operation_t *                   control_operation = NULL;

    drive_configuration_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s: entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);   

    fbe_drive_configuration_init_queuing_table();
    
    fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_drive_configuration_control_add_queuing_table_entry
 ****************************************************************
 * @brief
 *  This function processes usurper to add enhanced queuing timer entry.
 * 
 * @param packet - usurper packet.
 * 
 * @return status - The status of the operation.
 *
 * @version
 *  08/22/2012  Lili Chen  - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_drive_configuration_control_add_queuing_table_entry(fbe_packet_t *packet)
{
    fbe_payload_control_buffer_length_t                 len = 0;
    fbe_payload_ex_t *                                  payload = NULL;
    fbe_payload_control_operation_t *                   control_operation = NULL;
    fbe_drive_configuration_control_add_queuing_entry_t *    add_entry = NULL;
    fbe_status_t                                        status = FBE_STATUS_GENERIC_FAILURE;

    drive_configuration_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s: entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);   

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if(len != sizeof(fbe_drive_configuration_control_add_queuing_entry_t)){
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Invalid buffer_len \n", __FUNCTION__);

        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &add_entry); 
    if(add_entry == NULL){
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: fbe_payload_control_get_buffer fail\n", __FUNCTION__);

        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (!((add_entry->queue_entry.drive_info.drive_type > FBE_DRIVE_TYPE_INVALID) && 
        (add_entry->queue_entry.drive_info.drive_type < FBE_DRIVE_TYPE_LAST))) {
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Invalid drive type %d\n", __FUNCTION__, add_entry->queue_entry.drive_info.drive_type);

        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_drive_configuration_add_queuing_table_entry(&add_entry->queue_entry);
    
    fbe_payload_control_set_status (control_operation, (status == FBE_STATUS_OK ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE) );
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_drive_configuration_add_queuing_table_entry
 ****************************************************************
 * @brief
 *  This function adds enhanced queuing timer entry.
 * 
 * @param new_entry - new entry.
 * 
 * @return status - The status of the operation.
 *
 * @version
 *  08/22/2012  Lili Chen  - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_drive_configuration_add_queuing_table_entry(fbe_drive_configuration_queuing_record_t * new_entry)
{
    fbe_u32_t               table_entry = 0, entry_index = 0;
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_bool_t              entry_identical = FBE_FALSE;

    if (new_entry == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_spinlock_lock (&drive_configuration_service.queuing_table_lock);

    for (entry_index = 0; entry_index < MAX_QUEUING_TABLE_ENTRIES; entry_index++) {
        /* Check if the drive details are identical */
        entry_identical = fbe_equal_memory(&drive_configuration_service.queuing_table[entry_index].drive_info,
                                           &new_entry->drive_info,
                                           sizeof(fbe_drive_configuration_drive_info_t));
        if (entry_identical) {
            table_entry = entry_index;
            break;
        }
    }

    if (!entry_identical) {
        status  = fbe_drive_configuration_get_free_queuing_table_entry(&table_entry);
        if (status != FBE_STATUS_OK) {
            drive_configuration_trace(FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s: Unable to get free entry in error threshold table\n", __FUNCTION__);
            fbe_spinlock_unlock(&drive_configuration_service.queuing_table_lock);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* Copy the data to it */
    fbe_copy_memory (&drive_configuration_service.queuing_table[table_entry],
                     new_entry,
                     sizeof(fbe_drive_configuration_queuing_record_t));

    fbe_spinlock_unlock (&drive_configuration_service.queuing_table_lock);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_drive_configuration_get_free_queuing_table_entry
 ****************************************************************
 * @brief
 *  This function tries to get a free enhanced queuing timer entry.
 * 
 * @param packet - usurper packet.
 * 
 * @return status - The status of the operation.
 *
 * @version
 *  08/22/2012  Lili Chen  - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_drive_configuration_get_free_queuing_table_entry(fbe_u32_t * entry_index)
{
    fbe_u32_t       entry = 0;

    /* Look for a free entry */
    for (entry = 0; entry < MAX_QUEUING_TABLE_ENTRIES; entry++) {
        if (drive_configuration_service.queuing_table[entry].drive_info.drive_type == FBE_DRIVE_TYPE_INVALID){
            *entry_index = entry;
            return FBE_STATUS_OK;
        }
    }

    /* If we got here it's bad, we have no places left */
    return FBE_STATUS_INSUFFICIENT_RESOURCES;
}

/*!**************************************************************
 * @fn fbe_drive_configuration_control_activate_queuing_table
 ****************************************************************
 * @brief
 *  This function processes usurper to activate enhanced queuing timer table.
 * 
 * @param packet - usurper packet.
 * 
 * @return status - The status of the operation.
 *
 * @version
 *  08/22/2012  Lili Chen  - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_drive_configuration_control_activate_queuing_table(fbe_packet_t *packet)
{
    fbe_payload_ex_t *                                  payload = NULL;
    fbe_payload_control_operation_t *                   control_operation = NULL;
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;

    drive_configuration_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s: entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);   

    status = fbe_drive_configuration_activate_queuing_table();
    
    fbe_payload_control_set_status (control_operation, (status == FBE_STATUS_OK ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE) );
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_drive_configuration_activate_queuing_table
 ****************************************************************
 * @brief
 *  This function sends usurper command to PDOs to activate timer setting.
 * 
 * @param packet - None.
 * 
 * @return status - The status of the operation.
 *
 * @version
 *  08/22/2012  Lili Chen  - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_drive_configuration_activate_queuing_table(void)
{
    fbe_topology_control_get_physical_drive_objects_t * pDriveObjects = NULL;
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t       object_idx = 0;
    fbe_packet_t *                      packet = NULL;
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_physical_drive_mgmt_set_enhanced_queuing_timer_t set_timer;

    pDriveObjects = (fbe_topology_control_get_physical_drive_objects_t *)fbe_memory_native_allocate(sizeof(fbe_topology_control_get_physical_drive_objects_t));
    if (pDriveObjects == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Get all PDOs to update */
    status = fbe_drive_configuration_get_physical_drive_objects(pDriveObjects);
    if (status != FBE_STATUS_OK) {
        fbe_memory_native_release(pDriveObjects);
        return status;
    }
     
    /* Allocate packet.*/
    packet = (fbe_packet_t *)fbe_memory_native_allocate(sizeof(fbe_packet_t));
    if (packet == NULL) {
        fbe_memory_native_release(pDriveObjects);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    set_timer.lpq_timer_ms = ENHANCED_QUEUING_TIMER_MS_INVALID;
    set_timer.hpq_timer_ms = ENHANCED_QUEUING_TIMER_MS_INVALID;

    fbe_transport_initialize_packet(packet);
    payload = fbe_transport_get_payload_ex (packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);
    fbe_payload_control_build_operation (control_operation,
                                         FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_ENHANCED_QUEUING_TIMER,
                                         &set_timer,
                                         sizeof(fbe_physical_drive_mgmt_set_enhanced_queuing_timer_t));

    /* For each of them, we send the update usurper command */
    for (object_idx = 0; object_idx < pDriveObjects->number_of_objects; object_idx++) {
        status = fbe_drive_configuration_send_queuing_change_usurper(pDriveObjects->pdo_list[object_idx], packet);
        if (status != FBE_STATUS_OK) {
            drive_configuration_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: usurper to PDO 0x%x failed\n", __FUNCTION__, pDriveObjects->pdo_list[object_idx]);
        }
    }
    
    fbe_transport_destroy_packet(packet);
    fbe_memory_native_release(packet);
    fbe_memory_native_release(pDriveObjects);
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_drive_configuration_send_queuing_change_usurper
 ****************************************************************
 * @brief
 *  This function sends usurper command to PDO.
 * 
 * @param packet - usurper packet.
 * 
 * @return status - The status of the operation.
 *
 * @version
 *  08/22/2012  Lili Chen  - Created.
 *
 ****************************************************************/
fbe_status_t fbe_drive_configuration_send_queuing_change_usurper(fbe_object_id_t object_id, fbe_packet_t *packet)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_semaphore_t sem;

    fbe_semaphore_init(&sem, 0, 1);

    /* Set packet address.*/
    fbe_transport_set_address(packet,
                              FBE_PACKAGE_ID_PHYSICAL,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              object_id); 
    
    /* Mark packet with the attribute, either traverse or not */
    fbe_transport_set_packet_attr(packet, FBE_PACKET_FLAG_NO_ATTRIB);
    fbe_transport_set_completion_function (packet, fbe_drive_configuration_send_table_change_usurper_completion,  &sem);

    status = fbe_service_manager_send_control_packet(packet);
    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
        /* Only trace when it is an error we do not expect.*/
         drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s error in sending packet, status:%d\n", __FUNCTION__, status); 
         fbe_semaphore_destroy(&sem);
         return status;
    }

    /* Wait for the callback.  Our completion function is always called.*/
    fbe_semaphore_wait(&sem, NULL);

    fbe_semaphore_destroy(&sem);

    return status;
}

/*!**************************************************************
 * @fn fbe_drive_configuration_get_queuing_info
 ****************************************************************
 * @brief
 *  This function gets timers from enhanced queuing timer table.
 * 
 * @param registration_info - drive info.
 * @param lpq_timer - pointer to lpq timer.
 * @param hpq_timer - pointer to hpq timer.
 * 
 * @return status - The status of the operation.
 *
 * @version
 *  08/22/2012  Lili Chen  - Created.
 *
 ****************************************************************/
fbe_status_t fbe_drive_configuration_get_queuing_info(fbe_drive_configuration_registration_info_t *registration_info,
                                                      fbe_u32_t *lpq_timer, fbe_u32_t *hpq_timer)
{
    fbe_s32_t                               table_counter = 0;
    fbe_drive_configuration_drive_info_t *  table_entry;
    fbe_u32_t                               max_matches = 0;
    fbe_u32_t                               current_matches = 0;
    fbe_bool_t                              match_found = FBE_FALSE;

    *lpq_timer = ENHANCED_QUEUING_TIMER_MS_INVALID;
    *hpq_timer = ENHANCED_QUEUING_TIMER_MS_INVALID;
    
    if (!service_initialized){
        drive_configuration_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Service uninitialized\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_spinlock_lock (&drive_configuration_service.queuing_table_lock);

    /*let's find a match for the parameters that drive has
    the main concept is to do a match by this hirarchy: CLASS->Vendor->Product ID->REV->Serail number*/
    for (table_counter = 0; table_counter < MAX_QUEUING_TABLE_ENTRIES; table_counter++) {
        table_entry = &drive_configuration_service.queuing_table[table_counter].drive_info;
        if (table_entry->drive_type == FBE_DRIVE_TYPE_INVALID) {
            break;
        }

        current_matches = 0;/*we always start with 0*/

        if (table_entry->drive_type == registration_info->drive_type) {
            current_matches ++;

            if (table_entry->drive_vendor == registration_info->drive_vendor) {
                current_matches ++;
            }else if (registration_info->drive_vendor != FBE_DRIVE_VENDOR_INVALID && table_entry->drive_vendor != FBE_DRIVE_VENDOR_INVALID) {
                continue;
            }

            if (fbe_drive_configuration_string_match(table_entry->part_number, registration_info->part_number, FBE_SCSI_INQUIRY_TLA_SIZE)) {
                current_matches ++;
            }else if (*table_entry->part_number != 0 && *registration_info->part_number != 0) {
                continue;
            }

            if ( fbe_drive_configuration_string_match(table_entry->fw_revision, registration_info->fw_revision, FBE_SCSI_INQUIRY_REVISION_SIZE)) {
                current_matches ++;
            }else if (*table_entry->fw_revision != 0 && *registration_info->fw_revision != 0) {
                continue;
            }

            if (fbe_drive_configuration_sn_is_in_range(registration_info->serial_number, table_entry)) {
                current_matches ++;
            }else if (*registration_info->serial_number != 0 && *table_entry->serial_number_start != 0) {
                continue;
            }
        }

        /*we want the entry in the table that had the most matches to our drive*/
        if (current_matches > max_matches) {
            *lpq_timer = drive_configuration_service.queuing_table[table_counter].lpq_timer;
            *hpq_timer = drive_configuration_service.queuing_table[table_counter].hpq_timer;
            max_matches = current_matches;
            match_found = FBE_TRUE;
        }
    }
    fbe_spinlock_unlock (&drive_configuration_service.queuing_table_lock);

    if (!match_found){
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}
