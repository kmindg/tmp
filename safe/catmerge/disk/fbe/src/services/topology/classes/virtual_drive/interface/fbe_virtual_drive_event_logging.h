#ifndef VIRTUAL_DRIVE_EVENT_LOGGING_H
#define VIRTUAL_DRIVE_EVENT_LOGGING_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2011-2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_virtual_drive_event_logging.h
 ***************************************************************************
 *
 * @brief
 *  This file contains functions that are used to log
 *  events from virtual drive object.
 * 
 * 
 * @ingroup virtual_drive_class_files
 * 
 * @version
 *   05/23/2011:  Created. Vishnu Sharma
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe_base_object.h"
#include "fbe_base_service.h"
#include "fbe_service_manager.h"
#include "fbe/fbe_transport.h"
#include "fbe_block_transport.h"
#include "fbe_virtual_drive_private.h"
#include "fbe_spare.h"

/************************ 
 * FUNCTION_PROTOTYPES
 *************************/
fbe_status_t fbe_virtual_drive_event_logging_get_original_pvd_object_id(fbe_virtual_drive_t *virtual_drive_p,
                                                                        fbe_object_id_t *original_pvd_object_id_p,
                                                                        fbe_bool_t b_is_copy_in_progress);

fbe_status_t fbe_virtual_drive_event_logging_get_spare_pvd_object_id(fbe_virtual_drive_t *virtual_drive_p,
                                                                     fbe_object_id_t *spare_pvd_object_id_p,
                                                                     fbe_bool_t b_is_copy_in_progress);

fbe_status_t  fbe_virtual_drive_write_event_log(fbe_virtual_drive_t *virtual_drive_p,
                                                fbe_u32_t event_code,
                                                fbe_spare_internal_error_t internal_error_status,
                                                fbe_object_id_t original_pvd_object_id,
                                                fbe_object_id_t spare_pvd_object_id,
                                                fbe_packet_t *packet_p);



#endif /* VIRTUAL_DRIVE_EVENT_LOGGING_H */

/***************************************
 * end fbe_virtual_drive_event_logging.h
 ***************************************/
