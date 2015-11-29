/**********************************************************************
 *  Copyright (C)  EMC Corporation 2008
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 **********************************************************************/

/*!*************************************************************************
 * @file fbe_base_physical_drive_utils.c
 ***************************************************************************
 *
 *  The routines in this file contains the utility functions used by the 
 *  base physical drive object and that are common to all drive objects.
 *
 * NOTE: 
 *
 * HISTORY
 *   15-Sep-2010 burbaj - Created. 
 *
 ***************************************************************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_board.h"
#include "fbe/fbe_stat_api.h"
//#include "fbe/fbe_ctype.h"

#include "fbe_transport_memory.h"
#include "fbe_topology.h"
#include "fbe_scheduler.h"

#include "base_physical_drive_private.h"
#include "swap_exports.h"


static void fbe_base_drive_trace_queue_lengths(fbe_base_physical_drive_t *base_physical_drive, fbe_packet_t * packet);


/***************************************************************************
 *              fbe_trace_RBA_traffic
 ***************************************************************************
 *
 *  DESCRIPTION:
 *    Log physical package traffic for RBA performance analysis.
 *
 *  RETURN VALUE:
 *    None.
 **************************************************************************/
void fbe_trace_RBA_traffic(fbe_base_physical_drive_t* base_physical_drive, fbe_lba_t lba, fbe_block_count_t block_count, fbe_u32_t io_traffic_info)
{
    fbe_u64_t number_of_blocks = 0;
    fbe_u64_t io_parameters = 0;
    
    number_of_blocks = (fbe_u64_t) block_count;
    /* io_traffic_info already contains the type of traffic so now
         add in the port, enclosure and slot information so that
         io_traffic_info = 2 byte port | 2 byte enclosure | 2 byte slot | 2 byte operation         
     */
    io_parameters |= (base_physical_drive->port_number & 0xFFFF);
    io_parameters <<= 16;
    io_parameters |= (base_physical_drive->enclosure_number & 0xFFFF);
    io_parameters <<= 16;
    io_parameters |= (base_physical_drive->slot_number & 0xFFFF);
    io_parameters <<= 16;
    io_parameters |= (io_traffic_info & 0xFFFF);

#if defined(UMODE_ENV) || defined(SIMMODE_ENV)
    /* In user mode, this trace is displayed on console window.*/
    fbe_KvTrace("PDO: lba high: 0x%llx lba low: 0x%llx bl: 0x%llx obj/opcode: 0x%llx\n",
                /* Send LBA high part */
                ((fbe_u64_t)lba & 0xFFFFFFFF00000000),
                /* Send LBA low part */
                ((fbe_u64_t)lba & 0x00000000FFFFFFFF),
                /* Send the number of blocks */
                number_of_blocks,
                /* Send the traffic information */
                io_parameters);
#else   
    KTRACEX(TRC_K_TRAFFIC,
            /* Indicate that it's from the PDO */
            KT_FBE_PDO_TRAFFIC,
            /* Send LBA high part */
            ((fbe_u64_t)lba & 0xFFFFFFFF00000000),
            /* Send LBA low part */
            ((fbe_u64_t)lba & 0x00000000FFFFFFFF),
            /* Send the number of blocks */
            number_of_blocks,
            /* Send the traffic information */
            io_parameters);
#endif            
    return;
}

/***************************************************************************
 *              fbe_get_dc_system_time
 ***************************************************************************
 *
 *  DESCRIPTION:
 *    Convert fbe_get_system_time into 8 bytes format.
 *
 * RETURN VALUES
 *       status. 
 *
 * NOTES
 *
 * HISTORY
 *    4-April-2011 Christina Chiang - Created.
 **************************************************************************/

fbe_status_t 
fbe_get_dc_system_time(fbe_dc_system_time_t * p_system_time)
{
    fbe_system_time_t system_time;

    fbe_get_system_time(&system_time);
    p_system_time->year = (fbe_u8_t)(system_time.year % 100);
    p_system_time->month = (fbe_u8_t)system_time.month;
    p_system_time->day = (fbe_u8_t)system_time.day;
    p_system_time->hour = (fbe_u8_t)system_time.hour;
    p_system_time->minute = (fbe_u8_t)system_time.minute;
    p_system_time->second = (fbe_u8_t)system_time.second;
    p_system_time->milliseconds = system_time.milliseconds;

    return FBE_STATUS_OK;
}


