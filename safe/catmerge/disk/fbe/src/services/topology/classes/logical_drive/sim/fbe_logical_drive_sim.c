 /***************************************************************************
  * Copyright (C) EMC Corporation 2008
  * All rights reserved.
  * Licensed material -- property of EMC Corporation
  ***************************************************************************/
 
 /*!**************************************************************************
  * @file fbe_logical_drive_sim.c
  ***************************************************************************
  *
  * @brief
  *  This file contains sim mode only simulation functions.
  * 
  *  We need this file since we need the capability to mock certain functions
  *  when we are testing with MUT tests in sim mode.
  * 
  *  The interface to these functions like fbe_ldo_send_io_packet() or
  *  fbe_ldo_complete_packet() is the same for both sim and kernel mode.
  * 
  *  However, in sim mode we implement a function table.  The interface
  *  function like fbe_ldo_send_io_packet() simply calls the function in the
  *  function table.
  * 
  *  We also implement methods that allow us to change the function table.
  *  This allows tests in sim mode to substitute a mock implementation
  *  for these functions.  This allow us to unit test code that has dependencies
  *  (on functions like send packet or complete packet) in complete isolation.
  * 
  *  The mocks allow us to validate that the correct behavior has occurred
  *  without needing to bring up the entire system.
  *
  * @date 7/23/2008
  * @author RPF
  *
  ***************************************************************************/
 
 /*************************
  *   INCLUDE FILES
  *************************/
 
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_logical_drive.h"
#include "fbe_logical_drive_private.h"
#include "fbe_logical_drive_io_command.h"

 /*************************
  *   FUNCTION DEFINITIONS
  *************************/

fbe_status_t fbe_ldo_send_io_packet_default(fbe_logical_drive_t * const logical_drive_p,
                                            fbe_packet_t *io_packet_p);
fbe_status_t fbe_ldo_complete_io_packet_default(fbe_packet_t * packet);
fbe_status_t fbe_ldo_set_condition_default(fbe_lifecycle_const_t * class_const_p,
                                           struct fbe_base_object_s * object_p,
                                           fbe_lifecycle_cond_id_t cond_id);
fbe_status_t fbe_ldo_clear_condition_default(struct fbe_base_object_s * object_p);

/*!*********************************************************** 
 * @struct fbe_logical_drive_methods_s
 *  
 *   Logical drive methods.
 *  
 *  @brief This is the table of methods, which are used by the
 *         logical drive. This table of methods is used in
 *         the simulation environment in order to allow us to
 *         setup mocks or alternate handling for different functions.
 ************************************************************/

struct fbe_logical_drive_methods_s
{
    /*! Method to send an io packet to our edge. 
     */
    fbe_ldo_send_io_packet_fn send_io_packet;

    /*! Method to complete the io packet.
     */
    fbe_ldo_complete_io_packet_fn complete_io_packet;

    /*! Method to allocate a logical drive io cmd. 
     */
    fbe_ldo_allocate_io_cmd_fn allocate_io_cmd;

    /*! Method to free a logical drive io cmd. 
     */
    fbe_ldo_free_io_cmd_fn free_io_cmd;

    /*! Method to clear a lifecycle condition.
     */
    fbe_ldo_clear_condition_fn clear_condition;

    /*! Method to set a lifecycle condition.
     */
    fbe_ldo_set_condition_fn set_condition;
};
typedef struct fbe_logical_drive_methods_s fbe_logical_drive_methods_t;

/*! @def FBE_LOGICAL_DRIVE_DEFAULT_METHODS 
 *  
 *  @details This is the define for the default set of logical drive methods. 
 */
#define FBE_LOGICAL_DRIVE_DEFAULT_METHODS\
    fbe_ldo_send_io_packet_default,\
    fbe_ldo_complete_io_packet_default,\
    fbe_ldo_allocate_io_cmd,\
    fbe_ldo_free_io_cmd,\
    fbe_ldo_clear_condition_default,\
    fbe_ldo_set_condition_default

/*! @var fbe_logical_drive_methods_t fbe_logical_drive_methods
 *  
 * This is our current set of function pointers. 
 * These can be changed during unit testing to mock certain operations. 
 */
fbe_logical_drive_methods_t fbe_logical_drive_methods = 
{FBE_LOGICAL_DRIVE_DEFAULT_METHODS};

/*! @var const fbe_logical_drive_methods_t fbe_logical_drive_default_methods
 *  
 *   This is our default set of function pointers. We will use these to reset
 *   the fbe_logical_drive_methods when needed during testing.
 */
