/***************************************************************************
 * Copyright (C) EMC Corporation 2009 
 * All rights reserved. 
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_src_physical_drive_cmds.c
 ***************************************************************************
 *
 * @brief
 *  This file contains cli functions for the physical drive object.
 *
 * @ingroup fbe_cli
 *
 * @revision
 *   06/12/2009:  chenl6 - created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include <stdio.h>
#include "fbe_cli_private.h"
#include "fbe_trace.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe_transport_trace.h"
#include "fbe_base_object_trace.h"
#include "fbe_block_transport_trace.h"
#include "fbe/fbe_api_trace.h"
#include "fbe/fbe_file.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_api_common.h"
#include "fbe_cli_physical_drive_obj.h"
#include "fbe/fbe_api_esp_drive_mgmt_interface.h"
#include "fbe/fbe_api_block_transport_interface.h"

#define RCV_DIAG_DEFAULT 0X82
#define QDEPTH_DEFAULT 0xa
#define POWER_CYCLE_DEFAULT 0
#define POWER_CYCLE_MAX 127
#define DISPLAY_ABR 1

/*! @enum fbe_cli_pdo_cmd_type  
 *  @brief This is the set of operations that we can perform via the
 *         physical drive cli command.
 */
typedef enum 
{
    FBE_CLI_PDO_CMD_TYPE_INVALID = 0,
    FBE_CLI_PDO_CMD_TYPE_DISPLAY_INFO,
    FBE_CLI_PDO_CMD_TYPE_SET_STATE,
    FBE_CLI_PDO_CMD_TYPE_SET_TRACE_LEVEL,
    FBE_CLI_PDO_CMD_TYPE_POWER_CYCLE,
    FBE_CLI_PDO_CMD_TYPE_FORCE_RECOVERY,
    FBE_CLI_PDO_CMD_TYPE_RESET_DRIVE,
    FBE_CLI_PDO_CMD_TYPE_RESET_SLOT,
    FBE_CLI_PDO_CMD_TYPE_RECEIVE_DIAG_INFO,
    FBE_CLI_PDO_CMD_TYPE_GET_DISK_LOG,  
    FBE_CLI_PDO_CMD_TYPE_GET_QDEPTH,
    FBE_CLI_PDO_CMD_TYPE_SET_QDEPTH,
    FBE_CLI_PDO_CMD_TYPE_GET_DRIVE_LOG,
    FBE_CLI_PDO_CMD_TYPE_DC_RCV_DIAG_ENABLED,
    FBE_CLI_PDO_CMD_TYPE_FORCE_HEALTH_CHECK,
    FBE_CLI_PDO_CMD_TYPE_FORCE_HEALTH_CHECK_TEST,
    FBE_CLI_PDO_CMD_TYPE_DRIVE_FAULT_SET,
    FBE_CLI_PDO_CMD_TYPE_LINK_FAULT_UPDATE,
    FBE_CLI_PDO_CMD_TYPE_GET_MODE_PAGES,
    FBE_CLI_PDO_CMD_TYPE_GET_RLA_ABORT_REQUIRED,
    FBE_CLI_PDO_CMD_TYPE_MAX
}
fbe_cli_pdo_cmd_type;

/*! @enum fbe_cli_pdo_display_format_t  
 *  @brief Different ways we can display data.
 */
typedef enum
{
    FBE_CLI_PDO_DISPLAY_NORMAL = 0,
    FBE_CLI_PDO_DISPLAY_RAW = 1,
} fbe_cli_pdo_display_format_t;


/*! @enum fbe_cli_mode_sense_control_value_t  
 *  @brief defines the Page Control bits for Mode Sense command
 */
typedef fbe_u8_t fbe_cli_mode_sense_control_value_t;
#define   FBE_CLI_MODE_SENSE_CURRENT_PAGES     ((fbe_cli_mode_sense_control_value_t)0x0 << 6)
#define   FBE_CLI_MODE_SENSE_CHANGEABLE_PAGES  ((fbe_cli_mode_sense_control_value_t)0x1 << 6)
#define   FBE_CLI_MODE_SENSE_DEFAULT_PAGES     ((fbe_cli_mode_sense_control_value_t)0x2 << 6)
#define   FBE_CLI_MODE_SENSE_SAVED_PAGES       ((fbe_cli_mode_sense_control_value_t)0x3 << 6)



/***********************************************
 *   LOCAL FUNCTION PROTOTYPES.
 ***********************************************/
static fbe_cli_command_fn fbe_cli_physical_drive_get_command_function(fbe_cli_pdo_cmd_type cmd_type);
static fbe_status_t fbe_cli_physical_drive_display_info(fbe_u32_t object_id, fbe_u32_t context, fbe_package_id_t package_id);
static fbe_status_t fbe_cli_physical_drive_power_cycle(fbe_u32_t object_id, fbe_u32_t context, fbe_package_id_t package_id);
static fbe_status_t fbe_cli_physical_drive_receive_diag_info(fbe_u32_t object_id, fbe_u32_t context, fbe_package_id_t package_id);
static fbe_status_t fbe_cli_physical_drive_force_recovery(fbe_u32_t object_id, fbe_u32_t context, fbe_package_id_t package_id);
static fbe_status_t fbe_cli_physical_drive_reset_drive(fbe_u32_t object_id, fbe_u32_t context, fbe_package_id_t package_id);
static fbe_status_t fbe_cli_physical_drive_reset_slot(fbe_u32_t object_id, fbe_u32_t context, fbe_package_id_t package_id);
static fbe_status_t fbe_cli_physical_drive_get_disk_log(fbe_u32_t object_id, fbe_u32_t context, fbe_package_id_t package_id);
static fbe_status_t fbe_cli_physical_drive_get_qdepth(fbe_u32_t object_id, fbe_u32_t context, fbe_package_id_t package_id);
static fbe_status_t fbe_cli_physical_drive_set_qdepth(fbe_u32_t object_id, fbe_u32_t context, fbe_package_id_t package_id);
static fbe_status_t fbe_cli_physical_drive_get_drive_log(fbe_u32_t object_id, fbe_u32_t context, fbe_package_id_t package_id);
static fbe_status_t fbe_cli_physical_drive_dc_rcv_diag_enabled(fbe_u32_t object_id, fbe_u32_t context, fbe_package_id_t package_id);
static fbe_status_t fbe_cli_physical_drive_force_health_check(fbe_u32_t object_id, fbe_u32_t context, fbe_package_id_t package_id);
static fbe_status_t fbe_cli_physical_drive_force_health_check_test(fbe_u32_t object_id, fbe_u32_t context, fbe_package_id_t package_id);
static fbe_status_t fbe_cli_physical_drive_drive_fault_update(fbe_u32_t object_id, fbe_u32_t context, fbe_package_id_t package_id);
static fbe_status_t fbe_cli_physical_drive_link_fault_update(fbe_u32_t object_id, fbe_u32_t context, fbe_package_id_t package_id);
static fbe_status_t fbe_cli_physical_drive_get_mode_pages(fbe_u32_t object_id, fbe_cli_mode_sense_control_value_t pc, fbe_cli_pdo_display_format_t format);
static fbe_status_t fbe_cli_physical_drive_get_rla_abort_required(fbe_u32_t object_id, fbe_u32_t context, fbe_package_id_t package_id);
static fbe_status_t fbe_cli_physical_drive_set_io_throttle_MB(fbe_u32_t object_id, fbe_u32_t io_throttle_MB, fbe_package_id_t package_id);

static fbe_status_t fbe_cli_physical_drive_get_outstanding_io_count(fbe_u32_t object_id);
static fbe_status_t fbe_cli_physical_drive_get_throttle_info(fbe_u32_t object_id);
static fbe_status_t fbe_cli_physical_drive_set_enhanced_queuing_timer(fbe_u32_t object_id, fbe_u32_t lpq_timer, fbe_u32_t hpq_timer);

/*!**************************************************************
 * fbe_cli_physical_drive_get_command_function()
 ****************************************************************
 * @brief
 *  Convert a physical drive cli command to a command function.
 *
 * @param cmd_type - command to decode.               
 *
 * @return - command function for this command type.
 *
 * @author
 *  06/12/2009 - Created. chenl6
 *
 ****************************************************************/

static fbe_cli_command_fn fbe_cli_physical_drive_get_command_function(fbe_cli_pdo_cmd_type cmd_type)
{
    /* Convert a command type into the function ptr to execute this command.
     */
    switch (cmd_type)
    {
        case FBE_CLI_PDO_CMD_TYPE_SET_STATE:
            return(fbe_cli_set_lifecycle_state);
            break;
        case FBE_CLI_PDO_CMD_TYPE_DISPLAY_INFO:
            return(fbe_cli_physical_drive_display_info);
            break;
        case FBE_CLI_PDO_CMD_TYPE_SET_TRACE_LEVEL:
            return(fbe_cli_set_trace_level);
            break;
        case FBE_CLI_PDO_CMD_TYPE_POWER_CYCLE:
            return(fbe_cli_physical_drive_power_cycle);
            break;
        case FBE_CLI_PDO_CMD_TYPE_FORCE_RECOVERY:
            return(fbe_cli_physical_drive_force_recovery);
            break;
        case FBE_CLI_PDO_CMD_TYPE_RESET_DRIVE:
            return(fbe_cli_physical_drive_reset_drive);
            break;
        case FBE_CLI_PDO_CMD_TYPE_RESET_SLOT:
            return(fbe_cli_physical_drive_reset_slot);
            break;
        case FBE_CLI_PDO_CMD_TYPE_RECEIVE_DIAG_INFO:
            return(fbe_cli_physical_drive_receive_diag_info);
            break;
        case FBE_CLI_PDO_CMD_TYPE_GET_DISK_LOG:
            return(fbe_cli_physical_drive_get_disk_log);
            break;
        case FBE_CLI_PDO_CMD_TYPE_GET_QDEPTH:
            return(fbe_cli_physical_drive_get_qdepth);
            break;
        case FBE_CLI_PDO_CMD_TYPE_SET_QDEPTH:
            return(fbe_cli_physical_drive_set_qdepth);
            break;
        case FBE_CLI_PDO_CMD_TYPE_GET_DRIVE_LOG:
            return(fbe_cli_physical_drive_get_drive_log);
            break;
        case FBE_CLI_PDO_CMD_TYPE_DC_RCV_DIAG_ENABLED:
            return(fbe_cli_physical_drive_dc_rcv_diag_enabled);
            break;
        case FBE_CLI_PDO_CMD_TYPE_FORCE_HEALTH_CHECK:
            return(fbe_cli_physical_drive_force_health_check);
            break;
        case FBE_CLI_PDO_CMD_TYPE_FORCE_HEALTH_CHECK_TEST:
            return(fbe_cli_physical_drive_force_health_check_test);
            break;
        case FBE_CLI_PDO_CMD_TYPE_DRIVE_FAULT_SET:
            return(fbe_cli_physical_drive_drive_fault_update);
            break;
        case FBE_CLI_PDO_CMD_TYPE_LINK_FAULT_UPDATE:
            return(fbe_cli_physical_drive_link_fault_update);
            break;
        case FBE_CLI_PDO_CMD_TYPE_GET_RLA_ABORT_REQUIRED:
            return(fbe_cli_physical_drive_get_rla_abort_required);
            break;

        case FBE_CLI_PDO_CMD_TYPE_MAX:
        case FBE_CLI_PDO_CMD_TYPE_INVALID:
        default:
            fbe_cli_printf("physical drive cli cmd_type 0x%x no valid command function.\n", cmd_type);
            return(NULL);
    }; /* End of switch */
    return(NULL);
}
/******************************************
 * end fbe_cli_physical_drive_get_command_function()
 ******************************************/

/*!**************************************************************
 * fbe_cli_physical_drive_print_edge_path()
 ****************************************************************
 * @brief
 *  Helper function that prints the edge path based on enum value
 *  and space-formatted to fit within a display cell.
 *
 * @author
 *  08/02/2009 - Created. Armando Vega
 *
 ****************************************************************/
void  fbe_cli_physical_drive_print_edge_path(fbe_path_state_t path_state)
{
    switch(path_state)
    {
        case FBE_PATH_STATE_INVALID:
            fbe_cli_printf("INVALID  ");
            break;
        case FBE_PATH_STATE_ENABLED:
            fbe_cli_printf("ENABLED  ");
            break;
        case FBE_PATH_STATE_DISABLED:
            fbe_cli_printf("DISABLED ");
            break;
        case FBE_PATH_STATE_BROKEN:
            fbe_cli_printf("BROKEN   ");
            break;
        case FBE_PATH_STATE_SLUMBER:
            fbe_cli_printf("SLUMBER  ");
            break;
        case FBE_PATH_STATE_GONE:
            fbe_cli_printf("GONE     ");
            break;
        default:
            fbe_cli_printf("UNKNOWN  ");                
    }
}
/******************************************
 * end fbe_cli_physical_drive_print_edge_path()
 ******************************************/

/*!**************************************************************
 * fbe_cli_physical_drive_print_transport_id()
 ****************************************************************
 * @brief
 *  Helper function that prints the transport id based on enum value
 *  and space-formatted to fit within a display cell.
 *
 * @author
 *  08/02/2009 - Created. Armando Vega
 *
 ****************************************************************/
void  fbe_cli_physical_drive_print_transport_id(fbe_transport_id_t transport_id)
{
    switch(transport_id)
    {
        case FBE_TRANSPORT_ID_INVALID:
            fbe_cli_printf("INVALID   ");
            break;
        case FBE_TRANSPORT_ID_BASE:
            fbe_cli_printf("BASE      ");
            break;
        case FBE_TRANSPORT_ID_DISCOVERY:
            fbe_cli_printf("DISCOVERY ");
            break;
        case FBE_TRANSPORT_ID_SSP:
            fbe_cli_printf("SSP       ");
            break;
        case FBE_TRANSPORT_ID_BLOCK:
            fbe_cli_printf("BLOCK     ");
            break;
        case FBE_TRANSPORT_ID_SMP:
            fbe_cli_printf("SMP       ");
            break;
        case FBE_TRANSPORT_ID_STP:
            fbe_cli_printf("STP       ");
            break;
        case FBE_TRANSPORT_ID_LAST:
            fbe_cli_printf("LAST      ");
            break;
        default:
            fbe_cli_printf("UNKNOWN   ");
    }
}

