#include <stdlib.h>  // needed for atol

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_scheduler.h"
#include "fbe_diplex.h"
#include "fbe_transport_memory.h"
#include "base_object_private.h"
#include "base_physical_drive_private.h"
#include "base_physical_drive_init.h"
#include "fbe/fbe_stat_api.h"
#include "fbe/fbe_drive_configuration_interface.h"
#include "fbe_scsi.h"
#include "fbe/fbe_event_log_api.h"
#include "PhysicalPackageMessages.h"
#include "fbe_private_space_layout.h"

#include <stdlib.h>
#include <stdio.h>

/* Class methods forward declaration */
fbe_status_t base_physical_drive_load(void);
fbe_status_t base_physical_drive_unload(void);



/* Export class methods  */
fbe_class_methods_t fbe_base_physical_drive_class_methods = {FBE_CLASS_ID_BASE_PHYSICAL_DRIVE,
                                                    base_physical_drive_load,
                                                    base_physical_drive_unload,
                                                    fbe_base_physical_drive_create_object,
                                                    fbe_base_physical_drive_destroy_object,
                                                    fbe_base_physical_drive_control_entry,
                                                    fbe_base_physical_drive_event_entry,
                                                    fbe_base_physical_drive_io_entry,
                                                    fbe_base_physical_drive_monitor_entry};


/* GLOBALS */
typedef struct fbe_pdo_4k_capacity_override_table_entry_s
{
    fbe_u32_t capacity_gb;
    fbe_block_count_t  blocks;
} fbe_pdo_4k_capacity_override_table_entry_t;


/* Forward declaration */

static fbe_status_t fbe_base_physical_drive_map_flags_to_death_reason(fbe_payload_cdb_fis_error_flags_t error_flags,
                                                                      fbe_object_death_reason_t *death_reason);

static void fbe_base_physical_drive_update_traffic_load(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_process_incoming_io_duing_hibernate(fbe_base_physical_drive_t *base_physical_p);

fbe_status_t 
base_physical_drive_load(void)
{
    fbe_topology_class_trace(FBE_CLASS_ID_BASE_PHYSICAL_DRIVE,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);

    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_base_physical_drive_t) < FBE_MEMORY_CHUNK_SIZE);

    fbe_topology_class_trace(FBE_CLASS_ID_BASE_PHYSICAL_DRIVE,
                             FBE_TRACE_LEVEL_DEBUG_LOW,    
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "%s: object size %llu\n", __FUNCTION__, (unsigned long long)sizeof(fbe_base_physical_drive_t));

    /* FBE_ASSERT_AT_COMPILE_TIME((sizeof(fbe_physical_drive_attributes_t) * 8) > FBE_PHYSICAL_DRIVE_FLAG_LAST); */

    return fbe_base_physical_drive_monitor_load_verify();
}

