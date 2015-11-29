#ifndef __FBE_CLI_EXTPOOL_H__
#define __FBE_CLI_EXTPOOL_H__
/***************************************************************************
 * Copyright (C) EMC Corporation 2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_rginfo.h
 ***************************************************************************
 *
 * @brief
 *  This file contains extent pool info command cli definitions.
 *
 * @version
 *  6/24/2014 - Created. Rob Foley
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


void fbe_cli_extpool(int argc , char ** argv);
#define EXTPOOL_CMD_USAGE "\
extpool -help|h\n\
extpool -create -pool (pool id) -drive_count (drives) -drive_type (2-SAS   \n\
extpool -createlun  -pool (pool id) -lun (lun id) (-capacity (blocks) -capacitymb (megabytes) -capacitygb (megabytes)) \n\
extpool -destroy -pool (pool id)  \n\
extpool -destroylun -lun (lun id) \n\
\n\
Examples:\n\
   extpool -create -pool 0 -drive_count 20 -drive_type 2\n\
  To create pool with any drive type, do not provide -drive_type.\n\
   extpool -create -pool 0 -drive_count 20 \n\
   \n\
   extpool -createlun -pool 0 -lun 0 -capacity 100000\n\
   extpool -createlun -pool 0 -lun 0 -capacitygb 5000\n\
\n"

/*************************
 * end file fbe_cli_extpool.h
 *************************/

#endif /* end __FBE_CLI_EXTPOOL_H__ */
