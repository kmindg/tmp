/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_test_common_reboot.c
 ***************************************************************************
 *
 * @brief
 *  This file contains common notification functions.
 *  Created from the fbe_test_pp_util notification functions.
 *
 * @version
 *   09/27/2011 - Created. Amit Dhaduk
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_sim_transport.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_panic_sp_interface.h"
#include "fbe_test_common_utils.h"
#include "sep_utils.h"
#include "sep_tests.h"
#include "fbe_test.h"
#include "mut.h"

/*************************
 *   DEFINITIONS
 *************************/

/*!*******************************************************************
 * @def FBE_TEST_COMMON_REBOOT_WAIT_MSEC
 *********************************************************************
 * @brief Number of seconds we should wait for state changes.
 *        We set this relatively large so that if we do exceed this
 *        we will be sure something is wrong.
 *
 *********************************************************************/
#define FBE_TEST_COMMON_REBOOT_WAIT_MSEC    50000

/*************************
 *   VARIABLE DEFINITIONS
 *************************/

/*!*******************************************************************
 * @var     fbe_test_common_reboot_sep_params
 *********************************************************************
 * @brief   This is a local copy of the reboot parameters so that
 *          the request values are not over-written
 *
 *********************************************************************/
static fbe_sep_package_load_params_t fbe_test_common_reboot_sep_params = { 0 };


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!****************************************************************************
 *          fbe_test_common_reboot_save_sep_params()
 ******************************************************************************
 *
 * @brief   Save the requested boot parameters
 *
 * @param   sep_params_p - NULL - Load using default (i.e. without params)
 *                         Other - Load he paramesters specified.
 * @return  status
 *
 *****************************************************************************/
fbe_status_t fbe_test_common_reboot_save_sep_params(fbe_sep_package_load_params_t *sep_params_p)
{
    fbe_u32_t   hook_index;

    if (sep_params_p != NULL) {
        /*! @note Currently the only load parameter that are preserved are the 
         *        debug hooks and any reduced memory configuration.
         */
        for (hook_index = 0; hook_index < MAX_SCHEDULER_DEBUG_HOOKS; hook_index++) {
            /* Only save it if it is valid.
             */
            if ((sep_params_p->scheduler_hooks[hook_index].object_id != 0)                                  &&
                (sep_params_p->scheduler_hooks[hook_index].object_id != FBE_OBJECT_ID_INVALID)              &&
                (sep_params_p->scheduler_hooks[hook_index].monitor_state > SCHEDULER_MONITOR_STATE_INVALID) &&
                (sep_params_p->scheduler_hooks[hook_index].monitor_state < SCHEDULER_MONITOR_STATE_LAST)       ) {
                fbe_test_common_reboot_sep_params.scheduler_hooks[hook_index] = sep_params_p->scheduler_hooks[hook_index];
            } else {
                fbe_zero_memory(&fbe_test_common_reboot_sep_params.scheduler_hooks[hook_index], sizeof(fbe_scheduler_debug_hook_t));
            }
        }
        if (sep_params_p->init_data_params.number_of_main_chunks != 0) {
            fbe_test_common_reboot_sep_params.init_data_params = sep_params_p->init_data_params;
        }

        fbe_test_common_reboot_sep_params.flags = sep_params_p->flags;
    }

    return FBE_STATUS_OK;
}
/**********************************************
 * end fbe_test_common_reboot_save_sep_params()
 **********************************************/

/*!****************************************************************************
 *          fbe_test_common_reboot_restore_sep_params()
 ******************************************************************************
 *
 * @brief   Restore the requested boot parameters
 *
 * @param   sep_params_p - NULL - Load using default (i.e. without params)
 *                         Other - Load he paramesters specified.
 * @return  status
 *
 *****************************************************************************/
