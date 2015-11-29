#ifndef SUNBURST_H
#define SUNBURST_H 0x00000001   /* from common dir */
#define FLARE__STAR__H

/***************************************************************************
 * Copyright (C) EMC Corporation 1989-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *      sunburst.h
 ***************************************************************************
 *
 * DESCRIPTION:
 *   This header file contains the structures which are used to implement the
 *   Sunburst Protocol.
 *
 * NOTES:
 *   Don't forget to update ulogEventMsg()
 *
 * HISTORY:
 *    Created by Sam Pendleton 23-Jun-89, by merging "hi_data.h", and
 *       "host_error.h"
 *    17-Oct-91, Modifcations for Sauna - JAB
 *    20-AUG-93, Added 3 new opcodes for CM_UNIT_NUMBER. Bob D
 *    21-SEP-93, added a couple of new error codes     Bob D
 *    19-OCT-93  added a couple of new error codes     Bob D
 *    27-APR-94  added new codes for BBU sniffing      ABurns
 *    04-May-94  added HI_CHANGE_ADDRESSING_MODEL HI_SEND_DIAG,
 *               HI_SEND_DIAG_DUAL RER
 *    09-Jun-94  added HI_CHANGE_HOST_INTERFACE        RER
 *    08-Aug-94  added HOST_PEER_CONTROLLER_INITING    DGM
 *    03-Nov-94  Changed cache recovery errors to 900 class errors   RER
 *    04-Dec-94  Added stuff for HI_SAVE_PAGES. DGM
 *    22-Dec-94  Corrected CM_UNIT_NUMBER.  DGM
 *    22-Feb-95  Added HOST_SP_POWERUP, which is to be used to log the rev
 *               number at some point in the future.  DGM
 *    23-Feb-95  Removed freshly_bound_assign bit
 *               from the REQUEST_BLOCK flag field
 *    06-Apr-95  Removed support for obsolete corona reports. AB
 *    28-Jun-95  Added HOST_FAN_PACK_DISABLED and HOST_FAN_PACK_ENABLED.  DGM
 *    19-Nov-95  Added 0x6006 and 0x6007.  DGM
 *    04-Sep-96  Added HI_FLASH_REQUEST CNL
 *    01-Oct-96  Added Error codes for SBERR MPG
 *    22-Oct-97  Added SERIAL_DRIVE_DOWNLOAD CNL
 *    28-Aug-98  Added new codes:  MPA
 *                   HOST_CONTROL_BYTE_NOT_ZERO
 *                   HOST_PARAM_LENGTH_TOO_LONG
 *                   HOST_UNIT_TYPE_DOES_NOT_SUPPORT_CACHING
 *                   HOST_CURRENTLY_THIS_SYSTEM_TYPE
 *                   HOST_ILLEGAL_UNIT_TYPE
 *                   HOST_SYS_CONFIG_CHANGE_IN_PROGRESS
 *                   HOST_INSUFFICIENT_CONTROL_STRUCTURE_MEMORY
 *                   HOST_ILLEGAL_CRU_NUMBER
 *                   HOST_INVALID_ADRESSING_MODEL
 *                   HOST_INVALID_SYSTEM_TYPE
 *                   HOST_UNITS_STILL_BINDING
 *                   HOST_HOT_SPARE_IN_USE
 *                   HOST_BAD_PROM_IMAGE
 *                   HOST_DOWNLOAD_TIMEOUT
 *                   HOST_DISKS_NOT_IDLE
 *                   HOST_UCODE_UPDATE_IN_PROGRESS
 *                   HOST_TOO_MANY_WRITE_CACHE_PAGES
 *                   HOST_SUP_BUF_DB_LOCK_ERROR
 *                   HOST_SUP_BUF_SS_ERROR
 *                   HOST_SUP_BUF_2SSOF_ERROR
 *                   HOST_SUP_BUF_2SS_ERROR
 *                   HOST_TSS_DUAL_BOARD_SOFT_SHUTDOWN_FAILED
 *                   HOST_TSSOF_DUAL_BOARD_SOFT_SHUTDOWN_FAILED
 *                   HOST_INVALID_WRITE_CACHE_SIZE
 *                   HOST_INVALID_R3_MEM_SIZE
 *                   HOST_CANT_CHANGE_WCACHE_AND_R3_MEM_TOGETHER
 *                   HOST_R3_WRITE_BUFFERING_ENABLED
 *                   HOST_WRITE_CACHE_ALLOCATED
 *                   HOST_BANDWIDTH_MODE_R3_OPTIMIZED
 *                   HOST_OUT_OF_CACHE_MEMORY
 *                   HOST_INVALID_CONFIG_WORD
 *                   HOST_CANT_DECONFIGURE_PRESENT_CONTROLLER
 *                   HOST_FRU_FORMATTED_OR_NOT_LEGAL
 *                   HOST_LUNS_CACHE_DIRTY
 *                   HOST_PEER_LUNS_CACHE_DIRTY
 *                   HOST_CACHE_NOT_DISABLED
 *                   HOST_CACHE_NOT_CLEAN
 *                   HOST_PEER_CACHE_NOT_CLEAN
 *                   HOST_UNASSIGNED_CACHE_PAGES
 *                   HOST_READ_CACHE_NOT_DISABLED
 *                   HOST_COMMAND_UNSUPPORTED
 *                   HOST_COMMAND_UNSUPPORTED_FOR_THIS_SYSTEM_TYPE
 *                   HOST_ILLEGAL_VERIFY_TIME
 *                   HOST_BIND_NM1_NOT_AVAILABLE
 *                   HOST_BIND_SELECTED_FRU_CANT_BE_HS
 *                   HOST_BIND_BAD_STRIPES_NUMBER
 *                   HOST_BIND_ILLEGAL_CACHING_REQUEST
 *                   HOST_BIND_ILLEGAL_HATS_PER_UNIT_REQUEST
 *                   HOST_BIND_ILLEGAL_CACHE_REQUEST_FOR_R3_UNIT
 *                   HOST_NOT_ENOUGH_DB_DRIVES
 *    14-Jan-00  Added new codes:  SWF
 *                   HOST_USING_INVALID_WWN
 *    25-Jul-01  Moved fru_host_state_T to dba_export_api.h
 *
 * $Log: sunburst.h,v $
 * Revision 1.1.2.16  2000/06/08 16:00:56  fladm
 * User: dspeterson
 * Reason: sus_5756
 * Add Clear Cache Dirty functionality to fcli and mode-select
 *
 * Revision 1.66.12.2  2000/10/13 14:23:38  fladm
 * User: rfoley
 * Reason: MERGE
 * Initial update to merge cat1 and the raidNT changes into
 * the k10prep branch.
 *
 * Revision 1.66.12.1  2000/10/06 20:32:19  fladm
 * User: dibb
 * Reason: cleanup
 * merge from trunk as of update 2000_08_30_15_40_43 2114.
 * Preparation for merge to K10
 *
 * Revision 1.67  2000/10/05 15:18:49  fladm
 * User: dspeterson
 * Reason: sus_5756
 * Implement Clear Cache Dirty fcli command and modepage
 *
 * Revision 1.66.10.1  2000/07/21 17:04:03  fladm
 * User: rfoley
 * Reason: MERGE
 * Merge from raidmm -> cat1.
 *
 * Revision 1.57.2.1  2000/04/11 13:37:20  fladm
 * User: lvermeulen
 * Reason: Development
 * Clean up of compiler warnings.
 *
 * Revision 1.66  2000/02/09 18:05:32  fladm
 * User: hopkins
 * Reason: Trunk_EPR_Fixes
 * EPR 3632 - Moved error and information codes to
 * sunburst_errors.h
 *
 * Revision 1.65  2000/02/07 21:20:31  fladm
 * User: ellistb
 * Reason: Ethernet_Service_Port
 * Fixes for EPR 3570:
 * -- Streamlined CM/network stack config process and
 *    resolved configuration consistency issues.
 * -- Added pipe to Net Daemon in order to receive
 *    status from CM config operations
 * -- Rebooting the SP in order to re-enable
 *    network configuration over the ethernet interface is
 *    no longer necessary.
 * -- Network init/config is logged in ulog.
 * -- Telnet connects/disconnects/login-failures
 *    are logged in ulog.
 * -- Network stack BSD-style panic/log operations
 *    result in Flare panic and ulog operations.
 * -- "setlan" command now displays ethernet address
 *    of SP board.
 * -- Consolidated network port numbers, connection
 *    limits and memory allocation limits into
 *    new top-levvel include file (net_defs.h)
 * -- CM network database structures moved into
 *    separate include file (net_db.h).
 * -- Telnetd now displays hostname during login or
 *    IP address if hostname is null.
 * -- Added missing enable/disable interrupt calls in
 *    net_mem.c and net_notify.c
 * -- Removed debug printf() output.
 * -- Re-added missing panic() for LAN_INT interrupt
 *    when network is disabled.
 * -- Added debug code for tracing network-allocated
 *    hemi timers. Used for debugging EPR 3556.
 * -- Output from "netstat -i" command now fits in 80 column
 *    terminal window.
 * -- Replaced strcpy()'s with strncpy()'s when
 *    manipulating network hostname parameter to
 *    prevent buffer overruns.
 * -- Added panic codes for network stack and socket driver.
 * -- Moved ETHERNET_PANIC_BASE into panic.h
 *
 * Revision 1.64  2000/02/04 16:19:02  fladm
 * User: mogavero
 * Reason: Alpine_PhaseII
 * Added some sunburst codes for disk obituaries. The addition
 * of this functionality is being tracked by EPR 3533.
 *
 * Revision 1.63  2000/02/01 12:42:37  fladm
 * User: kingersoll
 * Reason: Hi5_EPR_Fixes
 * Fix most critical case for Hi5 EPR#3338
 *
 * Revision 1.62  2000/01/31 21:11:06  fladm
 * User: sfimlaid
 * Reason: Alpine_PhaseII
 * epr3509,3510,3533,getwwn
 *
 * Revision 1.61  2000/01/14 23:02:25  fladm
 * User: sfimlaid
 * Reason: Alpine_PhaseII
 * CM Fibre WWN type5 support
 *
 * Revision 1.60  2000/01/14 19:16:55  fladm
 * User: ellistb
 * Reason: Ethernet_Service_Port
 * Migration of Alpine-LAN code from netbush2 tree to
 * main.
 * Note: "-define NETWORK" must be specified on the
 * build command in order to enable networking. Two new
 * targets have been added to faciliate building alpine
 * images w/ network enabled: alpine_sim_net and
 * alpine_hw_net.
 * Check-in Contents:
 * - Two new components (flnet/flnet2) containing the
 *   core network stack, API's, Navi network interface daemon,
 *   and ethernet/socket drivers.
 * - CM support for network database and initialization
 *   and configuration.
 * - FCF/FED/SFE support for the Navi network interface
 * - Hemi support for the network daemon processes and
 *   new virtual drivers (ethernet, socket)
 * - Two new targets to onames.txt to build alpine
 *   with network enabled.
 *
 * Revision 1.59  1999/12/22 21:45:23  fladm
 * User: gpeterson
 * Reason: MERGE
 * merged in sunburst.h codes from other branches
 *
 * Revision 1.58  1999/12/13 14:19:29  fladm
 * User: jdidier
 * Reason: gcc_pedantic_compile
 * modifications to clean up gcc -pedantic warnings/errors in cache module
 *
 * Revision 1.57  1999/11/18 21:34:17  fladm
 * User: pkumar
 * Reason: HI5_BUG_FIXES
 * Added a new pound-define "HI_NON_NATIVE_CODE". This
 * accomodates for a new opcode typically used when we handle
 * a super-buffer write reqest of a non-native type of ucode
 * install.
 *
 * Revision 1.56  1999/11/09 18:55:10  fladm
 * User: mzelikov
 * Reason: TRUNK_EPR_2899
 * Added new codes:
 * HOST_FRU_EXPANSION_CHKPT_REBUILD_FAILED,
 * HOST_CANT_ASSIGN_EXP_CHKPTS_DB_MISMATCH.
 *
 * Revision 1.55  1999/10/29 21:31:25  fladm
 * User: kingersoll
 * Reason: Automode
 * Restrict host ability to execute binds, rg configs, etc if
 * automode is enabled.
 *
 * Revision 1.54  1999/10/28 18:42:27  fladm
 * User: bruce
 * Reason: HI5_BUG_FIXES
 * Added new sunburst error code HOST_L2_CACHE_NOT_ENABLED.
 *
 * Revision 1.53  1999/10/27 22:32:39  fladm
 * User: jjyoung
 * Reason: Automode
 * Added HI_AUTOMODE_CONFIG and HI_CHANGE_AUTOMODE_STATE.
 *
 * Revision 1.52  1999/10/26 02:31:28  fladm
 * User: mzelikov
 * Reason: TRUNK_EPR_2915
 * Added new UNBIND error code
 * HOST_UNBIND_WAIT_INTERNAL_ASSIGN_RELEASE.
 *
 * Revision 1.51  1999/10/21 23:09:12  fladm
 * User: myellen
 * Reason: Automode
 * Added define for new PIO command:  HI_PEER_ASSIGN
 *
 * Revision 1.50  1999/10/04 20:19:02  fladm
 * User: mzelikov
 * Reason: TRUNK_EPR_2674
 * Added two new information codes:
 * HOST_FRU_BAD_EXPANSION_CHKPTS and
 * HOST_FRU_EXPANSION_REVISION_MISMATCH.
 * Added new assign error:
 * HOST_CANT_ASSIGN_BAD_EXPANSION_CHKPT.
 *
 * Revision 1.49  1999/10/04 17:09:52  fladm
 * User: hopkins
 * Reason: Alpine_debug
 * EPR 2668.  Removed STORAGE_CENTRIC_INHIBIT from sim build's
 * environment.h.       Added new sunburst error code.
 *
 * Revision 1.48  1999/09/23 19:17:23  fladm
 * User: kdobkows
 * Reason: EPR_Fixes
 * Added HOST_UNSUPPORTED_CANT_ASSIGN sunburst code.
 *
 * Revision 1.47  1999/09/22 23:47:05  fladm
 * User: kschleicher
 * Reason: TEK_VF_changes
 * Added HOST_ASSIGN_REMAP_FAILED, HOST_ASSIGN_REMAP_STARTED,
 * and HOST_ASSIGN_REMAP_COMPLETED.
 *
 * Revision 1.46  1999/09/21 15:46:11  fladm
 * User: bcurran
 * Reason: TEK_VF_changes
 * Undo update due to bad merge.
 *
 * Revision 1.44  1999/07/21 23:11:40  fladm
 * User: mburnett
 * Reason: dsa_navi_interface
 * DSA/Navi interface additions
 *
 * Revision 1.43  1999/07/21 14:30:35  fladm
 * User: dbeauregard
 * Reason: Hiraid_EPR_Fixes
 * Added HOST_ILLEGAL_UPREV_CONFIGURATION and
 * HOST_ILLEGAL_DOWNREV_CONFIGURATION.
 *
 * Revision 1.42  1999/07/10 13:36:13  fladm
 * User: tdibb
 * Reason: K1_EPR_Fixes
 * EPR 1431 Added logging and workaround for TACHYON TS loss
 * of credit /loop state timeout errata
 *
 * Revision 1.41  1999/07/01 21:20:14  fladm
 * User: aldred
 * Reason: Storage_Centric
 * Changed HOST_SC_UNIT_STILL_BINDING to
 * HOST_ONLY_ONE_MAPPING_TO_BINDING_LUN to meet a Navi
 * request. Also changed HOST_SC_VLU_FLU_MAPPING_REMOVED to be
 * defined as 0x209B.
 *
 * Revision 1.40  1999/06/25 13:15:05  fladm
 * User: mzelikov
 * Reason: Storage_Centric_Regression_Testing
 * Added new code: HOST_SC_UNIT_STILL_BINDING 0x209A
 *
 * Revision 1.39  1999/06/18 18:24:22  fladm
 * User: dbeauregard
 * Reason: Hiraid_EPR_Fixes
 * Fix for Hiraid EPR #1677.  I pulled out recent changes
 * which replaced pending the aep when retrieving peer lun
 * info for mode page 0x37.
 *
 * Revision 1.38  1999/06/16 20:26:53  fladm
 * User: aldred
 * Reason: Storage_Centric_Regression_Testing
 * Added code HOST_VLU_CANT_MAP_TO_HOT_SPARE
 *
 * Revision 1.37  1999/06/08 18:28:35  fladm
 * User: cburns
 * Reason: K1_Development
 * Reason: K1_EPR_Fixes
 * Added 2 new messages to sunburst to rebort front end loop
 * initialization so do not get FIBRE_UNKNOWN messages.
 *
 * Revision 1.36  1999/06/08 11:48:08  fladm
 * User: dbeauregard
 * Reason: Hiraid_EPR_Fixes
 * Commented out HI_GET_PEER_FRU_INFO and HI_GET_PEER_LUN_INFO
 * which are no longer being used.
 *
 * Revision 1.35  1999/06/07 14:57:39  fladm
 * User: ellistb
 * Reason: PortableLUNS
 * Initial version of Portable LUNs.
 * Until Portable LUNs gets qualified, Portable LUNs
 * is by default disabled for all hardware builds and
 * enabled by default for all simulation builds.
 * Macros for overriding the default behavior are provided
 * in cm_raid_group.h
 *
 * Revision 1.34  1999/06/01 15:39:50  fladm
 * User: kdobkows
 * Reason: Storage_Centric_Regression_Testing
 * Added new code HOST_SC_CANT_REMOVE_DEFAULT_VA 0x208A.
 *
 * Revision 1.33  1999/05/17 15:21:37  fladm
 * User: dbeauregard
 * Reason: Hiraid_EPR_Fixes
 * Added HOST_UNIT_SHUTDOWN_FOR_UNBIND.
 *
 * Revision 1.32  1999/05/17 13:38:57  fladm
 * User: kdobkows
 * Reason: Storage_Centric_Regression_Testing
 * Added HOST_CANT_MODIFY_SPECIAL_VA.
 *
 * Revision 1.31  1999/05/07 19:32:04  fladm
 * User: snickel
 * Reason: Hiraid_EPR_Fixes
 * Send HOST_INCORRECT_CHECKSUM_TYPE to ULog if can't assign due to unsupported checksum format. (Hiraid EPR #1290)
 *
 * Revision 1.30  1999/05/06 18:54:36  fladm
 * User: zxiao
 * Reason: Hiraid_EPR_Fixes
 * Added HOST_UNBIND_WAIT_FOR_STOP_ZERO.
 *
 * Revision 1.29  1999/05/05 21:34:45  fladm
 * User: aldred
 * Reason: Storage_Centric_Regression_Testing
 * Fix EPRs 1244 and 1252.  Both are bind bugs introduced by SC development.
 *
 * Revision 1.28  1999/05/04 21:35:57  fladm
 * User: iknyazhitskiy
 * Reason: Storage_Centric
 * Added Heterogeneous Hosts Support.
 * An addition to system type and system options there are
 * initiator type and initiator options. The options and type
 * are loaded into IOPB ptr when request comes on board and
 * follow a command till completion.
 *
 * Revision 1.27  1999/04/30 20:15:58  fladm
 * User: hopkins
 * Reason: Storage_Centric
 * Added HOST_INV_TYPE code
 *
 * Revision 1.26  1999/04/23 16:40:22  fladm
 * User: cburns
 * Reason: K1_EPR_Fixes
 * Fix #define assignments for FCF codes.
 *
 * Revision 1.25  1999/04/22 18:20:57  fladm
 * User: cburns
 * Reason: K1_EPR_Fixes
 * Need to set the tcb_ptr->flag PEND_FREEZE before calling
 * fcf_freeze_exchange.  Fix for EPR# 884.
 *
 * Revision 1.24  1999/04/22 14:15:23  fladm
 * User: zxiao
 * Reason: Hiraid_EPR_Fixes
 * Change HOST_FRU_UNBIND to HOST_LUN_UNBIND.
 *
 * Revision 1.23  1999/04/13 13:27:35  fladm
 * User: dbeauregard
 * Reason: Hiraid_EPR_Fixes
 * Changed the level of severity for sunburst info codes
 * HOST_FRU_DB_RECONSTRUCTED and HOST_RG_DB_RECONSTRUCTED.
 *
 * Revision 1.22  1999/04/12 19:52:14  fladm
 * User: zxiao
 * Reason: Hiraid_EPR_Fixes
 * Added a new ulog: HOST_SP_RESYNCRONIZE_PANIC.
 *
 * Revision 1.21  1999/04/09 19:53:29  fladm
 * User: aldred
 * Reason: Storage_Centric
 * Removed CM_UNIT_NUMBER from flare.  It had a value of 255
 * and we now support 256 virtual logical units and this
 * scared me.  Anyway, the code was of no use.
 *
 * Revision 1.20  1999/04/07 20:50:30  fladm
 * User: hopkins
 * Reason: Storage_Centric
 * Added HOST_TLD_LENGTH_ERROR and renamed
 * HOST_OPERATION_COLLISION to HOST_OPERATION_DATA_OVERRUN
 *
 * Revision 1.19  1999/04/07 00:21:53  fladm
 * User: mramamurthy
 * Reason: Storage_Centric
 * Added sunburst code HOST_FLU_NAME_UPDATE_FAILED
 *
 * Revision 1.18  1999/04/02 22:59:50  fladm
 * User: hopkins
 * Reason: Storage_Centric
 * Added four new codes for errors when parsing SC input data
 *
 * Revision 1.17  1999/04/01 22:33:23  fladm
 * User: corzine
 * Reason: K1_Development
 * Added Sunburst and ULOG infrastructure for Diskmail ULOG entries.
 *
 * Revision 1.16  1999/04/01 18:39:36  fladm
 * User: aldred
 * Reason: Storage_Centric
 * Added support in cm_bind for storage centric VLU and FLU
 * type bind commands.
 *
 * Revision 1.15  1999/03/26 22:00:57  fladm
 * User: aldred
 * Reason: Storage_Centric
 * Added some new sunburst codes and changed a FKEY bit mask from 0x5 to 0x8
 *
 * Revision 1.14  1999/03/26 19:21:37  fladm
 * User: kdobkows
 * Reason: Storage_Centric
 * Added VLUT database sunburst codes.
 *
 * Revision 1.13  1999/03/22 19:51:29  fladm
 * User: hopkins
 * Reason: Storage_Centric
 * Added invalid VA Parameter code
 *
 * Revision 1.12  1999/03/18 15:05:55  fladm
 * User: bkeesee
 * Reason: Warm_Reboot
 * Added support for Warm Reboot
 *
 * Revision 1.11  1999/03/17 13:46:33  fladm
 * User: gordon
 * Reason: Storage_Centric
 * Added Host error codes relating to Feature Key operations.
 *
 * Revision 1.10  1999/03/15 19:00:46  fladm
 * User: dbeauregard
 * Reason: Hiraid_EPR_Fixes
 * Added new sunburst error codes for expansion errors (part
 * of fix for EPR 687).
 *
 * Revision 1.9  1999/03/04 19:15:36  fladm
 * User: gpeterson
 * Reason: Storage_Centric
 * Added some Storage Centric definies and macros
 *
 * Revision 1.8  1999/03/01 13:44:12  fladm
 * User: iknyazhitskiy
 * Reason: Storage_Centric
 * Management Login added
 *
 * Revision 1.7  1999/02/25 13:05:58  fladm
 * User: aldred
 * Reason: Storage_Centric
 * made some header file changes and made the host_interface_revision_number 8 bits
 *
 * Revision 1.6  1999/02/24 16:03:56  fladm
 * User: dibb
 * Reason: colossus_drive_filtering
 * add new code 0x6077, used when colossus drives are in some way incorporated into a multi-lun raid group.
 *
 * Revision 1.5  1999/02/23 01:38:52  fladm
 * User: zxiao
 * Reason: Development
 * Define FEATURE_DISK_ZEROING.
 *
 * Revision 1.4  1999/02/18 23:44:06  fladm
 * User: hopkins
 * Reason: Storage_Centric
 * Added one more Error code
 *
 * Revision 1.3  1999/02/17 16:04:35  fladm
 * User: hopkins
 * Reason: Storage_Centric
 * New #defines for Storage Centric
 *
 * Revision 1.2  1999/02/11 20:18:13  fladm
 * User: dbeauregard
 * Reason: Development
 * Added FEATURE_HI5_LUNS, HOST_EXPAND_XL_FLAG_INVALID, HOST_EXPANSION_XL_FAILED.
 *
 * Revision 1.1  1999/01/05 22:29:14  fladm
 * User: dibb
 * Reason: initial
 * Initial tree population
 *
 * Revision 1.82  1999/01/04 15:30:48  dbeauregard
 *  Added Flare Feature FEATURE_RAID_GROUPS to be used in Feature 5 of Mode page 0x37.
 *
 * Revision 1.81  1998/12/15 18:47:02  dbeauregard
 *  Added ABORT_INIT_BIND flag to be used when aborting binds on powerup.
 *
 * Revision 1.80  1998/12/07 20:42:31  jjyoung
 *  Merge from K2.  Added HOST_SP_FAILBACK_FAILED to notify that the failback failed probably because enclosure 0 is not present.
 *
 * Revision 1.79  1998/12/02 15:49:13  dbeauregard
 *  Set the INTERFACE_REVISION_NUMBER back to 0 to make life easier for all of our great Hiraid regression test engineers.
 *
 * Revision 1.78  1998/11/11 16:30:42  kingersoll
 *  Factory-zero operations now appear in the ulogs
 *
 * Revision 1.77  1998/10/28 16:29:39  dbeauregard
 *  Added HOST_CREATE_RG_BAD_FRU_CONFIGURATION, HOST_REMOVE_RG_ERROR_VALID_LUNS, HOST_EXPAND_ERROR_FRU_TOO_SMALL, and HOST_BIND_ILLEGAL_L2_CACHE_SIZE.  Removed HOST_CREATE_RAID_GROUP_ERROR and HOST_REMOVE_RAID_GROUP_ERROR.
 *
 * Revision 1.76  1998/10/26 15:08:58  kingersoll
 *  Added error codes for factory zero operations.
 *
 * Revision 1.75  1998/10/20 17:59:09  dbeauregard
 *  Added define for MAX_INTERFACE_REVISION_NUMBER.
 *
 * Revision 1.74  1998/10/14 18:40:51  bruce
 *  Initial version for new Hiraid tree;  synch up with k2.2 tree.
 *
 * Revision 1.73  1998/09/16 19:10:50  dbeauregard
 *  Added raid group configuration error codes.
 *
 * Revision 1.72  1998/09/11 14:20:02  dbeauregard
 *  Fixed bug with HOST_RAID_GROUP_BUSY_ERROR, should be 0x50FF instead of 0x5FFF.
 *
 * Revision 1.71  1998/09/02 15:00:16  kingersoll
 *  new command HI_ZERO_DISK.
 *
 * Revision 1.70  1998/08/26 14:56:35  dbeauregard
 *  Added HOST_RAID_GROUP_BUSY_ERROR to be used when the peer reports an error because the raid group is currently busy.
 *
 * Revision 1.69  1998/08/04 14:40:43  gtracy
 *  sync with k2_1_01_10. this added support for unbinding a unit while it is binding.
 *
 * Revision 1.68  1998/07/16 17:42:03  kingersoll
 *  new error code: HOST_BIND_BAD_ADDRESS_OFFSET
 *
 * Revision 1.67  1998/06/25 14:27:54  dbeauregard
 *  Modified to add Flare10 host interface support (Interface Revision 1).
 *
 * Revision 1.66  1998/06/12 18:44:50  bkhung
 *  Added HOST_BIND_WRITE_CACHE_NOT_CONFIGURED, HOST_UNIT_L2_CACHE_DIRTY_NO_RESOURCES, and HOST_CANT_ASSIGN_L2_CACHE_DIRTY_UNIT.
 *
 * Revision 1.64  1998/05/26 15:06:09  dbeauregard
 *  Merge with K2,  *  Added HOST_R3_MEMORY_SIZE_REDUCED, HOST_SPA_CACHE_SIZE_REDUCED and HOST SPB CACHE_SIZE_REDUCED.
 *
 * Revision 1.63  1998/05/01 12:30:00  browlands
 *  (from Mike Gordon) get sunburst.h in synch between various trees
 *
 * Revision 1.62  1998/04/17 17:43:54  bkhung
 *  Added defines to better describe SPS Failure status in the Ulog.
 *
 * Revision 1.61  1998/03/18 15:16:02  dbeauregard
 *  Two new defines: HOST_UNIT_RELEASED and HOST_EXPANSION_STOPPED.
 *
 * Revision 1.60  1998/02/19 15:10:09  dbeauregard
 *  Added unsolicited host messages for database consistency check.
 *
 * Revision 1.59  1998/01/19 21:36:46  dbeauregard
 *  RAID Group, LUN Partition, Expansion and Virtual Driver support added in the Great Merge of '98'.  (f10 -> hiraid)
 *
 * Revision 1.57  1997/12/05 17:52:46  dbeauregard
 * ADDED to PRODUCT hiraid from PRODUCT k5ph3 on Thu Dec 18 08:44:24 EST 1997
 *  Added INTERFACE_REVISION_NUMBER to support new feature in mode page 0x37h.
 *
 * Revision 1.56  1997/12/01 16:13:18  gtracy
 *  This implements the alt-cmi cache shutdown and recovery plan.
 *
 * Revision 1.55  1997/11/14 16:24:40  carll
 *  Added SERIAL_DRIVE_DOWNLOA
 *
 * Revision 1.54  1997/11/10 15:26:08  rfoley
 *  Added unsolicited code HOST_CONFIG_TOO_MUCH_CACHE.
 *
 * Revision 1.53  1997/10/30 14:38:24  lathrop
 *  make PCMCIA_CARD_REMOVED unused, remove PCMCIA_CARD_INSERTED/PCMCIA_CARD_WRITE_ENABLED/PCMCIA_CARD_WIPED_BY_USER_REQUEST/HOST_FIBRE_LINK_UP/HOST_FI
 *
 * Revision 1.52  1997/10/01 14:40:11  sears
 * ADDED to PRODUCT k5ph3 from PRODUCT k5ph2 on Tue Oct 21 10:57:14 EDT 1997
 *  New #defines for log events.
 *
 * Revision 1.51  1997/09/10 14:20:43  tolvanen
 *  added information-only code HOST_CM_EXPECTED_SP_RESTART to give reason for kill_thyself restarts
 *
 * Revision 1.50  1997/06/05 19:06:05  lathrop
 *  change SM_READ_AND_LOCK-SM_READ_AND_XOR, MD_SM_READ_AND_LOCK-MD_SM_READ_AND_XOR
 *
 * Revision 1.49  1997/05/27 18:44:23  jwentwor
 *  Added HI_CHANGE_DISCOVERY_FLAGS
 *
 * Revision 1.48  1997/05/21 15:53:56  jwentwor
 *  add HOST_RECOVERABLE_RESET_EVENT (SML)
 *
 * Revision 1.47  1997/05/06 16:44:31  lathrop
 *  add HOST_UNCORRECTABLE_VERIFY_ERROR from F950p
 *
 * Revision 1.1  1997/05/02 20:47:54  lathrop
 * Initial revision
 *
 * Revision 1.46  1997/04/11 14:54:22  tolvanen
 *  added codes HOST_ENCL_INVAL_ADDR, HOST_ENCL_DUP_ADDR, HOST_NON_R3_LUNS_CANT_CONVERT
 *
 * Revision 1.45  1997/04/04 21:07:21  kdobkows
 *  added HI_DRIVE_DOWNLOAD and HOST_DUAL_BOARD_DRIVE_DOWNLOAD_ERROR for dual board disk drive firmware downloads
 *
 * Revision 1.44  1997/03/20 16:17:17  tolvanen
 *  added HI_CHANGE_VERIFY_CONFIG, HOST_DEGRADED_NONVOL_VERIFY_DATA_LOSS, HOST_TERMPOWER_LOW, HOST_NON_R3_CANT_ASSIGN
 *
 * Revision 1.43  1997/02/21 17:16:08  jwentwor
 *  change HI_FORMAT to HI_FORMAT_DISK, add HI_CHANGE_AUTO_FORMAT/HOST_FRU_UNFORMATTED_STATE (SML)
 *
 * Revision 1.42  1997/02/11 16:30:11  lathrop
 *  add HOST_SP_PROM_REV/HOST_FLARE_LOADED/HOST_DRIVE_CODE_LOADED/HOST_FRU_BIND/HOST_FRU_UNBIND/HOST_FUSE_BLOWN from Flare 9.50+
 *
 * Revision 1.41  1997/01/21 16:04:27  lathrop
 *  add codes from f-9.60
 *
 * Revision 1.40  1997/01/10 15:10:57  lathrop
 *  move types to bottom of file, combine w/ Flare9.60, remove obs types
 *
 * Revision 1.39  1997/01/06 17:16:13  jwentwor
 *  Added unsol codes for new Front End fibre events
 *
 * Revision 1.38  1996/12/06 19:18:38  jwentwor
 *  Added Front End unsol events & 'sending crazy peer' event
 *
 * Revision 1.37  1996/11/25 17:55:55  tolvanen
 *  added failover/failback informatory unsolicited msg codes
 *
 * Revision 1.36  1996/11/06 18:50:25  jwentwor
 *  Added new Fibre related events and enclosure state change event
 *
 * Revision 1.35  1996/10/22 14:32:25  aldred
 *  added HOST_FRU_EQZ_* and HOST_BCKGRND_CHKPT_VERIFY_ABORTED for NLM port.
 *
 ***************************************************************************/

