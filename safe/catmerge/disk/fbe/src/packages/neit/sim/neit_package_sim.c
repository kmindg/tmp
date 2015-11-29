/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. 
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file neit_package_sim.c
 ***************************************************************************
 *
 * @brief This file is part of D
 * 
 *
 * History:
 *  05/01/2012  Created. kothal
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_package.h"
#include "fbe_topology.h"
#include "fbe_service_manager.h"
#include "fbe/fbe_registry.h"



/*!***************************************************************
 * fbe_api_neit_package_common_init()
 ****************************************************************
 * @brief
 *  This is for initialized the fbe_api in user space.
 *
 * @param 
 * 
 * @return fbe_status_t
 *
 * History:
 *  05/01/2012  Created. kothal
 *
 ****************************************************************/
fbe_status_t fbe_api_neit_package_common_init(void)
{
    fbe_status_t status;

    status = fbe_api_common_init_sim();  
	
	/* Set the function entries for physical package*/
    fbe_api_set_simulation_io_and_control_entries(FBE_PACKAGE_ID_PHYSICAL, 
                                                  fbe_topology_send_io_packet, 
                                                  fbe_service_manager_send_control_packet);
	
	/* Set the function entries for neit package*/
    fbe_api_set_simulation_io_and_control_entries(FBE_PACKAGE_ID_NEIT, 
                                                  fbe_topology_send_io_packet, 
                                                  fbe_service_manager_send_control_packet);
    
    return status;
}
/**********************************
    end fbe_api_neit_package_common_init()
***********************************/

/*!***************************************************************
 * fbe_api_neit_common_package_destroy()
 ****************************************************************
 * @brief
 *  This destroys the fbe_api in user space.
 *
 * @param 
 * 
 * @return fbe_status_t
 *
 * History:
 *  05/01/2012  Created. kothal
 *
 ****************************************************************/
fbe_status_t fbe_api_neit_package_common_destroy(void)
{
    fbe_status_t status;

    status = fbe_api_common_destroy_sim();

    return status;
}
/**********************************
    end fbe_api_neit_common_package_destroy()
***********************************/
