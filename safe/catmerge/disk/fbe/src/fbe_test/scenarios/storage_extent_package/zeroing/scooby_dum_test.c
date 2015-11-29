/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file scooby_dum_test.c
 ***************************************************************************
 *
 * @brief
 *   This file contains tests of non configured drive zeroing.
 *
 * @version
 *   01/06/2010 - Created. Amit Dhaduk
 *
 ***************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "fbe_test_package_config.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_sim_server.h"

#include "fbe_test_common_utils.h"
#include "fbe_test_configurations.h"
#include "pp_utils.h"
#include "sep_utils.h"
#include "sep_zeroing_utils.h"
#include "sep_tests.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_physical_drive_interface.h"

//-----------------------------------------------------------------------------
//  TEST DESCRIPTION:

char* scooby_dum_short_desc = "Disk Zeroing background operation test";

char* scooby_dum_long_desc ="\n\n\
This scenario validates disk zeroing background operation for unbound drive in\n\
single and dual SP configuration.\n\n\
Single SP test:\n\n\
STEP 1: Bring up the system with default configuration.\n\
STEP 2: Initiate disk zeroing with sending usurper command.\n\
STEP 3: Following steps execute by disk zeroing background operation.\n\
        - Read zero checkpoint.\n\
        - Send permission request to upstream object if it is there. For unbound pvd, there is no upstream objects so it does\n\
          not required to send permission request.\n\
        - Read PVD's bitmap data and determine if needs to perform zero IO or not.\n\
           + If no need to perform zero IO then update the paged metadata and increment the zero checkpoint.\n\
           + If required to perform zero IO then send zero IO request to downstream object.\n\
                ++ When zero IO gets complete, perform paged metadata update and increment the zero checkpoint.\n\
        - When zero checkpoint reach at PVD's export capacity then it perform following:\n\
           +  Set zero checkpoint to end marker(0xFFFFFFFF)\n\
           +  Clear the PVD's zero on demand state.\n\
        - Continue above steps until zero checkpoint reach at PVD's export capacity.\n\
STEP 4:  Verify that zero checkpoint is set to 0xFFFFFFFF and complete the test.\n\n\
Dual SP test:\n\n\
STEP 1:\n\
- Start disk zeroing and wait for completion when Active and Passive both SPs are running.\n\
- Check that disk zeroing completed and zero checkpoint set to 0xFFFFFFFF on both SPs.\n\
STEP 2:\n\
- Start disk zeroing on active sp.\n\
- When disk zeroing is in progress on active sp, shutdown the active SP.\n\
- Check that the other SP which is up and running continue disk zeroing and complete it. \n\
\n\
Test Specific Switches:\n\
          - none.\n\
Outstanding Issues:[TODO]\n\
        - Need to add following test cases for system initiated disk zeroing\n\
          + When system come up and found that zero checkpoint is valid, disk zeroing should start and complete\n\
            for following use cases\n\
                - system drives\n\
                - unbound non system drives\n\
                - 512 block size SAS drives\n\
                - 520 block size SAS drives\n\
        - Need to add following test cases for zero_disk user command to start disk zeroing background operation\n\
          + Perform zero_disk command from passive sp\n\
          + Perform zero_disk command to bound pvd and it should fail\n\
          + Perform zero_disk command to 512 block size unbound SAS drive\n\
        - Optimization\n\
          During disk zero background operation, if there is no need to perform zero IO and no need to perform\n\
          paged metadata update then we can skip perform update paged metadata step.\n\
        - Need some code cleanup once background zeroing by default enable in the tree.\n\
\n\
Description last updated: 11/10/2011\n";

/*!*******************************************************************
 * @var scooby_dum_disk_set
 *********************************************************************
 * @brief disk set to perform disk zeroing 
 *********************************************************************/
fbe_test_raid_group_disk_set_t scooby_dum_disk_set[] = {

    {0,0,5}, {0,0,6}, {0,0,7},
};

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/