/*
 * LOCAL TEXT unique_file_id[] = "$Header: /fdescm/project/CVS/flare/src/include/sunburst.h,v 1.66.12.2 2000/10/13 14:23:38 fladm Exp $"
 */

/*************************
 * INCLUDE FILES
 *************************/

#include "generics.h"
#include "corona_config.h"
#include "sg.h"
#include "sunburst_errors.h"

#include "fed_types.h"

/*******************************
 * LITERAL DEFINITIONS
 *******************************/

#define SUNBURST_PROTOCOL_REVISION 0x00000004

/* The following defines the Flare Feature list that is included in Page 0x37
 * following the Interface Revision ID.
 */
#define FLARE_FEATURE_MASK             0x03
#define FEATURE_RAID_GROUPS            0x01
#define FEATURE_HI5_LUNS               0x02
#define FEATURE_DISK_ZEROING           0x04
#define FEATURE_FKEY_DB                0x08
#define FEATURE_DSA                    0x10

/* The following are the major revision number for flare starting with
 * Denali with revision 4.
 */
#define FLARE_MAJOR_REVISION_0        0 /* Debug */
#define FLARE_MAJOR_REVISION_1        1 /* Beta releases */
#define FLARE_MAJOR_REVISION_4        4 /* Denali */
#define FLARE_MAJOR_REVISION_5        5 /* Partition Denali */

