#ifndef SUNBURST_ERRORS_H
#define SUNBURST_ERRORS_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * $Id: sunburst_errors.h,v 1.5 2000/02/18 21:38:55 fladm Exp $
 ***************************************************************************
 *
 * DESCRIPTION: Error and information codes formerly in sunburst.h
 *
 * NOTES:
 *        WARNING!!! This file must be kept in sync with Sustaining's FDE
 *        versions.  If you are adding a code or codes to this
 *        file in another branch or configuration management system, you
 *        are making a mistake which may have severe consequences for our
 *        product.  Please follow the procedure below to change this file:
 *
 *        1. Check this file out of ADE Main.
 *        2. Make addition(s).
 *        3. Check file in.
 *        4. Follow the propagation rules to get the file to other branches.
 *
 *        Note that you must check the file in before you know you have
 *        successfully updated the master, since the ADE allows multiple
 *        checkouts.
 *
 * HISTORY:
 *    30-May-00  CP  Added HOST_HOTSPARE_CONFIG_RESET to fix Remedy defects
 *                   6403, 6849 and 6900.
 *    16-Jan-01  LBV Added INVALID_SCSI_SIZE.
 *    29-Nov-01  CJH Marked HOST_CANNOT_MARK_UNIT_CLEAN no longer unused.
 *    20-Sep-06  JSA Added HOST_XPE_ENGINE_ID_FAULTED.
 *    04-Oct-06  JSA Added HOST_XPE_ENGINE_ID_FAULTED.
 *
 * $Log: sunburst_errors.h,v $
 * Revision 1.1.2.15  2000/06/06 18:58:34  fladm
 * User: cphyfe
 * Reason: SUS_6403
 * Fix Remedy defects 6403, 6849 and 6900.  If Flare crashes
 * at the right (wrong) time before a rebuild or after an
 * equalization, the hotspare information in the glut and
 * fru_tbl on the database drives can be out of sequence. The
 * fix is to verify and repair these inconsistencies during
 * Flare powerup (boot).
 *
 * Revision 1.6  2000/02/29 16:43:09  fladm
 * User: ellistb
 * Reason: EPR_Fixes
 * Reason: Ethernet_Service_Port
 * Fix for EPR 3739
 *
 * Revision 1.5  2000/02/18 21:38:55  fladm
 * User: ellistb
 * Reason: EPR_Fixes
 * Added HOST_FLNET_TELNET_NEGOTIATION_TIMEOUT
 * and HOST_FLNET_TELNET_NEGOTIATION_MAX codes (EPR 3687).
 *
 * Revision 1.4  2000/02/11 22:35:51  fladm
 * User: frazier
 * Reason: Ethernet_Service_Port
 * Update for EPR-3562 to allow use of SC APIs via the Flare network
 * interface. Basically, this update hardcodes the initialization of
 * each network interface request to the PHYSICAL_VA.
 *
 * Revision 1.3  2000/02/11 20:28:07  fladm
 * User: pmisra
 * Reason: Trunk_EPR_Fixes
 * Added additional unsolicited log information
 * HOST_DISK_OBITUARY_WRITE_FAILED (EPR_3534).
 *
 * Revision 1.2  2000/02/11 18:47:54  fladm
 * User: ellistb
 * Reason: main_EPR_Fixes
 * Fix for EPR 3623: Values for network-related sunburst
 * codes that are informational only were moved into
 * the 0x70YY range according to Clariion standards.
 *
 * Revision 1.1  2000/02/09 18:05:34  fladm
 * User: hopkins
 * Reason: Trunk_EPR_Fixes
 * EPR 3632 - New file containing the error and information
 * codes formerly in sunburst.h
 *
 **********************************************************************/

/*
 * LOCAL TEXT unique_file_id[] = "$Header: /fdescm/project/CVS/flare/src/include/sunburst_errors.h,v 1.5 2000/02/18 21:38:55 fladm Exp $"
 */

/***********************************************************************
 * IMPORTANT  !!  IMPORTANT  !!  IMPORTANT  !!  IMPORTANT  !!  IMPORTANT
 *
 * This file contains global data that is useful to other projects
 * outside of Flare.  The values defined here represent an interface
 * to the outside world and are shared by the other groups.  As a result
 * a few rules apply when modifying this file.
 *
 *  1 - Do not delete (ever) a value defined in this file
 *
 *  2 - Only #define's of new values are allowed in this file
 *
 *  3 - Only add values of like data (error code, id value, etc.)
 *      to this file.
 *
 *  4 - Try to understand how values and ranges are being allocated
 *      before adding new entries to ensure they are added in the
 *      correct area.
 *
 *  5 - If you have any doubts consult project or group leaders
 *      before making a permanent addition.
 *
 *  6 - See the WARNING at the top of the file for further instructions.
 *
 ************************************************************************/

/**************************************************************
 * The following list of definitions are error codes which are
 * returned through the error code field of the Status Block.
 *
 * Several Notes:
 * 1) Event codes of the 0x6000, 0x7000, 0x8000, 0x9000, and 0xA000
 *    classes are for entries in the Unsolicited Log.  Each
 *    class has a specific meaning:
 *      0x6000 = Informational
 *      0x7000 = Informational
 *      0x8000 = Soft Error
 *      0x9000 = Hard Error
 *      0xA000 = Fatal Error
 *      0xB000 TO 0xD0FF = Informational
 *
 * 2) Event codes of other classes are for internal Flare use
 *    only (although that information may be tucked into the
 *    Information Bytes of Request Sense Data for some events).
 *
 * 3) For historical reasons, the middle-high nibble of event
 *    codes is discarded by Flare when reporting events to a
 *    host or via Gridman.  Thus, an error code of 0xABCD will
 *    be reported as 0xACD.  So, always set the middle high
 *    nibble to ZERO to avoid confusion.
 *
 **************************************************************/


/*************************************************************
 * IMPORTANT * IMPORTANT * IMPORTANT * IMPORTANT * IMPORTANT *
 *************************************************************
 * REMEMBER to update catmerge\interface\k10kernelmsg.mc     *
 * file when you add a new sunburst errors greater than      *
 * 0x6000 because NT event log will retrieve the description *
 * of a specific event log message from k10kernelmsg.mc file *
 * otherwise the NT event log will not display the descrip-  *
 * ion properly.                                             *
 * Also, ulog_utils.c must be updated to get the correct     *
 * text strings in the Flare ULOG (which is still used by    *
 * the NAS version of Chameleon).                            *
 * Thirdly, remember to update fdb_getloc.c so that fdb will *
 * report the message correctly.                             *
 *************************************************************/

#define HOST_ERROR_CLASS_MASK                        0xF000

/* Good Status ... */

#define HOST_REQUEST_COMPLETE                        0x0000

/* WARNING! See warning at top of file before making any changes or additions */

/* Request Interpretation Errors */

#define HOST_INTERPRETATION_MASK                     0x1000

#define HOST_REQUEST_BLOCK_CHECKSUM_ERROR            0x1000
#define HOST_SCATTER_GATHER_CHECKSUM_ERROR           0x1001 /* unused */
#define HOST_UNDEFINED_FLAG                          0x1002 /* unused */
#define HOST_ILLEGAL_UNIT_NUMBER                     0x1003
#define HOST_UNDEFINED_REQUEST_OPCODE                0x1004
#define HOST_SCATTER_GATHER_LIST_LENGTH_ERROR        0x1005
#define HOST_SCATTER_GATHER_LIST_ERROR               0x1006
#define HOST_MAX_DISK_ADDRESS_EXCEEDED               0x1007 /* unused */
#define HOST_INCONSISTENT_TRANSFER_COUNTS            0x1008 /* unused */
#define HOST_ILLEGAL_SUB_OPERATION_NUMBER            0x1009
#define HOST_ILLEGAL_SUB_OPERATION_ARGUMENT          0x100A
#define HOST_ILLEGAL_FLAG                            0x100B
#define HOST_ILLEGAL_TRANSFER_COUNT                  0x100C
#define HOST_ILLEGAL_TRANSFER_ADDRESS                0x100D /* unused */
#define HOST_SCSI_MAX_TRANSFER_SIZE_EXCEEDED         0x100E /* unused */
#define HOST_UNDEFINED_REPORT_TYPE                   0x100F /* unused */
#define HOST_BIND_TABLE_CHECKSUM_ERROR               0x1010
#define HOST_NO_IO_OPERATION                         0x1011 /* unused */
#define HOST_BAD_ELEMENT_SIZE                        0x1012 /* unused */
#define HOST_CONTROL_BYTE_NOT_ZERO                   0x1013
#define HOST_PARAM_LENGTH_TOO_LONG                   0x1014
#define HOST_UNIT_TYPE_DOES_NOT_SUPPORT_CACHING      0x1015
#define HOST_CURRENTLY_THIS_SYSTEM_TYPE              0x1016
#define HOST_ILLEGAL_UNIT_TYPE                       0x1017
#define HOST_SYS_CONFIG_CHANGE_IN_PROGRESS           0x1018
#define HOST_INSUFFICIENT_CONTROL_STRUCTURE_MEMORY   0x1019
#define HOST_ILLEGAL_CRU_NUMBER                      0x101A
#define HOST_INVALID_ADRESSING_MODEL                 0x101B
#define HOST_INVALID_SYSTEM_TYPE                     0x101C
#define HOST_UNITS_STILL_BINDING                     0x101D
#define HOST_UNITS_STILL_UNBINDING                   0x101E
#define HOST_UNAUTHORIZED_ACCESS_ATTEMPTED           0x101F

#define HOST_INVALID_TLD                             0x1020
#define HOST_UNAUTHORIZED_ACCESS                     0x1021 /* from k1ph1 */
#define HOST_INVALID_WWN                             0x1022
#define HOST_INV_PORT_ID                             0x1023 /* from k1ph1 */
#define HOST_NAME_TOO_LONG                           0x1024
#define HOST_INV_OPTIONS                             0x1025
#define HOST_INV_TYPE                                0x1026
#define HOST_INVALID_PASSWORD                        0x1027 /* from k1ph1 */
#define HOST_INCOMPLETE_INFO                         0x1028
#define HOST_NON_EXISTENT_FLU                        0x1029 /* from k1ph1 */
#define HOST_FLU_WWN_MISMATCH                        0x102A
#define HOST_INV_FEAT_STATUS                         0x102B
#define HOST_INV_FEAT_QUANTITY                       0x102C
#define HOST_INV_FEAT_CRC                            0x102D
#define HOST_FEAT_GUI_TOO_LONG                       0x102E
#define HOST_INVALID_OLD_PASSWORD                    0x102F
#define HOST_PASSWORD_TOO_LONG                       0x1030
#define HOST_INVALID_VLU_NUM                         0x1031 /* from k1ph1 */
#define HOST_EXTRANEOUS_INFORMATION                  0x1032
#define HOST_INVALID_VLU_REMOVE                      0x1033
#define HOST_OPERATION_DATA_OVERRUN                  0x1034
#define HOST_INV_FEAT_ID                             0x1035
#define HOST_INV_VA_PARAMETER                        0x1036
#define HOST_VLUT_UPDATE_FAILED                      0x1037
#define HOST_DBID_TLD_MISMATCH                       0x1038
#define HOST_TLD_NESTING_ERROR                       0x1039
#define HOST_INV_TAG_USAGE                           0x103A
#define HOST_UNKNOWN_TAG                             0x103B
#define HOST_TLD_LENGTH_ERROR                        0x103C
/* Returned on attempt to bind R3 or HotSpare w/ DSA */
#define HOST_ILLEGAL_DSA_UNIT_TYPE                   0x103D
/* Returned on attempt to enable DSA on R3, HotSpare */
#define HOST_INV_DSA_OPTIONS                         0x103E

/* Front end transfer error */
#define HOST_FE_XFER_ERROR                           0x103F
#define HOST_DISK_OBIT_RETRIEVAL_ERROR               0x1040
#define HOST_ILLEGAL_DSA_R3_MEM_SET                  0X1041 /* from K1SC,DSA */

/*
 * Generic error for internal problems with Admin Library-Flare interface
 * This error should NEVER be returned to Navi or a Host.
 */
#define HOST_ADMIN_LIB_FLARE_INTERNAL_ERROR          0x1042


/* WARNING! See warning at top of file before making any changes or additions */

/* Operational Errors */

#define HOST_OPERATIONAL_MASK                        0x2000

#define HOST_MICROCODE_CHECKSUM_ERROR                0x2001 /* unused */
#define HOST_HARD_DISK_ERROR                         0x2002 /* unused */
#define HOST_OPEN_FAILURE                            0x2003 /* unused */
#define HOST_CLOSE_FAILURE                           0x2004 /* unused */
#define HOST_UNOPENED_DEVICE                         0x2005 /* unused */
#define HOST_UNKNOWN_REQUEST_IDENTIFIER              0x2006 /* unused */
#define HOST_INSUFFICIENT_TRANSFER_SPACE             0x2007
#define HOST_UNIT_ALREADY_IN_SHUTDOWN                0x2008


#define HOST_UNIT_ALREADY_ASSIGNED                   0x200B
#define HOST_UNIT_NOT_ASSIGNED                       0x200C

#define HOST_UNIT_NOT_BOUND                          0x200E
#define HOST_UNIT_NOT_CLOSED                         0x200F
#define HOST_UNIT_BOUND_PERMANENTLY                  0x2010

#define HOST_BROKEN_UNIT                             0x2015
#define HOST_BAD_FRU_SIGNATURE                       0x2016
#define HOST_UNIT_ALREADY_ASSIGNING                  0x2017
#define HOST_DUAL_BOARD_ASSIGN_ERROR                 0x2018
#define HOST_ASSIGN_DATABASE_FAILURE                 0x2019
#define HOST_REQUESTS_OUTSTANDING                    0x201A
#define HOST_HOT_SPARE_IN_USE                        0x201B

#define HOST_UNIT_VERIFY_ERROR                       0x201D /* unused */
#define HOST_UNINITIALIZED_NON_VOL_ERROR             0x201E
#define HOST_NON_VOL_VERIFY_FAILED                   0x201F

/* These are cache specific error codes */
#define HOST_UNIT_CACHE_DIRTY_NO_DATA                0x2020
#define HOST_UNIT_CACHE_DB_UPDATE_FAILED             0x2021
#define HOST_DUAL_BOARD_CACHE_ERROR                  0x2022
#define HOST_CACHE_ENABLE_DISABLE_FAILED             0x2023
#define HOST_ILLEGAL_CACHE_PAGE_SIZE                 0x2024
#define HOST_ILLEGAL_WATERMARK_VALUE                 0x2025
#define HOST_UNABLE_TO_RECONFIG_CACHE                0x2026
#define HOST_SYSTEM_CACHE_DB_UPDATE_FAILED           0x2027
#define HOST_ILLEGAL_USABLE_CACHE_SIZE_VALUE         0x2028
#define HOST_ILLEGAL_BBU_TIMEOUT_VALUE               0x2029

#define HOST_ILLEGAL_PRIORITY_LRU_LENGTH             0x2030 /* unused */
#define HOST_ILLEGAL_PREFETCH_SIZE                   0x2031
#define HOST_ILLEGAL_READ_RETENTION_PRIORITY         0x2032
#define HOST_BAD_PROM_IMAGE                          0x2033
#define HOST_DOWNLOAD_TIMEOUT                        0x2034
#define HOST_DISKS_NOT_IDLE                          0x2035
#define HOST_UCODE_UPDATE_IN_PROGRESS                0x2036

