#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_sim_server.h"
#include "sep_utils.h"
#include "mut.h"
#include "fbe/fbe_random.h"

#define ZERO_ERROR 0

fbe_bool_t fbe_test_common_util_should_disable_package(fbe_package_id_t package_id)
{
    if (package_id == FBE_PACKAGE_ID_NEIT) {
        return FBE_TRUE;
    }
    return FBE_FALSE;
}
/*!**************************************************************
 * @fn fbe_test_common_util_package_destroy()
 ****************************************************************
 * @brief
 *  save the trace error and destroy the package.
 *
 * @param package_list.
 *
 * @return fbe_status_t - FBE_STATUS_OK/FBE_STATUS_GENERIC_FAILURE.
 *
 * HISTORY:
 *  6/10/2010 - Created. VG
 *
 ****************************************************************/
fbe_status_t fbe_test_common_util_package_destroy(fbe_test_package_list_t *to_be_unloaded)
{
    fbe_status_t status;
    fbe_package_id_t current_package_id;
    fbe_u32_t i;

    /* Disable packages that need it.
     */
    for (i = 0; i < to_be_unloaded->number_of_packages; i ++ ) {
          current_package_id = to_be_unloaded->package_list[i];
          if (fbe_test_common_util_should_disable_package(current_package_id)) {
              mut_printf(MUT_LOG_HIGH, "Disable Package 0x%X.", current_package_id);
              status = fbe_api_trace_get_error_counter(&to_be_unloaded->package_error[current_package_id], current_package_id);
              MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "failed to get trace error counters from Package!");
    
              to_be_unloaded->package_disable_status[current_package_id] = fbe_api_sim_server_disable_package(current_package_id);
          }
    }
    /* save the error trace and unload the packages */
    for (i = 0; i < to_be_unloaded->number_of_packages; i ++ ) {
        current_package_id = to_be_unloaded->package_list[i];

        if (!fbe_test_common_util_should_disable_package(current_package_id)) {
            mut_printf(MUT_LOG_HIGH, "Unload Package 0x%X.",  current_package_id);
            status = fbe_api_trace_get_error_counter(&to_be_unloaded->package_error[current_package_id], current_package_id);
            MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "failed to get trace error counters from Package!");
            to_be_unloaded->package_destroy_status[current_package_id] = fbe_api_sim_server_unload_package(current_package_id);
        }
    }

    /* unload the packages last that we already disabled.
     * We do this now so that we unload these packages once the others are already gone. 
     * This helps us in cases where we have dependencies like sep on neit. 
     * By unloading neit last we ensure that sep is no longer sending requests to it when the unload occurs. 
     */
    for (i = 0; i < to_be_unloaded->number_of_packages; i ++ ) {
        current_package_id = to_be_unloaded->package_list[i];

        if (fbe_test_common_util_should_disable_package(current_package_id)) {
            mut_printf(MUT_LOG_HIGH, "Unload Disabled Package 0x%X.",  current_package_id);
            to_be_unloaded->package_destroy_status[current_package_id] = fbe_api_sim_server_unload_package(current_package_id);
        }
    }
    return status;
}

/*!**************************************************************
 * @fn fbe_test_common_util_verify_package_trace_error()
 ****************************************************************
 * @brief
 *  Verify the package trace.
 *
 * @param package_list.
 *
 * @return fbe_status_t - FBE_STATUS_OK/FBE_STATUS_GENERIC_FAILURE.
 *
 * HISTORY:
 *  6/10/2010 - Created. VG
 *
 ****************************************************************/