/*!**************************************************************
 * fbe_cli_physical_drive_display_info()
 ****************************************************************
 * @brief
 *  This function does a terse display of the base object info
 *  suitable for display in a table.
 *
 * @param object_id - the object to display.
 * @param context - 0 - normal display, 1 - abbreviated display,
 *                  2 - abbreviated display with header (first drive)..
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  06/12/2009 - Created. chenl6
 *
 ****************************************************************/

static fbe_status_t fbe_cli_physical_drive_display_info(fbe_u32_t object_id, fbe_u32_t context,fbe_package_id_t package_id)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_status_t block_edge_status = FBE_STATUS_OK;
    fbe_status_t discovery_edge_status = FBE_STATUS_OK;
    fbe_status_t ssp_edge_status = FBE_STATUS_OK;
    fbe_lifecycle_state_t  lifecycle_state;
    fbe_api_trace_level_control_t trace_control;
    fbe_const_class_info_t *class_info_p;
    fbe_port_number_t port = FBE_PORT_NUMBER_INVALID;
    fbe_enclosure_number_t encl = FBE_ENCLOSURE_NUMBER_INVALID;
    fbe_enclosure_slot_number_t slot = FBE_ENCLOSURE_SLOT_NUMBER_INVALID;
    fbe_api_get_discovery_edge_info_t discovery_edge_info = {0};
    fbe_api_get_block_edge_info_t block_edge_info = {0};
    fbe_api_get_ssp_edge_info_t ssp_edge_info = {0};
#ifdef  BLOCKSIZE_512_SUPPORTED
    fbe_api_get_stp_edge_info_t stp_edge_info = {0};
#endif  
    fbe_object_death_reason_t death_reason = FBE_DEATH_REASON_INVALID;
    const fbe_u8_t * death_reason_str = NULL;
    fbe_char_t block_edge_error_str[110] = {0};
    fbe_char_t discovery_edge_error_str[110] = {0};
    fbe_char_t ssp_edge_error_str[110] = {0};
    fbe_bool_t b_death_reason = FBE_FALSE;
    fbe_bool_t b_print_physical_info = FBE_FALSE;
    fbe_bool_t b_print_wpd_info = FBE_FALSE;
    fbe_api_dieh_stats_t  dieh_stats = {0};   
    fbe_object_id_t pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_api_provision_drive_info_t provision_drive_info = {0};
    fbe_physical_drive_information_t physical_drive_info = {0}; 
    fbe_physical_drive_attributes_t attributes = {0};
    fbe_u32_t drive_writes_per_day = 0;

    /* Get our lifecycle state.
     */
    status = fbe_api_get_object_lifecycle_state(object_id, &lifecycle_state, package_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("can't get object lifecycle_state for id [%x], status: %x\n",
                object_id, status);
        return status;
    }

    status  = fbe_api_get_object_death_reason(object_id, &death_reason, &death_reason_str, package_id);
    if ((status == FBE_STATUS_OK) && (death_reason != FBE_DEATH_REASON_INVALID)) {
        b_death_reason = FBE_TRUE;
    }

    /* Fetch the trace level.
     */
    trace_control.trace_type = FBE_TRACE_TYPE_OBJECT;
    trace_control.fbe_id = object_id;
    trace_control.trace_level = FBE_TRACE_LEVEL_INVALID;
    trace_control.trace_flag = FBE_TRACE_FLAG_DEFAULT;

    status = fbe_api_trace_get_level(&trace_control, package_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nFailed to get trace control for object_id: 0x%x status: 0x%x\n",
                object_id, status);
        return status;
    }

    /* Make sure this is a valid object.
     */
    status = fbe_cli_get_class_info(object_id, package_id, &class_info_p);
    if (status != FBE_STATUS_OK)
    {
        /* Just return the above function displayed an error.
         */
        fbe_cli_error("\nFailed to get class info for object_id: 0x%x status: 0x%x\n",
                object_id, status);
        return status;
    }

    status = fbe_cli_get_bus_enclosure_slot_info(object_id,
            class_info_p->class_id,
            &port,
            &encl,
            &slot,
            package_id);

    if (status != FBE_STATUS_OK)
    {
        /* Just return the above function displayed an error.
         */
        fbe_cli_error("\nFailed to get bus, enclosure and slot information for object_id: 0x%x status: 0x%x\n",
                object_id, status);
        return status;
    }

    status = fbe_api_physical_drive_get_drive_information(object_id, &physical_drive_info, FBE_PACKET_FLAG_NO_ATTRIB);

    if (status == FBE_STATUS_OK)
    {
        b_print_physical_info = FBE_TRUE;
        drive_writes_per_day = (fbe_u32_t)(physical_drive_info.drive_parameter_flags & FBE_PDO_FLAG_FLASH_DRIVE_WRITES_PER_DAY);
        if((physical_drive_info.drive_parameter_flags & FBE_PDO_FLAG_FLASH_DRIVE) || 
                            (physical_drive_info.drive_parameter_flags & FBE_PDO_FLAG_MLC_FLASH_DRIVE) ||
                            (physical_drive_info.drive_RPM == 0))
        {
            b_print_wpd_info = FBE_TRUE;
        }
    }
    else
    {
        fbe_cli_error("\nERROR: failed status 0x%x to get physical drive information for object_id: 0x%x\n",
                      status, object_id);
    }


    status = fbe_api_physical_drive_get_attributes(object_id, &attributes);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nERROR: failed status 0x%x to get physical drive attributes for object_id: 0x%x\n",
                      status, object_id);
    }


    /* Print full disk information if abbreviated context is not used
    */
    if (context != DISPLAY_ABR)
    {
        /* Get the discovery edge info
         */
        discovery_edge_status = fbe_api_get_discovery_edge_info(object_id, &discovery_edge_info);

        if (discovery_edge_status != FBE_STATUS_OK)
        {
            sprintf(discovery_edge_error_str,
                "ERROR: failed to get physical drive discovery edge info for object_id: 0x%x\n",
                object_id);

        }

        /* Get the ssp edge info.
         */
        ssp_edge_status = fbe_api_get_ssp_edge_info(object_id, &ssp_edge_info);

        if (ssp_edge_status != FBE_STATUS_OK)
        {
            sprintf(ssp_edge_error_str,
                "ERROR: failed to get physical drive ssp edge info for object_id: 0x%x\n",
                object_id);
        }

       /* Get dieh information
        */
       status =  fbe_api_physical_drive_get_dieh_info(object_id, &dieh_stats, FBE_PACKET_FLAG_NO_ATTRIB);
        if (status != FBE_STATUS_OK) {        
            fbe_cli_error("\nERROR: failed status 0x%x to get DIEH information for object_id: 0x%x\n",
                status, object_id);
        }
        
        /* Get DF and EOL bit info
        */
        
        /* Get the provision drive object using b_e_s info */
        status = fbe_api_provision_drive_get_obj_id_by_location_from_topology(port, encl, slot, &pvd_object_id);
        
        if(status != FBE_STATUS_OK)
        {
            /* Unable to get the object id of the drive */
            fbe_cli_printf ("%s:Failed to get PVD object id of %d_%d_%d ! Sts:%X\n", __FUNCTION__, port, encl, slot, (int)status);
        }
        else
        {    
            status = fbe_api_provision_drive_get_info(pvd_object_id, &provision_drive_info);
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_error("\nFailed to get PVD Info for object ID: 0x%x. \n", pvd_object_id);
                pvd_object_id = FBE_OBJECT_ID_INVALID;
            }

            block_edge_status = fbe_api_get_block_edge_info(pvd_object_id, 0, &block_edge_info, FBE_PACKAGE_ID_SEP_0);

            if (block_edge_status != FBE_STATUS_OK)
              {
                sprintf(block_edge_error_str,
                    "Block Edge information not available. Could not get physical drive block edge info for object_id: 0x%x\n",
                    pvd_object_id);
              }
        }

        fbe_cli_printf("====================================================================================================================================\n");
        fbe_cli_printf("OBJID |B_E_D  |STATE     |CLASS NAME                |VENDOR  |MODEL   |REV |SN                  |TLA         |DCH 0x  |EH|KH|EL|DF|\n");
        
        fbe_cli_printf("0x%-4x|%d_%d_%-2d |%-10s|%-26s|",
            object_id, 
            port, 
            encl, 
            slot,
            fbe_base_object_trace_get_state_string(lifecycle_state),
            class_info_p->class_name);      
    
        /* Print these cells only when physical info is enabled, else fill with N/A.
         */
        if (b_print_physical_info == FBE_TRUE)
        {
            fbe_cli_printf("%-8s|%8.8s|%-4s|%-20s|%-12s|",
                physical_drive_info.vendor_id,                
                physical_drive_info.product_id,
                physical_drive_info.product_rev,
                physical_drive_info.product_serial_num,
                physical_drive_info.tla_part_number);
        }
        else
        {
            fbe_cli_printf("NA      |NA|NA  |NA              |NA         |");
        }

        fbe_cli_printf("%8x|%-2d|%-2d|",    
            dieh_stats.drive_configuration_handle,
            dieh_stats.dieh_state.eol_call_home_sent,
            dieh_stats.dieh_state.kill_call_home_sent);
        
        if (FBE_OBJECT_ID_INVALID == pvd_object_id)
        {
            fbe_cli_printf("%2s|%2s|\n", 
                           "NA", "NA");
        }
        else
        {
            fbe_cli_printf("%2s|%2s|\n", 
                           provision_drive_info.end_of_life_state ? "T" : "F", provision_drive_info.drive_fault_state ? "T" : "F");
        }

        if (b_death_reason)
        {
            fbe_cli_printf("DEATH REASON: (%s)\n", death_reason_str);
        }

        fbe_cli_printf("\nPART NUMBER  |BLOCK SIZE |MMode |MediaThrsh |WPD |\n");
        fbe_cli_printf("%-13s|", physical_drive_info.dg_part_number_ascii);
	fbe_cli_printf("%-11d|", physical_drive_info.block_size);
        fbe_cli_printf("0x%04x|", attributes.maintenance_mode_bitmap);
        fbe_cli_printf("%11d|", dieh_stats.dieh_state.media_weight_adjust);
        if (b_print_wpd_info == FBE_TRUE)
        {
            fbe_cli_printf("%4d|", drive_writes_per_day); 
        }
        else
        {
            fbe_cli_printf("N/A |");
        }
        fbe_cli_printf("\n");

        fbe_cli_printf("\nEDGE INFO|PATH STATE    |PATH ATTR  |TRANSPORT ID  |CLIENT ID/IDX        |SERVER ID/IDx        |\n");

        /* Block Edge row
         */
        if (block_edge_status == FBE_STATUS_OK)
        {
            fbe_cli_printf("Block    | 0x%x ", block_edge_info.path_state);
            fbe_cli_physical_drive_print_edge_path(block_edge_info.path_state);
            fbe_cli_printf("|");
            fbe_cli_printf("0x%-9x|", block_edge_info.path_attr);
            fbe_cli_printf("0x%x ", block_edge_info.transport_id);
            fbe_cli_physical_drive_print_transport_id(block_edge_info.transport_id);
            fbe_cli_printf("|");
            fbe_cli_printf("0x%-8x/0x%-8x|", block_edge_info.client_id, block_edge_info.client_index);
            fbe_cli_printf("0x%-8x/0x%-8x|\n", block_edge_info.server_id, block_edge_info.server_index);
        }
        else
        {
            fbe_cli_error("%s\n",block_edge_error_str);
        }

        /* Discovery Edge row
         */
        if (discovery_edge_status == FBE_STATUS_OK)
        {
            fbe_cli_printf("Discovery| 0x%x ", discovery_edge_info.path_state);
            fbe_cli_physical_drive_print_edge_path(discovery_edge_info.path_state);
            fbe_cli_printf("|");
            fbe_cli_printf("0x%-9x|", discovery_edge_info.path_attr);
            fbe_cli_printf("0x%x ", discovery_edge_info.transport_id);
            fbe_cli_physical_drive_print_transport_id(discovery_edge_info.transport_id);
            fbe_cli_printf("|");
            fbe_cli_printf("0x%-8x/0x%-8x|", discovery_edge_info.client_id, discovery_edge_info.client_index);
            fbe_cli_printf("0x%-8x/0x%-8x|\n", discovery_edge_info.server_id, discovery_edge_info.server_index);
        }
        else
        {
            fbe_cli_error("%s\n",discovery_edge_error_str);
        }

        if ((class_info_p->class_id > FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_FIRST) &&
            (class_info_p->class_id < FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_LAST)  &&
            (ssp_edge_status == FBE_STATUS_OK))
        {
            /* SSP row
            */
            fbe_cli_printf("SSP      | 0x%x ", ssp_edge_info.path_state);
            fbe_cli_physical_drive_print_edge_path(ssp_edge_info.path_state);
            fbe_cli_printf("|");
            fbe_cli_printf("0x%-9x|", ssp_edge_info.path_attr);
            fbe_cli_printf("0x%x ", ssp_edge_info.transport_id);
            fbe_cli_physical_drive_print_transport_id(ssp_edge_info.transport_id);
            fbe_cli_printf("|");        
            fbe_cli_printf("0x%-8x/0x%-8x|", ssp_edge_info.client_id, ssp_edge_info.client_index);
            fbe_cli_printf("0x%-8x/0x%-8x|\n", ssp_edge_info.server_id, ssp_edge_info.server_index);
        }
        else if (ssp_edge_status != FBE_STATUS_OK)
        {
            fbe_cli_error("%s\n",ssp_edge_error_str);
        }