#define HOST_MICROCODE_FRUS_FORMATTING               0x2037 /* unused */
#define HOST_FRU_NOT_READY_FOR_MICROCODE             0x2038 /* unused */
#define HOST_INCORRECT_MICROCODE_TRANSFER_SIZE       0x2039 /* unused */
#define HOST_WOULD_CAUSE_UNIT_FAILURE                0x203A /* unused */
#define HOST_INVALID_INSTALL_SEQUENCE_NUMBER         0x203B /* unused */
#define HOST_CANNOT_MARK_UNIT_CLEAN                  0x2040
#define HOST_DUAL_BOARD_PW_ERROR                     0x2041
#define HOST_DUAL_BOARD_PW_REQ_BUSY                  0x2042
#define HOST_SYSTEM_DB_UPDATE_FAILED                 0x2043
#define HOST_DUAL_BOARD_SOFT_SHUTDOWN_FAILED         0x2044
#define HOST_DUAL_BOARD_SUP_BUF_ERROR                0x2045
#define HOST_CHANGE_SYSTEM_CONFIG_FAILED             0x2046

#define HOST_BAD_PROM_REV                            0x2047
#define HOST_INVALID_MEM_CONFIGURATION               0x2048
#define HOST_CACHE_CONFIG_ERROR_AUTOMODE             0x2049
#define HOST_ILLEGAL_VAULT_FAULT_OVERRIDE_VALUE      0x204A
#define HOST_ILLEGAL_MAX_PREFETCH_SIZE_VALUE         0x2050
#define HOST_ILLEGAL_DISABLE_PREFETCH_SIZE_VALUE     0x2051
#define HOST_ILLEGAL_WRITE_ASIDE_SIZE                0x2052
#define HOST_ILLEGAL_CACHE_IDLE_DELAY_VALUE          0x2053
#define HOST_ILLEGAL_PREFETCH_SEGMENT_SIZE_VALUE     0x2054
#define HOST_ILLEGAL_PREFETCH_IDLE_COUNT_VALUE       0x2055
#define HOST_ILLEGAL_PREFETCH_TYPE_FLAG_VALUE        0x2056
#define HOST_ILLEGAL_CACHE_IDLE_THRESHOLD_VALUE      0x2057
#define HOST_ILLEGAL_DEFAULT_OWNERSHIP               0x2058
#define HOST_ILLEGAL_REBUILD_PRIORITY                0x2059
#define HOST_UNABLE_TO_CLEAR_PCMCIA_MEMORY           0x205A /* unused */
/*#define HOST_DUAL_BOARD_DRIVE_DOWNLOAD_ERROR         0x205B repalced with HOST_DRIVE_DOWNLOAD_ERROR */
#define HOST_UNIT_L2_CACHE_DIRTY_NO_RESOURCES        0x205C
#define HOST_TRESPASS_RESERVATION_ERROR              0x205D
#define HOST_SUP_BUF_SS_ERROR                        0x205E
#define HOST_SUP_BUF_2SSOF_ERROR                     0x205F
#define HOST_SUP_BUF_2SS_ERROR                       0x2060
#define HOST_TSS_DUAL_BOARD_SOFT_SHUTDOWN_FAILED     0x2061
#define HOST_TSSOF_DUAL_BOARD_SOFT_SHUTDOWN_FAILED   0x2062
#define HOST_INVALID_WRITE_CACHE_SIZE                0x2063
#define HOST_INVALID_R3_MEM_SIZE                     0x2064
#define HOST_CANT_CHANGE_WCACHE_AND_R3_MEM_TOGETHER  0x2065
#define HOST_R3_WRITE_BUFFERING_ENABLED              0x2066
#define HOST_WRITE_CACHE_ALLOCATED                   0x2067
#define HOST_BANDWIDTH_MODE_R3_OPTIMIZED             0x2068
#define HOST_OUT_OF_CACHE_MEMORY                     0x2069
#define HOST_INVALID_CONFIG_WORD                     0x206A
#define HOST_CANT_DECONFIGURE_PRESENT_CONTROLLER     0x206B
#define HOST_FRU_FORMATTED_OR_NOT_LEGAL              0x206C
#define HOST_LUNS_CACHE_DIRTY                        0x206D
#define HOST_PEER_LUNS_CACHE_DIRTY                   0x206E
#define HOST_CACHE_NOT_DISABLED                      0x206F
#define HOST_CACHE_NOT_CLEAN                         0x2070
#define HOST_PEER_CACHE_NOT_CLEAN                    0x2071
#define HOST_UNASSIGNED_CACHE_PAGES                  0x2072
#define HOST_READ_CACHE_NOT_DISABLED                 0x2073
#define HOST_COMMAND_UNSUPPORTED                     0x2074
#define HOST_COMMAND_UNSUPPORTED_FOR_THIS_SYSTEM_TYPE 0x2075
#define HOST_ILLEGAL_VERIFY_PRIORITY                 0x2076
#define HOST_TOO_MANY_WRITE_CACHE_PAGES              0x2077
#define HOST_SUP_BUF_DB_LOCK_ERROR                   0x2078
#define HOST_NO_WRITE_CACHE_ALLOCATED                0x2079
#define HOST_SC_INITIATOR_LIST_FULL                  0x207A
#define HOST_SC_INITIATOR_ALREADY_EXISTS             0x207B
#define HOST_SC_INVALID_PORT                         0x207C
#define HOST_SC_INITIATOR_NOT_FOUND                  0x207D
#define HOST_SC_VA_ALREADY_EXISTS                    0x207E /* from k1ph1 */
#define HOST_SC_VA_LIST_FULL                         0x207F
#define HOST_SC_VLU_FLU_MAP_FULL                     0x2080
#define HOST_SC_FLU_NOT_FOUND                        0x2081
#define HOST_SC_VLU_ALREADY_MAPPED                   0x2082
#define HOST_SC_VA_NOT_FOUND                         0x2083
#define HOST_SC_VA_STILL_MAPPED                      0x2084
#define HOST_SC_INVALID_FLU_NUMBER                   0x2085
#define HOST_SC_VLU_ALREADY_EXISTS                   0x2086
#define HOST_SC_INVALID_VLU_NUMBER                   0x2087
#define HOST_SC_FLU_ALREADY_MAPPED                   0x2088
/* #define HOST_SC_FEATURE_NOT_FOUND                 0x2088 - k1phase1 */
#define HOST_SC_VLUT_LOCKED                          0x2089
#define HOST_SC_CANT_REMOVE_DEFAULT_VA               0x208A
/* #define HOST_SC_INVALID_VLU_COUNT                 0x208A - k1phase1 */
#define HOST_SC_DB_UPDATE_FAILED                     0x208B
#define HOST_ILLEGAL_FLU_NUMBER                      0x208C
#define HOST_FLU_NUMBER_IS_ALREADY_BOUND             0x208D
#define HOST_ILLEGAL_VIRTUAL_ARRAY                   0x208E
#define HOST_INVALID_CMND_FOR_NULL_VA                0x208F /* from k1ph1 */
#define HOST_SUP_BUF_WR_ERROR                        0x2090 /* Warm Reboot */
#define HOST_FKEY_FLARE_DATABASE_FULL                0x2091
#define HOST_FKEY_NON_FLARE_DATABASE_FULL            0x2092
#define HOST_FKEY_CRC_ERROR                          0x2093
#define HOST_FKEY_UPDATE_IN_PROGRESS                 0x2094
#define HOST_FKEY_UPDATE_FAILED                      0x2095
#define HOST_FKEY_INVALID_FKEY_DATA                  0x2096
#define HOST_FLU_NAME_UPDATE_FAILED                  0x2097
#define HOST_CANT_MODIFY_SPECIAL_VA                  0x2098
#define HOST_VLU_CANT_MAP_TO_HOT_SPARE               0x2099
#define HOST_ONLY_ONE_MAPPING_TO_BINDING_LUN         0x209A

/* unsoloicited errore code when a vlu flu map was created when
   bind was ongoing, but the bind failed */
#define HOST_SC_VLU_FLU_MAPPING_REMOVED              0x209B
#define HOST_SC_INITIATOR_ILLEGAL_OPERATION          0x209C

#define HOST_INVALID_L2_CACHE_SIZE                   0x209D
#define HOST_DUAL_BOARD_READ_CACHE_ERROR             0x209E

/* Error code for when the user attempts to enable/disable
 * verify sniffing.
 */
#define HOST_ILLEGAL_SNIFF_VERIFY_STATE_CHANGE       0x209F
#define HOST_SNIFF_VERIFY_RATE_NOT_SUPPORTED         0x20A0

/* Error code when the user attempts to enable/disable the WCA
 * state and it fails.
 */
#define HOST_WCA_ENABLE_DISABLE_FAILED               0x20A1

/* Error code for when to check write cache size greater than
 * 3 GB when WCA disable
 */
#define HOST_ILLEGAL_WRITE_CACHE_SIZE                0x20A2

/*
 * Flare Network-related operational error codes
 *
 * Note: There are several "holes" here that resulted
 * from moving some codes into the 0x70YY range to
 * address EPR 3738.
 *
 * Note: Flare Network-related informational codes
 *       are in the 0x70YY range according to Flare standards.
 */

#define HOST_FLNET_CONFIG_FAILED                    0x20B2
#define HOST_FLNET_CONFIG_SUCCESS_DB_FAILED         0x20B3
#define HOST_FLNET_DB_LOCKED                        0x20B4
#define HOST_FLNET_TH_FULL                          0x20BD
#define HOST_FLNET_TH_EMPTY                         0x20BE
#define HOST_FLNET_TH_ALREADY_EXIST                 0x20BF
#define HOST_FLNET_TH_NOT_EXIST                     0x20C0
#define HOST_FLNET_NETWORK_NOT_INITTED              0x20C1

/* Flash LED request errors */
#define HOST_FLASH_ENCLOSURE_MISSING                0x20C2
#define HOST_FLASH_HARDWARE_ERROR                   0x20C3

/* NVCA Driver specific error codes */
#define HOST_ILLEGAL_PCI_CACHE_MEM_SIZE             0x20C4
#define HOST_UNBIND_RETRY_ERROR                     0x20C5

/* WARNING! See warning at top of file before making any changes or additions */

/* Aborts  */
#define HOST_ABORT_MASK                              0x3000

#define HOST_REQUESTED_SCSI_ABORT                    0x3000 /* unused */
#define HOST_REQUESTED_SHUTDOWN_ABORT                0x3001 /* unused */
#define HOST_ABORT_FAILED                            0x3002 /* unused */
#define HOST_ABORT_UNIT_MISMATCH                     0x3003 /* unused */


/* WARNING! See warning at top of file before making any changes or additions */

/* SCSI Errors */
#define HOST_SCSI_ERROR_MASK                         0x4000
#define HOST_CONTROLLER_DETECTED_SCSI_PROTOCOL_ERROR 0x4000


/* WARNING! See warning at top of file before making any changes or additions */

/* Bind Table Errors */
#define HOST_BIND_ERROR_MASK                         0x503F
#define HOST_BIND_BAD_BIND_TYPE                      0x5000
#define HOST_BIND_BAD_FRU_COUNT                      0x5001
#define HOST_BIND_BAD_FRU_NUM                        0x5002
#define HOST_BIND_FRU_ALREADY_BOUND                  0x5003
#define HOST_BIND_BAD_FRU_CONFIGURATION              0x5004
#define HOST_BIND_BAD_ELEMENT_SIZE                   0x5005
#define HOST_BIND_BAD_ADDRESS_OFFSET                 0x5006
#define HOST_BIND_FRUS_NOT_READY                     0x5007
#define HOST_DUAL_BOARD_BIND_ERROR                   0x5008
#define HOST_DUAL_BOARD_UNBIND_ERROR                 0x5009
#define HOST_UNIT_ALREADY_UNBINDING                  0x500A
#define HOST_UNIT_STILL_BINDING                      0x500B
#define HOST_BIND_FRU_TYPE_MISMATCH                  0x500C

#define HOST_UNIT_TOO_LARGE                          0x500F
#define HOST_BIND_ONE_FRU_FORMATTING                 0x5010
#define HOST_BIND_ONE_FRU_POWERING_UP                0x5011
#define HOST_BIND_DB_UPDATE_FAILED                   0x5012
#define HOST_BIND_FRU_SIGNATURE_FAILED               0x5013
#define HOST_UNIT_NUMBER_IS_ALREADY_BOUND            0x5014
#define HOST_BAD_R3_MEM_REQUEST                      0x5015
#define HOST_BIND_TOO_MANY_PARTITIONS                0x5016
#define HOST_BIND_WRITE_CACHE_NOT_CONFIGURED         0x5017
#define HOST_BIND_BAD_BSF_FLAG                       0x5018
#define HOST_BIND_ILLEGAL_L2_CACHE_SIZE              0x5019
#define HOST_NO_VLU_BIND_AND_MGMTVA                  0x501A
#define HOST_UNIT_CONFIG_ERROR_AUTOMODE              0x501B
#define HOST_BIND_INVALID_ALIGN_LBA                  0x501C
#define HOST_UNBIND_DB_REBUILDING                    0x501D

/* RAID Group Config Error */
#define HOST_RAID_GROUP_NUMBER_INVALID               0x5020
#define HOST_CREATE_RG_BAD_FRU_CONFIGURATION         0x5021
#define HOST_DUAL_BOARD_CREATE_RG_ERROR              0x5022
#define HOST_REMOVE_RG_ERROR_VALID_LUNS              0x5023
#define HOST_DUAL_BOARD_REMOVE_RG_ERROR              0x5024
#define HOST_RG_CONFIG_BAD_OPCODE                    0x5025
#define HOST_RG_CONFIG_BAD_OPTION                    0x5026

#define HOST_BIND_NM1_NOT_AVAILABLE                  0x5027
#define HOST_BIND_SELECTED_FRU_CANT_BE_HS            0x5028
#define HOST_BIND_BAD_STRIPES_NUMBER                 0x5029
#define HOST_BIND_ILLEGAL_CACHING_REQUEST            0x502A
#define HOST_BIND_ILLEGAL_HATS_PER_UNIT_REQUEST      0x502B /* unused */
#define HOST_BIND_ILLEGAL_CACHE_REQUEST_FOR_R3_UNIT  0x502C

#define  HOST_UNBIND_WAIT_FOR_STOP_ZERO              0x502D /* No longer used as of 12/19/08 */
#define  HOST_UNBIND_WAIT_INTERNAL_ASSIGN_RELEASE    0x502E
#define  HOST_UNBIND_WAIT_FOR_STOP_BGS               0x502F
/* RAID Group EXPANSION Errors */
#define HOST_EXPAND_BAD_FRU_NUM                      0x5030
#define HOST_EXPAND_BAD_FRU_CONFIGURATION            0x5031
#define HOST_EXPAND_TABLE_CHECKSUM_ERROR             0x5032
#define HOST_EXPAND_BAD_GROUP_NUM                    0x5033
#define HOST_EXPAND_BAD_LUN_TYPE                     0x5034
#define HOST_DUAL_BOARD_EXPAND_ERROR                 0x5035
#define HOST_EXPAND_FRUS_NOT_READY                   0x5036
#define HOST_EXPAND_DB_UPDATE_FAILED                 0x5037
#define HOST_EXPAND_ERROR_INCOMPATIBLE_TYPE          0x5038
#define HOST_EXPAND_ERROR_FRU_TOO_SMALL              0x5039
#define HOST_EXPAND_XL_FLAG_INVALID                  0x503A
#define HOST_EXPAND_ZERO_FAILED                      0x503B
#define HOST_EXPAND_STOP_VP_ERROR                    0x503C
#define HOST_EXPAND_FRU_TYPE_MISMATCH                0x503D