/* The following section defines masks used for the configuration register. */

#define ARGUMENT_MASK 0xFFFF0000
#define ARGUMENT_SHIFT 16
#define COMMAND_MASK  0x0000FF00
#define COMMAND_SHIFT 8
#define SEQUENCE_MASK 0x000000FF

/* The sauna design doesn't require SG entries.  We are leaving 1
 * for space saving and minimal code modifcations.  JAB 10/17/91
 * The SG_MAX_ENTRY defines the maximum length of the scatter/gather list.
 */
#define SG_MAX_ENTRY 1

/* SG_SECTION defines the number of sg entries contained in a request 
 * block, or an extended sg list.
 */
#define SG_SECTION 1

/* The following masks are used to test addresses and counts of the scatter
 * gather entries on Corona Requests.  These requests require that no
 * odd byte, or odd word transfers be used.
 * 8/96 -- these are still used in cm_move_data.c file.
 */
#define SG_ADDRESS_MASK          0xFFFFFFFC
#define SG_COUNT_MASK            0x03FFFFFC
#define SG_BYTE_COUNT_MASK       0x03FFFFFF
#define SG_ADDRESS_MODIFIER_MASK 0xFC000000

/* The request is the entry which defines what the host wants us
 * to do.  It contains the opcode, disk address, transfer block
 * count, etc.  This request is part of a request block.  It is also
 * union'ed with a byte array used for SCSI CDB's.  The software will
 * be capable of determining which entity is contained within the
 * request by examining the unit number.  Unit numbers are assigned
 * to SCSI devices, and Unit Numbers are assigned Corona devices.
 */

