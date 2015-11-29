/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file yabbadoo_test.c
 ***************************************************************************
 *
 * @brief   This scenario verify that when LUN object issues a Zero request
 *          for the entire extent of a LUN when it is bound, Background zeroing
 *          starts and completes.
 *
 * @version
 *  4/22/2010  Amit Dhaduk  - Created.
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "sep_tests.h"
#include "sep_utils.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "pp_utils.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_random.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * yabbadoo_short_desc = "LUN Bind Initiates Background Zeroing of LUN extent";
char * yabbadoo_long_desc ="\
The YabbaDoo validates background zeroing is initiated when a normal bind is\n\
performed.\n\
The expected sequence is:\n\
    o LUN object sends a Zero request via the I/O path for the entire extent of\n\
      the LUN to the downstream raid group\n\
    o Raid group object splits zero request as neccessary and sends each sub-request\n\
      to the assocated VD which is then forwarded to the assocaited PVD.\n\
    o PVD object sets the `Consumed' bits to (1) for the associated chunks of the\n\
      LUN. (Its assumed that all Consumed bit are 0 when the bind occurs)\n\
\n\
Dependencies:\n\
    + The LUN object monitor has the ability to send zero request.\n\
    + The associated raid group object can process the zero request.\n\
    + The provision drive can retrieve and set bitmaps.\n\
    + The provision drive automatically initiates background zeroing.\n\
    + The provision drive supports a `Are all `Consumed' bits set for extent?' control code.\n\
\n"\
"Starting Config:\n\
        [PP] armada board\n\
        [PP] SAS PMC port\n\
        [PP] viper enclosure\n\
        [PP] 3 SAS drives\n\
        [PP] 3 logical drive\n\
        [SEP] 3 provision drive\n\
        [SEP] 3 virtual drive\n\
        [SEP] parity raid group (raid 5 - 520)\n\
\n\
\n"
"STEP 1: Create raid group configuration\n"
"STEP 2: Create a LUN(0)\n"
"STEP 3: Initiate disk zeroing on all 3 disks part of Raid Group\n"
"        - Verify that disk zeroing started event log message has been logged\n"
"        - Wait for disk zeroing completed on all 3 drives.\n"
"        - Verify that disk zeroing completed event log message has been logged\n"
"STEP 4 Bind a one more LUN(1)lun in same Raid Group\n"
"        - Verify that lun zeroing started event log message has been logged\n"
"STEP 5: Initiate disk zeroing on all 3 disks part of Raid Group\n"
"STEP 6: Verify that LUN zeroing completed with status 100%\n"
"STEP 7: Unbind LUN 0 and 1 \n"
"STEP 8  Bind a LUN(123)in same Raid Group\n"
"        - Verify that lun zeroing started event log message has been logged\n"
"STEP 9: Initiate disk zeroing on all 3 disks part of Raid Group\n"
"STEP 10: Verify that LUN zeroing completed with status 100%\n"
"\n"
"\n";





/*!*******************************************************************
 * @def YABBADOO_TEST_NS_TIMEOUT
 *********************************************************************
 * @brief  Notification to timeout value in milliseconds 
 *
 *********************************************************************/
#define YABBADOO_TEST_NS_TIMEOUT        25000 /*wait maximum of 25 seconds*/


/*!*******************************************************************
 * @def YABBADOO_POLLING_INTERVAL
 *********************************************************************
 * @brief polling interval time to check for disk zeroing complete
 *
 *********************************************************************/
#define YABBADOO_POLLING_INTERVAL 100 /*ms*/

/*!*******************************************************************
 * @def YABBADOO_TEST_RAID_GROUP_ID
 *********************************************************************
 * @brief RAID Group id used by this test
 *
 *********************************************************************/
#define YABBADOO_TEST_RAID_GROUP_ID        0

/*!*******************************************************************
 * @def YABBADOO_TEST_RAID_GROUP_COUNT
 *********************************************************************
 * @brief Max number of raid groups we will test with.
 *
 *********************************************************************/
#define YABBADOO_TEST_RAID_GROUP_COUNT 1

