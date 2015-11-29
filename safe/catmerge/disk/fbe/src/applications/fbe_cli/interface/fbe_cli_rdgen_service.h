#ifndef __FBE_CLI_RDGEN_SERVICE_H__
#define __FBE_CLI_RDGEN_SERVICE_H__
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
 
/*!*************************************************************************
 * @file fbe_cli_rdgen_service.h
 ***************************************************************************
 *
 * @brief
 *  This file contains rdgen service cli definitions.
 *
 * @version
 *   3/27/2010:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

#include "fbe/fbe_cli.h"

void fbe_cli_cmd_rdgen_service(int argc , char ** argv);

#define RDGEN_SERVICE_USAGE "\
rdgen [switches] <operation> <LUN # | *> <threads> [max I/O block size in hex]\n"\
"General Switches:\n\
     -d[isplay] [tag | *]    - Display active threads for a tag, which by default is a unit\n\
     -dp                     - Display performance stats for active threads\n\
     -s                      - Display summary of performance stats for active threads\n\
     -stop      [tag | *]    - stop I/O generation on a unit or group. Does not wait for these to stop.\n\
     -stop_sync [tag | *]    - stop I/O generation syncronously, waiting for all requests to stop.\n\
     -t or -trace_level [level]     - Set the trace level of the rdgen service \n\
     -init_dps (size in mb) 		- Initialize the DPS memory in RDGEN. Optionally provide memory size. \n\
                                      If memory size not provided, a default size is selected.\n\
                                      With this option rdgen tries to get memory from SP Cache.\n\
     -init_dps_cmm (size in mb)		- Initialize the DPS memory in RDGEN by using CMM.\n\
     -reset_stats                   - Reset the rdgen statistics.  Also resets all per thread statistics.\n\
     -default_timeout_msec (milliseconds) - Set the default time to timeout and abort I/Os.\n\
     -h                      - display this help text\n\
     -object_id [id]  - Object id to run I/O to.  LUN # following operation is required, but ignored.\n\
     -rg_object_id [id]  - RAID Group object id to which we will start threads on all of its objects.\n\
                           When this argument is provided, the LUN # following the operation is required, but ignored.\n\
     -package_id [physical | sep] - Package to run to for above object id. \n\
     -parity, -mirror, -striper, -vd, -pvd, -ldo, -sas_pdo - Run to all objects of this class\n\
     -enable_sys_threads [0,1] - enable/disable rdgen running in system threads.\n\
Operations:\n\
     syntax - [switches] <operation> <tag | *> <threads> [max I/O block size]\n\
     r  - Read only\n\
     h  - Read/Check\n\
     w  - Write only\n\
     c  - Write/Read/Check\n\
     z  - Zero Fill\n\
     y  - Zero Fill/Read/Check\n\
     u  - Unmap\n\
     v  - Verify\n"

#define RDGEN_SERVICE_USAGE_SWITCHES "\
Switches for starting I/O generation:\n\
     -num_ios [ios]          - Number of I/Os to run before stopping\n\
     -num_msec [msec]        - Number of milliseconds to run before stopping\n\
     -repeat_lba             - Only run I/O to this lba.\n\
     -inc_lba [blocks]       - For sequential increasing, increment by this amount instead of by the size of the I/O.\n\
                               For example, with rdgen -constant -seq -inc_lba 10 r 0 1 100, we start a \n\
                                sequential stream of 100 block I/Os incrementing by 10 instead of 100, so\n\
                                the I/O size is 100, but the lbas are 0, 10, 20,etc.\n\
                                with -inc_lba 110, the lbas would be 0, 110, 220, 330, etc.\n\
     -inc_blocks [blocks]    - Instead of a constant or random block count, the block count will increase\n\
                               from min_blocks to max_blocks with this increment. \n\
                               For example if min blocks is 20 and max blocks is 100, and inc_blocks is 20,\n\
                               start with 20, and then be 40, 60, 80, and 100, and then go back to 20, 40, etc.\n\
     -min_blocks [blocks]    - This is the smallest block count we will allow.\n\
                               This is ignored if -constant is used \n\
                               If -min_blocks is 50 and max blocks (last argument to rdgen) is 100, \n\
                               then rdgen will choose random block counts between 50 and 100 inclusive.\n\
                               If -min_blocks is 20 and max blocks is 100, and -inc_blocks is 20,\n\
                               rden will use the block counts 20, 40, 60, 80, 100 \n\
                               For example if min blocks is 20 and max blocks is 100, and inc_blocks is 20,\n\
                               start with 20, and then be 40, 60, 80, and 100, and then go back to 20, 40, etc.\n\
     -min_lba [lba]          - Specifies the start of the test range. By default this is 0. \n\
     -max_lba [lba]          - Specifies the end of the test range. By default this is the capacity of the device.\n\
     -random_start_lba       - By default the start lba is lba 0.  This switch indicates to use a random start lba.\n\
     -start_lba [lba]        - Specifies the lba for the first I/O.  By default this is 0. \n\
     -constant               - Only use this number of blocks for each request (constant I/O block size).\n\
     -align                  - Align all lbas and block counts to the input block count.\n\
     -seq or -inc            - sequential lbas (increasing)\n\
     -dec                    - decreasing sequential lbas\n\
     -cat_inc                - sequential increasing caterpillar. When multiple threads are used,\n\
                               they will coordinate to cover the test area in parallel.\n\
     -cat_dec                - decreasing sequential caterpillar. When multiple threads are used,\n\
                               they will coordinate to cover the test area in parallel.\n\
     -priority [number]      - Priority of this operation.  1=LOW, 2=NORMAL, 3=URGENT.  \n\
     -abort                  - abort I/Os randomly during I/O generation.\n\
                               The time to abort will be chosen randomly between 0 and 1000 milliseconds.\n\
                               Automatically continues on abort errors.\n\
     -abort_ms (msec)        - Abort I/O within this number of milliseconds\n\
     -random_abort (msec)    - Randomly abort I/O within a time between 0 and the specified milliseconds.\n\
     -continue_on_error      - Automatically continues even if we get aborted or media errors.\n\
     -continue_on_all_errors - Automatically continues even if we get any error.\n\
     -panic_on_all_errors    - Automatically panic if we get any error.\n\
     -sequence_number        - Use a fixed sequence number.  Useful to use this with -sp and -cp.\n\
     -fixed_random_seed      - Use a fixed random seed.  This allows generation of random but\n\
                               repeatable lba/block count I/O loads.\n\
     -delay_msec (msec)      - Insert a fixed delay on each I/O of msec.\n\
     -random_delay (msec)    - Insert a random delay on each I/O up to msec.\n\
     -sp                     - Only available for writes.  Does a sequential write\n\
                               through all blocks on the LUN, writing all the blocks\n\
                               with the rdgen pattern. This only does one pass and exits.\n\
     -cp                     - Only available for reads.  Does a sequential read\n\
                               through all blocks on the LUN, comparing all the blocks\n\
                               with the rdgen pattern. This only does one pass and exits.\n\
     -perf or -performance   - Run in a performance mode where we limit our overhead.\n\
     -non_cached             - For write operations do the operation non-cached.\n\
     -affine [core]          - Core to affine these rdgen threads to.\n\
     -allow_fail_congestion or -afc - I/O is allowed to fail due to congestion.\n\
     -affinity_spread or -as - For each object we will spread the affinity of multiple threads out\n\
                               across all cores.\n\
     -no_panic               - Do not panic if we detect a mismatch in the pattern.\n\
     -clear                  - Use a pattern of a zeroed block (including zeros in metadata).\n\
     -zeros                  - Use a pattern of a zeroed block with a good checksum.\n\
     -device (name)          - Run IRP I/O to the specified device.  (Omit the \\Device\\ ) \n\
     -clar_lun (number)      - Run IRP I/O to the CLARiiONdiskN. Where N is the device number.   \n\
     -sep_lun (number)       - Run IRP I/O to the FlareDiskN. Where N is the device number.   \n\
     -irp_dca                - Select the IRP DCA interface(See e.g below need device number).\n\
     -irp_sgl                - Select the IRP sgl interface.\n\
     -irp_mj                 - Select the IRP MJ interface.\n\
     -peer_options (type)    - Select the type of peer options.\n\
                               possible types are: peer_random, load_balance, and peer_only\n\
     -pass_count [number]    - Number of pass to run.  If pass_count is set to  0, rdgen will runs until \n\
                               user stops it. \n"

#define RDGEN_SERVICE_USAGE_EXAMPLES "\
Examples:\n\
  1 thread 1 block random lba read I/O to all LUNs: \n\
             rdgen r 0 1 1\n\
  performance 8kb write test to one object with affined to one core, random/16 block aligned lba, constant size, 16 blocks: \n\
             rdgen -object_id 0x130 -package_id sep -affine 1 -constant -perf -align w 0 1 10\n\
  performance 1kb read test to one lun: \n\
             rdgen -affine 1 -constant -perf -align r 0 1 2\n\
  Limit the test area between block 0x1000 and 0x50000: \n\
             rdgen -min_lba 100 -max_lba 50000 r 0 1 2\n\
  Do constant sized 0x100 block I/O with an increment of 10, this gives an lba sequence of 0, 0x10, 0x20, 0x30 etc, \n\
              but the block count is 0x100: \n\
             rdgen -seq -inc_lba 10 -constant r 0 1 100\n\
  Insert a 5 second delay on each I/O. Single threaded read test of 1 block.\n\
             rdgen -random_delay 5000 r 0 1 1\n\
  Insert a random delay of up to 500 milliseconds on each I/O. Single threaded read test of 1 block.\n\
             rdgen -delay_msec 500 r 0 1 1\n\
  performance 1kb read test to device CLARiiONdisk0 using the IRP DCA interface:\n\
             rdgen -clar_lun 0 -affine 1 -constant -perf -align r 0 1 2\n\
  performance 1 kb read to the LUN exported by sep using the IRP SGL interface:\n\
             rdgen -sep_lun 0 -irp_sgl -affine 1 -constant -perf -align r 0 1 2\n\
  performance 1 kb read to the LUN exported by sep using the IRP DCA interface:\n\
             rdgen -sep_lun 0 -irp_dca -affine 1 -constant -perf -align r 0 1 2\n\
  performance 1 kb read to the LUN exported by sep using the IRP MJ interface:\n\
             rdgen -sep_lun 0 -irp_mj -affine 1 -constant -perf -align r 0 1 2\n\
  performance 1 kb read to the device CLARiiONdisk10 using the IRP DCA interface:\n\
             rdgen -device CLARiiONdisk10 -affine 1 -constant -perf -align r 0 1 2\n"
/*************************
 * end file fbe_cli_rdgen_service.h
 *************************/

#endif /* end __FBE_CLI_RDGEN_SERVICE_H__ */
