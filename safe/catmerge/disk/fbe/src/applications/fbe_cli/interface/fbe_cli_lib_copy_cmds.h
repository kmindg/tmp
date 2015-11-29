#ifndef __FBE_CLI_LIB_COPY_CMDS_H__
#define __FBE_CLI_LIB_COPY_CMDS_H__
/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_lib_copy_cmds.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of commands of FBE Cli related to COPY cmds.
 *
 * @version
 *   06/04/2012:  Created. Vera Wang
 *
 ***************************************************************************/

void fbe_cli_copy(fbe_s32_t argc, fbe_s8_t ** argv);

#define FBE_CLI_COPY_USAGE  "\
copy - Initial copy command. \n\
usage:   copy -help                                   - Display help information\n\
         copy -copyTo source_object_id dest_object_id - Copy from source_object to dest_object\n\
         copy -copyTo -sd b_e_d -dd b_e_d - Copy from source drive in Bus_encl_slot format to destination drive\n\
         copy -proactiveSpare object_id               - Proactive copy from object.\n\n\
Example: copy -copyTo 0x105 0x109.\n\
         copy -copyTo -sd 0_1_0 -dd 0_1_1\n"

#endif /* end __FBE_CLI_DATABASE_H__ */
/*************************
 * end file fbe_cli_database.h
 *************************/
