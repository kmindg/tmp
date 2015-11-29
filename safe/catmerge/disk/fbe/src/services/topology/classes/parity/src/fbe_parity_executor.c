 /***************************************************************************
  * Copyright (C) EMC Corporation 2009
  * All rights reserved.
  * Licensed material -- property of EMC Corporation
  ***************************************************************************/
 
 /*!**************************************************************************
  * @file fbe_parity_executor.c
  ***************************************************************************
  *
  * @brief
  *  This file contains the executer functions for the parity object.
  * 
  *  This includes the fbe_parity_io_entry() function which is our entry
  *  point for functional packets.
  * 
  * @ingroup parity_class_files
  * 
  * @version
  *   9/1/2009 - Created. Rob Foley
  *
  ***************************************************************************/
 
 /*************************
  *   INCLUDE FILES
  *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_parity.h"
#include "fbe_parity_private.h"
#include "fbe/fbe_winddk.h"
#include "fbe_service_manager.h"

 /*************************
  *   FUNCTION DEFINITIONS
  *************************/

/*!***************************************************************
 * fbe_parity_io_entry()
 ****************************************************************
 * @brief
 *  This function is the entry point for
 *  functional transport operations to the parity object.
 *  The FBE infrastructure will call this function for our object.
 *
 * @param object_handle - The parity handle.
 * @param packet_p - The io packet that is arriving.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_GENERIC_FAILURE if transport is unknown,
 *                 otherwise we return the status of the transport handler.
 *
 * @author
 *  9/1/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t 
fbe_parity_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet_p)
{
    /*! Call the base class to start the I/O.
     */
    return fbe_raid_group_io_entry(object_handle, packet_p);
}
/* end fbe_parity_io_entry() */

/*************************
 * end file fbe_base_config_executer.c
 *************************/
 
 