#define SYSTEM_DRIVE_UNCONSUMED_END     0x10000
#define CHUNK_SIZE                      0x800
#define PVD_OBJ_ID_FOR_DRIVE_0           0x1

#define SCOOBY_DUM_WAIT_MSEC          50000

static fbe_api_rdgen_context_t scoobydum_context[1];

/*****************************************
 * LOCAL FUNCTIONS
 *****************************************/
static void scooby_dum_send_io(fbe_object_id_t   pvd_object_id, fbe_api_rdgen_context_t* context_p,
                               fbe_rdgen_operation_t rdgen_operation);

void scooby_dum_system_drive_zeroing(fbe_object_id_t  pvd_object_id);
static void scooby_dum_setup_zeroing_hook(void);
fbe_status_t scooby_dum_wait_for_target_SP_hook(fbe_object_id_t pvd_object_id, 
                                                fbe_u32_t state, 
                                                fbe_u32_t substate);
fbe_status_t scooby_dum_delete_debug_hook(fbe_object_id_t pvd_object_id, 
                                          fbe_u32_t state, 
                                          fbe_u32_t substate);
                               
/*!****************************************************************************
 *  scooby_dum_setup
 ******************************************************************************
 *
 * @brief
 *   This is the setup function for the scooby_dum test.  
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void scooby_dum_setup(void)
{    
    mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for Scooby Dum test ===\n");

    if (fbe_test_util_is_simulation())
    {
       /* load the physical package */
        zeroing_physical_config();
        
        scooby_dum_setup_zeroing_hook();
        //sep_config_load_sep();
    }

    return;
}

/***************************************************************
 * end scooby_dum_setup()
 ***************************************************************/

/*!****************************************************************************
 *  scooby_dum_dualsp_setup
 ******************************************************************************
 *
 * @brief
 *   This is the setup function for the scooby_dum test.  
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void scooby_dum_dualsp_setup(void)
{

    mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for Scooby Dum dual SP test ===\n");

    if (fbe_test_util_is_simulation())
    {
        /* Load the config on both SPs. */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        zeroing_physical_config();

        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        zeroing_physical_config();

        //sep_config_load_packages_both_sps(FBE_TRUE,FBE_TRUE,FBE_FALSE);
        sep_config_load_sep_with_time_save(FBE_TRUE);

        //fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        //sep_config_load_sep_setup_package_entries();
        //fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        //sep_config_load_sep_setup_package_entries();

        /* We will work primarily with SPA.  The configuration is automatically
         * instantiated on both SPs.  We no longer create the raid groups during 
         * the setup.
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    }

    return;
}

/***************************************************************
 * end scooby_dum_dualsp_setup()
 ***************************************************************/