fbe_status_t 
base_physical_drive_unload(void)
{
    fbe_topology_class_trace(FBE_CLASS_ID_BASE_PHYSICAL_DRIVE,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_physical_drive_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle)
{
    fbe_base_physical_drive_t * base_physical_drive;
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;

    fbe_topology_class_trace(FBE_CLASS_ID_BASE_PHYSICAL_DRIVE,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);

    /* Call parent constructor */
    status = fbe_base_discovering_create_object(packet, object_handle);
    if(status != FBE_STATUS_OK){
        return status;
    }

    /* Set class id */
    base_physical_drive = (fbe_base_physical_drive_t *)fbe_base_handle_to_pointer(*object_handle);
    fbe_zero_memory(&base_physical_drive->drive_info, sizeof(fbe_base_physical_drive_info_t));

    fbe_base_object_set_class_id((fbe_base_object_t *) base_physical_drive, FBE_CLASS_ID_BASE_PHYSICAL_DRIVE);  

    base_physical_drive->block_count = 0;
    base_physical_drive->block_size = 0;
    fbe_zero_memory(&base_physical_drive->address, sizeof(fbe_address_t));
    base_physical_drive->generation_code = 0;
    base_physical_drive->element_type = FBE_PAYLOAD_DISCOVERY_ELEMENT_TYPE_INVALID;

    fbe_block_transport_server_init(&base_physical_drive->block_transport_server);

    /* DIEH */
    fbe_stat_drive_error_init(&base_physical_drive->drive_error);
    base_physical_drive->drive_configuration_handle = FBE_DRIVE_CONFIGURATION_SERVICE_INVALID_HANLDLE;
    fbe_base_physical_drive_clear_dieh_state(base_physical_drive);  

    base_physical_drive->logical_error_action = FBE_PDO_ACTION_KILL_ON_MULTI_BIT_CRC;  /* default is kill on multibit.  DMO responsible for changing */

    base_physical_drive->cached_path_attr = 0;
    
    /* Set the invalid value for the position of disk. */
    base_physical_drive->port_number      = FBE_PORT_NUMBER_INVALID;
    base_physical_drive->enclosure_number = FBE_ENCLOSURE_NUMBER_INVALID;
    base_physical_drive->slot_number      = FBE_SLOT_NUMBER_INVALID;
    base_physical_drive->slot_reset = 0;
    /*set IO load and priority related parameters*/
    base_physical_drive->highest_outstanding_ios = 0;
    base_physical_drive->total_io_priority = 0;
    base_physical_drive->total_outstanding_ios = 0;
    base_physical_drive->previous_traffic_priority = FBE_TRAFFIC_PRIORITY_VERY_LOW;
    
    /* Enhanced queuing is disabled by default */
    fbe_base_physical_drive_set_enhanced_queuing_supported(base_physical_drive, FBE_FALSE);
    /*set the default time it takes the drive to become ready*/
    base_physical_drive->drive_info.max_becoming_ready_latency_in_seconds = FBE_MAX_BECOMING_READY_LATENCY_DEFAULT;

    fbe_base_physical_drive_reset_statistics(base_physical_drive);

    /*spin up value to at least one since the path that increment it works only if the drive was previously spun dowm
    and usualy at boot time the drive is already spinning*/
    base_physical_drive->drive_info.spinup_count = 1;
    
    /* Disk Collect. */
    base_physical_drive->dc_lba = 0;
    fbe_zero_memory(&base_physical_drive->dc, sizeof(fbe_base_physical_drive_dc_data_t)); 
    
    payload = fbe_transport_get_payload_ex(packet);
    base_pdo_payload_transaction_flag_init(&payload->physical_drive_transaction);   /* why init this? it's is not an IO packet*/

    base_physical_drive->logical_state = FBE_BLOCK_TRANSPORT_LOGICAL_STATE_UNINITALIZED;
    base_physical_drive->activate_count = 0;

    base_physical_drive->service_time_limit_ms = fbe_dcs_get_svc_time_limit();  /*todo: leaving per PDO limit for now since I don't want to break any fbe_tests */
    base_physical_drive->quiesce_start_time_ms = 0;
    base_physical_drive->first_port_timeout = 0;
    base_physical_drive->first_port_timeout_successful_io_count = 0;
 
    fbe_base_physical_drive_init_local_state(base_physical_drive);
    fbe_base_pdo_init_flags(base_physical_drive);    

    /* Initialize as maintenance needed.  Bits will be cleared as PDO verifies correct settings */
    fbe_pdo_maintenance_mode_set_flag(&base_physical_drive->maintenance_mode_bitmap, FBE_PDO_MAINTENANCE_FLAG_EQ_NOT_SUPPORTED);
    fbe_pdo_maintenance_mode_set_flag(&base_physical_drive->maintenance_mode_bitmap, FBE_PDO_MAINTENANCE_FLAG_SANITIZE_STATE);
#ifdef FBE_HANDLE_UNKNOWN_TLA_SUFFIX
    fbe_pdo_maintenance_mode_set_flag(&base_physical_drive->maintenance_mode_bitmap, FBE_PDO_MAINTENANCE_FLAG_INVALID_IDENTITY);
#endif

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_physical_drive_destroy_object( fbe_object_handle_t object_handle)
{
    fbe_status_t status;
    fbe_base_physical_drive_t * base_physical_drive;

    fbe_topology_class_trace(FBE_CLASS_ID_BASE_PHYSICAL_DRIVE,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);
    
    base_physical_drive = (fbe_base_physical_drive_t *)fbe_base_handle_to_pointer(object_handle);


    /* Cleanup */
    fbe_block_transport_server_destroy(&base_physical_drive->block_transport_server);

    /* Call parent destructor */
    status = fbe_base_discovering_destroy_object(object_handle);
    return status;
}


fbe_status_t 
fbe_base_physical_drive_set_capacity(fbe_base_physical_drive_t * base_physical_drive, fbe_lba_t block_count, fbe_block_size_t  block_size)
{
    if (fbe_base_physical_drive_set_block_size(base_physical_drive, block_size) == FBE_STATUS_OK) {
        fbe_base_physical_drive_set_block_count(base_physical_drive, block_count);
    }
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_physical_drive_set_block_size(fbe_base_physical_drive_t * base_physical_drive, fbe_block_size_t  block_size)
{
    base_physical_drive->block_size = block_size;
    return FBE_STATUS_OK;
}


fbe_status_t 
fbe_base_physical_drive_get_capacity(fbe_base_physical_drive_t * base_physical_drive, fbe_lba_t * block_count, fbe_block_size_t * block_size)
{
    *block_count = base_physical_drive->block_count;
    *block_size = base_physical_drive->block_size;

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_physical_drive_get_exported_capacity(fbe_base_physical_drive_t * base_physical_drive, fbe_block_size_t client_block_size, fbe_lba_t * capacity_in_client_blocks_p)
{
    /* Check that block size has been set.  Done by read_capacity condition 
     */
    if (base_physical_drive->block_size == 0)   
    {
        fbe_topology_class_trace(FBE_CLASS_ID_BASE_PHYSICAL_DRIVE,
                                 FBE_TRACE_LEVEL_INFO, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "BPDO get_exported_capacity obj:0x%x block size not set\n",  
                                 base_physical_drive->base_discovering.base_discovered.base_object.object_id);
        *capacity_in_client_blocks_p = 0;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Only this block size is supported.
     */
    if (client_block_size != FBE_BE_BYTES_PER_BLOCK) {
        fbe_topology_class_trace(FBE_CLASS_ID_BASE_PHYSICAL_DRIVE,
                                 FBE_TRACE_LEVEL_ERROR, 
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "BPDO get_exported_capacity obj:0x%x client block size %d unsupported\n",  
                                 base_physical_drive->base_discovering.base_discovered.base_object.object_id,
                                 client_block_size);
        *capacity_in_client_blocks_p = 0;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (base_physical_drive->block_size == client_block_size) {
        *capacity_in_client_blocks_p = base_physical_drive->block_count;
    }
    else if ((base_physical_drive->block_size > client_block_size)        &&
             ((base_physical_drive->block_size % client_block_size) == 0)    ) {
        *capacity_in_client_blocks_p = base_physical_drive->block_count * (base_physical_drive->block_size / client_block_size);
    } 
    else {
        fbe_topology_class_trace(FBE_CLASS_ID_BASE_PHYSICAL_DRIVE,
                                 FBE_TRACE_LEVEL_ERROR, 
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "BPDO get_exported_capacity obj:0x%x physical block size %d unsupported\n",
                                 base_physical_drive->base_discovering.base_discovered.base_object.object_id,
                                 base_physical_drive->block_size);
        *capacity_in_client_blocks_p = 0;
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_physical_drive_get_optimal_block_alignment(fbe_base_physical_drive_t * base_physical_drive, fbe_block_size_t client_block_size, fbe_block_size_t * optimal_block_alignment_p)
{
    /* Only this block size is supported.
     */
    if (client_block_size != 520) {
        fbe_topology_class_trace(FBE_CLASS_ID_BASE_PHYSICAL_DRIVE,
                                 FBE_TRACE_LEVEL_ERROR, 
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s obj: 0x%x unsupported client block size: %d\n", 
                                 __FUNCTION__, 
                                 base_physical_drive->base_discovering.base_discovered.base_object.object_id,
                                 client_block_size);
        *optimal_block_alignment_p = 0;
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (base_physical_drive->block_size == client_block_size) {
        *optimal_block_alignment_p = 1;
    }
    else if ((base_physical_drive->block_size > client_block_size)        &&
             ((base_physical_drive->block_size % client_block_size) == 0)    ) {
        *optimal_block_alignment_p = base_physical_drive->block_size / client_block_size;
    } else {
        fbe_topology_class_trace(FBE_CLASS_ID_BASE_PHYSICAL_DRIVE,
                                 FBE_TRACE_LEVEL_ERROR, 
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s obj: 0x%x unsupported physical block size: %d\n", 
                                 __FUNCTION__, 
                                 base_physical_drive->base_discovering.base_discovered.base_object.object_id,
                                 base_physical_drive->block_size);
        *optimal_block_alignment_p = 0;
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_physical_drive_set_block_count(fbe_base_physical_drive_t * base_physical_drive, fbe_lba_t physical_drive_block_count)
{
    fbe_status_t    status;
    fbe_lba_t       exported_capacity;

    base_physical_drive->block_count = physical_drive_block_count;
    status = fbe_base_physical_drive_get_exported_capacity(base_physical_drive, FBE_BE_BYTES_PER_BLOCK,
                                                           &exported_capacity);
    if (status == FBE_STATUS_OK) {
        fbe_block_transport_server_set_capacity(&base_physical_drive->block_transport_server, exported_capacity);
    }
    return status;
}

fbe_status_t 
fbe_base_physical_drive_get_adderess(fbe_base_physical_drive_t * base_physical_drive, fbe_address_t * address)
{
    *address = base_physical_drive->address;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_physical_drive_set_block_transport_const(fbe_base_physical_drive_t * base_physical_drive,
                                                  fbe_block_transport_const_t * block_transport_const)
{
    fbe_status_t status;

    status = fbe_block_transport_server_set_block_transport_const(&base_physical_drive->block_transport_server,
                                                                  block_transport_const,
                                                                  base_physical_drive);

    return status;
}

fbe_status_t 
fbe_base_physical_drive_block_transport_enable_tags(fbe_base_physical_drive_t * base_physical_drive)
{
    fbe_status_t status;

    status = fbe_block_transport_server_enable_tags(&base_physical_drive->block_transport_server);

    return status;
}

fbe_status_t 
fbe_base_physical_drive_set_outstanding_io_max(fbe_base_physical_drive_t * base_physical_drive,
                                                  fbe_u32_t outstanding_io_max)
{
    fbe_status_t status;

    status = fbe_block_transport_server_set_outstanding_io_max(&base_physical_drive->block_transport_server,
                                                                    outstanding_io_max);

    return status;
}

fbe_status_t 
fbe_base_physical_drive_set_stack_limit(fbe_base_physical_drive_t * base_physical_drive)
{
    fbe_status_t status;

    status = fbe_block_transport_server_set_stack_limit(&base_physical_drive->block_transport_server);

    return status;
}

fbe_status_t 
fbe_base_physical_drive_get_outstanding_io_max(fbe_base_physical_drive_t * base_physical_drive,
                                                  fbe_u32_t *outstanding_io_max)
{
    fbe_status_t status;

    status = fbe_block_transport_server_get_outstanding_io_max(&base_physical_drive->block_transport_server,
                                                                    outstanding_io_max);

    return status;
}

void pdo_psl_safety(fbe_base_physical_drive_t * physical_drive_p, fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_block_operation_t *block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    fbe_payload_block_operation_opcode_t block_operation_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_private_space_layout_region_t psl_region;
    fbe_u32_t psl_index = 0;
    fbe_u64_t psl_start_lba = 0;
    fbe_u64_t psl_end_lba = 0;
    fbe_u64_t packet_start_lba = 0;
    fbe_u64_t packet_end_lba = 0;

    if(block_operation_p == NULL) {
        return;
    }
    
    fbe_payload_block_get_opcode(block_operation_p, &block_operation_opcode);

    if( (physical_drive_p->port_number == 0) &&
        (physical_drive_p->enclosure_number == 0) &&
        (
            (block_operation_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE) ||
            (block_operation_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY) ||
            (block_operation_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME)
        ) )
    {
        packet_start_lba    = block_operation_p->lba;
        packet_end_lba      = block_operation_p->lba + block_operation_p->block_count - 1;
        
        /*fbe_base_object_trace((fbe_base_object_t*)physical_drive_p, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Caught write packet to 0_0_%d\n", 
                              __FUNCTION__, physical_drive_p->slot_number); */
                                      
        for(psl_index = 0; psl_index < FBE_PRIVATE_SPACE_LAYOUT_MAX_REGIONS; psl_index++) {
            status = fbe_private_space_layout_get_region_by_index(psl_index, &psl_region);
            if(status != FBE_STATUS_OK) {
                break;
            }
            else if(psl_region.region_id == FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_INVALID) {
                break;
            }
            else if(psl_region.region_type != FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_UNUSED) {
                continue;
            }
            else if(! fbe_private_space_layout_does_region_use_fru(psl_region.region_id, physical_drive_p->slot_number)) {
                continue;
            }

            psl_start_lba   = psl_region.starting_block_address;
            psl_end_lba     = psl_region.starting_block_address + psl_region.size_in_blocks - 1;

            if(
               /* Packet LBA range starts inside the PSL LBA range */
               ((packet_start_lba >= psl_start_lba)   && (packet_start_lba <= psl_end_lba)) ||
               /* Packet LBA range ends inside the PSL LBA range */
               ((packet_end_lba >= psl_start_lba)     && (packet_end_lba <= psl_end_lba)) ||
               /* Packet LBA range spans the entire PSL LBA range */
               (((packet_start_lba <= psl_start_lba)) && ((packet_end_lba >= psl_end_lba)))
               )
            {
                fbe_base_object_trace((fbe_base_object_t*)physical_drive_p, 
                                      FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s Overlap LBA = 0x%08llX Size = 0x%08llX Region ID = %d\n", 
                                      __FUNCTION__, block_operation_p->lba, block_operation_p->block_count, psl_region.region_id);
            }
        }
    }
}

#define FBE_PDO_ENFORCE_PSL_SAFETY 0

fbe_status_t 
fbe_base_physical_drive_bouncer_entry(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status;
#if 0
    fbe_payload_ex_t * payload = NULL;
    fbe_object_id_t my_object_id;
    fbe_time_t total_io_op_time = 0;
    
#if FBE_PDO_ENFORCE_PSL_SAFETY
    pdo_psl_safety(base_physical_drive, packet);
#endif

    payload = fbe_transport_get_payload_ex(packet);

    fbe_base_object_get_object_id((fbe_base_object_t *)base_physical_drive,&my_object_id);

    fbe_payload_ex_set_pdo_object_id(payload, my_object_id);
#endif

    /* The packet may be enqueued */
    fbe_transport_set_cancel_function(packet, fbe_base_object_packet_cancel_function, (fbe_base_object_t *)base_physical_drive);

    /*we need to keep track of the IOs that go through so we can tell how loaded the drive is.*/
    fbe_base_physical_drive_update_traffic_load(base_physical_drive, packet);

    /* Set the time stamp of the I/O in the packet when IO first enters PDO, before having a chance to be queued.  
       We will use this to determine service time. 
    */
    fbe_transport_set_physical_drive_io_stamp(packet, fbe_get_time());

    /*and sent it to the transport bouncer*/
    status = fbe_block_transport_server_bouncer_entry(&base_physical_drive->block_transport_server,
                                                        packet,
                                                        base_physical_drive);
    return status;
}

fbe_status_t 
fbe_base_physical_drive_get_dev_info(fbe_base_physical_drive_t * base_physical_drive, fbe_physical_drive_information_t * drive_information)
{
    fbe_base_physical_drive_info_t * info_ptr = & base_physical_drive->drive_info;
    fbe_u8_t description_buff[FBE_SCSI_DESCRIPTION_BUFF_SIZE+1];
    fbe_lba_t cap64 = 0;
    
    fbe_zero_memory(drive_information, sizeof(fbe_physical_drive_information_t));
    fbe_zero_memory(description_buff, FBE_SCSI_DESCRIPTION_BUFF_SIZE+1);
 
    /* Fill all the information in the structure. */
    fbe_copy_memory (drive_information->vendor_id, info_ptr->vendor_id, FBE_SCSI_INQUIRY_VENDOR_ID_SIZE); 
    fbe_copy_memory (drive_information->product_id, info_ptr->product_id, FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE);
    fbe_copy_memory (drive_information->product_rev, info_ptr->revision, info_ptr->product_rev_len);
    fbe_copy_memory (drive_information->product_serial_num, info_ptr->serial_number, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE);
    fbe_copy_memory (drive_information->tla_part_number, info_ptr->tla, FBE_SCSI_INQUIRY_TLA_SIZE);
    fbe_copy_memory (drive_information->dg_part_number_ascii, info_ptr->part_number, FBE_SCSI_INQUIRY_PART_NUMBER_SIZE);
    fbe_copy_memory (drive_information->bridge_hw_rev, info_ptr->bridge_hw_rev, FBE_SCSI_INQUIRY_VPD_BRIDGE_HW_REV_SIZE);

    drive_information->drive_vendor_id = info_ptr->drive_vendor_id; 
    drive_information->product_rev_len = info_ptr->product_rev_len;

    fbe_base_physical_drive_get_exported_capacity(base_physical_drive, FBE_BE_BYTES_PER_BLOCK, &drive_information->gross_capacity); /* capacity in 520 blocks */    
    drive_information->speed_capability = info_ptr->speed_capability; 
    drive_information->drive_price_class = info_ptr->drive_price_class;
    drive_information->drive_parameter_flags &= (~FBE_PDO_FLAG_MLC_FLASH_DRIVE);
    drive_information->drive_parameter_flags |= (info_ptr->drive_parameter_flags & FBE_PDO_FLAG_MLC_FLASH_DRIVE);
    drive_information->drive_parameter_flags &= (~FBE_PDO_FLAG_FLASH_DRIVE_WRITES_PER_DAY);
    drive_information->drive_parameter_flags |= (info_ptr->drive_parameter_flags & FBE_PDO_FLAG_FLASH_DRIVE_WRITES_PER_DAY);
    drive_information->drive_parameter_flags &= (~FBE_PDO_FLAG_USES_DESCRIPTOR_FORMAT);
    drive_information->drive_parameter_flags |= (info_ptr->drive_parameter_flags & FBE_PDO_FLAG_USES_DESCRIPTOR_FORMAT);
    drive_information->drive_parameter_flags &= (~FBE_PDO_FLAG_SUPPORTS_UNMAP);
    drive_information->drive_parameter_flags |= (info_ptr->drive_parameter_flags & FBE_PDO_FLAG_SUPPORTS_UNMAP);
    drive_information->drive_parameter_flags &= (~FBE_PDO_FLAG_SUPPORTS_WS_UNMAP);
    drive_information->drive_parameter_flags |= (info_ptr->drive_parameter_flags & FBE_PDO_FLAG_SUPPORTS_WS_UNMAP);

    drive_information->block_count = base_physical_drive->block_count;
    drive_information->block_size = base_physical_drive->block_size;
    drive_information->drive_qdepth = info_ptr->drive_qdepth;
    drive_information->drive_RPM = info_ptr->drive_RPM;    
    drive_information->spin_down_qualified = (info_ptr->powersave_attr & FBE_BASE_PHYSICAL_DRIVE_POWERSAVE_SUPPORTED)? FBE_TRUE : FBE_FALSE;
    base_physical_drive_get_up_time(base_physical_drive, &drive_information->spin_up_time_in_minutes);
    base_physical_drive_get_down_time(base_physical_drive, &drive_information->stand_by_time_in_minutes);
    drive_information->spin_up_count = info_ptr->spinup_count;   

    fbe_base_physical_drive_get_port_number(base_physical_drive, &drive_information->port_number);
    fbe_base_physical_drive_get_enclosure_number(base_physical_drive, &drive_information->enclosure_number);
    fbe_base_physical_drive_get_slot_number(base_physical_drive, &drive_information->slot_number);

    drive_information->enhanced_queuing_supported = fbe_base_physical_drive_is_enhanced_queuing_supported(base_physical_drive);
    drive_information->drive_type = fbe_base_physical_drive_get_drive_type(base_physical_drive);

    /* Setup dynamic drive description for Generic Clariion drive.
     */
    cap64 = drive_information->gross_capacity;
    /* Currently, this calculation won't cause overflow since we 
     * don't expect to support drives larger than 10 TB.
     * -- Note from generic drive capacity support
     */
    //cap64 *= (base_physical_drive->block_size);
    cap64*= 520;
    cap64 /= 1000*1000*1000; 
    fbe_sprintf (description_buff , FBE_SCSI_DESCRIPTION_BUFF_SIZE, "%llu GB GEN Disk \n", (unsigned long long)cap64);
    fbe_copy_memory (drive_information->drive_description_buff, description_buff, FBE_SCSI_DESCRIPTION_BUFF_SIZE);
    


    return FBE_STATUS_OK;
}

/*!*************************************************************************
* @fn fbe_base_physical_drive_get_drive_type()
***************************************************************************
*
* @brief      This function returns the FBE drive type
*
* @param      base_physical_drive - ptr to base PDO.
*
* @return     drive type
*
* @author   08/25/2015 Wayne Garrett  - Created.
*
***************************************************************************/
fbe_drive_type_t fbe_base_physical_drive_get_drive_type(fbe_base_physical_drive_t *base_physical_drive)
{
    fbe_drive_type_t drive_type = FBE_DRIVE_TYPE_INVALID;
    fbe_base_physical_drive_info_t * info_ptr = & base_physical_drive->drive_info;
    fbe_u32_t drive_writes_per_day;

    /*get the type of the drive*/
    switch (((fbe_base_object_t *)base_physical_drive)->class_id) {
        case FBE_CLASS_ID_SAS_PHYSICAL_DRIVE:
            drive_type = FBE_DRIVE_TYPE_SAS;
            if (info_ptr->drive_RPM<= FBE_SAS_NL_RPM_LIMIT)
            {
                /* If spindle speed is less than 7200, it is a NL SAS drive */
                drive_type = FBE_DRIVE_TYPE_SAS_NL;
            }
            break;
        case FBE_CLASS_ID_SATA_PHYSICAL_DRIVE:
            drive_type = FBE_DRIVE_TYPE_SATA;
            break;
        case FBE_CLASS_ID_SAS_FLASH_DRIVE:
            /* NOTE, only HE drives have different drive types (SATA paddle card vs SAS).  This was the existing behavior.
            Since this is not a platform refresh, and inorder to avoid breaking anything, the remaing flash types will
            continue to use only one drive type (i.e  FBE_DRIVE_TYPE_SAS...) 
            */
            drive_writes_per_day = (fbe_u32_t)(info_ptr->drive_parameter_flags & FBE_PDO_FLAG_FLASH_DRIVE_WRITES_PER_DAY);
            if (fbe_equal_memory(info_ptr->vendor_id, "SATA-", 5))   /*if has a SATA paddlecard */
            {
                if ( ((info_ptr->drive_parameter_flags & FBE_PDO_FLAG_MLC_FLASH_DRIVE) > 0) &&       
                        (drive_writes_per_day < FBE_MLC_HIGH_ENDURANCE_LIMIT) )
                {
                    // If the wpd is greater than or equal to the Medium Endurance Limit (10), set to ME SSD type.
                    if (drive_writes_per_day >= FBE_MLC_MEDIUM_ENDURANCE_LIMIT)
                    {
                        drive_type = FBE_DRIVE_TYPE_SAS_FLASH_ME;   /* note, using SAS type even though it has a SATA paddlecard*/
                    }
                    else 
                    {
                        // If the wpd is greater than or equal to the Low Endurance Limit (3), set to LE SSD type. 
                        if (drive_writes_per_day >= FBE_MLC_LOW_ENDURANCE_LIMIT)
                        {
                            drive_type = FBE_DRIVE_TYPE_SAS_FLASH_LE;  /* note, using SAS type even though it has a SATA paddlecard*/
                        }
                        else
                        {
                            // The wpd is less than the Lower Endurance, set it to RI SSD type.
                            drive_type = FBE_DRIVE_TYPE_SAS_FLASH_RI;  /* note, using SAS type even though it has a SATA paddlecard*/
                        }
                    }
                }
                else
                {
                    drive_type = FBE_DRIVE_TYPE_SATA_FLASH_HE;
                }
            }
            else /* SAS SSD */
            {
                // If it is a Flash drive and the writes per day (wpd) is less than the High Endurance Limit (20), then
                // need to check for different flash drives such as ME, LE, and RI type.  Otherwise, it set to HE SSD type. 
                if ( ((info_ptr->drive_parameter_flags & FBE_PDO_FLAG_MLC_FLASH_DRIVE) > 0) && 
                     (drive_writes_per_day < FBE_MLC_HIGH_ENDURANCE_LIMIT) )
                {
                    // If the wpd is greater than or equal to the Medium Endurance Limit (10), set to ME SSD type.
                    if (drive_writes_per_day >= FBE_MLC_MEDIUM_ENDURANCE_LIMIT)
                    {
                        drive_type = FBE_DRIVE_TYPE_SAS_FLASH_ME;
                    }
                    else 
                    {
                        // If the wpd is greater than or equal to the Low Endurance Limit (3), set to LE SSD type. 
                        if (drive_writes_per_day >= FBE_MLC_LOW_ENDURANCE_LIMIT)
                        {
                            drive_type = FBE_DRIVE_TYPE_SAS_FLASH_LE;
                        }
                        else
                        {
                            // The wpd is less than the Lower Endurance, set it to RI SSD type.
                            drive_type = FBE_DRIVE_TYPE_SAS_FLASH_RI;
                        }
                    }
                }
                else
                {
                    drive_type = FBE_DRIVE_TYPE_SAS_FLASH_HE;
                }
            }
            break;
        case FBE_CLASS_ID_SATA_FLASH_DRIVE:
            drive_type = FBE_DRIVE_TYPE_SATA_FLASH_HE;
            break;
        case FBE_CLASS_ID_SATA_PADDLECARD_DRIVE:
            drive_type = FBE_DRIVE_TYPE_SATA_PADDLECARD;   
            break;
        default:
            drive_type = FBE_DRIVE_TYPE_INVALID;
            break;
    }

    return drive_type;
}

fbe_status_t 
fbe_base_physical_drive_get_drive_info(fbe_base_physical_drive_t * base_physical_drive,
                                                  fbe_base_physical_drive_info_t ** drive_info)
{
    *drive_info = &base_physical_drive->drive_info;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_physical_drive_set_default_timeout(fbe_base_physical_drive_t * base_physical_drive, fbe_time_t timeout)
{
    if (timeout <= 0 || base_physical_drive == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    base_physical_drive->default_timeout = timeout;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_physical_drive_get_default_timeout(fbe_base_physical_drive_t * base_physical_drive, fbe_time_t *timeout)
{
    if (timeout == NULL || base_physical_drive == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *timeout = base_physical_drive->default_timeout;
    return FBE_STATUS_OK;

}

fbe_status_t 
fbe_base_physical_drive_io_fail(fbe_base_physical_drive_t * base_physical_drive, /* IN */
                                fbe_drive_configuration_handle_t drive_configuration_handle, /* IN */
                                fbe_drive_configuration_record_t * threshold_rec_p, /* IN */
                                fbe_payload_cdb_fis_error_flags_t error_flags, /* IN */
                                fbe_u8_t opcode,   /*IN */
                                fbe_port_request_status_t port_status,  /* IN */
                                fbe_scsi_sense_key_t sk, /* IN */
                                fbe_scsi_additional_sense_code_t asc, /* IN */
                                fbe_scsi_additional_sense_code_qualifier_t ascq) /* IN */
{
    fbe_status_t status;
    fbe_stat_action_t           stat_action  = FBE_STAT_ACTION_FLAG_FLAG_NO_ACTION;
    fbe_object_death_reason_t   death_reason;
    fbe_time_t                  current_time_ms;

    if (FBE_DRIVE_CONFIGURATION_SERVICE_INVALID_HANLDLE == drive_configuration_handle) {
        /* drive is not registered.  DIEH error weighting can't be caclulated. */
        fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_WARNING,
                            "%s config handle invalid.  DIEH being skipped\n", __FUNCTION__);
        return FBE_STATUS_OK;
    }

    fbe_base_object_lock((fbe_base_object_t *)base_physical_drive);
    
    status = fbe_stat_io_completion(((fbe_base_object_t*)base_physical_drive)->object_id,
                                    threshold_rec_p,
                                    &base_physical_drive->dieh_state,
                                    &base_physical_drive->drive_error,
                                    error_flags,
                                    opcode,
                                    port_status,
                                    sk,asc,ascq,                                   
                                    fbe_base_physical_drive_dieh_is_emeh_mode(base_physical_drive),
                                    &stat_action);


    /* Determine if port is hung by checking if we've been getting port TIMEOUTs for longer than
       the service_timeout and no successful IO has completed.  */
   
    if (port_status == FBE_PORT_REQUEST_STATUS_ABORT_TIMEOUT)
    {
        current_time_ms = fbe_get_time();

        /* Is this the first link error? */
        if (base_physical_drive->first_port_timeout == 0) 
        {
            base_physical_drive->first_port_timeout_successful_io_count = base_physical_drive->drive_error.io_counter;
            base_physical_drive->first_port_timeout = current_time_ms;
        }
        /* Have we hit a port lockup?*/
        else if (base_physical_drive->first_port_timeout_successful_io_count == base_physical_drive->drive_error.io_counter &&  /* successful io count hasn't changed */
                 (current_time_ms - base_physical_drive->first_port_timeout) > base_physical_drive->service_time_limit_ms)      /* AND exceeded our svc time */
        {
            /* Do a Health Check, which will issue a reset to break us out of a port 
               lockup situation.   If the reset doesn't get us out,  then after enough 
               HCs DIEH will shoot the drive. */

            fbe_base_drive_initiate_health_check(base_physical_drive, FBE_TRUE /*is_service_timeout*/);  

            fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_WARNING,
                           "%s: initiated health check for port lockup. first:0x%llx current:0x%llx\n", 
                           __FUNCTION__, base_physical_drive->first_port_timeout, current_time_ms);

            base_physical_drive->first_port_timeout = 0;  /* clear timeout so it starts over if port gets locked up again. */

        }        
        /* Is this a typical port error? */
        else if (base_physical_drive->first_port_timeout_successful_io_count != base_physical_drive->drive_error.io_counter) /* successful io count has changed. */                 
        {
            /* Successful IO count has changed since we last recorded the port timeout.  
               It's not a lockup case, but maybe this the begining of the lockup.
               So re-set the first port timeout. */
            base_physical_drive->first_port_timeout_successful_io_count = base_physical_drive->drive_error.io_counter;
            base_physical_drive->first_port_timeout = current_time_ms;

        }
    }

    fbe_base_object_unlock((fbe_base_object_t *)base_physical_drive);

    if (stat_action & FBE_STAT_ACTION_FLAG_FLAG_END_OF_LIFE) {
        fbe_base_physical_drive_customizable_trace(base_physical_drive,
                                                   FBE_TRACE_LEVEL_INFO,
                                                   "%s drive end of life initiated\n", __FUNCTION__);

        fbe_base_physical_drive_set_path_attr(base_physical_drive, FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE);      
    }

    if ((stat_action & FBE_STAT_ACTION_FLAG_FLAG_END_OF_LIFE_CALL_HOME) &&
        !base_physical_drive->dieh_state.eol_call_home_sent)         /* only send once */
    {
        fbe_base_physical_drive_customizable_trace(base_physical_drive,
                            FBE_TRACE_LEVEL_INFO,
                             "%s drive end of life call home\n", __FUNCTION__);

        base_physical_drive->dieh_state.eol_call_home_sent = FBE_TRUE;

        fbe_base_physical_drive_set_path_attr(base_physical_drive, 
                                              FBE_BLOCK_PATH_ATTR_FLAGS_CALL_HOME_END_OF_LIFE);
    }

    if(stat_action & FBE_STAT_ACTION_FLAG_FLAG_RESET){
        fbe_base_physical_drive_customizable_trace(base_physical_drive,
                            FBE_TRACE_LEVEL_INFO,
                            "%s fbe stat flag reset\n", __FUNCTION__);

        /* If resetting due to link error, then it's highly probably that drive may logout.  Since a reset 
           transitions to Activate state, we need to notify PVD that this is link related before edges go
           disabled */
        if (error_flags & FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_LINK)
        {
            /* Notify monitor to set link_fault */
            fbe_base_pdo_set_flags_under_lock(base_physical_drive, FBE_PDO_FLAGS_SET_LINK_FAULT);
        }

        status = fbe_lifecycle_set_cond(&fbe_base_physical_drive_lifecycle_const, 
                                        (fbe_base_object_t*)base_physical_drive, 
                                        FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_RESET);
    }

    if(stat_action & FBE_STAT_ACTION_FLAG_FLAG_FAIL){

        fbe_base_physical_drive_customizable_trace(base_physical_drive,
                            FBE_TRACE_LEVEL_INFO,
                            "%s fbe stat flag fail\n", __FUNCTION__);

        status = fbe_base_physical_drive_map_flags_to_death_reason(error_flags, &death_reason);
        if (status == FBE_STATUS_OK) {
            fbe_base_discovered_set_death_reason((fbe_base_discovered_t *)base_physical_drive, death_reason);
        }

        /* We need to set the BO:FAIL lifecycle condition */
        status = fbe_lifecycle_set_cond(&fbe_base_physical_drive_lifecycle_const, 
                                        (fbe_base_object_t*)base_physical_drive, 
                                        FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);
        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace(base_physical_drive,  FBE_TRACE_LEVEL_ERROR,
                    "%s Failed to set condition\n",__FUNCTION__);
        }
    }
    
    if((stat_action & FBE_STAT_ACTION_FLAG_FLAG_FAIL_CALL_HOME) &&
       !base_physical_drive->dieh_state.kill_call_home_sent)   /* only send once */
    {
        fbe_base_physical_drive_customizable_trace(base_physical_drive,
                            FBE_TRACE_LEVEL_INFO,
                            "%s fbe stat flag fail call home\n", __FUNCTION__);

        base_physical_drive->dieh_state.kill_call_home_sent = FBE_TRUE;

        fbe_base_physical_drive_set_path_attr(base_physical_drive, 
                                              FBE_BLOCK_PATH_ATTR_FLAGS_CALL_HOME_KILL);
    }
    
    return status;
}

fbe_status_t 
fbe_base_physical_drive_get_configuration_handle(fbe_base_physical_drive_t * base_physical_drive, 
                                                 fbe_drive_configuration_handle_t * drive_configuration_handle)
{

    *drive_configuration_handle = base_physical_drive->drive_configuration_handle;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_physical_drive_io_success(fbe_base_physical_drive_t * base_physical_drive)
{
    fbe_atomic_increment(&base_physical_drive->drive_error.io_counter);
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_physical_drive_reset_completed(fbe_base_physical_drive_t * base_physical_drive)
{
    fbe_stat_drive_reset(base_physical_drive->drive_configuration_handle,
                         &base_physical_drive->drive_error);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_physical_drive_get_drive_vendor_id(fbe_base_physical_drive_t * base_physical_drive, fbe_drive_vendor_id_t * vendor_id)
{
    * vendor_id = base_physical_drive->drive_info.drive_vendor_id;
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_base_physical_drive_map_flags_to_death_reason(fbe_payload_cdb_fis_error_flags_t error_flags,
                                                                      fbe_object_death_reason_t *death_reason)
{
    if (error_flags & FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_DATA)
    {
        *death_reason = FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_DATA_THRESHOLD_EXCEEDED;
    }     
    else if (error_flags & FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_RECOVERED)
    {
        *death_reason = FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_RECOVERED_THRESHOLD_EXCEEDED;
    }    
    else if (error_flags & FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_MEDIA)
    {
        *death_reason = FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_MEDIA_THRESHOLD_EXCEEDED;
    }    
    else if (error_flags & FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_HARDWARE)
    {
        *death_reason = FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_HARDWARE_THRESHOLD_EXCEEDED;
    }    
    else if (error_flags & FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_HEALTHCHECK)
    {
        *death_reason = FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_HEALTHCHECK_THRESHOLD_EXCEEDED;
    }    
    else if (error_flags & FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_LINK)
    {
        *death_reason = FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_LINK_THRESHOLD_EXCEEDED;
    }    
    else if (error_flags & FBE_PAYLOAD_CDB_FIS_ERROR_FLAG_FATAL)
    {
        *death_reason = FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_FATAL_ERROR;
    }    
    else
    {
        fbe_topology_class_trace(FBE_CLASS_ID_BASE_PHYSICAL_DRIVE,
                                 FBE_TRACE_LEVEL_ERROR, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s unable to map flag: 0x%x\n", __FUNCTION__, error_flags);
        return FBE_STATUS_GENERIC_FAILURE;
    }
        
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_physical_drive_set_sp_id(fbe_base_physical_drive_t * base_physical_drive, fbe_u8_t sp_id)
{
    if (base_physical_drive == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    base_physical_drive->sp_id = sp_id;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_physical_drive_get_sp_id(fbe_base_physical_drive_t * base_physical_drive, fbe_u8_t * sp_id)
{
    if (base_physical_drive == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *sp_id = base_physical_drive->sp_id;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_physical_drive_set_port_number(fbe_base_physical_drive_t * base_physical_drive, fbe_port_number_t port_number)
{
    if (base_physical_drive == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    base_physical_drive->port_number = port_number;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_physical_drive_get_port_number(fbe_base_physical_drive_t * base_physical_drive, fbe_port_number_t * port_number)
{
    if (base_physical_drive == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *port_number = base_physical_drive->port_number;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_physical_drive_set_enclosure_number(fbe_base_physical_drive_t * base_physical_drive, fbe_enclosure_number_t enclosure_number)
{
    if (base_physical_drive == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    base_physical_drive->enclosure_number = enclosure_number;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_physical_drive_get_enclosure_number(fbe_base_physical_drive_t * base_physical_drive, fbe_enclosure_number_t * enclosure_number)
{
    if (base_physical_drive == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *enclosure_number = base_physical_drive->enclosure_number;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_physical_drive_set_slot_number(fbe_base_physical_drive_t * base_physical_drive, fbe_slot_number_t slot_number)
{
    if (base_physical_drive == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    base_physical_drive->slot_number = slot_number;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_physical_drive_get_slot_number(fbe_base_physical_drive_t * base_physical_drive, fbe_slot_number_t * slot_number)
{
    if (base_physical_drive == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *slot_number = base_physical_drive->slot_number;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_physical_drive_set_bank_width(fbe_base_physical_drive_t * base_physical_drive, fbe_u32_t bank_width)
{
    if (base_physical_drive == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    base_physical_drive->bank_width = bank_width;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_physical_drive_get_bank_width(fbe_base_physical_drive_t * base_physical_drive, fbe_u32_t  * bank_width)
{
    if (base_physical_drive == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *bank_width = base_physical_drive->bank_width;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_physical_drive_set_powercycle_duration(fbe_base_physical_drive_t * base_physical_drive, 
                                                             fbe_u32_t duration /*in 500ms increments*/)
{
    if (base_physical_drive == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    base_physical_drive->drive_info.powercycle_duration = duration;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_physical_drive_get_powercycle_duration(fbe_base_physical_drive_t * base_physical_drive, 
                                                             fbe_u32_t *duration /*in 500ms increments*/)
{
    if (duration == NULL || base_physical_drive == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *duration = base_physical_drive->drive_info.powercycle_duration;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_physical_drive_get_serial_number(fbe_base_physical_drive_t * base_physical_drive, 
                                                             fbe_physcial_drive_serial_number_t *ser_no)
{
    if (ser_no == NULL || base_physical_drive == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    fbe_copy_memory (ser_no->serial_number, base_physical_drive->drive_info.serial_number, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE);    
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_physical_drive_set_powersave_attr(fbe_base_physical_drive_t * base_physical_drive, fbe_u32_t attr)
{
    if (base_physical_drive == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    base_physical_drive->drive_info.powersave_attr = attr;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_physical_drive_get_powersave_attr(fbe_base_physical_drive_t * base_physical_drive, fbe_u32_t * attr)
{
    if (base_physical_drive == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *attr = base_physical_drive->drive_info.powersave_attr;
    return FBE_STATUS_OK;
}

/* TLA's for drives that are qualified for the spindown feature.
 */
FBE_TLA_IDs fbe_TLA_spindown_tbl[] =
{"005048797" /* 1TB Hitatchi Gemini-K           */,
 "005048853" /* 1TB WD Hulk                     */,
 "005048829" /* 1TB Seagate Moose               */,
 FBE_DUMMY_TLA_ENTRY /* Dummy last array entry - MUST be last entry to exit loop search */
};

fbe_bool_t 
fbe_base_physical_drive_is_spindown_qualified(fbe_base_physical_drive_t * base_physical_drive, fbe_u8_t * tla)
{
    fbe_u32_t i;
    fbe_path_attr_t path_attr = 0;
    fbe_status_t status = FBE_STATUS_OK;

    if (tla == NULL) {
        return FBE_FALSE;
    }

    status = fbe_base_discovered_get_path_attributes((fbe_base_discovered_t *)base_physical_drive, &path_attr);
    if (status == FBE_STATUS_OK && (path_attr & FBE_DISCOVERY_PATH_ATTR_POWERSAVE_SUPPORTED))
    {
        if (fbe_equal_memory(tla + FBE_SCSI_INQUIRY_TLA_SIZE - FBE_SPINDOWN_STRING_LENGTH, 
            FBE_SPINDOWN_SUPPORTED_STRING, FBE_SPINDOWN_STRING_LENGTH)) {
            return FBE_TRUE;
        } else {
            /* Search the table of known TLA numbers for drives that are qualified for spindown. */
            for (i=0; (!fbe_equal_memory(fbe_TLA_spindown_tbl[i].tla, FBE_DUMMY_TLA_ENTRY, 9)); i++) {
                if (fbe_equal_memory(tla, fbe_TLA_spindown_tbl[i].tla, 9)) {
                    return FBE_TRUE;
                    break;
                }
            }
        }
    }
    return FBE_FALSE;
}

void
fbe_base_physical_drive_customizable_trace(fbe_base_physical_drive_t * base_physical_drive, 
                      fbe_trace_level_t trace_level,
                      const fbe_char_t * fmt, ...)
{
    va_list argList;
    fbe_u8_t header[FBE_TRACE_PDO_DSK_POS_SIZE+FBE_TRACE_PDO_SER_NUM_SIZE];
        
    if(trace_level <= ((fbe_base_object_t *)base_physical_drive)->trace_level)
    {
    fbe_zero_memory(header, FBE_TRACE_PDO_DSK_POS_SIZE+FBE_TRACE_PDO_SER_NUM_SIZE);
        csx_p_snprintf(header, FBE_TRACE_PDO_DSK_POS_SIZE, "DSK:%d_%d_%d", 
                            base_physical_drive->port_number, 
                            base_physical_drive->enclosure_number, 
                            base_physical_drive->slot_number);
 
        va_start(argList, fmt);
        fbe_trace_report_w_header(FBE_COMPONENT_TYPE_OBJECT,
                         ((fbe_base_object_t *)base_physical_drive)->object_id,
                         trace_level,
                         header,
                         fmt, 
                         argList);
        va_end(argList);
    }

}

fbe_status_t
base_physical_drive_get_dc_error_counts(fbe_base_physical_drive_t * base_physical_drive)
{
    fbe_status_t status;
    fbe_drive_threshold_and_exceptions_t threshold_rec = {0};
    fbe_u8_t id;

    id = base_physical_drive->dc.number_record;
    if (id >= FBE_DC_IN_MEMORY_MAX_COUNT) {
        fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                  FBE_TRACE_LEVEL_INFO,
                                  "%s Stat Error counts can't log numer_record:0x%X.\n",
                                  __FUNCTION__, id);
        return FBE_STATUS_OK;
    }
    
    fbe_base_physical_drive_update_error_counts(base_physical_drive, &base_physical_drive->dc.record[id].u.data.error_count);

    fbe_copy_memory(&base_physical_drive->dc.record[id].u.data.error_count.drive_error, &base_physical_drive->drive_error, sizeof(fbe_drive_error_t));
    
    /* Calculate thresholds */
    status = fbe_drive_configuration_get_threshold_info(base_physical_drive->drive_configuration_handle, &threshold_rec);

    if (status != FBE_STATUS_OK)
    {
        fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_ERROR,
                                          "%s get_threshold_info failed. handle:0x%x\n", __FUNCTION__, base_physical_drive->drive_configuration_handle);

        /* mark ratios as invalid */
        base_physical_drive->dc.record[id].u.data.error_count.io_error_ratio =           FBE_STAT_INVALID_RATIO;
        base_physical_drive->dc.record[id].u.data.error_count.recovered_error_ratio =    FBE_STAT_INVALID_RATIO;
        base_physical_drive->dc.record[id].u.data.error_count.media_error_ratio =        FBE_STAT_INVALID_RATIO;
        base_physical_drive->dc.record[id].u.data.error_count.hardware_error_ratio =     FBE_STAT_INVALID_RATIO;
        base_physical_drive->dc.record[id].u.data.error_count.healthCheck_error_ratio =  FBE_STAT_INVALID_RATIO;
        base_physical_drive->dc.record[id].u.data.error_count.link_error_ratio =         FBE_STAT_INVALID_RATIO;
        base_physical_drive->dc.record[id].u.data.error_count.reset_error_ratio =        FBE_STAT_INVALID_RATIO;
        base_physical_drive->dc.record[id].u.data.error_count.power_cycle_error_ratio =  FBE_STAT_INVALID_RATIO;
        base_physical_drive->dc.record[id].u.data.error_count.data_error_ratio =         FBE_STAT_INVALID_RATIO;
    }
    else
    {
        /* Check io_error ratio */
        fbe_stat_get_ratio(&threshold_rec.threshold_info.io_stat, 
                            base_physical_drive->drive_error.io_counter, 
                            base_physical_drive->drive_error.io_error_tag, 
                            &base_physical_drive->dc.record[id].u.data.error_count.io_error_ratio);
    
        fbe_stat_get_ratio(&threshold_rec.threshold_info.recovered_stat, 
                            base_physical_drive->drive_error.io_counter, 
                            base_physical_drive->drive_error.recovered_error_tag, 
                            &base_physical_drive->dc.record[id].u.data.error_count.recovered_error_ratio);
    
        fbe_stat_get_ratio(&threshold_rec.threshold_info.media_stat, 
                            base_physical_drive->drive_error.io_counter, 
                            base_physical_drive->drive_error.media_error_tag, 
                            &base_physical_drive->dc.record[id].u.data.error_count.media_error_ratio);
    
        fbe_stat_get_ratio(&threshold_rec.threshold_info.hardware_stat, 
                            base_physical_drive->drive_error.io_counter, 
                            base_physical_drive->drive_error.hardware_error_tag, 
                            &base_physical_drive->dc.record[id].u.data.error_count.hardware_error_ratio);
    
       fbe_stat_get_ratio(&threshold_rec.threshold_info.healthCheck_stat, 
                            base_physical_drive->drive_error.io_counter, 
                            base_physical_drive->drive_error.healthCheck_error_tag, 
                            &base_physical_drive->dc.record[id].u.data.error_count.healthCheck_error_ratio);
    
        fbe_stat_get_ratio(&threshold_rec.threshold_info.link_stat, 
                            base_physical_drive->drive_error.io_counter, 
                            base_physical_drive->drive_error.link_error_tag, 
                            &base_physical_drive->dc.record[id].u.data.error_count.link_error_ratio);
    
        fbe_stat_get_ratio(&threshold_rec.threshold_info.reset_stat, 
                            base_physical_drive->drive_error.io_counter, 
                            base_physical_drive->drive_error.reset_error_tag, 
                            &base_physical_drive->dc.record[id].u.data.error_count.reset_error_ratio);
    
        fbe_stat_get_ratio(&threshold_rec.threshold_info.power_cycle_stat, 
                            base_physical_drive->drive_error.io_counter, 
                            base_physical_drive->drive_error.power_cycle_error_tag, 
                            &base_physical_drive->dc.record[id].u.data.error_count.power_cycle_error_ratio);
    
        fbe_stat_get_ratio(&threshold_rec.threshold_info.data_stat, 
                            base_physical_drive->drive_error.io_counter, 
                            base_physical_drive->drive_error.data_error_tag, 
                            &base_physical_drive->dc.record[id].u.data.error_count.data_error_ratio);

    }

    return FBE_STATUS_OK;
}

/**************************************************************************
* base_physical_drive_fill_dc_header()                  
***************************************************************************
*
* DESCRIPTION
*       This function fill the header of Disk Collect structure.
*       
* PARAMETERS
*       base_physical_drive - Pointer to base physical drive object.
*
* RETURN VALUES
*       status. 
*
* NOTES
*
* HISTORY
*   23-Mar-2010 Christina Chiang - Created.
***************************************************************************/

fbe_status_t
base_physical_drive_fill_dc_header(fbe_base_physical_drive_t * base_physical_drive)
{
    fbe_dc_system_time_t system_time;
    fbe_base_physical_drive_info_t * info_ptr = & base_physical_drive->drive_info;
     
    if (( base_physical_drive->dc.number_record == 0) && (base_physical_drive->dc_lba == 0))
    {
        /* Fill in the header for disk collect. */
        fbe_get_dc_system_time(&system_time);
        base_physical_drive->dc.magic_number = FBE_MAGIC_NUMBER_OBJECT+1;
        base_physical_drive->dc.dc_timestamp = system_time;
        base_physical_drive->dc.record[0].extended_A07 = 0;
        base_physical_drive->dc.record[0].dc_type = FBE_DC_HEADER;
        base_physical_drive->dc.record[0].record_timestamp = system_time;
        fbe_base_physical_drive_get_port_number(base_physical_drive, (fbe_port_number_t *)&base_physical_drive->dc.record[0].u.header.bus);
        fbe_base_physical_drive_get_enclosure_number(base_physical_drive, (fbe_enclosure_number_t *)&base_physical_drive->dc.record[0].u.header.encl);
        fbe_base_physical_drive_get_slot_number(base_physical_drive, (fbe_slot_number_t *)&base_physical_drive->dc.record[0].u.header.slot);
        fbe_copy_memory (base_physical_drive->dc.record[0].u.header.product_rev, info_ptr->revision, FBE_SCSI_INQUIRY_REVISION_SIZE);
        fbe_copy_memory (base_physical_drive->dc.record[0].u.header.product_serial_num, info_ptr->serial_number, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE);
        fbe_copy_memory (base_physical_drive->dc.record[0].u.header.tla_part_number, info_ptr->tla, FBE_SCSI_INQUIRY_TLA_SIZE);
        base_physical_drive->dc.record[0].u.header.drive_vendor_id = info_ptr->drive_vendor_id;   
        base_physical_drive->dc.number_record++;
    }
   
    return FBE_STATUS_OK;
}

/**************************************************************************
* fbe_sas_physical_drive_fill_dc_death_reason()                  
***************************************************************************
*
* DESCRIPTION
*       This function fill the death reason in the disk collect structure.
*       
* PARAMETERS
*       sas_physical_drive - Pointer to SAS physical drive object.
*       payload_cdb_operation - Pointer to the cdb payload.
*       death_reason - Death reason is set in CM.
*
* RETURN VALUES
*       status. 
*
* NOTES
*
* HISTORY
*   26-May-2010 Christina Chiang - Created.
***************************************************************************/
fbe_status_t
base_physical_drive_fill_dc_death_reason(fbe_base_physical_drive_t * base_physical_drive, FBE_DRIVE_DEAD_REASON death_reason)
{
    fbe_atomic_t dc_lock = FBE_FALSE;
    fbe_status_t status;
    fbe_u8_t id;
    fbe_dc_system_time_t system_time;


    /* Grap the lock. */
    dc_lock = fbe_atomic_exchange(&base_physical_drive->dc_lock, FBE_TRUE);
    if (dc_lock == FBE_TRUE) {
        /* Don't get the lock. */
        fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                 FBE_TRACE_LEVEL_INFO,
                                 "%s DC is locked. New lock:0x%llX Prior lock:0x%llX.\n",
                                  __FUNCTION__,
                  (unsigned long long)base_physical_drive->dc_lock,
                  (unsigned long long)dc_lock);
                                  
        /* Don't get the lock, save the death reason and reschedule the condidtion. */
        base_physical_drive->death_reason = death_reason;
        return FBE_STATUS_OK;
    }
   
    id = base_physical_drive->dc.number_record;
    if (id < FBE_DC_IN_MEMORY_MAX_COUNT)
    {
        /* Fill in the data for disk collect. */
        fbe_get_dc_system_time(&system_time);
        base_physical_drive->dc.magic_number = FBE_MAGIC_NUMBER_OBJECT+1;
        base_physical_drive->dc.dc_timestamp = system_time;
        base_physical_drive->dc.record[id].extended_A07 = death_reason;
        base_physical_drive->dc.record[id].dc_type = FBE_DC_NONE;
        base_physical_drive->dc.record[id].record_timestamp = system_time;
        base_physical_drive->dc.number_record++;
    }    
    
    /* Release the lock. */
    fbe_atomic_exchange(&base_physical_drive->dc_lock, FBE_FALSE);
    fbe_base_physical_drive_customizable_trace(base_physical_drive,
                        FBE_TRACE_LEVEL_INFO,
                        "%s fbe DC set flush data condition.\n", __FUNCTION__);

    status = fbe_lifecycle_set_cond(&fbe_base_physical_drive_lifecycle_const, 
                         (fbe_base_object_t*)base_physical_drive, 
                         FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_DISK_COLLECT_FLUSH);
    return status;
}

/**************************************************************************
* base_physical_drive_is_vault_drive()                  
***************************************************************************
*
* DESCRIPTION
*       This function check if the drive is a vauld drive.
*       
* PARAMETERS
*       base_physical_drive - Pointer to base physical drive object.
*
* RETURN VALUES
*       FBE_TRUE or FBE_FALSE. 
*
* NOTES
*
* HISTORY
*   19-AUG-2010 Christina Chiang - Created.
***************************************************************************/

fbe_bool_t
base_physical_drive_is_vault_drive(fbe_base_physical_drive_t * base_physical_drive)
{
    if ((base_physical_drive->port_number == 0) &&
    (base_physical_drive->enclosure_number == 0) &&
    (base_physical_drive->slot_number < 4))
    {
        fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                 FBE_TRACE_LEVEL_INFO,
                                 "%s Disk %d is a vault Drive. \n",  __FUNCTION__, base_physical_drive->slot_number);
       return FBE_TRUE;
    }
  
    return FBE_FALSE;
}
    
    

void fbe_base_physical_drive_update_traffic_load(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_traffic_priority_t      packet_traffic_priority = FBE_TRAFFIC_PRIORITY_INVALID;
    fbe_u32_t                   outstanding_ios = 0;
    fbe_u32_t                   max_outstanding = 0;

    /*

        To avoid warning C4197, we will do some casting games based on Microsofft recommandations
        this is done because atomic is volatile

    */

    /*let's see whats the packet paiority*/
    fbe_transport_get_traffic_priority(packet, &packet_traffic_priority);

    /*and add it to the total priority count. Later we will get an average*/
    fbe_atomic_add(&base_physical_drive->total_io_priority, (fbe_u64_t)packet_traffic_priority);

    /*increment the total IOs we had so we can calculate average later*/
    fbe_atomic_increment(&base_physical_drive->total_outstanding_ios);

    /*we also remember the highest outstanding IOs we had*/
    fbe_block_transport_server_get_outstanding_io_count(&base_physical_drive->block_transport_server ,&outstanding_ios);
    fbe_block_transport_server_get_outstanding_io_max(&base_physical_drive->block_transport_server, &max_outstanding);

    /*we care only how much we used out of the drive buffer*/
    if ((base_physical_drive->highest_outstanding_ios < outstanding_ios) &&
        (outstanding_ios <= max_outstanding)) {
        fbe_atomic_exchange(&base_physical_drive->highest_outstanding_ios, (fbe_u64_t)outstanding_ios);
    }

    /*we are done now. When the monitor kicks in, it will use all this data to calculate the load and if needed, advertise it on the tree*/

}

fbe_status_t fbe_base_physical_drive_process_block_transport_event(fbe_block_transport_event_type_t event_type,
                                                                   fbe_block_trasnport_event_context_t context)
{
    fbe_base_physical_drive_t *base_physical_p = (fbe_base_physical_drive_t *)context;

    switch(event_type) {
    case FBE_BLOCK_TRASPORT_EVENT_TYPE_IO_WAITING_ON_QUEUE:
        /*there is an incomming IO, we need to wake up the object and make sure once it's in ready, to send the IO*/
        base_physical_drive_process_incoming_io_duing_hibernate(base_physical_p);
        break;
    default:
        fbe_base_physical_drive_customizable_trace(base_physical_p,
                                                   FBE_TRACE_LEVEL_ERROR,
                                                   "%s can't process event:%d from block transport\n",__FUNCTION__, event_type);

    }
    

    return FBE_STATUS_OK;
}

static fbe_status_t base_physical_drive_process_incoming_io_duing_hibernate(fbe_base_physical_drive_t *base_physical_p)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;

    /*set a condition on the READY rotary once the object becomes alive*/
    status = fbe_lifecycle_set_cond(&fbe_base_physical_drive_lifecycle_const,
                                    (fbe_base_object_t*)base_physical_p,
                                     FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_DEQUEUE_PENDING_IO);

    if (status != FBE_STATUS_OK) {
         fbe_base_physical_drive_customizable_trace(base_physical_p,
                                                    FBE_TRACE_LEVEL_ERROR,
                                                    "%s can't set condition to process IO after drive wakeup !\n",__FUNCTION__ );

         return FBE_STATUS_GENERIC_FAILURE;

    }

    /*and move it to activate so it starts waking up*/
    status = fbe_lifecycle_set_cond(&fbe_base_physical_drive_lifecycle_const, 
                                    (fbe_base_object_t*)base_physical_p, 
                                    FBE_BASE_OBJECT_LIFECYCLE_COND_ACTIVATE);

    if (status != FBE_STATUS_OK) {
         fbe_base_physical_drive_customizable_trace(base_physical_p,
                                                    FBE_TRACE_LEVEL_ERROR,
                                                    "%s can't set object to activate after drive wakeup !\n",__FUNCTION__ );

         return FBE_STATUS_GENERIC_FAILURE;

    }
    

    return FBE_STATUS_OK;
}

void base_physical_drive_increment_down_time(fbe_base_physical_drive_t * base_physical_drive)
{
    fbe_u64_t   down_time = base_physical_drive->drive_info.spin_time;

    /*High part is down time*/
    down_time |= 0x00000000FFFFFFFF;
    down_time ++;
    base_physical_drive->drive_info.spin_time &= 0x00000000FFFFFFFF;
    base_physical_drive->drive_info.spin_time |= down_time;
}

void base_physical_drive_increment_up_time(fbe_base_physical_drive_t * base_physical_drive)
{
    /*Low part is up time*/
    base_physical_drive->drive_info.spin_time ++;
    
}

/*how long in minutes was the drive idle*/
fbe_status_t base_physical_drive_get_down_time(fbe_base_physical_drive_t * base_physical_drive, fbe_u32_t *get_down_time)
{
    fbe_u64_t   down_time = base_physical_drive->drive_info.spin_time;

    /*High part is down time*/
    down_time &= 0xFFFFFFFF00000000;
    down_time >>= 32;

    *get_down_time = (fbe_u32_t)down_time;

    return FBE_STATUS_OK;


}

/*how long in minutes was the drive spinning*/
fbe_status_t base_physical_drive_get_up_time(fbe_base_physical_drive_t * base_physical_drive, fbe_u32_t *get_up_time)
{
    fbe_u64_t   up_time = base_physical_drive->drive_info.spin_time;

    /*Low part is up time*/
    up_time &= 0x00000000FFFFFFFF;
    *get_up_time = (fbe_u32_t)up_time;

    return FBE_STATUS_OK;
}


fbe_status_t 
fbe_base_physical_drive_set_glitch_time(fbe_base_physical_drive_t * base_physical_drive, fbe_u64_t glitch_time)
{
    if (base_physical_drive == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    base_physical_drive->glitch_time = glitch_time;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_physical_drive_get_glitch_time(fbe_base_physical_drive_t * base_physical_drive, fbe_u64_t * glitch_time)
{
    if (base_physical_drive == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *glitch_time = base_physical_drive->glitch_time;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_physical_drive_set_glitch_end_time(fbe_base_physical_drive_t * base_physical_drive, fbe_u64_t glitch_end_time)
{
    if (base_physical_drive == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    base_physical_drive->glitch_end_time = glitch_end_time;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_physical_drive_get_glitch_end_time(fbe_base_physical_drive_t * base_physical_drive, fbe_u64_t * glitch_end_time)
{
    if (base_physical_drive == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *glitch_end_time = base_physical_drive->glitch_end_time;
    return FBE_STATUS_OK;
}

fbe_bool_t fbe_base_physical_drive_is_enhanced_queuing_supported(fbe_base_physical_drive_t * base_physical_drive)
{
    return base_physical_drive->drive_info.supports_enhanced_queuing;
}

void fbe_base_physical_drive_set_enhanced_queuing_supported(fbe_base_physical_drive_t * base_physical_drive, fbe_bool_t is_supported)
{
    base_physical_drive->drive_info.supports_enhanced_queuing = is_supported;
}

fbe_status_t fbe_base_physical_drive_get_logical_drive_state(fbe_base_physical_drive_t * base_physical_drive, fbe_block_transport_logical_state_t *logical_state)
{
    if (base_physical_drive == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *logical_state = base_physical_drive->logical_state;
    return FBE_STATUS_OK;

}

/*!**************************************************************
 * fbe_base_physical_drive_set_path_attr()
 ****************************************************************
 * @brief
 *  This function will set the path attribute on the edge.   If
 *  edge doesn't exist it is cached and set when PDO goes to
 *  Ready state.
 *
 * @param - base_physical_drive - ptr to base PDO.
 * @param - path_attr  - path attribute to be set
 *
 * @author
 *  17-Jul-2012:  Wayne Garrett  -  Created.
 *
 ****************************************************************/
void fbe_base_physical_drive_set_path_attr(fbe_base_physical_drive_t * base_physical_drive, fbe_block_path_attr_flags_t path_attr)
{
    fbe_u8_t deviceStr[FBE_PHYSICAL_DRIVE_STRING_LENGTH];
    fbe_physcial_drive_serial_number_t  serial_number;
    fbe_status_t status = FBE_STATUS_OK;

    /* Keep track of all path attributes set.  There are cases where this can be called before edges are connected.   When
       drive goes to Ready (and edges are connected) these cached attrs will be set.  If edges happen to already exist
       then it doesn't hurt to cache them and re-set the attr next time we hit Ready */

    /* Do not update cache path attributes for link fault */
    if (path_attr != FBE_BLOCK_PATH_ATTR_FLAGS_LINK_FAULT)
    {
        base_physical_drive->cached_path_attr |= path_attr;
    }

    /* Set the attrs on the actual edge if they exist */
    if (fbe_block_transport_server_is_edge_connected(&base_physical_drive->block_transport_server))
    {
        /* Log the Event */
        fbe_zero_memory(&deviceStr[0], FBE_PHYSICAL_DRIVE_STRING_LENGTH);

        /* Create string with Bus, Enclosure and Slot information*/ 
        status = fbe_base_physical_drive_create_drive_string(base_physical_drive, 
                                                             &deviceStr[0],
                                                             FBE_PHYSICAL_DRIVE_STRING_LENGTH,
                                                             TRUE);

        /* Get Serial Number */
        fbe_zero_memory(&serial_number, sizeof(fbe_physcial_drive_serial_number_t));
        fbe_base_physical_drive_get_serial_number(base_physical_drive, &serial_number);

        /* Generate the correct event based on the path attribute set.
         */
        switch (path_attr) 
        {
            case FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE:
                /* EOL or PFA */
                if (fbe_dcs_param_is_enabled(FBE_DCS_PFA_HANDLING))
                {
                    fbe_event_log_write(PHYSICAL_PACKAGE_INFO_PDO_SET_EOL, NULL, 0, "%s %s",
                                        &deviceStr[0],
                                        serial_number.serial_number);
                }
                else
                {
                    /* PFA posting disabled generate a warning and return.
                     */
                    base_physical_drive->cached_path_attr &= ~path_attr;
                    fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                                               FBE_TRACE_LEVEL_WARNING,
                                                               "%s PFA HANDLING DISABLED\n", __FUNCTION__);
                    return;  
                }
                break;

            case FBE_BLOCK_PATH_ATTR_FLAGS_LINK_FAULT:
                fbe_event_log_write(PHYSICAL_PACKAGE_INFO_PDO_SET_LINK_FAULT, NULL, 0, "%s %s",
                                    &deviceStr[0],
                                    serial_number.serial_number);
                break;

            case FBE_BLOCK_PATH_ATTR_FLAGS_DRIVE_FAULT:
                fbe_event_log_write(PHYSICAL_PACKAGE_INFO_PDO_SET_DRIVE_FAULT, NULL, 0, "%s %s",
                                    &deviceStr[0],
                                    serial_number.serial_number);
                break;

            case FBE_BLOCK_PATH_ATTR_FLAGS_CALL_HOME_END_OF_LIFE:
                fbe_event_log_write(PHYSICAL_PACKAGE_ERROR_PDO_SET_EOL, NULL, 0, "%s %s",
                                    &deviceStr[0],
                                    serial_number.serial_number);
                break;

            case FBE_BLOCK_PATH_ATTR_FLAGS_CALL_HOME_KILL:
                fbe_event_log_write(PHYSICAL_PACKAGE_ERROR_PDO_SET_DRIVE_KILL, NULL, 0, "%s %s",
                                    &deviceStr[0],
                                    serial_number.serial_number);
                break;

            default:
                /* Do nothing in default option*/
                break;

        } /* end switch on path attribute*/

        /* Set the path attribute*/
        status = fbe_block_transport_server_set_path_attr_all_servers( &base_physical_drive->block_transport_server, path_attr);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                           FBE_TRACE_LEVEL_ERROR,
                                           "%s Failed to set path attr: %s status: 0x%x\n",
                                           __FUNCTION__,fbe_base_physical_drive_get_path_attribute_string(path_attr), status);
        }
        else
        {
            /* clear the cache path attribute*/
            base_physical_drive->cached_path_attr &= ~path_attr;
        }
    }
    else
    {
        fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_WARNING, 
                                                   "set_path_attr: PDO edge not connected.  path_attr:%s.\n",
                                                   fbe_base_physical_drive_get_path_attribute_string(path_attr));
    }
}


/*how long in minutes was the drive idle*/
fbe_status_t base_physical_drive_set_down_time(fbe_base_physical_drive_t * base_physical_drive, fbe_u32_t *set_down_time)
{
    fbe_u64_t   down_time = *set_down_time;
    
    down_time >>= 32;
    /*High part is down time*/
    down_time &= 0xFFFFFFFF00000000;

    base_physical_drive->drive_info.spin_time = down_time;

    return FBE_STATUS_OK;

}

/*how long in minutes was the drive spinning*/
fbe_status_t base_physical_drive_set_up_time(fbe_base_physical_drive_t * base_physical_drive, fbe_u32_t *set_up_time)
{
    fbe_u64_t   up_time = *set_up_time;
   
    /*Low part is up time*/
    up_time &= 0x00000000FFFFFFFF;

    base_physical_drive->drive_info.spin_time = up_time;

    return FBE_STATUS_OK;

}

/*!*************************************************************************
* @fn fbe_base_physical_drive_send_data_change_notification()
***************************************************************************
*
* @brief      This function send an event notification for the specified device
*             from PDO.
*
* @param      base_physical_drive - ptr to base PDO.
* @param      pDataChangeInfo - Notification data changed
*
* @return     void.
*
* @author   08/02/2012 kothal  - Created.
*
***************************************************************************/
void fbe_base_physical_drive_send_data_change_notification(fbe_base_physical_drive_t * base_physical_drive, fbe_notification_data_changed_info_t * pDataChangeInfo)
{
    fbe_notification_info_t     notification;
    fbe_object_id_t             objectId;
    
    fbe_base_object_get_object_id((fbe_base_object_t *)base_physical_drive, &objectId);

    fbe_set_memory(&notification, 0, sizeof(fbe_notification_info_t));
    notification.notification_type = FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED; 
    notification.object_type = FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE;   
    notification.class_id = FBE_CLASS_ID_BASE_PHYSICAL_DRIVE;    
    notification.notification_data.data_change_info = *pDataChangeInfo;

    fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_DEBUG_LOW, 
                                        "Sending object data change notification, dataType 0x%X, location %d_%d_%d, deviceMask 0x%llx.\n", 
                                        pDataChangeInfo->data_type,
                                        pDataChangeInfo->phys_location.bus,
                                        pDataChangeInfo->phys_location.enclosure,
                                        pDataChangeInfo->phys_location.slot,
                                        pDataChangeInfo->device_mask);

    fbe_notification_send(objectId, notification);

    return;
}   


fbe_status_t 
fbe_base_physical_drive_set_io_throttle_max(fbe_base_physical_drive_t * base_physical_drive,fbe_block_count_t io_throttle_max)
{
    fbe_status_t status;

    status = fbe_block_transport_server_set_io_throttle_max(&base_physical_drive->block_transport_server, io_throttle_max);

    return status;
}

/* Exceptions for enabling unmap support
 * Currently used to ignore the vpd B2 page and ignore port underrun errors for these drives
 */
fbe_part_number_ids_t fbe_part_number_unmap_exceptions_tbl[] =
{
    "DG118033078" /* 100 gb micron buckhorn          */,
    "DG118033079" /* 200 gb micron buckhorn          */,
    "DG118033080" /* 400 gb micron buckhorn          */,
    FBE_DUMMY_PN_ENTRY /* Dummy last array entry - MUST be last entry to exit loop search */
};

/*!*************************************************************************
* @fn fbe_base_physical_drive_is_unmap_supported_override()
***************************************************************************
*
* @brief      This function determines if the drive supports unmap regardless
*             of the VPD page. Exception was made for Micron Buckhorn drives
*
* @param      base_physical_drive - ptr to base PDO.
*
* @return     void.
*
* @author   06/20/2015 Deanna Heng  - Created.
*
***************************************************************************/
fbe_bool_t 
fbe_base_physical_drive_is_unmap_supported_override(fbe_base_physical_drive_t * base_physical_drive)
{
    fbe_u32_t i;

    /* Search the table of known Part numbers for drives that are qualified for unmap. */
    for (i=0; (!fbe_equal_memory(fbe_part_number_unmap_exceptions_tbl[i].pn, FBE_DUMMY_PN_ENTRY, (fbe_u32_t)strlen(FBE_DUMMY_PN_ENTRY))); i++) {
        if (fbe_equal_memory(base_physical_drive->drive_info.part_number, 
                             fbe_part_number_unmap_exceptions_tbl[i].pn, 
                             (fbe_u32_t)strlen(fbe_part_number_unmap_exceptions_tbl[i].pn))) {
            return FBE_TRUE;
            break;
        }
    }

    return FBE_FALSE;
}