/* Zero on demand errors */
#define HOST_EXPAND_ZERO_ON_DEMAND_ERROR             0x503E
#define HOST_UNIT_ZERO_ON_DEMAND_FAILED              0x503F

/* Zero_disk errors */
#define HOST_ZERO_DISK_FRU_NOT_AVAILABLE             0x5040
#define HOST_ZERO_DISK_DB_UPDATE_FAILED              0x5041
#define HOST_ZERO_DISK_INVALID_PARAMETER             0x5042

/* Database Write errors. */
#define HOST_BIND_DB_REWITE_FAILED                   0x5043
#define HOST_UNIT_FLU_VLU_MAPPING_EXIST_ERROR        0x5044
#define HOST_DB_NOT_COMMITTED                        0x5045
/*#define HOST_MOD_CAP_DB_WRITE_FAILURE                0x5046*/

/* Portable LUN's errors */
#define HOST_CLEAR_EXPORT_ERROR                      0x5050
#define HOST_DUAL_BOARD_CLEAR_EXPORT_ERROR           0x5051
#define HOST_EXPORT_ERROR                            0x5052
#define HOST_DUAL_BOARD_EXPORT_ERROR                 0x5053
#define HOST_IMPORT_ERROR                            0x5054
#define HOST_DUAL_BOARD_IMPORT_ERROR                 0x5055

/* ODBS Error */
#define HOST_ODBS_ERROR                              0x5060

/* FRU Type mismatch when trying to create RAID group */
#define HOST_CREATE_RG_FRU_TYPE_MISMATCH             0x5070

/* Loop Speed Reconfigure Error Code */
#define HOST_LOOP_SPEED_RESET_PEER_DEAD_ERROR        0x5071
#define HOST_LOOP_SPEED_RESET_CACHE_DISABLE_ERROR    0x5072

/* Configure IO Ports Command Errors */
#define HOST_PORT_DATA_CMD_PEER_SP_REMOVED          0x5073
#define HOST_PORT_DATA_CMD_ILLEGAL_PORT_INFO        0x5074
#define HOST_PORT_DATA_CMD_INCORRECT_SLOT_PORT_NUMBER   0x5075

/* RAID Group Creation Error  */
#define HOST_CREATE_HS_RG_ON_SYSTEM_DRIVE            0x5080

/* RG standby exit error */
#define HOST_REMOVE_RG_SPIN_UP_FAILED                0x5081

/* bind error during online firmware download */
#define HOST_BIND_ONLINE_FIRMWARE_DOWNLOAD           0x5082

#define HOST_REMOVE_RG_WAIT_FOR_EXIT_STANDBY         0x5083

/* Operation(expand/defragmentation) not supported on given RAID type*/
#define HOST_EXPAND_BAD_OPERATION                    0x5084

/* NDU checks for CM and BGS conditions that will prevent an NDU from running between two versions which are 
 * not both Flare-based
 */
#define HOST_NDU_CHECKS_FOUND_RG_DEFRAG                 0x5085     
#define HOST_NDU_CHECKS_FOUND_BACKGROUND_VERIFY         0x5086
#define HOST_NDU_CHECKS_FOUND_DB_REBUILD                0x5087
#define HOST_NDU_CHECKS_FOUND_SPARING                   0x5088
#define HOST_NDU_CHECKS_FOUND_DISK_ZERO                 0x5089
#define HOST_NDU_CHECKS_FOUND_NEEDS_REBUILD             0x508A
#define HOST_NDU_CHECKS_FOUND_PROBATION                 0x508B
#define HOST_NDU_CHECKS_FOUND_POWER_SAVING              0x508C


/* RAID Group Busy Error */
#define HOST_RAID_GROUP_BUSY_ERROR                   0x50FF

 /*LUN Shrink Error */
/*#define HOST_MOD_CAP_ILLEGAL_SIZE                    0X5100*/

 /* Internal Modify Capacity */
#define HOST_UNIT_LUN_SHRINK_ZERO_MAP_MOVE_FAILED    0x5200



   /******************************************/
   /*      Unsolicited Information codes     */
   /******************************************/


/* WARNING! See warning at top of file before making any changes or additions */

/*************************************************************
 * IMPORTANT * IMPORTANT * IMPORTANT * IMPORTANT * IMPORTANT *
 *************************************************************
 * REMEMBER to update catmerge\interface\k10kernelmsg.mc     *
 * file when you add a new sunburst errors greater than      *
 * 0x6000 because NT event log will retrieve the description *
 * of a specific event log message from k10kernelmsg.mc file *
 * otherwise the NT event log will not display the descrip-  *
 * ion properly.                                             *
 * Also, ulog_utils.c must be updated to get the correct     *
 * text strings in the Flare ULOG (which is still used by    *
 * the NAS version of Chameleon).                            *
 *************************************************************/


#define HOST_INFORMATION_GROUP0_MASK                 0x6000

#define HOST_SP_POWERUP                              0x6001
#define HOST_FRU_ENABLED                             0x6002
#define HOST_FRU_REBUILD_STARTED                     0x6003
#define HOST_FRU_REBUILD_COMPLETED                   0x6004
#define HOST_FRU_REBUILD_HALTED                      0x6005
#define HOST_UNIT_SHUTDOWN_FOR_TRESPASS              0x6006
#define HOST_UNIT_SHUTDOWN_FOR_CHBIND                0x6007
#define HOST_FRU_READY                               0x6008
#define HOST_FORMAT_STARTED                          0x6009
#define HOST_UNIT_ENABLED                            0x600A
#define HOST_FORMAT_FAILED                           0x6010
#define HOST_SP_PROM_REV                             0x6011
#define HOST_NOT_ENOUGH_DB_DRIVES                    0x6012
#define HOST_FRU_EQZ_STARTED                         0x6013
#define HOST_FRU_EQZ_COMPLETED                       0x6014
#define HOST_FRU_EQZ_ABORTED                         0x6015
#define HOST_FLARE_LOADED                            0x6016 /* unused */
#define HOST_DRIVE_CODE_LOADED                       0x6017
#define HOST_CM_EXPECTED_SP_RESTART                  0x6018
#define HOST_UNCORRECTABLE_VERIFY_ERROR_PPC          0x6019 /* Renamed to 904F for K-10+ */
#define HOST_DEGRADED_NONVOL_VERIFY_DATA_LOSS_PPC    0x6020 /* Renamed to 9050 for K-10+ */
#define HOST_BCKGRND_CHKPT_VERIFY_STARTED            0x6021
#define HOST_BCKGRND_CHKPT_VERIFY_COMPLETE           0x6022
#define HOST_FAN_PACK_DISABLED                       0x6023 /* unused */
#define HOST_FAN_PACK_ENABLED                        0x6024 /* unused */
#define HOST_UNIT_INTERNAL_ASSIGN_RELEASE            0x6025
#define HOST_UNIT_INTERNAL_ASSIGN_RELEASE_FOR_UNBIND 0x6026
#define HOST_FRU_BAD_EXPANSION_CHKPT                 0x6027
#define HOST_FRU_EXPANSION_REVISION_MISMATCH         0x6028
#define HOST_FRU_EXPANSION_CHKPT_REBUILD_FAILED      0x6029
#define HOST_SCHEDULED_BBU_TEST                      0x602A
// The following SPS codes have been changed to Error messages
// Do not reuse these values
//#define HOST_INVALID_SPS_POWER_CABLE_1_CONFIG        0x602B
//#define HOST_INVALID_SPS_POWER_CABLE_2_CONFIG        0x602C
//#define HOST_INVALID_SPS_SERIAL_CABLE_CONFIG         0x602D
//#define HOST_INVALID_SPS_MULTIPLE_CABLES_CONFIG      0x602E
#define HOST_BBU_ENABLED_INVALID_CONFIG              0x602F

#define HOST_FAN_INSTALLED                           0x6030
#define HOST_VSC_INSTALLED                           0x6031
#define HOST_AC_INSTALLED                            0x6032 /* unused */
#define HOST_FAN_SPEED_INCREASED                     0x6033 /* unused */
#define HOST_FAN_SPEED_DECREASED                     0x6034 /* unused */
#define HOST_LOGICAL_SECTOR_DATA_ERROR               0x6035
/*      HOST_BBU_REMOVED                             0x6036 - moved to 900D */
#define HOST_BBU_RECHARGING                          0x6037
#define HOST_BBU_ENABLED                             0x6038
#define HOST_SYSTEM_CACHE_ENABLED                    0x6039
#define HOST_ENCL_1GIG_MODE                          0x603A
#define HOST_ENCL_2GIG_MODE                          0x603B
// #define HOST_SPS_CANNOT_OPEN_SERIAL_PORT             0x603C Changed to a Warning 8081
#define HOST_SPS_TYPE_NOT_RECOMMENDED                0x603D
#define HOST_LCC_OPEN_PORT_GLITCH                    0x603E
#define HOST_RESUME_PROM_READ_COMPLETED              0x603F

/*      HOST_MIRROR_RECONSTRUCTED                    0x6040 -
                                                     from 009-1854.04 */
/*      HOST_BBU_NOT_READY                           0X6040     in Hiraid */
#define HOST_FRU_REBUILD_ABORTED                     0x6041
#define HOST_BCKGRND_CHKPT_VERIFY_ABORTED            0x6042
#define HOST_PEER_CONTROLLER_INITING                 0x6043
#define HOST_PEER_CONTROLLER_INSERTED                0x6044
#define HOST_NAVI_FRU_BIND                           0x6045
#define HOST_NAVI_LUN_UNBIND                         0x6046
#define HOST_FUSE_BLOWN                              0x6047 /* unused */
#define HOST_TERMPOWER_LOW                           0x6048 /* unused */
#define HOST_BBU_NOT_READY                           0x6049

#define HOST_SPS_INVALID_RESPONSE                    0x604A
#define HOST_USER_WWN_SEED_UPDATE                    0x604B
#define HOST_USER_EMC_SERIAL_NUM_UPDATE              0x604C
#define HOST_USER_EMC_PART_NUM_UPDATE                0x604D
#define HOST_DB_WWN_SEED_UPDATE                      0x604E
#define HOST_CM_EXPECTED_SP_RESTART_DB               0x604F

#define HOST_FRU_SIGNATURE_ERROR_PPC                 0x6050 /* Renamed to 9051 for K-10+ */
#define HOST_DISK_OBITUARY_WRITE_FAILED              0x6051 /* Disk Obit */
#define HOST_CANT_ENABLE_512MB_VAULT                 0x6052
#define HOST_USING_512MB_VAULT                       0x6053
#define HOST_CACHE_DUMPING                           0x6054

#define HOST_BBU_ENABLED_UNKNOWN_CONFIG              0x6055
#define HOST_BBU_SHORT_TEST_ENABLED                  0x6056

#define HOST_CACHE_DUMP_COMPLETE                     0x6057
#define HOST_USER_CACHE_ENABLING                     0x6058 /* unused */
#define HOST_USER_CACHE_DISABLING                    0x6059
#define HOST_L2_CACHE_NOT_ENABLED                    0x605A
#define HOST_L2_CACHE_NOT_CLEAN                      0x605B
#define HOST_L2_CACHE_ENABLED                        0x605C
#define HOST_HA_CACHE_VAULT_DISABLED                 0x605D
#define HOST_HA_CACHE_VAULT_ENABLED                  0x605E
#define HOST_CACHE_HW_REDUNDANCY_BYPASS_ENABLED      0x605F
#define HOST_AC_POWER_FAILURE                        0x6060
#define HOST_BBU_SNIFFING_ENABLING                   0x6061
#define HOST_BBU_SNIFFING_DISABLING                  0x6062
#define HOST_BBU_SNIFF_INITIATING                    0x6063
#define HOST_BBU_CACHE_DISABLING                     0x6064
#define HOST_SYSTEM_CONFIG_SINGLE_SP                 0x6065
#define HOST_SYSTEM_CONFIG_DUAL_SP                   0x6066
#define HOST_CACHE_RECOVERING                        0x6067
#define HOST_CACHE_RECOVERED                         0x6068
#define HOST_BAD_TERMPOWER_FUSE                      0x6069 /* Classic
                                                             * Flare */
#define HOST_SOFT_VAULT_LOAD_FAILURE                 0x606A
#define HOST_SOFT_SBERR_DETECTED_PPC                 0x606B /* Renamed to 8061 for K-10+ */
#define HOST_FAN_PACK_DEGRADED_PPC                   0x606C /* Renamed to 9052 for K-10+ */
#define HOST_SP_SENDING_CRAZY_PEER                   0x606D
#define HOST_SP_USING_ALT_CMI_CHANNEL                0x606E /* unused */
#define HOST_CONFIG_TOO_MUCH_CACHE                   0x606F


/* WARNING! See warning at top of file before making any changes or additions */

/* What's this?  The PCMCIA and 'standard' trees went off on
 * their own ways for a while, and both used 0x6070, but for
 * different things.  Here we attempt to at least get everything
 * noted in this file, so you don't do the same thing....
 */
#ifdef NUC_ENV
#define PCMCIA_CARD_REMOVED                          0x6070 /* unused */
#define HOST_MEMORY_DUMP                             0x6074

#else
#define HOST_MEMORY_DUMP                             0x6070
#define PCMCIA_CARD_REMOVED                          0x6074 /* unused */

#endif

#define PCMCIA_CARD_INSERTED                         0x6071 /* Classic
                                                             * Flare */
#define PCMCIA_CARD_WRITE_ENABLED                    0x6072 /* Classic
                                                             * Flare */
#define PCMCIA_CARD_WIPED_BY_USER_REQUEST            0x6073 /* Classic
                                                             * Flare */

#define HOST_SOFT_WS_SBERR_DETECTED_PPC              0x6075 /* Renamed to 8062 for K-10+ */
#define HOST_SOFT_RS_SBERR_DETECTED_PPC              0x6076 /* Renamed to 8063 for K-10+ */

/* The following are error codes which call out the
 * fact that a drive is not suitable for use in
 * partitioned Raid groups
 */
#define HOST_DRIVE_DOES_NOT_SUPPORT_PARTITIONS       0x6077

/* Miscellaneous informational unsolicited error codes, continued
 */
#define HOST_DSKML_SP_LINK_UP_WILL_REBOOT            0x6078

/* The following two codes are issued when the databases are
 * reconstructed on powerup.
 */
#define HOST_FRU_DB_RECONSTRUCTED                    0x6079
#define HOST_RG_DB_RECONSTRUCTED                     0x607A

/*
 * Additional Rebuild error codes
 */
#define HOST_DB_REBUILD_STARTED                     0x607B
#define HOST_DB_REBUILD_COMPLETED                   0x607C
#define HOST_REBUILD_COMPLETED                      0x607D
#define HOST_EQUALIZE_COMPLETED                     0x607E

#define HOST_TELCO_PS_DETECTED                      0x607F

/* The following are informational unsolicited error
 * codes for enhanced error reporting which replaces
 * the generic "635" error code.
 */
