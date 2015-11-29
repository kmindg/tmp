/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved. Licensed material -- property of EMC
 * Corporation
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_package.h"
#include "fbe_topology.h"
#include "fbe_service_manager.h"
#include "fbe/fbe_registry.h"



/*!***************************************************************
 * fbe_api_esp_common_init()
 ****************************************************************
 * @brief
 *  This is for initialized the fbe_api in user space.
 *
 * @param 
 * 
 * @return fbe_status_t
 *
 * @author
 *  18-March-2010: Created.
 *
 ****************************************************************/
fbe_status_t fbe_api_esp_common_init(void)
{
    fbe_status_t status;

    status = fbe_api_common_init_sim();

    /* Set the function entries for physical package*/
    fbe_api_set_simulation_io_and_control_entries(FBE_PACKAGE_ID_PHYSICAL, 
                                                  fbe_topology_send_io_packet, 
                                                  fbe_service_manager_send_control_packet);

    fbe_api_set_simulation_io_and_control_entries(FBE_PACKAGE_ID_SEP_0, 
                                                  fbe_topology_send_io_packet, 
                                                  fbe_service_manager_send_control_packet);

	fbe_api_set_simulation_io_and_control_entries(FBE_PACKAGE_ID_ESP, 
                                                  fbe_topology_send_io_packet, 
                                                  fbe_service_manager_send_control_packet);

    //fbe_service_manager_set_sep_control_entry((fbe_service_control_entry_t)fbe_service_manager_send_control_packet);
    
    return status;
}
/**********************************
    end fbe_api_esp_common_init()
***********************************/

/*!***************************************************************
 * fbe_api_esp_common_destroy()
 ****************************************************************
 * @brief
 *  This destroys the fbe_api in user space.
 *
 * @param 
 * 
 * @return fbe_status_t
 *
 * @author
 *  06-January-2012: Created.
 *
 ****************************************************************/
fbe_status_t fbe_api_esp_common_destroy(void)
{
    fbe_status_t status;

    status = fbe_api_common_destroy_sim();

    return status;
}
/**********************************
    end fbe_api_esp_common_destroy()
***********************************/

/*!***************************************************************
 *      fbe_get_registry_path()
 ****************************************************************
 * @brief
 *  This function return the registry path for ESP.
 *
 * @param 
 * 
 * @return fbe_u8_t *  Path for ESP registry
 *
 * @author
 *  22 -April-2010: Vaibhav Gaonkar Created.
 *
 ****************************************************************/
fbe_u8_t* fbe_get_registry_path(void)
{
    return(EspRegPath);
}
/**********************************
    end fbe_get_registry_path()
***********************************/

/*!***************************************************************
 *      fbe_get_fup_registry_path()
 ****************************************************************
 * @brief
 *  This function return the registry path for firmware upgrade.
 *
 * @param 
 * 
 * @return fbe_u8_t *  Path for K10DriverConfig registry
 *
 * @author
 *  19-Aug-2010: PHE - Created.
 *
 ****************************************************************/
fbe_u8_t* fbe_get_fup_registry_path(void)
{
    return K10DriverConfigRegPath;
}
/**********************************
    end fbe_get_fup_registry_path()
***********************************/
