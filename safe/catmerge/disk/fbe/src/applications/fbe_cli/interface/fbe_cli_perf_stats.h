#ifndef __fbe_cli_cmd_perf_stats_H__
#define __fbe_cli_cmd_perf_stats_H__
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_cmd_perf_stats.h
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
#include "fbe/fbe_bvd_interface.h"


void fbe_cli_cmd_perfstats(int argc , char ** argv);
typedef enum fbe_cli_perfstats_tags_s{
    FBE_CLI_PERFSTATS_TAG_LUN_RBLK                      = 0x00000001,
    FBE_CLI_PERFSTATS_TAG_LUN_WBLK                      = 0x00000002,
    FBE_CLI_PERFSTATS_TAG_LUN_RIOPS                     = 0x00000004,
    FBE_CLI_PERFSTATS_TAG_LUN_WIOPS                     = 0x00000008,
    FBE_CLI_PERFSTATS_TAG_LUN_STPC                      = 0x00000010,
    FBE_CLI_PERFSTATS_TAG_LUN_MR3W                      = 0x00000020,
    FBE_CLI_PERFSTATS_TAG_LUN_RHIS                      = 0x00000040,
    FBE_CLI_PERFSTATS_TAG_LUN_WHIS                      = 0x00000080,
    FBE_CLI_PERFSTATS_TAG_RG_RBLK                       = 0x00000100,
    FBE_CLI_PERFSTATS_TAG_RG_WBLK                       = 0x00000200,
    FBE_CLI_PERFSTATS_TAG_RG_RIOPS                      = 0x00000400,
    FBE_CLI_PERFSTATS_TAG_RG_WIOPS                      = 0x00000800,
    FBE_CLI_PERFSTATS_TAG_PDO_RBLK                      = 0x00001000,
    FBE_CLI_PERFSTATS_TAG_PDO_WBLK                      = 0x00002000,
    FBE_CLI_PERFSTATS_TAG_PDO_SBLK                      = 0x00004000,
    FBE_CLI_PERFSTATS_TAG_PDO_UTIL                      = 0x00008000,
    FBE_CLI_PERFSTATS_TAG_PDO_RIOPS                     = 0x00010000,
    FBE_CLI_PERFSTATS_TAG_PDO_WIOPS                     = 0x00020000,
    FBE_CLI_PERFSTATS_TAG_LUN_NZQA                      = 0x00040000,
    FBE_CLI_PERFSTATS_TAG_LUN_SAQL                      = 0x00080000,
    FBE_CLI_PERFSTATS_TAG_LUN_CRRT                      = 0x00100000,
    FBE_CLI_PERFSTATS_TAG_LUN_CWRT                      = 0x00200000,
    FBE_CLI_PERFSTATS_TAG_PDO_THIS                      = 0x00400000,
    FBE_CLI_PERFSTATS_TAG_ALL                           = 0xFFFFFFFF
}fbe_cli_perfstats_tags_t;

#define FBE_CLI_PERFSTATS_DEFAULT_DURATION_MSEC 60000
#define FBE_CLI_PERFSTATS_DEFAULT_DIRECTORY "perflogs"
#define FBE_CLI_PERFSTATS_DEFAULT_SAMPLE_COUNT 16384

#define PERFSTATS_CMD_USAGE "perfstats -[enable|disable|clear|log|dump] -package [sep|pp]\n\
Filter option (valid for -enable, -disable, -log, and -dump): \n\
\t-object_id <hex_obj_id> \n\
\t-object_id: hex object ID to a LUN or PDO object. \n\
\t-b <bus #> -e <enclosure #> -s <slot #>: Specifies a drive via <B_E_S> format only for PDO statistics.\n\
\tIf neither an object ID or a drive B_E_S are specified, we'll use every object in the specified package.\n\
Log options:\n\
\t-summed: Sum all per-core stats, default behavior is to log core stats into separate fields\n\
\t-dir: directory to store stats in, default is C:\\perflog\\ \n\
\t-interval: seconds between log writes, default is 60 seconds. \n\
\t\t (Warning: frequent polls can have an adverse effect on system performance.)\n\
\t-tag <tag1 tag2 tagN>: which statistics to collect. If this isn't specified, all are logged.\n\
Tags:\n\
    lun_rblk: Read blocks/s for LUN \n\
    lun_wblk: Write blocks/s Written for LUN \n\
    lun_riops: Read IOPS  for LUN\n\
    lun_wiops: Write IOPS for LUN\n\
    lun_stpc: Stripe Crossings for LUN \n\
    lun_mr3w: MR3 (full stripe) writes for LUN \n\
    lun_rhis: Read Size Historgram for LUN (recommend summing for readability) \n\
    lun_whis: Write Size Historgram for LUN (recommend summing for readability) \n\
    lun_crrt: Cumulative Read Response Time for LUN \n\
    lun_cwrt: Cumulative Write Response Time for LUN \n\
    lun_nzqa: Non-zero Queue Arrivals for LUN \n\
    lun_saql: Sum Arrival Queue Length for LUN \n\
    rg_rblk: Read blocks/s  for RG (will display per-disk)\n\
    rg_wblk: Write blocks/s Written for RG(will display per-disk)\n\
    rg_riops: Read IOPS for RG (will display per-disk)\n\
    rg_wiops: Write IOPS for RG (will display per-disk)\n\
    pdo_rblk: Blocks read from Physical Drive\n\
    pdo_wblk: Blocks written to Physical Drive\n\
    pdo_sblk: Blocks seeked for Physical Drive\n\
    pdo_riops: Read IOPS for Physical Drive\n\
    pdo_wiops: Write IOPS for Physical Drive\n\
    pdo_util: Utilization of Physical Drive\n\
    pdo_this: Service Time Histogram for Physical Drive\n\
    \n"

/*************************
 * end file fbe_cli_cmd_perf_stats.h
 *************************/

#endif /* end __fbe_cli_cmd_perf_stats_H__ */