#define HOST_INVALID_DATA_SECTOR_READ               0x6080
#define HOST_INVALID_PARITY_SECTOR_READ             0x6081
#define HOST_INVALID_SECTOR_READ                    0x6082  /* unused */
#define HOST_DATA_SECTOR_RECONSTRUCTED              0x6083
#define HOST_PARITY_SECTOR_RECONSTRUCTED            0x6084  /* unused */
#define HOST_HARD_ERROR                             0x6085
#define HOST_COMMAND_COMPLETE                       0x6086
#define HOST_STRIPE_RECONSTRUCTED                   0x6087
#define HOST_COMMAND_DROPPED                        0x6088
#define HOST_SECTOR_RECONSTRUCTED                   0x6089
#define HOST_UNCORRECTABLE_PARITY_SECTOR_PPC        0x608A  /* moved to 9053 */
#define HOST_UNCORRECTABLE_DATA_SECTOR_PPC          0x608B  /* moved to 9054 */
#define HOST_HARD_MBUS_CHECKSUM_ERROR               0x608C
#define HOST_SOFT_MBUS_CHECKSUM_ERROR               0x608D
#define HOST_WRITE_STAMP_ERROR                      0x608E
#define HOST_TIME_STAMP_ERROR                       0x608F
#define HOST_DRIVE_DIED                             0x6090
#define HOST_DEVICE_READ_CHECKSUM_ERROR             0x6091
#define HOST_INCOHERENT_STRIPE                      0x6092
#define HOST_UNCORRECTABLE_STRIPE_PPC               0x6093  /* moved to 9055 */
#define HOST_PARITY_INVALIDATED_PPC                 0x6094  /* moved to 9056 */
#define HOST_UNCORRECTABLE_SECTOR_PPC               0x6095  /* moved to 9057 */
#define HOST_MIRROR_INVALIDATED_PPC                 0x6096 /* Renamed to 8064 for K-10+ */
#define HOST_READ_XOR_ERROR                         0x6097
#define HOST_BBU_TESTING                            0x6098
#define HOST_R3_MEMORY_SIZE_REDUCED                 0x6099
#define HOST_SPA_READ_CACHE_SIZE_REDUCED            0x609A
#define HOST_SPB_READ_CACHE_SIZE_REDUCED            0x609B

#define HOST_ATM_SERIAL_PORT_ERROR                  0x609C

#define HOST_BE_FLT_RECOVERY_STARTED                0x609D
#define HOST_BE_FLT_RECOVERY_COMPLETED              0x609E

/*Informational error Codes for Trident/Iron Clad power supplies */
#define HOST_XPE_OK_TO_SHUTDOWN                     0x609F


/* New Soft Media error code */
#define HOST_DISK_SOFT_MEDIA_ERROR                  0x60A0

/* Error code that displays the FRU serial number. This error
   appears immediately after the HOST_FRU_READY error message
*/
#define HOST_FRU_INFORMATION                  0x60A1

/* Messages that appears when hotspare swapin/swapout */

#define HOST_DRIVE_SWAP_IN                    0x60A2
#define HOST_DRIVE_SWAP_OUT                   0x60A3

// the DH requested a proactive spare
#define HOST_DISK_DH_PROACTIVE_SPARE_REQUEST        0x60A4

// the user requested a proactive spare from Navi
#define HOST_DISK_NAVI_PROACTIVE_SPARE_REQUEST      0x60A5

// the user requested a proactive spare from FCLI
#define HOST_DISK_FCLI_PROACTIVE_SPARE_REQUEST      0x60A6

// the proactive sparing request passed all local checks
// and will be initiated (still pending the peer's approval
#define HOST_DISK_PROACTIVE_SPARE_OPERATION_INITIATED 0x60A7

#define HOST_PROACTIVE_COPY_COMPLETED   0x60A8

#define HOST_FRU_PRO_COPY_STARTED       0x60A9

#define HOST_PROACTIVE_SPARE_SWAP_IN       0x60AA

#define HOST_PROACTIVE_SPARE_SWAP_OUT       0x60AB

#define HOST_FRU_PRO_COPY_COMPLETED     0x60B0

#define HOST_FRU_PRO_COPY_ABORTED       0x60B1

/* fix_disk's started, completed, and failed error codes */

#define HOST_FIX_DISK_STARTED           0x60B2
#define HOST_FIX_DISK_COMPLETED         0x60B3
#define HOST_FIX_DISK_FAILED            0x60B4

/* Disk Firmware upgrade process error codes */
#define DISK_FW_REQUEST_RECEIVED_LCC_FW_ABORTING 0x60B5
#define LCC_FW_UPGRADE_ABORTED 0x60B6
#define HOST_DISK_FW_UPGRD_REQ_RECEIVED                    0x60B7
#define HOST_DISK_FW_UPGRD_COMPLETED                         0x60B8
#define HOST_DISK_FW_UPGRD_PEER_COMPLETED               0x60B9

/* LCC Firmware upgrade process error codes */
#define HOST_LCC_FW_UPGRD_REQ_ENQUEUED_DUE_TO_ENCL_ADDED          0x60BA
#define HOST_LCC_FW_UPGRD_REQ_ENQUEUED_DUE_TO_PEER_SP_REBOOT   0x60BB

/* WARNING! See warning at top of file before making any changes or additions */

/* FIBRE channel unsolicited events. 0x60C0-0x60CF
 */
#define HOST_FIBRE_LOOP_ERR                         0x60C0
#define HOST_FIBRE_LOOP_DOWN                        0x60C1
#define HOST_FIBRE_LOOP_HUNG                        0x60C2
#define HOST_FIBRE_LOOP_OPERATIONAL                 0x60C3
#define HOST_FIBRE_NODE_ERROR                       0x60C4
#define HOST_FIBRE_NODE_LOGIN_RETRY                 0x60C5
#define HOST_FIBRE_NODE_LOGIN_FAILED                0x60C6
#define HOST_FIBRE_UNKNOWN_EVENT                    0x60C7

#define HOST_ENCL_4GIG_MODE                         0x60C8

#define HOST_FIBRE_UNKNOWN                          0x60C9  /* Classic
                                                             * Flare */

/* Added NEW FE/BE  events here since there are no
 * more codes below in the 6 range. This is for errata
 * logging.
 */
#define HOST_FE_FIBRE_LP_TO_BB_CRED_ERRATA          0x60CA
#define HOST_BE_FIBRE_LP_TO_BB_CRED_ERRATA          0x60CB

#define HOST_BBU_TEST_CONFIG_CHANGE                 0x60CC

/* Dual Back-End failover/failback control events 0x60D0-0x60D7
 */
#define HOST_SP_FAILOVER_INITIATED                  0x60D0
#define HOST_SP_FAILOVER_DENIED                     0x60D1
#define HOST_SP_FAILAWAY_COMPLETE                   0x60D2
#define HOST_SP_FAILBACK_COMPLETE                   0x60D3
#define HOST_SP_FAILOVER_FROM_SP_COMPLETE           0x60D4
#define HOST_SP_FAILOVER_TO_SP_COMPLETE             0x60D5
#define HOST_SP_FAILBACK_FAILED                     0x60D6


/* Front End FIBRE channel events 0x60D7-0x60EF
 */
#define HOST_FE_RE_INIT_LOOP                        0x60D7
#define HOST_FE_INVAL_TOPOLOGY                      0x60D8
#define HOST_FE_LOSS_OF_SYNC                        0x60D9
#define HOST_FE_LOSS_OF_SIG                         0x60DA
#define HOST_FE_LINK_FLT                            0x60DB
#define HOST_FE_DIRECTED_RST                        0x60DC
#define HOST_FE_RETRIES_EXCEEDED                    0x60DD
#define HOST_FE_LPE_RXD                             0x60DE
#define HOST_FE_LPB_RXD                             0x60DF

#define HOST_FE_PORT_CLOSED                         0x60E0
#define HOST_FE_PORT_ENABLED                        0x60E1
#define HOST_FE_FIBRE_LOOP_BAD                      0x60E2
#define HOST_FE_FIBRE_INITIATOR_GONE                0x60E3
#define HOST_FE_FIBRE_LOOP_DOWN                     0x60E4
#define HOST_FE_FIBRE_LOOP_UP                       0x60E5
#define HOST_FE_FIBRE_LOOP_TO                       0x60E6
#define HOST_FE_FIBRE_LIP_TO                        0x60E7
#define HOST_FE_FIBRE_LUP_TO                        0x60E8
#define HOST_FE_FIBRE_MSG_DROP                      0x60E9
#define HOST_FE_TARGET_LOOP_INIT                    0x60EA
#define HOST_FE_TARGET_CHIP_RST                     0x60EB
#define HOST_FE_OVERLAPPED_CMD_DET                  0x60EC
#define HOST_FE_FRAME_DISCARDED                     0x60ED
#define HOST_FE_SOFT_ALPA_OBTAINED                  0x60EE  /* Classic
                                                             * Flare */
#define HOST_FE_HAMMER_FED                          0x60EF


/* WARNING! See warning at top of file before making any changes or additions */

/* Expansion Driver Unsolicited Events 0x60F0-0x60FF
 */
#define HOST_XD_STARTED                             0x60F0
#define HOST_XD_FINISHED                            0x60F1
#define HOST_XD_HALTED                              0x60F2
#define HOST_XD_RESTARTED                           0x60F3
#define HOST_XD_READ_FAILED                         0x60F4
#define HOST_XD_WRITE_FAILED                        0x60F5
#define HOST_XD_CHECKPOINT_FAILED                   0x60F6

#define HOST_EXPANSION_STOPPED                      0x60F7

#define HOST_LOG_HW_ERR_PPC                         0x60F8 /* Renamed to 8065 for K-10+ */
#define HOST_SOFT_PEER_BUS_ERROR                    0x60F9

#define HOST_ZERO_DISK_STARTED                      0x60FA
#define HOST_ZERO_DISK_CANCELED                     0x60FB
#define HOST_ZERO_DISK_COMPLETED                    0x60FC

#define HOST_EXPANSION_XL_FAILED                    0x60FD

#define HOST_SP_RESYNCRONIZE_PANIC                  0x60FE
#define HOST_STORED_SYSTEM_TYPE_INVALID             0x60FF

/* WARNING! See warning at top of file before making any changes or additions */

/* More Unsolicited Information codes */
#define HOST_INFORMATION_GROUP1_MASK                0x7000

/* Unsolicited codes for Portable LUN's */
#define HOST_FRU_EXPORTED                           0x7001
#define HOST_FRU_IMPORT_STARTED                     0x7002
#define HOST_FRU_FORCED_IMPORT_STARTED              0x7003
#define HOST_FRU_RESUMING_IMPORT                    0x7004
#define HOST_FRU_IMPORT_NONFATAL_ERROR              0x7005
#define HOST_FRU_IMPORT_FATAL_ERROR                 0x7006
#define HOST_FRU_IMPORT_FAILED                      0x7007
#define HOST_FRU_IMPORT_COMPLETED                   0x7008
#define HOST_FRU_EXPORT_STARTED                     0x7009
#define HOST_FRU_EXPORT_FATAL_ERROR                 0x700A
#define HOST_FRU_EXPORT_NONFATAL_ERROR              0x700B
#define HOST_FRU_EXPORT_FAILED                      0x700C
#define HOST_FRU_EXPORT_COMPLETED                   0x700D
#define HOST_FRU_CLEAR_EXPORT_STARTED               0x700E
#define HOST_FRU_CLEAR_EXPORT_FAILED                0x700F
#define HOST_FRU_CLEAR_EXPORT_COMPLETED             0x7010

#define HOST_HOTSPARE_CONFIG_RESET                  0x7011
#define HOST_SHUTDOWN_CACHE_DISABLING               0x7012

#define HOST_SP_ULOG_CLEAR                          0x7013
#define HOST_SYSTEM_READ_CACHE_ENABLED              0x7014
#define HOST_SYSTEM_READ_CACHE_DISABLED             0x7015
#define HOST_POSSIBLE_CHASSIS_REPLACEMENT           0x7016
#define HOST_NONVOL_REINITIALIZED                   0x7017

#ifdef VOD_FEATURE
/* Status codes for remapping in video flares */
#define HOST_ASSIGN_REMAP_FAILED                        0x7020
#define HOST_ASSIGN_REMAP_STARTED                       0x7021
#define HOST_ASSIGN_REMAP_COMPLETED                     0x7022
#endif

#define HOST_SMU_FIRMWARE_REVISION         0x7023
#define HOST_UNABLE_TO_VALIDATE_SMU_IDENTITY    0x7024
#define HOST_IO_PORT_LINK_DOWN                  0x7025
#define HOST_IO_PORT_LINK_UP                    0x7026
#define HOST_IO_PORT_LINK_DEGRADED              0x7027

/*
 * Flare Network-related informational codes
 * These codes are use for recording events in the ulog.
 * Note: Flare Network-related operational error codes
 *       are in the 0x20YY range
 */
#define HOST_FLNET_NETWORK_INITTED                  0x7030
#define HOST_FLNET_NETWORK_CONFIGURED               0x7031
#define HOST_FLNET_NETWORK_STARTED                  0x7032
#define HOST_FLNET_NETWORK_STOPPED                  0x7033
#define HOST_FLNET_NETWORK_UNCONFIGURED             0x7034
#define HOST_FLNET_TELNET_CONNECT                   0x7035
#define HOST_FLNET_TELNET_DISCONNECT                0x7036
#define HOST_FLNET_FCLI_LOGIN_DISABLED              0x7037
#define HOST_FLNET_FCLI_LOGIN_FAILED                0x7038
#define HOST_FLNET_FCLI_USER_LOGIN                  0x7039
#define HOST_FLNET_FCLI_SERVICE_LOGIN               0x703A
#define HOST_FLNET_FCLI_LOGIN_MAX_CONNECTIONS       0x703B
#define HOST_FLNET_CFG_UCAST_IP_ACCEPTED            0x703C
#define HOST_FLNET_CFG_UCAST_IP_ERROR               0x703D
#define HOST_FLNET_CFG_UDP_BCAST_ACCEPTED           0x703E
#define HOST_FLNET_CFG_UDP_BCAST_ERROR              0x703F
#define HOST_FLNET_CFG_NETMASK_UPDATE_RECVD         0x7040
#define HOST_FLNET_CFG_NETMASK_UPDATE_TIMEOUT       0x7041
#define HOST_FLNET_BSD_DUPLICATE_IP_ADDR            0x7042
#define HOST_FLNET_BSD_ATTEMPTED_SRCRT              0x7043
#define HOST_FLNET_TELNET_NEGOTIATION_TIMEOUT       0x7044
#define HOST_FLNET_TELNET_NEGOTIATION_MAX           0x7045
#define HOST_FLNET_SOCKETS_EXHAUSTED                0x7046
#define HOST_FLNET_NETWORK_INIT_FAILED              0x7047
#define HOST_FLNET_NETWORK_START_FAILED             0x7048
#define HOST_FLNET_CFG_ADD_ROUTE_ERROR              0x7049
#define HOST_FLNET_CFG_ADD_IFADDR_ERROR             0x704A
#define HOST_FLNET_CFG_DEL_IFADDR_ERROR             0x704B
#define HOST_FLNET_CFG_SET_IF_UP_ERROR              0x704C
#define HOST_FLNET_CFG_SET_IF_DOWN_ERROR            0x704D
#define HOST_FLNET_BSD_INTERNAL_EXCEPTION           0x704E
#define HOST_LCC_MCODE_UPGRADE_MESSAGE              0x704F

/* Unsolicited Codes for Data Directory Main Service 0x7050 - 0x7059 */