const fbe_logical_drive_methods_t fbe_logical_drive_default_methods = 
{FBE_LOGICAL_DRIVE_DEFAULT_METHODS};


/*! @fn fbe_ldo_set_send_io_packet_fn(
 *                                  const fbe_ldo_send_io_packet_fn send_io_packet_fn)
 *  
 *  @brief This allows us to set the send io packet function in simulation.
 *  
 *  @param send_io_packet_fn - Function to substitute into the methods table.
 */
void
fbe_ldo_set_send_io_packet_fn(const fbe_ldo_send_io_packet_fn send_io_packet_fn)
{
    fbe_logical_drive_methods.send_io_packet = send_io_packet_fn;
    return;
}
/* end fbe_ldo_set_send_io_packet_fn()   */

/*! @fn fbe_ldo_get_send_io_packet_fn(void)
 *  
 *  @brief This allows us to get the send io packet function in simulation.
 *  
 *  @return fbe_ldo_send_io_packet_fn - Our current send io packet function.
 */
fbe_ldo_send_io_packet_fn fbe_ldo_get_send_io_packet_fn(void)
{
     return fbe_logical_drive_methods.send_io_packet;
}
/* end fbe_ldo_get_send_io_packet_fn()   */

/*! @fn fbe_ldo_set_complete_io_packet_fn(
 *                                  const fbe_ldo_complete_io_packet_fn complete_io_packet_fn)
 *  
 *  @brief This allows us to set the complete io packet function in simulation.
 *  
 *  @param complete_io_packet_fn - Function to substitute into the methods table.
 */
void
fbe_ldo_set_complete_io_packet_fn(const fbe_ldo_complete_io_packet_fn complete_io_packet_fn)
{
    fbe_logical_drive_methods.complete_io_packet = complete_io_packet_fn;
    return;
}
/* end fbe_ldo_set_complete_io_packet_fn()   */

/*! @fn fbe_ldo_get_complete_io_packet_fn(void)
 *  
 *  @brief This allows us to get the complete io packet function in simulation.
 *  
 *  @return fbe_ldo_complete_io_packet_fn - Our current complete io packet function.
 */
fbe_ldo_complete_io_packet_fn fbe_ldo_get_complete_io_packet_fn(void)
{
     return fbe_logical_drive_methods.complete_io_packet;
}
/* end fbe_ldo_get_complete_io_packet_fn()   */

/*! @fn fbe_ldo_set_allocate_io_cmd_fn(
 *                           const fbe_ldo_allocate_io_cmd_fn allocate_io_cmd_fn)
 *  
 *  @brief This allows us to set the allocate io command function in simulation.
 *  
 *  @param allocate_io_cmd_fn - Function to substitute into the methods table.
 */
void
fbe_ldo_set_allocate_io_cmd_fn(const fbe_ldo_allocate_io_cmd_fn allocate_io_cmd_fn)
{
    fbe_logical_drive_methods.allocate_io_cmd = allocate_io_cmd_fn;
    return;
}
/* end fbe_ldo_set_allocate_io_cmd_fn()   */

/*! @fn fbe_ldo_get_allocate_io_cmd_fn(void)
 *  
 *  @brief This allows us to get the allocate io command function in simulation.
 *  
 *  @return fbe_ldo_allocate_io_cmd_fn - Our current allocate io command
 *                                       function.
 */
fbe_ldo_allocate_io_cmd_fn fbe_ldo_get_allocate_io_cmd_fn(void)
{
     return fbe_logical_drive_methods.allocate_io_cmd;
}
/* end fbe_ldo_get_allocate_io_cmd_fn()   */

/*! @fn fbe_ldo_set_free_io_cmd_fn(
 *                           const fbe_ldo_free_io_cmd_fn free_io_cmd_fn)
 *  
 *  @brief This allows us to set the free io command function in simulation.
 *  
 *  @param free_io_cmd_fn - Function to substitute into the methods table.
 */
void
fbe_ldo_set_free_io_cmd_fn(const fbe_ldo_free_io_cmd_fn free_io_cmd_fn)
{
    fbe_logical_drive_methods.free_io_cmd = free_io_cmd_fn;
    return;
}
/* end fbe_ldo_set_free_io_cmd_fn()   */

/*! @fn fbe_ldo_get_free_io_cmd_fn(void)
 *  
 *  @brief This allows us to get the free io command function in simulation.
 *  
 *  @return fbe_ldo_free_io_cmd_fn - Our current free io command function.
 */