/* The following defines apply to the opcode field of the CORONA REQUEST. */

/* This set of opcodes can be sent to units on the controller.  They cannot
 * be sent to the controller.
 */

#define HI_CLOSE             0x00000001
#define HI_OPEN              0x00000002
#define HI_READ              0x00000003
#define HI_WRITE             0x00000004
#define HI_REPORT            0x00000005
#define HI_SHUTDOWN          0x00000006
#define HI_CORRUPT_CRC       0x00000007

#define HI_SCSI_CDB          0x00000040
#define HI_SCSI_CANCEL       0x00000041

/* Requests sent to this unit number are sent to the Config mgr. */
#define CM_UNIT_NUMBER       ((UINT_8E) -1)

/* This set of opcodes can be sent to unit CM_UNIT_NUMBER only.
 * They cannot be sent to the units of the controller.
 */

#define HI_ASSIGN                 0x00000080
#define HI_BIND                   0x00000081
#define HI_CHANGE_FRU_STATE       0x00000082
#define HI_UNSOLICITED_CTRL       0x00000083
#define HI_FORMAT_DISK            0x00000084
#define HI_INSTALL                0x00000085
#define HI_LOG_CONTROL            0x00000086
#define HI_TRESPASS               0x00000087
#define HI_RELEASE                0x00000088
/*#define HI_CONTROLLER_REPORT    0x00000089 */
#define HI_ZERO_DISK              0x0000008A
#define HI_UNBIND                 0x0000008B
#define HI_BACKDOOR_POWER         0x0000008C
#define HI_RG_CONFIGURATION       0x0000008D
#define HI_DIAGNOSE               0x0000008E
#define HI_ERASE_DATABASE         0x0000008F
#define HI_DOWNLOAD               0x00000090
#define HI_FRU_VERIFY             0x00000091
#define HI_CHANGE_BIND            0x00000092
#define HI_ABORT_BIND             0x00000093
#define HI_UNIT_CACHE_PARAM       0x00000094
#define HI_EN_DIS_SYSTEM_CACHE    0x00000095
#define HI_SYSTEM_CACHE_PARAM     0x00000096
#define HI_SYNCHRONIZE_TIME       0x00000097
#define HI_PW                     0x00000098
#define HI_PW_REQ                 0x00000099
#define HI_CHANGE_HOST_INTERFACE  0x0000009A
#define HI_EN_DIS_BBU_SNIFF       0x0000009B
#define HI_SEND_DIAG              0x0000009C
#define HI_SEND_DIAG_DUAL         0x0000009D
#define HI_SUP_BUF                0x0000009E
#define HI_PERIODIC_ERRORS        0x0000009F
#define HI_STATS_LOGGING          0x000000A0
#define HI_CHANGE_SYSTEM_TYPE     0x000000A1
#define HI_CHANGE_R3_PARAMETERS   0x000000A2
#define HI_CHANGE_FR3_PARAMETERS  HI_CHANGE_R3_PARAMETERS
#define HI_CHANGE_SYSTEM_CONFIG   0x000000A3
#define HI_SAVE_PAGES             0x000000A4
#define HI_CHANGE_EXT_SYS_MEM     0x000000A5
#define HI_EN_DIS_READ_CACHE      0x000000A6
/* #define HI_GET_PEER_LUN_INFO      0x000000A7  *** OBSOLETE ***
#define HI_GET_PEER_FRU_INFO      0x000000A8 */
#define HI_WIPE_PCMCIA_MEMORY     0x000000A9
#define HI_FLASH_REQUEST          0x000000AA
#define HI_CHANGE_AUTO_FORMAT     0x000000AB
#define HI_CHANGE_VERIFY_CONFIG   0x000000AC
#define HI_DRIVE_DOWNLOAD         0x000000AD
#define HI_CHANGE_DISCOVERY_FLAGS 0x000000AE
#define SERIAL_DRIVE_DOWNLOAD     0x000000AF
#define HI_CHANGE_SERIAL_NUMBER   0x000000B0
#define HI_SC_CMD                 0x000000B1
#define HI_SUP_BUF_WARM_REBOOT    0x000000B2    /* Warm Reboot */
#define HI_CLEAR_FRU_EXPORT       0x000000B3
#define HI_PEER_ASSIGN            0x000000B4
#define HI_AUTOMODE_CONFIG        0x000000B5
#define HI_CHANGE_AUTOMODE_STATE  0x000000B6
#define HI_NON_NATIVE_CODE        0x000000B7
#define HI_FLNET_CONFIG           0x000000B8
#define HI_SEND_DIAG_DUAL_HARD    0x000000B9
#define HI_FORCECLEAN             0x000000BA
#define HI_ADM_SYNCHRONIZE_TIME   0x000000BB
#define HI_REBUILD_FRU_SIG        0x000000BC
#define HI_CHANGE_SYSTEM_ID_INFO  0x000000BD
#define HI_GROUP_VERIFY           0x000000BE
#define HI_DRIVE_GETLOG           0x000000BF
#define HI_REBOOT_SP              0x000000C0
#define HI_ADM_SYSTEM_BUS_RESET   0x000000C1
#define HI_LCC_SYNC_CMD           0x000000C2
#define HI_LCC_STATS_CMD          0x000000C3
#define HI_PROACTIVE_SPARE        0x000000C4
#define HI_LCC_FAULT_INSERTION    0x000000C5
#define HI_GET_YUKON_LOG_CMD      0x000000C6
#define HI_FRU_POWER              0x000000C7
#define HI_XPE_STATS_CMD          0x000000C8
#define HI_CONFIG_MGMT_PORT       0x000000C9
#define HI_SET_PORT_DATA          0x000000CA
#define HI_LED_CTRL_REQUEST       0x000000CB
#define HI_DB_READ_WRITE_TBL      0x000000CC
#define HI_INITIALIZE_ENCLOSURE   0x000000CD
#define HI_ADM_REBOOT             0x000000CE
#define HI_ADM_SHUTDOWN           0x000000CF
#define HI_GET_EXPANDER_TRACE_CMD 0x000000D0
#define HI_FCLI_EXPANDER_CMDS     0x000000D1
#define HI_DRIVE_DOWNLOAD_ABORT   0x000000D2
#define HI_POWER_SAVING_CONFIG    0x000000D3
#define HI_BGS_HALT_CMD           0x000000D4
#define HI_BGS_UNHALT_CMD         0x000000D5
#define HI_MODIFY_CAPACITY        0x000000D6
#define HI_MODIFY_INT_CAPACITY    0x000000D7
#define HI_FCLI_FETCH_AEN_ENCL_INFO_CMDS     0x000000D8
#define HI_DBC_CMD                0x000000D9
#define HI_SET_SLIC_UPGRADE       0x000000DA
#define HI_FUP_ABORT              0x000000DB
#define HI_FUP_RESUME             0x000000DC
#define HI_EFUP_FW_UPGRADE        0x000000DD
#define HI_EFUP_FW_UPGRADE_STAT   0x000000DE
#define HI_PS_MARGIN_CONTROL      0x000000DF
#define HI_NDU_GO_FORWARD         0x000000E0
#define HI_SYSTEM_MEM_VAL_CHANGE  0x000000E1