#define HOST_DD_MS_DISK_INSERTED                       0x7050
#define HOST_DD_MS_FULL_INIT_STARTED                   0x7051
#define HOST_DD_MS_DISK_REMOVED                        0x7052
#define HOST_DD_MS_FULL_INIT_COMPLETED                 0x7053
#define HOST_DD_MS_RECONSTRUCTED                       0x7054
#define HOST_DD_MS_FULL_RECONSTRUCTED                  0x7055
#define HOST_DD_MS_RECONSTRUCTION_ABORTED              0x7056
#define HOST_DD_MS_DISK_INSERTED_ZERO_MARK_DETECTED    0x7057
#define HOST_DD_MS_LAYOUT_COMMITTED                    0x7058
#define HOST_DD_MS_WRITE_NEW_LAYOUT_FAILED_ON_DISK     0x7059

/* Unsolicited Codes for xPE fans & power supplies 0x7060 - 0x7069 */
#define HOST_XPE_PSA_INSTALLED                      0x7060
#define HOST_XPE_PSB_INSTALLED                      0x7061
#define HOST_XPE_FAN_MOD1_INSTALLED                 0x7062
#define HOST_XPE_FAN_MOD2_INSTALLED                 0x7063
#define HOST_XPE_FAN_MOD3_INSTALLED                 0x7064
#define HOST_XPE_FAN_MOD1_FAULTED                   0x7065
#define HOST_XPE_FAN_MOD2_FAULTED                   0x7066
#define HOST_XPE_FAN_MOD3_FAULTED                   0x7067
#define HOST_XPE_SINGLE_FAN_FAULT                   0x7068
#define HOST_XPE_PEER_PS_INSTALLED                  0x7069

#define HOST_RECOMMEND_DISK_FIRMWARE_UPGRADE        0x7070 /* Used in Longbow */
#define HOST_COMMAND_FAILED                         0x7071

/* Unsolicited Codes for AC power conditions. */
#define HOST_XPE_PSA_AC_POWER_RESTORED              0x7073
#define HOST_XPE_PSB_AC_POWER_RESTORED              0x7074
#define HOST_XPE_PSA_AC_POWER_TEST                  0x7075
#define HOST_XPE_PSB_AC_POWER_TEST                  0x7076

/* Unsolicited Codes for LCC mCode download. */
#define HOST_LCC_MCODE_UPGRADE_COMPLETE             0x7077
#define HOST_LCC_MCODE_UPGRADE_ALL_COMPLETE         0x7078
#define HOST_LCC_MCODE_UPGRADE_AT_REV               0x7079


/* Unsolicited Codes for BIOS, POST and FRUMON Revision */
#define HOST_BIOS_REV                               0x7080
#define HOST_POST_REV                               0x7081
#define HOST_FRUMON_REV                             0x7082

/* Unsolicited Codes for AC power failure to an xPE power supply. */
/* These were originally 900 level, but decision made to not */
/* phone call these in, so making 700 level. */
#define HOST_XPE_PSA_AC_POWER_FAIL                  0x7083
#define HOST_XPE_PSB_AC_POWER_FAIL                  0x7084


/* Database copy of system serial number updated from Resume PROM. */
#define HOST_SERIAL_NUM_UPDATE_FROM_RESUME_PROM     0x7085

/* Unsolicited Codes for support of X1-Lite.  This system has  */
/* a legal configuration of only one SP and one PS, but these  */
/* should be in the A slots.  Following codes are for a single */
/* SP or PS in the B slot.                                     */
#define HOST_POWER_SUPPLY_IN_INCORRECT_SLOT         0x7086
#define HOST_SP_IN_INCORRECT_SLOT                   0x7087

/* Unsolicited Codes for LCC mCode download. */
#define HOST_LCC_MCODE_UPGRADE_CHECK                0x7088
#define HOST_LCC_MCODE_UPGRADE_STARTING             0x7089

/* This code gets logged when cache won't enable.  The extended code
 * indicates the reason, to allow the user to fix it.
 */
#define HOST_SYSTEM_WRITE_CACHE_ENABLE_PENDING      0x708A

/* These error codes are used to indicate that the drives are physically
 * inserted or removed as opposed to being bypassed or unbypassed by
 * software
 */
#define HOST_DRIVE_PHYS_REMOVED                     0x708B
#define HOST_DRIVE_PHYS_INSERTED                    0x708C

/* The System Orientation field was updated */
#define HOST_USER_SYSTEM_ORIENTATION_UPDATE          0x708D

/* PCIMEM/ NonvolCache(nvcache) Informational codes */
#define HOST_PCIMEM_CARD_ERASED                     0x708E
#define HOST_PCIMEM_CARD_CONFIGURED                 0x708F
#define HOST_PCIMEM_CARD_BATTERIES_DISABLED         0x7090
#define HOST_PCIMEM_CARD_INITIALIZED                0x7091
#define HOST_LUN_CACHE_DIRTY_CLEARED                0x7092

/* Unsolicited Codes related to cabling of a new enclosure. */
/* When adding a new enclosure CM will try to determine if  */
/* both LCCs are connected to the same BE Bus.  The follow- */
/* ing codes, along with the code HOST_ENCLOSURE_BE_LOOP_   */
/* INCORRECT indicate what CM has found.                    */
#define HOST_ENCLOSURE_BE_LOOP_CORRECT               0x7093
#define HOST_ENCLOSURE_BE_LOOP_CONNECTED             0x7094

#define HOST_CACHE_CLEAN_WITH_DIRTY_UNITS            0x7095

// A degraded CMI channel has been restored
#define HOST_CMI_CONNECTION_RESTORED                 0x7096
#define HOST_UNIT_CAPACITY_MODIFIED                  0x7097


#define HOST_DRIVE_PBC_STATUS_CHANGED                0x7098
#define HOST_PEER_REQUESTED_FRU_POWER_DOWN           0x7099
#define HOST_CACHE_DIRTY_PEER_HAS_DATA               0x709A
#define HOST_CACHE_DIRTY_PEER_VAULT_NOT_READY        0x709B
#define HOST_DRIVE_UNSTABLE_KEEPING_OFFLINE          0x709C

/* Unsolicited codes related to enabling/disabling ATA drive */
/* write cache.*/
#define HOST_ATA_WR_CACHE_ENA                        0x709D
#define HOST_ATA_WR_CACHE_DIS                        0x709E

/* codes for yukon eventlog extraction */
#define HOST_LCC_GET_YUKON_LOG_SUCCEEDED             0x709F

/* This is in K10KernelMsg.mc so I am puting it here to
   reserve the number */
//#define HOST_PEER_ALIVE_DURING_CONVERSION            0x70A0

/* Verify information codes.
 */
#define HOST_SNIFF_VERIFY_ENABLED                    0x70A1
#define HOST_SNIFF_VERIFY_DISABLED                   0x70A2
#define HOST_SNIFF_VERIFY_USER_CONTROL_ENABLED       0x70A3
#define HOST_SNIFF_VERIFY_USER_CONTROL_DISABLED      0x70A4

 /* NR information codes.
 */
#define HOST_NR_STARTED                              0x70A5
#define HOST_NR_PEER_STARTED                         0x70A6
#define HOST_NR_REQUEST_COMPLETE                     0x70A7

/* Logged when an enclosure goes online */

#define HOST_ENCL_ADDED                              0x70A8

/*  Unsolicited codes for Hammer series fans.  */
#define HOST_XPE_FAN_MOD_A_INSTALLED                 0x70A9
#define HOST_XPE_FAN_MOD_A_FAULTED                   0x70AA
#define HOST_XPE_FAN_MOD_B_INSTALLED                 0x70AB
#define HOST_XPE_FAN_MOD_B_FAULTED                   0x70AC
#define HOST_XPE_FAN_MOD_C_INSTALLED                 0x70AD
#define HOST_XPE_FAN_MOD_C_FAULTED                   0x70AE
#define HOST_XPE_FAN_MOD_D_INSTALLED                 0x70AF
#define HOST_XPE_FAN_MOD_D_FAULTED                   0x70B0

 /* XD internal failure, Informational type only    */
#define HOST_XD_INTERNAL_ASSIGN_FAILED               0x70B1

#define HOST_LCC_SPECIAL_MODE                        0x70E8

/*  Unsolicited codes for HammerHead power supply status.  */
#define HOST_XPE_PS_SMB_ACCESS_FAILED                0x70B2

/* Informational code for Rebuild Logging */
#define HOST_RBLD_MAX_LOG_EXISTS                     0x70B3

/* Enter Probation Error Message  */
#define HOST_FRU_START_PROBATION                     0x70B4

/* End Probation Error Message    */
#define HOST_FRU_END_PROBATION                       0x70B5

/* This is in K10KernelMsg.mc so I am puting it here to
   reserve the number */
//#define HOST_PEER_SP_BOOT_SUCCESS_32BIT               0x70B6
//#define HOST_PEER_SP_BOOT_OTHER_32BIT                 0x70B7

/*  A request to reboot an SP has been initiated */
#define HOST_SP_REBOOT_INITIATED                     0x70B8

/* HammerHead - conversion commit check flag disabled */
#define HOST_CONVERSION_COMMIT_CHECK                 0x70B9

/* Cache enable pending codes */
#define HOST_CACHE_ENABLE_PENDING_NO_CACHE           0x70BA
#define HOST_CACHE_ENABLE_PENDING_USER_DISABLED      0x70BB
#define HOST_CACHE_ENABLE_PENDING_NOT_HA             0x70BC
#define HOST_CACHE_ENABLE_PENDING_VAULT_NOT_RDY      0x70BD
#define HOST_CACHE_ENABLE_PENDING_PEER_CA_INIT_FAIL  0x70BE
#define HOST_CACHE_ENABLE_PENDING_CA_INIT_FAILED     0x70BF
#define HOST_CACHE_ENABLE_PENDING_PEER_SP_NOT_RDY    0x70C0
#define HOST_CACHE_ENABLE_PENDING_NO_CA_TABLE        0x70C1
#define HOST_CACHE_ENABLE_PENDING_NOT_ENOUGH_PAGES   0x70C2

#define HOST_CACHE_ENABLE_PENDING_XPE_OR_DPE_FAN     0x70C3
#define HOST_CACHE_ENABLE_PENDING_DAE_0_FAN          0x70C4
#define HOST_CACHE_ENABLE_PENDING_XPE_OR_DPE_PS      0x70C5
#define HOST_CACHE_ENABLE_PENDING_DAE_0_PS           0x70C6
#define HOST_CACHE_ENABLE_PENDING_SPS                0x70C7
#define HOST_CACHE_ENABLE_PENDING_CMI_CHANNEL        0x70C8
#define HOST_CACHE_ENABLE_PENDING_SAFE_DISKS         0x70C9
/* FRU was not marked for probation Message    */
#define HOST_FRU_NOT_PROBATION                       0x70CA

/* Probation was cancelled for fru    */
#define HOST_FRU_CANCEL_PROBATION                    0x70CB

/* Probation fru is powering up.*/
#define HOST_FRU_POWERUP_PROBATION                   0x70CC


/*
 *Unsolicited Codes for Back End SFPs (informational)
 */
#define HOST_SFP_GOOD                                0x70CD
#define HOST_SFP_INSERTED                            0x70CE
#define HOST_SFP_REMOVED                             0x70CF



/* Fast Bind */
#define HOST_UNIT_ZEROING_ON_DEMAND_STARTED          0x70D0
#define HOST_UNIT_ZEROING_ON_DEMAND_CONTINUED        0x70D1
#define HOST_UNIT_ZEROING_ON_DEMAND_TRANS_ON_ASSIGN  0x70D2
#define HOST_UNIT_ZEROING_ON_DEMAND_COMPLETE         0x70D3
#define HOST_UNIT_BACKGROUND_ZEROING_STARTED         0x70D4
#define HOST_UNIT_BACKGROUND_ZEROING_HALTED          0x70D5
#define HOST_UNIT_ZERO_MAP_PAGE_RECOVERED            0x70D6
#define HOST_UNIT_ZERO_MAP_HEADER_RECOVERED          0x70D7


/*  A management fru has been inserted or has stopped failing. */
#define HOST_XPE_MGMT_FRU_A_INSTALLED_RESTORED        0x70DC
#define HOST_XPE_MGMT_FRU_B_INSTALLED_RESTORED        0x70DD
#define HOST_XPE_MGMT_FRU_A_LINK_UP                   0x70DE
#define HOST_XPE_MGMT_FRU_B_LINK_UP                   0x70DF

/* Back End Device Speed Codes */
#define HOST_CURRENT_LOOP_SPEED                      0x70E0
#define HOST_CURRENT_ENCL_SPEED                      0x70E1
#define HOST_LOOP_SPEED_REINIT                       0x70E2

/* Host drive user initiated command*/
#define HOST_DRIVE_USER_INITIATED_COMMAND            0x70E3

/* Unsolicited Codes for xPE power supplies */
#define HOST_XPE_PSA0_INSTALLED                      0x70E4
#define HOST_XPE_PSA1_INSTALLED                      0x70E5
#define HOST_XPE_PSB0_INSTALLED                      0x70E6
#define HOST_XPE_PSB1_INSTALLED                      0x70E7

/* Reserving a Error code for Brion Philbin for Relese 19 ATA
 * changes
 */
#define HOST_LCC_SPECIAL_MODE                        0x70E8

/* Unsolicited Codes for xPE Management Modules, originally Nor'Easter */
#define HOST_XPE_MGMT_FRU_A_NO_NETWORK_CONNECTION    0x70E9
#define HOST_XPE_MGMT_FRU_B_NO_NETWORK_CONNECTION    0x70EA


#define HOST_LOOP_SPEED_RESET_CACHE_DISABLE_FAILED   0x70EB

#define LCC_STATS_RETRIEVAL_FAILED                   0x70EC

/* Unsolicited Codes for xPE power supply ac fail */
#define HOST_XPE_PSA0_AC_FAIL                       0x70ED
#define HOST_XPE_PSA1_AC_FAIL                       0x70EE
#define HOST_XPE_PSB0_AC_FAIL                       0x70EF
#define HOST_XPE_PSB1_AC_FAIL                       0x70F0

#define HOST_XPE_PSA0_AC_RESTORED                   0x70F1
#define HOST_XPE_PSA1_AC_RESTORED                   0x70F2
#define HOST_XPE_PSB0_AC_RESTORED                   0x70F3
#define HOST_XPE_PSB1_AC_RESTORED                   0x70F4

#define HOST_XPE_PSA0_AC_TEST                       0x70F5
#define HOST_XPE_PSA1_AC_TEST                       0x70F6
#define HOST_XPE_PSB0_AC_TEST                       0x70F7
#define HOST_XPE_PSB1_AC_TEST                       0x70F8

#define HOST_LCC_STATS_SUSPECT_COMPONENT            0x70FA
#define HOST_FRUMON_UPGRADE_COMMIT_BLOCKED          0x70FC
#define HOST_LCC_REV_MISMATCH                       0x70FF

/* Error codes 0xB000 - 0xD0FF are reserved for Informational
 * Message See comments towards end of file
 */


/*************************************************************
 * IMPORTANT * IMPORTANT * IMPORTANT * IMPORTANT * IMPORTANT *
 *************************************************************
 * REMEMBER to update catmerge\interface\k10kernelmsg.mc     *
 * file when you add a new sunburst errors greater than      *
 * 0x6000 because NT event log will retrieve the description *
 * of a specific event log message from k10kernelmsg.mc file *
 * otherwise the NT event log will not display the descrip-  *
 * ion properly.                                             *
 * Also, ulog_utils.c must be updated to get the correct     *
 * text strings in the Flare ULOG (which is still used by    *
 * the NAS version of Chameleon).                            *
 *************************************************************/