fbe_status_t fbe_test_common_reboot_restore_sep_params(fbe_sep_package_load_params_t *sep_params_p)
{
    fbe_u32_t   hook_index;

    if (sep_params_p != NULL) {
        /*! @note Currently the only load parameter that are preserved are the 
         *        debug hooks and any reduced memory configuration.
         */
        for (hook_index = 0; hook_index < MAX_SCHEDULER_DEBUG_HOOKS; hook_index++) {
            /* Only restore it if it is valid.
             */
            if ((fbe_test_common_reboot_sep_params.scheduler_hooks[hook_index].object_id != 0)                                  &&
                (fbe_test_common_reboot_sep_params.scheduler_hooks[hook_index].object_id != FBE_OBJECT_ID_INVALID)              &&
                (fbe_test_common_reboot_sep_params.scheduler_hooks[hook_index].monitor_state > SCHEDULER_MONITOR_STATE_INVALID) &&
                (fbe_test_common_reboot_sep_params.scheduler_hooks[hook_index].monitor_state < SCHEDULER_MONITOR_STATE_LAST)       ) {
                sep_params_p->scheduler_hooks[hook_index] = fbe_test_common_reboot_sep_params.scheduler_hooks[hook_index];
            }
        }

        if (fbe_test_common_reboot_sep_params.init_data_params.number_of_main_chunks != 0) {
            sep_params_p->init_data_params = fbe_test_common_reboot_sep_params.init_data_params;
        }

        sep_params_p->flags |= fbe_test_common_reboot_sep_params.flags;
    }

    return FBE_STATUS_OK;
}
/************************************************
 * end fbe_test_common_reboot_restore_sep_params()
 ************************************************/

/*!****************************************************************************
 *          fbe_test_common_reboot_this_sp()
 ******************************************************************************
 *
 * @brief   This is the reboot SP function.
 *
 * @param   sep_params_p - NULL - Load using default (i.e. without params)
 *                         Other - Load he paramesters specified.
 * @param   neit_params_p - NULL - Load using default (i.e. without params)
 *                         Other - Load he paramesters specified.
 * @return  status
 *
 * @author
 *   09/27/2011 - Created. Amit Dhaduk
 *****************************************************************************/
