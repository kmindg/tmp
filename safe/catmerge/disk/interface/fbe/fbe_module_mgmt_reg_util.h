/***************************************************************************
 * Copyright (C) EMC Corporation 2010 - 2012
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

#ifndef FBE_MODULE_MGMT_REG_UTIL_H
#define FBE_MODULE_MGMT_REG_UTIL_H

/*
 * These are the string values expected to be found or placed in the
 * Registry for the CPD PortParams entries.
 */
#define BE_NORM_STRING "BE_NORM"
#define FE_NORM_STRING "FE_NORM"
#define FE_SPECIAL_STRING "FE_SPECIAL"
#define BOOT_STRING "BOOT"
#define UNC_STRING "UNC"
#define FC_STRING "fcdmtl"
#define ISCSI_CPDISQL_STRING "cpdisql"

#ifdef C4_INTEGRATED
// need another driver name for VNXe
#define ISCSI_GENERAL_STRING "iscsicsx"
#else
#define ISCSI_GENERAL_STRING "iscsiwsk"
#endif /* C4_INTEGRATED - C4ARCH - networking */

#define SAS_STRING  "saspmc"
#define FCOE_STRING "fcoedmql"
#define SAS_FE_STRING "saslsi"
#define FC_16G_STRING "fcdmql"
#define ISCSI_QL_STRING "iscsiql"
#define UNKNOWN_PROTOCOL_STRING "@"
#define PORT_PARAMS_DEFAULT_STRING "{[*]J(@,UNC,?)} "

#define INVALID_CORE_NUMBER 0xFFFFFFFE
#define INVALID_PHY_MAP     0x0

#endif /* FBE_MODULE_MGMT_REG_UTIL_H */

/*******************************
 * end fbe_module_mgmt_reg_util.h
 *******************************/