fbe_ldo_free_io_cmd_fn fbe_ldo_get_free_io_cmd_fn(void)
{
     return fbe_logical_drive_methods.free_io_cmd;
}
/* end fbe_ldo_get_free_io_cmd_fn()   */

/*! @fn fbe_ldo_set_clear_condition_fn(
 *                           const fbe_ldo_clear_condition_fn clear_condition_fn)
 *  
 *  @brief This allows us to set the clear condition function in simulation.
 *  
 *  @param clear_condition_fn - Function to substitute into the methods table.
 */
void
fbe_ldo_set_clear_condition_fn(const fbe_ldo_clear_condition_fn clear_condition_fn)
{
    fbe_logical_drive_methods.clear_condition = clear_condition_fn;
    return;
}
/* end fbe_ldo_set_clear_condition_fn()  */

/*! @fn fbe_ldo_get_clear_condition_fn(void)
 *  
 *  @brief This allows us to get the clear condition function in simulation.
 *  
 *  @return fbe_ldo_clear_condition_fn - Our current clear condition function.
 */
fbe_ldo_clear_condition_fn fbe_ldo_get_clear_condition_fn(void)
{
     return fbe_logical_drive_methods.clear_condition;
}
/* end fbe_ldo_get_clear_condition_fn() */

/*! @fn fbe_ldo_set_condition_set_fn(
 *                           const fbe_ldo_set_condition_fn set_condition_fn)
 *  
 *  @brief This allows us to set the set condition function in simulation.
 *  
 *  @param set_condition_fn - Function to substitute into the methods table.
 */
void
fbe_ldo_set_condition_set_fn(const fbe_ldo_set_condition_fn set_condition_fn)
{
    fbe_logical_drive_methods.set_condition = set_condition_fn;
    return;
}
/* end fbe_ldo_set_condition_set_fn() */

/*! @fn fbe_ldo_get_set_condition_fn(void)
 *  
 *  @brief This allows us to get the set condition function in simulation.
 *  
 *  @return fbe_ldo_set_condition_fn - Our current set condition function.
 */
fbe_ldo_set_condition_fn fbe_ldo_get_set_condition_fn(void)
{
     return fbe_logical_drive_methods.set_condition;
}
/* end fbe_ldo_get_set_condition_fn() */

/*!***************************************************************
 * @fn fbe_logical_drive_reset_methods
 ****************************************************************
 * @brief
 *  This function is called to reset the methods to the defaults. 
 *
 * PARAMETERS:
 *  none.
 *
 * RETURNS:
 *  none
 *
 * HISTORY:
 *  06/12/08 - Created. RPF
 *
 ****************************************************************/
void fbe_logical_drive_reset_methods(void)
{
    /* Simply reset the methods to what they are initially.
     */
    fbe_logical_drive_methods = fbe_logical_drive_default_methods;
    return;
}
/* end fbe_logical_drive_reset_methods() */

/****************************************************************
 * fbe_logical_drive_initialize_for_test()
 ****************************************************************
 * DESCRIPTION:
 *  Initialize the logical drive for use in a testing environment
 *  where the physical package is not present.
 *
 * PARAMETERS:
 *  logical_drive_p, [I] - The object being initialized.
 *
 * RETURNS:
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * HISTORY:
 *  06/11/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_logical_drive_initialize_for_test(fbe_logical_drive_t * const logical_drive_p)
{
    /* Initialize the base class.
     * We cannot call the base class since it depends on things in the FBE infrastructure 
     * which we do not setup for this unit test. 
     */
    fbe_spinlock_init(&logical_drive_p->base_discovered.base_object.object_lock);
    fbe_spinlock_init(&logical_drive_p->base_discovered.base_object.terminator_queue_lock);
    fbe_queue_init(&logical_drive_p->base_discovered.base_object.terminator_queue_head);

    /* Simply call our init function.
     */
    fbe_logical_drive_init_members(logical_drive_p);

    return FBE_STATUS_OK;
}
/* end fbe_logical_drive_initialize_for_test() */

/****************************************************************
 * fbe_ldo_setup_defaults()
 ****************************************************************
 * DESCRIPTION:
 *  This function initializes the logical drive object's
 *  function table.
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
    /* Just set the methods ptr to the global instance of methods for this object.
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
    fbe_ldo_send_io_packet_fn send_packet_fn = fbe_ldo_get_send_io_packet_fn();

    /* Send out the io packet.  Our callback always gets executed.
     */
    status = (send_packet_fn)(logical_drive_p, packet_p);

    /* We just return the status.  Our io packet is guaranteed
     * to be completed even if we fail sending.
     */
    return status;
}
/* end fbe_ldo_send_io_packet() */