#ifdef  BLOCKSIZE_512_SUPPORTED
        else if ((class_info_p->class_id > FBE_CLASS_ID_SATA_PHYSICAL_DRIVE_FIRST) &&
                (class_info_p->class_id < FBE_CLASS_ID_SATA_PHYSICAL_DRIVE_LAST))
        {
            /* Get the stp edge info.
            */        
            status = fbe_api_get_stp_edge_info(object_id, &stp_edge_info);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_error("\nERROR: failed to get physical drive stp edge info for object_id: 0x%x\n",
                    object_id);
                return status;
            }
            /* STP row
            */
            fbe_cli_printf("STP      | 0x%x ", stp_edge_info.path_state);
            fbe_cli_physical_drive_print_edge_path(stp_edge_info.path_state);
            fbe_cli_printf("|");
            fbe_cli_printf("0x%-9x|", stp_edge_info.path_attr);
            fbe_cli_printf("0x%x ", stp_edge_info.transport_id);
            fbe_cli_physical_drive_print_transport_id(stp_edge_info.transport_id);
            fbe_cli_printf("|");        
            fbe_cli_printf("0x%-8x/0x%-8x|", stp_edge_info.client_id, stp_edge_info.client_index);
            fbe_cli_printf("0x%-8x/0x%-8x|\n", stp_edge_info.server_id, stp_edge_info.server_index);
        }
#endif
    
        fbe_cli_printf("\nDIEH ERR |IO COUNT  |Cummul R/T|Recov Err |Media Err |Hw Err    |HChk Err  |Link Err  |Data Error|Reset     |Pwr Cycle |\n");
    
        fbe_cli_printf("Ratios   |NA        |%-10d|%-10d|%-10d|%-10d|%-10d|%-10d|%-10d|%-10d|%-10d|\n",
            dieh_stats.error_counts.io_error_ratio,
            dieh_stats.error_counts.recovered_error_ratio,
            dieh_stats.error_counts.media_error_ratio,
            dieh_stats.error_counts.hardware_error_ratio,
            dieh_stats.error_counts.healthCheck_error_ratio,
            dieh_stats.error_counts.link_error_ratio,
            dieh_stats.error_counts.data_error_ratio,
            dieh_stats.error_counts.reset_error_ratio,
            dieh_stats.error_counts.power_cycle_error_ratio);

        fbe_cli_printf("Tags     |0x%08llx|0x%-8llX|0x%-8llX|0x%-8llX|0x%-8llX|0x%-8llX|0x%-8llX|0x%-8llX|0x%-8llX|0x%-8llX|\n\n\n",
            dieh_stats.error_counts.drive_error.io_counter,
            dieh_stats.error_counts.drive_error.io_error_tag,
            dieh_stats.error_counts.drive_error.recovered_error_tag,
            dieh_stats.error_counts.drive_error.media_error_tag,
            dieh_stats.error_counts.drive_error.hardware_error_tag,
            dieh_stats.error_counts.drive_error.healthCheck_error_tag,
            dieh_stats.error_counts.drive_error.link_error_tag,
            dieh_stats.error_counts.drive_error.data_error_tag,
            dieh_stats.error_counts.drive_error.reset_error_tag,
            dieh_stats.error_counts.drive_error.power_cycle_error_tag);

        
    }
    else
    {
        fbe_cli_printf("0x%04x", object_id);
        fbe_cli_printf("|%-17s|",fbe_base_object_trace_get_state_string(lifecycle_state));
        fbe_cli_printf("%-26s|", class_info_p->class_name);
        fbe_cli_printf("%d_%d_%-2d |",port,encl,slot);
        if (b_print_physical_info == FBE_TRUE)
        {
            fbe_cli_printf("%-8s|", physical_drive_info.vendor_id);
            fbe_cli_printf("%-8s|", physical_drive_info.product_id);
            fbe_cli_printf("%-4s|", physical_drive_info.product_rev);
            fbe_cli_printf("%-16s|", physical_drive_info.product_serial_num);
            fbe_cli_printf("%-11s\n", physical_drive_info.tla_part_number);
        }
        else
        {
            fbe_cli_printf("%-8s|", "NA");
            fbe_cli_printf("%-8s|", "NA");
            fbe_cli_printf("%-4s|", "NA");
            fbe_cli_printf("%-16s|", "NA");
            fbe_cli_printf("%-11s\n", "NA");
        }
    }

    return status;
}
/******************************************
 * end fbe_cli_physical_drive_display_info()
 ******************************************/

/*!**************************************************************
 * fbe_cli_physical_drive_display_abr_header()
 ****************************************************************
 * @brief
 *  Helper function that displays header for abbreviated drive information
 *
 * @author
 *  05/29/2012 - Created. vegaa
 *
 ****************************************************************/
static void fbe_cli_physical_drive_display_abr_header(void)
{
    fbe_cli_printf("------|-----------------|--------------------------|-------|--------|----------------|----|----------------|------------\n");
    fbe_cli_printf("ObjId |State            |Class name                |B_E_D  |Vendor  |ID              |Rev |SN              |TLA         \n");
    fbe_cli_printf("------|-----------------|--------------------------|-------|--------|----------------|----|----------------|------------\n");
}
/******************************************
 * end fbe_cli_physical_drive_display_abr_header()
 ******************************************/

/*!**************************************************************
 * fbe_cli_physical_drive_display_stats()
 ****************************************************************
 * @brief
 *  This function displays PDO's health statistics.
 *
 * @param object_id - the object to display.
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  11/19/2009 - Created. hakimh
 *  12/15/2011 - Changed fbe_api interface and added more dieh info.  Wayne Garrett
 *
 ****************************************************************/
fbe_status_t fbe_cli_physical_drive_display_stats(fbe_u32_t object_id)
{
    fbe_status_t          status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_dieh_stats_t  dieh_stats = {0};   

    status =  fbe_api_physical_drive_get_dieh_info(object_id, &dieh_stats, FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK) {        
        return status;
    }

    fbe_cli_printf("\nDrive Configuration Handle: 0x%x\n",dieh_stats.drive_configuration_handle);

    fbe_cli_printf("\nError Ratios\n");
    fbe_cli_printf("   Cumulative -        Ratio:%d, Tripped Actions:0x%x\n", dieh_stats.error_counts.io_error_ratio, dieh_stats.dieh_state.category.io.tripped_bitmap);
    fbe_cli_printf("   Recovered Error -   Ratio:%d, Tripped Actions:0x%x\n", dieh_stats.error_counts.recovered_error_ratio, dieh_stats.dieh_state.category.recovered.tripped_bitmap);
    fbe_cli_printf("   Media Error -       Ratio:%d, Tripped Actions:0x%x\n", dieh_stats.error_counts.media_error_ratio, dieh_stats.dieh_state.category.media.tripped_bitmap);
    fbe_cli_printf("   Hardware Error -    Ratio:%d, Tripped Actions:0x%x\n", dieh_stats.error_counts.hardware_error_ratio, dieh_stats.dieh_state.category.hardware.tripped_bitmap);
    fbe_cli_printf("   HealthCheck Error - Ratio:%d, Tripped Actions:0x%x\n", dieh_stats.error_counts.healthCheck_error_ratio, dieh_stats.dieh_state.category.healthCheck.tripped_bitmap);
    fbe_cli_printf("   Link Error -        Ratio:%d, Tripped Actions:0x%x\n", dieh_stats.error_counts.link_error_ratio, dieh_stats.dieh_state.category.link.tripped_bitmap);
    fbe_cli_printf("   Data Error -        Ratio:%d, Tripped Actions:0x%x\n", dieh_stats.error_counts.data_error_ratio, dieh_stats.dieh_state.category.data.tripped_bitmap);
    fbe_cli_printf("   Reset -             Ratio:%d\n", dieh_stats.error_counts.reset_error_ratio);    
    fbe_cli_printf("   Power Cycle -       Ratio:%d\n", dieh_stats.error_counts.power_cycle_error_ratio);

    fbe_cli_printf("\nError Tags\n");
    fbe_cli_printf("   IO Count:              0x%08llx\n", dieh_stats.error_counts.drive_error.io_counter);    
    fbe_cli_printf("   Cumulative Error Tag:  0x%llX\n", (unsigned long long)dieh_stats.error_counts.drive_error.io_error_tag);
    fbe_cli_printf("   Recovered Error Tag:   0x%llX\n", (unsigned long long)dieh_stats.error_counts.drive_error.recovered_error_tag);
    fbe_cli_printf("   Media Error Tag:       0x%llX\n", (unsigned long long)dieh_stats.error_counts.drive_error.media_error_tag);
    fbe_cli_printf("   Hardware Error Tag:    0x%llX\n", (unsigned long long)dieh_stats.error_counts.drive_error.hardware_error_tag);
    fbe_cli_printf("   HealthCheck Error Tag: 0x%llX\n", (unsigned long long)dieh_stats.error_counts.drive_error.healthCheck_error_tag);
    fbe_cli_printf("   Link Error Tag:        0x%llX\n", (unsigned long long)dieh_stats.error_counts.drive_error.link_error_tag);
    fbe_cli_printf("   Data Error Tag:        0x%llX\n", (unsigned long long)dieh_stats.error_counts.drive_error.data_error_tag);
    fbe_cli_printf("   Reset Error Tag:       0x%llX\n", (unsigned long long)dieh_stats.error_counts.drive_error.reset_error_tag);

    fbe_cli_printf("\nCall Home\n");
    fbe_cli_printf("   DIEH EOL call home sent:  %d\n", dieh_stats.dieh_state.eol_call_home_sent);
    fbe_cli_printf("   DIEH Kill call home sent: %d\n", dieh_stats.dieh_state.kill_call_home_sent);

    return status;
}

/*!**************************************************************
 * fbe_cli_physical_drive_power_cycle()
 ****************************************************************
 * @brief
 *  This function sends a power control command to PDO.
 *
 * @param object_id - the object to display.
 * @param context - power control command.
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  07/31/2009 - Created. chenl6
 *
 ****************************************************************/

static fbe_status_t fbe_cli_physical_drive_power_cycle(fbe_u32_t object_id, fbe_u32_t context, fbe_package_id_t package_id)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_lifecycle_state_t  lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;

    status = fbe_api_get_object_lifecycle_state(object_id, &lifecycle_state, package_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("can't get object lifecycle_state for id [%x], status: %x\n",
                      object_id, status);
        return status;
    }

    if (lifecycle_state != FBE_LIFECYCLE_STATE_READY)
    {
        fbe_cli_error("object id [%x] not in READY state, lifecycle state: %x\n",
                      object_id, lifecycle_state);
        return status;
    }

    status = fbe_api_physical_drive_power_cycle(object_id, context);

    return status;
}
/******************************************
 * end fbe_cli_physical_drive_power_cycle()
 ******************************************/

/*!**************************************************************
 * fbe_cli_physical_drive_force_recovery()
 ****************************************************************
 * @brief
 *  This function sends a power control command to PDO.
 *
 * @param object_id - the object to display.
 * @param context - not used here.
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  08/31/2010 - Created. chenl6
 *
 ****************************************************************/

static fbe_status_t fbe_cli_physical_drive_force_recovery(fbe_u32_t object_id, fbe_u32_t context, fbe_package_id_t package_id)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_lifecycle_state_t  lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;

    status = fbe_api_get_object_lifecycle_state(object_id, &lifecycle_state, package_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("can't get object lifecycle_state for id [%x], status: %x\n",
                      object_id, status);
        return status;
    }

    if (lifecycle_state != FBE_LIFECYCLE_STATE_FAIL)
    {
        fbe_cli_error("object id [%x] not in FAIL state, lifecycle state: %x\n",
                      object_id, lifecycle_state);
        return status;
    }

    status = fbe_api_physical_drive_power_cycle(object_id, 0);

    return status;
}
/*********************************************
 * end fbe_cli_physical_drive_force_recovery()
 *********************************************/

/*!**************************************************************
 * fbe_cli_physical_drive_reset_drive()
 ****************************************************************
 * @brief
 *  This function sends a RESET DRIVE command to PDO.
 *
 * @param object_id - the object to display.
 * @param context - none.
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  09/21/2010 - Created. chenl6
 *
 ****************************************************************/

static fbe_status_t fbe_cli_physical_drive_reset_drive(fbe_u32_t object_id, fbe_u32_t context, fbe_package_id_t package_id)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_lifecycle_state_t  lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;

    status = fbe_api_get_object_lifecycle_state(object_id, &lifecycle_state, package_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("can't get object lifecycle_state for id [%x], status: %x\n",
                      object_id, status);
        return status;
    }

    if (lifecycle_state != FBE_LIFECYCLE_STATE_READY)
    {
        fbe_cli_error("object id [%x] not in READY state, lifecycle state: %x\n",
                      object_id, lifecycle_state);
        return status;
    }

    status = fbe_api_physical_drive_reset(object_id, FBE_PACKET_FLAG_NO_ATTRIB);

    return status;
}
/******************************************
 * end fbe_cli_physical_drive_reset_drive()
 ******************************************/

/*!**************************************************************
 * fbe_cli_physical_drive_reset_slot()
 ****************************************************************
 * @brief
 *  This function sends a RESET PHY command to PDO.
 *
 * @param object_id - the object to display.
 * @param context - none.
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  09/21/2010 - Created. chenl6
 *
 ****************************************************************/

static fbe_status_t fbe_cli_physical_drive_reset_slot(fbe_u32_t object_id, fbe_u32_t context, fbe_package_id_t package_id)
{
    fbe_status_t status = FBE_STATUS_OK;

    status = fbe_api_physical_drive_reset_slot(object_id);

    return status;
}
/******************************************
 * end fbe_cli_physical_drive_reset_slot()
 ******************************************/

/*!**************************************************************
 * fbe_cli_physical_drive_receive_diag_info()
 ****************************************************************
 * @brief
 *  This function sends a RECEIVE DIAG command to PDO.
 *
 * @param object_id - the object to display.
 * @param context - page code.
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  09/11/2009 - Created. chenl6
 *
 ****************************************************************/