#define YABBADOO_TEST_LUNS_PER_RAID_GROUP  2

#define YABBADOO_TEST_CHUNKS_PER_LUN  3



/*!*******************************************************************
 * @var yabbadoo_rg_configuration
 *********************************************************************
 * @brief This is rg_configuration
 *
 *********************************************************************/
static fbe_test_rg_configuration_t yabbadoo_rg_configuration[] = 
{
     /* width,  capacity          raid type,                  class,        block size  RAID-id.  bandwidth.*/
    {2,         0xE000,  FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER,  520,            0,        0},
    {4,         0xE000,  FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER,  520,            1,        0},
    {6,         0xE000,  FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER,  520,            2,        0},
    {3,         0xE000,  FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            3,        0},
    {2,         0xE000,  FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            4,        0},
    {3,         0xE000,  FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,  520,            5,        0},
    {4,         0xE000,  FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            6,        0},
    {5,         0xE000,  FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,   520,            7,        0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};


static void yabbadoo_test_create_lun(fbe_lun_number_t   lun_number,fbe_test_rg_configuration_t *rg_config_ptr);
static void yabbadoo_test_destroy_lun(fbe_lun_number_t   lun_number);
static void yabbadoo_test_create_rg(void);
static void yabbadoo_test_start_disk_zeroing(fbe_test_rg_configuration_t *rg_config_p);
static void yabbadoo_test_check_disk_zeroing_status(fbe_test_rg_configuration_t *rg_config_p);
static fbe_bool_t yabbadoo_lun_zeroing_check_event_log_msg(fbe_object_id_t pvd_object_id,
                                                fbe_lun_number_t lun_number, fbe_bool_t  is_lun_zero_start );
static fbe_status_t yabbadoo_wait_for_lun_zeroing_complete(fbe_u32_t lun_number, 
                                                fbe_u32_t timeout_ms, fbe_package_id_t package_id);
static fbe_status_t yabbadoo_wait_for_disk_zeroing_complete(fbe_object_id_t object_id, 
                                                fbe_u32_t timeout_ms, fbe_package_id_t package_id);
static void yabbadoo_run_tests(fbe_test_rg_configuration_t *rg_config_ptr ,void * context_p);
static void yabbadoo_test_check_lun_zeroing_started(fbe_lun_number_t   lun_number);



/*!****************************************************************************
 *  yabbadoo_test
 ******************************************************************************
 *
 * @brief
 *   This is the main entry point for the yabbadoo test.  
 *
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 *  4/22/2010 - Created. Amit Dhaduk
 *  5/17/2011 - Modified. Vishnu Sharma
 *
 *****************************************************************************/
static void yabbadoo_run_tests(fbe_test_rg_configuration_t *rg_config_ptr ,void * context_p)
{
    fbe_u32_t                   lun_number = 0;
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u32_t                   rg_index = 0;
    fbe_u32_t                   num_raid_groups = 0;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_s32_t                   lun_index = 0;
    

    num_raid_groups = fbe_test_get_rg_array_length(rg_config_ptr);


    mut_printf(MUT_LOG_LOW, "=== Yabbadoo test start ===\n");	

    for(rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {

        rg_config_p = rg_config_ptr + rg_index;

        if(fbe_test_rg_config_is_enabled(rg_config_p))
        {
    
            mut_printf(MUT_LOG_LOW, "=== Test 1   LUN Bind - Disk zeroing test ===\n");        

            for(lun_index = 0; lun_index <YABBADOO_TEST_LUNS_PER_RAID_GROUP; lun_index++)
            {
                yabbadoo_test_destroy_lun(lun_index + lun_number);
            }

            /* clear the old statastics */
            status = fbe_api_clear_event_log(FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "== %s Failed to clear SEP event log! == ");
                        
            yabbadoo_test_create_lun(lun_number,rg_config_p);

            //yabbadoo_test_start_disk_zeroing(rg_config_p);
            yabbadoo_test_check_disk_zeroing_status(rg_config_p);
            
            mut_printf(MUT_LOG_LOW, "=== Test 2   LUN Bind - LUN zeroing test ===\n");        

            lun_number++;
            yabbadoo_test_create_lun(lun_number,rg_config_p);

            /*Make sure that lun is binded but zeroing is not started yet */
            //yabbadoo_test_check_lun_zeroing_started(lun_number);
            

            /* wait for lun zeroing complete */
            status = yabbadoo_wait_for_lun_zeroing_complete(lun_number, 200000, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            yabbadoo_test_destroy_lun(lun_number);
            
            mut_printf(MUT_LOG_LOW, "=== Test 3   LUN Unbind/Bind - LUN zeroing test ===\n");        

            /* create a new lun */
            yabbadoo_test_create_lun(lun_number,rg_config_p);

            /*Make sure that lun is binded but zeroing is not started yet */
           // yabbadoo_test_check_lun_zeroing_started(lun_number);

            /* verify that LUN zeroing completed and its status  should be 100% */
            status = yabbadoo_wait_for_lun_zeroing_complete(lun_number, 200000, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            lun_number++;
        }
    }

    mut_printf(MUT_LOG_LOW, "=== Yabbadoo test complete ===\n");	
    return;
}
/******************************************
 * end yabbadoo_test()
 ******************************************/

/*!****************************************************************************
 * yabbadoo_cleanup()
 ******************************************************************************
 * @brief
 *  Cleanup function.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  5/06/2011 - Created. Vishnu Sharma
 ******************************************************************************/

void yabbadoo_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}

/*!****************************************************************************
 * yabbadoo_test()
 ******************************************************************************
 * @brief
 *  Run lun zeroing test across different RAID groups.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  5/06/2011 - Created. Vishnu Sharma
 ******************************************************************************/
void yabbadoo_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    
    rg_config_p = &yabbadoo_rg_configuration[0];

    fbe_test_run_test_on_rg_config(rg_config_p,NULL,yabbadoo_run_tests,
                                   YABBADOO_TEST_LUNS_PER_RAID_GROUP,
                                   YABBADOO_TEST_CHUNKS_PER_LUN);
     return;
}

/*!****************************************************************************
 *  yabbadoo_test_start_disk_zeroing
 ******************************************************************************
 *
 * @brief
 *    This function starts a disk zeroing operation on selected provision drives
 *    and wait for its completion. It also checks that disk zero start and 
 *    complete event message logs correctly.
 *
 * @return  void
 *
 * @author
 *    11/18/2010 - Created. Amit Dhaduk
 *    5/17/2011 - Modified. Vishnu Sharma  
 *****************************************************************************/
static void yabbadoo_test_start_disk_zeroing(fbe_test_rg_configuration_t *rg_config_p)
{

    fbe_u32_t position;
    fbe_object_id_t pvd_object_id[3];
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE; 

    /* initiate disk zeroing on all rg disk set */
    for(position = 0; position < rg_config_p->width; position++)
    {
        /* get the pvd object-id for this position. */
        status = fbe_api_provision_drive_get_obj_id_by_location(
                    rg_config_p->rg_disk_set[position].bus,
                    rg_config_p->rg_disk_set[position].enclosure,
                    rg_config_p->rg_disk_set[position].slot,
                    &pvd_object_id[position]);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, pvd_object_id[position]);

        /* start disk zeroing */
        status = fbe_api_provision_drive_initiate_disk_zeroing(pvd_object_id[position]);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "Disk Zeroing started for PVD object 0x%x, Bus.Encl.slot %d.%d.%d\n", pvd_object_id[position],
                                                            rg_config_p->rg_disk_set[position].bus,
                                                            rg_config_p->rg_disk_set[position].enclosure,
                                                            rg_config_p->rg_disk_set[position].slot);
        
    }

    return;
}
/******************************************
 * end yabbadoo_test_start_disk_zeroing()
 ******************************************/