/*!**************************************************************
 * fbe_base_physical_drive_clear_dieh_state()
 ****************************************************************
 * @brief
 *  This function clears DIEH (Drive Improved Error Handling) 
 *  state info.
 *
 * @param base_physical_drive - pointer to PDO object
 *
 * @return void
 *
 * @author
 *  04/17/2012 : Wayne Garrett -- Created.
 *
 ****************************************************************/
void         
fbe_base_physical_drive_clear_dieh_state(fbe_base_physical_drive_t * base_physical_drive)
{
    fbe_zero_memory(&base_physical_drive->dieh_state.category, sizeof (base_physical_drive->dieh_state.category));
    base_physical_drive->dieh_state.media_weight_adjust = FBE_STAT_WEIGHT_CHANGE_NONE;   
    base_physical_drive->dieh_state.eol_call_home_sent = FBE_FALSE;
    base_physical_drive->dieh_state.kill_call_home_sent = FBE_FALSE;
    base_physical_drive->dieh_mode = FBE_DRIVE_MEDIA_MODE_THRESHOLDS_DEFAULT;
}


/*!**************************************************************
 * fbe_base_physical_drive_reset_statistics()
 ****************************************************************
 * @brief
 *  This function resets drive statistics info.
 *
 * @param base_physical_drive - pointer to PDO object
 *
 * @return void
 *
 * @author
 *  02/26/2013 : Darren Insko - Created.
 *
 ****************************************************************/         
fbe_time_t 
fbe_base_physical_drive_reset_statistics(fbe_base_physical_drive_t * base_physical_drive)
{
    fbe_time_t current_time;

    current_time = fbe_get_time_in_us();

    base_physical_drive->stats_num_io_in_progress = 0;    
    base_physical_drive->prev_start_lba = 0;

    base_physical_drive->start_idle_timestamp = current_time;
    base_physical_drive->start_busy_timestamp = current_time;

    return current_time;
}


/*!**************************************************************
 * @fn fbe_base_drive_test_and_set_health_check
 ****************************************************************
 * @brief
 *  This function tests if service time has exceeded.  If so it
 *  will initiate a health check, that's if health check is enabled.
 *  Function will return true only if health check has been
 *  initiated.  This will let caller decide how to handle the IO
 *  since PDO will be transitioning out of the Ready state.
 * 
 * @param base_physical_drive - ptr to this object
 * 
 * @return fbe_bool_t - FBE_TRUE if health check has been initated.
 *
 * @author
 *  11/08/2012  Wayne Garrett - Created.
 *
 ****************************************************************/
fbe_bool_t 
fbe_base_drive_test_and_set_health_check(fbe_base_physical_drive_t *base_physical_drive, fbe_packet_t * packet)
{   

    /* If our total pdo service time for this I/O is greater than the max pdo service time ms,
     * then tell the upper level about timeout errors and fail the I/O.
     */
    if (fbe_base_drive_is_service_time_exceeded(base_physical_drive, packet, base_physical_drive->service_time_limit_ms))
    {
        if (fbe_dcs_is_healthcheck_enabled())
        {
            /* If we are in a middle of a download when this happens, then easiest thing to do is abort it so we can process the health check.
               However, due to complicated nature of download state machine, we can't enter Health Check yet.   So return IO
               as retryable.  Eventually when download is aborted, this retried IO will trigger Health Check. 
               TODO: This is not a complete solution due to race conditions at PVD level.  This will be revisted during MR1.
            */
            if (fbe_base_physical_drive_is_local_state_set(base_physical_drive, FBE_BASE_DRIVE_LOCAL_STATE_DOWNLOAD))
            {
                fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_WARNING,
                                                           "test_and_set_health_check - abrt dl in progress.  ignore health check\n");
                fbe_lifecycle_set_cond(&fbe_base_physical_drive_lifecycle_const, (fbe_base_object_t*)base_physical_drive,
                                       FBE_BASE_DISCOVERED_LIFECYCLE_COND_ABORT_FW_DOWNLOAD);
                return FBE_FALSE;
            }

            /* proceed with health check */
            fbe_base_drive_initiate_health_check(base_physical_drive, FBE_TRUE /*is_service_timeout*/);
            return FBE_TRUE;
        }
    }

    return FBE_FALSE;
}

/*!**************************************************************
 * @fn fbe_base_drive_trace_queue_lengths
 ****************************************************************
 * @brief
 *  Traces the lengths of PDO's transport queues.   
 * 
 * @param base_physical_drive - ptr to this object
 * @param packet - ptr to IO packet
 * 
 * @return void 
 *
 * @author
 *  05/01/2013  Wayne Garrett - Created.
 *
 ****************************************************************/