static fbe_status_t fbe_cli_physical_drive_receive_diag_info(fbe_u32_t object_id, fbe_u32_t context, fbe_package_id_t package_id)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_physical_drive_mgmt_receive_diag_page_t diag_info = {0};

    /* Get and display diag page info.
     */
    diag_info.page_code = context;
    status = fbe_api_physical_drive_receive_diag_info(object_id, &diag_info);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nERROR: failed status 0x%x to get diag page for object_id: 0x%x\n",
                      status, object_id);
        return status;
    }

    fbe_cli_printf("\n");
    fbe_cli_printf("RECEIVE DIAG PAGE page 0x%x:\n", diag_info.page_code);

    if (diag_info.page_code == 0x82)
    {
        fbe_physical_drive_receive_diag_pg82_t * pg82 = &(diag_info.page_info.pg82);
        fbe_cli_printf("   Initiate Port:              0x%x\n", pg82->initiate_port);
        fbe_cli_printf("   Port A: Invalid Dword:      0x%x\n", pg82->port[0].invalid_dword_count);
        fbe_cli_printf("   Port A: Loss of Sync:       0x%x\n", pg82->port[0].loss_sync_count);
        fbe_cli_printf("   Port A: PHY Reset:          0x%x\n", pg82->port[0].phy_reset_count);
        fbe_cli_printf("   Port A: Invalid CRC:        0x%x\n", pg82->port[0].invalid_crc_count);
        fbe_cli_printf("   Port A: Disparity Error:    0x%x\n", pg82->port[0].disparity_error_count);
        fbe_cli_printf("   Port B: Invalid Dword:      0x%x\n", pg82->port[1].invalid_dword_count);
        fbe_cli_printf("   Port B: Loss of Sync:       0x%x\n", pg82->port[1].loss_sync_count);
        fbe_cli_printf("   Port B: PHY Reset:          0x%x\n", pg82->port[1].phy_reset_count);
        fbe_cli_printf("   Port B: Invalid CRC:        0x%x\n", pg82->port[1].invalid_crc_count);
        fbe_cli_printf("   Port B: Disparity Error:    0x%x\n", pg82->port[1].disparity_error_count);
    }

    return status;
}
/****************************************************************
 * end fbe_cli_physical_drive_receive_diag_info()
 ****************************************************************/

/*!**************************************************************
 * fbe_cli_physical_drive_get_disk_log()
 ****************************************************************
 * @brief
 *  This function sends a READ command to PDO to retrive the error file.
 *
 * @param object_id - the object to display.
 * @param context - page code.
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  09/1/2010 - Created. CLC
 *
 ****************************************************************/

static fbe_status_t fbe_cli_physical_drive_get_disk_log(fbe_u32_t object_id, fbe_u32_t context, fbe_package_id_t package_id)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_port_number_t port = FBE_PORT_NUMBER_INVALID;
    fbe_enclosure_number_t encl = FBE_ENCLOSURE_NUMBER_INVALID;
    fbe_enclosure_slot_number_t slot = FBE_ENCLOSURE_SLOT_NUMBER_INVALID;
    fbe_file_handle_t file_handle = FBE_FILE_INVALID_HANDLE;
    fbe_u8_t drive_name[30];
    fbe_u8_t data_buffer[DC_TRANSFER_SIZE+1];
    int access_mode = 0;
    fbe_lba_t lba =0;
    fbe_u32_t bytes_to_xfer = DC_TRANSFER_SIZE;
    fbe_u32_t bytes_sent;
    fbe_const_class_info_t *class_info_p = NULL;
    
    /* Only Support SAS drive for now.        */
    status = fbe_cli_get_class_info(object_id, package_id, &class_info_p);

    if ((status != FBE_STATUS_OK) || 
        (class_info_p->class_id <= FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_FIRST) ||
        (class_info_p->class_id >= FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_LAST))
    {
        /* Just return the above function displayed an error.
         */
        fbe_cli_error("This object id 0x%x is not a SAS physical drive but a %s (0x%x).\n",
                      object_id, class_info_p->class_name, class_info_p->class_id);
        return status;
    }
    
    /* Get disk position. */
    status = fbe_cli_get_bus_enclosure_slot_info(object_id, 
                                                 FBE_CLASS_ID_SAS_PHYSICAL_DRIVE,
                                                 &port,
                                                 &encl,
                                                 &slot,
                                                 package_id);
                                                 
    fbe_zero_memory(drive_name, sizeof(drive_name));
    _snprintf(drive_name, sizeof(drive_name), "log_disk%d_%d_%d", port, encl, slot);
    
    /* Create a file.  */
    access_mode |= FBE_FILE_RDWR|(FBE_FILE_CREAT | FBE_FILE_TRUNC);
    if ((file_handle = fbe_file_open(drive_name, access_mode, 0, NULL)) == FBE_FILE_INVALID_HANDLE)
    {
        /* The target file doesn't exist.        */
        fbe_cli_printf("The log_disk%d_%d_%d file is not Existed . \n", port, encl, slot);
        return FBE_STATUS_GENERIC_FAILURE; 
    }
    else
    {
        /* Try to seek to the correct location.*/
        if (fbe_file_lseek(file_handle, 0, 0) == -1)
        {
            fbe_cli_printf("%s drive %s attempt to seek past end of disk.\n",
                                           __FUNCTION__, drive_name);
                                
            status =  FBE_STATUS_GENERIC_FAILURE;
        }
        else
        {
            while (lba < 4096)
            {
                /* Read the data via PDO.  */
                fbe_zero_memory( data_buffer, sizeof(data_buffer));
                status = fbe_api_physical_drive_get_disk_error_log(object_id, lba, data_buffer);
                if (status != FBE_STATUS_OK)
                {
                    fbe_cli_error("\nERROR: failed status 0x%x to get log from disk for object_id: 0x%x\n",
                                           status, object_id);
                    fbe_file_close(file_handle);        
                    return status;
                }
                
                /* Write data to disk. */
                bytes_sent = fbe_file_write(file_handle,
                                           data_buffer,
                                           bytes_to_xfer,
                                           NULL);
                
                /* Check if we transferred enough.
                 */
                if (bytes_sent != bytes_to_xfer)
                {
                    /* Error, we didn't transfer what we expected.
                     */
                    fbe_cli_printf("%s drive %s unexpected transfer. bytes_received = %d, bytes_to_xfer = %d. \n",
                                   __FUNCTION__, drive_name, bytes_sent, bytes_to_xfer);
                                
                    fbe_file_close(file_handle);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                lba += 1;
            }
        }
        
        /* Close the file since we are done with it.
         */
        fbe_file_close(file_handle);
    }
    return status;          
}

/****************************************************************
 * end fbe_cli_physical_drive_get_disk_log()
 ****************************************************************/
 
/*!**************************************************************
 * fbe_cli_physical_drive_get_qdepth()
 ****************************************************************
 * @brief
 *  This function gets queue depth for a PDO.
 *
 * @param object_id - the object to display.
 * @param context - none.
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  09/30/2010 - Created. chenl6
 *
 ****************************************************************/
static fbe_status_t fbe_cli_physical_drive_get_qdepth(fbe_u32_t object_id, fbe_u32_t context, fbe_package_id_t package_id)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t qdepth;

    status = fbe_api_physical_drive_get_object_queue_depth(object_id, &qdepth, FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nERROR: failed status 0x%x to get queue depth for object_id: 0x%x\n",
                      status, object_id);
        return status;
    }

    fbe_cli_printf("\n");
    fbe_cli_printf("Object ID 0x%x: Queue Depth 0x%x\n", object_id, qdepth);

    return status;
}
/******************************************
 * end fbe_cli_physical_drive_get_qdepth()
 ******************************************/

/*!**************************************************************
 * fbe_cli_physical_drive_set_qdepth()
 ****************************************************************
 * @brief
 *  This function sets queue depth for a PDO.
 *
 * @param object_id - the object to display.
 * @param context - queue depth.
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  09/30/2010 - Created. chenl6
 *
 ****************************************************************/
static fbe_status_t fbe_cli_physical_drive_set_qdepth(fbe_u32_t object_id, fbe_u32_t context, fbe_package_id_t package_id)
{
    fbe_status_t status = FBE_STATUS_OK;

    status = fbe_api_physical_drive_set_object_queue_depth(object_id, context, FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nERROR: failed status 0x%x to set queue depth for object_id: 0x%x\n",
                      status, object_id);
        return status;
    }

    return status;
}
/******************************************
 * end fbe_cli_physical_drive_set_qdepth()
 ******************************************/

/*!**************************************************************
 * fbe_cli_physical_drive_get_drive_log()
 ****************************************************************
 * @brief
 *  This function gets drive log (smart dump) for a PDO.
 *
 * @param object_id - the object to display.
 * @param context - none.
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  09/30/2010 - Created. chenl6
 *
 ****************************************************************/
static fbe_status_t fbe_cli_physical_drive_get_drive_log(fbe_u32_t object_id, fbe_u32_t context, fbe_package_id_t package_id)
{
    fbe_status_t status = FBE_STATUS_OK;

    fbe_cli_error("\nERROR: get_drivelog not supported yet\n");

    return status;
}
/******************************************
 * end fbe_cli_physical_drive_get_drive_log()
 ******************************************/

/*!**************************************************************
 * fbe_cli_physical_drive_dc_rcv_diag_enabled()
 ****************************************************************
 * @brief
 *  This function enable collecting receive diagnostic page once per day.
 *
 * @param object_id - the object to display.
 * @param context - enabled(1) or disabled(0).
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  10/10/2010 - Created. CLC
 *
 ****************************************************************/
 
static fbe_status_t fbe_cli_physical_drive_dc_rcv_diag_enabled(fbe_u32_t object_id, fbe_u32_t context, fbe_package_id_t package_id)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_const_class_info_t *class_info_p = NULL;
    
    /* Only Support SAS drive for now.        */
    status = fbe_cli_get_class_info(object_id, package_id, &class_info_p);

    if ((status != FBE_STATUS_OK) || 
        (class_info_p->class_id <= FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_FIRST) ||
        (class_info_p->class_id >= FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_LAST))
    {
        /* Just return the above function displayed an error.
         */
        fbe_cli_error("%s This object id 0x%x is not a SAS physical drive but a %s (0x%x).\n",
                      __FUNCTION__, object_id, class_info_p->class_name, class_info_p->class_id);
        return status;
    }

    status = fbe_api_physical_drive_dc_rcv_diag_enabled(object_id, context);

    return status;
}
/****************************************************************
 * end fbe_cli_physical_drive_dc_rcv_diag_enabled()
 ****************************************************************/
 
/*!**************************************************************
 * fbe_cli_physical_drive_force_health_check()
 ****************************************************************
 * @brief
 *  This function sends a health check control command to PDO.
 *
 * @param object_id - drive object id.
 * @param context - not used here.
 * @param package_id - package id of that object.
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  05/01/2012 - Created. Darren Insko
 *
 ****************************************************************/
 
static fbe_status_t fbe_cli_physical_drive_force_health_check(fbe_u32_t object_id, fbe_u32_t context, fbe_package_id_t package_id)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_const_class_info_t *class_info_p = NULL;
    fbe_lifecycle_state_t  lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    
    /* Only Support SAS drive for now.        */
    status = fbe_cli_get_class_info(object_id, package_id, &class_info_p);

    if ((status != FBE_STATUS_OK) || 
        (class_info_p->class_id <= FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_FIRST) ||
        (class_info_p->class_id >= FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_LAST))
    {
        /* Just return the above function displayed an error.
         */
        fbe_cli_error("%s This object id 0x%x is not a SAS physical drive but a %s (0x%x).\n",
                      __FUNCTION__, object_id, class_info_p->class_name, class_info_p->class_id);
        return status;
    }

    status = fbe_api_get_object_lifecycle_state(object_id, &lifecycle_state, package_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("can't get object lifecycle_state for id [%x], status: %x\n",
                      object_id, status);
        return status;
    }

    if (lifecycle_state != FBE_LIFECYCLE_STATE_READY)
    {
        fbe_cli_error("object id [%x] not in READY state, lifecycle state: %x\n",
                      object_id, lifecycle_state);
        return status;
    }

    status = fbe_api_physical_drive_health_check(object_id);

    return status;
}
/****************************************************************
 * end fbe_cli_physical_drive_force_health_check()
 ****************************************************************/

/*!**************************************************************
 * fbe_cli_physical_drive_force_health_check_test()
 ****************************************************************
 * @brief
 *  This function sends a health check TEST control command to PDO.
 *
 * @param object_id - drive object id.
 * @param context - not used here.
 * @param package_id - package id of that object.
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  05/01/2012 - Created. Darren Insko
 *
 ****************************************************************/
 
static fbe_status_t fbe_cli_physical_drive_force_health_check_test(fbe_u32_t object_id, fbe_u32_t context, fbe_package_id_t package_id)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_const_class_info_t *class_info_p = NULL;
    fbe_lifecycle_state_t  lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    
    /* Only Support SAS drive for now.        */
    status = fbe_cli_get_class_info(object_id, package_id, &class_info_p);

    if ((status != FBE_STATUS_OK) || 
        (class_info_p->class_id <= FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_FIRST) ||
        (class_info_p->class_id >= FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_LAST))
    {
        /* Just return the above function displayed an error.
         */
        fbe_cli_error("%s This object id 0x%x is not a SAS physical drive but a %s (0x%x).\n",
                      __FUNCTION__, object_id, class_info_p->class_name, class_info_p->class_id);
        return status;
    }

    status = fbe_api_get_object_lifecycle_state(object_id, &lifecycle_state, package_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("can't get object lifecycle_state for id [%x], status: %x\n",
                      object_id, status);
        return status;
    }

    if (lifecycle_state != FBE_LIFECYCLE_STATE_READY)
    {
        fbe_cli_error("object id [%x] not in READY state, lifecycle state: %x\n",
                      object_id, lifecycle_state);
        return status;
    }

    status = fbe_api_physical_drive_health_check_test(object_id);

    return status;
}
/****************************************************************
 * end fbe_cli_physical_drive_force_health_check_test()
 ****************************************************************/

/*!**************************************************************
 * fbe_cli_cmd_physical_drive()
 ****************************************************************
 * @brief
 *   This function will allow viewing and changing attributes for a physical drive.
 *  
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @return - none.  
 *
 * @author
 *  06/12/2009 - Created. chenl6
 *
 ****************************************************************/

