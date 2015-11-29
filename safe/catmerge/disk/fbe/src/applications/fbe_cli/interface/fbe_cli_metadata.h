#ifndef __FBE_CLI_METADATA_H__
#define __FBE_CLI_METADATA_H__
/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_metadata.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of commands of FBE Cli related to metadata
 *
 * @version
 *   05/14/2012:  Created. Jingcheng Zhang
 *
 ***************************************************************************/

void fbe_cli_metadata(fbe_s32_t argc, fbe_s8_t ** argv);
/* Function prototypes*/
void fbe_cli_cmd_set_default_nonpaged_metadata(fbe_s32_t argc,char ** argv);
static void fbe_cli_set_default_nonpaged_metadata(fbe_u32_t object_id);

#define FBE_CLI_METADATA_USAGE  "\
metadata or md - Show metadata information\n\
usage:  metadata -getNonPagedVersion  object_id         - Get version info of an object metdata.\n\
"
	
	/* Usage*/
#define SET_NP_USAGE                               \
	" setdefaultnonpagedmd / setnp	 - set object nonpaged metadata\n" \
	" Usage:  \n"\
	"		setnp -h\n"\
	"		setnp objectid\n"
	

#endif /* end __FBE_CLI_METADATA_H__ */
/*************************
 * end file fbe_cli_metadata.h
 *************************/
