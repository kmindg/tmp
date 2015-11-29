/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_test_provision_drive.c
 ***************************************************************************
 *
 * @brief
 *  This file contains provision drive test related functions.
 *
 * @version
 *   10/17/2012:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "sep_tests.h"
#include "sep_utils.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_database_interface.h"

#include "fbe_base_object_trace.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/


/*!**************************************************************
 * fbe_test_wait_for_all_pvds_ready()
 ****************************************************************
 * @brief
 *  This function waits for all physical drives to have a
 *  provision drive and waits for that provision drive to be ready.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  10/17/2012 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_wait_for_all_pvds_ready(void)
{
    fbe_status_t status;
    fbe_u32_t num_objects = 0;
    fbe_object_id_t *obj_list_p = NULL;
    fbe_physical_drive_information_t drive_info;
    fbe_u32_t index;
    fbe_object_id_t pvd_object_id;
    fbe_u32_t timeout_ms = 0;
    fbe_class_id_t sas_drv_classs_id;
    fbe_bool_t bootflash_mode_flag = FBE_FALSE;

    status = fbe_api_database_check_bootflash_mode(&bootflash_mode_flag);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    for (sas_drv_classs_id = FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_FIRST + 1; sas_drv_classs_id < FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_LAST; sas_drv_classs_id++)
    {
        status = fbe_api_enumerate_class(sas_drv_classs_id, FBE_PACKAGE_ID_PHYSICAL, &obj_list_p, &num_objects);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "waiting for %d pvds to be ready, class %d", num_objects, sas_drv_classs_id);

        for (index = 0; index < num_objects; index++)
        {
            /* we will not allow user drive online on boot flash mode */
            if (obj_list_p[index] > 4 && bootflash_mode_flag)
            {
                break;
            }
            status = fbe_api_physical_drive_get_drive_information(obj_list_p[index], &drive_info, FBE_PACKET_FLAG_NO_ATTRIB);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            while (timeout_ms < FBE_TEST_WAIT_TIMEOUT_MS)
            {
                status = fbe_api_provision_drive_get_obj_id_by_location(drive_info.port_number, drive_info.enclosure_number, drive_info.slot_number,
                                                                        &pvd_object_id);
                if ((status != FBE_STATUS_OK) || (pvd_object_id == FBE_OBJECT_ID_INVALID))
                {
                    mut_printf(MUT_LOG_TEST_STATUS, "pvd for %d_%d_%d does not exist yet status: %d ",
                               drive_info.port_number, drive_info.enclosure_number, drive_info.slot_number, status);
                    timeout_ms += 1000;
                    fbe_api_sleep(1000);
                }
                else
                {
                    break;
                }
            }
            if (timeout_ms >= FBE_TEST_WAIT_TIMEOUT_MS)
            {
                MUT_FAIL_MSG("Timeout on waiting for pvd object");
            }
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            MUT_ASSERT_INT_NOT_EQUAL(pvd_object_id, FBE_OBJECT_ID_INVALID);
            status = fbe_api_wait_for_object_lifecycle_state(pvd_object_id, FBE_LIFECYCLE_STATE_READY, FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            mut_printf(MUT_LOG_TEST_STATUS, "pvd: %d_%d_%d  object_id: 0x%x is ready.",
                       drive_info.port_number, drive_info.enclosure_number, drive_info.slot_number, pvd_object_id);
        }

        mut_printf(MUT_LOG_TEST_STATUS, "waiting for %d pvds to be ready...Complete", num_objects);
        if (obj_list_p!=NULL)
        {
            fbe_api_free_memory(obj_list_p);
        }
    }
}
/******************************************
 * end fbe_test_wait_for_all_pvds_ready()
 ******************************************/

