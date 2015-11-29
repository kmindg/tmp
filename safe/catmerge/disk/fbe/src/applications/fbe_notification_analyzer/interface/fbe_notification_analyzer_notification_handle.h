#ifndef __FBE_NOTIFICATION_ANALYZER_NOTIFICATION_HANDLE_H__
#define __FBE_NOTIFICATION_ANALYZER_NOTIFICATION_HANDLE_H__
/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_notification_analyzer_notification_handle.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of fbe_notification_analyzer_notification handle
 *
 * @version
 *   05/30/2012:  Created. Vera Wang
 *
 ***************************************************************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe_notification_interface.h"
#include "fbe_package.h"
#include "fbe_topology_interface.h"

typedef struct fbe_notification_analyzer_notification_info_s{
	fbe_queue_element_t		queue_element;/*must be first to simplify things*/
    fbe_object_id_t         object_id;
	fbe_notification_info_t notification_info;
}fbe_notification_analyzer_notification_info_t;

typedef struct fbe_notification_analyzer_notifinication_thread_info_s{
    FILE* fp;
}fbe_notification_analyzer_notifinication_thread_info_t;


fbe_status_t fbe_notification_analyzer_notification_handle_init(FILE * fp,fbe_notification_type_t type,
                                                                fbe_package_notification_id_mask_t package,fbe_topology_object_type_t object_type);
fbe_status_t fbe_notification_analyzer_notification_handle_destroy(void);

#endif

/**************************************************
 * end file fbe_notification_analyzer_notification_handle.h
 **************************************************/
