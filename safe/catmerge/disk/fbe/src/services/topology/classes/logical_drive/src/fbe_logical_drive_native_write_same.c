/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * fbe_logical_drive_write_same.c
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains functions of the logical drive object's native write same
 *  state machine.
 * 
 *  A native write same is a write same command where we will be able to issue
 *  a single write same command to the next object.
 *
 * HISTORY
 *   02/11/2008 Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_logical_drive.h"
#include "fbe_logical_drive_private.h"
#include "fbe_logical_drive_io_command.h"


/****************************************
 * LOCALLY DEFINED FUNCTIONS
 ****************************************/

static void 
fbe_ldo_io_cmd_native_ws_setup_sub_io_packet(fbe_logical_drive_io_cmd_t * const io_cmd_p);
static fbe_status_t
fbe_ldo_io_cmd_native_ws_count_sgs(fbe_logical_drive_io_cmd_t * const io_cmd_p);
fbe_ldo_state_status_enum_t 
fbe_ldo_io_cmd_native_write_same_state0(fbe_logical_drive_io_cmd_t * const io_cmd_p);
fbe_ldo_state_status_enum_t 
fbe_ldo_io_cmd_native_write_same_state10(fbe_logical_drive_io_cmd_t * const io_cmd_p);
fbe_ldo_state_status_enum_t 
fbe_ldo_io_cmd_native_write_same_state20(fbe_logical_drive_io_cmd_t * const io_cmd_p);

/****************************************************************
 * fbe_ldo_io_cmd_native_write_same_state0()
 ****************************************************************
 * DESCRIPTION:
 *  This function handles generating a native write same command.
 *
 * PARAMETERS:
 *  io_cmd_p, [I] - The logical drive io cmd object to execute.
 *
 * RETURNS:
 *  Always returns FBE_LDO_STATE_STATUS_EXECUTING.
 *
 * HISTORY:
 *  02/08/08 - Created. RPF
 *
 ****************************************************************/
fbe_ldo_state_status_enum_t 
fbe_ldo_io_cmd_native_write_same_state0(fbe_logical_drive_io_cmd_t * const io_cmd_p)
{
    /* First count how many sgs we need.
     */
    if ( fbe_ldo_io_cmd_native_ws_count_sgs(io_cmd_p) != FBE_STATUS_OK )
    {
        /* Something is wrong with the input sg.
         * Return error now.
         */
        fbe_ldo_io_cmd_set_state(io_cmd_p, fbe_ldo_io_cmd_illegal_command);
        return FBE_LDO_STATE_STATUS_EXECUTING;
    }
    else 
    {
        /* Transition to the next state.
         */
        fbe_ldo_io_cmd_set_state(io_cmd_p, fbe_ldo_io_cmd_native_write_same_state10);
    
        /* Now allocate the sg list and wait for it if necessary.
          */
        fbe_ldo_io_cmd_allocate_sg(io_cmd_p, 
                                   fbe_ldo_io_cmd_get_sg_entries(io_cmd_p));
    }
    
    /* Wait for the next state to get executed. 
     */
    return FBE_LDO_STATE_STATUS_WAITING;
}
/* end fbe_ldo_io_cmd_native_write_same_state0() */

/****************************************************************
 * fbe_ldo_io_cmd_native_write_same_state10()
 ****************************************************************
 * DESCRIPTION:
 *  This function handles setting up the sg list and sub io packet
 *  for the native write same, and then sending out the
 *  sub io packet.
 *
 * PARAMETERS:
 *  io_cmd_p, [I] - The logical drive io cmd object to execute.
 *
 * RETURNS:
 *  Always returns FBE_LDO_STATE_STATUS_EXECUTING.
 *
 * HISTORY:
 *  02/11/08 - Created. RPF
 *
 ****************************************************************/
fbe_ldo_state_status_enum_t 
fbe_ldo_io_cmd_native_write_same_state10(fbe_logical_drive_io_cmd_t * const io_cmd_p)
{
    fbe_logical_drive_t * logical_drive_p = fbe_ldo_io_cmd_get_logical_drive( io_cmd_p );
    fbe_block_size_t imported_block_size = fbe_ldo_get_imported_block_size(logical_drive_p);
    fbe_block_size_t exported_block_size = fbe_ldo_io_cmd_get_block_size( io_cmd_p );

    /* Transition to the next state.
     */
    fbe_ldo_io_cmd_set_state(io_cmd_p, fbe_ldo_io_cmd_native_write_same_state20);

    /* Setup the sgs using the standard write same setup method.
     * This uses the io command's write_same.sg_bytes field to know how much 
     * to place into the sg. 
     */
    fbe_ldo_io_cmd_wr_same_setup_sg(io_cmd_p);

    /* Setup our sub io packet.
     */
    fbe_ldo_io_cmd_native_ws_setup_sub_io_packet(io_cmd_p);

    /* Send out the io packet.  Our callback always gets executed, thus
     * there is no status to handle here.
     */
    fbe_ldo_io_cmd_send_io(io_cmd_p);

    /* Wait for the next state to get executed. 
     */
    return FBE_LDO_STATE_STATUS_WAITING;
}
/* end fbe_ldo_io_cmd_native_write_same_state10() */

/****************************************************************
 * fbe_ldo_io_cmd_native_write_same_state20()
 ****************************************************************
 * DESCRIPTION:
 *  This function handles sending status on a native write same.
 *
 * PARAMETERS:
 *  io_cmd_p, [I] - The logical drive io cmd object to execute.
 *
 * RETURNS:
 *  Always returns FBE_LDO_STATE_STATUS_EXECUTING.
 *
 * HISTORY:
 *  02/11/08 - Created. RPF
 *
 ****************************************************************/