/*!**************************************************************
 * fbe_test_wait_for_rg_pvds_ready()
 ****************************************************************
 * @brief
 *  This function waits for all rg pvds to be ready.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  1/18/2013 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_wait_for_rg_pvds_ready(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status;
    fbe_u32_t timeout_ms = 0;
    fbe_u32_t position;
    fbe_object_id_t pvd_object_id;

    for (position = 0; position < rg_config_p->width; position++)
    {
        while (timeout_ms < FBE_TEST_WAIT_TIMEOUT_MS)
        {

            status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[position].bus,
                                                                    rg_config_p->rg_disk_set[position].enclosure,
                                                                    rg_config_p->rg_disk_set[position].slot,
                                                                    &pvd_object_id);
            if ((status != FBE_STATUS_OK) || (pvd_object_id == FBE_OBJECT_ID_INVALID))
            {
                mut_printf(MUT_LOG_TEST_STATUS, "pvd for %d_%d_%d does not exist yet status: %d ",
                           rg_config_p->rg_disk_set[position].bus,
                           rg_config_p->rg_disk_set[position].enclosure,
                           rg_config_p->rg_disk_set[position].slot, status);
                timeout_ms += 100;
                fbe_api_sleep(100);
            }
            else
            {
                break;
            }
        }
        if (timeout_ms >= FBE_TEST_WAIT_TIMEOUT_MS)
        {
            MUT_FAIL_MSG("Timeout on waiting for pvd object");
        }
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_wait_for_object_lifecycle_state(pvd_object_id, FBE_LIFECYCLE_STATE_READY, FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
}
/******************************************
 * end fbe_test_wait_for_rg_pvds_ready()
 ******************************************/

/*!**************************************************************
 *          fbe_test_provision_drive_get_paged_metadata_summary()
 ****************************************************************
 *
 * @brief   Display the paged metadata.
 *
 * @param   pvd object_id
 * @param   b_force_unit_access - FBE_TRUE - Must get data from disk
 *
 * @return  None.
 *
 * @author
 *  08/06/2015  Ron Proulx  - Created
 *
 ****************************************************************/
void fbe_test_provision_drive_get_paged_metadata_summary(fbe_object_id_t pvd_object_id, fbe_bool_t b_force_unit_access)
{
    fbe_u32_t index;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_api_provision_drive_get_paged_info_t paged_info;

    status = fbe_api_provision_drive_get_paged_bits(pvd_object_id, &paged_info, b_force_unit_access);
    if (status != FBE_STATUS_OK)
    {
        MUT_ASSERT_TRUE_MSG((status == FBE_STATUS_OK),"pvd_info error on get paged bits");
        return;
    }
    mut_printf(MUT_LOG_TEST_STATUS,"chunk_count:        0x%llx", (unsigned long long)paged_info.chunk_count);
    mut_printf(MUT_LOG_TEST_STATUS,"num_valid_chunks:   0x%llx", paged_info.num_valid_chunks);
    mut_printf(MUT_LOG_TEST_STATUS,"num_nz_chunks:      0x%llx", paged_info.num_nz_chunks);
    mut_printf(MUT_LOG_TEST_STATUS,"num_uz_chunks:      0x%llx", paged_info.num_uz_chunks);
    mut_printf(MUT_LOG_TEST_STATUS,"num_cons_chunks:    0x%llx", paged_info.num_cons_chunks);
    mut_printf(MUT_LOG_TEST_STATUS,"\n");
    mut_printf(MUT_LOG_TEST_STATUS,"valid nz uz cons: count\n");

    /* Valid set first.
     */
    for ( index = 0; index < FBE_API_PROVISION_DRIVE_BIT_COMBINATIONS; index++)
    {
        if (index & 1)
        {
            mut_printf(MUT_LOG_TEST_STATUS," %d    %d  %d    %d      0x%lld",
                           (index & 1), /* Is valid set ? */
                           (index & (1 << 1)) ? 1 : 0,  /* Is nz set? */
                           (index & (1 << 2)) ? 1 : 0,  /* Is uz set? */
                           (index & (1 << 3)) ? 1 : 0,  /* Is cons set? */
                            paged_info.bit_combinations_count[index]);
        }
    }

    /* Valid not set last.
     */
    for ( index = 0; index < FBE_API_PROVISION_DRIVE_BIT_COMBINATIONS; index++)
    {
        if ((index & 1) == 0)
        {
            mut_printf(MUT_LOG_TEST_STATUS," %d    %d  %d    %d      %lld",
                           (index & 1), /* Is valid set ? */
                           (index & (1 << 1)) ? 1 : 0,  /* Is nz set? */
                           (index & (1 << 2)) ? 1 : 0,  /* Is uz set? */
                           (index & (1 << 3)) ? 1 : 0,  /* Is cons set? */
                            paged_info.bit_combinations_count[index]);
        }
    }
    return;
}
/************************************************************
 * end fbe_test_provision_drive_get_paged_metadata_summary()
 ************************************************************/


/**************************************
 * end file fbe_test_provision_drive.c
 **************************************/