/* The following define trespass options (ie. HI_TRESPASS).
 */

#define HI_TRESPASS_HONOR_RESERVATION 0x00000001


/* The following definitions are to be used with "HI_REPORT" opcode,
 * as sub-operation codes.
 */

#define OUTSTANDING_REQUEST_INFORMATION  0x00000001
#define UNIT_INFORMATION                 0x00000002
#define STRIPE_PARAMETERS                0x00000003

/* Read and Write IO Flags
 * The following bits are defined as flags for Read and Write commands.
 * THEY NO LONGER ARE CONSIDERED AS PART OF THE FLAGS FOR REQUEST-BLOCKS,
 * SINCE REQUEST-BLOCKS ARE NO LONGER USED FOR READS AND WRITES.  Therefore
 * only GENERATE routines use these.    31-Mar-94   Dave DesRoches
 */
#define CORRUPT_PARITY           0x00000080
#define CORRUPT_DATA             0x00000040
#define WRITE_VERIFY             0x00000020
#define AUTO_SENSE               0x00000010
#define CORRECT_PARITY           0x00000008
#define OPTIMIZE                 0x00000004
#define DIRECTION                0x00000002
#define SERIALIZE                0x00000001

/* The following definitions are used for the sub_argument 
 * of CHANGE_FRU_STATE requests.
 */