/* 16-bit extended status codes for Portable LUN's Export */
#define HOST_EXPORT_INVALID_RG                      0x01
#define HOST_EXPORT_RG_BUSY                         0x02
#define HOST_EXPORT_MAX_EXPORTED_DISKS              0x03
#define HOST_EXPORT_HOTSPARE_IN_USE                 0x04
#define HOST_EXPORT_FRU_NOT_READY                   0x05
#define HOST_EXPORT_VAULT_DRIVE_NEEDED              0x06
#define HOST_EXPORT_LUN_CACHE_DIRTY                 0x07
#define HOST_EXPORT_LUN_NOT_READY                   0x08
#define HOST_EXPORT_UNSUPPORTED_UNIT_TYPE           0x09
#define HOST_EXPORT_RELEASE_ERROR                   0x0A
#define HOST_EXPORT_UNBIND_ERROR                    0x0B
#define HOST_EXPORT_WRITE_ERROR                     0x0C
#define HOST_EXPORT_READ_ERROR                      0x0D
#define HOST_EXPORT_INTERNAL_ERROR                  0x0E
#define HOST_EXPORT_PEER_SP_ERROR                   0x0F
#define HOST_EXPORT_DB_READ_ERROR                   0x10

/* 16-bit extended status codes for Portable LUN's Import */
#define HOST_IMPORT_NO_RG_DB_DISK                   0x01
#define HOST_IMPORT_INVALID_RG_DB_DISK              0x02
#define HOST_IMPORT_RG_DB_MISMATCH                  0x03
#define HOST_IMPORT_INVALID_FRU_SIG                 0x04
#define HOST_IMPORT_MISSING_IMPORT_INFO             0x05
#define HOST_IMPORT_FRU_MISMATCH                    0x06
#define HOST_IMPORT_UNSUPPORTED_FLARE_REV           0x07
#define HOST_IMPORT_FRU_NOT_READY                   0x08
#define HOST_IMPORT_FRU_NOT_EXPORTED                0x09
#define HOST_IMPORT_FRU_REMOVED                     0x0A
#define HOST_IMPORT_ALL_LUNS_ACTIVE                 0x0B
#define HOST_IMPORT_ALL_RAID_GROUPS_ACTIVE          0x0C
#define HOST_IMPORT_DATABASE_NOT_UPDATABLE          0x0D
#define HOST_IMPORT_UNSUPPORTED_CHECKSUM_TYPE       0x0E
#define HOST_IMPORT_READ_ERROR                      0x1F
#define HOST_IMPORT_WRITE_ERROR                     0x10
#define HOST_IMPORT_INTERNAL_ERROR                  0x11
#define HOST_IMPORT_DEFINE_ERROR                    0x12
#define HOST_IMPORT_UNSUPPORTED_UNIT_TYPE           0x13
#define HOST_IMPORT_MAX_EXPORTED_DISKS              0x14
#define HOST_IMPORT_INVALID_FRU_NUMBERS             0x15
#define HOST_IMPORT_INVALID_LUN_NUMBERS             0x16
#define HOST_IMPORT_INVALID_RAID_GROUP              0x17
#define HOST_IMPORT_PEER_SP_ERROR                   0x18
#define HOST_IMPORT_WRITE_CACHE_NOT_CONFIGURED      0x19

/* 16-bit extended status codes for Portable LUN's Clear-Export */
#define HOST_CLEAR_EXPORT_FRU_NOT_EXPORTED          0x01
#define HOST_CLEAR_EXPORT_BAD_FRU_NUMBER            0x02
#define HOST_CLEAR_EXPORT_FRU_NOT_READY             0x03
#define HOST_CLEAR_EXPORT_DEFINE_ERROR              0x04
#define HOST_CLEAR_EXPORT_READ_ERROR                0x05
#define HOST_CLEAR_EXPORT_WRITE_ERROR               0x06
#define HOST_CLEAR_EXPORT_PEER_SP_ERROR             0x07


/* WARNING! See warning at top of file before making any changes or additions */

/* Soft Unsolicited Errors */
#define HOST_SOFT_UNSOLICITED_MASK                  0x8000
#define HOST_SOFT_SCSI_BUS_ERROR                    0x8001
#define HOST_ILLEGAL_SCSI_BUS_INTERRUPT             0x8002  /* unused */
#define HOST_RECOMMEND_DISK_REPLACEMENT             0x8003
#define HOST_SOFT_SBERR_WS_TOLERANCE_ERROR          0x8004
#define HOST_SOFT_SBERR_RS_TOLERANCE_ERROR          0x8005
#define HOST_RECOMMEND_DISK_FIRMWARE_UPGRADE_PPC    0x8006
#define HOST_INFO_WARM_REBOOT                       0x8010  /* Warm Reboot */

#define HOST_SOFT_MEDIA_ERROR                       0x8020
#define HOST_UNBIND_HOT_SPARE_IN_USE                0x8030

#define HOST_SECTOR_INVALIDATED                     0x8040

/* extended status for HOST_SECTOR_INVALIDATED */
#define HS_INVALID_VERIFY                           0x00
#define HS_INVALID_CACHE                            0x01
#define HS_INVALID_REBUILD                          0x02
#define HS_INVALID_MP_REBUILD                       0x03
#define HS_INVALID_MS_REBUILD                       0x04

#define HOST_LCC_RESET_ENCL_CHANGED_STATE           0x8050 
#define HOST_ENCL_INVAL_ADDR                        0x8051
#define HOST_ENCL_DUP_ADDR                          0x8052
#define HOST_LCC_CABLES_CROSSED                     0x8053
#define HOST_LCC_CABLES_WRONG_INPUT_PORT            0x8054

/* Unsolicited Codes for xPE 0x8055 */
#define HOST_XPE_CHANGED_STATE                      0x8055
#define HOST_ENCL_25V_FAULT                         0x8056
#define HOST_ENCL_33V_FAULT                         0x8057

#define HOST_RECOVERABLE_RESET_EVENT                0x8060  /* unused */
#define HOST_SOFT_SBERR_DETECTED                    0x8061
#define HOST_SOFT_WS_SBERR_DETECTED                 0x8062
#define HOST_SOFT_RS_SBERR_DETECTED                 0x8063
#define HOST_MIRROR_INVALIDATED                     0x8064
#define HOST_LOG_HW_ERR                             0x8065

/* Unsolicited Codes for LCC mCode Download */
#define HOST_LCC_MCODE_UPGRADE_ERROR                0x8066

#define HOST_RESUME_PROM_READ_FAILED                0x8067

/* Database copy of system serial number still different from Resume PROM
 * copy after update. */
#define HOST_SERIAL_NUM_UPDATE_FAILED               0x8068

/* Unsolicited Codes for LCC mCode Download */
#define HOST_LCC_MCODE_UPGRADE_AT_LOWER_REV         0x8069


/* Battery on the SP board is either missing or running with low
 * power
 */
#define HOST_SP_BATTERY_LOW_OR_MISSING              0x8070

/* Unsolicited Code for support of X1-Lite.  This system has     */
/* a legal configuration of only one SP and 3 disks, 0 - 2.  The */
/* single should be in the A slot, but flare will come up if it  */
/* is in the B slot.  However, the single SP B, would not have   */
/* a mirrored boot disk.                                         */
#define HOST_BOOT_DISK_NOT_MIRRORED                 0x8071

/* CMI unsolicited codes */
#define HOST_CMI_CONNECTION_DEGRADED                0x8073

#define HOST_DRIVE_STATE_CHANGED                    0x8074

/* Unsolicited Code related to cabling of a new enclosure.  */
/* When adding a new enclosure CM will try to determine if  */
/* both LCCs are connected to the same BE Bus.  The follow- */
/* ing code, along with the codes HOST_ENCLOSURE_BE_LOOP_   */
/* CONNECTED and HOST_ENCLOSURE_BE_LOOP_CORRECT indicate    */
/* what CM has found.                                       */
#define HOST_ENCLOSURE_BE_LOOP_INCORRECT            0x8075

/*
 * New codes for Stiletto 2G Support
 */
#define HOST_ENCL_UNSUPPORTED_4GIG_MODE             0x8076
#define HOST_DRIVE_INIT_FAILURE                     0x8077
#define HOST_LCC_GENERAL_FAULT                      0x8078

/* codes for yukon eventlog extraction */
#define HOST_LCC_GET_YUKON_LOG_FAILED               0x8079

/*  Unsolicited codes for SMBus read/write errors */
#define HOST_SMB_ACCESS_FAILED                      0x807A

/* code for aborting the cache dump to vault */
#define HOST_CACHE_DUMP_ABORTED                     0x807B

/*  A management fru has been removed or is failing. */
#define HOST_XPE_MGMT_FRU_A_FAULTED_REMOVED         0x807C
#define HOST_XPE_MGMT_FRU_B_FAULTED_REMOVED         0x807D
#define HOST_XPE_MGMT_FRU_A_NO_LINK                 0x807E
#define HOST_XPE_MGMT_FRU_B_NO_LINK                 0x807F

#define HOST_CANNOT_READ_REG_SPS_DEVICE             0x8080
#define HOST_SPS_CANNOT_OPEN_SERIAL_PORT            0x8081

/*  Unsolicited warning level codes for Hammer series fans.  */
#define HOST_XPE_FAN_A_FAULTED_REMOVED              0x8082
#define HOST_XPE_FAN_B_FAULTED_REMOVED              0x8083
#define HOST_XPE_FAN_C_FAULTED_REMOVED              0x8084
#define HOST_XPE_FAN_D_FAULTED_REMOVED              0x8085

/*  Unsolicited warning level codes for Hammer IO cards.  */
#define HOST_FIBRE_IO_CARD_FAULTED                  0x8086

/* Unsolicited Codes for Engine ID pin faults */
#define HOST_XPE_ENGINE_ID_FAULTED                  0x8087



/* This code skips ulog and generates the event directly through 
 * KLogWriteEntry. This code is reserved and considered in use.
 * A core driver did not return from DriverEntry with success status. 
 */
//#define HOST_PEER_SP_DRIVER_PROBLEM                 0x808B

/* Unsolicited Code related to enclosure stability. This event is 
 * introduced when any enclosure is marked as unstable.
 */
#define HOST_UNSTABLE_ENCLOSURE                     0x808C

/* Flare reports "Power supply Faulted or Removed" when 
 * Power supply goes in Overtemp condition. This is required 
 * as currently the Flare does not report any changes when 
 * the PS detects the OverTemp condition.
 */

#define HOST_PS_DETECTED_AMBIENT_OVERTEMP           0x808D

/* For Management module service port */
#define HOST_MGMT_FRU_A_SERVICE_PORT_LINK_UP        0x8095
#define HOST_MGMT_FRU_B_SERVICE_PORT_LINK_UP        0x8096



// resume prom write failure
#define HOST_RESUME_PROM_WRITE_FAILED               0x808E

#define HOST_ICA_FLASH_ERASING_ERROR                0x808F
#define HOST_SP_SHUTDOWN_FAILED                     0x8090
#define HOST_SP_REBOOT_FAILED                       0x8091
#define HOST_SP_SHUTDOWN_REQUEST_TO_PEER_FAILED     0x8092
#define HOST_SP_REBOOT_REQUEST_TO_PEER_FAILED       0x8093

#define HOST_SATA_HOTSPARE_SWAPPED_INTO_SAS         0x8094

#define HOST_IO_MODULE_REMOVED                      0x8097

#define HOST_DRIVE_DOWNLOAD_ERROR                   0x80A0
#define HOST_DISK_FW_UPGRD_REQ_REJECTED             0x80A1 

#define HOST_DISK_CLAR_STRING_MISSING               0x80A2

/* Unbind needs to be failed because the LUN is running a background service */
#define HOST_UNBIND_BGS_RUNNING                     0x80A3 

#define HOST_DISK_SEL_ID_ERROR                      0x80A4

#define HOST_REQUEST_NOT_SUPPORTED_DURING_NDU       0x80A5

// Firmware Update failure in EFUP thread
#define HOST_FUP_FAILED                             0x80A6

#define HOST_ENCL_SHUTDOWN_SCHEDULED                0x80A7 

/*************************************************************
 * IMPORTANT * IMPORTANT * IMPORTANT * IMPORTANT * IMPORTANT *
 *************************************************************
 * REMEMBER to update catmerge\interface\k10kernelmsg.mc     *
 * file when you add a new sunburst errors greater than      *
 * 0x6000 because NT event log will retrieve the description *
 * of a specific event log message from k10kernelmsg.mc file *
 * otherwise the NT event log will not display the descrip-  *
 * ion properly.                                             *
 * Also, ulog_utils.c must be updated to get the correct     *
 * text strings in the Flare ULOG (which is still used by    *
 * the NAS version of Chameleon).                            *
 *************************************************************/


/* WARNING! See warning at top of file before making any changes or additions */

/* Hard Unsolicited Errors */
#define HOST_HARD_UNSOLICITED_MASK                  0x9000
#define HOST_HARD_SCSI_BUS_ERROR                    0x9001
#define HOST_FAN_SHUTDOWN_REMOVED                   0x9003
#define HOST_VSC_SHUTDOWN_REMOVED                   0x9004
#define HOST_EGG_CRATE_OVER_TEMPERATURE             0x9005  /* unused */
#define HOST_UNIT_HAS_BEEN_SHUTDOWN                 0x9006
#define HOST_CONTROLLER_FIRMWARE_PANIC              0x9007
#define HOST_FAULT_CACHE_DISABLING                  0x9008
#define HOST_CANT_DUMP_SAFE                         0x9009
#define HOST_CANT_ASSIGN_CACHE_DIRTY_UNIT           0x900A
#define HOST_CANT_INIT_NEW_CACHE                    0x900B
#define HOST_CACHE_IMAGE_BIGGER_THAN_MEMORY         0x900C
#define HOST_BBU_REMOVED                            0x900D
#define HOST_BBU_DISABLED_SAYS_READY                0x900E
#define HOST_CACHE_RECOVERED_WITH_ERRORS            0x900F
#define HOST_CACHE_RECOVERY_FAILED                  0x9010

#define HOST_INVALID_SPS_POWER_CABLE_1_CONFIG       0x9011
#define HOST_INVALID_SPS_POWER_CABLE_2_CONFIG       0x9012
#define HOST_INVALID_SPS_SERIAL_CABLE_CONFIG        0x9013
#define HOST_INVALID_SPS_MULTIPLE_CABLES_CONFIG     0x9014
#define HOST_UNSUPPORTED_SPS_TYPE                   0x9015
#define HOST_SPS_TEST_NOT_RUN                       0x9016

