#ifndef FBE_NOTIFICATION_ANALYZER_FILE_ACCESS_H
#define FBE_NOTIFICATION_ANALYZER_FILE_ACCESS_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_notification_analyzer_file_access.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of file_access functions for FBE_NOTIFICATION_ANALYZER
 *
 * @version
 *   08/08/2011:  Created. Vera Wang
 *
 ***************************************************************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe_notification_analyzer_notification_handle.h"

FILE* fbe_notification_analyzer_open_file(const char* file_name, char fMode);
fbe_status_t fbe_notification_analyzer_write_to_file(FILE * fp,fbe_notification_analyzer_notification_info_t * content);
fbe_status_t fbe_notification_analyzer_close_file(FILE * fp);

#endif
/**************************************************
 * end file fbe_notification_analyzer_file_access.h
 **************************************************/