fbe_status_t fbe_test_common_util_verify_package_trace_error(fbe_test_package_list_t *to_be_unloaded)
{
    fbe_package_id_t current_package_id;

    fbe_u32_t i;
    
    /* Verify the error trace count*/
    for (i = 0; i < to_be_unloaded->number_of_packages; i ++ ) {
        current_package_id = to_be_unloaded->package_list[i];
        mut_printf(MUT_LOG_HIGH, "%s: Package 0x%X.", __FUNCTION__, current_package_id);
        MUT_ASSERT_INT_EQUAL_MSG(ZERO_ERROR, to_be_unloaded->package_error[current_package_id].trace_critical_error_counter, 
                                 "Package has reported critical trace errors!");
        MUT_ASSERT_INT_EQUAL_MSG(ZERO_ERROR, to_be_unloaded->package_error[current_package_id].trace_error_counter, 
                                 "Package has reported error traces!");
    }
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_test_common_util_verify_package_destroy_status()
 ****************************************************************
 * @brief
 *  Verify the package destroy status.
 *
 * @param package_list.
 *
 * @return fbe_status_t - FBE_STATUS_OK/FBE_STATUS_GENERIC_FAILURE.
 *
 * HISTORY:
 *  6/10/2010 - Created. VG
 *
 ****************************************************************/
fbe_status_t fbe_test_common_util_verify_package_destroy_status(fbe_test_package_list_t *to_be_unloaded)
{
    fbe_package_id_t current_package_id;
    fbe_u32_t i;
    
    /* Verify the error trace count*/
    for (i = 0; i < to_be_unloaded->number_of_packages; i ++ ) {
        current_package_id = to_be_unloaded->package_list[i];
        mut_printf(MUT_LOG_HIGH, "%s: Package 0x%X.", __FUNCTION__, current_package_id);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, to_be_unloaded->package_destroy_status[current_package_id], 
                                 "Package destroy failed!");
    }
    return FBE_STATUS_OK;
}

/*!*******************************************************************
 * @var fbe_test_common_b_is_simulation
 *********************************************************************
 * @brief Global to keep track of if we are testing on hardware
 *        or in simulation.
 *        We arbitrarily set this to sim by default.
 *        Sim tests don't need to set this, but
 *        hardware tests will use fbe_test_util_set_hardware_mode()
 *        to set that we are on hardware.
 * 
 *
 *********************************************************************/
static fbe_bool_t fbe_test_common_b_is_simulation = FBE_TRUE;
/*!**************************************************************
 * fbe_test_util_is_simulation()
 ****************************************************************
 * @brief
 *   Return if we are testing in sim or on hardware.
 *
 * @param None.          
 *
 * @return FBE_TRUE - Testing in simulation
 *        FBE_FALSE - Testing on hardware.
 *
 * @author
 *  2/18/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_bool_t fbe_test_util_is_simulation(void)
{
    return fbe_test_common_b_is_simulation;
}
/******************************************
 * end fbe_test_util_is_simulation()
 ******************************************/

/*!**************************************************************
 * fbe_test_util_set_hardware_mode()
 ****************************************************************
 * @brief
 *   Set that we are testing on the hardware.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  2/18/2011 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_util_set_hardware_mode(void)
{
    fbe_test_common_b_is_simulation = FBE_FALSE;
}
/******************************************
 * end fbe_test_util_set_hardware_mode() */

fbe_bool_t fbe_test_random_seed_initted = FBE_FALSE;
/*!**************************************************************
 * fbe_test_init_random_seed()
 ****************************************************************
 * @brief
 *  Initialize the random seed for fbe_test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  4/16/2012 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_init_random_seed(void)
{
    fbe_u32_t random_seed = (fbe_u32_t)fbe_get_time();

    /* The random seed allows us to make tests more random. 
     * We seed the random number generator with the time. 
     * We also allow the user to provide the random seed so that they can 
     * be able to reproduce failures more easily. 
     */
    fbe_test_sep_util_get_cmd_option_int("-test_random_seed", &random_seed);

    if (!fbe_test_random_seed_initted)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "random seed is: 0x%x", random_seed);
        fbe_random_set_seed(random_seed);
        fbe_test_random_seed_initted = FBE_TRUE;
    }
}
/******************************************
 * end fbe_test_init_random_seed()
 ******************************************/
/*!**************************************************************
 * fbe_test_common_util_test_setup_init()
 ****************************************************************
 * @brief
 *   Any initialization that needs to be done before test starts
 *   is done in this function.
 *
 * @param None.          
 *  
 * @note This function should only be called in simulation after  
 *       all the packages have been loaded.  
 *  
 * @return fbe_status_t - FBE_STATUS_OK/FBE_STATUS_GENERIC_FAILURE.
 *
 * @author
 *  06/10/2011 - Created. Vamsi V
 *
 ****************************************************************/