/*!****************************************************************************
 *  yabbadoo_test_check_disk_zeroing_status
 ******************************************************************************
 *
 * @brief
 *    This function verify that disk zeroing gets complete for all the associated 
 *    provision drives and make sure that "Disk zeroing complete" event message
 *    has been logged.
 *
 * @return  void
 *
 * @author
 *    11/18/2010 - Created. Amit Dhaduk
 *    5/17/2011 - Modified. Vishnu Sharma
 *****************************************************************************/
static void yabbadoo_test_check_disk_zeroing_status(fbe_test_rg_configuration_t *rg_config_p)
{

    fbe_u32_t position;
    fbe_object_id_t pvd_object_id[3];
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE; 

    /* wait for disk zeroing complete on all rg disk set */
    for(position = 0; position < rg_config_p->width; position++)
    {

        /* get the pvd object-id for this position. */
        status = fbe_api_provision_drive_get_obj_id_by_location(
                    rg_config_p->rg_disk_set[position].bus,
                    rg_config_p->rg_disk_set[position].enclosure,
                    rg_config_p->rg_disk_set[position].slot,
                    &pvd_object_id[position]);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, pvd_object_id[position]);


        /* wait for disk zeroing complete */
        status = yabbadoo_wait_for_disk_zeroing_complete(pvd_object_id[position], 200000, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


        /* check that event is logged for disk zeroing completed */
        status = fbe_test_sep_drive_check_for_disk_zeroing_event(pvd_object_id[position], 
                            SEP_INFO_PROVISION_DRIVE_OBJECT_DISK_ZEROING_COMPLETED,  
                            rg_config_p->rg_disk_set[position].bus,
                            rg_config_p->rg_disk_set[position].enclosure,
                            rg_config_p->rg_disk_set[position].slot); 
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK); 

        mut_printf(MUT_LOG_TEST_STATUS, "Disk Zeroing Completed for PVD object 0x%x, Bus.Encl.slot %d.%d.%d\n", pvd_object_id[position],
                                                            rg_config_p->rg_disk_set[position].bus,
                                                            rg_config_p->rg_disk_set[position].enclosure,
                                                            rg_config_p->rg_disk_set[position].slot);

    }

    return;
}
/******************************************
 * end yabbadoo_test_check_disk_zeroing_status()
 ******************************************/

