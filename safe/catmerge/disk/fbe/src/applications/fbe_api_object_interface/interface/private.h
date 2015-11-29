/*********************************************************************
 * Copyright (C) EMC Corporation, 1989 - 2011
 * All Rights Reserved.
 * Licensed Material-Property of EMC Corporation.
 * This software is made available solely pursuant to the terms
 * of a EMC license agreement which governs its use.
 *********************************************************************/

/*********************************************************************
 *
 * Description:
 *      This is a global header file for the FBEAPIX class library
 *      that contains FBE and FBE API includes. It also contains
 *      some FBE variable declarations.
 *
 *  History:
 *      12-May-2010 : initial version
 *
 *********************************************************************/

#ifndef PRIVATE_H
#define PRIVATE_H

/*********************************************************************
 * Includes : system files 
 *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <errno.h>
#include <malloc.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <signal.h>

/*********************************************************************
 * fbe includes // catmerge\disk\interface\fbe directory (fbe_api)
 *********************************************************************/
extern "C" {
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_sim_transport.h"
#include "fbe/fbe_api_sim_transport_packet_interface.h"

#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_panic_sp_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_power_saving_interface.h"

#include "fbe/fbe_api_event_log_interface.h"

#include "fbe_database.h"
#include "fbe/fbe_api_block_transport_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_cmi_interface.h"
#include "fbe/fbe_api_bvd_interface.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "fbe/fbe_api_esp_drive_mgmt_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_esp_sps_mgmt_interface.h"
#include "fbe/fbe_api_esp_eir_interface.h"
#include "fbe/fbe_api_metadata_interface.h"
#include "fbe/fbe_api_esp_ps_mgmt_interface.h"
#include "fbe/fbe_api_esp_board_mgmt_interface.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_esp_cooling_mgmt_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe/fbe_api_esp_module_mgmt_interface.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "fbe/fbe_api_system_interface.h"
#include "fbe/fbe_api_logical_error_injection_interface.h"
#include "fbe/fbe_api_system_bg_service_interface.h"
#include "fbe/fbe_api_system_time_threshold_interface.h"
#include "fbe/fbe_file.h"

/*********************************************************************
 * fbe includes // catmerge\disk\interface\fbe directory (fbe)
 *********************************************************************/

#include "fbe/fbe_class.h"
#include "fbe/fbe_cli.h"
#include "fbe/fbe_enclosure.h"
#include "fbe/fbe_emcutil_shell_include.h"
#include "fbe/fbe_library_interface.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_package.h"
#include "fbe/fbe_physical_drive.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe/fbe_trace_interface.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_limits.h"

/*********************************************************************
 * fbe includes // catmerge\disk\fbe\interface directory (fbe)
 *********************************************************************/

#include "fbe_interface.h"
#include "fbe_service_manager.h"
#include "fbe_trace.h"
/*********************************************************************
 * fbe includes // catmerge\disk\fbe\src\lib\enclosure_data_access
 *********************************************************************/

#include "../fbe/src/lib/enclosure_data_access/edal_processor_enclosure_data.h"

/*********************************************************************
 * includes // catmerge\interface
 *********************************************************************/

#include "../../interface/fbe_private_space_layout.h"
}

/*********************************************************************
 * fbe declarations needed if calling fbecli library - review and
 * remove duplicates
 *********************************************************************/

static HANDLE CSX_MAYBE_UNUSED outFileHandle = NULL;
static fbe_bool_t CSX_MAYBE_UNUSED simulation_mode = FBE_FALSE;
static char CSX_MAYBE_UNUSED filename[25] = "fbeapiLib";
static unsigned int CSX_MAYBE_UNUSED outputMode = FBE_CLI_CONSOLE_OUTPUT_FILE;
typedef fbe_u32_t fbe_object_id_t;

#endif /* PRIVATE_H */