static void fbe_base_drive_trace_queue_lengths(fbe_base_physical_drive_t *base_physical_drive, fbe_packet_t * packet)
{
    fbe_packet_priority_t queue_length[FBE_PACKET_PRIORITY_LAST-1] = {0};
    fbe_u32_t priority, index;
    fbe_u32_t io_count = 0;

    fbe_spinlock_lock(&base_physical_drive->block_transport_server.queue_lock);
    for(priority = FBE_PACKET_PRIORITY_LAST - 1; priority > FBE_PACKET_PRIORITY_INVALID; priority--)
    {
        index = priority - 1;
        queue_length[index] = fbe_queue_length(&base_physical_drive->block_transport_server.queue_head[index]);
    }
 
    fbe_block_transport_server_get_outstanding_io_count(&base_physical_drive->block_transport_server, &io_count);

    fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_WARNING,
                                               "PDO transport q lengths. URGENT:%d NORMAL:%d LOW:%d, outstanding:%d\n",
                                               queue_length[FBE_PACKET_PRIORITY_URGENT-1], 
                                               queue_length[FBE_PACKET_PRIORITY_NORMAL-1], 
                                               queue_length[FBE_PACKET_PRIORITY_LOW-1],
                                               io_count );


    fbe_spinlock_unlock(&base_physical_drive->block_transport_server.queue_lock);
}


/*!**************************************************************
 * @fn fbe_base_drive_is_remap_service_time_exceeded
 ****************************************************************
 * @brief
 *  This function tests if the remap service time has exceeded.  
 * 
 * @param base_physical_drive - ptr to this object
 * 
 * @return fbe_bool_t - FBE_TRUE if health check has been initated.
 *
 * @author
 *  11/08/2012  Wayne Garrett - Created.
 *
 ****************************************************************/
fbe_bool_t fbe_base_drive_is_service_time_exceeded(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet, fbe_time_t service_time_limit_ms)
{
    fbe_time_t current_pdo_ms;
    fbe_time_t start_time;
    fbe_time_t total_ms;

    fbe_transport_get_physical_drive_service_time(packet, &current_pdo_ms);

    /* As an optimization only check for timeout if we came in with a service time.
     */
    if (current_pdo_ms > 0)
    {
        fbe_transport_get_physical_drive_io_stamp(packet, &start_time);
        /* total = current PDO time + time for current operation */
        total_ms = current_pdo_ms + fbe_get_elapsed_milliseconds(start_time); 

        if (total_ms > service_time_limit_ms)
        {
            fbe_payload_ex_t * payload = fbe_transport_get_payload_ex(packet);
            fbe_payload_block_operation_t * block_operation = NULL;
            fbe_payload_block_operation_opcode_t block_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID;
            fbe_lba_t lba =  FBE_LBA_INVALID;
            fbe_block_count_t block_count = FBE_CDB_BLOCKS_INVALID;
                    
            block_operation = fbe_payload_ex_get_block_operation(payload);
            if (block_operation != NULL)
            {
                fbe_payload_block_get_opcode(block_operation, &block_opcode);
                fbe_payload_block_get_lba(block_operation, &lba);
                fbe_payload_block_get_block_count(block_operation, &block_count);
            }

            fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_WARNING,
                                                       "total service time %lld > %lld for pkt:%p blkop:%d lba:0x%llx cnt:0x%llx pri:%d\n",
                                                       total_ms, service_time_limit_ms, packet, block_opcode, lba, block_count, packet->packet_priority);
            /* print stats for diagnosability */
            fbe_base_drive_trace_queue_lengths(base_physical_drive, packet);

            return FBE_TRUE;
        }    
    }    
    return FBE_FALSE;
}


/*!**************************************************************
 * @fn fbe_base_drive_initiate_health_check
 ****************************************************************
 * @brief
 *  This function initate a health check due to serviceability
 *  timeout.
 * 
 * @param base_physical_drive - ptr to PDO object
 * @param is_service_time - Indicates if HC being set due to
 *              a service timeout.  In this case we need to
 *              set a TIMEOUT attribute so RAID can handle this
 *              appropriately.
 * 
 * @return none.
 *
 * @author
 *  11/08/2012  Wayne Garrett - Created.
 *
 ****************************************************************/
