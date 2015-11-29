/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_private_space_layout.c
 *
 * @brief
 *  This file describes the Private Space Layout, and contains functions
 *  used to access it.
 *
 * @version
 *   2011-04-27 - Created. Matthew Ferson
 *
 ***************************************************************************/

/*
 * INCLUDE FILES
 */
#include "fbe/fbe_disk_block_correct.h"
#include "fbe/fbe_api_sim_transport_packet_interface.h"

static fbe_bool_t is_destroy_needed = FBE_TRUE;

/*!**************************************************************
 * fbe_disk_block_correct_initialize_sim()
 ****************************************************************
 * @brief
 *  Initialize the fbe_api
 *
 * @param 
 *  bb_command - command specified by the user
 *
 * @return 
 *  status - fbe status
 *
 ****************************************************************/
fbe_status_t fbe_disk_block_correct_initialize_sim(fbe_bad_blocks_cmd_t * bb_command)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_sim_transport_connection_target_t       sp_to_connect = FBE_SIM_SP_A;

    /*initialize the simulation side of fbe api*/
    fbe_api_common_init_sim();

    /*set function entries for commands that would go to the physical package*/
    fbe_api_set_simulation_io_and_control_entries (FBE_PACKAGE_ID_PHYSICAL,
                                                   fbe_api_sim_transport_send_io_packet,
                                                   fbe_api_sim_transport_send_client_control_packet);

    /*set function entries for commands that would go to the sep package*/
    fbe_api_set_simulation_io_and_control_entries (FBE_PACKAGE_ID_SEP_0,
                                                   fbe_api_sim_transport_send_io_packet,
                                                   fbe_api_sim_transport_send_client_control_packet);

	/*set function entries for commands that would go to the esp package*/
    fbe_api_set_simulation_io_and_control_entries (FBE_PACKAGE_ID_ESP,
                                                   fbe_api_sim_transport_send_io_packet,
                                                   fbe_api_sim_transport_send_client_control_packet);

	/*set function entries for commands that would go to the neit package*/
    fbe_api_set_simulation_io_and_control_entries (FBE_PACKAGE_ID_NEIT,
                                                   fbe_api_sim_transport_send_io_packet,
                                                   fbe_api_sim_transport_send_client_control_packet);

    /*need to initialize the client to connect to the server*/
    sp_to_connect = FBE_SIM_SP_A;
    fbe_api_sim_transport_reset_client(FBE_CLIENT_MODE_DEV_PC);
    fbe_api_sim_transport_set_target_server(sp_to_connect);
    status = fbe_api_sim_transport_init_client(sp_to_connect, FBE_TRUE);/*connect w/o any notifications enabled*/
    if (status != FBE_STATUS_OK) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "Can't connect to FBE BAD BLOCKS Server, make sure FBE is running !!!");
        return status;
    }

    return status;
}

/*!**************************************************************
 * fbe_disk_block_correct_destroy_fbe_api_sim()
 ****************************************************************
 * @brief
 *  Destroy the fbe_api
 *
 * @param 
 *  in - dummy parameter
 *
 * @return 
 *  status - fbe status
 *
 ****************************************************************/
void __cdecl fbe_disk_block_correct_destroy_fbe_api_sim(fbe_u32_t in)
{
    FBE_UNREFERENCED_PARAMETER(in);

	fbe_api_trace(FBE_TRACE_LEVEL_INFO,"\nDestroying fbe api...");

    if(is_destroy_needed)
    {
        /* Set this to FALSE when we call this function */
        is_destroy_needed = FBE_FALSE;
        /* destroy job notification must be done before destroy client,
         * since it uses the socket connection. */
        fbe_api_common_destroy_job_notification();
        fbe_api_sim_transport_destroy_client(FBE_SIM_SP_A);
        fbe_api_common_destroy_sim();
        fflush(stdout);
    }

    return;
}


