#ifndef __FBE_CLI_CMS_H__
#define __FBE_CLI_CMS_H__
/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_cms.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of commands of FBE Cli related to clustered memory
 *
 *
 ***************************************************************************/

void fbe_cli_cms(fbe_s32_t argc, fbe_s8_t ** argv);

#define FBE_CLI_CMS_USAGE  "\
cluster_memory or cms - Show Cluster Memory information\n\
usage:  cms -get_info                              - Get general information regarding the service.\n\
"

#endif /* end __FBE_CLI_CMS_H__ */
/*************************
 * end file fbe_cli_database.h
 *************************/