/*!****************************************************************************
 *  yabbadoo_test_check_lun_zeroing_started
 ******************************************************************************
 *
 * @brief
 *    This function verify that lun zeroing is not started  yet. 
 *    
 *
 * @return  void
 *
 * @author
 *        5/17/2011 - Created. Vishnu Sharma
 *****************************************************************************/
static void yabbadoo_test_check_lun_zeroing_started(fbe_lun_number_t   lun_number)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_api_lun_get_zero_status_t       lu_zero_status;
    fbe_object_id_t                     lu_object_id;
    
    /* get the lun zero percentange after we start disk zeroing. */
    status = fbe_api_database_lookup_lun_by_number(lun_number, &lu_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

   /*get lun zeroing status*/
    status = fbe_api_lun_get_zero_status(lu_object_id, &lu_zero_status);

    if (status == FBE_STATUS_GENERIC_FAILURE){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: Get Lifecycle state failed with status %d\n", __FUNCTION__, status);          
        return ;
    }

    fbe_api_trace(FBE_TRACE_LEVEL_INFO,
                  "%s: lu_object_id 0x%x has not yet started zeroing %d zeroing\n", 
                  __FUNCTION__, lu_object_id, lu_zero_status.zero_percentage);

    /*Make sure that disk zeroing is not started yet  */
    MUT_ASSERT_TRUE(lu_zero_status.zero_percentage == 0);
        
    return;
}

/******************************************
 * end yabbadoo_test_check_lun_zeroing_started()
 ******************************************/


/*!****************************************************************************
 *  yabbadoo_setup
 ******************************************************************************
 *
 * @brief
 *   This is the setup function for the yabbadoo test.  
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 *  4/22/2010 - Created. Amit Dhaduk 
 *  5/17/2011 - Modified. Vishnu Sharma
 *****************************************************************************/

