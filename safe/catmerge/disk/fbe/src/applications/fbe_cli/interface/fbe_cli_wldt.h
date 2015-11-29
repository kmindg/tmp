#ifndef __FBE_CLI_WLDT_H__
#define __FBE_CLI_WLDT_H__
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_wldt.h
 ***************************************************************************
 *
 * @brief
 *  This file contains LUN/RG command cli definitions.
 *
 * @version
 *  07/01/2010 - Created. Swati Fursule
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

#include "fbe/fbe_cli.h"
#include "fbe/fbe_lun.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_job_service_interface.h"


void fbe_cli_wldt(int argc , char ** argv);

#define WLDT_CMD_USAGE "\
 wldt <flags>\n\
 Lun number, object_id, slot_id and disk_pos are hexadecimal.\n\
Flags:\n\
    -object_id/-o [object_id_value] - Get the write log info from the raid group with this object_id. \n\
    -lun_num/-l [lun_id_value] - Get the write log info from the raid group containing this lun number. \n\
    -raid_group/-rg [raid_group_number] - Get the write log info from the raid group with this rg number. \n\
    -slot/-s [slot_id] - Access only this slot id (default is all valid slots).\n\
    -all_slots/-as - All slots (default is valid slots only).\n\
    -all_headers/-ah - All headers (default is valid headers only).\n\
    -raw_headers/-rh - Print raw header blocks (in case they are garbled and can't be interpreted as headers).\n\
    -raw_disk/-rd [disk_pos block_num count] - Print raw header and count data blocks from selected disk and block number.\n\
                   block_num -> (block_num + count) must fit in 0 -> (slot_size_in_blocks - 1) range\n\
Examples:  \n\
    Read and display valid write log slots and valid contained headers from raid group object id 0x13c:\n\
      wldt -object_id 0x13c\n\
    Read and display all write log slots and valid headers from raid group object id 0x13c:\n\
      wldt -o 0x13c -as\n\
    Read and display valid write log slots and all contained headers raid group containing lun number 4:\n\
      wldt -lun_id 4 -ah\n\
    Read and display valid write log slots and valid contained headers raid group containing lun number 4:\n\
      wldt -l 4 \n\
    Read and display valid write log slots and raw contained headers for raid group object_id 0x13c:\n\
      wldt -o 0x13c -rh\n\
    Read and display valid write log slot 0x11 and raw header and data for disk 2 for raid group object_id 0x13c:\n\
      wldt -o 0x13c -s 0x11 -rd 0x2\n"


/*************************
 * end file fbe_cli_wldt.h
 *************************/

#endif /* end __FBE_CLI_WLDT_H__ */