/****************************************************************
 * fbe_ldo_send_io_packet_default()
 ****************************************************************
 * DESCRIPTION:
 *  This function sends an IO packet.  This is the default implementation
 *  of this function.
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
fbe_status_t fbe_ldo_send_io_packet_default(fbe_logical_drive_t * const logical_drive_p,
                                            fbe_packet_t *packet_p)
{
    fbe_status_t status;

    /* Send out the io packet. Send this to our only edge.
     */
    status = fbe_block_transport_send_functional_packet(&logical_drive_p->block_edge, 
                                                        packet_p);
    return status;
}
/* end fbe_ldo_send_io_packet_default() */

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
    fbe_ldo_free_io_cmd_fn free_fn = fbe_ldo_get_free_io_cmd_fn();

    /* Just call the free function.
     */
    (free_fn)(io_cmd_p);
    return;
}
/* end fbe_ldo_io_cmd_free() */

/*!***************************************************************
 * @fn fbe_ldo_complete_io_packet_default(fbe_packet_t * packet_p)
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
fbe_ldo_complete_io_packet_default(fbe_packet_t * packet_p)
{
    return fbe_transport_complete_packet(packet_p);
}
/**************************************
 * end fbe_ldo_complete_io_packet_default()
 **************************************/

/*!***************************************************************
 * @fn fbe_ldo_complete_packet(fbe_packet_t * packet_p)
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
    fbe_ldo_complete_io_packet_fn complete_packet_fn = fbe_ldo_get_complete_io_packet_fn();
    fbe_status_t status;

    status = (complete_packet_fn)(packet_p);

    return status;
}
/**************************************
 * end fbe_ldo_complete_packet()
 **************************************/

/*!***************************************************************
 * @fn fbe_ldo_clear_condition_default(struct fbe_base_object_s * object_p)
 ****************************************************************
 * @brief
 *  This function clears a condition.
 * 
 *  This is the default function that we place into the
 *  methods table for sim.
 *
 * @param object_p - Ptr to object to clear the condition on.
 *
 * @return fbe_status_t
 *
 * HISTORY:
 *  10/21/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_clear_condition_default(struct fbe_base_object_s * object_p)
{    
    return(fbe_lifecycle_clear_current_cond(object_p));
}
/**************************************
 * end fbe_ldo_clear_condition_default()
 **************************************/

/*!***************************************************************
 * @fn fbe_ldo_set_condition_default(
 *                           fbe_lifecycle_const_t * class_const_p,
 *                           struct fbe_base_object_s * object_p,
 *                           fbe_lifecycle_cond_id_t cond_id)
 ****************************************************************
 * @brief
 *  This is the function for the setting of a condition.
 * 
 *  This is the default function that we place into the
 *  methods table for sim.
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
fbe_status_t 
fbe_ldo_set_condition_default(fbe_lifecycle_const_t * class_const_p,
                              struct fbe_base_object_s * object_p,
                              fbe_lifecycle_cond_id_t cond_id)
{    
    return(fbe_lifecycle_set_cond(class_const_p,
                                  object_p,
                                  cond_id));
}
/**************************************
 * end fbe_ldo_set_condition_default()
 **************************************/

/*!***************************************************************
 * @fn fbe_ldo_clear_condition(struct fbe_base_object_s * object_p)
 ****************************************************************
 * @brief
 *  This function clears a condition.
 * 
 *  For user mode we call the method that was already setup.
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
    fbe_ldo_clear_condition_fn clear_cond_fn = fbe_ldo_get_clear_condition_fn();
    fbe_status_t status;

    status = (clear_cond_fn)(object_p);

    return status;
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
 *  For user mode we call the method that was already setup.
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
    fbe_ldo_set_condition_fn set_cond_fn = fbe_ldo_get_set_condition_fn();
    fbe_status_t status;

    status = (set_cond_fn)(class_const_p, object_p, cond_id);

    return status;
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
 *  Since this is the simulation version we call the mock for
 *  this function.
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
    fbe_ldo_allocate_io_cmd_fn allocate_fn = fbe_ldo_get_allocate_io_cmd_fn();
    fbe_status_t status;

    /* Just call the allocate function.
     */
    status = (allocate_fn)(packet_p, completion_function, completion_context);
    return status;
}
/**************************************
 * end fbe_ldo_io_cmd_allocate()
 **************************************/

 /*************************
  * end file fbe_logical_drive_sim.c
  *************************/
 
 
