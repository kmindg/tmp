 /***************************************************************************
  * Copyright (C) EMC Corporation 2009
  * All rights reserved.
  * Licensed material -- property of EMC Corporation
  ***************************************************************************/
 
 /*!**************************************************************************
  * @file fbe_bvd_interface_executor.c
  ***************************************************************************
  *
  * @brief
  *  This file contains the executer functions for bvd interfcace.
  * 
  *
  ***************************************************************************/
 
 /*************************
  *   INCLUDE FILES
  *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_winddk.h"
#include "fbe_service_manager.h"
#include "fbe_bvd_interface_private.h"

/*************************
*   FUNCTION DEFINITIONS
*************************/

/***************************************************************************************************************************/
fbe_status_t fbe_bvd_interface_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet_p)
{
    fbe_status_t 							status = FBE_STATUS_GENERIC_FAILURE;

    if (packet_p->base_edge == NULL)
    {
        fbe_transport_set_status(packet_p, FBE_STATUS_NO_OBJECT, __LINE__);
        fbe_transport_complete_packet_async(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /*when the packet arrives here from the sep shim, it already contains the edge we need to send it on
	all we do is upcast the base_edge in he packet to be a block edge*/
	status = fbe_block_transport_send_funct_packet_params((fbe_block_edge_t *)packet_p->base_edge, packet_p, FBE_FALSE);
    return status;
}


fbe_status_t fbe_bvd_interface_block_transport_entry(fbe_transport_entry_context_t context, 
                                                     fbe_packet_t * packet_p)
{
    fbe_status_t 				status = FBE_STATUS_GENERIC_FAILURE;
    fbe_bvd_interface_t *		bvd_p = NULL;

    bvd_p = (fbe_bvd_interface_t *)context;

   /*TODO - add logic here*/
    return status;
}

