 /***************************************************************************
  * Copyright (C) EMC Corporation 2008
  * All rights reserved.
  * Licensed material -- property of EMC Corporation
  ***************************************************************************/
 
 /*!**************************************************************************
  * @file fbe_mirror_executor.c
  ***************************************************************************
  *
  * @brief
  *  This file contains the executer functions for the mirror.
  * 
  *  This includes the fbe_mirror_io_entry() function which is our entry
  *  point for functional packets.
  * 
  * @ingroup mirror_class_files
  * 
  * @version
  *   05/20/2009:  Created. RPF
  *
  ***************************************************************************/
 
 /*************************
  *   INCLUDE FILES
  *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_mirror.h"
#include "fbe_mirror_private.h"
#include "fbe/fbe_winddk.h"
#include "fbe_service_manager.h"

 /*************************
  *   FUNCTION DEFINITIONS
  *************************/

/*!***************************************************************
 * fbe_mirror_io_entry()
 ****************************************************************
 * @brief
 *  This function is the entry point for
 *  functional transport operations to the mirror object.
 *  The FBE infrastructure will call this function for our object.
 *
 * @param object_handle - The mirror handle.
 * @param packet_p - The io packet that is arriving.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_GENERIC_FAILURE if transport is unknown,
 *                 otherwise we return the status of the transport handler.
 *
 * @author
 *  05/22/09 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_mirror_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet_p)
{
    /*! Make sure that this is a mirror class.
     */
    if (fbe_base_object_get_class_id(object_handle) != FBE_CLASS_ID_MIRROR)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    

    /*! Call the base class to start the I/O.
     */
    return fbe_raid_group_io_entry(object_handle, packet_p);

}
/* end fbe_mirror_io_entry() */

/*************************
 * end file fbe_base_config_executer.c
 *************************/
 
 