void yabbadoo_setup(void)
{

    mut_printf(MUT_LOG_LOW, "=== yabbadoo setup ===\n");	

    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;

        rg_config_p = &yabbadoo_rg_configuration[0];
        
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);

        elmo_load_config(rg_config_p, YABBADOO_TEST_LUNS_PER_RAID_GROUP, YABBADOO_TEST_CHUNKS_PER_LUN);

        /* set the trace level */
        elmo_set_trace_level(FBE_TRACE_LEVEL_INFO);    
    }
    
    return; 
}
/******************************************
 * end yabbadoo_setup()
 ******************************************/


/*!****************************************************************************
 *  yabbadoo_wait_for_disk_zeroing_complete
 ******************************************************************************
 *
 * @brief
 *   This function checks disk zeroing complete status with reading checkpoint 
 *    in loop. It exits only when disk zeroing completed or time our occurred.
 *
 * @param object_id    - provision drive object id
 * @param timeout_ms   - max timeout value to check for disk zeroing  complete
 * 
 * @return fbe_status_t              fbe status 
 *
 * @author
 *  4/22/2010 - Created. Amit Dhaduk
 *
 *****************************************************************************/

static fbe_status_t yabbadoo_wait_for_disk_zeroing_complete(fbe_object_id_t object_id, fbe_u32_t timeout_ms, fbe_package_id_t package_id)
{
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u32_t               current_time = 0;
    fbe_lba_t				zero_checkpoint;
    fbe_u16_t				zero_percentage;
    
    while (current_time < timeout_ms){

		/* get the current disk zeroing checkpoint */
        status = fbe_api_provision_drive_get_zero_checkpoint(object_id, &zero_checkpoint); 
        
        if (status == FBE_STATUS_GENERIC_FAILURE){
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: Get Lifecycle state failed with status %d\n", __FUNCTION__, status);          
            return status;
        }
        /* test the API for disk zeroing percentage */
        status = fbe_api_provision_drive_get_zero_percentage(object_id, &zero_percentage);       

        if (status != FBE_STATUS_OK){                    
            return status;
        }
        /* check if disk zeroing completed */
        if (zero_checkpoint == FBE_LBA_INVALID){
            
            /* test the API for disk zeroing percentage completed */
            MUT_ASSERT_INT_EQUAL(zero_percentage, 100); 

            return status;
        }
        current_time = current_time + YABBADOO_POLLING_INTERVAL;
        fbe_api_sleep (YABBADOO_POLLING_INTERVAL);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                  "%s: object_id %d checkpoint-0x%llx, %d ms!\n", 
                  __FUNCTION__, object_id,
		  (unsigned long long)zero_checkpoint, timeout_ms);

    return FBE_STATUS_GENERIC_FAILURE;
}

/******************************************
 * end yabbadoo_wait_for_disk_zeroing_complete()
 ******************************************/



/*!****************************************************************************
 *  yabbadoo_wait_for_lun_zeroing_complete
 ******************************************************************************
 *
 * @brief
 *   This function checks whether lun zeroing gets completed or not.
 *   and make sure that "LUN zeroing complete" event message has been logged.
 *
 * @return  void
 *
 * @author
 *    11/18/2010 - Created. Amit Dhaduk
 *
 *
 *****************************************************************************/