/*!****************************************************************************
 *  scooby_dum_test
 ******************************************************************************
 *
 * @brief
 *   This function is used to perform disk zeroing in signle SP configuration.
 *   It initiates disk zeroing and verify that disk zeroing completed 
 *   successfully.
 *
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void scooby_dum_test(void)
{
    fbe_u32_t           port;
    fbe_u32_t           enclosure;
    fbe_u32_t           slot;
    fbe_object_id_t   object_id;
    fbe_status_t       status; 
    fbe_u32_t           timeout_ms = 30000;
    fbe_bool_t          is_enabled = FBE_FALSE;
    
    port = 0;
    enclosure = 0;
    slot = 0;
    /* get the PVD object ID */
    status = fbe_api_provision_drive_get_obj_id_by_location(port, enclosure, slot, &object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    fbe_api_base_config_is_background_operation_enabled(object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING,
                                                        &is_enabled);
    if(is_enabled)
    {
        mut_printf(MUT_LOG_LOW, "=== Test 1  single SP - System disk zeroing test ===\n");
    
        scooby_dum_system_drive_zeroing(object_id); 
    }
    /* 
     *  Test 2: Start disk zeroing and make sure that the disk zeroing completed in
     *          single SP configuration.
     */
    mut_printf(MUT_LOG_LOW, "=== Test 2  single SP - Disk zeroing test ===\n");

    /* Disable dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    /* Lookup the disk location */
    port = scooby_dum_disk_set[0].bus;
    enclosure = scooby_dum_disk_set[0].enclosure;
    slot = scooby_dum_disk_set[0].slot;

    /* Wait for the pvd to come online */
    status = fbe_test_sep_drive_verify_pvd_state(port, enclosure, slot,
                                                 FBE_LIFECYCLE_STATE_READY,
                                                 timeout_ms);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* get the PVD object ID */
    status = fbe_api_provision_drive_get_obj_id_by_location(port, enclosure, slot, &object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);   

    mut_printf(MUT_LOG_LOW, "=== starting disk zeroing for PVD obj id 0x%x(be patient, it will take some time)\n", object_id);	

    /* start disk zeroing */
    status = fbe_api_provision_drive_initiate_disk_zeroing(object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait for disk zeroing to complete */
    status = sep_zeroing_utils_check_disk_zeroing_status(object_id, 200000, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* disk zeroing completed */
    mut_printf(MUT_LOG_LOW, "=== disk zeroing completed for PVD obj id 0x%x\n", object_id);	

    return;
}
/***************************************************************
 * end scooby_dum_test()
 ***************************************************************/


/*!****************************************************************************
 *  scooby_dum_dualsp_test
 ******************************************************************************
 *
 * @brief
 *   This function is used to perform disk zeroing in dual SP configuration.
 *   It initiates disk zeroing and verify that disk zeroing completed 
 *   successfully on both the SPs. It also verify that disk zeroing continue on peer
 *   sp if active SP shutdown while disk zeroing running.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void scooby_dum_dualsp_test(void)
{

    fbe_u32_t                   port;
    fbe_u32_t                   enclosure;
    fbe_u32_t                   slot;
    fbe_object_id_t             object_id;
    fbe_object_id_t             peer_object_id;
    fbe_lba_t                   peer_sp_checkpoint;
    fbe_status_t                status = FBE_STATUS_OK; 
    fbe_u32_t           timeout_ms = 30000;
    fbe_sim_transport_connection_target_t   local_sp = FBE_SIM_INVALID_SERVER;
    fbe_sim_transport_connection_target_t   peer_sp  = FBE_SIM_INVALID_SERVER;
	fbe_api_provision_drive_info_t      pvd_info;

	/* 
     *  Test 2: Start disk zeroing and make sure that the disk zeroing completed 
     *          on both the SPs in dual SP configuration.
     */
    mut_printf(MUT_LOG_LOW, "=== Test 2  dual SP - Disk zeroing test ===\n");

    /* Enable dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    /* Get the local SP ID */
    local_sp = fbe_api_sim_transport_get_target_server();
    MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, local_sp);
    mut_printf(MUT_LOG_LOW, "=== ACTIVE(local) side (%s) ===", local_sp == FBE_SIM_SP_A ? "SP A" : "SP B");

    /* set the passive SP */
    peer_sp = (local_sp == FBE_SIM_SP_A ? FBE_SIM_SP_B : FBE_SIM_SP_A);

    /*  Set the target server to the peer SP */
    fbe_api_sim_transport_set_target_server(peer_sp);

    /* Lookup the disk location */
    port = scooby_dum_disk_set[1].bus;
    enclosure = scooby_dum_disk_set[1].enclosure;
    slot = scooby_dum_disk_set[1].slot;

    /*  Set the target server to the local SP */
    fbe_api_sim_transport_set_target_server(local_sp);

    /* Wait for the pvd to come online */
    status = fbe_test_sep_drive_verify_pvd_state(port, enclosure, slot,
                                                 FBE_LIFECYCLE_STATE_READY,
                                                 timeout_ms);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* get the PVD object ID */
    status = fbe_api_provision_drive_get_obj_id_by_location(port, enclosure, slot, &object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "=== starting disk zeroing on PVD obj 0x%x(be patient, it will take some time)===\n", object_id);	

    /* start disk zeroing */
    status = fbe_api_provision_drive_initiate_disk_zeroing(object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait for disk zeroing complete on local SP*/
    status = sep_zeroing_utils_check_disk_zeroing_status(object_id, 200000, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* disk zeroing completed */
    mut_printf(MUT_LOG_LOW, "=== Completed disk zeroing on local SP for PVD obj 0x%x===\n", object_id);	

    /* set the target for peer SP */
    fbe_api_sim_transport_set_target_server(peer_sp);

    /* get the PVD object ID on peer */
    status = fbe_api_provision_drive_get_obj_id_by_location(port, enclosure, slot, &peer_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);       

	/* verify that disk zeroing also completed on peer SP */
    status = fbe_api_provision_drive_get_zero_checkpoint(peer_object_id, &peer_sp_checkpoint);        
    MUT_ASSERT_INTEGER_EQUAL(peer_sp_checkpoint, FBE_LBA_INVALID);

    /* disk zeroing completed */
    mut_printf(MUT_LOG_LOW, "=== Completed disk zeroing on peer SP for PVD obj 0x%x===\n", peer_object_id);	

    /* 
     *  Test 3: Shutdown the active SP when while disk zeroing running.
     *          It is expected that peer SP will become the active sp and disk zeroing
     *          will continue and completed on it.
     */
    mut_printf(MUT_LOG_LOW, "=== Test 3  dusl SP - Disk zeroing - Active SP shutdown test ===\n");     

    /* Lookup the disk location */
    port = scooby_dum_disk_set[2].bus;
    enclosure = scooby_dum_disk_set[2].enclosure;
    slot = scooby_dum_disk_set[2].slot;     

    //  Set the target server to the local SP
    fbe_api_sim_transport_set_target_server(local_sp);

    /* Wait for the pvd to come online */
    status = fbe_test_sep_drive_verify_pvd_state(port, enclosure, slot,
                                                 FBE_LIFECYCLE_STATE_READY,
                                                 timeout_ms);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	/* get the PVD object ID */
    status = fbe_api_provision_drive_get_obj_id_by_location(port, enclosure, slot, &object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	fbe_api_provision_drive_get_info(object_id, &pvd_info);

    mut_printf(MUT_LOG_LOW, "=== starting disk zeroing for PVD obj 0x%x(be patient, it will take some time)===\n", object_id);	

    /* start disk zeroing */
    status = fbe_api_provision_drive_initiate_disk_zeroing(object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait for disk zeroing to be reach at zero checkpoint 0x1000 */
    status = sep_zeroing_utils_check_disk_zeroing_status(object_id, 200000, 0x1000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "=== disk zeroing reach at checkpoint 0x1000 on local SP ===\n");

    /* shutdown the active SP */
    mut_printf(MUT_LOG_LOW, "=== shutdown active(local) SP ===\n");
    fbe_test_sep_util_destroy_sep_physical();

    /* set target to peer SP */
    fbe_api_sim_transport_set_target_server(peer_sp);

    /* get the PVD object ID on peer */
    status = fbe_api_provision_drive_get_obj_id_by_location(port, enclosure, slot, &peer_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);       

	/* get the current disk zeroing checkpoint on passive SP */
    status = fbe_api_provision_drive_get_zero_checkpoint(peer_object_id, &peer_sp_checkpoint);        
    mut_printf(MUT_LOG_LOW, "=== continue disk zero on peer - obj id 0x%x, chkpt on peer 0x%llx\n", peer_object_id, (unsigned long long)peer_sp_checkpoint);

    /* start disk zeroing on peer SP */
    status = fbe_api_provision_drive_initiate_disk_zeroing(peer_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* verify that disk zeroing completed on peer SP */
    status = sep_zeroing_utils_check_disk_zeroing_status(peer_object_id, 200000, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* disk zeroing completed */
    mut_printf(MUT_LOG_LOW, "=== disk zeroing completed on peer SP ===\n");	

    /* Disable dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;
}
/***************************************************************
 * end scooby_dum_dualsp_test()
 ***************************************************************/
 
/*!****************************************************************************
 *  scooby_dum_send_io
 ******************************************************************************
 *
 * @brief
 *   This function is used to perform disk zeroing in signle SP configuration.
 *   It initiates disk zeroing and verify that disk zeroing completed 
 *   successfully.
 *
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
static void scooby_dum_send_io(fbe_object_id_t   pdo_object_id,
                               fbe_api_rdgen_context_t* context_p,
                               fbe_rdgen_operation_t rdgen_operation)
{
    fbe_status_t      status;
    fbe_lba_t        lba;

    for(lba=0; lba < (SYSTEM_DRIVE_UNCONSUMED_END - CHUNK_SIZE); lba+=0x800)
    {
        status = fbe_api_rdgen_send_one_io(context_p,
                                       pdo_object_id,
                                       FBE_CLASS_ID_INVALID,
                                       FBE_PACKAGE_ID_PHYSICAL,
                                       rdgen_operation,
                                       FBE_RDGEN_PATTERN_LBA_PASS,
                                       lba,
                                       0x800,
                                       FBE_RDGEN_OPTIONS_INVALID,
                                       0, 0,  /*no expiration or abort time */
                                       FBE_API_RDGEN_PEER_OPTIONS_INVALID);

        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
        // Make sure that there were no errors
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate no errors ==\n", __FUNCTION__);
        MUT_ASSERT_INT_EQUAL(context_p->status, FBE_STATUS_OK);
    
        // Check the packet status, block status and block qualifier
        MUT_ASSERT_INT_EQUAL(context_p->start_io.status.status, FBE_STATUS_OK);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    }
    

    return;


}/******************************************
 * end scooby_dum_send_io()
 ******************************************/

/*!****************************************************************************
 *  scooby_dum_system_drive_zeroing
 ******************************************************************************
 *
 * @brief
 *   This function is used to perform disk zeroing in signle SP configuration.
 *   It initiates disk zeroing and verify that disk zeroing completed 
 *   successfully.
 *
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void scooby_dum_system_drive_zeroing(fbe_object_id_t  pvd_object_id)
{

    fbe_api_rdgen_context_t *                         context_p = &scoobydum_context[0];
    fbe_u32_t                                         chunk_count;
    fbe_lba_t                                         zero_checkpoint;
    fbe_object_id_t                                   pdo_object_id;
    fbe_status_t                                      status;
    fbe_u32_t                                         port;
    fbe_u32_t                                         enclosure;
    fbe_u32_t                                         slot;
    fbe_api_provisional_drive_paged_bits_t            *pvd_metadata;
    fbe_api_base_config_metadata_paged_get_bits_t paged_get_bits;

    /* Wait for the debug hook to hit which was set during SP reboot to confirm that NR get marked */
    status = scooby_dum_wait_for_target_SP_hook(pvd_object_id,
                                       SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
                                       FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_provision_drive_get_zero_checkpoint(pvd_object_id, &zero_checkpoint);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_UINT64_EQUAL(zero_checkpoint,0);

    port = 0;
    enclosure = 0;
    slot = 0;

    fbe_api_get_physical_drive_object_id_by_location(port, enclosure, slot, &pdo_object_id);
    scooby_dum_send_io(pdo_object_id, context_p, FBE_RDGEN_OPERATION_WRITE_ONLY); 

    /* delete the debug hook */
    scooby_dum_delete_debug_hook(pvd_object_id, 
                                 SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
                                 FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY );

    /* wait for disk zeroing to complete */
    status = sep_zeroing_utils_check_disk_zeroing_status(pvd_object_id, 200000, SYSTEM_DRIVE_UNCONSUMED_END+0x1000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    scooby_dum_send_io(pdo_object_id, context_p, FBE_RDGEN_OPERATION_READ_CHECK);

    chunk_count = (SYSTEM_DRIVE_UNCONSUMED_END - CHUNK_SIZE)/CHUNK_SIZE;

    /* Read metadata */        
    paged_get_bits.metadata_offset = 0;
    paged_get_bits.metadata_record_data_size = sizeof(fbe_api_provisional_drive_paged_bits_t);
    * (fbe_u32_t *)paged_get_bits.metadata_record_data = 0;
    paged_get_bits.get_bits_flags = 0;
    status = fbe_api_base_config_metadata_paged_get_bits(pvd_object_id, &paged_get_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    pvd_metadata = (fbe_api_provisional_drive_paged_bits_t *)&paged_get_bits.metadata_record_data[0];
    MUT_ASSERT_INT_EQUAL(pvd_metadata->need_zero_bit, 1);

    return;


}/******************************************
 * end scooby_dum_system_drive_zeroing()
 ******************************************/

/*!****************************************************************************
 *  scooby_dum_setup_zeroing_hook
 ******************************************************************************
 *
 * @brief
 *  This is the reboot SP function for the doodle test. This function preset the
 *  debug hook before load sep and neit package so that SP boot up with debug hook
 *  set.
 *
 * @param in_current_rg_config_p - Pointer to the raid group config info
 *        state - the state in the monitor 
 *        substate - the substate in the monitor 
 *
 * @return  None 
 *
 * @author
 *   01/19/2012 - Created. Ashwin Tamilarasan
 *****************************************************************************/
static void scooby_dum_setup_zeroing_hook(void)
{
    fbe_sep_package_load_params_t    sep_params_p;
    

    mut_printf(MUT_LOG_LOW, "===  Setting up Zeroing hook before we load SEP ===\n");

    fbe_zero_memory(&sep_params_p, sizeof(fbe_sep_package_load_params_t));

    /* Initialize the scheduler hook */
    sep_params_p.scheduler_hooks[0].object_id = PVD_OBJ_ID_FOR_DRIVE_0;
    sep_params_p.scheduler_hooks[0].monitor_state = SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO;
    sep_params_p.scheduler_hooks[0].monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY;
    sep_params_p.scheduler_hooks[0].check_type = SCHEDULER_CHECK_STATES;
    sep_params_p.scheduler_hooks[0].action = SCHEDULER_DEBUG_ACTION_PAUSE;
    sep_params_p.scheduler_hooks[0].val1 = NULL;
    sep_params_p.scheduler_hooks[0].val2 = NULL;

    /* and this will be our signal to stop */
    sep_params_p.scheduler_hooks[1].object_id = FBE_OBJECT_ID_INVALID;

    /* load sep and neit package with preset the debug hook */
    sep_config_load_sep_and_neit_params(&sep_params_p, NULL);

    //status = fbe_test_sep_util_wait_for_database_service(DOODLE_WAIT_MSEC);
    //MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;
}
/******************************************
 * end scooby_dum_setup_zeroing_hook()
 ******************************************/

/*!**************************************************************
 * scooby_dum_wait_for_target_SP_hook()
 ****************************************************************
 * @brief
 *  Wait for the hook to be hit
 *
 * @param rg_object_id - the rg object id to wait for
 *        state - the state in the monitor 
 *        substate - the substate in the monitor           
 *
 * @return fbe_status_t - status 
 *
 * @author
 *  8/8/2011 - Created. Amit Dhaduk
 *
 ****************************************************************/
fbe_status_t scooby_dum_wait_for_target_SP_hook(fbe_object_id_t pvd_object_id, 
                                                fbe_u32_t state, 
                                                fbe_u32_t substate) 
{
    fbe_scheduler_debug_hook_t      hook;
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       current_time = 0;
    fbe_u32_t                       timeout_ms = SCOOBY_DUM_WAIT_MSEC;
    
    hook.monitor_state = state;
    hook.monitor_substate = substate;
    hook.object_id = pvd_object_id;
    hook.check_type = SCHEDULER_CHECK_STATES;
    hook.action = SCHEDULER_DEBUG_ACTION_PAUSE;
    hook.val1 = NULL;
    hook.val2 = NULL;
    hook.counter = 0;

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for the hook. rg obj id: 0x%X state: %d substate: %d.", 
               pvd_object_id, state, substate);

    while (current_time < timeout_ms){

        status = fbe_api_scheduler_get_debug_hook(&hook);

        if (hook.counter != 0) {
            mut_printf(MUT_LOG_TEST_STATUS,
		       "We have hit the debug hook %llu times",
		       (unsigned long long)hook.counter);
            return status;
        }
        current_time = current_time + 200;
        fbe_api_sleep (200);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                  "%s: object %d did not hit state %d substate %d in %d ms!\n", 
                  __FUNCTION__, pvd_object_id, state, substate, timeout_ms);

    return FBE_STATUS_GENERIC_FAILURE;
}
/******************************************
 * end scooby_dum_wait_for_target_SP_hook()
 ******************************************/

/*!**************************************************************
 * scooby_dum_delete_debug_hook()
 ****************************************************************
 * @brief
 *  Delete the debug hook for the given state and substate.
 *
 * @param rg_object_id - the rg object id to wait for
 *        state - the state in the monitor 
 *        substate - the substate in the monitor           
 *
 * @return fbe_status_t - status 
 *
 * @author
 *  8/8/2011 - Created. Amit Dhaduk
 *
 ****************************************************************/
fbe_status_t scooby_dum_delete_debug_hook(fbe_object_id_t pvd_object_id, 
                                          fbe_u32_t state, 
                                          fbe_u32_t substate) 
{
    fbe_scheduler_debug_hook_t      hook;
    fbe_status_t                    status = FBE_STATUS_OK;

    hook.object_id = pvd_object_id;
    hook.action = SCHEDULER_DEBUG_ACTION_PAUSE;
    hook.check_type = SCHEDULER_CHECK_STATES;
    hook.monitor_state = state;
    hook.monitor_substate = substate;
    hook.val1 = NULL;
    hook.val2 = NULL;
    hook.counter = 0;

    status = fbe_api_scheduler_get_debug_hook(&hook);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "The hook was hit %llu times",
	       (unsigned long long)hook.counter);

    mut_printf(MUT_LOG_TEST_STATUS, "Deleting Debug Hook: pvd obj id 0x%X state %d, substate %d", pvd_object_id, state, substate);

    status = fbe_api_scheduler_del_debug_hook(pvd_object_id,
                            state,
                            substate,
                            NULL,
                            NULL,
                            SCHEDULER_CHECK_STATES,
                            SCHEDULER_DEBUG_ACTION_PAUSE);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    return FBE_STATUS_OK;
}
/******************************************
 * end scooby_dum_delete_debug_hook()
 ******************************************/

/*!**************************************************************
 * scooby_dum_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the scooby_dum test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  7/15/2011 - Created. Arun S
 *
 ****************************************************************/

void scooby_dum_cleanup(void)
{
    mut_printf(MUT_LOG_LOW, "=== %s starts ===\n", __FUNCTION__);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_sep_physical();
    }

    mut_printf(MUT_LOG_LOW, "=== %s completed ===\n", __FUNCTION__);
    return;
}
/******************************************
 * end scooby_dum_cleanup()
 ******************************************/

/*!**************************************************************
 * scooby_dum_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the scooby_dum dual sp test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  7/15/2011 - Created. Arun S
 *
 ****************************************************************/

void scooby_dum_dualsp_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Always clear dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {
        /* We already have cleanedup SPA. It's time to cleanup SPB. */
        fbe_test_sep_util_destroy_sep_physical();
    }

    return;
}
/******************************************
 * end scooby_dum_dualsp_cleanup()
 ******************************************/



/*************************
 * end file scooby_dum_test.c
 *************************/
