#ifndef FBE_API_RAW_MIRROR_SERVICE_INTERFACE_H
#define FBE_API_RAW_MIRROR_SERVICE_INTERFACE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_raw_mirror_service_interface.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the fbe_api interface for the raw mirror service.
 *  This service allows I/O to be generated to a raw mirror.
 * 
 *  In production code, a raw mirror is used to access the system LUNs non-paged metadata
 *  and their configuration tables.  There is no object interface for this data.  It is
 *  accessed early in boot before any objects have been created.  
 * 
 *  The raw mirror service is used to test fbe_raid_raw_mirror_library
 *  and associated RAID library code.
 * 
 *  The code is this file is used by test code to interface with the raw mirror service
 *  to pass along I/O requests to the raw mirror.
 *
 * @version
 *  10/2011 - Created. Susan Rundbaken
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_raw_mirror_service_interface.h"


/*************************
 *   DEFINITIONS
 *************************/

/*!********************************************************************* 
 * @struct fbe_api_raw_mirror_service_context_t 
 *  
 * @brief 
 *   Contains the context we use to facilitate sending I/O requests to
 *   the raw mirror.
 *
 **********************************************************************/
typedef struct fbe_api_raw_mirror_service_context_s
{
    /*! Semaphore to use to wait for raw mirror control packet to complete. 
     */
    fbe_semaphore_t semaphore; 
    fbe_semaphore_t semaphore_io; 

    fbe_status_t status; /*! Status of the raw mirror control operation. */
    fbe_packet_t packet; /*! Packet to use for sending the control operation. */
    fbe_packet_t packet_io; /*! Packet to use for sending the control operation. */
    
    /*! The control operation to send to the raw mirror service. 
     */
    fbe_raw_mirror_service_control_start_io_t start_io; 
}
fbe_api_raw_mirror_service_context_t;


/*************************
 *   FUNCTION PROTOTYPES
 *************************/

fbe_status_t fbe_api_raw_mirror_service_test_context_init(fbe_api_raw_mirror_service_context_t *context_p,
                                                          fbe_raw_mirror_service_operation_t raw_mirror_operation,
                                                          fbe_data_pattern_t pattern,
                                                          fbe_u32_t sequence_id,
                                                          fbe_u32_t block_size,
                                                          fbe_u32_t block_count,
                                                          fbe_lba_t start_lba,
                                                          fbe_u64_t sequence_num,
                                                          fbe_bool_t dmc_expected_b,
                                                          fbe_u32_t max_passes,
                                                          fbe_u32_t num_threads);
                
fbe_status_t fbe_api_raw_mirror_service_run_test(fbe_api_raw_mirror_service_context_t *context_p,
                                                 fbe_package_id_t package_id);

fbe_status_t fbe_api_raw_mirror_service_stop_test(fbe_api_raw_mirror_service_context_t *context_p,
                                                  fbe_package_id_t package_id);
fbe_status_t fbe_api_raw_mirror_service_wait_pvd_edge_enabled(fbe_api_raw_mirror_service_context_t *context_p,
                                                              fbe_package_id_t package_id);

fbe_status_t fbe_api_raw_mirror_service_test_context_destroy(fbe_api_raw_mirror_service_context_t *context_p);

#endif /* FBE_API_RAW_MIRROR_SERVICE_INTERFACE_H */

/*************************
 * end file fbe_api_raw_mirror_service_interface.h
 *************************/