#define ENABLE_FRU         0x0001
#define DISABLE_FRU        0x0002
#define POWER_ON_FRU       0x0003
#define POWER_OFF_FRU      0x0004
#define LED_ON_FRU         0x0005
#define LED_OFF_FRU        0x0006
#define FORMAT_LOAD_FRU    0x0007

/* The following defines are "whys" to be used as extended
 * statuses on cache unsoliciteds
 */
#define EXT_STAT_PEER_DEAD      0xC0
#define EXT_STAT_BBU_FAULT      0xC1
#define EXT_STAT_AC_FAULT       0xC2
#define EXT_STAT_SAFE_GONE      0xC3
#define EXT_STAT_NO_MEM_LEFT    0xC4
#define EXT_STAT_USER_REQUEST   0xC5





/**************************************************************
 *
 *   SUNBURST ERROR CODES HAVE BEEN MOVED TO sunburst_errors.h
 *
 **************************************************************/



/* The following section defines the stuff to be placed
 * into the status field of the status block.
 */
#define HOST_STATUS_IS_OK                           0x00
#define HOST_STATUS_IS_ERROR                        0x01
#define HOST_STATUS_IS_ABORT                        0x02
#define HOST_STATUS_IS_INFORMATION                  0x03


/* At various points in the MD or BEM state machines an extended status word
 * may be sent to the Ulog to provide information on soft or hard errors which
 * were encountered. These Ulog messages replace the generic "Logical Sector
 * Data Error" (635) error messages.
 *
 * These Ulog messages have an extended status word with the following format:
 *
 *  31         24 23         16 15          8 7           0
 * ---------------------------------------------------------
 * |             |  State      |             | DH Extended |
 * |    0x00     |  Machine    |    State    | Status      |
 * ---------------------------------------------------------
 *
 * where,
 *   State Machine identifies the MD or BEM state machine which generated the
 *   Ulog message.
 *
 *   State identifies the state which generated the Ulog message.
 *
 *   DH Extended Status is a status byte which is passed up from DH when
 *   available.
 *
 * The following macros generate the extended status word within the BEM and MD
 * state machines.
 */