fbe_status_t fbe_test_common_reboot_this_sp(fbe_sep_package_load_params_t *sep_params_p, 
                                            fbe_neit_package_load_params_t *neit_params_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_sim_transport_connection_target_t   current_sp;

    fbe_test_mark_sp_up(fbe_test_conn_target_to_sp(fbe_api_sim_transport_get_target_server()));
    /* Get the current target server.
     */
    current_sp = fbe_api_sim_transport_get_target_server();

    /* shutdown the logical package - sep & neit*/
    if (fbe_test_sep_util_get_encryption_test_mode()) {
        fbe_test_sep_util_destroy_kms();
    }
    fbe_test_sep_util_destroy_neit_sep();

    // let's wait awhile before bringing SPA back up
    fbe_api_sleep(2000);

    status = fbe_api_sim_transport_set_target_server(current_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Populate the parameters.*/
    if (sep_params_p != NULL)
    {
        fbe_test_common_reboot_save_sep_params(sep_params_p);
        sep_config_fill_load_params(sep_params_p);
        fbe_test_common_reboot_restore_sep_params(sep_params_p);
    }

    /* load sep and neit package with preset the debug hook */
    sep_config_load_sep_and_neit_params(sep_params_p, neit_params_p);
    if (fbe_test_sep_util_get_encryption_test_mode()) {
        sep_config_load_kms(NULL);
    }

    /* Allow objects to be load-balanced between the SPs. */
	fbe_api_database_set_load_balance(FBE_TRUE);

    return status;
}
/******************************************
 * end fbe_test_common_reboot_this_sp()
 ******************************************/

/*!****************************************************************************
 *          fbe_test_common_reboot_sp()
 ******************************************************************************
 *
 * @brief   This is the reboot SP function.
 *
 * @param   sp_to_reboot - SP to reboot
 * @param   sep_params_p - NULL - Load using default (i.e. without params)
 *                         Other - Load he paramesters specified.
 * @param   neit_params_p - NULL - Load using default (i.e. without params)
 *                         Other - Load he paramesters specified.
 * @return  status
 *
 * @author
 *   09/27/2011 - Created. Amit Dhaduk
 *****************************************************************************/
fbe_status_t fbe_test_common_reboot_sp(fbe_sim_transport_connection_target_t sp_to_reboot,
                                       fbe_sep_package_load_params_t *sep_params_p, 
                                       fbe_neit_package_load_params_t *neit_params_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_sim_transport_connection_target_t   current_sp = sp_to_reboot;

    /* shutdown the logical package - sep & neit*/
    if (fbe_test_sep_util_get_encryption_test_mode()) {
        fbe_test_sep_util_destroy_kms();
    }
    fbe_test_sep_util_destroy_neit_sep();

    // let's wait awhile before bringing SPA back up
    fbe_api_sleep(2000);

    status = fbe_api_sim_transport_set_target_server(current_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_test_mark_sp_up(fbe_test_conn_target_to_sp(fbe_api_sim_transport_get_target_server()));

    /* Populate the parameters.*/
    if (sep_params_p != NULL)
    {
        fbe_test_common_reboot_save_sep_params(sep_params_p);
        sep_config_fill_load_params(sep_params_p);
        fbe_test_common_reboot_restore_sep_params(sep_params_p);
    }

    /* load sep and neit package with preset the debug hook */
    sep_config_load_sep_and_neit_params(sep_params_p, neit_params_p);
    if (fbe_test_sep_util_get_encryption_test_mode()) {
        sep_config_load_kms(NULL);
    }
    /* Allow objects to be load-balanced between the SPs. */
	fbe_api_database_set_load_balance(FBE_TRUE);
    
    return status;
}
/******************************************
 * end fbe_test_common_reboot_sp()
 ******************************************/

/*!****************************************************************************
 *          fbe_test_common_boot_sp()
 ******************************************************************************
 *
 * @brief   This is the boot SP function.
 *
 * @param   sp_to_reboot - SP to reboot
 * @param   sep_params_p - NULL - Load using default (i.e. without params)
 *                         Other - Load he paramesters specified.
 * @param   neit_params_p - NULL - Load using default (i.e. without params)
 *                         Other - Load he paramesters specified.
 * @return  status
 *
 * @author
 *   09/27/2011 - Created. Amit Dhaduk
 *****************************************************************************/
fbe_status_t fbe_test_common_boot_sp(fbe_sim_transport_connection_target_t sp_to_reboot,
                                     fbe_sep_package_load_params_t *sep_params_p, 
                                     fbe_neit_package_load_params_t *neit_params_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_sim_transport_connection_target_t   current_sp;

    /* Get the current SP.
     */
    current_sp = fbe_api_sim_transport_get_target_server();

    /* Set the target server to the desired SP.
     */
    status = fbe_api_sim_transport_set_target_server(sp_to_reboot);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_test_mark_sp_up(fbe_test_conn_target_to_sp(fbe_api_sim_transport_get_target_server()));

    /* Populate the parameters.*/
    if (sep_params_p != NULL)
    {
        fbe_test_common_reboot_save_sep_params(sep_params_p);
        sep_config_fill_load_params(sep_params_p);
        fbe_test_common_reboot_restore_sep_params(sep_params_p);
    }

    /* Load sep and neit package with preset the debug hook */
    sep_config_load_sep_and_neit_params(sep_params_p, neit_params_p);

    /* Allow objects to be load-balanced between the SPs. */
	fbe_api_database_set_load_balance(FBE_TRUE);

    /* Restore the current SP to what it was when we entered.
     */
    status = fbe_api_sim_transport_set_target_server(current_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return status;
}
/******************************************
 * end fbe_test_common_boot_sp()
 ******************************************/

/*!****************************************************************************
 *          fbe_test_common_reboot_both_sps()
 ******************************************************************************
 *
 * @brief   Reboot both SPs (simulate an NDU ?)
 *
 * @param   None
 *
 * @return  status
 * 
 * @note    Not clear what the purpose of this method it.  Is it suppose to
 *          simulate a disruptive NDU ??!!  The array loosing power ??
 *
 * @author
 *   09/27/2011 - Created. Amit Dhaduk
 * 
 *****************************************************************************/
fbe_status_t fbe_test_common_reboot_both_sps(void)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* reboot both SPs at the same time
     */
    fbe_test_sep_util_destroy_neit_sep_both_sps();

    /* Reboot both SPs allowing load-balancing of objects across the SPs. */
    sep_config_load_sep_and_neit_load_balance_both_sps();

    status = fbe_test_sep_util_wait_for_database_service(FBE_TEST_COMMON_REBOOT_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return status;
}
/******************************************
 * fbe_test_common_reboot_both_sps()
 ******************************************/

fbe_status_t fbe_test_common_panic_both_sps(void)
{
	fbe_transport_connection_target_t   current_sp;

	/* Panic both SP's */
	current_sp = fbe_api_transport_get_target_server();

	fbe_api_transport_set_target_server(FBE_TRANSPORT_SP_A);
    fbe_test_mark_sp_down(fbe_test_conn_target_to_sp(fbe_api_sim_transport_get_target_server()));
	fbe_api_panic_sp_async();

	fbe_api_transport_set_target_server(FBE_TRANSPORT_SP_B);
    fbe_test_mark_sp_down(fbe_test_conn_target_to_sp(fbe_api_sim_transport_get_target_server()));
	fbe_api_panic_sp_async();

	fbe_api_transport_set_target_server(current_sp);

	return FBE_STATUS_OK;
}


/*!****************************************************************************
 *          fbe_test_common_reboot_sp_neit_sep_kms()
 ******************************************************************************
 *
 * @brief   This is the reboot SP function.
 *
 * @param   sp_to_reboot - SP to reboot
 * @param   sep_params_p - NULL - Load using default (i.e. without params)
 *                         Other - Load he paramesters specified.
 * @param   neit_params_p - NULL - Load using default (i.e. without params)
 *                         Other - Load he paramesters specified.
 * @return  status
 *
 * @author
 *  12/11/2013  Lili Chen  - Created.
 *****************************************************************************/
fbe_status_t fbe_test_common_reboot_sp_neit_sep_kms(fbe_sim_transport_connection_target_t sp_to_reboot,
                                       fbe_sep_package_load_params_t *sep_params_p, 
                                       fbe_neit_package_load_params_t *neit_params_p,
                                       fbe_kms_package_load_params_t *kms_params_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_sim_transport_connection_target_t   current_sp;

    current_sp = fbe_api_sim_transport_get_target_server();

    status = fbe_api_sim_transport_set_target_server(sp_to_reboot);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* shutdown the logical package - sep & neit*/
    fbe_test_sep_util_destroy_neit_sep_kms();

    // let's wait awhile before bringing SPA back up
    fbe_api_sleep(2000);

    status = fbe_api_sim_transport_set_target_server(sp_to_reboot);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Populate the parameters.*/
    if (sep_params_p != NULL)
    {
        fbe_test_common_reboot_save_sep_params(sep_params_p);
        sep_config_fill_load_params(sep_params_p);
        fbe_test_common_reboot_restore_sep_params(sep_params_p);
    }

    /* load sep and neit package with preset the debug hook */
    sep_config_load_sep_and_neit_params(sep_params_p, neit_params_p);

    /* Allow objects to be load-balanced between the SPs. */
	fbe_api_database_set_load_balance(FBE_TRUE);

    /* load kms package with preset the debug hook */
    sep_config_load_kms(kms_params_p);

    status = fbe_api_sim_transport_set_target_server(current_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return status;
}
/******************************************
 * end fbe_test_common_reboot_sp_neit_sep_kms()
 ******************************************/

fbe_status_t fbe_test_common_reboot_both_sps_neit_sep_kms(fbe_sim_transport_connection_target_t active_sp,
                                       fbe_sep_package_load_params_t *sep_params_p, 
                                       fbe_neit_package_load_params_t *neit_params_p,
                                       fbe_kms_package_load_params_t *kms_params_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;

    fbe_test_sep_util_destroy_neit_sep_kms_both_sps();

    // let's wait awhile before bringing both sps back up
    fbe_api_sleep(2000);

    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Populate the parameters.*/
    if (sep_params_p != NULL)
    {
        fbe_test_common_reboot_save_sep_params(sep_params_p);
        sep_config_fill_load_params(sep_params_p);
        fbe_test_common_reboot_restore_sep_params(sep_params_p);
    }

    /* load sep and neit package with preset the debug hook */
    sep_config_load_sep_and_neit_params(sep_params_p, neit_params_p);

    /* Allow objects to be load-balanced between the SPs. */
	fbe_api_database_set_load_balance(FBE_TRUE);

    /* load kms package with preset the debug hook */
    sep_config_load_kms(kms_params_p);

    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Populate the parameters.*/
    if (sep_params_p != NULL)
    {
        fbe_test_common_reboot_save_sep_params(sep_params_p);
        sep_config_fill_load_params(sep_params_p);
        fbe_test_common_reboot_restore_sep_params(sep_params_p);
    }

    /* load sep and neit package with preset the debug hook */
    sep_config_load_sep_and_neit_params(sep_params_p, neit_params_p);

    /* Allow objects to be load-balanced between the SPs. */
	fbe_api_database_set_load_balance(FBE_TRUE);

    /* load kms package with preset the debug hook */
    sep_config_load_kms(kms_params_p);

    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return status;
}

/************************************
 * end file fbe_test_common_reboot.c
 ************************************/