fbe_ldo_state_status_enum_t 
fbe_ldo_io_cmd_native_write_same_state20(fbe_logical_drive_io_cmd_t * const io_cmd_p)
{
    /* Transition to the finished state.
     */
    fbe_ldo_io_cmd_set_state(io_cmd_p, fbe_ldo_io_cmd_finished);

    /* The operation has completed, set the status in the master io packet
     * from the status in the sub io packet.
     */
    fbe_ldo_io_cmd_set_status_from_sub_io_packet(io_cmd_p);
    
    /* Return executing, so we can execute the finished state.
     */
    return FBE_LDO_STATE_STATUS_EXECUTING;
}
/* end fbe_ldo_io_cmd_native_write_same_state20() */

/****************************************************************
 * fbe_ldo_io_cmd_native_ws_setup_sub_io_packet()
 ****************************************************************
 * DESCRIPTION:
 *  This function initializes our sub io packet for a native
 *  write same request.
 *
 * PARAMETERS:
 *  io_cmd_p, [I] - The logical drive io cmd object to execute.
 *
 * RETURNS:
 *  None.
 *
 * HISTORY:
 *  02/11/08 - Created. RPF
 *
 ****************************************************************/
static void 
fbe_ldo_io_cmd_native_ws_setup_sub_io_packet(fbe_logical_drive_io_cmd_t * const io_cmd_p)
{
    fbe_logical_drive_t * logical_drive_p = fbe_ldo_io_cmd_get_logical_drive( io_cmd_p );
    fbe_lba_t current_lba;
    fbe_block_count_t current_blocks;

    /* Simply map the given parameters from the io command from
     * the exported to the imported block size.
     */
    fbe_ldo_map_lba_blocks(fbe_ldo_io_cmd_get_lba( io_cmd_p ),
                           fbe_ldo_io_cmd_get_blocks( io_cmd_p ),
                           fbe_ldo_io_cmd_get_block_size( io_cmd_p ),
                           fbe_ldo_io_cmd_get_optimum_block_size(io_cmd_p),
                           fbe_ldo_get_imported_block_size(logical_drive_p),
                           fbe_ldo_io_cmd_get_imported_optimum_block_size(io_cmd_p),
                           &current_lba,
                           &current_blocks);

    /* Set the parameters for the write same request.
     * We will use these as we finish pieces of the request to determine if we 
     * are done. 
     */
    fbe_ldo_io_cmd_set_ws_current_lba(io_cmd_p, current_lba);
    fbe_ldo_io_cmd_set_ws_current_blks(io_cmd_p, current_blocks);
    fbe_ldo_io_cmd_set_ws_blks_remaining(io_cmd_p, current_blocks);

    /* Then initialize the generic fields in our io packet.
     */
    fbe_ldo_io_cmd_build_sub_packet(io_cmd_p,
                                    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME,
                                    current_lba,
                                    current_blocks);

    return;
}
/* end fbe_ldo_io_cmd_native_ws_setup_sub_io_packet() */

/****************************************************************
 * fbe_ldo_io_cmd_native_ws_count_sgs()
 ****************************************************************
 * DESCRIPTION:
 *  This function initializes our sub io packet for a native
 *  write same request.
 *
 * PARAMETERS:
 *  io_cmd_p, [I] - The logical drive io cmd object to execute.
 *
 * RETURNS:
 *  None.
 *
 * HISTORY:
 *  02/11/08 - Created. RPF
 *
 ****************************************************************/
static fbe_status_t
fbe_ldo_io_cmd_native_ws_count_sgs(fbe_logical_drive_io_cmd_t * const io_cmd_p)
{
    fbe_logical_drive_t * logical_drive_p = fbe_ldo_io_cmd_get_logical_drive( io_cmd_p );
    fbe_block_size_t imported_block_size = fbe_ldo_get_imported_block_size(logical_drive_p);
    fbe_block_size_t exported_block_size = fbe_ldo_io_cmd_get_block_size( io_cmd_p );
    fbe_sg_element_t *client_sg_p = fbe_ldo_io_cmd_get_client_sg_ptr(io_cmd_p);
    fbe_u32_t sg_entries;

    /* Count the number of sgs required for a single exported block.
     */
    fbe_u32_t sg_elements_per_block = 
        fbe_sg_element_count_entries_for_bytes(client_sg_p, exported_block_size);

    if (sg_elements_per_block == 0)
    {
        /* Something is wrong, we did not find any sgs.
         */
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* The number of sgs we need will be however many sgs it takes to replicate
     * the single exported block to fill the imported block.
     */
    sg_entries = (fbe_u32_t)(imported_block_size / exported_block_size);

    /* Also take into account if each block is more than one sg element.
     */
    sg_entries *= sg_elements_per_block;

    /* Add one for the null terminator.
     */
    sg_entries++;
    fbe_ldo_io_cmd_set_sg_entries(io_cmd_p, sg_entries);

    /* Our sg list will have a single imported block.
     */
    fbe_ldo_io_cmd_set_ws_sg_bytes(io_cmd_p, imported_block_size);

    return FBE_STATUS_OK;
}
/* end fbe_ldo_io_cmd_native_ws_count_sgs() */

/************************************************************
 * end file fbe_logical_drive_native_write_same.c
 ***********************************************************/