static fbe_status_t yabbadoo_wait_for_lun_zeroing_complete(fbe_u32_t lun_number, fbe_u32_t timeout_ms, fbe_package_id_t package_id)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u32_t                           current_time = 0;
    fbe_api_lun_get_zero_status_t       lu_zero_status;
    fbe_object_id_t                     lu_object_id;
    fbe_bool_t                          is_msg_present = FBE_FALSE;

    /* get the lun zero percentange after we start disk zeroing. */
    status = fbe_api_database_lookup_lun_by_number(lun_number, &lu_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    
    while (current_time < timeout_ms){

        status = fbe_api_lun_get_zero_status(lu_object_id, &lu_zero_status);

        if (status == FBE_STATUS_GENERIC_FAILURE){
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: Get Lifecycle state failed with status %d\n", __FUNCTION__, status);          
            return status;
        }

        fbe_api_trace(FBE_TRACE_LEVEL_INFO,
                      "%s: lu_object_id 0x%x has completed %d zeroing\n", 
                      __FUNCTION__, lu_object_id, lu_zero_status.zero_percentage);

        /* check if disk zeroing completed */
        if (lu_zero_status.zero_percentage == 100)
        {
            /* check an event message for lun zeroing completed
             * Event log message may take little bit more time to log in
             */
            is_msg_present = yabbadoo_lun_zeroing_check_event_log_msg(lu_object_id, lun_number, FBE_FALSE);
            if(is_msg_present)
            {
                mut_printf(MUT_LOG_TEST_STATUS, "LUN Zeroing completed for object 0x%x\n", lu_object_id);
                return status;
            }
        }

        current_time = current_time + YABBADOO_POLLING_INTERVAL;
        fbe_api_sleep (YABBADOO_POLLING_INTERVAL);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                  "%s: lu_object_id %d percentage-0x%x, event msg present %d, %d ms!, \n", 
                  __FUNCTION__, lu_object_id, lu_zero_status.zero_percentage, is_msg_present, timeout_ms);

    return FBE_STATUS_GENERIC_FAILURE;
}

/******************************************
 * end yabbadoo_wait_for_lun_zeroing_complete()
 ******************************************/


/*!****************************************************************************
 *  yabbadoo_test_create_lun
 ******************************************************************************
 *
 * @brief
 *   This is the test function for creating a LUN.
 *
 * @param   lun_number - name of LUN to create
 *
 * @return  None 
 * 5/17/2011 - Modified. Vishnu Sharma
 *
 *****************************************************************************/
static void yabbadoo_test_create_lun(fbe_lun_number_t   lun_number,
                                     fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                       status;
    fbe_api_lun_create_t               fbe_lun_create_req;
    fbe_object_id_t                    object_id;
	fbe_bool_t                         is_msg_present = FBE_FALSE;  
    fbe_job_service_error_type_t       job_error_type;      
   
    /* Create a LUN */
    fbe_zero_memory(&fbe_lun_create_req, sizeof(fbe_api_lun_create_t));
    fbe_lun_create_req.raid_type        = rg_config_p->raid_type;
    fbe_lun_create_req.raid_group_id    = rg_config_p->raid_group_id;
    fbe_lun_create_req.lun_number       = lun_number;
    fbe_lun_create_req.capacity         = 0x2000;
    fbe_lun_create_req.placement        = FBE_BLOCK_TRANSPORT_BEST_FIT;
    fbe_lun_create_req.ndb_b            = FBE_FALSE;
    fbe_lun_create_req.noinitialverify_b = FBE_FALSE;
    fbe_lun_create_req.addroffset       = FBE_LBA_INVALID;
    fbe_lun_create_req.world_wide_name.bytes[0] = (fbe_u8_t)fbe_random() & 0xf;
    fbe_lun_create_req.world_wide_name.bytes[1] = (fbe_u8_t)fbe_random() & 0xf;

	status = fbe_api_create_lun(&fbe_lun_create_req, FBE_TRUE, YABBADOO_TEST_NS_TIMEOUT, &object_id, &job_error_type);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);   
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, object_id);

    /* check an event message for lun zeroing started */
    is_msg_present = yabbadoo_lun_zeroing_check_event_log_msg(object_id, lun_number, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, is_msg_present, "LUN Zeroing start - Event log msg is not present!");

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: LUN 0x%X created successfully! ===\n", __FUNCTION__, object_id);

    mut_printf(MUT_LOG_TEST_STATUS, "LUN Zeroing started for LU object 0x%x\n", object_id);

    return;

}  
/**************************************************
 * end yabbadoo_test_create_lun()
 **************************************************/

/*!****************************************************************************
 *  yabbadoo_test_destroy_lun
 ******************************************************************************
 *
 * @brief
 *   This is the test function for destroying a LUN.  
 *
 * @param   lun_number - name of LUN to destroy
 *
 * @return  None 
 *
 *****************************************************************************/
