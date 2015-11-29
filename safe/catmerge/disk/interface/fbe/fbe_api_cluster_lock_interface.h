#ifndef FBE_API_CLUSTER_LOCK_INTERFACE_H
#define FBE_API_CLUSTER_LOCK_INTERFACE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_cluster_lock_interface.h
 ***************************************************************************
 *
 * @brief
 *  This file contains declaration of API to interface with cluster lock service.
 *  
 * @version
 *
 ***************************************************************************/

#include "fbe/fbe_types.h"


/*use this macro and the closing one: FBE_API_CPP_EXPORT_END to export your functions to C++ applications*/
FBE_API_CPP_EXPORT_START

fbe_status_t FBE_API_CALL fbe_api_cluster_lock_release_lock(fbe_bool_t *data_stale);
fbe_status_t FBE_API_CALL fbe_api_cluster_lock_shared_lock(fbe_bool_t *data_stale);
fbe_status_t FBE_API_CALL fbe_api_cluster_lock_exclusive_lock(fbe_bool_t *data_stale);

/*! @} */ /* end of group fbe_api_event_log_service_interface */
FBE_API_CPP_EXPORT_END

#endif /* FBE_API_CLUSTER_LOCK_INTERFACE_H */