void fbe_cli_cmd_physical_drive(int argc , char ** argv)
{
    fbe_u32_t object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t port = FBE_API_PHYSICAL_BUS_COUNT;
    fbe_u32_t enclosure = FBE_API_ENCLOSURES_PER_BUS;
    fbe_u32_t slot = FBE_API_ENCLOSURE_SLOTS;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_cli_pdo_cmd_type command = FBE_CLI_PDO_CMD_TYPE_DISPLAY_INFO;
    fbe_lba_t lba = 0;
    fbe_u32_t len = 0;
    fbe_u32_t class_id = 0;
    fbe_u32_t i = 0;
    fbe_bool_t b_target_single = FBE_FALSE;
    fbe_lba_t start_lba = 0;
    fbe_lba_t end_lba = 0;
    fbe_lba_t inc_lba = 0;
    fbe_u32_t args = 0;    
    fbe_cli_pdo_display_format_t display_format = FBE_CLI_PDO_DISPLAY_NORMAL;
    fbe_cli_mode_sense_control_value_t mode_sense_pc = FBE_CLI_MODE_SENSE_SAVED_PAGES;

    /* Defaulted to object order for backward compatibility */
    fbe_cli_pdo_order_t order = OBJECT_ORDER; 

    /* Context for the command.
     */
    fbe_u32_t context = 0;

    fbe_cli_command_fn command_fn = NULL;
    
    /* If a write uncorrect is set will check if it is a SATA drive.
     */
    fbe_bool_t b_write_uncorrectable_set = FBE_FALSE;
    fbe_bool_t b_create_read_uncorrectable = FBE_FALSE;
    fbe_bool_t b_create_read_unc_range = FBE_FALSE;

    /* By default if we are not given any arguments we do a 
     * terse display of all object. 
     * By default we also have -all enabled. 
     */

    /*
     * Parse the command line.
     */
    while (argc > 0)
    {
        if ((strcmp(*argv, "-help") == 0) || (strcmp(*argv, "-h") == 0)) {
            /* If they are asking for help, just display help and exit.
             */
            fbe_cli_printf("%s", PHYSICAL_DRIVE_USAGE);
            return;
        }
        else if (strcmp(*argv, "-all") == 0)
        {
            /*Deprecated parameter, left here to avoid declaring invalid command*/
            fbe_cli_printf("\nCommand -all is not required (deprecated)\n");
        }

        if(strcmp(*argv, "-set_io_throttle") == 0){
            argc--;
            argv++;
            if(argc == 0){
                fbe_cli_error("-set_io_throttle expected throttle max argument in MB \n");
                return;
            }
            i = (fbe_u32_t)strtoul(*argv, 0, 0);
            for (class_id = FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_FIRST +1; class_id < FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_LAST; class_id++) {
                fbe_cli_execute_command_for_objects(class_id,
                                                     FBE_PACKAGE_ID_PHYSICAL,
                                                     FBE_API_PHYSICAL_BUS_COUNT,
                                                     FBE_API_ENCLOSURES_PER_BUS,
                                                     FBE_API_ENCLOSURE_SLOTS,
                                                     fbe_cli_physical_drive_set_io_throttle_MB,
                                                     i);
            }
            return;
        }

        if(strcmp(*argv, "-mode_select") == 0)
        {
            status = fbe_api_get_physical_drive_object_id_by_location(port, enclosure, slot, &object_id);
            if ((status != FBE_STATUS_OK) || (object_id == FBE_OBJECT_ID_INVALID)) {
                /* Just return the above function displayed an error. */
                fbe_cli_error("\nFailed to get physical drive object handle for port: %d enclosure: %d slot: %d\n", port, enclosure, slot);
                return;
            }

            fbe_cli_printf("mode select %d_%d_%d \n", port, enclosure, slot);
            fbe_api_physical_drive_mode_select(object_id);
            return;
        }

        if ((strcmp(*argv, "-get_io_count") == 0) ||
            (strcmp(*argv, "-get_io_count_verbose") == 0)){
            fbe_bool_t verbose = FBE_FALSE;
            if (strcmp(*argv, "-get_io_count_verbose") == 0){
                verbose = FBE_TRUE;
            }
            argc--;
            argv++;
            if(argc == 0){
                fbe_cli_error("-get_io_count expected position x_x_x; too few arguments \n");
                return;
            }

            if(sscanf(*argv,"%d_%d_%d", &port, &enclosure, &slot) != 3){
                fbe_cli_error("-get_io_count expected position x_x_x; invalid format %s \n", *argv);
                return;
            }

            status = fbe_api_get_physical_drive_object_id_by_location(port, enclosure, slot, &object_id);
            if ((status != FBE_STATUS_OK) || (object_id == FBE_OBJECT_ID_INVALID)) {
                /* Just return the above function displayed an error. */
                fbe_cli_error("\nFailed to get physical drive object handle for port: %d enclosure: %d slot: %d\n", port, enclosure, slot);
                return;
            }
            fbe_cli_printf("Object ID 0x%x: port: %d enclosure: %d slot: %d\n", object_id, port, enclosure, slot);
            for(i = 0; i < 20; i++){
                if (verbose){
                    fbe_cli_physical_drive_get_throttle_info(object_id);
                }
                else {
                    fbe_cli_physical_drive_get_outstanding_io_count(object_id);
                }
                fbe_api_sleep(200);
            }

            return;
        }

        if(strcmp(*argv, "-set_queuing_timer") == 0)
        {
            fbe_u32_t lpq_timer, hpq_timer;

            if (object_id == FBE_OBJECT_ID_INVALID) {
                status = fbe_api_get_physical_drive_object_id_by_location(port, enclosure, slot, &object_id);
                if ((status != FBE_STATUS_OK) || (object_id == FBE_OBJECT_ID_INVALID)) {
                    /* Just return the above function displayed an error. */
                    fbe_cli_error("\nFailed to get physical drive object handle for port: %d enclosure: %d slot: %d\n", port, enclosure, slot);
                    return;
                }
            }

            argc--;
            argv++;
            if(argc != 2)
            {
                fbe_cli_error("-set_queuing_timer, expected lpq_timer hpq_timer, too few arguments \n");
                return;
            }

            lpq_timer = (fbe_u32_t)strtoul(argv[0], 0, 0);
            hpq_timer = (fbe_u32_t)strtoul(argv[1], 0, 0);
            fbe_cli_physical_drive_set_enhanced_queuing_timer(object_id, lpq_timer, hpq_timer);
            return;
        }

        if ((strcmp(*argv, "-object") == 0) || (strcmp(*argv, "-o") == 0))
        {
            /* Filter by one object id.
             */
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("-object, expected object_id, too few arguments \n");
                return;
            }
            object_id = (fbe_u32_t)strtoul(*argv, 0, 0);
            b_target_single = FBE_TRUE;
        }
        else if ((strcmp(*argv, "-p") == 0) || (strcmp(*argv, "-b") == 0))
        {
            /* Get Port argument.
             */
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("-p, expected port, too few arguments \n");
                return;
            }
            port = (fbe_u32_t)strtoul(*argv, 0, 0);
            if (port >= FBE_API_PHYSICAL_BUS_COUNT)
            {
                fbe_cli_error("\nInvalid port number, valid range is 0 to %d\n", FBE_API_PHYSICAL_BUS_COUNT);
                return;
            }
        }
        else if (strcmp(*argv, "-e") == 0)
        {
            /* Get Enclosure argument.
             */
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("-e, expected enclosure, too few arguments \n");
                return;
            }
            enclosure = (fbe_u32_t)strtoul(*argv, 0, 0);
        if (enclosure >= FBE_API_ENCLOSURES_PER_BUS)
            {
                fbe_cli_error("\nInvalid enclosure number, valid range is 0 to %d\n", FBE_API_ENCLOSURES_PER_BUS);
                return;
            }
        }
        else if ((strcmp(*argv, "-s") == 0) || (strcmp(*argv, "-d") == 0))
        {
            /* Get Slot argument.
             */
            argc--;
            argv++;
            if (argc == 0)
            {
                fbe_cli_error("-s, expected slot, too few arguments \n");
                return;
            }
            slot = (fbe_u32_t)strtoul(*argv, 0, 0);
            if (slot >= FBE_API_ENCLOSURE_SLOTS)
            {
                fbe_cli_error("\nInvalid slot number, valid range is 0 to %d\n", FBE_API_ENCLOSURE_SLOTS);
                return;
            }
        }
        else if ((strcmp(*argv, "-trace_level") == 0) ||
                 (strcmp(*argv, "-t") == 0))
        {
            /* Get Trace Level argument.
             */
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("-trace_level, expected trace level argument. \n");
                return;
            }
            /* Context is the new trace level to set.
             */
            context = (fbe_u32_t)strtoul(*argv, 0, 0);
            command = FBE_CLI_PDO_CMD_TYPE_SET_TRACE_LEVEL;
        }
        else if ((strcmp(*argv, "-set_state") == 0) ||
                 (strcmp(*argv, "-set") == 0))
        {
            /* Get lifecycle state argument.
             */
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("-set_state, expected lifecycle state argument. \n");
                return;
            }
            /* Context is the lifecycle state to set to.
             */
            context = (fbe_u32_t)strtoul(*argv, 0, 0);
            command = FBE_CLI_PDO_CMD_TYPE_SET_STATE;
        }
        else if ((strcmp(*argv, "-set_write_uncorrectable") == 0) ||
                 (strcmp(*argv, "-set_writeunc") == 0))
        {
            /* Send a Write Uncorrectable to SATA with a lba.
             */
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("-set_write_uncorrectable, expected lba arguments. \n");
                return;
            }
            
            /* The next argument is the LBA value(always interpreted in HEX).
             */
            if ((**argv == '0') && (*(*argv + 1) == 'x' || *(*argv + 1) == 'X'))
            {
                *argv += 2;
            }
            
            /* Context is the lba to set to.
             */
            len = (fbe_u32_t)strlen(*argv);  
            if ((lba = ((fbe_lba_t)_strtoui64(*argv, argv+len, 16))) == -1)
            {
                fbe_cli_error("-set_write_uncorrectable, Invalid argument (LBA).\n");
                return;
            }
        
            /* command = FBE_CLI_PDO_CMD_TYPE_SET_WRITE_UNCORRECTABLE; */
            b_target_single = FBE_TRUE;
            b_write_uncorrectable_set = FBE_TRUE;
        }
        else if ((strcmp(*argv, "-create_read_unc") == 0) ||
                 (strcmp(*argv, "-create_read_uncorrectable") == 0))
        {
            /* Use a write Long command to create a read uncorrectable error at a 
             * given lba for a specified drive.
             */

            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("An LBA argument is required. \n");
                return;
            }
            
            /* The next argument is the LBA value(always interpreted in HEX).
            */
            if ((**argv == '0') && (*(*argv + 1) == 'x' || *(*argv + 1) == 'X'))
            {
                *argv += 2;
            }
            
            /* Context is the lba to set to.
            */
            len = (fbe_u32_t)strlen(*argv);  
            if ((lba = ((fbe_lba_t)_strtoui64(*argv, argv+len, 16))) == -1)
            {
                fbe_cli_error("The LBA argument is invalid.\n");
                return;
            }
           
            b_target_single = FBE_TRUE;
            b_create_read_uncorrectable = FBE_TRUE;
            
        }       
        else if (strcmp(*argv, "-create_read_unc_range") == 0)
        {
            /* Use a write Long command to create a read uncorrectable error over a 
             * range of lba's for a specified drive.
             */

            for (args = 1; args <=3; args++)
            {
               argc--;
               argv++;
                if(argc == 0)
                {
                    fbe_cli_error("An LBA argument is required. \n");
                    return;
                }
            
                /* The next argument is the LBA value(always interpreted in HEX).
                */
                if ((**argv == '0') && (*(*argv + 1) == 'x' || *(*argv + 1) == 'X'))
                {
                    *argv += 2;
                }
            
                len = (fbe_u32_t)strlen(*argv);
                if (args == 1)
                {
                    if ((start_lba = ((fbe_lba_t)_strtoui64(*argv, argv+len, 16))) == -1)
                    {
                        fbe_cli_error("The starting LBA argument is invalid.\n");
                        return;
                    }
                }
                if (args == 2)
                {
                    if ((end_lba = ((fbe_lba_t)_strtoui64(*argv, argv+len, 16))) == -1)
                    {
                        fbe_cli_error("The ending LBA argument is invalid.\n");
                        return;
                    }
                }
                if (args == 3)
                {
                    if ((inc_lba = ((fbe_lba_t)_strtoui64(*argv, argv+len, 16))) == -1)
                    {
                        fbe_cli_error("The increment LBA argument is invalid.\n");
                        return;
                    }
                }
            }

            b_target_single = FBE_TRUE;
            b_create_read_unc_range = FBE_TRUE;
        }

        else if ((strcmp(*argv, "-display") == 0) ||
                 (strcmp(*argv, "-d") == 0))
        {
            /* Just display a table of info on these objects.
             */
            command = FBE_CLI_PDO_CMD_TYPE_DISPLAY_INFO;
        }
        else if(strcmp(*argv, "-power_cycle") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                context = POWER_CYCLE_DEFAULT;
            }
            else
            {
                context = (fbe_u32_t)strtoul(*argv, 0, 0);
            }
            if (context > POWER_CYCLE_MAX)
            {
                fbe_cli_error("-power_cycle, invalid duration %d \n", context);
                return;
            }
            command = FBE_CLI_PDO_CMD_TYPE_POWER_CYCLE;
        }
        else if(strcmp(*argv, "-force_recovery") == 0)
        {
            command = FBE_CLI_PDO_CMD_TYPE_FORCE_RECOVERY;
        }
        else if(strcmp(*argv, "-reset_drive") == 0)
        {
            command = FBE_CLI_PDO_CMD_TYPE_RESET_DRIVE;
        }
        else if(strcmp(*argv, "-reset_slot") == 0)
        {
            command = FBE_CLI_PDO_CMD_TYPE_RESET_SLOT;
        }
        else if(strcmp(*argv, "-receive_diag") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                context = RCV_DIAG_DEFAULT;
            }
            else
            {
                context = (fbe_u32_t)strtoul(*argv, 0, 0);
            }
            if (context != RCV_DIAG_DEFAULT)
            {
                fbe_cli_error("-receive_diag, invalid page_code 0x%x \n", context);
                return;
            }
            command = FBE_CLI_PDO_CMD_TYPE_RECEIVE_DIAG_INFO;
        }
        else if(strcmp(*argv, "-get_DC_log") == 0)
        {
            command = FBE_CLI_PDO_CMD_TYPE_GET_DISK_LOG;
        }
        else if(strcmp(*argv, "-get_qdepth") == 0)
        {
            command = FBE_CLI_PDO_CMD_TYPE_GET_QDEPTH;
        }
        else if(strcmp(*argv, "-set_qdepth") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                context = QDEPTH_DEFAULT;
            }
            else
            {
                context = (fbe_u32_t)strtoul(*argv, 0, 0);
            }
            command = FBE_CLI_PDO_CMD_TYPE_SET_QDEPTH;
        }
        else if(strcmp(*argv, "-get_drivelog") == 0)
        {
            command = FBE_CLI_PDO_CMD_TYPE_GET_DRIVE_LOG;
        }
        else if((strcmp(*argv, "-dc_rcv_diag_ena") == 0) ||
                         (strcmp(*argv, "-dcRcv") == 0))
        {
            argc--;
            argv++;
          
            if(argc == 0)
            {
                fbe_cli_error("-dc_rcv_diag_ena, expected 0 for disabled or 1 for enabled (Default is disabled. \n");
                return;
            }
            
            /* Context is the lifecycle state to set to.
             */
            context = (fbe_u32_t)strtoul(*argv, 0, 0);
            
            if ((context != 0) && (context != 1))
            {
                fbe_cli_printf("PDO command invalid argument %s\n", *argv);
                return;
            }
            command = FBE_CLI_PDO_CMD_TYPE_DC_RCV_DIAG_ENABLED;
        }
        else if(strcmp(*argv, "-force_health_check") == 0)
        {
            command = FBE_CLI_PDO_CMD_TYPE_FORCE_HEALTH_CHECK;
        }
        else if(strcmp(*argv, "-sim_test_health_check") == 0)
        {
            command = FBE_CLI_PDO_CMD_TYPE_FORCE_HEALTH_CHECK_TEST;
        }
        else if (strcmp(*argv, "-order") == 0)
        {
            /* Get order argument.
             */
            argc--;
            argv++;
            if (argc == 0)
            {
                fbe_cli_error("-order, expected order value, too few arguments \n");
                return;
            }           

            order = (strcmp(*argv, "p") == 0) ?     PHYSICAL_ORDER : 
                    ((strcmp(*argv, "o") == 0) ?    OBJECT_ORDER : 
                                                    ORDER_UNDEFINED);

            if (ORDER_UNDEFINED == order)
            {
                fbe_cli_error("\nInvalid order parameter, acceptable choices are p (physical) or o (object)\n");
                return;
            }
        }
        else if (strcmp(*argv, "-abr") == 0)
        {
            /* Abbreviated mode only valid for display command, else declare error
             */
            if ( command == FBE_CLI_PDO_CMD_TYPE_DISPLAY_INFO)
            {
                context = DISPLAY_ABR;
            }
            else
            {
                fbe_cli_error("\nAbbreviated parameter only valid on display or default pdo command\n");
                return;
            }
        }
        
        else if((strcmp(*argv, "-set_drive_fault") == 0) ||
                (strcmp(*argv, "-setdf") == 0))
        {
            argc--;
            argv++;           
            
            /* Context is the state to set to.
             */
            context = 1;            
           
            command = FBE_CLI_PDO_CMD_TYPE_DRIVE_FAULT_SET;
        }
        else if((strcmp(*argv, "-link_fault") == 0) ||
                (strcmp(*argv, "-setlf") == 0))
        {
            argc--;
            argv++;
            
            if(argc == 0)
            {
                fbe_cli_error("-link_fault. expected 0 for disabled or 1 for enabled \n");
                return;
            }
            
            /* Context is the state to set to.
             */
            context = (fbe_u32_t)strtoul(*argv, 0, 0);
            
            if ((context != 0) && (context != 1))
            {
                fbe_cli_printf("PDO command invalid argument %s\n", *argv);
                return;
            }
            command = FBE_CLI_PDO_CMD_TYPE_LINK_FAULT_UPDATE;
        }
        else if(strncmp(*argv, "-get_mode_pages", FBE_CLI_PDO_ARG_MAX) == 0)
        {
            command = FBE_CLI_PDO_CMD_TYPE_GET_MODE_PAGES;
        }
        else if(strncmp(*argv, "-pc", FBE_CLI_PDO_ARG_MAX) == 0)
        {
            argc--;
            argv++;

            if(argc == 0)
            {
                fbe_cli_error("-pc. missing paramater \n");
                return;
            }
            if (strncmp(*argv, "current", FBE_CLI_PDO_ARG_MAX) == 0)
            {
                mode_sense_pc = FBE_CLI_MODE_SENSE_CURRENT_PAGES;
            }
            else if (strncmp(*argv, "changeable", FBE_CLI_PDO_ARG_MAX) == 0)
            {
                mode_sense_pc = FBE_CLI_MODE_SENSE_CHANGEABLE_PAGES;
            }
            else if (strncmp(*argv, "default", FBE_CLI_PDO_ARG_MAX) == 0)
            {
                mode_sense_pc = FBE_CLI_MODE_SENSE_DEFAULT_PAGES;
            }
            else if (strncmp(*argv, "saved", FBE_CLI_PDO_ARG_MAX) == 0)
            {
                mode_sense_pc = FBE_CLI_MODE_SENSE_SAVED_PAGES;
            }
            else
            {
                fbe_cli_error("-pc. invalid paramater \n");
                return;
            }
        }
        else if (strncmp(*argv, "-raw", FBE_CLI_PDO_ARG_MAX) == 0)
        {
            display_format = FBE_CLI_PDO_DISPLAY_RAW;            
        }
        else if(strcmp(*argv, "-get_rla_abort_required") == 0)
        {
            command = FBE_CLI_PDO_CMD_TYPE_GET_RLA_ABORT_REQUIRED;
        }
        else
        {
            fbe_cli_printf("PDO command invalid argument %s\n", *argv);
            return;
        }

        argc--;
        argv++;

    }   /* end of while */

    /*
     * Check if only a single drive is selected by looking at the p/e/s settings.
     * If one or more are used and not already set to single by another parameter
     * then set to a drive sub-set. Note that if a value higher than the max capacity
     * at this point it is treated as a disabled filter for a port, enclosure or slot.
     */
    if ((port < FBE_API_PHYSICAL_BUS_COUNT) && 
        (enclosure < FBE_API_ENCLOSURES_PER_BUS) &&
        (slot < FBE_API_ENCLOSURE_SLOTS))
    {
        /* All three selected, set to single drive */
        b_target_single = FBE_TRUE;
    }
    else if ((!b_target_single) &&
           ((command == FBE_CLI_PDO_CMD_TYPE_POWER_CYCLE) ||
            (command == FBE_CLI_PDO_CMD_TYPE_FORCE_RECOVERY) ||
            (command == FBE_CLI_PDO_CMD_TYPE_RESET_DRIVE) ||
            (command == FBE_CLI_PDO_CMD_TYPE_RESET_SLOT) ||
            (command == FBE_CLI_PDO_CMD_TYPE_RECEIVE_DIAG_INFO) ||
            (command == FBE_CLI_PDO_CMD_TYPE_GET_QDEPTH) ||
            (command == FBE_CLI_PDO_CMD_TYPE_SET_QDEPTH) ||
            (command == FBE_CLI_PDO_CMD_TYPE_GET_DRIVE_LOG) ||
            (command == FBE_CLI_PDO_CMD_TYPE_DRIVE_FAULT_SET) ||
            (command == FBE_CLI_PDO_CMD_TYPE_LINK_FAULT_UPDATE) ||
            (command == FBE_CLI_PDO_CMD_TYPE_GET_MODE_PAGES) ||
            (command == FBE_CLI_PDO_CMD_TYPE_GET_RLA_ABORT_REQUIRED)
            ))
    {
        /* The user input specifies multiple drives,
         * but the command to be issued can only be sent to a single object.
         * Declare error and return.
         */
        fbe_cli_error("Command does not support multiple drives, object or physical location must be specified.\n");
        return;
    }

    if (b_target_single)
    {
        /* The user either specified a specific object via -object 
         * or should have specified a specific drive by location (i.e. p_e_s). 
         * We need to validate that there is a valid drive object passed in or 
         * obtained from physical location.
         */

        fbe_const_class_info_t *class_info_p;

        if (object_id == FBE_OBJECT_ID_INVALID)
        {
            if ((port >= FBE_API_PHYSICAL_BUS_COUNT) ||
                (enclosure >= FBE_API_ENCLOSURES_PER_BUS) ||
                (slot >= FBE_API_ENCLOSURE_SLOTS))
            {
                fbe_cli_error("Command does not support multiple drives, object or physical location must be specified.\n");
                return;
            }

            /* Get the object handle for this target.
             */
            status = fbe_api_get_physical_drive_object_id_by_location(port, enclosure, slot, &object_id);

            if ((status != FBE_STATUS_OK) || (object_id == FBE_OBJECT_ID_INVALID))
            {
                /* Just return the above function displayed an error.
                 */
                fbe_cli_error("\nFailed to get physical drive object handle for port: %d enclosure: %d slot: %d\n",
                              port, enclosure, slot);
                return;
            }
        }
        else if (object_id > FBE_MAX_OBJECTS)
        {
            return;
        }
        else if ((port < FBE_API_PHYSICAL_BUS_COUNT) ||
                 (enclosure < FBE_API_ENCLOSURES_PER_BUS) ||
                 (slot < FBE_API_ENCLOSURE_SLOTS))
        {
            /*Object-id is mutually exclusive with port, enclosure and slot*/
            fbe_cli_error("\nCannot use port, enclosure or slot parameters with object-id at the same time\n");
            return;
        }

        /* Make sure this is a valid physical drive.
         */
        status = fbe_cli_get_class_info(object_id, FBE_PACKAGE_ID_PHYSICAL, &class_info_p);

        if (status != FBE_STATUS_OK)
        {
            /* Just return the above function displayed an error.
             */
            return;
        }

        /* Now that we have all the info, make sure this is a physical drive.
         */
        if ((class_info_p->class_id <= FBE_CLASS_ID_PHYSICAL_DRIVE_FIRST) ||
            (class_info_p->class_id >= FBE_CLASS_ID_PHYSICAL_DRIVE_LAST))
        {
            fbe_cli_error("This object id 0x%x is not a physical drive but a %s (0x%x).\n",
                          object_id, class_info_p->class_name, class_info_p->class_id);
            return;
        }
        
        if ( b_write_uncorrectable_set == FBE_TRUE)
        {
            if (class_info_p->class_id != FBE_CLASS_ID_SATA_PHYSICAL_DRIVE)
            {
                fbe_cli_error("This object id 0x%x is not a SATA physical drive but a %s (0x%x).\n",
                              object_id, class_info_p->class_name, class_info_p->class_id);
            }
            else
            {
                //status = fbe_cli_set_write_uncorrectable(object_id, lba, FBE_PACKAGE_ID_PHYSICAL);
                fbe_cli_set_write_uncorrectable(object_id, lba);
            }
            return;
        }
        else if (b_create_read_uncorrectable)
        {
            if (class_info_p->class_id == FBE_CLASS_ID_SATA_PHYSICAL_DRIVE)
            {
                fbe_cli_error("Command not supported on a SATA drive. Object id: 0x%x\n", object_id);
            }
            else
            {
                fbe_cli_create_read_uncorrectable(object_id, lba);
            }
            return;
        }
        else if (b_create_read_unc_range)
        {
            if (class_info_p->class_id == FBE_CLASS_ID_SATA_PHYSICAL_DRIVE)
            {
                fbe_cli_error("Command not supported on a SATA drive. Object id: 0x%x\n", object_id);
            }
            else
            {
                fbe_cli_create_read_unc_range(object_id, start_lba, end_lba, inc_lba);
            }
            return;
        }
        else if (command == FBE_CLI_PDO_CMD_TYPE_GET_MODE_PAGES)
        {
            fbe_cli_physical_drive_get_mode_pages(object_id, mode_sense_pc, display_format);
            return;
        }
    }

    /* Convert the command to a function.
     */
    command_fn = fbe_cli_physical_drive_get_command_function(command);

    if (command_fn == NULL)
    {
        fbe_cli_error("can't get command function for command %d status: %x\n",
                      command, status);
        return;
    }

    if (b_target_single)
    {
        /* If this is a display command and context is abbreviated, print header
         */
        if ( (command == FBE_CLI_PDO_CMD_TYPE_DISPLAY_INFO) && (context == DISPLAY_ABR))
        {
            fbe_cli_physical_drive_display_abr_header();
        }
        /* Execute the command against this single object.
         */
        status = (command_fn)(object_id, context, FBE_PACKAGE_ID_PHYSICAL);

        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("\nCommand failed to port: %d enclosure: %d slot: %d status: 0x%x.\n", port, enclosure, slot, status);
            return;
        }
    }
    else
    {
        /*
         * Execute command on multiple drives.
         *
         * If this is a display command and context is abbreviated, skip base drives
         */
        if (!((command == FBE_CLI_PDO_CMD_TYPE_DISPLAY_INFO) && (context == DISPLAY_ABR)))
        {
            /* Execute the command against multiple base physical drives if selected.
             */
            status = fbe_cli_execute_command_for_objects_order_defined(FBE_CLASS_ID_BASE_PHYSICAL_DRIVE,
                                                                       FBE_PACKAGE_ID_PHYSICAL,
                                                                       port,
                                                                       enclosure,
                                                                       slot,
                                                                       order,
                                                                       command_fn,
                                                                       context /* no context */);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("\nPhysical drive command to multiple base drives failed status: 0x%x.\n", status);
            }
        }

        /* If this is a display command and context is abbreviated, print header
         */
        if ( (command == FBE_CLI_PDO_CMD_TYPE_DISPLAY_INFO) && (context == DISPLAY_ABR))
        {
            fbe_cli_physical_drive_display_abr_header();
        }
        /* Execute the command on multiple sas physical drives if selected.
         */
        for (class_id = FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_FIRST +1; class_id < FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_LAST; class_id++)
        {
            status = fbe_cli_execute_command_for_objects_order_defined(class_id,
                                                                       FBE_PACKAGE_ID_PHYSICAL,
                                                                       port,
                                                                       enclosure,
                                                                       slot,
                                                                       order,
                                                                       command_fn,
                                                                       context /* no context */);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("\nPhysical drive command to multiple SAS drives failed status: 0x%x.\n", status);
            }
        }