static void yabbadoo_test_destroy_lun(fbe_lun_number_t   lun_number)
{
    fbe_status_t                            status;
    fbe_api_lun_destroy_t       fbe_lun_destroy_req;
    fbe_job_service_error_type_t job_error_type;
    
    fbe_lun_destroy_req.lun_number = lun_number;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Issue LUN destroy! ===\n");

    /* Destroy a LUN */
    status = fbe_api_destroy_lun(&fbe_lun_destroy_req, FBE_TRUE,  YABBADOO_TEST_NS_TIMEOUT, &job_error_type);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    mut_printf(MUT_LOG_TEST_STATUS, "=== LUN destroyed successfully! ===\n");

    return;

}  
/**************************************************
 * end yabbadoo_test_destroy_lun()
 **************************************************/


/*!**************************************************************
 * yabbadoo_test_create_rg()
 ****************************************************************
 * @brief
 *  This is the test function for creating a Raid Group.
 *
 * @param None.              
 *
 * @return None.
 *
 ****************************************************************/

static void yabbadoo_test_create_rg(void)
{
    fbe_status_t                                status;
    fbe_object_id_t                             rg_object_id;
    fbe_job_service_error_type_t                job_error_code;
    fbe_status_t                                job_status;

    /* Initialize local variables */
    rg_object_id        = FBE_OBJECT_ID_INVALID;

    /* Create the SEP objects for the configuration */

    /* Create a RG */
    mut_printf(MUT_LOG_TEST_STATUS, "=== create a RAID Group ===\n");
    status = fbe_test_sep_util_create_raid_group(yabbadoo_rg_configuration);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "RG creation failed");

    /* wait for notification from job service. */
    status = fbe_api_common_wait_for_job(yabbadoo_rg_configuration[0].job_number,
                                         YABBADOO_TEST_NS_TIMEOUT,
                                         &job_error_code,
                                         &job_status,
                                         NULL);

    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to created RG");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");

    /* Verify the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(
             yabbadoo_rg_configuration[0].raid_group_id, &rg_object_id);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    /* Verify the raid group get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, 
                                                     FBE_LIFECYCLE_STATE_READY, 20000,
                                                     FBE_PACKAGE_ID_SEP_0);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "Created Raid Group object %d\n", rg_object_id);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: END\n", __FUNCTION__);

    return;

} 
/**************************************************
 * end yabbadoo_test_create_rg()
 **************************************************/


/*!****************************************************************************
 *  yabbadoo_lun_zeroing_check_event_log_msg
 ******************************************************************************
 *
 * @brief
 *   This is the test function to check event messages for LUN zeroing operation.
 *
 * @param   pvd_object_id       - provision drive object id
 * @param   lun_number          - LUN number
 * @param   is_lun_zero_start  - 
 *                  FBE_TRUE  - check message for lun zeroing start
 *                  FBE_FALSE - check message for lun zeroing complete
 *
 * @return  is_msg_present 
 *                  FBE_TRUE  - if expected event message found
 *                  FBE_FALSE - if expected event message not found
 *
 * @author
 *  4/22/2010 - Created. Amit Dhaduk 
 *
 *****************************************************************************/
static fbe_bool_t yabbadoo_lun_zeroing_check_event_log_msg(fbe_object_id_t lun_object_id,
                                    fbe_lun_number_t lun_number, fbe_bool_t  is_lun_zero_start )
{

    fbe_status_t                        status;               
	fbe_bool_t                          is_msg_present = FBE_FALSE;    
    fbe_u32_t                           message_code;               


    if(is_lun_zero_start == FBE_TRUE)
    {
        message_code = SEP_INFO_LUN_OBJECT_LUN_ZEROING_STARTED;
    }
    else
    {
        message_code = SEP_INFO_LUN_OBJECT_LUN_ZEROING_COMPLETED;
    }

    /* check that given event message is logged correctly */
    status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_SEP_0, 
                    &is_msg_present, 
                    message_code, lun_number, lun_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return is_msg_present; 
}
/*******************************************************
 * end yabbadoo_lun_zeroing_check_event_log_msg()
 *******************************************************/


/*************************
 * end file yabbadoo_test.c
 *************************/