#define HOST_HARD_MEDIA_ERROR                       0x9020
#define HOST_VAULT_LOAD_FAILED                      0x9021
#define HOST_VAULT_LOAD_INCONSISTENT                0x9022
#define HOST_VAULT_LOAD_FAILED_BITMAP_OK            0x9023
#define HOST_VAULT_DISKS_SCRAMBLED                  0x9024
#define HOST_SB_CACHE_PROM_REV                      0x9025  /* unused */
#define HOST_R3_CANT_ASSIGN_NO_MEM                  0x9026
#define HOST_OLD_R3_CANT_ASSIGN                     0x9027  /* unused */
#define HOST_R3_CANT_INIT_NO_MEM                    0x9028
#define PCMCIA_CARD_WRITE_PROTECT                   0x9030  /* unused */
#define PCMCIA_CARD_BATTERY_FAILED                  0x9031  /* unused */
#define PCMCIA_CARD_ABSENT                          0x9032  /* unused */
#define PCMCIA_POST_FAILED                          0x9033  /* unused */
#define PCMCIA_DATA_LOCKED                          0x9034  /* unused */
#define PCMCIA_BAD_SIZE                             0x9035  /* unused */
#define PCMCIA_DMA_XFER_FAILED                      0x9036  /* unused */
#define OBSOLETE_HOST_COMMAND_FAILED                0x9037  /* unused */
#define HOST_NON_R3_CANT_ASSIGN                     0x9038
#define HOST_NON_R3_LUNS_CANT_CONVERT               0x9039
#define HOST_BBU_FAULTED                            0x9040
#define HOST_BBU_BAT_ONLINE                         0x9041
#define HOST_SBERR_THRESHOLD_EXCEEDED               0x9042
#define HOST_CANT_ASSIGN_L2_CACHE_DIRTY_UNIT        0x9043
#define HOST_HARD_PEER_BUS_ERROR                    0x9044
#define HOST_DSKML_SP_LINK_DOWN                     0x9045
#define HOST_DSKML_SP_LINK_DOWN_WILL_SUSPEND_PEER   0x9046
#define HOST_DSKML_SP_LINK_DOWN_WILL_SUSPEND_SELF   0x9047
#define HOST_DSKML_SP_LINK_ERROR_WILL_REBOOT        0x9048
#define HOST_ILLEGAL_UPREV_CONFIGURATION            0x9049
#define HOST_ILLEGAL_DOWNREV_CONFIGURATION          0x904A
#define HOST_UNSUPPORTED_CANT_ASSIGN                0x904B
#define HOST_CANT_ASSIGN_BAD_EXPANSION_CHKPTS       0x904C
#define HOST_CANT_ASSIGN_EXP_CHKPTS_DB_MISMATCH     0x904D
#define HOST_USING_INVALID_WWN                      0x904E
#define HOST_UNCORRECTABLE_VERIFY_ERROR             0x904F  /* was 6019 */
#define HOST_DEGRADED_NONVOL_VERIFY_DATA_LOSS       0x9050  /* was 6020 */
#define HOST_FRU_SIGNATURE_ERROR                    0x9051  /* was 6050 */
#define HOST_FAN_PACK_DEGRADED                      0x9052  /* was 606C */
#define HOST_UNCORRECTABLE_PARITY_SECTOR            0x9053  /* was 608A */
#define HOST_UNCORRECTABLE_DATA_SECTOR              0x9054  /* was 608B */
#define HOST_UNCORRECTABLE_STRIPE                   0x9055  /* was 6093 */
#define HOST_PARITY_INVALIDATED                     0x9056  /* was 6094 */
#define HOST_UNCORRECTABLE_SECTOR                   0x9057  /* was 6095 */

/* Unsolicited Codes for xPE fans & power supplies */
#define HOST_XPE_PSA_FAULTED_REMOVED                0x9060
#define HOST_XPE_PSB_FAULTED_REMOVED                0x9061
#define HOST_XPE_MULTI_FAN_FAULT                    0x9062

/* HOST_XPE_PSA_AC_POWER_FAIL is moved to 7083
  * so 0x9063 is unused. 
  * Using it for new error code HOST_XPE_SHTDN_DUE_TO_MULTI_FAN_FAULT
  */
  
#define HOST_XPE_SHTDN_DUE_TO_MULTI_FAN_FAULT                    0x9063
#define HOST_XPE_PSB_AC_POWER_FAIL_UNUSED           0x9064 /* moved to 7084 */

#define HOST_ENCL_TYPE_RG_TYPE_INCOMPATIBLE         0x9065

/* Cache not enabled, cache dirty */
#define HOST_WRITE_CACHE_DIRTY_UNITS                0x9066

/* PCI memory card defines - ACADIA */
#define HOST_PCIMEM_DMA_FAILURE                     0x9067
#define HOST_PCIMEM_CLEAR_CACHE_FAILED              0x9068
#define HOST_PCIMEM_NO_CARD_PRESENT                 0x9069
#define HOST_PCIMEM_WRONG_CARD                      0x906A
#define HOST_PCIMEM_NO_MEMORY                       0x906B
#define HOST_PCIMEM_POST_TEST_FAILED                0x906C
#define HOST_PCIMEM_CARD_FAILURE                    0x906D
#define HOST_PCIMEM_BATTERY_CHARGE_FAILURE          0x906E

/* Piranha Fan and Power supply error codes */
#define HOST_SYS_FAN_1_FAULTED                      0x9070
#define HOST_SYS_FAN_2_FAULTED                      0x9071
#define HOST_SYS_FAN_3_FAULTED                      0x9072
#define HOST_SYS_FAN_4_FAULTED                      0x9073
#define HOST_SYS_FAN_5_FAULTED                      0x9074
#define HOST_CPU_FAN_6_FAULTED                      0x9075
#define HOST_CPU_FAN_7_FAULTED                      0x9076
#define HOST_POWER_SUPPLY_A_FAULTED                 0x9077
#define HOST_POWER_SUPPLY_B_FAULTED                 0x9078
#define HOST_RESUME_DB_SERIALNUM_READ_FAILED        0x9079

/* Unsolicited codes related to enabling/disabling ATA drive */
/* write cache.*/
#define HOST_ATA_WR_CACHE_ENA_FAILED                 0x907A
#define HOST_ATA_WR_CACHE_DIS_FAILED                 0x907B

/* To log disk powered down info as an error */
#define HOST_DISK_POWERED_DOWN_INFO                  0x907C

/* To log power fault as an error for Yosemite hardware */
#define HOST_POWER_SUPPLY_YOSEMITE_A_FAULTED         0x907D
#define HOST_POWER_SUPPLY_YOSEMITE_B_FAULTED         0x907E

/* Additional Unsolicited Codes for Hammer xPE fans & power supplies */
#define HOST_XPE_PEER_PS_FAULTED_REMOVED             0x907F

/* Additional Unsolicited Codes for Hammer xPE fans & power supplies */
#define HOST_LCC_ACK_LONG_TIME_OUT_INFO              0x9080
#define HOST_LCC_LARGEST_ACK_TIMEOUT_INFO            0x9081

#define HOST_ENCL_DRIVE_TYPE_MISMATCH                0x9082

/*
 *Unsolicited Codes for Back End SFPs (Error)
*/
#define HOST_SFP_FAULTED                             0x9083
#define HOST_SFP_UNQUALIFIED                         0x9084

/* Zero map page and header errors */
#define HOST_UNIT_ZERO_MAP_PAGE_ERROR                0x9085
#define HOST_UNIT_ZERO_MAP_HEADER_ERROR              0x9086

/* Additional Unsolicited Codes for Hammer power supplies */
#define HOST_XPE_PEER_PS_STATE_UNKNOWN               0x9087
#define HOST_XPE_PSA0_FAULTED_REMOVED                0x9088
#define HOST_XPE_PSA1_FAULTED_REMOVED                0x9089
#define HOST_XPE_PSB0_FAULTED_REMOVED                0x908A
#define HOST_XPE_PSB1_FAULTED_REMOVED                0x908B

/* Unsolicited Codes for Hammerhead speed step (geyserville) */
#define HOST_SP_CPU_SPEED_STEP_DOWN_REBOOT           0x908C

/* Code for recommending proactive sparing. */
#define HOST_PROACTIVE_COPY_TO_HOTSPARE_RECOMMENDED  0x908D

/* Codes for IO carriers. */
#define HOST_IO_CARRIER_A_FAULTED                    0x908E
#define HOST_IO_CARRIER_B_FAULTED                    0x908F


#define HOST_IO_PORT_FAULTED                         0x9090
#define HOST_IO_MODULE_FAULTED                       0x9091
#define HOST_IO_MODULE_INCORRECT                     0x9092
#define HOST_ENV_INTERFACE_FAILED                    0x9093

/* If a Green/Power Saving Drive spinup failed */
#define HOST_DRIVE_SPINUP_FAILED                     0x9094
#define HOST_LCC_STATS_SUSPECT_BE_COMPONENT          0x9095

/* Zodiac magnum suitcase relavent error codes
*/
#define HOST_SP_SHUT_DOWN                            0X9096
#define HOST_DETECTED_ENCL_AMBIENT_OVERTEMP          0X9097

#define HOST_MCU_HOST_RESET_ERROR                    0x9098
#define HOST_MCU_HOST_RESET_ERROR_WITH_FRU_PART_NUM  0x9099

/* Code for can't assign temporary cache dirty luns */
#define HOST_TEMPORARY_CANT_ASSIGN_CACHE_DIRTY_UNIT  0x909A


/* This code skips ulog and generates the event directly through 
 * KLogWriteEntry.Added here just to reserve the number.  */
//#define HOST_FAILED_TO_GET_DRIVE_HANDLE_FOR_SLOT  0x909B

#define HOST_PS_MARGIN_TEST_FAIL_BAD_PS              0X909D
#define HOST_PS_MARGIN_TEST_FAIL                     0X909E
#define HOST_UNSUPPORTED_HW_FOR_THIS_PLATFORM        0X909F

/* DAE expander fault error code. 
 * This code skips ulog and generates the event directly through 
 * KLogWriteEntry.Added here just to reserve the number.  */
#define HOST_EXPANDER_FAULTED                        0X90A3

/*************************************************************
 * IMPORTANT * IMPORTANT * IMPORTANT * IMPORTANT * IMPORTANT *
 *************************************************************
 * REMEMBER to update catmerge\interface\k10kernelmsg.mc     *
 * file when you add a new sunburst errors greater than      *
 * 0x6000 because NT event log will retrieve the description *
 * of a specific event log message from k10kernelmsg.mc file *
 * otherwise the NT event log will not display the descrip-  *
 * ion properly.                                             *
 * Also, ulog_utils.c must be updated to get the correct     *
 * text strings in the Flare ULOG (which is still used by    *
 * the NAS version of Chameleon).                            *
 *************************************************************/

/* WARNING! See warning at top of file before making any changes or additions */

/* Fatal Unsolicited Errors */
#define HOST_FATAL_UNSOLICITED_MASK                 0xA000
#define HOST_DEAD_SCSI_BUS                          0xA002  /* unused */
#define HOST_NON_VOL_UNINITIALIZED                  0xA005
#define HOST_EGG_CRATE_SHUTDOWN                     0xA006  /* unused */
/* HOST_FRU_POWERED_DOWN message has been removed;
 * instead of that there is new event message HOST_DRIVE_TAKEN_OFFLINE;
 * which directly log into system event log.
 */
#define HOST_FRU_POWERED_DOWN                       0xA007
#define HOST_DB_SYNC_ERROR                          0xA008
#define HOST_FRU_DRIVE_TOO_SMALL                    0xA009
#define HOST_FRU_DRIVE_TOO_LARGE                    0xA00A

#define HOST_INCONSISENT_DB_DETECTED                0xA00B

#define HOST_DB_WRITE_FAILED                        0xA00C

#define HOST_PEER_CONTROLLER_REMOVED                0xA011
#define HOST_HARD_CACHE_MEMORY_ERROR                0xA012  /* unused */
#define HOST_FRU_UNSUPPORTED_DRIVE_TYPE             0xA013

#define HOST_FRU_UNSUPPORTED_DRIVE_FW               0xA016
#define HOST_FRU_UNFORMATTED_DRIVE                  0xA017

#define HOST_FRU_DRIVE_CAUSING_LOOP_FAILURE         0xA018

#define HOST_VLUT_VA_DB_RECONSTRUCTED               0xA019
/* Added for the CHECKSUM_TYPE mis-match */
#define HOST_INCORRECT_CHECKSUM_TYPE                0xA020

/* A021 and A022 are absolete as ps_rating realted code
     is removed from R30 */
#define HOST_INCORRECT_PS_WATTAGE_RATING            0xA021
#define HOST_PS_WATTAGE_RATING_UNKNOWN              0xA022
#define HOST_PEER_CONTROLLER_DOWN                   0xA023

#define HOST_FRU_DRIVE_BAD_TRANSMITTER              0xA024

/* Log a enclosure going into a faulted state */

#define HOST_ENCL_CHANGED_STATE                     0xA025

/* Rebuild Logger related error codes */
#define HOST_RBLD_LOG_EXISTS_ERR                    0xA026
#define HOST_RBLD_LOG_NOT_EXIST_ERR                 0xA027
#define HOST_RBLD_LOG_MEM_FAILED_ERR                0xA028
#define HOST_RBLD_LOG_UPDATE_ERR                    0xA029

/* Peer Boot Logging */
#define HOST_PEER_SP_POST_FAIL                      0xA02A
#define HOST_PEER_SP_POST_FAIL_2FRU                 0xA02B
#define HOST_PEER_SP_POST_FAIL_UNK                  0xA02C
#define HOST_PEER_SP_POST_HUNG                      0xA02D
#define HOST_PEER_SP_POST_FAIL_BLADE                0xA02E

/* unassigned and broken LUs detected, block commit */
#define HOST_UNASSIGNED_LU_COMMIT_BLOCKED           0xA030

/* Back End Loop Speed Change Code */
#define HOST_ENCL_SPEED_CHANGE_INCAPABLE            0xA031
#define HOST_ENCL_SPEED_CHANGE_RETRIES_EXCEEDED     0xA032

// failed a swap-out of a hot spare
#define HOST_FRU_SWAP_OUT_FAILED                    0xA033

/* Event Introduced to address the stuck Rebuild issue. */
#define HOST_DB_REBUILD_CHECKPOINT_ERROR            0xA034

/* More Peer Boot Logging */
#define HOST_PEER_SP_POST_FAIL_3FRU                 0xA035
#define HOST_PEER_SP_POST_FAIL_4FRU                 0xA036

/* Proactive sparing request failures */
#define HOST_SPARE_NOT_AVAILABLE                    0xA037
#define HOST_SPARE_DRIVE_REBUILDING                 0xA038
#define HOST_SPARE_DRIVE_PROBATIONAL                0xA039
#define HOST_SPARE_DRIVE_FAULTED                    0xA03A
#define HOST_SPARE_DRIVE_POWERING_UP                0xA03B
#define HOST_SPARE_UNSUPPORTED_RG_TYPE              0xA03C
#define HOST_SPARE_NO_USER_LUNS                     0xA03D
#define HOST_SPARE_SWAP_IS_IN_PROGRESS              0xA03E
#define HOST_SPARE_HS_ALREADY_SWAPPED_IN            0xA03F
#define HOST_PROACTIVE_SPARE_LIMIT_EXCEEDED         0xA040
#define HOST_SPARE_USER_LUN_DEGRADED_OR_BROKEN      0xA041
#define HOST_PROACTIVE_SPARE_ACTIVE_IN_RG           0xA042
#define HOST_SPARE_RETRY_REQUEST                    0xA043
#define HOST_PROACTIVE_SPARE_COMMIT_REQUIRED        0xA044
#define HOST_PROACTIVE_SPARE_SYSTEM_NOT_READY       0xA045

/*LCC/BCC microcode upgrade failures*/
#define HOST_LCC_MCODE_UPGRADE_SERIAL_READ_ERROR    0xA046
#define HOST_LCC_MCODE_UPGRADE_PEER_SP_UNRESPONSIVE 0xA047
#define HOST_LCC_MCODE_UPGRADE_FAILURE              0xA048
#define HOST_LCC_MCODE_UPGRADE_LCC_NOT_RESPONDING   0xA049