#ifdef  BLOCKSIZE_512_SUPPORTED
        /* If this is a display command and context is abbreviated, skip SATA drives
         */
        if (!((command == FBE_CLI_PDO_CMD_TYPE_DISPLAY_INFO) && (context == DISPLAY_ABR_CONTEXT)))
        {
            /* Execute the command on multiple sata physical drives if selected.
             */
            for (class_id = FBE_CLASS_ID_SATA_PHYSICAL_DRIVE_FIRST +1; class_id < FBE_CLASS_ID_SATA_PHYSICAL_DRIVE_LAST; class_id++)
            {
                status = fbe_cli_execute_command_for_objects_order_defined(class_id,
                                                                           FBE_PACKAGE_ID_PHYSICAL,
                                                                           port,
                                                                           enclosure,
                                                                           slot,
                                                                           order,
                                                                           command_fn,
                                                                           context /* no context */);
                if (status != FBE_STATUS_OK)
                {
                    fbe_cli_printf("\nPhysical drive command to multiple SATA drives failed status: 0x%x.\n", status);
                }
            } /* end for */
        } /* end if */
#endif
    } /* end else */
    return;
}      
/******************************************
 * end fbe_cli_cmd_physical_drive()
 ******************************************/


static fbe_status_t fbe_cli_physical_drive_get_outstanding_io_count(fbe_u32_t object_id)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t outstanding_io_count = 0;

    status = fbe_api_physical_drive_get_outstanding_io_count(object_id, &outstanding_io_count);
    if (status != FBE_STATUS_OK) {
        fbe_cli_error("\nERROR: failed status 0x%x to get outstanding io_count object_id: 0x%x\n", status, object_id);
        return status;
    }

    fbe_cli_printf("Object ID 0x%x: outstanding io_count %d\n", object_id, outstanding_io_count);

    return status;
}
static fbe_status_t fbe_cli_physical_drive_get_throttle_info(fbe_u32_t object_id)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_block_transport_get_throttle_info_t get_throttle;
    fbe_u32_t index;

    status = fbe_api_block_transport_get_throttle_info(object_id, FBE_PACKAGE_ID_PHYSICAL, &get_throttle);
    if (status != FBE_STATUS_OK) {
        fbe_cli_error("\nERROR: failed status 0x%x to get throttle info object_id: 0x%x\n", status, object_id);
        return status;
    }

    fbe_cli_printf("io_throttle_count:     %lld\n", get_throttle.io_throttle_count);
    fbe_cli_printf("io_throttle_max:       %lld\n", get_throttle.io_throttle_max);
    fbe_cli_printf("outstanding_io_count:  %lld\n", get_throttle.outstanding_io_count);
    fbe_cli_printf("outstanding_io_max:    %d\n", get_throttle.outstanding_io_max);
    for (index = 0; index < FBE_PACKET_PRIORITY_LAST - 1; index++)
    {
        fbe_cli_printf("%d) %d ", index, get_throttle.queue_length[index]);
    }
    fbe_cli_printf("\n");
    return status;
}
 
