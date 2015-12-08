/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved. Licensed material -- property of EMC
 * Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_peer_boot_utils.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the function related to utility function required for
 *  peer boot log and MCU reset host
 *
 * @ingroup board_mgmt_class_files
 *
 * @version
 *  05-Jan-2011: Created  Vaibhav Gaonkar
 *  28-July-2012: Chengkai Hu - Change file from board_mgmt\utils\fbe_board_mgmt_utils.c to 
*                               lib\fbe_peer_boot_utils\src\fbe_peer_boot_utils.c
 ***************************************************************************/
/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_peer_boot_utils.h"
#include "fbe/fbe_api_board_interface.h"
#include "generic_utils_lib.h"
#include "fbe/fbe_api_resume_prom_interface.h"
#include "specl_types.h"

/* This contain pre-defined fault expander code with there description,
 * subfru and timeout.
 */
fbe_peer_boot_entry_t fbe_PeerBootEntries[] =
{//  code,     description,                       state         timeout, numberOfSubFrus, replace
    {0x00, "CPU Module Fault",                  FBE_PEER_FAILURE,       0,1,  FBE_SUBFRU_CPU, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x01, "I/O Module 0 Fault",                FBE_PEER_FAILURE,       0,1,  FBE_SUBFRU_IO_0, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x02, "I/O Module 1 Fault",                FBE_PEER_FAILURE,       0,1,  FBE_SUBFRU_IO_1, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x03, "BEM Fault",                         FBE_PEER_FAILURE,       0,1,  FBE_SUBFRU_BEM, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x04, "CPU Module,I/O Module 0 Fault",     FBE_PEER_FAILURE,       0,2,  FBE_SUBFRU_CPU, FBE_SUBFRU_IO_0, FBE_SUBFRU_NONE},
    {0x05, "CPU Module,I/O Module 1 Fault",     FBE_PEER_FAILURE,       0,2,  FBE_SUBFRU_CPU, FBE_SUBFRU_IO_1, FBE_SUBFRU_NONE},
    {0x06, "CPU Module,Base Module Fault",      FBE_PEER_FAILURE,       0,2,  FBE_SUBFRU_CPU, FBE_SUBFRU_BEM, FBE_SUBFRU_NONE},
    {0x07, "CPU Module,All DIMMs Fault",        FBE_PEER_FAILURE,       0,2,  FBE_SUBFRU_CPU, FBE_SUBFRU_ALL_DIMMS, FBE_SUBFRU_NONE},
    {0x08, "All DIMMs Fault",                   FBE_PEER_FAILURE,       0,1,  FBE_SUBFRU_ALL_DIMMS, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x09, "DIMM 0 Fault",                      FBE_PEER_FAILURE,       0,1,  FBE_SUBFRU_DIMM_0, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x0A, "DIMM 1 Fault",                      FBE_PEER_FAILURE,       0,1,  FBE_SUBFRU_DIMM_1, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x0B, "DIMM 2 Fault",                      FBE_PEER_FAILURE,       0,1,  FBE_SUBFRU_DIMM_2, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x0C, "DIMM 3 Fault",                      FBE_PEER_FAILURE,       0,1,  FBE_SUBFRU_DIMM_3, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x0D, "DIMM 4 Fault",                      FBE_PEER_FAILURE,       0,1,  FBE_SUBFRU_DIMM_4, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x0E, "DIMM 5 Fault",                      FBE_PEER_FAILURE,       0,1,  FBE_SUBFRU_DIMM_5, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x0F, "DIMM 6 Fault",                      FBE_PEER_FAILURE,       0,1,  FBE_SUBFRU_DIMM_6, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x10, "DIMM 7 Fault",                      FBE_PEER_FAILURE,       0,1,  FBE_SUBFRU_DIMM_7, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x11, "DIMM 0 & 1 Fault",                  FBE_PEER_FAILURE,       0,2,  FBE_SUBFRU_DIMM_0, FBE_SUBFRU_DIMM_1, FBE_SUBFRU_NONE},
    {0x12, "DIMM 2 & 3 Fault",                  FBE_PEER_FAILURE,       0,2,  FBE_SUBFRU_DIMM_2, FBE_SUBFRU_DIMM_3, FBE_SUBFRU_NONE},
    {0x13, "DIMM 4 & 5 Fault",                  FBE_PEER_FAILURE,       0,2,  FBE_SUBFRU_DIMM_4, FBE_SUBFRU_DIMM_5, FBE_SUBFRU_NONE},
    {0x14, "DIMM 6 & 7 Fault",                  FBE_PEER_FAILURE,       0,2,  FBE_SUBFRU_DIMM_6, FBE_SUBFRU_DIMM_7, FBE_SUBFRU_NONE},
    {0x15, "Processor VRM 0 Fault",             FBE_PEER_FAILURE,       0,1,  FBE_SUBFRU_VRM_0, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x16, "Processor VRM 1 Fault",             FBE_PEER_FAILURE,       0,1,  FBE_SUBFRU_VRM_1, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x17, "Management(SAN only)Fault",         FBE_PEER_FAILURE,       0,1,  FBE_SUBFRU_MANAGEMENT, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x18, "Base Module or Midplane fuse to Base Module",       FBE_PEER_FAILURE, 0,2,  FBE_SUBFRU_BEM, FBE_SUBFRU_ENCL, FBE_SUBFRU_NONE}, /*TO BE FIXED*/
    // Entire blade needs to be replaced.
    {0x19, "Diags can't isolate the fault",     FBE_PEER_FAILURE,       0,1,  FBE_SUBFRU_BLADE, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},  // Special case. All blade frus need to be replaced.
    {0x1A, "Enclosure/Midplane Fault",          FBE_PEER_FAILURE,       0,1,  FBE_SUBFRU_ENCL, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x1B, "SFP module Fault",                  FBE_PEER_FAILURE,       0,1,  FBE_SUBFRU_SFP, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x1C, "Invalid Code",                      FBE_PEER_INVALID_CODE,  0,0,  FBE_SUBFRU_NONE, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x1D, "Invalid Code",                      FBE_PEER_INVALID_CODE,  0,0,  FBE_SUBFRU_NONE, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x1E, "Invalid Code",                      FBE_PEER_INVALID_CODE,  0,0,  FBE_SUBFRU_NONE, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x1F, "Invalid Code",                      FBE_PEER_INVALID_CODE,  0,0,  FBE_SUBFRU_NONE, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x20, "Invalid Code",                      FBE_PEER_INVALID_CODE,  0,0,  FBE_SUBFRU_NONE, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x21, "Invalid Code",                      FBE_PEER_INVALID_CODE,  0,0,  FBE_SUBFRU_NONE, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x22, "Invalid Code",                      FBE_PEER_INVALID_CODE,  0,0,  FBE_SUBFRU_NONE, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x23, "Invalid Code",                      FBE_PEER_INVALID_CODE,  0,0,  FBE_SUBFRU_NONE, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x24, "Invalid Code",                      FBE_PEER_INVALID_CODE,  0,0,  FBE_SUBFRU_NONE, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x25, "In Degraded Mode",                  FBE_PEER_SUCCESS,       0,0,  FBE_SUBFRU_NONE, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x26, "Fault in CMI path (SAN use only)",  FBE_PEER_FAILURE,       0,0,  FBE_SUBFRU_NONE, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x27, "Illegal Config(i.e.SAN in NAS enclosure)",FBE_PEER_FAILURE,  0,0,  FBE_SUBFRU_NONE, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x28, "Software image corrupt",            FBE_PEER_FAILURE,       0,0,  FBE_SUBFRU_NONE, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x29, "Can't access disks",                FBE_PEER_FAILURE,       0,0,  FBE_SUBFRU_NONE, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x2A, "Firmware update of local blade",    FBE_PEER_BOOTING,     300,1,  FBE_SUBFRU_CPU, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x2B, "Software panic/core dump",          FBE_PEER_DUMPING,   14400,0,  FBE_SUBFRU_NONE, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x2C, "Application running",               FBE_PEER_SUCCESS,       0,0,  FBE_SUBFRU_NONE, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x2D, "O/S running",                       FBE_PEER_BOOTING,   14400,0,  FBE_SUBFRU_NONE, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x2E, "Firmware update of remote FRU",     FBE_PEER_BOOTING,     300,0,  FBE_SUBFRU_NONE, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x2F, "Blade being serviced",              FBE_PEER_POST,       0,0,  FBE_SUBFRU_NONE, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x30, "Invalid Code",                      FBE_PEER_INVALID_CODE,  0,0,  FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x31, "Invalid Code",                      FBE_PEER_INVALID_CODE,  0,0,  FBE_SUBFRU_NONE, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x32, "Invalid Code",                      FBE_PEER_INVALID_CODE,  0,0,  FBE_SUBFRU_NONE, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x33, "Invalid Code",                      FBE_PEER_INVALID_CODE,  0,0,  FBE_SUBFRU_NONE, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    // Entire blade needs to be replaced
    {0x34, "POST testing entire blade",         FBE_PEER_BOOTING,     300,1,  FBE_SUBFRU_BLADE, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE}, // Special case. All blade frus need to be replaced.
    {0x35, "POST testing CPU module & I/O module 0",FBE_PEER_BOOTING, 300,2,  FBE_SUBFRU_CPU, FBE_SUBFRU_IO_0, FBE_SUBFRU_NONE},
    {0x36, "POST testing CPU module & I/O module 1",FBE_PEER_BOOTING, 300,2,  FBE_SUBFRU_CPU, FBE_SUBFRU_IO_1, FBE_SUBFRU_NONE},
    {0x37, "POST testing CPU module & Base Module",         FBE_PEER_BOOTING, 300,2,  FBE_SUBFRU_CPU, FBE_SUBFRU_BEM, FBE_SUBFRU_NONE},
    {0x38, "POST testing CPU module & all DIMMs",   FBE_PEER_BOOTING, 300,2,  FBE_SUBFRU_CPU, FBE_SUBFRU_ALL_DIMMS, FBE_SUBFRU_NONE},
    {0x39, "POST testing CPU module ",              FBE_PEER_BOOTING, 300,1,  FBE_SUBFRU_CPU, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x3A, "POST operation complete",               FBE_PEER_BOOTING, 600,0,  FBE_SUBFRU_NONE, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x3B, "BIOS testing CPU module & I/O module 0",FBE_PEER_BOOTING, 300,2,  FBE_SUBFRU_CPU, FBE_SUBFRU_IO_0, FBE_SUBFRU_NONE},
    {0x3C, "BIOS testing CPU module & I/O module 1",FBE_PEER_BOOTING, 300,2,  FBE_SUBFRU_CPU, FBE_SUBFRU_IO_1, FBE_SUBFRU_NONE},
    {0x3D, "BIOS testing CPU module & Base Module",         FBE_PEER_BOOTING, 300,2,  FBE_SUBFRU_CPU, FBE_SUBFRU_BEM, FBE_SUBFRU_NONE},
    {0x3E, "BIOS testing CPU module & all DIMMs ",  FBE_PEER_BOOTING, 300,2,  FBE_SUBFRU_CPU, FBE_SUBFRU_ALL_DIMMS, FBE_SUBFRU_NONE},
    {0x3F, "BIOS testing CPU module",               FBE_PEER_BOOTING, 300,1,  FBE_SUBFRU_CPU, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x40, "I/O Module 2 Fault",                FBE_PEER_FAILURE,       0,1,  FBE_SUBFRU_IO_2, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x41, "I/O Module 3 Fault",                FBE_PEER_FAILURE,       0,1,  FBE_SUBFRU_IO_3, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x42, "I/O Module 4 Fault",                FBE_PEER_FAILURE,       0,1,  FBE_SUBFRU_IO_4, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x43, "I/O Module 5 Fault",                FBE_PEER_FAILURE,       0,1,  FBE_SUBFRU_IO_5, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x44, "Double Wide I/O Module 0 Fault",        FBE_PEER_FAILURE,   0,1,  FBE_SUBFRU_DW_IO_0, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x45, "Double Wide I/O Module 1 Fault",        FBE_PEER_FAILURE,   0,1,  FBE_SUBFRU_DW_IO_1, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x46, "POST testing CPU module & I/O module 2",FBE_PEER_BOOTING, 300,2,  FBE_SUBFRU_CPU, FBE_SUBFRU_IO_2, FBE_SUBFRU_NONE},
    {0x47, "POST testing CPU module & I/O module 3",FBE_PEER_BOOTING, 300,2,  FBE_SUBFRU_CPU, FBE_SUBFRU_IO_3, FBE_SUBFRU_NONE},
    {0x48, "POST testing CPU module & I/O module 4",FBE_PEER_BOOTING, 300,2,  FBE_SUBFRU_CPU, FBE_SUBFRU_IO_4, FBE_SUBFRU_NONE},
    {0x49, "POST testing CPU module & I/O module 5",FBE_PEER_BOOTING, 300,2,  FBE_SUBFRU_CPU, FBE_SUBFRU_IO_5, FBE_SUBFRU_NONE},
    {0x4A, "POST testing CPU module & Double Wide I/O module 0",FBE_PEER_BOOTING, 300,2,  FBE_SUBFRU_CPU, FBE_SUBFRU_DW_IO_0, FBE_SUBFRU_NONE},
    {0x4B, "POST testing CPU module & Double Wide I/O module 1",FBE_PEER_BOOTING, 300,2,  FBE_SUBFRU_CPU, FBE_SUBFRU_DW_IO_1, FBE_SUBFRU_NONE},
    {0x4C, "BIOS testing CPU module & I/O module 2",FBE_PEER_BOOTING, 300,2,  FBE_SUBFRU_CPU, FBE_SUBFRU_IO_2, FBE_SUBFRU_NONE},
    {0x4D, "BIOS testing CPU module & I/O module 3",FBE_PEER_BOOTING, 300,2,  FBE_SUBFRU_CPU, FBE_SUBFRU_IO_3, FBE_SUBFRU_NONE},
    {0x4E, "BIOS testing CPU module & I/O module 4",FBE_PEER_BOOTING, 300,2,  FBE_SUBFRU_CPU, FBE_SUBFRU_IO_4, FBE_SUBFRU_NONE},
    {0x4F, "BIOS testing CPU module & I/O module 5",FBE_PEER_BOOTING, 300,2,  FBE_SUBFRU_CPU, FBE_SUBFRU_IO_5, FBE_SUBFRU_NONE},
    {0x50, "BIOS testing CPU module & Double Wide I/O module 0",FBE_PEER_BOOTING, 300,2,  FBE_SUBFRU_CPU, FBE_SUBFRU_DW_IO_0, FBE_SUBFRU_NONE},
    {0x51, "BIOS testing CPU module & Double Wide I/O module 1",FBE_PEER_BOOTING, 300,2,  FBE_SUBFRU_CPU, FBE_SUBFRU_DW_IO_1, FBE_SUBFRU_NONE},
    {0x52, "POST testing Base Module & I/O module 4",  FBE_PEER_BOOTING, 300,2,  FBE_SUBFRU_BEM, FBE_SUBFRU_IO_4, FBE_SUBFRU_NONE},
    {0x53, "POST testing Base Module & I/O module 5",  FBE_PEER_BOOTING, 300,2,  FBE_SUBFRU_BEM, FBE_SUBFRU_IO_5, FBE_SUBFRU_NONE},
    {0x54, "POST testing Base Module & I/O module 4,5",FBE_PEER_BOOTING, 300,3,  FBE_SUBFRU_BEM, FBE_SUBFRU_IO_4, FBE_SUBFRU_IO_5},
    {0x55, "POST testing CPU, Base Module & I/O module 4",FBE_PEER_BOOTING, 300,3,  FBE_SUBFRU_CPU, FBE_SUBFRU_BEM, FBE_SUBFRU_IO_4},
    {0x56, "POST testing CPU, Base Module & I/O module 5",FBE_PEER_BOOTING, 300,3,  FBE_SUBFRU_CPU, FBE_SUBFRU_BEM, FBE_SUBFRU_IO_5},
    {0x57, "POST testing CPU, Base Module & I/O module 4, 5",FBE_PEER_BOOTING, 300,4,  FBE_SUBFRU_CPU, FBE_SUBFRU_BEM, FBE_SUBFRU_IO_4}, //NEED 4 FRUS IDENTIFIED
    {0x58, "POST testing CPU module & Management A",FBE_PEER_BOOTING, 300,2,  FBE_SUBFRU_CPU, FBE_SUBFRU_MANAGEMENTA, FBE_SUBFRU_NONE},
    {0x59, "POST testing CPU module & Management B",  FBE_PEER_BOOTING, 300,2,  FBE_SUBFRU_CPU, FBE_SUBFRU_MANAGEMENTB, FBE_SUBFRU_NONE},
    {0x5A, "POST testing CPU module & Management A,B",FBE_PEER_BOOTING, 300,3,  FBE_SUBFRU_CPU, FBE_SUBFRU_MANAGEMENTA, FBE_SUBFRU_MANAGEMENTB},
    {0x5B, "Compact Flash Faulted",             FBE_PEER_FAILURE,       0,1,  FBE_SUBFRU_CF, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x5C, "POST testing Compact Flash Module", FBE_PEER_BOOTING,     300,1,  FBE_SUBFRU_CF, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x5D, "BIOS testing Compact Flash Module", FBE_PEER_BOOTING,     300,1,  FBE_SUBFRU_CF, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x5E, "POST testing Compact Flash & CPU module",FBE_PEER_BOOTING, 300,2, FBE_SUBFRU_CF, FBE_SUBFRU_CPU, FBE_SUBFRU_NONE},
    // Entire blade needs to be replaced.
    {0x5F, "POST testing Entire CPU module assembly",FBE_PEER_BOOTING, 600,1,  FBE_SUBFRU_BLADE, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x60, "UDOC Faulted",                      FBE_PEER_FAILURE,       0,1,  FBE_SUBFRU_UDOC, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x61, "POST testing UDOC",                 FBE_PEER_BOOTING,     300,1,  FBE_SUBFRU_UDOC, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x62, "BIOS testing UDOC",                 FBE_PEER_BOOTING,     300,1,  FBE_SUBFRU_UDOC, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x63, "POST testing CPU module & VRM 0",   FBE_PEER_BOOTING,     300,2,  FBE_SUBFRU_CPU, FBE_SUBFRU_VRM_0, FBE_SUBFRU_NONE},
    {0x64, "POST testing CPU module & VRM 1",   FBE_PEER_BOOTING,     300,2,  FBE_SUBFRU_CPU, FBE_SUBFRU_VRM_1, FBE_SUBFRU_NONE},
    {0x65, "POST testing CPU module & VRM 0, 1",FBE_PEER_BOOTING,     300,3,  FBE_SUBFRU_CPU, FBE_SUBFRU_VRM_0, FBE_SUBFRU_VRM_1},
    {0x66, "CPU booted to flash",               FBE_PEER_FAILURE,       0,0,  FBE_SUBFRU_NONE, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x67, "CPU image incorrect for the current platform", FBE_PEER_FAILURE, 0,0,  FBE_SUBFRU_NONE, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x68, "CPU Module,I/O Module 2 Fault",     FBE_PEER_FAILURE,       0,2,  FBE_SUBFRU_CPU, FBE_SUBFRU_IO_2, FBE_SUBFRU_NONE},
    {0x69, "CPU Module,I/O Module 3 Fault",     FBE_PEER_FAILURE,       0,2,  FBE_SUBFRU_CPU, FBE_SUBFRU_IO_3, FBE_SUBFRU_NONE},
    {0x6A, "CPU Module,I/O Module 4 Fault",     FBE_PEER_FAILURE,       0,2,  FBE_SUBFRU_CPU, FBE_SUBFRU_IO_4, FBE_SUBFRU_NONE},
    {0x6B, "CPU Module,I/O Module 5 Fault",     FBE_PEER_FAILURE,       0,2,  FBE_SUBFRU_CPU, FBE_SUBFRU_IO_5, FBE_SUBFRU_NONE},
    {0x6C, "CPU Module,Base Module & I/O module 4",     FBE_PEER_FAILURE,       0,3,  FBE_SUBFRU_CPU, FBE_SUBFRU_BEM, FBE_SUBFRU_IO_4},
    {0x6D, "CPU Module,Base Module & I/O module 5",     FBE_PEER_FAILURE,       0,3,  FBE_SUBFRU_CPU, FBE_SUBFRU_BEM, FBE_SUBFRU_IO_5},
    {0x6E, "DIMMs 0 & 2",                       FBE_PEER_FAILURE,       0,2,  FBE_SUBFRU_DIMM_0, FBE_SUBFRU_DIMM_2, FBE_SUBFRU_NONE},
    {0x6F, "DIMMs 1 & 3",                       FBE_PEER_FAILURE,       0,2,  FBE_SUBFRU_DIMM_1, FBE_SUBFRU_DIMM_3, FBE_SUBFRU_NONE},
    {0x70, "DIMMs 4 & 6",                       FBE_PEER_FAILURE,       0,2,  FBE_SUBFRU_DIMM_4, FBE_SUBFRU_DIMM_6, FBE_SUBFRU_NONE},
    {0x71, "DIMMs 5 & 7",                       FBE_PEER_FAILURE,       0,2,  FBE_SUBFRU_DIMM_5, FBE_SUBFRU_DIMM_7, FBE_SUBFRU_NONE},
    {0x72, "DIMMs 0,1,2,3",                     FBE_PEER_FAILURE,       0,4,  FBE_SUBFRU_DIMM_0, FBE_SUBFRU_DIMM_1, FBE_SUBFRU_DIMM_2},//NEED 4 FRUS IDENTIFIED
    {0x73, "DIMMs 4,5,6,7",                     FBE_PEER_FAILURE,       0,4,  FBE_SUBFRU_DIMM_4, FBE_SUBFRU_DIMM_5, FBE_SUBFRU_DIMM_6},//NEED 4 FRUS IDENTIFIED
    {0x74, "MRC Fault Refer to Peer DMI log",   FBE_PEER_FAILURE,       0,0,  FBE_SUBFRU_NONE, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x75, "CPU, Base Module & I/O module 4, 5",FBE_PEER_FAILURE,       0,4,  FBE_SUBFRU_CPU, FBE_SUBFRU_BEM, FBE_SUBFRU_IO_4}, //NEED 4 FRUS IDENTIFIED
    {0x76, "USB Loader Start",                  FBE_PEER_BOOTING,       0,0,  FBE_SUBFRU_NONE, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x77, "USB Loader is loading POST",        FBE_PEER_BOOTING,       0,0,  FBE_SUBFRU_NONE, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x78, "USB Loader is loading Enginuity",   FBE_PEER_BOOTING,       0,0,  FBE_SUBFRU_NONE, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x79, "USB Loader Ends Successfully",      FBE_PEER_BOOTING,       0,0,  FBE_SUBFRU_NONE, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x7A, "USB Loader failed loading POST",    FBE_PEER_FAILURE,       0,1,  FBE_SUBFRU_CPU, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x7B, "USB Loader failed loading Enginuity",   FBE_PEER_FAILURE,   0,1,  FBE_SUBFRU_CPU, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x7C, "Initializing Boot Path",            FBE_PEER_BOOTING,    600,0,  FBE_SUBFRU_NONE, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0x7D, "Invalid Memory Configuration",      FBE_PEER_FAILURE,       0,0,  FBE_SUBFRU_ALL_DIMMS, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
    {0xFF, "BIOS testing CPU module",           FBE_PEER_BOOTING,    300,1,  FBE_SUBFRU_CPU, FBE_SUBFRU_NONE, FBE_SUBFRU_NONE},
};

/* This contain pre-defined CPU register code with their values.(for fault register)
 */
static fbe_peer_boot_cpu_reg_entry_t fbe_pbl_cpuRegEntries[] =
{
    //  status    timeout       state              description

    //BIOS status code                                                                                  
    { 0x00000000  , 300  , FBE_PEER_BOOTING, {{PBL_FRU_CPU,0}}}, //"BIOS MRC"                           
    { 0x00000001  , 300  , FBE_PEER_BOOTING, {{PBL_FRU_CPU,0}, {PBL_FRU_ALL_DIMM,0}}}, //"SMI Handler"  
    { 0x00000002  , 300  , FBE_PEER_BOOTING, {{PBL_FRU_CPU,0}, {PBL_FRU_SLIC,0}}},                      
    { 0x00000003  , 300  , FBE_PEER_BOOTING, {{PBL_FRU_CPU,0}, {PBL_FRU_SLIC,1}}},                      
    { 0x00000004  , 300  , FBE_PEER_BOOTING, {{PBL_FRU_CPU,0}, {PBL_FRU_SLIC,2}}},                      
    { 0x00000005  , 300  , FBE_PEER_BOOTING, {{PBL_FRU_CPU,0}, {PBL_FRU_SLIC,3}}},                      
    { 0x00000006  , 300  , FBE_PEER_BOOTING, {{PBL_FRU_CPU,0}, {PBL_FRU_SLIC,4}}},                      
    { 0x00000007  , 300  , FBE_PEER_BOOTING, {{PBL_FRU_CPU,0}, {PBL_FRU_SLIC,5}}},                      
    { 0x00000008  , 300  , FBE_PEER_BOOTING, {{PBL_FRU_CPU,0}, {PBL_FRU_SLIC,6}}},                      
    { 0x00000009  , 300  , FBE_PEER_BOOTING, {{PBL_FRU_CPU,0}, {PBL_FRU_SLIC,7}}},                      
    { 0x0000000A  , 300  , FBE_PEER_BOOTING, {{PBL_FRU_CPU,0}, {PBL_FRU_SLIC,8}}},                      
    { 0x0000000B  , 300  , FBE_PEER_BOOTING, {{PBL_FRU_CPU,0}, {PBL_FRU_SLIC,9}}},                      
    { 0x0000000C  , 300  , FBE_PEER_BOOTING, {{PBL_FRU_CPU,0}, {PBL_FRU_SLIC,10}}},                     
    { 0x0000000D  , 300  , FBE_PEER_BOOTING, {{PBL_FRU_CPU,0}, {PBL_FRU_SLIC,11}}},                     
    { 0x0000000E  , 300  , FBE_PEER_BOOTING, {{PBL_FRU_CPU,0}, {PBL_FRU_SLIC,12}}},                     
    { 0x0000000F  , 300  , FBE_PEER_BOOTING, {{PBL_FRU_CPU,0}, {PBL_FRU_SLIC,13}}},                     
    { 0x00000010  , 300  , FBE_PEER_BOOTING, {{PBL_FRU_CPU,0}, {PBL_FRU_SLIC,14}}},                     
    { 0x00000011  , 300  , FBE_PEER_BOOTING, {{PBL_FRU_CPU,0}, {PBL_FRU_SLIC,15}}},                     
    { 0x00000012  , 300  , FBE_PEER_BOOTING, {{PBL_FRU_CPU,0}, {PBL_FRU_SLIC,16}}},                     
    { 0x00000013  , 300  , FBE_PEER_BOOTING, {{PBL_FRU_CPU,0}, {PBL_FRU_SLIC,17}}},                     
    { 0x00000014  , 300  , FBE_PEER_BOOTING, {{PBL_FRU_CPU,0}, {PBL_FRU_SLIC,18}}},                     
    { 0x00000015  , 300  , FBE_PEER_BOOTING, {{PBL_FRU_CPU,0}, {PBL_FRU_SLIC,19}}},                     
    { 0x00000016  , 300  , FBE_PEER_BOOTING, {{PBL_FRU_CPU,0}, {PBL_FRU_SLIC,20}}},                     
    { 0x00000017  , 300  , FBE_PEER_BOOTING, {{PBL_FRU_CPU,0}, {PBL_FRU_SLIC,21}}},                     
    { 0x00000018  , 300  , FBE_PEER_BOOTING, {{PBL_FRU_CPU,0}, {PBL_FRU_SLIC,22}}},                     
    //POST status code                                                                                 
    { 0x01000000  , 300  , FBE_PEER_BOOTING, {{PBL_FRU_INVALID,0}}}, //"POST Initializing"              
    { 0x01000001  , 300  , FBE_PEER_BOOTING, {{PBL_FRU_INVALID,0}}}, //"POST Start"                     
    { 0x01000002  , 2700 , FBE_PEER_BOOTING, {{PBL_FRU_CPU,0}}}, //"POST Motherboard Test"             
    { 0x01000003  , 600  , FBE_PEER_BOOTING, {{PBL_FRU_SLIC,0}}}, //"POST IO Module 0 Test"             
    { 0x01000004  , 600  , FBE_PEER_BOOTING, {{PBL_FRU_SLIC,1}}}, //"POST IO Module 1 Test"             
    { 0x01000005  , 600  , FBE_PEER_BOOTING, {{PBL_FRU_SLIC,2}}}, //"POST IO Module 2 Test"             
    { 0x01000006  , 600  , FBE_PEER_BOOTING, {{PBL_FRU_SLIC,3}}}, //"POST IO Module 3 Test"             
    { 0x01000007  , 600  , FBE_PEER_BOOTING, {{PBL_FRU_SLIC,4}}}, //"POST IO Module 4 Test"             
    { 0x01000008  , 600  , FBE_PEER_BOOTING, {{PBL_FRU_SLIC,5}}}, //"POST IO Module 5 Test"             
    { 0x01000009  , 600  , FBE_PEER_BOOTING, {{PBL_FRU_SLIC,6}}}, //"POST IO Module 6 Test"             
    { 0x0100000A  , 600  , FBE_PEER_BOOTING, {{PBL_FRU_SLIC,7}}}, //"POST IO Module 7 Test"             
    { 0x0100000B  , 600  , FBE_PEER_BOOTING, {{PBL_FRU_SLIC,8}}}, //"POST IO Module 8 Test"             
    { 0x0100000C  , 600  , FBE_PEER_BOOTING, {{PBL_FRU_SLIC,9}}}, //"POST IO Module 9 Test"             
    { 0x0100000D  , 600  , FBE_PEER_BOOTING, {{PBL_FRU_SLIC,10}}}, //"POST IO Module 10 Test"           
    { 0x0100000E  , 600  , FBE_PEER_BOOTING, {{PBL_FRU_SLIC,11}}}, //"POST IO Module 11 Test"           
    { 0x0100000F  , 600  , FBE_PEER_BOOTING, {{PBL_FRU_SLIC,12}}}, //"POST IO Module 12 Test"           
    { 0x01000010  , 600  , FBE_PEER_BOOTING, {{PBL_FRU_SLIC,13}}}, //"POST IO Module 13 Test"           
    { 0x01000011  , 600  , FBE_PEER_BOOTING, {{PBL_FRU_SLIC,14}}}, //"POST IO Module 14 Test"           
    { 0x01000012  , 600  , FBE_PEER_BOOTING, {{PBL_FRU_SLIC,15}}}, //"POST IO Module 15 Test"           
    { 0x01000013  , 600  , FBE_PEER_BOOTING, {{PBL_FRU_MGMT,0}}}, //"POST Management Module A Test"     
    { 0x01000014  , 600  , FBE_PEER_BOOTING, {{PBL_FRU_MGMT,1}}}, //"POST Management Module B Test"     
    { 0x01000015  , 600  , FBE_PEER_BOOTING, {{PBL_FRU_POWER,0}}}, //"POST Power Supply A Test"         
    { 0x01000016  , 600  , FBE_PEER_BOOTING, {{PBL_FRU_POWER,1}}}, //"POST Power Supply B Test"         
    { 0x01000017  , 600  , FBE_PEER_BOOTING, {{PBL_FRU_BATTERY,0}}}, //"POST BBU A Test"                
    { 0x01000018  , 600  , FBE_PEER_BOOTING, {{PBL_FRU_BATTERY,1}}}, //"POST BBU B Test"                
    { 0x01000019  , 0    , FBE_PEER_BOOTING, {{PBL_FRU_INVALID,0}}},                                    
    { 0x0100001A  , 0    , FBE_PEER_BOOTING, {{PBL_FRU_INVALID,0}}},                                    
    { 0x0100001B  , 0    , FBE_PEER_BOOTING, {{PBL_FRU_INVALID,0}}}, //"POST End"                       
    { 0x0100001C  , 0    , FBE_PEER_FAILURE, {{PBL_FRU_INVALID,0}}}, //"POST Illegal Configuration"     
    { 0x0100001D  , 300  , FBE_PEER_FAILURE, {{PBL_FRU_INVALID,0}}}, //"POST Software Image Corrupt"    
    { 0x0100001E  , 300  , FBE_PEER_FAILURE, {{PBL_FRU_INVALID,0}}}, //"POST Cannot Access Disk"        
    { 0x0100001F  , 0    , FBE_PEER_BOOTING, {{PBL_FRU_INVALID,0}}}, //"POST Local Update In Progress"  
    { 0x01000020  , 0    , FBE_PEER_FAILURE, {{PBL_FRU_INVALID,0}}}, //"POST Software Panic"            
    { 0x01000021  , 0    , FBE_PEER_POST,    {{PBL_FRU_INVALID,0}}}, //"POST Application Running"       
    { 0x01000022  , 0    , FBE_PEER_POST,    {{PBL_FRU_INVALID,0}}}, //"POST OS Running"                
    { 0x01000023  , 0    , FBE_PEER_POST,    {{PBL_FRU_INVALID,0}}}, //"POST Remote Update in Progress" 
    { 0x01000024  , 0    , FBE_PEER_POST,    {{PBL_FRU_INVALID,0}}}, //"POST Blade Being Serviced"      
    { 0x01000025  , 300  , FBE_PEER_BOOTING, {{PBL_FRU_ALL_DIMM,0}}}, //"POST Memory Test"              
    { 0x01000026  , 300  , FBE_PEER_BOOTING, {{PBL_FRU_ONBOARD_DRIVE,0}}}, //"POST Disk 0 Test"         
    { 0x01000027  , 300  , FBE_PEER_BOOTING, {{PBL_FRU_ONBOARD_DRIVE,1}}}, //"POST Disk 1 Test"         
    { 0x01000028  , 300  , FBE_PEER_BOOTING, {{PBL_FRU_ONBOARD_DRIVE,2}}}, //"POST Disk 2 Test"         
    { 0x01000029  , 300  , FBE_PEER_BOOTING, {{PBL_FRU_ONBOARD_DRIVE,3}}}, //"POST Disk 3 Test"         
    { 0x0100002A  , 600  , FBE_PEER_BOOTING, {{PBL_FRU_CACHECARD}}}, //"POST Cache Card Test"                
    { 0x0100002B  , 600  , FBE_PEER_BOOTING, {{PBL_FRU_BM}}}, //"POST Base Module Test"                 
    { 0x0100002C  , 600  , FBE_PEER_BOOTING, {{PBL_FRU_FAN, 0}}},                                       
    { 0x0100002D  , 600  , FBE_PEER_BOOTING, {{PBL_FRU_FAN, 1}}},                                       
    { 0x0100002E  , 600  , FBE_PEER_BOOTING, {{PBL_FRU_FAN, 2}}},                                       
    { 0x0100002F  , 600  , FBE_PEER_BOOTING, {{PBL_FRU_FAN, 3}}},                                       
    { 0x01000030  , 600  , FBE_PEER_BOOTING, {{PBL_FRU_FAN, 4}}},                                       
    { 0x01000031  , 600  , FBE_PEER_BOOTING, {{PBL_FRU_FAN, 5}}},                                       
    //OS status code                                                                                    
    { 0x02000000  , 0    , FBE_PEER_DEGRADED,{{PBL_FRU_INVALID,0}}}, //"Degraded Mode"                  
    { 0x02000001  , 0    , FBE_PEER_DUMPING, {{PBL_FRU_INVALID,0}}}, //"Core Dumping"                   
    { 0x02000002  , 0    , FBE_PEER_SUCCESS, {{PBL_FRU_INVALID,0}}}, //"Application Running"            
    { 0x02000003  , 0    , FBE_PEER_SW_LOADING, {{PBL_FRU_INVALID,0}}}, //"OS Running"                  
    { 0x02000004  , 0    , FBE_PEER_SUCCESS, {{PBL_FRU_INVALID,0}}}, //"OS Blade Being Serviced"        
    { 0x02000005  , 0    , FBE_PEER_FAILURE, {{PBL_FRU_INVALID,0}}},  //"Illegal Memory Configuration"   
    { 0xFFFFFFFF  , 0    , FBE_PEER_UNKNOWN, {{PBL_FRU_INVALID,0}}}  //"BMC Reset Value"   
};

/*!**************************************************************
 * fbe_board_mgmt_get_device_type_and_location()
 ****************************************************************
 * @brief
 *  This function return device_location and device_type depend upon
 *  resume prom ID.
 *
 * @param sp - Sp id
 * @param subFru - SubFru
 * @param device_location - Ptr to fbe_device_physical_location_t
 * @param device_type - Ptr to device_type
 *
 * @return fbe_pe_resume_prom_id - Resume prom ID.
 *
 * @author
 *  05-Jan-2011: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
fbe_status_t
fbe_board_mgmt_get_device_type_and_location(SP_ID sp,
                                            fbe_subfru_replacements_t subFru,
                                            fbe_device_physical_location_t *device_location,
                                            fbe_u64_t *device_type)
{
    fbe_status_t status = FBE_STATUS_OK;

    switch(subFru)
    {
        case FBE_SUBFRU_ENCL:
            device_location->slot = 0;
            device_location->sp = sp;
            *device_type = FBE_DEVICE_TYPE_ENCLOSURE;
            break;

        case FBE_SUBFRU_CPU:
            device_location->slot = 0;
            device_location->sp = sp;
            *device_type = FBE_DEVICE_TYPE_SP;
            break;

        case FBE_SUBFRU_MANAGEMENT:
        case FBE_SUBFRU_MANAGEMENTA:
        case FBE_SUBFRU_MANAGEMENTB:
            device_location->slot = 0;
            device_location->sp = sp;
            *device_type = FBE_DEVICE_TYPE_MGMT_MODULE;
            break;

        case FBE_SUBFRU_BEM:
            device_location->slot = 0;
            device_location->sp = sp;
            *device_type = FBE_DEVICE_TYPE_BACK_END_MODULE;
            break;

        case FBE_SUBFRU_IO_0:
            device_location->slot = 0;
            device_location->sp = sp;
            *device_type = FBE_DEVICE_TYPE_IOMODULE;
            break;

        case FBE_SUBFRU_IO_1:
            device_location->slot = 1;
            device_location->sp = sp;
            *device_type = FBE_DEVICE_TYPE_IOMODULE;
            break;

        case FBE_SUBFRU_IO_2:
            device_location->slot = 2;
            device_location->sp = sp;
            *device_type = FBE_DEVICE_TYPE_IOMODULE;
            break;

        case FBE_SUBFRU_IO_3:
            device_location->slot = 3;
            device_location->sp = sp;
            *device_type = FBE_DEVICE_TYPE_IOMODULE;
            break;

        case FBE_SUBFRU_IO_4:
            device_location->slot = 4;
            device_location->sp = sp;
            *device_type = FBE_DEVICE_TYPE_IOMODULE;
            break;

        case FBE_SUBFRU_IO_5:
            device_location->slot = 5;
            device_location->sp = sp;
            *device_type = FBE_DEVICE_TYPE_IOMODULE;
            break;

        default:
            *device_type = FBE_DEVICE_TYPE_INVALID;
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    return status;
}
/******************************************************
* end of fbe_board_mgmt_get_device_type_and_location()
******************************************************/


/*!**************************************************************
 * fbe_board_mgmt_utils_get_suFruMessage()
 ****************************************************************
 * @brief
 *  This function return string for subFru
 *
 * @param subFru - Sub Fru replacement
 *
 * @return fbe_u8_t - Ptr to sub Fru string.
 *
 * @author
 *  24-Jan-2011: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
fbe_u8_t *
fbe_board_mgmt_utils_get_suFruMessage(fbe_subfru_replacements_t subFru)
{
    fbe_u8_t *subFruString;

    switch(subFru)
    {
        case FBE_SUBFRU_NONE:
            subFruString = "NONE";
            break;

        case FBE_SUBFRU_CPU:
            subFruString = "CPU Module";
            break;

        case FBE_SUBFRU_IO_0:
            subFruString = "IO Module0";
            break;

        case FBE_SUBFRU_IO_1:
            subFruString = "IO Module1";
            break;

        case FBE_SUBFRU_IO_2:
            subFruString = "IO Module2";
            break;

        case FBE_SUBFRU_IO_3:
            subFruString = "IO Module3";
            break;

        case FBE_SUBFRU_IO_4:
            subFruString = "IO Module4";
            break;

        case FBE_SUBFRU_IO_5:
            subFruString = "IO Module5";
            break;

        case FBE_SUBFRU_BEM:
            subFruString = "Base Module";
            break;

        case FBE_SUBFRU_ALL_DIMMS:
            subFruString = "All DIMMs";
            break;

        case FBE_SUBFRU_DIMM_0:
            subFruString = "DIMM 0";
            break;

        case FBE_SUBFRU_DIMM_1:
            subFruString = "DIMM 1";
            break;

        case FBE_SUBFRU_DIMM_2:
            subFruString = "DIMM 2";
            break;

        case FBE_SUBFRU_DIMM_3:
            subFruString = "DIMM 3";
            break;

        case FBE_SUBFRU_DIMM_4:
            subFruString = "DIMM 4";
            break;

        case FBE_SUBFRU_DIMM_5:
            subFruString = "DIMM 5";
            break;

        case FBE_SUBFRU_DIMM_6:
            subFruString = "DIMM 6";
            break;

        case FBE_SUBFRU_DIMM_7:
            subFruString = "DIMM 7";
            break;

        case FBE_SUBFRU_VRM_0:
            subFruString = "VRM 0";
            break;

        case FBE_SUBFRU_VRM_1:
            subFruString = "VRM 1";
            break;

        case FBE_SUBFRU_MANAGEMENT:
            subFruString = "Management Module";
            break;

        case FBE_SUBFRU_ENCL:
            subFruString = "Midplane";
            break;

        case FBE_SUBFRU_SFP:
            subFruString = "SFP Module";
            break;

        case FBE_SUBFRU_CACHE_CARD:
            subFruString = "Cache Card";
            break;

        case FBE_SUBFRU_MANAGEMENTA:
            subFruString = "Management A";
            break;

        case FBE_SUBFRU_MANAGEMENTB:
            subFruString = "Management B";
            break;

        case FBE_SUBFRU_CF:
            subFruString = "Compact Flash";
            break;

        case FBE_SUBFRU_UDOC:
            subFruString = "UDOC";
            break;

        case FBE_SUBFRU_BLADE:
            subFruString = "BLADE";
            break;

        case FBE_SUBFRU_DW_IO_0:
            subFruString = "Double Wide IO Module0";
            break;

        case FBE_SUBFRU_DW_IO_1:
            subFruString = "Double Wide IO Module1";
            break;

        default:
            subFruString = "Unknown";
            break;
    }

    return subFruString;
}
/*!**************************************************************
 * fbe_pbl_decodeFru()
 ****************************************************************
 * @brief
 *  This function decode fru type into fru string.
 *
 * @param  
 *        fru - fru type to decode. 
 * @return N/A 
 *        fru string.
 * @author
 *  21-Feb-2013: Chengkai Hu - Created
 *  29-Oct-2013: Chengkai Hu - Add onboard drive and all dimm.
 *
 ****************************************************************/
const char *
fbe_pbl_decodeFru(fbe_peer_boot_fru_t  fru)
{

    switch(fru)
    {
        case PBL_FRU_DIMM:
            return "DIMM";
            break;
        case PBL_FRU_DRIVE:
            return "Drive";
            break;
        case PBL_FRU_SLIC:        
            return "IO Module";
            break;
        case PBL_FRU_POWER:       
            return "Power Supply";
            break;
        case PBL_FRU_BATTERY:     
            return "Battery";
            break;
        case PBL_FRU_SUPERCAP:    
            return "SuperCap";
            break;
        case PBL_FRU_FAN:         
            return "Fan";
            break;
        case PBL_FRU_I2C:         
            return "I2C";
            break;
        case PBL_FRU_CPU:         
            return "CPU";
            break;
        case PBL_FRU_MGMT:        
            return "Management Module";
            break;
        case PBL_FRU_BM:
            return "Base Module";
            break ;
        case PBL_FRU_EFLASH:      
            return "eFlash";
            break;
        case PBL_FRU_CACHECARD:     
            return "Cache Card";
            break;
        case PBL_FRU_MIDPLANE:    
            return "Midplane";
            break;
        case PBL_FRU_CMI:         
            return "CMI";
            break;
        case PBL_FRU_ALL_FRU:     
            return "All FRUs";
            break;
        case PBL_FRU_EXTERNAL_FRU:
            return "External FRU";
            break;
        case PBL_FRU_ALL_DIMM:
            return "All DIMM";
            break;
        case PBL_FRU_ONBOARD_DRIVE:
            return "Onboard drive";
            break;
        default:
            return "Unkown FRU";
            break;
    }
    
    return "Unkown FRU";
}
/***********************************************************
* end of fbe_pbl_decodeFru()
***********************************************************/
/*!**************************************************************
 * fbe_pbl_mapFbeDeviceType()
 ****************************************************************
 * @brief
 *  This function map fru type to fbe device type.
 *
 * @param  
 *        fru - fru type to map. 
 * @return N/A 
 *        fbe device type.
 * @author
 *  5-Nov-2013: Chengkai Hu - Created
 *
 ****************************************************************/
fbe_u64_t
fbe_pbl_mapFbeDeviceType(fbe_peer_boot_fru_t  fru)
{

    switch(fru)
    {
        case PBL_FRU_DRIVE:
            return FBE_DEVICE_TYPE_DRIVE;
            break;
        case PBL_FRU_SLIC:        
            return FBE_DEVICE_TYPE_IOMODULE;
            break;
        case PBL_FRU_POWER:       
            return FBE_DEVICE_TYPE_PS;
            break;
        case PBL_FRU_BATTERY:     
            return FBE_DEVICE_TYPE_BATTERY;
            break;
        case PBL_FRU_FAN:         
            return FBE_DEVICE_TYPE_FAN;
            break;
        case PBL_FRU_CPU:
            return FBE_DEVICE_TYPE_SP;
            break;
        case PBL_FRU_MGMT:        
            return FBE_DEVICE_TYPE_MGMT_MODULE;
            break;
        case PBL_FRU_BM:
            return FBE_DEVICE_TYPE_BACK_END_MODULE;
            break ;
        case PBL_FRU_MIDPLANE:
            return FBE_DEVICE_TYPE_ENCLOSURE;
            break;
        default:
            return FBE_DEVICE_TYPE_INVALID;
            break;
    }
    
    return FBE_DEVICE_TYPE_INVALID;
}
/***********************************************************
* end of fbe_pbl_mapFbeDeviceType()
***********************************************************/
/*!**************************************************************
 * fbe_pbl_BootPeerSuccessful()
 ****************************************************************
 * @brief
 *  Determine if Peer Boot was successful
 *
 * @param bootState - boot state code
 * @return Boot successful
 * 
 * @author
 *  19-Nov-2012 Randall Porteus - Created
 *
 ****************************************************************/
fbe_bool_t
fbe_pbl_BootPeerSuccessful(fbe_peer_boot_states_t      bootState)
{
    if (bootState == FBE_PEER_SUCCESS)
    {
        return TRUE;
    }
    else
        return FALSE;
}
/************************************************
* end of fbe_pbl_BootPeerSuccessful()
************************************************/
/*!**************************************************************
 * fbe_pbl_BootPeerUnknown()
 ****************************************************************
 * @brief
 *  Determine if Peer Boot is Unknown
 *
 * @param bootState - boot state code
 * @return Boot Unknown
 * 
 * @author
 *  19-Nov-2012 Randall Porteus - Created
 *
 ****************************************************************/
fbe_bool_t
fbe_pbl_BootPeerUnknown(fbe_peer_boot_states_t      bootState)
{
    if (bootState == FBE_PEER_UNKNOWN)
    {
        return TRUE;
    }
    else
        return FALSE;
}
/************************************************
* end of fbe_pbl_BootPeerUnknown()
************************************************/
/*!**************************************************************
 * fbe_pbl_decodeBootState()
 ****************************************************************
 * @brief
 *  Decodes boot state code
 *
 * @param bootState - boot state code
 * @return string of boot state
 * 
 * @author
 *  15-Aug-2012 Chengkai Hu - Created
 *
 ****************************************************************/
const char*
fbe_pbl_decodeBootState(fbe_peer_boot_states_t      bootState)
{
    const char* rtnStr = "blank";

    switch (bootState)
    {
        case FBE_PEER_SUCCESS:
            rtnStr = "Boot successfully";
        break;
        case FBE_PEER_DEGRADED:
            rtnStr = "Degraded";
        break;
        case FBE_PEER_BOOTING:
            rtnStr = "Booting";
        break;
        case FBE_PEER_FAILURE:
            rtnStr = "Boot failed";
        break;
        case FBE_PEER_POST:
            rtnStr = "Held in POST";
        break;
        case FBE_PEER_HUNG:
            rtnStr = "Boot hung";
        break;
        case FBE_PEER_DUMPING:
            rtnStr = "Dumping";
        break;
        case FBE_PEER_UNKNOWN:
            rtnStr = "Unknown boot state";
        break;
        case FBE_PEER_SW_LOADING:
            rtnStr = "Software loading";
        break;
        case FBE_PEER_DRIVER_LOAD_ERROR:
            rtnStr = "Driver load error";
        break;
        case FBE_PEER_INVALID_CODE:
        default:
            rtnStr = "Invalid boot state code";
        break;
    }
    return (rtnStr);
}
/***********************************************************
* end of fbe_pbl_decodeBootState()
***********************************************************/
/*!**************************************************************
 * fbe_pbl_getPeerBootState()
 ****************************************************************
 * @brief
 *  This function get peer boot status from fault register.
 *
 * @param  
 *        fltRegStatus - This is fault register handle
 * @return N/A 
 * 
 * @author
 *  28-July-2012: Chengkai Hu - Created
 *
 ****************************************************************/
fbe_peer_boot_states_t
fbe_pbl_getPeerBootState(SPECL_FAULT_REGISTER_STATUS  *fltRegStatus)
{
    fbe_peer_boot_states_t  status = FBE_PEER_INVALID_CODE;
    fbe_u32_t               statusCount = 0, eachStatus = 0;
    
    if (NULL == fltRegStatus)
    {
        return status;
    }
      
    statusCount = sizeof(fbe_pbl_cpuRegEntries)/sizeof(fbe_peer_boot_cpu_reg_entry_t);

    for(eachStatus=0; eachStatus < statusCount; eachStatus++)
    {
        if (fbe_pbl_cpuRegEntries[eachStatus].status == fltRegStatus->cpuStatusRegister)
        {
            return fbe_pbl_cpuRegEntries[eachStatus].bootState;
        }
    }
    return status;
}
/***********************************************************
* end of fbe_pbl_getPeerBootState()
***********************************************************/
/*!**************************************************************
 * fbe_pbl_getPeerBootTimeout()
 ****************************************************************
 * @brief
 *  This function get peer boot timeout value from cpu register code.
 *
 * @param  
 *        fltRegStatus - This includes status code in CPU register 
 * @return N/A 
 *        timeout value in second
 * @author
 *  18-Feb-2013: Chengkai Hu - Created
 *
 ****************************************************************/
fbe_u32_t
fbe_pbl_getPeerBootTimeout(SPECL_FAULT_REGISTER_STATUS  *fltRegStatus)
{
    fbe_u32_t       statusCount = 0, eachStatus = 0;
    fbe_u32_t       timeout = 0;
      
    if (NULL == fltRegStatus)
    {
        return timeout;
    }
        
    statusCount = sizeof(fbe_pbl_cpuRegEntries)/sizeof(fbe_peer_boot_cpu_reg_entry_t);

    for(eachStatus=0; eachStatus < statusCount; eachStatus++)
    {
        if (fbe_pbl_cpuRegEntries[eachStatus].status == fltRegStatus->cpuStatusRegister)
        {
            return fbe_pbl_cpuRegEntries[eachStatus].timeout;
        }
    }
    return timeout;
}
/***********************************************************
* end of fbe_pbl_getPeerBootTimeout()
***********************************************************/
/*!**************************************************************
 * fbe_pbl_getBadFruNum()
 ****************************************************************
 * @brief
 *  This function get number of bad FRU by peer boot status.
 *
 * @param  
 *        status - peer boot status code in cpu register
 * @return N/A 
 *        number of bad fru 
 * @author
 *  29-Oct-2013: Chengkai Hu - Created
 *
 ****************************************************************/
fbe_u32_t
fbe_pbl_getBadFruNum(fbe_u32_t cpuStatus)
{
    fbe_u32_t       statusCount = 0, eachStatus = 0;
    fbe_u32_t       fruCount = 0, eachFru = 0;
    fbe_u32_t       number = 0;
      
    statusCount = sizeof(fbe_pbl_cpuRegEntries)/sizeof(fbe_peer_boot_cpu_reg_entry_t);

    for(eachStatus=0; eachStatus < statusCount; eachStatus++)
    {
        if (fbe_pbl_cpuRegEntries[eachStatus].status == cpuStatus)
        {
            fruCount = sizeof(fbe_pbl_cpuRegEntries[eachStatus].badFru)/sizeof(fbe_peer_boot_bad_fru_t);
            for(eachFru=0; eachFru < fruCount; eachFru++)
            {
                if (fbe_pbl_cpuRegEntries[eachStatus].badFru[eachFru].type != PBL_FRU_INVALID)
                {
                    number++;
                }
            }
            return number;
        }
    }
    return number;
}
/***********************************************************
* end of fbe_pbl_getBadFruNum()
***********************************************************/
/*!**************************************************************
 * fbe_pbl_getBadFru()
 ****************************************************************
 * @brief
 *  This function get bad FRU by peer boot status and index of bad FRU.
 *
 * @param  
 *        status - peer boot status code in cpu register
 * @return N/A 
 *        number of bad fru 
 * @author
 *  6-Nov-2013: Chengkai Hu - Created
 *
 ****************************************************************/
fbe_status_t
fbe_pbl_getBadFru(fbe_u32_t                 cpuStatus,
                  fbe_u32_t                 fruIndex,
                  fbe_peer_boot_bad_fru_t * badFru)
{
    fbe_u32_t statusCount = 0, eachStatus = 0;

    if (fruIndex > FBE_MAX_SUBFRU_REPLACEMENTS-1 || NULL == badFru)
    {
        return FBE_STATUS_FAILED;
    }
    statusCount = sizeof(fbe_pbl_cpuRegEntries)/sizeof(fbe_peer_boot_cpu_reg_entry_t);

    for(eachStatus=0; eachStatus < statusCount; eachStatus++)
    {
        if (fbe_pbl_cpuRegEntries[eachStatus].status == cpuStatus)
        {
            *badFru = fbe_pbl_cpuRegEntries[eachStatus].badFru[fruIndex];
            return FBE_STATUS_OK;
        }
    }
    return FBE_STATUS_FAILED;
}
/***********************************************************
* end of fbe_pbl_getBadFru()
***********************************************************/

