
/***************************************************************************
 * Copyright (C) EMC Corporation 1989-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                bgsl_logical_package_init.c
 *************************************************************************
 *
 *  DESCRIPTION:        Logical package initialization code.
 *
 *  FUNCTIONS:
 *
 *  LOCAL FUNCTIONS:
 *
 *  NOTES:
 *
 ***************************************************************************/


#include "bgsl_base_service.h"
#include "bgsl_service_manager.h"

#include "fbe/bgsl_logical_package.h"
#include "fbe/bgsl_panic.h"
#include "fbe/bgs_memory.h"

extern bgsl_status_t bgs_svc_init(void);
extern bgsl_status_t bgsl_flare_adapter_init(void);

extern bgsl_service_methods_t bgsl_l_bgs_svc_methods;
extern bgsl_service_methods_t bgsl_l_flare_driver_service_methods;

extern bgsl_logical_package_data_t* bgsl_logical_package_data_p_g;

static const bgsl_service_methods_t * logical_package_service_table[] = {   &bgsl_l_bgs_svc_methods,
                                                                            &bgsl_l_flare_driver_service_methods,
																			NULL };
#if defined(UMODE_ENV) || defined(SIMMODE_ENV)
    /* Function declaration for packet interception integration from catmerge\disk\fbe\src\packet_interceptor\bgsl_packet_interceptor.c */
    bgsl_status_t bgsl_packet_interceptor_init_service_table(const bgsl_service_methods_t * service_table[], bgsl_package_id_t package_id);
#endif

// ???
// This will be called from a different location eventually.
bgsl_status_t 
logical_package_init(void)
{
    bgsl_status_t status;
    KvTrace("bgsl_logical_pkg: %s entry\n", __FUNCTION__);

#if !defined(UMODE_ENV) && !defined(SIMMODE_ENV)
    /* Initialize the memory */
    status =  bgs_memory_init();
    if(status != BGSL_STATUS_OK) {
        KvTrace("logical_package_init: %s bgsl_memory_init fail status %X\n", __FUNCTION__, status);
        return status;
    }
#endif

#if 0 // This initializes certain things for allocating/deallocating memory from
    //    memory service. logical package will not use memory service at this time.
    //    So, this initialization is not required now.
    /* Initialize mgmt_transport */
    status =  bgsl_transport_init();
    if(status != BGSL_STATUS_OK) {
        KvTrace("bgsl_logical_pkg: %s mgmt_transport_init fail status %X\n", __FUNCTION__, status);
        return status;
    }
#endif

#if 0 // Not used in logical package.
    /* Initialize notification service */
    status = bgsl_notification_init();
    if(status != BGSL_STATUS_OK) {
        KvTrace("bgsl_logical_pkg: %s bgsl_notification_init fail status %X\n", __FUNCTION__, status);
        return status;
    }
#endif

    /* The LAST step: Used to be. Causes issues. Need to discuss with Peter ??? */
    status = bgsl_service_manager_init_service_table(logical_package_service_table, BGSL_PACKAGE_ID_LOGICAL_0);

    if(status != BGSL_STATUS_OK) {
        KvTrace("bgsl_logical_pkg: %s bgsl_service_manager_init fail status %X\n", __FUNCTION__, status);
        return status;
    }

    // Allocate and initialize some memory for the package's block data
    bgsl_logical_package_data_p_g = (bgsl_logical_package_data_t *)bgs_malloc(sizeof(bgsl_logical_package_data_t));

    if (bgsl_logical_package_data_p_g == NULL)
    {
        // Unable to malloc memory
        KvTrace("bgsl_logical_pkg: _init failed to malloc memory\n");
        return BGSL_STATUS_GENERIC_FAILURE;
    }

    //Clear the memory we just got.
    memset(bgsl_logical_package_data_p_g, 0, sizeof(bgsl_logical_package_data_t));

    // Initialize the master packet queue's spin lock
    bgsl_spinlock_init(&bgsl_logical_package_data_p_g->bgs_master_packet_queue_spin_lock);


    /* Initialize BGS service init. */
    status = bgs_svc_init();
    if (status != BGSL_STATUS_OK) {
        KvTrace("bgsl_logical_pkg: %s bgs_svc_init failed. status=%X\n", __FUNCTION__, status);
        return status;
    }
    status = bgsl_flare_adapter_init();
    if (status != BGSL_STATUS_OK) {
        KvTrace("bgsl_logical_pkg: %s bgsl_flare_adapter_init failed. status=%X\n", __FUNCTION__, status);
        return status;
    }
/* FBE30
    status = fbe_job_service_init();
    if (status != FBE_STATUS_OK) {
        KvTrace("fbe_logical_pkg: %s fbe_job_service_init failed. status=%X\n", __FUNCTION__, status);
        return status;
    }
*/
    KvTrace("bgsl_logical_pkg: %s success\n", __FUNCTION__);
    return BGSL_STATUS_OK;
}
