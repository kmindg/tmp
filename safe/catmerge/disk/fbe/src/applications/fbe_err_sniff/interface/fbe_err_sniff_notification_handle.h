#ifndef __FBE_ERR_SNIFF_NOTIFICATION_HANDLE_H__
#define __FBE_ERR_SNIFF_NOTIFICATION_HANDLE_H__
/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_err_sniff_notification_handle.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of commands of FBE Cli related to database
 *
 * @version
 *   08/08/2011:  Created. Vera Wang
 *
 ***************************************************************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"

fbe_status_t fbe_err_sniff_notification_handle_init(FILE* fp, fbe_char_t panic_flag);
fbe_status_t fbe_err_sniff_notification_handle_destroy(void);

#endif

/**************************************************
 * end file fbe_err_sniff_notification_handle.h
 **************************************************/