/*!**************************************************************
 * fbe_cli_physical_drive_drive_fault_update
 ****************************************************************
 * @brief
 *  This function set or clear Drive fault for a drive.
 *
 * @param object_id - the object to display.
 * @param context - enabled(1) or disabled(0).
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  07/25/2012 - Created. kothal
 *
 ****************************************************************/
 
static fbe_status_t fbe_cli_physical_drive_drive_fault_update(fbe_u32_t object_id, fbe_u32_t context, fbe_package_id_t package_id)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_const_class_info_t *class_info_p = NULL;
    fbe_bool_t b_set_or_clear_drive_fault = (fbe_bool_t)context;
      
    /* Only Support SAS drive for now.        */
    status = fbe_cli_get_class_info(object_id, package_id, &class_info_p);
 
    if ((status != FBE_STATUS_OK) || 
            (class_info_p->class_id <= FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_FIRST) ||
            (class_info_p->class_id >= FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_LAST))
    {
        /* Just return the above function displayed an error.
         */
        fbe_cli_error("%s This object id 0x%x is not a SAS physical drive but a %s (0x%x).\n",
                      __FUNCTION__, object_id, class_info_p->class_name, class_info_p->class_id);
        return status;
    }
 
    status = fbe_api_physical_drive_update_drive_fault(object_id, b_set_or_clear_drive_fault);
    if (status != FBE_STATUS_OK) {
        fbe_cli_error( "%s: failed to update drive fault flag.\n", __FUNCTION__);   
        return status;
    }
    
    status = fbe_cli_set_lifecycle_state(object_id, FBE_LIFECYCLE_STATE_FAIL, package_id);
    if (status != FBE_STATUS_OK) {
        fbe_cli_error( "%s: failed to set the lifecycle state of PDO to Fail.\n", __FUNCTION__);   
        return status;
    }
    
    fbe_cli_printf("\nSuccessfully set the drive fault flag\n\n");
    return status;    
}

/****************************************************************
 * end fbe_cli_physical_drive_drive_fault_update()
 ****************************************************************/
/*!**************************************************************
 * fbe_cli_physical_drive_link_fault_set
 ****************************************************************
 * @brief
 *  This function set or clear Link fault for a drive.
 *
 * @param object_id - the object to display.
 * @param context - enabled(1) or disabled(0).
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  07/25/2012 - Created. kothal
 *
 ****************************************************************/
 
static fbe_status_t fbe_cli_physical_drive_link_fault_update(fbe_u32_t object_id, fbe_u32_t context, fbe_package_id_t package_id)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_const_class_info_t *class_info_p = NULL;
    fbe_lifecycle_state_t  lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_bool_t b_set_clear_link_fault = (fbe_bool_t)context;
    
    /* Only Support SAS drive for now.        */
    status = fbe_cli_get_class_info(object_id, package_id, &class_info_p);

    if ((status != FBE_STATUS_OK) || 
        (class_info_p->class_id <= FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_FIRST) ||
        (class_info_p->class_id >= FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_LAST))
    {
        /* Just return the above function displayed an error.
         */
        fbe_cli_error("%s This object id 0x%x is not a SAS physical drive but a %s (0x%x).\n",
                      __FUNCTION__, object_id, class_info_p->class_name, class_info_p->class_id);
        return status;
    }
    
    /* Get our lifecycle state.
     */
    status = fbe_api_get_object_lifecycle_state(object_id, &lifecycle_state, package_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("can't get object lifecycle_state for id [%x], status: %x\n",
                object_id, status);
        return status;
    }   
   
    if (context == 0)
    {        
        status = fbe_api_physical_drive_update_link_fault(object_id, b_set_clear_link_fault);
        if (status != FBE_STATUS_OK) {
            fbe_cli_error( "%s: failed to clear link fault flag.\n", __FUNCTION__);   
            return status;
        }        
        
        status = fbe_cli_set_lifecycle_state(object_id, FBE_LIFECYCLE_STATE_FAIL, package_id);
        if (status != FBE_STATUS_OK) {
            fbe_cli_error( "%s: failed to set the lifecycle state of PDO to Activate/Fail.\n", __FUNCTION__);   
            return status;
        }
       
        fbe_cli_printf("\nSuccessfully cleared the link fault flag\n\n");
    }
    else
    { // When PDO goes to ready, it clears the attr ? 
        
        status = fbe_cli_set_lifecycle_state(object_id, FBE_LIFECYCLE_STATE_ACTIVATE, package_id);
        if (status != FBE_STATUS_OK) {
            fbe_cli_error( "%s: failed to set the lifecycle state of PDO to Ready.\n", __FUNCTION__);   
            return status;
        } 
        
        fbe_cli_printf("\nSuccessfully set the link fault flag\n\n");
    }        
    
    return status;  
}
/****************************************************************
 * end fbe_cli_physical_drive_link_fault_set()
 ****************************************************************/

/*!**************************************************************
 * fbe_cli_physical_drive_get_mode_pages
 ****************************************************************
 * @brief
 *  Displays Default mode pages
 *
 * @param object_id - the object to display.
 * @param format - display format
 * @param pc - mode sense page control value.
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  10/10/2013 - Wayne Garrett - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_cli_physical_drive_get_mode_pages(fbe_u32_t object_id, fbe_cli_mode_sense_control_value_t pc, fbe_cli_pdo_display_format_t format)
{
    fbe_status_t status;    
    const fbe_u16_t buffer_size = FBE_CLI_PDO_MAX_MODE_SENSE_10_SIZE;
    fbe_u32_t i = 0;
    const fbe_u32_t mode_pg_header_len = 8;
    fbe_physical_drive_send_pass_thru_t *mode_sense_op;
    fbe_u8_t page_code = 0;
    fbe_u8_t subpage_code = 0;

    mode_sense_op = fbe_api_allocate_memory(sizeof(fbe_physical_drive_send_pass_thru_t));
    if (NULL == mode_sense_op)
    {
        fbe_cli_error("%s failed pass thru alloc.\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_zero_memory(mode_sense_op, sizeof(fbe_physical_drive_send_pass_thru_t));

    /* 1. create CDB to get default mode pages
       2. send pass-thru cmd
       3. display results
    */

    /* 1. create sg buffer and CDB to get default mode pages */       
    mode_sense_op->response_buf = fbe_api_allocate_memory(buffer_size);
    if (NULL == mode_sense_op->response_buf)
    {
        fbe_cli_error("%s failed recv buff alloc.\n",__FUNCTION__);
        fbe_api_free_memory(mode_sense_op);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*TODO: need a generic utility for creating CDBs that can be called anywhere. I 
      envision a self contained scsi library that handles cdb creation, sense data
      parsing and anything else related to managing scsi cmds. -wayne */

    /* Return all subpage 00h mode pages in page_0 format */
    page_code = 0x3F;  
    subpage_code = 0x0;  

    mode_sense_op->cmd.cdb_length = 10;
    mode_sense_op->cmd.cdb[0] = FBE_SCSI_MODE_SENSE_10;
    mode_sense_op->cmd.cdb[2] = pc | page_code;     
    mode_sense_op->cmd.cdb[3] = subpage_code;
    mode_sense_op->cmd.cdb[7] = ((buffer_size & 0xFF00) >> 8);
    mode_sense_op->cmd.cdb[8] = (buffer_size & 0xFF);

    mode_sense_op->cmd.transfer_count = buffer_size;    
    mode_sense_op->cmd.payload_cdb_task_attribute = FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE;
    mode_sense_op->cmd.payload_cdb_flags = FBE_PAYLOAD_CDB_FLAGS_DATA_IN;

    mode_sense_op->status = FBE_STATUS_PENDING;

    /* 2. send pass-thru cmd */
    status = fbe_api_physical_drive_send_pass_thru(object_id, mode_sense_op);
    if (status != FBE_STATUS_OK) 
    {
        fbe_cli_error( "fbe_api_physical_drive_send_pass_thru failed. status: %d\n", status);
        fbe_api_free_memory(mode_sense_op->response_buf);
        fbe_api_free_memory(mode_sense_op);
        return status;
    }  

    /* 3. display results */  
    if (FBE_CLI_PDO_DISPLAY_NORMAL == format)
    {
        i=0;
        fbe_cli_printf("Mode Parameter List\n");
        fbe_cli_printf("   Mode Parameter Header:\n      ");
        while (i < mode_pg_header_len)
        {
            fbe_cli_printf("%02x, ", mode_sense_op->response_buf[i]);
            i++;
        }
        fbe_cli_printf("\n");

        fbe_cli_printf("   Remaining:\n      ");   /*TODO: break this out per page. -wayne*/
        while (i < mode_sense_op->cmd.transfer_count)
        {
            fbe_cli_printf("%02x, ", mode_sense_op->response_buf[i]);
            i++;
    
            if ((i-mode_pg_header_len)%16 == 0)
            {
                fbe_cli_printf("\n      ");
            }
        }
        fbe_cli_printf("\n");
    }
    else // FBE_CLI_PDO_DISPLAY_RAW
    {
        i=0;
        while (i < mode_sense_op->cmd.transfer_count)
        {
            fbe_cli_printf("%02x, ", mode_sense_op->response_buf[i]);
            i++;
    
            if (i%16 == 0)
            {
                fbe_cli_printf("\n");
            }
        }
        fbe_cli_printf("\n");
    }

    fbe_api_free_memory(mode_sense_op->response_buf);
    fbe_api_free_memory(mode_sense_op);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_cli_physical_drive_get_rla_abort_required()
 ****************************************************************
 * @brief
 *  This function gets the current state indicating whether or not Read Look Ahead (RLA)
 *  abort is required\n",
 *
 * @param object_id - the physical drive object's (PDO's) ID.
 * @param context - not used here.
 * @param package_id - package id of that object.
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  07/01/2014 - Created. Darren Insko
 *
 ****************************************************************/
 
static fbe_status_t fbe_cli_physical_drive_get_rla_abort_required(fbe_u32_t object_id, fbe_u32_t context, fbe_package_id_t package_id)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_const_class_info_t *class_info_p = NULL;
    fbe_bool_t b_is_required = FBE_FALSE;
    
    /* Only Support SAS drive for now.        */
    status = fbe_cli_get_class_info(object_id, package_id, &class_info_p);

    if ((status != FBE_STATUS_OK) || 
        (class_info_p->class_id <= FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_FIRST) ||
        (class_info_p->class_id >= FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_LAST))
    {
        /* Just return the above function displayed an error.
         */
        fbe_cli_error("%s This object id 0x%x is not a SAS physical drive but a %s (0x%x).\n",
                      __FUNCTION__, object_id, class_info_p->class_name, class_info_p->class_id);
        return status;
    }

    status = fbe_api_physical_drive_get_rla_abort_required(object_id, &b_is_required);

    if (status != FBE_STATUS_OK) {
        fbe_cli_error("ERROR: Object ID 0x%x status 0x%x. Failed to determine whether or not RLA abort is required\n",
                      object_id, status);
    }
    else
    {
        fbe_cli_printf("Object ID 0x%x: Read Look Ahead (RLA) abort is %s\n", object_id,
                       (b_is_required == FBE_TRUE ? "required" : "not required"));
    }

    return status;
}
/*****************************************************
 * end fbe_cli_physical_drive_get_rla_abort_required()
 *****************************************************/