#define MD_EXTENDED_STATUS(m_md_state_machine,m_md_state,m_dh_extended_status) \
(((m_md_state_machine) << 16) | ((m_md_state) << 8) | (m_dh_extended_status))

#define BEM_EXTENDED_STATUS(m_bem_state_machine,m_bem_state)     \
(((m_bem_state_machine) << 16) | ((m_bem_state) << 8))

/*
 * The following macro generate extended status used currently 
 * in rebuild, CM, and verify for sending extendedStatus
 * to unsolicited log with information about group/percentage, lun, fru.
 * We format the message so that we always display 7 to 8 digits.
 * For that, we replace the 'first' argument by '0xFF' in the case
 * when 'first' is 0. This is to account for the fact that leading zeroes are
 * not displayed. The legal values of 'first' in our cases are 0 - 240 - when
 * 'first' is used in CM module to display group number, and 0x0 - 0x99 
 * -when 'first' is used to display percentage
 * DIMS: 84686,84307
 */

#define HI_EXTENDED_STATUS(first, second ) \
    ((first) == 0) ? \
    ((0xFFFF << 16) | ((second) & 0xFFFF )): \
    ((((first) & 0xFFFF) << 16) | (((second) & 0xFFFF )))

/* 
 * The following macro is devised to represent a decimal number 'd' by a hex number
 * 'h' so, that when hex number is displayed in hex format, it looks like original
 * number 'd' displayed in decimal format.
 * The algoritm uses ingeger division: h = (d/10)*6 + d; both d and h have to be 
 * declared as integers. The range of numbers to use: d = 0 - 99; 
 * This is used to display percentage by numbers in decimal range 0 - 99
 * DIMS: 84686
 */

