/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_api_discovery_interface.c
 ***************************************************************************
 *
 *  Description
 *      Discovery related APIs 
 *      
 *
 *  History:
 *      10/10/08    sharel - Created
 *    
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_object_map-interface.h"



/**************************************
				Local variables
**************************************/

/*******************************************
				Local functions
********************************************/


/*********************************************************************
 *            fbe_api_ppfd_get_total_number_of_drives ()
 *********************************************************************
 *
 *  Description: return total number of drives on a port
 *
 *	Inputs: port_number - backend port number
 * 			total_drives  - total drives on this port
 *
 *  Return Value: success or failue
 *
 *  History:
 *    11/24/08: sharel	created
 *    
 *********************************************************************/
fbe_status_t fbe_api_ppfd_get_total_number_of_drives (fbe_u32_t port_number, fbe_u32_t *total_drives)
{
	return fbe_api_object_map_interface_get_total_logical_drives (port_number, total_drives);

}
