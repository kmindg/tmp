#ifndef __FBE_CLI_LUNINFO_H__
#define __FBE_CLI_LUNINFO_H__
/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_luninfo.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definition for Luninfo command of FBE Cli.
 *
 * @date
 * 5/5/2011 - Created. Harshal Wanjari 
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_cli.h"
#include "fbe/fbe_lun.h"
#include "fbe/fbe_api_common.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
void fbe_cli_cmd_luninfo(int argc, char** argv);
void fbe_cli_luninfo_print_lun_in_rg(fbe_object_id_t rg_id);
void fbe_cli_luninfo_print_all_luns(void);
void fbe_cli_luninfo_print_lun_details(fbe_object_id_t lun_id);
void fbe_cli_lun_get_cache_zero_bitmap_block_count(fbe_s32_t argc, fbe_s8_t ** argv);
void fbe_cli_lib_lun_display_raid_info(fbe_object_id_t lun_object_id);
void fbe_cli_luninfo_unexport_all_luns(void);
#define LUNINFO_CMD_USAGE "\
luninfo/ li - Get information about lun\n\
\n\
Usage:\n\
luninfo -lun <lun>\n\
        -rg <rg>\n\
        -all\n\
        -map_lba (lba) - Map this lba to a pba.  -lun argument is required after this option.\n\
        -clear_fault - Allow the LUN to come back ready after being taken offline due to too many unexpected errors.\n\
        -disable_fail_on_error - Prevent the LUN from failing due to too many unexpected errors.\n\
        -enable_fail_on_error - Enable handling so the LUN will fail due to too many unexpected errors.\n\
        -raid_info - Display information we return on a get raid info ioctl.\n\
        -enable_loopback - Enable I/O loopback.\n\
        -disable_loopback - Disable I/O loopback.\n\
        -get_loopback - Get I/O loopback information.\n\
        -unexport - Unexport lun from blockshim.\n\
\n\
E.g\n\
  luninfo -lun 0\n\
  luninfo -map_lba 2d9c -lun 0\n\
  luninfo -clear_fault -lun 0\n\
  luninfo -disable_fail_on_error -lun 0\n\
  luninfo -enable_fail_on_error -lun 0\n\
  luninfo -raid_info -lun 0\n\
  luninfo -enable_loopback -lun 0\n\
  luninfo -disable_loopback -lun 0\n\
  luninfo -get_loopback -lun 0\n\
  luninfo -unexport -lun 0\n\
\n"

#define CACHE_ZERO_BITMAP_COUNT_USAGE "\
cache_zero_bitmap_count [switches]    \n\
General Switches:\n\
  -cap         Lun capacity in blocks \n\
\n\
E.g\n\
cache_zero_bitmap_count -cap 5000 \n\
\n"

/*************************
 * end file fbe_cli_luninfo.h
 *************************/

#endif /* end __FBE_CLI_LUNINFO_H__ */