void 
fbe_base_drive_initiate_health_check(fbe_base_physical_drive_t *base_physical_drive, fbe_bool_t is_service_timeout)
{
    /* Initiate health check should only be called once.  If already started then log an error and return.*/
    if (fbe_base_physical_drive_is_local_state_set(base_physical_drive, FBE_BASE_DRIVE_LOCAL_STATE_HEALTH_CHECK))
    {
        fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_WARNING,
                                                   "%s Health Check already in progress\n", __FUNCTION__);
        return;
    }
    fbe_base_physical_drive_set_local_state(base_physical_drive, FBE_BASE_DRIVE_LOCAL_STATE_HEALTH_CHECK);

    /* only HC due to service timouts should set the TIMEOUT attr */
    if (is_service_timeout)
    {
        fbe_base_physical_drive_set_path_attr(base_physical_drive,  FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS); /*flag cleared by RAID.*/
    }

    /* Transition to ACTIVATE state and set required edge attributes to notify PVD. RAID depends on PDO going to Activate */
    fbe_lifecycle_set_cond(&fbe_base_physical_drive_lifecycle_const,
                           (fbe_base_object_t*)base_physical_drive,
                           FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_INITIATE_HEALTH_CHECK);
}


/*!**************************************************************
 * @fn fbe_base_drive_is_remap_service_time_exceeded
 ****************************************************************
 * @brief
 *  This function tests if the remap service time has exceeded.  
 * 
 * @param packet - ptr to packet
 * @param base_physical_drive - ptr to this object
 * 
 * @return fbe_bool_t - FBE_TRUE if health check has been initated.
 *
 * @author
 *  11/08/2012  Wayne Garrett - Created.
 *
 ****************************************************************/
fbe_bool_t 
fbe_base_drive_is_remap_service_time_exceeded(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_time_t  service_time_limit_ms;

    service_time_limit_ms = fbe_dcs_get_remap_svc_time_limit();
    if (fbe_base_physical_drive_dieh_is_emeh_mode(base_physical_drive)) {
        service_time_limit_ms = fbe_dcs_get_emeh_svc_time_limit();
    }

    return fbe_base_drive_is_service_time_exceeded(base_physical_drive, packet, service_time_limit_ms);
}

/*!**************************************************************
 * @fn fbe_base_pdo_is_system_drive
 ****************************************************************
 * @brief
 *  This function checks if it is a system drive; 0_0_0..0_0_3 
 * 
 * @param base_physical_drive - ptr to this object
 * 
 * @return fbe_bool_t - FBE_TRUE if it's a system drive
 *
 * @author
 *  06/05/2014  Wayne Garrett - Created.
 *
 ****************************************************************/
fbe_bool_t
fbe_base_pdo_is_system_drive(fbe_base_physical_drive_t * base_physical_drive)
{
    if (base_physical_drive->port_number == 0 &&
        base_physical_drive->enclosure_number == 0 &&
        base_physical_drive->slot_number >= 0 &&
        base_physical_drive->slot_number <= 3)
    {
        return FBE_TRUE;
    }
    else
    {
        return FBE_FALSE;
    }
}

/* TLA's for legacy SED drives which did not use key mechanism in TLA suffix
 * for SED identification.
 */
FBE_TLA_IDs fbe_legacy_SED_TLA_tbl[] =
{
	"005050454",
	"005049574",
	"005050457",
	"005050243",
	"005049583",
	"005049584",
	"005050242",
	"005049581",
	"005050455",
	"005049582",
	"005049582",
	"005050630",
	"005050632",
	"005050634",
	"005050637",
	"005050638",
	"005050454",
	"005050455",
	"005050457",
	"005050456",
	"005050458",
	FBE_DUMMY_TLA_ENTRY /* Dummy last array entry - MUST be last entry to exit loop search */
};