/*!*******************************************************************
 * @var fbe_cli_cmd_set_pdo_drive_states()
 *********************************************************************
 * @brief Function to set drive states
 * 
 * @return - none.  
 *
 * @author
 *  11/02/2012 - Created. kothal
 *********************************************************************/
void fbe_cli_cmd_set_pdo_drive_states(fbe_s32_t argc, fbe_s8_t ** argv)
{
    fbe_status_t    status = FBE_STATUS_INVALID;  
    fbe_object_id_t pdo_object_id = FBE_OBJECT_ID_INVALID;
    fbe_bool_t b_set_clear_link_fault = FALSE;
    fbe_bool_t b_set_drive_fault = FALSE;
    
    if((1 == argc) &&
            ((strcmp(*argv, "-help") == 0) ||(strcmp(*argv, "-h") == 0)))
    {
        /* If they are asking for help, just display help and exit. */
        fbe_cli_printf("%s", SET_PDO_SRVC_USAGE);
        return;
    }

    while (argc > 0)
    {
        if(strcmp(*argv, "-object_id") == 0)
        {
            /* Get the object id from command line. */
            argc--;
            argv++;           
            
            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("Please provide value for -object_id switch. \n");
                return;
            }
            else{
                
                pdo_object_id = (fbe_u32_t)strtoul(*argv, 0, 0);               
                
                if(pdo_object_id>FBE_MAX_PHYSICAL_OBJECTS)
                {
                    fbe_cli_error("Invalid Physical Drive Object Id. \n\n");           
                    return;
                }
            }
            /* Increment the arguments */
            argc--;
            argv++;           
        }
        else if (strcmp(*argv, "-bes") == 0)
        {
            argc--;
            argv++;
            status = fbe_cli_pdo_parse_bes_opt(*argv, &pdo_object_id);
            if (status != FBE_STATUS_OK)
            {
                /* the parse function prints the error msgs.*/
                return;
            }
            argc--;
            argv++;
        }
        else if (strcmp(*argv, "-list_sanitize_patterns") == 0)
        {
            fbe_cli_printf("------------------------\n");
            fbe_cli_printf(" 0x%02X: Erase Only\n", FBE_SCSI_FORMAT_UNIT_SANITIZE_PATTERN_ERASE_ONLY);
            fbe_cli_printf(" 0x%02X: Overwrite and Erase\n", FBE_SCSI_FORMAT_UNIT_SANITIZE_PATTERN_OVERWRITE_AND_ERASE);
            fbe_cli_printf(" 0x%02X: AFSSI\n", FBE_SCSI_FORMAT_UNIT_SANITIZE_PATTERN_AFSSI);
            fbe_cli_printf(" 0x%02X: NSA\n", FBE_SCSI_FORMAT_UNIT_SANITIZE_PATTERN_NSA);
            fbe_cli_printf("------------------------\n");
            return; 
        }
        else if (strcmp(*argv, "-sanitize") == 0)
        {
            fbe_scsi_sanitize_pattern_t pattern;
            argc--;
            argv++; 

            if (pdo_object_id == FBE_OBJECT_ID_INVALID)
            {
                fbe_cli_error("Invalid object_id. \n%s\n", SET_PDO_SRVC_USAGE);
                return;
            }

            if (argc <= 0)
            {
                fbe_cli_error("Missing Arg. \n%s\n", SET_PDO_SRVC_USAGE);
                return;
            }

            pattern = (fbe_scsi_sanitize_pattern_t)strtoul(*argv, 0, 0);  

            status = fbe_api_physical_drive_sanitize(pdo_object_id, pattern);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_error("Cmd failed.  Verify drive is SSD and using correct pattern. See -list_sanitize_patterns\n");
            }
            return;
        }    
        else if (strcmp(*argv, "-sanitize_status") == 0)
        {
            fbe_physical_drive_sanitize_info_t sanitize_info;
            fbe_u8_t *status_str = NULL;

            if (pdo_object_id == FBE_OBJECT_ID_INVALID)
            {
                fbe_cli_error("Invalid object_id. \n%s\n", SET_PDO_SRVC_USAGE);
                return;
            }

            status = fbe_api_physical_drive_get_sanitize_status(pdo_object_id, &sanitize_info);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_error("Cmd failed.  Verify pattern arg using -list_sanitize_patterns\n");
                return;
            }
            switch(sanitize_info.status)
            {
                case FBE_DRIVE_SANITIZE_STATUS_OK:
                    status_str = "NOT_IN_PROGRESS";
                    break;
                case FBE_DRIVE_SANITIZE_STATUS_IN_PROGRESS:
                    status_str = "IN_PROGRESS";
                    break;
                case FBE_DRIVE_SANITIZE_STATUS_RESTART:
                    status_str = "RESTART_NEEDED";
                    break;
                default:
                    status_str = "Invalid Value";
                    break;
            }
            fbe_cli_printf("status:  %s (%d)\n", status_str, sanitize_info.status);
            if (sanitize_info.status == FBE_DRIVE_SANITIZE_STATUS_IN_PROGRESS)
            {
                fbe_cli_printf("percent: %d (0x%04x)\n", (int)((sanitize_info.percent/(float)FBE_DRIVE_MAX_SANITIZATION_PERCENT)*100), sanitize_info.percent);
            }
            return;
        }
        else if(strcmp(*argv, "-set_media_threshold") == 0)
        {
            fbe_physical_drive_set_dieh_media_threshold_t threshold = {0};

            if (pdo_object_id == FBE_OBJECT_ID_INVALID)
            {
                fbe_cli_error("Invalid object_id. \n%s\n", SET_PDO_SRVC_USAGE);
                return;
            }

            /* get cmd arg*/
            argc--;
            argv++;
            if (argc <= 0)
            {
                fbe_cli_error("Please provide correct arguments. \n");
                return;
            }

            if(strcmp(*argv, "default")==0)
            {
                threshold.cmd = FBE_DRIVE_THRESHOLD_CMD_RESTORE_DEFAULTS;
            }
            else if (strcmp(*argv, "disable")==0)
            {
                threshold.cmd = FBE_DRIVE_THRESHOLD_CMD_DISABLE;
            }
            else if (strcmp(*argv, "increase")==0)
            {
                threshold.cmd = FBE_DRIVE_THRESHOLD_CMD_INCREASE;             
                argc--;
                argv++;
                if (argc <= 0)
                {
                    fbe_cli_error("Please provide correct arguments for 'increase'. \n");
                    return;
                }
                threshold.option = strtoul(*argv,0,0);                
            }
            else
            {
                fbe_cli_error("Invalid cmd argument. \n");
                return;
            }

            status = fbe_api_physical_drive_set_dieh_media_threshold(pdo_object_id, &threshold);

            if (status != FBE_STATUS_OK)
            {
                fbe_cli_error("Cmd failed\n");
                return;
            }
            return;
        }
        else if(strcmp(*argv, "-link_fault") == 0)
        {
            /* Get the object id from command line. */
            argc--;
            argv++;
            
            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("Please provide correct arguments. \n");
                return;
            }
            if(strcmp(*argv, "set")==0)
            {
                status = fbe_cli_set_lifecycle_state(pdo_object_id, FBE_LIFECYCLE_STATE_ACTIVATE, FBE_PACKAGE_ID_PHYSICAL);
                if (status != FBE_STATUS_OK) {
                    fbe_cli_error( "%s: failed to set the lifecycle state of PDO to Ready, status: 0x%x\n", __FUNCTION__, status);
                    return;
                }                
               
                fbe_cli_printf("\nSuccessfully set link fault flag\n\n");
                return;
            }
            else if(strcmp(*argv, "clear")==0)
            {               
                b_set_clear_link_fault = FALSE;
                status = fbe_api_physical_drive_update_link_fault(pdo_object_id, b_set_clear_link_fault);     
                if (status != FBE_STATUS_OK)
                {
                    fbe_cli_error( "%s: failed to clear link fault flag, status: 0x%x\n", __FUNCTION__, status);
                    return;
                }
                
                status = fbe_cli_set_lifecycle_state(pdo_object_id, FBE_LIFECYCLE_STATE_FAIL, FBE_PACKAGE_ID_PHYSICAL);
                if (status != FBE_STATUS_OK) {
                    fbe_cli_error( "%s: failed to set the lifecycle state of PDO to Activate/Fail,  status: 0x%x\n", __FUNCTION__, status);   
                    return;
                }
                
                fbe_cli_printf("\nSuccessfully cleared link fault flag\n\n");
                return;
            }
            else
            {
                fbe_cli_error("\nPlease provide correct arguments, either set or clear. \n");
                fbe_cli_printf("%s", SET_PDO_SRVC_USAGE);
                return;
            }   
        }
        
        else if(strcmp(*argv, "-drive_fault") == 0)
        {
            /* Get the object id from command line. */
            argc--;
            argv++;
            
            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("Please provide correct arguments. \n");
                return;
            }
            if(strcmp(*argv, "set")==0) {
                b_set_drive_fault = TRUE;
                 status = fbe_api_physical_drive_update_drive_fault(pdo_object_id, b_set_drive_fault);
                if (status != FBE_STATUS_OK) {
                    fbe_cli_error( "%s: failed to set drive fault flag, status: 0x%x\n", __FUNCTION__, status);
                    return;
                }
                fbe_cli_printf("\nSuccessfully set drive fault flag\n\n");
                return;
            }            
            else {
                fbe_cli_error("\nPlease provide correct arguments, either set or get. \n");
                fbe_cli_printf("%s", SET_PDO_SRVC_USAGE);
                return;
            }   
        }
        else if(strcmp(*argv, "-format_block_size") == 0)   /* USE WITH CARE */
        {
            fbe_u32_t block_size;
            if (pdo_object_id == FBE_OBJECT_ID_INVALID)
            {
                fbe_cli_error("Invalid object_id. \n%s\n", SET_PDO_SRVC_USAGE);
                return;
            }

            /* Get the object id from command line. */
            argc--;
            argv++;

            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("Please provide correct arguments. \n");
                return;
            }

            block_size = strtoul(*argv, 0, 0); 
            fbe_api_physical_drive_format_block_size(pdo_object_id, block_size);
            return;
        }
        else
        {
            fbe_cli_error("\nNeed to provide valid command option. \n");
            fbe_cli_printf("%s", SET_PDO_SRVC_USAGE);
            return;
        }
    }
    
} /* end of fbe_cli_cmd_set_pdo_drive_states() */


static fbe_status_t fbe_cli_physical_drive_set_io_throttle_MB(fbe_u32_t object_id, fbe_u32_t io_throttle_MB, fbe_package_id_t package_id)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* We need to set I/O throlle for all drives */

    status = fbe_api_physical_drive_set_object_io_throttle_MB(object_id, io_throttle_MB);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nERROR: failed status 0x%x to set io_throttle for object_id: 0x%x\n",
                      status, object_id);
        return status;
    }

    return status;
}

void fbe_cli_cmd_set_ica_stamp(int argc , char ** argv)
{
    fbe_u32_t slot = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_object_id_t object_id = FBE_OBJECT_ID_INVALID;

    for(slot = 0; slot < 3; slot++){
        /* Get the object handle for this target. */
        status = fbe_api_get_physical_drive_object_id_by_location(0, 0, slot, &object_id);
        if ((status != FBE_STATUS_OK) || (object_id == FBE_OBJECT_ID_INVALID)) {
            fbe_cli_printf("Drive slot: %d not found\n\n", slot);
            continue;
        }
        fbe_cli_printf("Drive slot: %d found object_id 0x%X\n\n", slot, object_id);
        /* Set ICA stamp */
        status = fbe_api_physical_drive_interface_generate_ica_stamp(object_id);
        if (status != FBE_STATUS_OK){
            fbe_cli_printf("Drive slot: %d ICA stamp FAILED\n\n", slot);
        } else {
            fbe_cli_printf("Drive slot: %d ICA stamp created\n\n", slot);
        }
    }
 
    return;
}      


static fbe_status_t fbe_cli_physical_drive_set_enhanced_queuing_timer(fbe_u32_t object_id, fbe_u32_t lpq_timer, fbe_u32_t hpq_timer)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* We need to set enhanced queuing timer */
    fbe_cli_printf("set_queuing_timer lpq_timer %d hpq_timer %d \n", lpq_timer, hpq_timer);
    status = fbe_api_physical_drive_set_enhanced_queuing_timer(object_id, lpq_timer, hpq_timer);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nERROR: failed status 0x%x to set enhanced queuing timer for object_id: 0x%x\n",
                      status, object_id);
        return status;
    }

    return status;
}

/*!**************************************************************
 *  fbe_cli_pdo_parse_bes_opt()
 ****************************************************************
 *
 * @brief   Parses the b_e_d format option and returns its PDO
 *          object ID.
 * 
 * @param bes_str - string with format b_e_s.
 * @param pdo_object_id - object id
 * 
 * @return returns OK if PDO object exists.
 * 
 * @author
 *  7/02/2014 - Created. Wayne Garrett
 *
 ****************************************************************/
fbe_status_t fbe_cli_pdo_parse_bes_opt(char *bes_str, fbe_object_id_t * pdo_object_id_p)
{
    fbe_status_t status;
    fbe_job_service_bes_t location;

    status = fbe_cli_convert_diskname_to_bed(bes_str, &location);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nInvalid b_e_d format\n");        
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = fbe_api_get_physical_drive_object_id_by_location(location.bus, location.enclosure, location.slot, pdo_object_id_p);
    if (status != FBE_STATUS_OK || (*pdo_object_id_p) == FBE_OBJECT_ID_INVALID)
    {
        fbe_cli_error("\nFailed to get object ID for %d_%d_%d. status:%d\n", 
                      location.bus, location.enclosure, location.slot, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*************************
 * end file fbe_cli_src_physical_drive_cmds.c
 *************************/