/* Proactive sparing failures */
#define HOST_SYSTEM_LUN_DEGRADED_OR_BROKEN          0xA04A

/* Raid Coherency error.
 */
#define HOST_COHERENCY_ERROR                        0xA04B

/* Raid checksum errors.
 */
#define HOST_DATA_CHECKSUM_ERROR                    0xA04C
#define HOST_PARITY_CHECKSUM_ERROR                  0xA04D

/* Timeouts can occur while reading the LCC revision */
#define HOST_LCC_TIMEOUT_ERROR                      0xA04E

// While performing swaping sanity checks, we found a internal
// database inconsistency - instead of panicing, let's just
// abort the swap operation and alert the customer to request
// support
#define HOST_FRU_SWAP_INTERNAL_DB_ERROR             0xA04F

/* When synchronizing background zero elements during initialization,
 * the peer SP had an element in an unexpected state.  Handle the 
 * state and log an event log message with a dial-home instead of
 * panicing. 
 */
#define HOST_UNIT_BACKGROUND_ZEROING_PEER_BAD_STATE 0xA050

/* LBA STamp Error.
 */
#define HOST_LBA_STAMP_ERROR                        0xA051


/* FRU Sig Error */
#define HOST_FRU_SIG_ERROR_INVALID_FRU_SLOT         0xA052
#define HOST_FRU_SIG_ERROR_INVALID_WWN_SEED         0xA053
#define HOST_FRU_SIG_ERROR_INVALID_PARTITION        0xA054
#define HOST_CPU_FAULTED                            0xA056

// This message goes to the event log directly.
#define HOST_PEER_SP_POST_FAIL_FRU_PART_NUM         0xA055

/* Commit failed because DB rebuild is in progress */
#define HOST_COMMIT_ERROR_DB_REBUILD_IN_PROGRESS    0xA057

/* Halt BGS services on unrecoverable errors */
#define HOST_BGS_HALT_SERVICE_ON_ERROR              0xA058

/********************************************************************
 * Error codes 0xB000 to 0xD0FF are reserved for Informational
 * Messages ONLY
 ********************************************************************/
/*************************************************************
 * IMPORTANT * IMPORTANT * IMPORTANT * IMPORTANT * IMPORTANT *
 *************************************************************
 * REMEMBER to update catmerge\interface\k10kernelmsg.mc     *
 * file when you add a new sunburst errors greater than      *
 * 0x6000 because NT event log will retrieve the description *
 * of a specific event log message from k10kernelmsg.mc file *
 * otherwise the NT event log will not display the descrip-  *
 * ion properly.                                             *
 * Also, ulog_utils.c must be updated to get the correct     *
 * text strings in the Flare ULOG (which is still used by    *
 * the NAS version of Chameleon).                            *
 *************************************************************/
/************ ADD NEW CODES AFTER THIS LINE *************************/

#define HOST_INFORMATION_GROUP2_MASK                   0xB000

#define HOST_LCC_EXT_REQUEST_INITIATED                 0xB001
#define HOST_LCC_EXT_REQUEST_COMPLETED_NO_ERROR        0xB002
#define HOST_LCC_EXT_REQUEST_COMPLETED_WITH_ERROR      0xB003
#define HOST_LCC_EXT_REQUEST_INVALID_PARAMETER         0xB004

#define HOST_LCC_REQ_FRUMON_ERROR                      0xB005
#define HOST_LCC_REQ_ERROR_BAD_ENCLOSURE_STATE         0xB006
#define HOST_LCC_REQ_ERROR_ENCLOSURE_NOT_SUPPORTED     0xB007
#define HOST_LCC_REQ_ERROR_NOT_ENOUGH_MEMORY           0xB008

/*For Fru power command*/
#define HOST_FRU_CMD_RESET                             0xB009
#define HOST_FRU_CMD_POWERED_OFF                       0xB00A
#define HOST_FRU_CMD_POWERED_UP                        0xB00B    
#define HOST_FRU_CMD_POWERED_CYCLE                     0xB00C

// These codes skip ulog and the events are generated directly through 
// KLogWriteEntry. The codes are reserved and considered in use. These
// are informational events to help understand boot time events.
//#define HOST_PEER_SP_SW_COMPONENT_CHANGED_32BIT       0xB00D
/* For TestXpeEnv test tool*/
#define HOST_TEST_XPE_ENV_ENABLED                      0xB00E
#define HOST_TEST_XPE_ENV_DISABLED                     0xB00F

/* Flare reports "Power supply Installed or Restored" when 
 * Power supply goes in Overtemp condition. This is required 
 * as currently the Flare does not report any changes when 
 * the PS clears the OverTemp condition.
 */

#define HOST_PS_CLEARED_AMBIENT_OVERTEMP               0xB010

#define HOST_ICA_FLASH_ERASING                         0xB011

#define HOST_SP_SHUTDOWN_INITIATED                     0xB012
#define HOST_SP_SHUTDOWN_BUTTON_PRESSED                0xB013
#define HOST_SP_SHUTDOWN_REQUEST_TO_PEER               0xB014
#define HOST_SP_SHUTDOWN_REQUEST_FROM_PEER             0xB015
#define HOST_SP_REBOOT_REQUEST_TO_PEER                 0xB016
#define HOST_SP_REBOOT_REQUEST_FROM_PEER               0xB017

#define SAS_HOTSPARE_SWAPPED_INTO_SATA                 0xB018
#define HOST_DRIVE_DOWNLOAD_STARTED                    0xB01A
/* For IO carrier A */
#define HOST_IO_CARRIER_A_INSTALLED                    0xB01B
#define HOST_IO_CARRIER_A_REMOVED                      0xB01C

/* For Management module A service port */
#define HOST_MGMT_FRU_A_SERVICE_PORT_NO_LINK           0xB01D

/* Status sent back to Navi/FCLI.
 * It is not used in the event log 
 */
#define HOST_MGMT_PORT_CONFIG_INVALID_PARAMETERS       0xB01E

/* For Management module A Management port configure command.*/
#define HOST_MGMT_FRU_A_MGMT_PORT_CONFIG_USER_INITIATED_COMMAND   0xB01F

/* */

// Request made to change the IO Port Settings
#define HOST_IO_PORT_CONFIG                            0xB020
/* For IO carrier B */
#define HOST_IO_CARRIER_B_INSTALLED                    0xB021
#define HOST_IO_CARRIER_B_REMOVED                      0xB022

/* For Management module B service port */
#define HOST_MGMT_FRU_B_SERVICE_PORT_NO_LINK           0xB023

/* For Management module B Management port configure command.*/
#define HOST_MGMT_FRU_B_MGMT_PORT_CONFIG_USER_INITIATED_COMMAND 0xB024


#define HOST_IO_MODULE_ENABLED                         0xB025 

/* Cache will log this message when it issue IORB_OP_CORRUPT_CRC.*/
#define HOST_CACHE_CORRUPT_CRC_ERROR               0xB026

#define HOST_PORT_ENABLED                              0xB027

#define HOST_MGMT_PORT_RESTORE_COMPLETED               0xB028
#define HOST_MGMT_PORT_RESTORE_FAILED                  0xB029
#define HOST_MGMT_PORT_CONFIG_COMPLETED                0xB02A
/* Status sent back to FCLI. */
#define HOST_MGMT_PORT_CONFIG_FAILED                   0xB02B
/* Status sent back to FCLI. It is not logged in Ulog or event log. */
#define HOST_MGMT_PORT_CONFIG_IN_PROGRESS              0xB02C

/* A message for sysio error*/
#define HOST_INVALID_SYSIO_RESPONSE                    0xB02D 

#define HOST_IO_PORT_CONFIGURED                        0xB02E
#define HOST_IO_PORTS_PERSISTED                        0xB02F

/* Memory persistence status log message */
#define HOST_MEMORY_PERSIST_STATUS                     0xB030 

/* For status when cache is evaluating and re-allocating persisted cache */
#define HOST_FLARE_PERSIST_STATUS                      0xB031

/* Command cannot be completed because the drive is in the standby state */
#define HOST_DRIVE_IN_STANDBY                          0xB032

#define HOST_IO_PORT_OVER_LIMIT                        0xB034
#define HOST_IO_PORTS_PHYSICAL_SUMMARY                 0xB035
/* Special case were we force AUTO Negotiation ON because of Broadcom Errata */
#define HOST_MGMT_PORT_FORCE_AUTONEG_ON                0xB036

#define HOST_CACHE_WCA_ENABLED                         0xB037

#define HOST_SMB_ACCESS_SUCCESS                0xB038
#define HOST_FRU_STABILITY_DISABLED                    0xB039 
#define HOST_FRU_STABILITY_ENABLED                     0xB03A
#define HOST_FCLI_READ_DB_PATCH_UP_STATUS              0xB03B
/* For Read only Background Verify
*/
#define HOST_READ_ONLY_BCKGRND_VERIFY_STARTED          0xB03C
#define HOST_READ_ONLY_BCKGRND_VERIFY_ABORTED          0xB03D
#define HOST_READ_ONLY_BCKGRND_VERIFY_COMPLETED        0xB03E

/* For Background Verify Avoidance
 */
#define HOST_CORRECTING_WITH_NEW_DATA                   0xB03F

 /* LUN Shrink Error Codes
 */
#define HOST_MOD_CAP_DB_WRITE_FAILURE                  0xB040
#define HOST_MOD_CAP_ILLEGAL_SIZE                      0xB041
#define HOST_EXT_MOD_CAP_STARTED                       0xB042
#define HOST_EXT_MOD_CAP_COMPLETED                     0xB043


/* LCC commit collision process error codes */
#define HOST_COMMIT_REQUEST_RECEIVED                    0xB044
#define HOST_COMMIT_STARTED                             0xB045
#define HOST_COMMIT_COMPLETED                           0xB046
#define HOST_COMMIT_REQUEST_RECEIVED_LCC_FW_ABORTING    0xB047

/* LCC Firmware upgrade enqueue process error codes */
#define HOST_LCC_FW_UPGRD_REQ_ENQUEUED_DUE_TO_DISK_FW_REQUEST  0xB048
#define HOST_LCC_FW_UPGRD_REQ_ENQUEUED_DUE_TO_COMMIT_REQUEST   0xB049

/* LCC commit collision process error codes for peer */
#define HOST_COMMIT_PEER_STARTED                               0xB04A
#define HOST_COMMIT_PEER_COMPLETED                             0xB04B

#define HOST_BGS_HALT_CMD_ISSUED                       0xB04C
#define HOST_BGS_UNHALT_CMD_ISSUED                     0xB04D

#define HOST_SLIC_UPGRADE_INIT                         0xB04E

/* Dim 170460(Event log messages for FCLI unbind and bind commands.) 
*/
#define HOST_FCLI_FRU_BIND                             0xB04F
#define HOST_FCLI_LUN_UNBIND                           0xB050


// #define HOST_REQUEST_NOT_SUPPORTED_DURING_NDU       0xB051   /* changed to soft error 80A5 */

/* Zodiac magnum suitcase relavent error codes
*/
#define HOST_DETECTED_ENCL_AMBIENT_OVERTEMP_OK                  0XB052
#define HOST_SP_CLEARED_SHUTDOWN_WARNING                        0XB053
#define HOST_SMU_FRU_PHYSICALLY_INSERTED                        0XB054
#define HOST_SMU_FRU_PHYSICALLY_REMOVED                         0XB055
#define HOST_SP_MEZZANINE_CARD_INSERTED                         0XB056
#define HOST_SP_MEZZANINE_CARD_REMOVED                          0XB057
#define HOST_SP_MEZZANINE_CARD_TYPE                             0XB058
#define HOST_MONITORING_HARDWARE_STATE_OK                       0XB059
#define HOST_MONITORING_SYSTEM_STATE_OK                         0XB05A
#define HOST_MONITORING_HARDWARE_STATE_FAULTED                  0XB05B
#define HOST_MONITORING_SYSTEM_STATE_FAULTED                    0XB05C


/* Database Does Not Match Errors */
#define HOST_DB_GLUT_MISMATCH_ERROR                     0xB05D
#define HOST_DB_RAID_GROUP_MISMATCH_ERROR               0xB05E

/* Dim 218162(Event log messages for DH AEL call home.) 
*/
#define HOST_AEL_PROACTIVE_COPY_TO_HOTSPARE_RECOMMENDED 0xB05F
#define HOST_AEL_DISK_REPLACEMENT_RECOMMENDED           0xB060

#define HOST_MCU_HOST_RESET_INFO                        0xB061

#define HOST_LCC_FW_UPGRD_PAUSED                        0xB062
#define HOST_LCC_FW_UPGRD_REQ_ENQUEUED                  0xB063
#define HOST_LCC_FW_UPGRD_RESUMED                       0xB064

/* Error code related to CFD feature.
 */
#define HOST_CFD_PUP_BLOCKED_SERIAL                     0xB065
#define HOST_CFD_PUP_BLOCKED_REASON_POSITION            0xB066
#define HOST_CFD_MISSING_DISK_SERIAL                    0xB067
#define HOST_SFP_BAD_EMC_CHKSUM                         0xB068

/* AEN Driver Unsolicited Events for Enclosure Fault Logging : 0xB069-0xB06B
 */
#define HOST_AEN_ENCL_CAME_ONLINE                   0xB069
#define HOST_AEN_ENCL_GONE_OFFLINE                  0xB06A
#define HOST_AEN_ENCL_FAULTED                       0xB06B

/* WARNING! See warning at top of file before making any changes or additions */

/* EFUP related messages */
#define HOST_FUP_STARTED                                0xB06C
#define HOST_FUP_COMPLETE                               0xB06D
#define HOST_FUP_REV                                    0xB06E
#define HOST_FUP_ALL_COMPLETE                           0xB06F
#define HOST_FUP_IMAGE_READ                             0xB070
#define HOST_FUP_RETRY                                  0xB071
#define HOST_FUP_STATE_CHANGE                           0xB072

#define HOST_CLD_FAILED_CACHE_ZERO                     0xB099
#define HOST_CLD_DIRTY_CACHE_ZERO                      0xB09A


/*
 * Temporary debug event codes for 363393.  These should be removed when the
 * root cause to that incident is found.
 */
#define HOST_FEXPORT_ICA_PERSIST_DETECTED               0xB09B
#define HOST_FLEXPORT_ICA_PERSIST_STARTED               0xB09C

/* AEN Driver Unsolicited Events for AEN event Logging : 0xB09D-0xB09F
 */
#define HOST_AEN_GENERAL_ERROR                                   0xB09D
#define HOST_AEN_DB_UPDATE                                           0xB09E
#define HOST_AEN_INVALID_LOCAL_LIST                           0xB09F
#define HOST_ENCL_SHUTDOWN_CANCELLED                          0xB0A0

/* Incompatible memory configuration */
#define HOST_MEM_CFG_INCOMPATIBLE                             0xB0A1

#define HOST_ENCLOSURE_CONNECTOR_STATUS                       0xB0A2

/* For Hitachi Ralston Peak drives */
#define HOST_DIE_RETIREMENT_START                      0xB0A3  
#define HOST_DIE_RETIREMENT_END                        0xB0A4 

/* DAE expander state change code. 
 * This code skips ulog and generates the event directly through 
 * KLogWriteEntry.Added here just to reserve the number.  */
#define HOST_EXPANDER_STATE_CHANGE                            0XB0A5

#endif /* MUST be last line in file */

/*
 * End of $Id: sunburst_errors.h,v 1.5 2000/02/18 21:38:55 fladm Exp $
 */
