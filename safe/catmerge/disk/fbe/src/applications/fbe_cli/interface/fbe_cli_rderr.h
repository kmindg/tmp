#ifndef __FBE_CLI_RDERR_H__
#define __FBE_CLI_RDERR_H__
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_rdt.h
 ***************************************************************************
 *
 * @brief
 *  This file contains LUN/RG command cli definitions.
 *
 * @version
 *   6/17/2010:  Created. Swati Fursule
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


void fbe_cli_rderr(int argc , char ** argv);

/*!*******************************************************************
 * @def RDERR_CMD_USAGE
 *********************************************************************
 * @brief Usage string for Rderr 
 *
 *********************************************************************/
#define RDERR_CMD_USAGE "\
rderr [switches] <recnum | all>\n\
Switches:\n\
     -help |h  - Displays help\n\
     -display |d - display records\n\
     -info |i - display information on available tables\n\
     -table |t - switch to new default table [0..max_tables]\n\
     -modify |m - [start end] modify records\n\
     -create|c - create records\n\
     -remove|r - [start end] remove records\n\
     -mode    [CNT | ALW | RND | SKP] [start end]\n\
              change mode for range of records or all records\n\
     -errtype [HARD | CRC | TS | WS | BTS | BWS] [start end]\n\
              change type for range of records or all records\n\
     -gap [blocks] [start] [end]\n\
              Change the gap in between records for the given\n\
              range of records.  This keeps the LBA at the start\n\
              of the range the same, but following LBAS will be\n\
              offset by the given block count.\n\
     -pos [bitmask] [start] [end]\n\
              Change the position bitmask for the given\n\
              range of records.\n\
     -hardware_tables | hw  - enable selection/display of hardware specific tables\n\
     -simulation_tables | sim  - enable selection/display of simulation specific tables\n\
     -enable|ena  - enable RDERR\n\
     -disable| dis- disable RDERR\n\
     -enable_for_rg | ena_rg - [Raid Group Number] enable RDERR for specific Raid group.\n\
     -disable_for_rg | dis_rg - [Raid Group Number]disable RDERR for specific Raid group.\n\
     -enable_for_object | ena_obj - [Object Id, Package Id] enable RDERR for specific object id in given package id.\n\
     -disable_for_object |dis_obj - [Object Id, Package Id] disable RDERR for specific object id in given package id\n\
     -enable_for_class |ena_cla - [Object Id, Package Id] enable RDERR for specific class id in given package id.\n\
     -disable_for_class |dis_cla - [Object Id, Package Id] disable RDERR for specific class id in given package id\n\
     -stats - Display statistics of logical error injection service\n\
     -stats_for_rg- - [Raid Group Number] Display statistics for raid group.\n\
     -stats_for_object -[Object Id] Display statistics for Object.\n"

/*!*******************************************************************
 * @def FBE_RDERR_MAX_STR_LEN
 *********************************************************************
 * @brief Max number of string used for display rderr specific data.
 *
 *********************************************************************/
#define FBE_RDERR_MAX_STR_LEN        64

/*************************
 * end file fbe_cli_rderr.h
 *************************/

#endif /* end __FBE_CLI_RDERR_H__ */