/**************************************************************************
* fbe_base_physical_drive_is_sed_drive_tla()                  
***************************************************************************
*
* DESCRIPTION
*       Function to check if the TLA number is SED drive's TLA.
*       
* PARAMETERS
*		base_physical_drive - Pointer to base physical drive object
*       tla - Pointer to tla that needs to be evaluated.
*
* RETURN VALUES
*       True - If TLA matches that of a SED drive
*		False - If it does not
*
* HISTORY
*   10/29/2013 hakimh - Created.
***************************************************************************/
fbe_bool_t fbe_base_physical_drive_is_sed_drive_tla(fbe_base_physical_drive_t * base_physical_drive, fbe_u8_t * tla)
{	
	fbe_u32_t index;
	fbe_u32_t suffix_start = FBE_SCSI_INQUIRY_TLA_SIZE - FBE_SPINDOWN_STRING_LENGTH;
	
	if (tla != NULL) {
		// perform lookup of SED tla numbers for legacy SED drives to see 
		// if its SED drive
		for (index=0; (!fbe_equal_memory(fbe_legacy_SED_TLA_tbl[index].tla, FBE_DUMMY_TLA_ENTRY, 9)); 
				index++) {
			if (fbe_equal_memory(tla, fbe_legacy_SED_TLA_tbl[index].tla, 9)) {				
				fbe_base_physical_drive_customizable_trace(base_physical_drive,
						FBE_TRACE_LEVEL_DEBUG_HIGH,
						"%s match found: drive tla %s tla from table %s\n",
						__FUNCTION__, tla, fbe_legacy_SED_TLA_tbl[index].tla);
				return FBE_TRUE;
			}
		}
		
		for(index = 0; index < FBE_TLA_SUFFIX_LENGTH; index++) {
			if(*(tla+suffix_start+index)==FBE_TLA_SED_SUFFIX_KEY_CHAR)
				return FBE_TRUE;				
		}
		
	}
	
	return FBE_FALSE;
}


fbe_status_t fbe_base_pdo_send_discovery_edge_control_packet(fbe_base_physical_drive_t * base_physical_drive,
                                                             fbe_payload_control_operation_opcode_t control_code,
                                                             fbe_payload_control_buffer_t buffer,
                                                             fbe_payload_control_buffer_length_t buffer_length,                                                
                                                             fbe_packet_attr_t attr)
{
    fbe_status_t                        status = FBE_STATUS_OK, send_status;
    fbe_packet_t * packet_p = NULL;
    fbe_payload_ex_t *                  payload_p = NULL;
    fbe_payload_control_operation_t *   control_operation_p = NULL;    
    fbe_payload_control_status_t        control_status;
    fbe_cpu_id_t  cpu_id;

    /* Allocate packet.*/
    packet_p = fbe_transport_allocate_packet();
    if (packet_p == NULL) {
        fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_ERROR,
                        "%s: unable to allocate packet\n", 
                        __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_initialize_packet(packet_p);
    payload_p = fbe_transport_get_payload_ex (packet_p);
    control_operation_p = fbe_payload_ex_allocate_control_operation(payload_p);

    cpu_id = fbe_get_cpu_id();
    fbe_transport_set_cpu_id(packet_p, cpu_id);
    fbe_payload_control_build_operation (control_operation_p,
                                         control_code,
                                         buffer,
                                         buffer_length);
        
    fbe_transport_set_packet_attr(packet_p, attr);

    fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);


    send_status = fbe_discovery_transport_send_control_packet(&((fbe_base_discovered_t *)base_physical_drive)->discovery_edge, packet_p);

    if ((send_status != FBE_STATUS_OK)&&(send_status != FBE_STATUS_NO_OBJECT) && (send_status != FBE_STATUS_PENDING) && (send_status != FBE_STATUS_MORE_PROCESSING_REQUIRED)) {
        fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_WARNING,                        
                       "%s: error in sending 0x%x, packet_status:0x%x\n",
                       __FUNCTION__, control_code, send_status);  
    }

    fbe_transport_wait_completion(packet_p);


    fbe_payload_control_get_status(control_operation_p, &control_status);
    if (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK ) {
        fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_WARNING,
                       "%s: error in sending 0x%x, control_status:0x%x\n",
                       __FUNCTION__, control_code, control_status);

        status = FBE_STATUS_GENERIC_FAILURE;    
    }


    /* free the memory*/
    fbe_payload_ex_release_control_operation(payload_p, control_operation_p);
    fbe_transport_release_packet(packet_p);
    return status;
}


fbe_status_t fbe_base_physical_drive_get_platform_info(fbe_base_physical_drive_t * base_physical_drive,
                                                       SPID_PLATFORM_INFO *platform_info)
{
    fbe_status_t                            status;
    fbe_board_mgmt_platform_info_t          board_platform_info;

    /* Get the resume info of the board */    
    status = fbe_base_pdo_send_discovery_edge_control_packet(base_physical_drive,
                                                             FBE_BASE_BOARD_CONTROL_CODE_GET_PLATFORM_INFO,
                                                             &board_platform_info,
                                                             sizeof(fbe_board_mgmt_platform_info_t),                                     
                                                             FBE_PACKET_FLAG_TRAVERSE);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    *platform_info = board_platform_info.hw_platform_info;

    return FBE_STATUS_OK;
}