#define HI_PRCNT_TO_HEX(prcnt) \
    ((prcnt) >= 100) ? \
    0 : ((prcnt) + ((prcnt) / 10 ) * 6)


 /* The following are literals for the State Machine field 
 * of the MD or BEM extended status word. 
 */
#define MD_STATE_MACHINE        0x00
#define BEM_STATE_MACHINE       0x80

#define SM_CORONA_READ          0x01
#define SM_CORONA_WRITE         0x02
#define SM_READ_AND_XOR         0x03
#define SM_PARITY_WRITE         0x04
#define SM_DATA_READ_XOR        0x05
#define SM_DEVICE_READ          0x06
#define SM_VERIFY               0x07
#define SM_MIRROR_READ          0x08
#define SM_SIBLING_READ         0x09
#define SM_MIRROR_VERIFY        0x0A
#define SM_REBUILD_MIRROR       0x0B
#define SM_EQUALIZE             0x0C

#define MD_SM_CORONA_READ       (MD_STATE_MACHINE | SM_CORONA_READ)
#define MD_SM_CORONA_WRITE      (MD_STATE_MACHINE | SM_CORONA_WRITE)
#define MD_SM_READ_AND_XOR      (MD_STATE_MACHINE | SM_READ_AND_XOR)
#define MD_SM_PARITY_WRITE      (MD_STATE_MACHINE | SM_PARITY_WRITE)
#define MD_SM_DATA_READ_XOR     (MD_STATE_MACHINE | SM_DATA_READ_XOR)
#define MD_SM_DEVICE_READ       (MD_STATE_MACHINE | SM_DEVICE_READ)
#define MD_SM_VERIFY            (MD_STATE_MACHINE | SM_VERIFY)
#define MD_SM_MIRROR_READ       (MD_STATE_MACHINE | SM_MIRROR_READ)
#define MD_SM_SIBLING_READ      (MD_STATE_MACHINE | SM_SIBLING_READ)
#define MD_SM_MIRROR_VERIFY     (MD_STATE_MACHINE | SM_MIRROR_VERIFY)
#define MD_SM_REBUILD_MIRROR    (MD_STATE_MACHINE | SM_REBUILD_MIRROR)
#define MD_SM_EQUALIZE          (MD_STATE_MACHINE | SM_EQUALIZE)

#define BEM_SM_VERIFY           (BEM_STATE_MACHINE | SM_VERIFY)

/*********************************
 * STRUCTURE DEFINITIONS
 *********************************/

/* Once upon a time, the Corona Request block was filled out by
 * a host and sent to a controller to tell it what to do.
 * It is now only used to pass requests around internally by
 * Flare, primarily from processes to the CM.
 */
typedef struct corona_request
{
    UINT_32 opcode;
    UINT_32 block_count;
    UINT_32 logical_disk_address;
    UINT_32 sub_request_number;
    union
    {
    UINT_32 argument;
        OPAQUE_PTR argument_ptr;
    };
    UINT_32 reserved;
}
CORONA_REQUEST;

/* The Generic Request is used within Flare for passing "generic"
 * arguments to the CM, typically for operations which change
 * our configuration.  I created this to avoid having to keep
 * stuffing variables into a CORONA_REQUEST which have nothing to
 * do with the field names used in that structure.  DGM 11/94
 */
typedef struct generic_request
{
    UINT_32 opcode;
    UINT_32 argument[6];
}
GENERIC_REQUEST;

typedef union request
{
    CORONA_REQUEST crb;
    GENERIC_REQUEST grb;
}
REQUEST;

/* The Request Block contains a layer of information above
 * what is provided in the REQUEST.  It is also used only
 * internally by Flare to interface various processes with
 * the CM.
 * You might be tempted to think that the whole REQUEST
 * interface would be much simpler to follow and easier
 * to deal with if it was overhauled at some point.
 * I think you would be correct.  DGM
 */

/* Request Block Flags
 * The following values define bits in the flag(s) field of a
 * Request Block.
 */

#define USER_ABORT_BIND       0x01
        /* Indicates that an unbind of a lun presently binding
         * is requested by the user.  This differentiates a
         * user unbind request from an unbind because of an
         * AEP abnormal event.
         */

#define AUTO_TRESPASS_REQUEST 0x02
        /* Indicates that this Trespass request is part of an
         * auto-trespass request.
         */

#define ABORT_INIT_BIND       0x04
        /* Indeicate the a bind was occuring when this SP went down and
         * now we must abort the bind on powerup.
         */

#define CM_OP_STATUS_SENT 0x08
    /* Indicates that the host has already recieved status for 
     * the associated CM operation (used in the case of orphans).
     */

#define POWER_SAVING_STATS_LOGGING 0x10
    /* This flag is set when the RB is handling a power saving
     * statistics logging request.
     */

    /* ARS 478079: Changed from 0x11 to 0x20 since these flags are bit field flags and 
     *             should represent only one bit.
     */
#define FLARE_ALLOW_ASSIGN  0x20
    /* Indicates unit assign is allowed for BPF situation.
     */

typedef struct request_block
{
    OPAQUE_PTR host_request_identifier;
    UINT_32 transfer_byte_count;
    UINT_8E flags1;
    UINT_8E flags;
    UINT_32 unit_number;
    UINT_32 group_number;
    UINT_32 fru_number;
    UINT_32 reserved_0;
    REQUEST request_entry;
    UINT_32 reserved_1;
    UINT_32 scatter_gather_entry_count;
    SG_ENTRY scatter_gather[SG_SECTION];
    BITS_32 next_scatter_gather;
}
REQUEST_BLOCK;

/* The Status Block is used to give status back to the
 * originator of a Request Block.
 */

/* The following are values which are to be used in the type
 * field of the status block.
 */
#define HI_NORMAL_SUNBURST             0x01
#define HI_UNSOLICITED_SUNBURST        0x02
#define HI_AEP_CONTINUE                0x05

/* Defines for the diagnostic flag in the STATUS_BLOCK */
#define DIAG_REG_NOT_VALID  0x00
#define DIAG_REG_VALID      0x01

typedef struct status_block
{
    OPAQUE_PTR host_request_identifier;
    /* This 32 bit field really contains 3 things:
     *   bits 31 - 16  Error Code
     *   bits 15 -  8  Status
     *   bits  7 -  0    type
     *
     * THIS IS ONLY DONE TO GET AROUND
     * deficiencies of the compiler.
     */
    UINT_32 error_code_status_type;

    UINT_32 total_transfer_count;
    UINT_PTR unit_status_data[4];
    UINT_32 diagnostic_flag;
    UINT_32 diagnostic_regs[1];
    INT_32 checksum;
}
STATUS_BLOCK;

/*********************************
 * EXTERNAL REFERENCE DEFINITIONS
 *********************************/

/************************
 * PROTOTYPE DEFINITIONS
 ************************/
extern char *sunburst_code_itoa(unsigned int sunburst_code);

#endif /* MUST be last line in file */

/*
 * End of $Id: sunburst.h,v 1.66.12.2 2000/10/13 14:23:38 fladm Exp $
 */
