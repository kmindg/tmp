//***************************************************************************
// Copyright (C) EMC Corporation 2009
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//++
// File Name:
//      K10MiniportInfo.h
//
// Contents:
//      Defines the structure of the MINIPORT section of NVRAM. 
//      Note that this section is currently written to by SPECL and will
//      contain information about the IO devices attached to the system.
//
// Revision History:
//      03/04/09    Joe Ash       Created.
//
//--
#ifndef _K10_MINIPORT_INFORMATION_H_
#define _K10_MINIPORT_INFORMATION_H_

#include "generic_types.h"
#include "specl_types.h"

// the signature of the section
#define MINIPORT_INFO_SIGNATURE       "MINIPORT INFO"

#pragma pack(1)

//++
// Type:
//      NV_MINIPORT_INFO
//
// Description:
//      A definition for the entire miniport info section in NVRAM.
//
// Revision History:
//      03/04/09    Joe Ash       Created.
//
//--
typedef struct
{
    UINT_8E                             signature[16];
    ULONG                               length;
    PVOID                               cpd_dump_callback_ptr;
    SPID_HW_TYPE                        platform_type;
    ULONG                               midplaneWWNSeed;
    PSPECL_PS_SUMMARY                   ps_summary_ptr;
    PSPECL_BATTERY_SUMMARY              battery_summary_ptr;
    PSPECL_SUITCASE_SUMMARY             suitcase_summary_ptr;
    PSPECL_FAN_SUMMARY                  fan_summary_ptr;
    PSPECL_MGMT_SUMMARY                 mgmt_summary_ptr;
    PSPECL_IO_SUMMARY                   io_summary_ptr;
    PSPECL_BMC_SUMMARY                  bmc_summary_ptr;
    PSPECL_FLT_EXP_SUMMARY              flt_exp_summary_ptr;
    PSPECL_SLAVE_PORT_SUMMARY           slave_port_summary_ptr;
    PSPECL_LED_SUMMARY                  led_summary_ptr;
    PSPECL_DIMM_SUMMARY                 dimm_summary_ptr;
    PSPECL_FIRMWARE_SUMMARY             firmware_summary_ptr;
    PSPECL_MISC_SUMMARY                 misc_summary_ptr;
    PSPECL_PCI_SUMMARY                  pci_summary_ptr;
    PSPECL_SPS_RESUME                   sps_resume_ptr;
    PSPECL_SPS_SUMMARY                  sps_summary_ptr;
}NV_MINIPORT_INFO, *PNV_MINIPORT_INFO;

#pragma pack()

#endif  // _K10_MINIPORT_INFORMATION_H_
