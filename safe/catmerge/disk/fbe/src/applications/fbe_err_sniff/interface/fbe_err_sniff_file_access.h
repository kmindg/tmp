#ifndef FBE_ERR_SNIFF_FILE_ACCESS_H
#define FBE_ERR_SNIFF_FILE_ACCESS_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_err_sniff_file_access.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of file_access functions for fbe_err_sniff
 *
 * @version
 *   08/08/2011:  Created. Vera Wang
 *
 ***************************************************************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"

FILE* fbe_err_sniff_open_file(const char* file_name, char fMode);
fbe_status_t fbe_err_sniff_write_to_file(FILE * fp,const char* content);
fbe_status_t fbe_err_sniff_close_file(FILE * fp);

#endif
/**************************************************
 * end file fbe_err_sniff_file_access.h
 **************************************************/
