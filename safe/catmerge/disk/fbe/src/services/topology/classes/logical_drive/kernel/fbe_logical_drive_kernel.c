 /***************************************************************************
  * Copyright (C) EMC Corporation 2008
  * All rights reserved.
  * Licensed material -- property of EMC Corporation
  ***************************************************************************/
 
 /*!**************************************************************************
  * @file fbe_logical_drive_kernel.c
  ***************************************************************************
  *
  * @brief
  *  This file contains kernel only functions.
  *  These are needed since we remap the functions depending on if we are in
  *  user or kernel mode.
  * 
  *  In User mode we use "mock" functions in order that we can perform unit
  *  testing, but in kernel mode we just call the underlying functions.
  *
  * HISTORY
  *   7/23/2008:  Created. RPF
  *
  ***************************************************************************/
 
 /*************************
  *   INCLUDE FILES
  *************************/
 
 /*************************
  *   FUNCTION DEFINITIONS
  *************************/
 
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_logical_drive.h"
#include "fbe_logical_drive_private.h"
#include "fbe_logical_drive_io_command.h"

/****************************************************************
 * fbe_ldo_setup_defaults()
 ****************************************************************
 * DESCRIPTION:
 *  This function initializes the logical drive object's defaults.
 *
 * PARAMETERS:
 *  logical_drive_p, [I] - The logical drive object.
 *
 * RETURNS:
 *  none
 *
 * HISTORY:
 *  11/01/07 - Created. RPF
 *
 ****************************************************************/
void
fbe_ldo_setup_defaults(fbe_logical_drive_t * logical_drive_p)
{
    /* Nothing to do for now.
     */
    return;
}
/*********************************************
 * fbe_ldo_setup_defaults()
 *********************************************/
 
/****************************************************************
 * fbe_ldo_send_io_packet()
 ****************************************************************
 * DESCRIPTION:
 *  This function sends an IO packet.
 *
 * PARAMETERS:
 *  logical_drive_p, [I] - Logical drive to send for.
 *  packet_p, [I] - io packet to send.
 *
 * RETURNS:
 *  Status of the send.
 *
 * HISTORY:
 *  11/13/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_send_io_packet(fbe_logical_drive_t * const logical_drive_p,
                                    fbe_packet_t *packet_p)
{
    fbe_status_t status;

    /* Send out the io packet. Send this to our only edge.
     */
    status = fbe_block_transport_send_functional_packet(&logical_drive_p->block_edge, 
                                                        packet_p);
    return status;
}
/* end fbe_ldo_send_io_packet() */

/*!***************************************************************
 * @fn fbe_ldo_complete_packet(fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *  This function maps the packet completion function.
 *
 * @param packet_p - IO packet to complete.
 *
 * @return
 *  Status of the send.
 *
 * HISTORY:
 *  11/13/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t
fbe_ldo_complete_packet(fbe_packet_t * packet_p)
{
    return fbe_transport_complete_packet(packet_p);
}
/**************************************
 * end fbe_ldo_complete_packet()
 **************************************/

/****************************************************************
 * fbe_ldo_io_cmd_free()
 ****************************************************************
 * DESCRIPTION:
 *  This function frees the given input io cmd.
 *
 * PARAMETERS:
 *  io_cmd_p, [I] - The io cmd to free.
 *
 * RETURNS:
 *  NONE
 *
 * HISTORY:
 *  06/11/08 - Created. RPF
 *
 ****************************************************************/
void 
fbe_ldo_io_cmd_free(fbe_logical_drive_io_cmd_t *const io_cmd_p)
{
    /* We free this as a simple memory free since it is essentially a 
     * buffer. 
     */
    fbe_ldo_free_io_cmd(io_cmd_p);
    return;
}
/* end fbe_ldo_io_cmd_free() */

/*!***************************************************************
 * @fn fbe_ldo_clear_condition(struct fbe_base_object_s * object_p)
 ****************************************************************
 * @brief
 *  This function clears a condition.
 * 
 *  For kernel mode we simply call the lifecycle condition.
 *
 * @param object_p - Ptr to object to clear the condition on.
 *
 * @return fbe_status_t
 *
 * HISTORY:
 *  10/21/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_clear_condition(struct fbe_base_object_s * object_p)
{    
    return(fbe_lifecycle_clear_current_cond(object_p));
}
/**************************************
 * end fbe_ldo_clear_condition()
 **************************************/

/*!***************************************************************
 * @fn fbe_ldo_set_condition(fbe_lifecycle_const_t * class_const_p,
 *                           struct fbe_base_object_s * object_p,
 *                           fbe_lifecycle_cond_id_t cond_id)
 ****************************************************************
 * @brief
 *  This is the function for the setting of a condition.
 * 
 *  In Kernel mode we simply call the underlying lifecycle
 *  function to set the condition
 * 
 * @param class_const_p - Class constant pointer.
 * @param object_p - Ptr to object to set the condition on.
 * @param cond_id - The condition id being set.
 *
 * @return fbe_status_t
 *  
 *
 * HISTORY:
 *  10/21/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_set_condition(fbe_lifecycle_const_t * class_const_p,
                                                 struct fbe_base_object_s * object_p,
                                                 fbe_lifecycle_cond_id_t cond_id)
{    
    return(fbe_lifecycle_set_cond(class_const_p,
                                  object_p,
                                  cond_id));
}
/**************************************
 * end fbe_ldo_set_condition()
 **************************************/

/*!***************************************************************
 * @fn fbe_ldo_io_cmd_allocate(
 *               fbe_packet_t * const packet_p,
 *               fbe_memory_completion_function_t completion_function,
 *               fbe_memory_completion_context_t completion_context)
 ****************************************************************
 * @brief
 *  This function allocates an io command for the given logical drive.
 *  Since this is the kernel version we call the actual implementation
 *  of this function.
 *
 * @param packet_p - Memory request to allocate with.
 * @param completion_function - Fn to call when mem is available.
 * @param completion_context - Context to use when mem is available.
 *
 * @return
 *  FBE_STATUS_OK - This function will never fail.
 *                  Our completion function will always be called.
 *
 * HISTORY:
 *  11/06/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_io_cmd_allocate( fbe_packet_t * const packet_p,
                                      fbe_memory_completion_function_t completion_function,
                                      fbe_memory_completion_context_t completion_context)
{
    fbe_status_t status;

    /* Just call the allocate function.
     */
    status = fbe_ldo_allocate_io_cmd(packet_p, completion_function, completion_context);
    return status;
}
/**************************************
 * end fbe_ldo_io_cmd_allocate()
 **************************************/

 /*************************
  * end file fbe_logical_drive_kernel.c
  *************************/
 
 