fbe_status_t fbe_test_common_util_test_setup_init(void)
{
    fbe_test_init_random_seed();
    if (fbe_test_util_is_simulation())
    {
		/* We reload packages during test setup. So counters
		 * are already Zeroed.
		 */
        fbe_test_wait_for_all_pvds_ready();
    }
    else
    {
       /* Reset error counters before starting the test.
        */
        fbe_test_sep_util_sep_neit_physical_reset_error_counters();

        /* unconfigure all spare drives from the system 
        */
        fbe_test_sep_util_unconfigure_all_spares_in_the_system();

        fbe_test_sep_util_fail_drive();

        /* Verify that all PDO objects in ready state. If it is in fail state 
         * then try to bring it online.
         */
        fbe_test_sep_util_verify_pdo_objects_state_in_the_system();
    }
    fbe_test_sep_util_randmly_disable_system_zeroing();
    fbe_test_sep_reset_encryption_enabled();
    return FBE_STATUS_OK;
}
/*******************************************
 * end fbe_test_common_util_test_setup_init()
 *******************************************/


/*!*******************************************************************
 * @var fbe_test_default_extended_test_level
 *********************************************************************
 * @brief This is the default raid test level for this executable
 *
 *********************************************************************/
static fbe_u32_t fbe_test_default_extended_test_level = 0; 

/*!**************************************************************
 * fbe_test_set_default_extended_test_level()
 ****************************************************************
 * @brief
 *  Allows the client to change the default raid test level.
 *
 * @param  None.                
 *
 * @return None.   
 *
 * @author
 *  9/20/2011 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_set_default_extended_test_level(fbe_u32_t level)
{
    fbe_test_default_extended_test_level = level;
}
/******************************************
 * end fbe_test_set_default_extended_test_level()
 ******************************************/

/*!**************************************************************
 * fbe_test_get_default_extended_test_level()
 ****************************************************************
 * @brief
 *  Allows the client to change the default raid test level.
 *
 * @param  None.                
 *
 * @return Current default.  
 *
 * @author
 *  9/20/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_u32_t fbe_test_get_default_extended_test_level(void)
{
    return fbe_test_default_extended_test_level;
}
/******************************************
 * end fbe_test_get_default_extended_test_level()
 ******************************************/
/*!**************************************************************
 * fbe_sep_util_destroy_neit_sep_phy_expect_errs
 ****************************************************************
 * @brief
 *  This function destroys the packages but does not check
 *  error counters since the test intentionally injected
 *  all kinds of software errors.
 * 
 * @param None
 *
 * @return none
 *
 * @author
 * 08/10/2010 - Created. Jyoti Ranjan
 * 
 ****************************************************************/
void fbe_sep_util_destroy_neit_sep_phy_expect_errs(void)
{
    fbe_status_t status;
    fbe_test_package_list_t package_to_unload;    
    fbe_package_id_t current_package_id;
    fbe_u32_t package_index;
    fbe_zero_memory(&package_to_unload, sizeof(package_to_unload));

    mut_printf(MUT_LOG_LOW, "=== %s starts ===\n", __FUNCTION__);

    if(is_esp_loaded == FBE_TRUE)
    {
        package_to_unload.number_of_packages = 4;
        package_to_unload.package_list[0] = FBE_PACKAGE_ID_NEIT;
        package_to_unload.package_list[1] = FBE_PACKAGE_ID_SEP_0;
    	package_to_unload.package_list[2] = FBE_PACKAGE_ID_ESP;
        package_to_unload.package_list[3] = FBE_PACKAGE_ID_PHYSICAL;
    }
    else
    {
        package_to_unload.number_of_packages = 3;
        package_to_unload.package_list[0] = FBE_PACKAGE_ID_NEIT;
        package_to_unload.package_list[1] = FBE_PACKAGE_ID_SEP_0;
    	//package_to_unload.package_list[2] = FBE_PACKAGE_ID_ESP;
        package_to_unload.package_list[2] = FBE_PACKAGE_ID_PHYSICAL;
    }

    fbe_test_common_util_package_destroy(&package_to_unload);
    status = fbe_api_terminator_destroy();
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "destroy terminator api failed");
    
    /* print the error trace count*/
    for (package_index = 0; package_index < package_to_unload.number_of_packages; package_index++ ) 
    {
        current_package_id = package_to_unload.package_list[package_index];
        mut_printf(MUT_LOG_HIGH, 
                   "%s: Package ID 0x%X has trace_error_counter  = 0x%x and "
                   "trace_critical_error_counter = 0x%x\n",__FUNCTION__,
                   current_package_id,
                   package_to_unload.package_error[current_package_id].trace_error_counter,
                   package_to_unload.package_error[current_package_id].trace_critical_error_counter);
    }

    mut_printf(MUT_LOG_LOW, "=== %s completes ===\n", __FUNCTION__);

    return;
}
/***************************
 * end fbe_sep_util_destroy_neit_sep_phy_expect_errs()
 **************************/
