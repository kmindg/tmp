/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
 
/*!*************************************************************************
 * @file fbe_test_pp_utils.c
 ***************************************************************************
 *
 * @brief
 *  This file contains generic configuration utils to help
 *  creating physical objects.
 *
 * @version
 *   10/1/2009:  Created. Rob Foley
 *
 ***************************************************************************/
 
/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_power_supply_interface.h"
#include "fbe/fbe_api_cooling_interface.h"
#include "pp_utils.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_notification_lib.h"
#include "EmcPAL_Misc.h"
#include "fbe_private_space_layout.h"
#include "fbe/fbe_api_board_interface.h"
#include "sep_utils.h"

#define FBE_API_POLLING_INTERVAL 200 /*ms*/


/*!*******************************************************************
 * @var fbe_test_pp_util_unique_sas_address
 *********************************************************************
 * @brief Every time someone asks for a new sas address we will increment
 *        this number.
 *
 *********************************************************************/
static fbe_sas_address_t fbe_test_pp_util_unique_sas_address = 0x6000097B80000000;

#define GET_SAS_DRIVE_SAS_ADDRESS(BN, EN, SN) \
    (0x5000097B80000000 + ((fbe_u64_t)(BN) << 16) + ((fbe_u64_t)(EN) << 8) + (SN))
#define GET_SATA_DRIVE_SAS_ADDRESS(BN, EN, SN) \
    (0x5000097A80000000 + ((fbe_u64_t)(BN) << 16) + ((fbe_u64_t)(EN) << 8) + (SN))
#define GET_SAS_ENCLOSURE_SAS_ADDRESS(BN, EN) \
    (0x5000097A79000000 + ((fbe_u64_t)(EN) << 16) + BN)
#define GET_SAS_PMC_PORT_SAS_ADDRESS(BN) \
    (0x5000097A7800903F + (BN))

static fbe_notification_registration_id_t reg_id;

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
static fbe_status_t fbe_test_pp_util_internal_insert_voyager_sas_enclosure (fbe_terminator_api_device_handle_t parent_handle,
                                                      fbe_terminator_sas_encl_info_t *encl_info, 
                                                      fbe_terminator_api_device_handle_t *encl_handle,
                                                      fbe_u32_t *num_handles);
static fbe_status_t fbe_test_pp_util_internal_insert_viking_sas_enclosure (fbe_terminator_api_device_handle_t parent_handle,
                                                      fbe_terminator_sas_encl_info_t *encl_info, 
                                                      fbe_terminator_api_device_handle_t *encl_handle,
                                                      fbe_u32_t *num_handles);
static fbe_status_t fbe_test_pp_util_internal_insert_cayenne_sas_enclosure (fbe_terminator_api_device_handle_t parent_handle,
                                                      fbe_terminator_sas_encl_info_t *encl_info, 
                                                      fbe_terminator_api_device_handle_t *encl_handle,
                                                      fbe_u32_t *num_handles);
static fbe_status_t fbe_test_pp_util_internal_insert_naga_sas_enclosure (fbe_terminator_api_device_handle_t parent_handle,
                                                      fbe_terminator_sas_encl_info_t *encl_info, 
                                                      fbe_terminator_api_device_handle_t *encl_handle,
                                                      fbe_u32_t *num_handles);

static fbe_status_t fbe_test_pp_util_insert_sas_expanders(fbe_terminator_api_device_handle_t parent_handle,
                                                        fbe_terminator_sas_encl_info_t *encl_info, 
                                                        fbe_terminator_api_device_handle_t *encl_handle,
                                                        fbe_u32_t *num_handles,
                                                        fbe_sas_enclosure_type_t sasEnclType,
                                                        fbe_u8_t firstDrvExp,
                                                        fbe_u8_t numDrvExp);

static fbe_status_t fbe_test_pp_util_internal_insert_voyager_sas_drive (fbe_terminator_api_device_handle_t encl_handle,
                                       fbe_u32_t slot_number,
                                       fbe_terminator_sas_drive_info_t *drive_info, 
                                       fbe_terminator_api_device_handle_t *drive_handle);
static fbe_sas_drive_type_t fbe_test_pp_util_default_520_sas_drive_type = FBE_SAS_DRIVE_SIM_520;
static fbe_sas_drive_type_t fbe_test_pp_util_default_4k_sas_drive_type = FBE_SAS_DRIVE_UNICORN_4160;

/*!**************************************************************
 * fbe_test_pp_util_get_next_sas_address()
 ****************************************************************
 * @brief
 *  Get next sas address
 *
 * @param  -  none              
 *
 * @return fbe_sas_address_t a unique address.   
 *
 * @author
 *  11/11/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_sas_address_t fbe_test_pp_util_get_next_sas_address(void)
{
    return fbe_test_pp_util_unique_sas_address;
}
/******************************************
 * end fbe_test_pp_util_get_next_sas_address()
 ******************************************/

/*!**************************************************************
 * fbe_test_pp_util_get_unique_sas_address()
 ****************************************************************
 * @brief
 *  Get a unique sas address and increment this global.
 *
 * @param  -               
 *
 * @return fbe_sas_address_t a unique address.   
 *
 * @author
 *  11/11/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_sas_address_t fbe_test_pp_util_get_unique_sas_address(void)
{
    fbe_sas_address_t new_address = fbe_test_pp_util_unique_sas_address;
    fbe_test_pp_util_unique_sas_address++;
    return new_address;
}
/******************************************
 * end fbe_test_pp_util_get_unique_sas_address()
 ******************************************/

/*!**************************************************************
 * fbe_test_pp_util_get_extended_testing_level()
 ****************************************************************
 * @brief
 *  Return if we should execute additional extended testing.
 *
 * @param none       
 *
 * @return - fbe_u32_t - The level of extended testing we should
 *                       perform.  Typically 0 means a qual test,
 *                       and 1 and above is extended testing
 *                       as defined by the test.
 *
 ****************************************************************/
fbe_u32_t fbe_test_pp_util_get_extended_testing_level(void)
{
    extern fbe_u32_t fbe_sep_test_util_get_extended_testing_level(void);
    return fbe_sep_test_util_get_extended_testing_level();
}
/******************************************
 * end fbe_test_pp_util_get_extended_testing_level()
 ******************************************/

/*!**************************************************************
 * fbe_test_pp_utils_get_required_physical_blocks_per_drive()
 ****************************************************************
 * @brief
 *  Convert the requested capacity to the number of blocks required
 *  per drive.
 *
 * @param   required_capacity - Number of 520-bps blocks required
 * @param   drive_block_size - The physical block size of the drive
 * @param   required_blocks_per_drive_p - Addresss of required blocks
 *              (each block contains the bytes per physical block)
 * 
 * @return  status
 *
 ****************************************************************/
static fbe_status_t fbe_test_pp_utils_get_required_physical_blocks_per_drive(fbe_lba_t required_capacity,
                                                                    fbe_block_size_t drive_block_size,
                                                                    fbe_lba_t *required_blocks_per_drive_p)
{
    /*! @note Currently these are the only block size supported.
     */
    *required_blocks_per_drive_p = 0;
    switch(drive_block_size)
    {
        case FBE_BE_BYTES_PER_BLOCK:
            *required_blocks_per_drive_p = required_capacity;
            break;

        case FBE_4K_BYTES_PER_BLOCK:
            *required_blocks_per_drive_p = required_capacity / (FBE_4K_BYTES_PER_BLOCK / FBE_BE_BYTES_PER_BLOCK);
            break;

        default:
            /* The requested block size is not supported.
             */
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "=== %s Requested block size: %d NOT supported !!!",
                       __FUNCTION__, drive_block_size);
            MUT_ASSERT_INT_EQUAL(FBE_BE_BYTES_PER_BLOCK, drive_block_size);
            return FBE_STATUS_GENERIC_FAILURE;
    }
    
    return FBE_STATUS_OK;
}
/*********************************************************
 * end fbe_test_pp_utils_get_required_physical_blocks_per_drive()
 *********************************************************/

/*!**************************************************************
 * fbe_test_get_sas_drive_info_extend()
 ****************************************************************
 * @brief
 *  This function generates a new SAS drive .
 *
 * @param sas_drive_p - sas drive
 * @param backend_number - bus or port.
 * @param encl_number - enclosure number on that port
 * @param slot_number - number of the drive on that enclosure.
 * @param block_size - Number of bytes per block.
 * @param required_capacity - Number of client blocks required
 * @param sas_address - sas address
 * @param drive_type - drive type
 * 
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_test_get_sas_drive_info_extend(fbe_terminator_sas_drive_info_t *sas_drive_p,
                                         fbe_u32_t backend_number,
                                         fbe_u32_t encl_number,
                                         fbe_u32_t slot_number,
                                         fbe_block_size_t block_size,
                                         fbe_lba_t required_capacity,
                                         fbe_sas_address_t sas_address,
                                         fbe_sas_drive_type_t drive_type)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_lba_t       blocks_per_drive = required_capacity;

    status = fbe_test_pp_utils_get_required_physical_blocks_per_drive(required_capacity, block_size, &blocks_per_drive);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    sas_drive_p->backend_number = backend_number;
    sas_drive_p->encl_number = encl_number;
    sas_drive_p->sas_address = sas_address;
    sas_drive_p->drive_type = drive_type;
    sas_drive_p->capacity = blocks_per_drive;
    sas_drive_p->block_size = block_size;
    fbe_sprintf(sas_drive_p->drive_serial_number, sizeof(sas_drive_p->drive_serial_number), "%llX", (unsigned long long)sas_drive_p->sas_address);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_test_get_sas_drive_info_extend()
 ******************************************/
/*!**************************************************************
 * fbe_test_pp_utils_set_default_520_sas_drive()
 ****************************************************************
 * @brief
 *  Set the default 520 sas drive
 *
 * @param   drive_type_p - Addresss of drive type to populate
 *
 * @note    Only works in simulation environment.
 * 
 * @return  status
 *
 ****************************************************************/
void fbe_test_pp_utils_set_default_520_sas_drive(fbe_sas_drive_type_t drive_type)
{
    MUT_ASSERT_TRUE_MSG(drive_type > FBE_SAS_DRIVE_INVALID, "Invalid drive type <= FBE_SAS_DRIVE_INVALID");
    MUT_ASSERT_TRUE_MSG(drive_type < FBE_SAS_DRIVE_LAST, "Invalid drive type >= FBE_SAS_DRIVE_LAST");

    fbe_test_pp_util_default_520_sas_drive_type = drive_type;
}

/*!**************************************************************
 * fbe_test_pp_utils_set_default_4160_sas_drive()
 ****************************************************************
 * @brief
 *  Set the default 4k sas drive
 *
 * @param   drive_type_p - Addresss of drive type to populate
 *
 * @note    Only works in simulation environment.
 * 
 * @return  status
 *
 ****************************************************************/
void fbe_test_pp_utils_set_default_4160_sas_drive(fbe_sas_drive_type_t drive_type)
{
    MUT_ASSERT_TRUE_MSG(drive_type > FBE_SAS_DRIVE_INVALID, "Invalid drive type <= FBE_SAS_DRIVE_INVALID");
    MUT_ASSERT_TRUE_MSG(drive_type < FBE_SAS_DRIVE_LAST, "Invalid drive type >= FBE_SAS_DRIVE_LAST");

    fbe_test_pp_util_default_4k_sas_drive_type = drive_type;
}

/*!**************************************************************
 * fbe_test_pp_utils_get_default_drive_type_based_on_block_size()
 ****************************************************************
 * @brief
 *  Determine the default drive type based on the required block size.
 *
 * @param   drive_block_size - The physical block size of the drive
 * @param   drive_type_p - Addresss of drive type to populate
 *
 * @note    Only works in simulation environment.
 * 
 * @return  status
 *
 ****************************************************************/
static fbe_status_t fbe_test_pp_utils_get_default_drive_type_based_on_block_size(fbe_block_size_t drive_block_size,
                                                                                 fbe_sas_drive_type_t *drive_type_p)
{
    /*! @note Currently these are the only block size supported.
     */
    *drive_type_p = FBE_SAS_DRIVE_INVALID;
    switch(drive_block_size)
    {
         case FBE_BE_BYTES_PER_BLOCK:
            *drive_type_p = fbe_test_pp_util_default_520_sas_drive_type;
            break;

        case FBE_4K_BYTES_PER_BLOCK:
            *drive_type_p = fbe_test_pp_util_default_4k_sas_drive_type;
            break;
        default:
            /* The requested block size is not supported.
             */
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "=== %s Requested block size: %d NOT supported !!!",
                       __FUNCTION__, drive_block_size);
            MUT_ASSERT_INT_EQUAL(FBE_BE_BYTES_PER_BLOCK, drive_block_size);
            return FBE_STATUS_GENERIC_FAILURE;
    }
    
    return FBE_STATUS_OK;
}
/*********************************************************************
 * end fbe_test_pp_utils_get_default_drive_type_based_on_block_size()
 ********************************************************************/

/*!**************************************************************
 * fbe_test_get_sas_drive_info()
 ****************************************************************
 * @brief
 *  This function generates a new SAS drive .
 *
 * @param sas_drive_p - sas drive
 * @param backend_number - bus or port.
 * @param encl_number - enclosure number on that port
 * @param slot_number - number of the drive on that enclosure.
 * @param block_size - Number of bytes per block.
 * @param capacity - Number of blocks on the drive.
 * 
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_test_get_sas_drive_info(fbe_terminator_sas_drive_info_t *sas_drive_p,
                                         fbe_u32_t backend_number,
                                         fbe_u32_t encl_number,
                                         fbe_u32_t slot_number,
                                         fbe_block_size_t block_size,
                                         fbe_lba_t capacity)
{
    fbe_status_t            status;
    fbe_sas_drive_type_t    drive_type;

    status = fbe_test_pp_utils_get_default_drive_type_based_on_block_size(block_size, &drive_type);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_test_get_sas_drive_info_extend(sas_drive_p,
                        backend_number,
                        encl_number,
                        slot_number,
                        block_size,
                        capacity,
                        GET_SAS_DRIVE_SAS_ADDRESS(backend_number, encl_number, slot_number),
                        drive_type);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_test_get_sas_drive_info()
 ******************************************/

/*!**************************************************************
 * fbe_test_get_new_sas_drive_info()
 ****************************************************************
 * @brief
 *  This function generates a new SAS drive .
 *
 * @param sas_drive_p - sas drive
 * @param backend_number - bus or port.
 * @param encl_number - enclosure number on that port
 * @param slot_number - number of the drive on that enclosure.
 * @param block_size - Number of bytes per block.
 * @param capacity - Number of blocks on the drive.
 * 
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_test_get_new_sas_drive_info(fbe_terminator_sas_drive_info_t *sas_drive_p,
                                             fbe_u32_t backend_number,
                                             fbe_u32_t encl_number,
                                             fbe_u32_t slot_number,
                                             fbe_block_size_t block_size,
                                             fbe_lba_t capacity)
{
    fbe_status_t            status;
    fbe_sas_drive_type_t    drive_type;
    /* We will generate a new sas address for this drive, since it is a new drive.
     */
    fbe_sas_address_t new_address = fbe_test_pp_util_get_unique_sas_address();

    status = fbe_test_pp_utils_get_default_drive_type_based_on_block_size(block_size, &drive_type);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_test_get_sas_drive_info_extend(sas_drive_p,
                                       backend_number,
                                       encl_number,
                                       slot_number,
                                       block_size,
                                       capacity,
                                       new_address,
                                       drive_type);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_test_get_new_sas_drive_info()
 ******************************************/

/*!**************************************************************
 * fbe_test_get_sata_drive_info()
 ****************************************************************
 * @brief
 *  This function generates a new SATA drive .
 *
 * @param sata_drive_p - sata drive
 * @param backend_number - bus or port.
 * @param encl_number - enclosure number on that port
 * @param slot_number - number of the drive on that enclosure.
 * @param block_size - Number of bytes per block.
 * @param capacity - Number of blocks on the drive.
 * 
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_test_get_sata_drive_info(fbe_terminator_sata_drive_info_t *sata_drive_p,
                                          fbe_u32_t backend_number,
                                          fbe_u32_t encl_number,
                                          fbe_u32_t slot_number,
                                          fbe_block_size_t block_size,
                                          fbe_lba_t capacity)
{
    sata_drive_p->backend_number = backend_number;
    sata_drive_p->encl_number = encl_number;
    sata_drive_p->sas_address = GET_SATA_DRIVE_SAS_ADDRESS(sata_drive_p->backend_number, sata_drive_p->encl_number, slot_number);
    sata_drive_p->drive_type = FBE_SATA_DRIVE_SIM_512;
    sata_drive_p->capacity = capacity;
    sata_drive_p->block_size = block_size;
    fbe_sprintf(sata_drive_p->drive_serial_number, sizeof(sata_drive_p->drive_serial_number),
                "%llX", (unsigned long long)sata_drive_p->sas_address);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_test_get_sata_drive_info()
 ******************************************/

/*!**************************************************************
 * fbe_test_pp_util_insert_unique_sas_drive()
 ****************************************************************
 * @brief
 *  This function generates a new SAS drive with a unique SAS
 *  address.
 *
 * @param backend_number - bus or port.
 * @param encl_number - enclosure number on that port
 * @param slot_number - number of the drive on that enclosure.
 * @param block_size - Number of bytes per block.
 * @param capacity - Number of blocks on the drive.
 * @param drive_handle_p - output handle.
 * 
 * @return fbe_status_t
 *
 * @author
 *  12/14/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_test_pp_util_insert_unique_sas_drive(fbe_u32_t backend_number,
                                                   fbe_u32_t encl_number,
                                                   fbe_u32_t slot_number,
                                                   fbe_block_size_t block_size,
                                                   fbe_lba_t capacity,
                                                   fbe_api_terminator_device_handle_t  *drive_handle_p)
{
    fbe_status_t status;
    fbe_terminator_sas_drive_info_t sas_drive;
    fbe_api_terminator_device_handle_t encl_handle;

    status = fbe_api_terminator_get_enclosure_handle(backend_number, encl_number, &encl_handle);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    if((encl_number == 0) && (slot_number < 4) && (capacity < TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY)){
        capacity = TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY + TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY;
        mut_printf(MUT_LOG_LOW, "=== fbe_test_pp_util Increased drive capacity! ===\n");
    }

    status = fbe_test_get_new_sas_drive_info(&sas_drive, 
                                             backend_number, encl_number, slot_number, 
                                             block_size, capacity);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }
    mut_printf(MUT_LOG_LOW, "=== %s %d_%d_%d serial: %s inserted. ===",
               __FUNCTION__, backend_number, encl_number, slot_number, sas_drive.drive_serial_number);
    status  = fbe_api_terminator_insert_sas_drive(encl_handle, slot_number, &sas_drive, drive_handle_p);
    return status;
}
/******************************************
 * end fbe_test_pp_util_insert_unique_sas_drive()
 ******************************************/

/*!**************************************************************
 * fbe_test_pp_util_get_unique_sas_drive_info()
 ****************************************************************
 * @brief
 *  This function generates a new SAS drive with a unique SAS
 *  address.
 *
 * @param backend_number - bus or port.
 * @param encl_number - enclosure number on that port
 * @param slot_number - number of the drive on that enclosure.
 * @param block_size - Number of bytes per block.
 * @param capacity - Number of blocks on the drive.
 * @param drive_handle_p - output handle.
 * 
 * @return fbe_status_t
 *
 * @author
 *  06/29/2012 - Created. Ashwin Tamilarasan
 *
 ****************************************************************/
fbe_status_t fbe_test_pp_util_get_unique_sas_drive_info(fbe_u32_t backend_number,
                                                   fbe_u32_t encl_number,
                                                   fbe_u32_t slot_number,
                                                   fbe_block_size_t block_size,
                                                   fbe_lba_t *capacity_p,
                                                   fbe_api_terminator_device_handle_t  *drive_handle_p,
                                                   fbe_terminator_sas_drive_info_t *sas_drive_p)
{
    fbe_status_t status;
    //fbe_terminator_sas_drive_info_t sas_drive;
    fbe_api_terminator_device_handle_t encl_handle;

    status = fbe_api_terminator_get_enclosure_handle(backend_number, encl_number, &encl_handle);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    if((encl_number == 0) && (slot_number < 4) && (*capacity_p < TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY)){
        *capacity_p = TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY + TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY;
        mut_printf(MUT_LOG_LOW, "=== fbe_test_pp_util Increased drive capacity! ===\n");
    }

    status = fbe_test_get_new_sas_drive_info(sas_drive_p, 
                                             backend_number, encl_number, slot_number, 
                                             block_size, *capacity_p);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }
    
    return status;
}
/******************************************
 * end fbe_test_pp_util_get_unique_sas_drive_info()
 ******************************************/

/*!**************************************************************
 * fbe_test_pp_util_insert_unique_sas_drive_for_dualsp()
 ****************************************************************
 * @brief
 *  This function generates a new SAS drive with a unique SAS
 *  address.
 *
 * @param backend_number - bus or port.
 * @param encl_number - enclosure number on that port
 * @param slot_number - number of the drive on that enclosure.
 * @param block_size - Number of bytes per block.
 * @param capacity - Number of blocks on the drive.
 * @param drive_handle_p - output handle.
 * 
 * @return fbe_status_t
 *
 * @author
 *  06/29/2012 - Created. Ashwin Tamilarasan
 *
 ****************************************************************/
fbe_status_t fbe_test_pp_util_insert_unique_sas_drive_for_dualsp(fbe_u32_t backend_number,
                                                   fbe_u32_t encl_number,
                                                   fbe_u32_t slot_number,
                                                   fbe_block_size_t block_size,
                                                   fbe_lba_t capacity,
                                                   fbe_api_terminator_device_handle_t  *drive_handle_p,
                                                   fbe_terminator_sas_drive_info_t sas_drive)
{
    fbe_status_t status;
    //fbe_terminator_sas_drive_info_t sas_drive;
    fbe_api_terminator_device_handle_t encl_handle;

    status = fbe_api_terminator_get_enclosure_handle(backend_number, encl_number, &encl_handle);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    mut_printf(MUT_LOG_LOW, "=== %s %d_%d_%d serial: %s inserted. ===",
               __FUNCTION__, backend_number, encl_number, slot_number, sas_drive.drive_serial_number);
    status  = fbe_api_terminator_insert_sas_drive(encl_handle, slot_number, &sas_drive, drive_handle_p);
    return status;
}
/******************************************
 * end fbe_test_pp_util_insert_unique_sas_drive_for_dualsp()
 ******************************************/

/*!**************************************************************
 * fbe_test_pp_util_insert_sas_drive()
 ****************************************************************
 * @brief
 *  This function generates a new SAS drive.
 *
 * @param backend_number - bus or port.
 * @param encl_number - enclosure number on that port
 * @param slot_number - number of the drive on that enclosure.
 * @param block_size - Number of bytes per block.
 * @param capacity - Number of blocks on the drive.
 * @param drive_handle_p - output handle.
 * 
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_test_pp_util_insert_sas_drive(fbe_u32_t backend_number,
                                               fbe_u32_t encl_number,
                                               fbe_u32_t slot_number,
                                               fbe_block_size_t block_size,
                                               fbe_lba_t capacity,
                                               fbe_api_terminator_device_handle_t  *drive_handle_p)
{
    fbe_status_t status;
    fbe_terminator_sas_drive_info_t sas_drive;
    fbe_api_terminator_device_handle_t encl_handle;
    fbe_term_encl_connector_list_t connector_ids;

    status = fbe_api_terminator_get_enclosure_handle(backend_number, encl_number, &encl_handle);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    if((encl_number == 0) && (slot_number < 4) && (capacity < TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY)){
        capacity = TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY + TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY;
        mut_printf(MUT_LOG_LOW, "=== fbe_test_pp_util Increased drive capacity! ===\n");
    }

    status = fbe_test_get_sas_drive_info(&sas_drive, 
                                         backend_number, encl_number, slot_number, 
                                         block_size, capacity);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    status = fbe_api_terminator_get_connector_id_list_for_enclosure (encl_handle, &connector_ids);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    mut_printf(MUT_LOG_LOW, "=== %s %d_%d_%d serial: %s inserted. blks:0x%llx sz:0x%x cap:%llu MB drive_type:0x%x ===",
               __FUNCTION__, backend_number, encl_number, slot_number, sas_drive.drive_serial_number, sas_drive.capacity, sas_drive.block_size,  
               (sas_drive.block_size*sas_drive.capacity/(1024*1024)), sas_drive.drive_type);

    if (connector_ids.num_connector_ids == 0) // If this is one we have a Viper, Derringer, etc.
    {
        status  = fbe_api_terminator_insert_sas_drive(encl_handle, slot_number, &sas_drive, drive_handle_p);
    }
    else // It must be a Voyager. We will need to handle Viking later.
    {
        status  = fbe_test_pp_util_internal_insert_voyager_sas_drive(encl_handle, slot_number, &sas_drive, drive_handle_p);
    }

    return status;
}
/******************************************
 * end fbe_test_pp_util_insert_sas_drive()
 ******************************************/

/*!**************************************************************
 * fbe_test_pp_util_insert_sas_drive_extend()
 ****************************************************************
 * @brief
 *  This function generates a new SAS drive with a unique SAS
 *  address.
 *
 * @param backend_number - bus or port.
 * @param encl_number - enclosure number on that port
 * @param slot_number - number of the drive on that enclosure.
 * @param drive_physical_block_size - Physical drive bytes per sector
 * @param required_capacity - Required drive capacity in FBE_BE_BYTES_PER_BLOCK blocks
 * @param sas_address - Sas address
 * @param drive_type - drive type
 * @param drive_handle_p - output handle.
 * 
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_test_pp_util_insert_sas_drive_extend(fbe_u32_t backend_number,
                                                      fbe_u32_t encl_number,
                                                      fbe_u32_t slot_number,
                                                      fbe_block_size_t drive_physical_block_size,
                                                      fbe_lba_t required_capacity,
                                                      fbe_sas_address_t sas_address,
                                                      fbe_sas_drive_type_t drive_type,
                                                      fbe_api_terminator_device_handle_t  *drive_handle_p)
{
    fbe_status_t status;
    fbe_terminator_sas_drive_info_t sas_drive;
    fbe_api_terminator_device_handle_t encl_handle;
    fbe_term_encl_connector_list_t connector_ids;

    status = fbe_api_terminator_get_enclosure_handle(backend_number, encl_number, &encl_handle);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    status = fbe_test_get_sas_drive_info_extend(&sas_drive,
                                                backend_number, encl_number, slot_number,
                                                drive_physical_block_size, 
                                                required_capacity, 
                                                sas_address, 
                                                drive_type);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    mut_printf(MUT_LOG_LOW, "=== _insert_sas_drive_extend: %d_%d_%d serial: %s inserted. cap: 0x%llx. type:0x%x ===",
               backend_number, encl_number, slot_number, sas_drive.drive_serial_number, sas_drive.capacity, sas_drive.drive_type);

    status = fbe_api_terminator_get_connector_id_list_for_enclosure (encl_handle, &connector_ids);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }
    mut_printf(MUT_LOG_LOW, "=== _insert_sas_drive_extend: %d_%d_%d num_connector_ids: 0x%x ===",
              backend_number, encl_number, slot_number, connector_ids.num_connector_ids);

    if (connector_ids.num_connector_ids == 0) // If this is one we have a Viper, Derringer, etc.
    {
        status  = fbe_api_terminator_insert_sas_drive(encl_handle, slot_number, &sas_drive, drive_handle_p);
    }
    else // It must be a Voyager. We will need to handle Viking later.
    {
        status  = fbe_test_pp_util_internal_insert_voyager_sas_drive(encl_handle, slot_number, &sas_drive, drive_handle_p);
    }

    return status;
}
/******************************************
 * end fbe_test_pp_util_insert_sas_drive_extend()
 ******************************************/

/*!**************************************************************
 * fbe_test_pp_util_insert_sata_drive()
 ****************************************************************
 * @brief
 *  This function generates a new SATA drive.
 *
 * @param backend_number - bus or port.
 * @param encl_number - enclosure number on that port
 * @param slot_number - number of the drive on that enclosure.
 * @param block_size - Number of bytes per block.
 * @param capacity - Number of blocks on the drive.
 * @param drive_handle_p - output handle.
 * 
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_test_pp_util_insert_sata_drive(fbe_u32_t backend_number,
                                                fbe_u32_t encl_number,
                                                fbe_u32_t slot_number,
                                                fbe_block_size_t block_size,
                                                fbe_lba_t capacity,
                                                fbe_api_terminator_device_handle_t  *drive_handle_p)
{
    fbe_status_t                        status;
    static fbe_bool_t                   b_are_native_sata_drives_supported = FBE_FALSE;
    fbe_terminator_sata_drive_info_t    sata_drive;
    fbe_api_terminator_device_handle_t  encl_handle;

    status = fbe_api_terminator_get_enclosure_handle(backend_number, encl_number, &encl_handle);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    /* validate if native drive are no longer supported */
    if ((block_size == 512)                               &&
        (b_are_native_sata_drives_supported == FBE_FALSE)    )
    {
        /* The pdo code should reject this request*/
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, "=== %s %d_%d_%d block size: %d native SATA not supported ===",
               __FUNCTION__, backend_number, encl_number, slot_number, block_size);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    status = fbe_test_get_sata_drive_info(&sata_drive, 
                                          backend_number, encl_number, slot_number, 
                                          block_size, capacity);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }
    status  = fbe_api_terminator_insert_sata_drive(encl_handle, slot_number, &sata_drive, drive_handle_p);

    return status;
}
/******************************************
 * end fbe_test_pp_util_insert_sata_drive()
 ******************************************/

/*!**************************************************************
 * fbe_test_pp_util_insert_sas_drive_with_info()
 ****************************************************************
 * @brief
 *  This function inserts SAS drive with drive info.
 *
 * @param drive_info_p - input drive info
 * @param drive_handle_p - output handle.
 * 
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_test_pp_util_insert_sas_drive_with_info(fbe_terminator_sas_drive_info_t *drive_info_p,
                                                         fbe_api_terminator_device_handle_t  *drive_handle_p)
{
    fbe_status_t status;
    fbe_api_terminator_device_handle_t encl_handle;

    status = fbe_api_terminator_get_enclosure_handle(drive_info_p->backend_number, drive_info_p->encl_number, &encl_handle);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }
    mut_printf(MUT_LOG_LOW, "=== %s %d_%d_%d serial: %s inserted. ===",
               __FUNCTION__, drive_info_p->backend_number, drive_info_p->encl_number, drive_info_p->slot_number, 
               drive_info_p->drive_serial_number);
    status  = fbe_api_terminator_insert_sas_drive(encl_handle, drive_info_p->slot_number, drive_info_p, drive_handle_p);
    return status;
}
/******************************************
 * end fbe_test_pp_util_insert_sas_drive_with_info()
 ******************************************/

/*!**************************************************************
 * fbe_test_pp_util_insert_sas_enclosure()
 ****************************************************************
 * @brief
 *  This function inserts SAS enclosure.
 *
 * @param backend_number - bus or port.
 * @param encl_number - enclosure number on that port
 * @param uid - UID.
 * @param sas_address - SAS address.
 * @param capacity - Number of blocks on the drive.
 * @param encl_type - enclosure type
 * @param port_handle - port handle
 * @param enclosure_handle_p - output handle.
 * 
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_test_pp_util_insert_sas_enclosure(fbe_u32_t backend_number,
                                                     fbe_u32_t encl_number,
                                                     fbe_u8_t *uid,
                                                     fbe_sas_address_t sas_address,
                                                     fbe_sas_enclosure_type_t encl_type,
                                                     fbe_api_terminator_device_handle_t  port_handle,
                                                     fbe_api_terminator_device_handle_t  *enclosure_handle_p)
{
    fbe_status_t status;
    fbe_terminator_sas_encl_info_t  sas_encl;

    /* insert an enclosure.
     */
    sas_encl.backend_number = backend_number;
    sas_encl.encl_number = encl_number;
    fbe_copy_memory(sas_encl.uid, uid, sizeof(sas_encl.uid));
    sas_encl.sas_address = sas_address;
    sas_encl.encl_type = encl_type;
    status  = fbe_api_terminator_insert_sas_enclosure(port_handle, &sas_encl, enclosure_handle_p);
    return status;
}
/*************************************************************
 * end of fbe_test_pp_util_insert_sas_enclosure()
 *************************************************************/

/*!**************************************************************
 * fbe_test_pp_util_insert_viper_enclosure()
 ****************************************************************
 * @brief
 *  This function inserts viper enclosure.
 *
 * @param backend_number - bus or port.
 * @param encl_number - enclosure number on that port
 * @param port_handle - port handle
 * @param enclosure_handle_p - output handle.
 * 
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_test_pp_util_insert_viper_enclosure(fbe_u32_t backend_number,
                                                     fbe_u32_t encl_number,
                                                     fbe_api_terminator_device_handle_t  port_handle,
                                                     fbe_api_terminator_device_handle_t  *enclosure_handle_p)
{
    fbe_u8_t uid[TERMINATOR_ESES_ENCLOSURE_UNIQUE_ID_SIZE] = {encl_number};

    /* insert an enclosure.
     */
    return fbe_test_pp_util_insert_sas_enclosure(backend_number,
                                              encl_number,
                                              uid,
                                              GET_SAS_ENCLOSURE_SAS_ADDRESS(backend_number, encl_number),
                                              FBE_SAS_ENCLOSURE_TYPE_VIPER,
                                              port_handle,
                                              enclosure_handle_p);
}
/*************************************************************
 * end of fbe_test_pp_util_insert_viper_enclosure()
 *************************************************************/

fbe_status_t fbe_test_pp_util_insert_dpe_enclosure(fbe_u32_t backend_number,
                                                   fbe_u32_t encl_number,
                                                   fbe_sas_enclosure_type_t encl_type,
                                                   fbe_api_terminator_device_handle_t  port_handle,
                                                   fbe_api_terminator_device_handle_t  *enclosure_handle_p)
{
    fbe_u8_t uid[FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE] = {encl_number};

    /* insert an enclosure.
     */
    return fbe_test_pp_util_insert_sas_enclosure(backend_number,
                                              encl_number,
                                              uid,
                                              GET_SAS_ENCLOSURE_SAS_ADDRESS(backend_number, encl_number),
                                              encl_type,
                                              port_handle,
                                              enclosure_handle_p);
}

/*!**************************************************************
 * fbe_test_pp_util_insert_sas_port()
 ****************************************************************
 * @brief
 *  This function inserts SAS port.
 *
 * @param io_port_number - io port number.
 * @param portal_number - portal number
 * @param backend_number - back end number
 * @param sas_address - SAS address
 * @param port_type - port type 
 * @param port_handle_p - output handle.
 * 
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_test_pp_util_insert_sas_port(fbe_u32_t io_port_number,
                                  fbe_u32_t portal_number,
                                  fbe_u32_t backend_number,
                                  fbe_sas_address_t sas_address,
                                  fbe_port_type_t port_type,
                                  fbe_api_terminator_device_handle_t  *port_handle_p)
{
    fbe_status_t status;
    fbe_terminator_sas_port_info_t  sas_port;

    sas_port.sas_address = sas_address;
    sas_port.port_type = port_type;
    sas_port.io_port_number = io_port_number;
    sas_port.portal_number = portal_number;
    sas_port.backend_number = backend_number;

    status  = fbe_api_terminator_insert_sas_port(&sas_port, port_handle_p);
    return status;
}
/*************************************************************
 * end of fbe_test_pp_util_insert_sas_port()
 *************************************************************/

/*!**************************************************************
 * fbe_test_pp_util_insert_sas_pmc_port()
 ****************************************************************
 * @brief
 *  This function inserts SAS PMC port.
 *
 * @param io_port_number - io port number.
 * @param portal_number - portal number
 * @param backend_number - back end number
 * @param port_handle_p - output handle.
 * 
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_test_pp_util_insert_sas_pmc_port(fbe_u32_t io_port_number,
                                  fbe_u32_t portal_number,
                                  fbe_u32_t backend_number,
                                  fbe_api_terminator_device_handle_t  *port_handle_p)
{
    return fbe_test_pp_util_insert_sas_port(io_port_number,
                                portal_number,
                                backend_number,
                                GET_SAS_PMC_PORT_SAS_ADDRESS(backend_number),
                                FBE_PORT_TYPE_SAS_PMC,
                                port_handle_p);
}
/*************************************************************
 * end of fbe_test_pp_util_insert_sas_pmc_port()
 *************************************************************/

/*!**************************************************************
 * fbe_test_pp_util_insert_armada_board()
 ****************************************************************
 * @brief
 *  This function inserts armada board.
 *
 * @param none
 * 
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_test_pp_util_insert_armada_board(void)
{
    fbe_status_t                status;
    fbe_terminator_board_info_t board_info;
    char                       *value_string;
    fbe_u32_t                   value;
    fbe_bool_t                  b_is_term_completion_dpc = FBE_FALSE;

    /* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
    board_info.platform_type = SPID_DEFIANT_M4_HW_TYPE;
    status = fbe_private_space_layout_initialize_library(board_info.platform_type);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    value_string = mut_get_user_option_value("-term_set_completion_dpc");
    if (value_string != NULL) 
    {
        value = strtoul(value_string, 0, 0);
        b_is_term_completion_dpc = (value == 0) ? FBE_FALSE : FBE_TRUE;

        /* Send the usurper to set the terminator complete I/O at DPC 
         *  o FBE_TRUE - Terminator will complete I/Os at DPC level
         *  o FBE_FALSE - Terminator will complete I/Os at thread level
         */
        fbe_api_terminator_set_completion_dpc(b_is_term_completion_dpc);
    }

    /* Now insert the board*/
    status  = fbe_api_terminator_insert_board(&board_info);
    return status;
}
/*************************************************************
 * end of fbe_test_pp_util_insert_armada_board()
 *************************************************************/

/*!**************************************************************
 * fbe_test_pp_util_insert_board_of_type()
 ****************************************************************
 * @brief
 *  This function inserts caller specified board.
 *
 * @param platform_type - platform type, which is used to assign
 *                        appropriate  board.
 * 
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_test_pp_util_insert_board_of_type(SPID_HW_TYPE platform_type)
{
    fbe_status_t                status;
    fbe_terminator_board_info_t board_info;
    char                       *value_string;
    fbe_u32_t                   value;
    fbe_bool_t                  b_is_term_completion_dpc = FBE_FALSE;

    /* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
    board_info.platform_type = platform_type;
    status = fbe_private_space_layout_initialize_library(board_info.platform_type);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    value_string = mut_get_user_option_value("-term_set_completion_dpc");
    if (value_string != NULL) 
    {
        value = strtoul(value_string, 0, 0);
        b_is_term_completion_dpc = (value == 0) ? FBE_FALSE : FBE_TRUE;

        /* Send the usurper to set the terminator complete I/O at DPC 
         *  o FBE_TRUE - Terminator will complete I/Os at DPC level
         *  o FBE_FALSE - Terminator will complete I/Os at thread level
         */
        fbe_api_terminator_set_completion_dpc(b_is_term_completion_dpc);
    }

    /* Now insert the board*/
    status  = fbe_api_terminator_insert_board(&board_info);
    return status;
}
/*************************************************************
 * end of fbe_test_pp_util_insert_board_of_type()
 *************************************************************/


/*!**************************************************************
 * fbe_test_pp_util_insert_board()
 ****************************************************************
 * @brief
 *  This function inserts SAS board.
 *
 * @param none
 * 
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_test_pp_util_insert_board(void)
{
    fbe_status_t                status;
    fbe_terminator_board_info_t board_info;
    char                       *value_string;
    fbe_u32_t                   value;
    fbe_bool_t                  b_is_term_completion_dpc = FBE_FALSE;

    /* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
    board_info.platform_type = SPID_DEFIANT_M4_HW_TYPE;

    if (fbe_test_should_use_sim_psl()) {
        fbe_private_space_layout_initialize_fbe_sim();
    } else {
        status = fbe_private_space_layout_initialize_library(board_info.platform_type);
        if(status != FBE_STATUS_OK)
        {
            return status;
        }
    }

    value_string = mut_get_user_option_value("-term_set_completion_dpc");
    if (value_string != NULL) 
    {
        value = strtoul(value_string, 0, 0);
        b_is_term_completion_dpc = (value == 0) ? FBE_FALSE : FBE_TRUE;

        /* Send the usurper to set the terminator complete I/O at DPC 
         *  o FBE_TRUE - Terminator will complete I/Os at DPC level
         *  o FBE_FALSE - Terminator will complete I/Os at thread level
         */
        fbe_api_terminator_set_completion_dpc(b_is_term_completion_dpc);
    }

    /* Now insert the board*/
    status  = fbe_api_terminator_insert_board(&board_info);
    return status;
}
/*************************************************************
 * end of fbe_test_pp_util_insert_board()
 *************************************************************/

/*!**************************************************************
 * fbe_test_pp_util_remove_sas_drive()
 ****************************************************************
 * @brief
 *  This function removes SAS drive.
 *
 * @param port_number - port number
 * @param enclosure_number - enclosure number
 * @param slot_number - slot number
 * 
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_test_pp_util_remove_sas_drive(fbe_u32_t port_number, 
                                               fbe_u32_t enclosure_number, 
                                               fbe_u32_t slot_number)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_api_terminator_device_handle_t  drive_handle = 0;
    status = fbe_api_terminator_get_drive_handle(port_number,
                                                 enclosure_number,
                                                 slot_number,
                                                 &drive_handle);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    /* Remove and logout the drive using terminator by changing insert bit.
     */
    status = fbe_api_terminator_remove_sas_drive(drive_handle);

    return status;
}
/*************************************************************
 * end of fbe_test_pp_util_remove_sas_drive()
 *************************************************************/

/*!**************************************************************
 * fbe_test_pp_util_remove_sata_drive()
 ****************************************************************
 * @brief
 *  This function removes SATA drive.
 *
 * @param port_number - port number
 * @param enclosure_number - enclosure number
 * @param slot_number - slot number
 * 
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_test_pp_util_remove_sata_drive(fbe_u32_t port_number, 
                                                fbe_u32_t enclosure_number, 
                                                fbe_u32_t slot_number)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_api_terminator_device_handle_t  drive_handle = 0;
    status = fbe_api_terminator_get_drive_handle(port_number,
                                                 enclosure_number,
                                                 slot_number,
                                                 &drive_handle);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    /* Remove and logout the drive using terminator by changing insert bit.
     */
    status = fbe_api_terminator_remove_sata_drive(drive_handle);

    return status;
}
/*************************************************************
 * end of fbe_test_pp_util_remove_sata_drive()
 *************************************************************/

/*!**************************************************************
 * fbe_test_pp_util_verify_pdo_state()
 ****************************************************************
 * @brief
 *  This function verifies pdo state.
 *
 * @param port_number - port number
 * @param enclosure_number - enclosure number
 * @param slot_number - slot number
 * @param expected_state - expected state
 * @param timeout_ms - timeout in ms
 * 
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_test_pp_util_verify_pdo_state(fbe_u32_t port_number,
                                               fbe_u32_t enclosure_number,
                                               fbe_u32_t slot_number,
                                               fbe_lifecycle_state_t expected_state,
                                               fbe_u32_t timeout_ms)
{
    fbe_object_id_t object_id;
    fbe_status_t status;
    fbe_u32_t elapsed_time = 0;

    do
    {
        /* Get the physical drive object id by location of the drive.
         */
        status = fbe_api_get_physical_drive_object_id_by_location(port_number, enclosure_number, slot_number, &object_id);    
        if (status != FBE_STATUS_OK) {
            fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: fbe_api_get_physical_drive_object_id_by_location Failed for %d_%d_%d\n", __FUNCTION__,
                                                port_number, enclosure_number, slot_number);

            return status;
        }

        /* Object id must be valid with all other lifecycle state.
         */
        if (object_id == FBE_OBJECT_ID_INVALID) 
        {
            if (expected_state == FBE_LIFECYCLE_STATE_DESTROY)
            {
                /* It does not exist, this was expected.
                 */
                return FBE_STATUS_OK;
            }
            else
            {
                /* Wait for the object to come into existance.
                 */
                fbe_api_sleep(500);
                elapsed_time += 500;
            }
        }
    } while ((object_id == FBE_OBJECT_ID_INVALID) && (elapsed_time < timeout_ms));

    /* Verify whether physical drive object is in expected State, it waits
     * for the few seconds if it PDO is not in expected state.
     */
    status = fbe_api_wait_for_object_lifecycle_state(object_id, 
                                                     expected_state, 
                                                     timeout_ms,
                                                     FBE_PACKAGE_ID_PHYSICAL);

    return status;
}
/*************************************************************
 * end of fbe_test_pp_util_verify_pdo_state()
 *************************************************************/


/*!**************************************************************
 * fbe_test_pp_util_pull_drive()
 ****************************************************************
 * @brief
 *  This function pull drive. 
 *
 * @param port_number - port number
 * @param enclosure_number - enclosure number
 * @param slot_number - slot number
 * @param drive_handle_p - drive handle (output)
 * 
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_test_pp_util_pull_drive(fbe_u32_t port_number, 
                                         fbe_u32_t enclosure_number, 
                                         fbe_u32_t slot_number,
                                         fbe_api_terminator_device_handle_t *drive_handle_p)
{
    fbe_status_t status = FBE_STATUS_OK;
        
    status = fbe_api_terminator_get_drive_handle(port_number,
                                                 enclosure_number,
                                                 slot_number,
                                                 drive_handle_p);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    /* Logout the drive using terminator by changing insert bit
     * (but leave the data intact)
     */
    status = fbe_api_terminator_pull_drive(*drive_handle_p);

    return status;
}
/*************************************************************
 * end of fbe_test_pp_util_pull_drive()
 *************************************************************/

/*!**************************************************************
 * fbe_test_pp_util_reinsert_drive()
 ****************************************************************
 * @brief
 *  This function inserts drive.
 *
 * @param port_number - port number
 * @param enclosure_number - enclosure number
 * @param slot_number - slot number
 * @param drive_handle - drive handle
 * 
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_test_pp_util_reinsert_drive(fbe_u32_t port_number,
                                             fbe_u32_t encl_number,
                                             fbe_u32_t slot_number,
                                             fbe_api_terminator_device_handle_t drive_handle)
{
    fbe_status_t status;
    fbe_api_terminator_device_handle_t encl_handle;

    /* Get the enclosure handle.
     */
    status = fbe_api_terminator_get_enclosure_handle(port_number, 
                                                     encl_number, 
                                                     &encl_handle);
    if (status != FBE_STATUS_OK) { return status; }

    /* Reinsert the drive preserving the data that was there.
     */
    status = fbe_api_terminator_reinsert_drive(encl_handle, 
                                               slot_number,
                                               drive_handle);
    return status;
}
/*************************************************************
 * end of fbe_test_pp_util_reinsert_drive()
 *************************************************************/

/* the following is the utility functions for Object Lifecycle State Notification */
static void fbe_test_pp_util_lifecycle_state_notification_callback_function (update_object_msg_t * update_object_msg, void * context);

/*!****************************************************************************
 * fbe_test_pp_util_register_lifecycle_state_notification
 ******************************************************************************
 *
 * @brief
 *    This function intializes the notification semaphore in the ns_context and register the notification. 
 *
 * @param   ns_context  -  notification context
 *
 * @return   None
 *
 * @version
 *    04/1/2010 - Created. guov
 *
 ******************************************************************************/
void fbe_test_pp_util_register_lifecycle_state_notification (fbe_test_pp_util_lifecycle_state_ns_context_t *ns_context)
{
    fbe_status_t                        status;

    fbe_semaphore_init(&(ns_context->sem), 0, 1);
    fbe_spinlock_init(&(ns_context->lock));
    status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_ALL,
                                                                  FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL,
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_ALL,
                                                                  fbe_test_pp_util_lifecycle_state_notification_callback_function,
                                                                  ns_context,
                                                                  &reg_id);
    MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK); 
    mut_printf(MUT_LOG_LOW, "=== fbe_test_pp_util register Lifecycle State notification successfully! ===\n");
    return;
}
/*************************************************************
 * end of fbe_test_pp_util_register_lifecycle_state_notification()
 *************************************************************/

/*!****************************************************************************
 * fbe_test_pp_util_unregister_lifecycle_state_notification
 ******************************************************************************
 *
 * @brief
 *    This function unregister the notification and destroy the semaphore in the ns_context.
 *
 * @param   ns_context  -  notification context
 *
 * @return   None
 *
 * @version
 *    04/1/2010 - Created. guov
 *
 ******************************************************************************/
void fbe_test_pp_util_unregister_lifecycle_state_notification (fbe_test_pp_util_lifecycle_state_ns_context_t *ns_context)
{
    fbe_status_t                        status;

    status = fbe_api_notification_interface_unregister_notification(fbe_test_pp_util_lifecycle_state_notification_callback_function, reg_id);
    MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK); 
    mut_printf(MUT_LOG_LOW, "=== fbe_test_pp_util unregister Lifecycle State notification successfully! ===\n");
    fbe_spinlock_destroy(&(ns_context->lock));
    fbe_semaphore_destroy(&(ns_context->sem));
    return;
}
/*************************************************************
 * end of fbe_test_pp_util_unregister_lifecycle_state_notification()
 *************************************************************/

/*!****************************************************************************
 * fbe_test_pp_util_wait_lifecycle_state_notification
 ******************************************************************************
 *
 * @brief
 *    This function waits and checkes for timeout.
 *
 * @param   ns_context  -  notification context
 *
 * @return   None
 *
 * @version
 *    03/15/2010 - Created.
 *
 ******************************************************************************/
void fbe_test_pp_util_wait_lifecycle_state_notification (fbe_test_pp_util_lifecycle_state_ns_context_t *ns_context)
{
    fbe_status_t       nt_status;
    ns_context->state_match = FBE_FALSE;

    nt_status = fbe_semaphore_wait_ms(&(ns_context->sem), ns_context->timeout_in_ms);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, nt_status, "Wait for lifecycle state notification timed out!");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, ns_context->state_match, "Lifecycle state did not match!");
    return;
}
/*************************************************************
 * end of fbe_test_pp_util_wait_lifecycle_state_notification()
 *************************************************************/

/*!**************************************************************
 * fbe_test_pp_util_lifecycle_state_notification_callback_function()
 ****************************************************************
 * @brief
 *  The the callback function, where the sem is release when a match found.
 *  The test that waits for this notification will be blocked until 
 *  either a match found or timeout reaches.
 *
 * @param update_object_msg  - message fron the notification.   
 * @param context  - notification context.
 *
 * @return None.
 *
 ****************************************************************/
static void fbe_test_pp_util_lifecycle_state_notification_callback_function (update_object_msg_t * update_object_msg, void * context)
{
    fbe_test_pp_util_lifecycle_state_ns_context_t * ns_context_p = (fbe_test_pp_util_lifecycle_state_ns_context_t * )context;
    fbe_status_t status;
    fbe_lifecycle_state_t state;

    if ((update_object_msg->notification_info.notification_type & FBE_NOTIFICATION_TYPE_LIFECYCLE_ANY_STATE_CHANGE)
        && (update_object_msg->notification_info.object_type == ns_context_p->object_type))
    {
        status = fbe_notification_convert_notification_type_to_state(update_object_msg->notification_info.notification_type,
                                                                     &state);

        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        /* Lock the notifocation context, so only one notification can update it at a time*/
        fbe_spinlock_lock(&(ns_context_p->lock));
        if (ns_context_p->expected_lifecycle_state == state)
        {
            ns_context_p->state_match = FBE_TRUE;
            /* Release the semaphore */
            mut_printf(MUT_LOG_LOW, "%s: received a matching notification from object type %llu, lifecycle state is %d.\n",
                       __FUNCTION__, 
                       (unsigned long long)update_object_msg->notification_info.object_type,
                       state);
            fbe_semaphore_release(&(ns_context_p->sem), 0, 1, FALSE);
        }
        fbe_spinlock_unlock(&(ns_context_p->lock));
    }
    return;
}
/*************************************************************
 * end of fbe_test_pp_util_lifecycle_state_notification_callback_function()
 *************************************************************/

/*!**************************************************************
 * fbe_test_pp_util_create_physical_config_for_disk_counts()
 ****************************************************************
 * @brief
 *  Configure the test's physical configuration based on the
 *  raid groups we intend to create.
 *
 * @param num_520_drives - output number of 520 drives
 * @param num_4160_drives - output number of 4160 drives
 * @param drive_capacity - The capacity (in 520-bps blocks) of the drive
 *
 * @return None.
 *
 * @author
 *  8/16/2010 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_pp_util_create_physical_config_for_disk_counts(fbe_u32_t num_520_drives,
                                                             fbe_u32_t num_4160_drives,
                                                             fbe_block_count_t drive_capacity)
{
    fbe_status_t                        status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                           total_objects = 0;
    fbe_class_id_t                      class_id;
    fbe_api_terminator_device_handle_t  port_handle;
    fbe_api_terminator_device_handle_t  encl_handle;
    fbe_api_terminator_device_handle_t  drive_handle;
    fbe_u32_t slot;
    fbe_u32_t num_objects = 1; /* start with board */
    fbe_u32_t encl_number = 0;
    fbe_u32_t encl_index;
    fbe_u32_t num_520_enclosures;
    fbe_u32_t num_4160_enclosures;
    fbe_u32_t num_first_enclosure_drives;

    num_520_enclosures = (num_520_drives / FBE_TEST_DRIVES_PER_ENCL) + ((num_520_drives % FBE_TEST_DRIVES_PER_ENCL) ? 1 : 0);
    num_4160_enclosures = (num_4160_drives / FBE_TEST_DRIVES_PER_ENCL) + ((num_4160_drives % FBE_TEST_DRIVES_PER_ENCL) ? 1 : 0);

    /* Before loading the physical package we initialize the terminator.
     */
    status = fbe_api_terminator_init();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* insert a board
     */
    status = fbe_test_pp_util_insert_board();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    num_first_enclosure_drives = fbe_private_space_layout_get_number_of_system_drives();

    /* insert a port
     */
    fbe_test_pp_util_insert_sas_pmc_port(1, /* io port */
                                 2, /* portal */
                                 0, /* backend number */ 
                                 &port_handle);

    /* We start off with:
     *  1 board
     *  1 port
     *  1 enclosure
     *  plus one pdo for each of the first 10 drives.
     */
    num_objects = 3 +  num_first_enclosure_drives; 

    /* Next, add on all the enclosures and drives we will add.
     */
    num_objects += num_520_enclosures + num_520_drives;
    num_objects += num_4160_enclosures + num_4160_drives;

    /* First enclosure has 4 drives for system rgs.
     */
    fbe_test_pp_util_insert_viper_enclosure(0, encl_number, port_handle, &encl_handle);
    for (slot = 0; slot < num_first_enclosure_drives; slot++)
    {
        fbe_test_pp_util_insert_sas_drive(0, encl_number, slot, 520, TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY + drive_capacity, &drive_handle);
    }

    /* We inserted one enclosure above, so we are starting with encl #1.
     */
    encl_number = 1;

    /* Insert enclosures and drives for 520.  Start at encl number one.
     */
    for ( encl_index = 0; encl_index < num_520_enclosures; encl_index++)
    {
        fbe_test_pp_util_insert_viper_enclosure(0, encl_number, port_handle, &encl_handle);

        slot = 0;
        while ((slot < FBE_TEST_DRIVES_PER_ENCL) && num_520_drives)
        {
            fbe_test_pp_util_insert_sas_drive(0, encl_number, slot, 520, drive_capacity, &drive_handle);
            num_520_drives--;
            slot++;
        }
        encl_number++;
    }
    /* Insert enclosures and drives for 512.
     * We pick up the enclosure number from the prior loop. 
     */
    for ( encl_index = 0; encl_index < num_4160_enclosures; encl_index++)
    {
        fbe_test_pp_util_insert_viper_enclosure(0, encl_number, port_handle, &encl_handle);
        slot = 0;
        while ((slot < FBE_TEST_DRIVES_PER_ENCL) && num_4160_drives)
        {
            fbe_test_pp_util_insert_sas_drive(0, encl_number, slot, 4160, drive_capacity, &drive_handle);
            num_4160_drives--;
            slot++;
        }
        encl_number++;
    }

    mut_printf(MUT_LOG_LOW, "=== starting physical package===");
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== set Physical Package entries ===");
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Init fbe api on server */
    mut_printf(MUT_LOG_LOW, "=== starting Server FBE_API ===");
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait for the expected number of objects */
    status = fbe_api_wait_for_number_of_objects(num_objects, 60000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    mut_printf(MUT_LOG_LOW, "=== verifying configuration ===");

    /* We inserted a armada board so we check for it.
     * board is always assumed to be object id 0.
     */
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_LOW, "Verifying board type....Passed");

    /* Make sure we have the expected number of objects.
     */
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(total_objects == num_objects);
    mut_printf(MUT_LOG_LOW, "Verifying object count...Passed");

    if (fbe_test_should_use_sim_psl()) {
        mut_printf(MUT_LOG_TEST_STATUS, "Use FBE Simulator PSL.");
        fbe_api_board_sim_set_psl(0);
    }

    return;
}
/******************************************
 * end fbe_test_pp_util_create_physical_config_for_disk_counts()
 ******************************************/
/*!**************************************************************
 * fbe_test_pp_util_create_config_vary_capacity()
 ****************************************************************
 * @brief
 *  Configure the test's physical configuration based on the
 *  raid groups we intend to create.
 *
 * @param num_520_drives - output number of 520 drives
 * @param num_4160_drives - output number of 4160 drives
 * @param drive_capacity - The capacity (in 520-bps blocks) of the drive
 * @param extra - Blocks of extra for every other drive.
 * @return None.
 *
 * @author
 *  7/3/2014 - Created. Rob Foley
 ****************************************************************/

void fbe_test_pp_util_create_config_vary_capacity(fbe_u32_t num_520_drives,
                                                  fbe_u32_t num_4160_drives,
                                                  fbe_block_count_t drive_capacity,
                                                  fbe_block_count_t extra_blocks)
{
    fbe_status_t                        status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                           total_objects = 0;
    fbe_class_id_t                      class_id;
    fbe_api_terminator_device_handle_t  port_handle;
    fbe_api_terminator_device_handle_t  encl_handle;
    fbe_api_terminator_device_handle_t  drive_handle;
    fbe_u32_t slot;
    fbe_u32_t num_objects = 1; /* start with board */
    fbe_u32_t encl_number = 0;
    fbe_u32_t encl_index;
    fbe_u32_t num_520_enclosures;
    fbe_u32_t num_4160_enclosures;
    fbe_u32_t num_first_enclosure_drives;

    num_520_enclosures = (num_520_drives / FBE_TEST_DRIVES_PER_ENCL) + ((num_520_drives % FBE_TEST_DRIVES_PER_ENCL) ? 1 : 0);
    num_4160_enclosures = (num_4160_drives / FBE_TEST_DRIVES_PER_ENCL) + ((num_4160_drives % FBE_TEST_DRIVES_PER_ENCL) ? 1 : 0);

    /* Before loading the physical package we initialize the terminator.
     */
    status = fbe_api_terminator_init();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* insert a board
     */
    status = fbe_test_pp_util_insert_board();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    num_first_enclosure_drives = fbe_private_space_layout_get_number_of_system_drives();

    /* insert a port
     */
    fbe_test_pp_util_insert_sas_pmc_port(1, /* io port */
                                 2, /* portal */
                                 0, /* backend number */ 
                                 &port_handle);

    /* We start off with:
     *  1 board
     *  1 port
     *  1 enclosure
     *  plus one pdo for each of the first 10 drives.
     */
    num_objects = 3 +  num_first_enclosure_drives; 

    /* Next, add on all the enclosures and drives we will add.
     */
    num_objects += num_520_enclosures + num_520_drives;
    num_objects += num_4160_enclosures + num_4160_drives;

    /* First enclosure has 4 drives for system rgs.
     */
    fbe_test_pp_util_insert_viper_enclosure(0, encl_number, port_handle, &encl_handle);
    for (slot = 0; slot < num_first_enclosure_drives; slot++)
    {
        if ((slot % 2) == 0) {
            fbe_test_pp_util_insert_sas_drive(0, encl_number, slot, 520, TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY + drive_capacity, &drive_handle);
        } else {
            fbe_test_pp_util_insert_sas_drive(0, encl_number, slot, 520, 
                                              TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY + drive_capacity + extra_blocks, 
                                              &drive_handle);
        }
    }

    /* We inserted one enclosure above, so we are starting with encl #1.
     */
    encl_number = 1;

    /* Insert enclosures and drives for 520.  Start at encl number one.
     */
    for ( encl_index = 0; encl_index < num_520_enclosures; encl_index++)
    {
        fbe_test_pp_util_insert_viper_enclosure(0, encl_number, port_handle, &encl_handle);

        slot = 0;
        while ((slot < FBE_TEST_DRIVES_PER_ENCL) && num_520_drives)
        {
            if ((slot % 2) == 0) {
                fbe_test_pp_util_insert_sas_drive(0, encl_number, slot, 520, drive_capacity, &drive_handle);
            } else {
                fbe_test_pp_util_insert_sas_drive(0, encl_number, slot, 520, drive_capacity + extra_blocks, &drive_handle);
            }
            num_520_drives--;
            slot++;
        }
        encl_number++;
    }
    /* Insert enclosures and drives for 512.
     * We pick up the enclosure number from the prior loop. 
     */
    for ( encl_index = 0; encl_index < num_4160_enclosures; encl_index++)
    {
        fbe_test_pp_util_insert_viper_enclosure(0, encl_number, port_handle, &encl_handle);
        slot = 0;
        while ((slot < FBE_TEST_DRIVES_PER_ENCL) && num_4160_drives)
        {
            if ((slot % 2) == 0) {
                fbe_test_pp_util_insert_sas_drive(0, encl_number, slot, 4160, drive_capacity, &drive_handle);
            } else {
                fbe_test_pp_util_insert_sas_drive(0, encl_number, slot, 4160, drive_capacity + extra_blocks, &drive_handle);
            }
            num_4160_drives--;
            slot++;
        }
        encl_number++;
    }

    mut_printf(MUT_LOG_LOW, "=== starting physical package===");
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== set Physical Package entries ===");
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Init fbe api on server */
    mut_printf(MUT_LOG_LOW, "=== starting Server FBE_API ===");
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait for the expected number of objects */
    status = fbe_api_wait_for_number_of_objects(num_objects, 60000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    mut_printf(MUT_LOG_LOW, "=== verifying configuration ===");

    /* We inserted a armada board so we check for it.
     * board is always assumed to be object id 0.
     */
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_LOW, "Verifying board type....Passed");

    /* Make sure we have the expected number of objects.
     */
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(total_objects == num_objects);
    mut_printf(MUT_LOG_LOW, "Verifying object count...Passed");

    if (fbe_test_should_use_sim_psl()) {
        mut_printf(MUT_LOG_TEST_STATUS, "Use FBE Simulator PSL.");
        fbe_api_board_sim_set_psl(0);
    }

    return;
}
/******************************************
 * end fbe_test_pp_util_create_config_vary_capacity()
 ******************************************/
/*!**************************************************************
 * fbe_test_pp_common_destroy()
 ****************************************************************
 * @brief
 *  This function destroys PP.
 *
 * @param none
 * 
 * @return fbe_status_t
 *
 ****************************************************************/
void fbe_test_pp_common_destroy(void)
{
    fbe_status_t status;
    fbe_test_package_list_t package_list;

    mut_printf(MUT_LOG_LOW, "=== %s starts ===\n", __FUNCTION__);
        fbe_zero_memory(&package_list, sizeof(package_list));

    package_list.number_of_packages = 1;
    package_list.package_list[0] = FBE_PACKAGE_ID_PHYSICAL;

    fbe_test_common_util_package_destroy(&package_list);
    status = fbe_api_terminator_destroy();
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "destroy terminator api failed");

    fbe_test_common_util_verify_package_destroy_status(&package_list);
    fbe_test_common_util_verify_package_trace_error(&package_list);

    mut_printf(MUT_LOG_LOW, "=== %s completes ===\n", __FUNCTION__);

    return;
}
/*************************************************************
 * end of fbe_test_pp_common_destroy()
 *************************************************************/

/*!**************************************************************
 * fbe_test_neit_pp_common_destroy()
 ****************************************************************
 * @brief
 *  This function destroys NEIT and PP.
 *
 * @param none
 * 
 * @return fbe_status_t
 *
 ****************************************************************/
void fbe_test_neit_pp_common_destroy(void)
{
    fbe_status_t status;
    fbe_test_package_list_t package_list;

    mut_printf(MUT_LOG_LOW, "=== %s starts ===\n", __FUNCTION__);
    fbe_zero_memory(&package_list, sizeof(package_list));
    package_list.number_of_packages = 2;
    package_list.package_list[1] = FBE_PACKAGE_ID_NEIT;
    package_list.package_list[0] = FBE_PACKAGE_ID_PHYSICAL;
    
    fbe_test_common_util_package_destroy(&package_list);
    status = fbe_api_terminator_destroy();
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "destroy terminator api failed");

    fbe_test_common_util_verify_package_destroy_status(&package_list);
    fbe_test_common_util_verify_package_trace_error(&package_list);

    mut_printf(MUT_LOG_LOW, "=== %s completes ===\n", __FUNCTION__);

    return;
}
/*************************************************************
 * end of fbe_test_neit_pp_common_destroy()
 *************************************************************/

/*!**************************************************************
 *          fbe_test_pp_util_get_cmd_option()
 ****************************************************************
 * @brief
 *  Determine if an option is set.
 *
 * @param option_name_p - name of option to get
 *
 * @return fbe_bool_t - FBE_TRUE if value found FBE_FALSE otherwise.
 *
 ****************************************************************/
fbe_bool_t fbe_test_pp_util_get_cmd_option(char *option_name_p)
{
    if (mut_option_selected(option_name_p))
    {
        return FBE_TRUE;
    }
    return FBE_FALSE;
}
/*************************************************************
 * end of fbe_test_pp_util_get_cmd_option()
 *************************************************************/

/*!**************************************************************
 * fbe_test_pp_util_enable_trace_limits()
 ****************************************************************
 * @brief
 *  This function enables trace limits.
 *
 * @param none
 * 
 * @return fbe_status_t
 *
 ****************************************************************/
void fbe_test_pp_util_enable_trace_limits(void)
{
    fbe_status_t status;

    if (fbe_test_pp_util_get_cmd_option("-stop_physical_package_on_error"))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "-stop_physical_package_on_error set.  Enable error limits. ");

        /* Setup a limit for critical errors.  We stop on the first one.
         */
        status = fbe_api_trace_set_error_limit(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                               FBE_API_TRACE_ERROR_LIMIT_ACTION_STOP_SYSTEM,
                                               1,    /* Num errors. */
                                               FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Setup a limit for errors.  We stop on the first one.
         */
        status = fbe_api_trace_set_error_limit(FBE_TRACE_LEVEL_ERROR,
                                               FBE_API_TRACE_ERROR_LIMIT_ACTION_STOP_SYSTEM,
                                               1,    /* Num errors. */
                                               FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    return;
}
/*************************************************************
 * end of fbe_test_pp_util_enable_trace_limits()
 *************************************************************/

/*!****************************************************************************
 * @fn      fbe_zrt_wait_for_term_pp_sync(fbe_u32_t timeout_ms)
 *
 * @brief
 * This function waits for upto timeout_ms for the physical package
 * and terminator to get in sync with respect to the number of
 * objects.  The terminator count is expected to remain the same so
 * it's count is determined first.  The subsequent call wait for
 * objects polls the physical package until it sees that many ojects
 * or times out.
 *
 * @param timeout_ms - timeout in milleseconds.
 *
 * @return
 * FBE_STATUS_OK - if we are in sync.  Failure causes an assert which
 * stops the simulation.
 *
 * HISTORY
 *  08/03/11 :  Trupti Ghate-- Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_wait_for_term_pp_sync(fbe_u32_t timeout_ms)
{
    fbe_status_t status;
    fbe_terminator_device_count_t dev_counts;

    /* Let's see how many objects we have...
     */  
    status = fbe_test_calc_object_count(&dev_counts);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* wait for the expected number of objects */
    status = fbe_api_wait_for_number_of_objects(dev_counts.total_devices, timeout_ms, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");
    return FBE_STATUS_OK;
}

/*************************************************************
 * end of fbe_test_wait_for_term_pp_sync()
 *************************************************************/

/*!********************************************************************
 *  @fn fbe_test_pp_wait_for_ps_status ()
 *********************************************************************
 *  @brief
 *    This function poll the PP for the ps status.
 *
 *  @param pLocation
 *  @param expectedInserted
 *  @param expectedFaulted
 *  @param timeout_ms - timeout value in mili-second
 *
 *  @return fbe_status_t
 *
 *  @version
 *    12/05/2013 zhoue1
 *    
 *********************************************************************/
fbe_status_t fbe_test_pp_wait_for_ps_status(fbe_object_id_t object_id,
                                             fbe_device_physical_location_t * pLocation,
                                             fbe_bool_t expectedInserted,
                                             fbe_mgmt_status_t expectedFaulted,
                                             fbe_u32_t timeoutMs)
{
    fbe_status_t                         status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u32_t                            currentTime = 0;
    fbe_power_supply_info_t              getPsInfo = {0};

    while (currentTime < timeoutMs){
        fbe_zero_memory(&getPsInfo, sizeof(getPsInfo));
        getPsInfo.slotNumOnEncl = pLocation->slot;
        status = fbe_api_power_supply_get_power_supply_info(object_id, &getPsInfo);

        if (status != FBE_STATUS_OK){
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                          "%s: Get PS info returned with status 0x%X.\n",
                          __FUNCTION__, status); 
            return status;
        }
            
        if((getPsInfo.bInserted == expectedInserted) &&
           (getPsInfo.generalFault == expectedFaulted))
        {
            return status;
        }
        
        currentTime = currentTime + FBE_API_POLLING_INTERVAL;
        fbe_api_sleep (FBE_API_POLLING_INTERVAL);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                  "%s: wait for ps inserted %d, faulted %d failed within the expected %d ms!\n",
                  __FUNCTION__,
                  expectedInserted,
                  expectedFaulted,
                  timeoutMs);

    return FBE_STATUS_GENERIC_FAILURE;
}

/*!********************************************************************
 *  @fn fbe_test_pp_wait_for_cooling_status ()
 *********************************************************************
 *  @brief
 *    This function poll the PP for the cooling status.
 *
 *  @param pLocation
 *  @param expectedInserted
 *  @param expectedFaulted
 *  @param timeout_ms - timeout value in mili-second
 *
 *  @return fbe_status_t
 *
 *  @version
 *    12/04/2013  zhoue1
 *    
 *********************************************************************/
fbe_status_t fbe_test_pp_wait_for_cooling_status(fbe_object_id_t object_id,
                                             fbe_device_physical_location_t * pLocation,
                                             fbe_bool_t expectedInserted,
                                             fbe_mgmt_status_t expectedFaulted,
                                             fbe_u32_t timeoutMs)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u32_t                               currentTime = 0;
    fbe_cooling_info_t                      getFanInfo = {0};

    while (currentTime < timeoutMs){
        fbe_zero_memory(&getFanInfo, sizeof(getFanInfo));
        getFanInfo.slotNumOnEncl = pLocation->slot;
        status = fbe_api_cooling_get_fan_info(object_id, &getFanInfo);

        if (status != FBE_STATUS_OK){
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                          "%s: Get Fan info returned with status 0x%X.\n",
                          __FUNCTION__, status); 
            return status;
        }
            
        if((getFanInfo.inserted == expectedInserted) &&
           (getFanInfo.fanFaulted == expectedFaulted))
        {
            return status;
        }
        
        currentTime = currentTime + FBE_API_POLLING_INTERVAL;
        fbe_api_sleep (FBE_API_POLLING_INTERVAL);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                  "%s: wait for cooling inserted %d, faulted %d failed within the expected %d ms!\n",
                  __FUNCTION__,
                  expectedInserted,
                  expectedFaulted,
                  timeoutMs);

    return FBE_STATUS_GENERIC_FAILURE;
}

/*!********************************************************************
 * @fn  fbe_test_calc_object_count()
 *********************************************************************
 *
 * @brief
 * Calculates the total number of objects created. 
 *
 * History:
 *  8/3/2011 - Created. Trupti Ghate
 *
 *********************************************************************/
fbe_status_t fbe_test_calc_object_count(fbe_terminator_device_count_t *dev_counts)
{
    fbe_status_t status;

    if ((status = fbe_api_terminator_get_terminator_device_count(dev_counts)) != FBE_STATUS_OK)
    {
        mut_printf(MUT_LOG_LOW, "count terminator devices failed \n");
        return status;
    }
    else
    {
        mut_printf(MUT_LOG_LOW, 
            "term count devices: total_devices %d, boards %d, ports %d, encl %d, drives %d (log_drives %d)\n",
            dev_counts->total_devices, dev_counts->num_boards, dev_counts->num_ports,
            dev_counts->num_enclosures, dev_counts->num_drives, dev_counts->num_drives);
    }

    return FBE_STATUS_OK;
}
/*************************************************************
 * end of fbe_test_calc_object_count()
 *************************************************************/

/*!****************************************************************************
 * @fn      fbe_test_verify_term_pp_sync(void)
 *
 * @brief
 * This function verifies that the terminator and the physical package
 * are in sync up with respect to the number of objects.  The
 * terminator is asked with the results written to the dev_counts. 
 * The Physical package is asked with the results written to
 * total_objects.  If these don't match, we assert an error.
 *
 * @return
 * FBE_STATUS_OK - if we are in sync.  Failure causes an assert which
 * stops the simulation.
 *
 * HISTORY
 *  08/03/11 :  Trupti Ghate -- Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_verify_term_pp_sync(void)
{
    fbe_terminator_device_count_t   dev_counts;
    fbe_status_t                    status;
    fbe_u32_t                       total_objects = 0;

    status = fbe_test_calc_object_count(&dev_counts);    /* Count number of objects in Terminator */  
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL); /* Count number of objects in Physical Package */  
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    MUT_ASSERT_TRUE(total_objects == dev_counts.total_devices); /* They better match */
    return FBE_STATUS_OK;
}
/*************************************************************
 * end of fbe_test_verify_term_pp_sync()
 *************************************************************/

/*!****************************************************************************
 * @fn fbe_test_insert_sas_enclosure (fbe_terminator_api_device_handle_t parent_handle,
 *                              fbe_terminator_sas_encl_info_t *encl_info, 
 *                              fbe_terminator_api_device_handle_t *encl_handle,
 *                              fbe_u32_t *num_handles)
 ******************************************************************************
 * @brief
 *  This function inserts a SAS enclosure 
 *
 * @param None
 *
 * @return
 *  None
 *
 * @version
 *  27-Apr-2011: PHE - Copied over from fbe_zrt_insert_sas_enclosure.
 *
 *****************************************************************************/
fbe_status_t fbe_test_insert_sas_enclosure (fbe_terminator_api_device_handle_t parent_handle,
                                           fbe_terminator_sas_encl_info_t *encl_info, 
                                           fbe_terminator_api_device_handle_t *encl_handle,
                                           fbe_u32_t *num_handles)
{
    fbe_status_t                status;
    fbe_u8_t                    numDrvExp;
    fbe_u8_t                    firstDrvExp;
    fbe_sas_enclosure_type_t    sasEnclType = FBE_SAS_ENCLOSURE_TYPE_INVALID;

    switch(encl_info->encl_type)
    {
        case FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM :
            numDrvExp = 2;
            firstDrvExp = 4;
            sasEnclType = FBE_SAS_ENCLOSURE_TYPE_VOYAGER_EE;
        break;
        case FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP :
            numDrvExp = 4;
            firstDrvExp = 2;
            sasEnclType = FBE_SAS_ENCLOSURE_TYPE_VIKING_DRVSXP;
        break;
        case FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP :
            numDrvExp = 1;
            firstDrvExp = 4;
            sasEnclType = FBE_SAS_ENCLOSURE_TYPE_CAYENNE_DRVSXP;
        break;
        case FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP :
            numDrvExp = 2;
            firstDrvExp = 4;
            sasEnclType = FBE_SAS_ENCLOSURE_TYPE_NAGA_DRVSXP;
        break;
        default:
            //enclosures with single expander, like FBE_SAS_ENCLOSURE_TYPE_TABASCO
            status = fbe_api_terminator_insert_sas_enclosure (parent_handle, encl_info, encl_handle);
            *num_handles = 1;
        break;
    }

    if(sasEnclType != FBE_SAS_ENCLOSURE_TYPE_INVALID)
    {
        status = fbe_test_pp_util_insert_sas_expanders(parent_handle, encl_info, encl_handle, num_handles,
                                                       sasEnclType,
                                                       firstDrvExp,
                                                       numDrvExp);
    }
    return status;
}
/*************************************************************
 * end of fbe_test_insert_sas_enclosure()
 *************************************************************/

/*!****************************************************************************
 * @fn fbe_test_pp_util_internal_insert_voyager_sas_enclosure(fbe_terminator_api_device_handle_t parent_handle,
 *                                 fbe_terminator_sas_encl_info_t *encl_info, 
 *                                 fbe_terminator_api_device_handle_t *encl_handle,
 *                                 fbe_u32_t *num_handles)
 ******************************************************************************
 * @brief
 *  This function inserts a voyager enclosure and the corresponding edge expanders.
 *
 * @param parent_handle - parent where the new enclosure will be inserted.
 * @param encl_info - characteristics of the enclosure to insert (ICM)
 * @param ret_handles - fills up with terminator handles for ICM, EE0
 * and EE1.  The num_handles will be set to 3.
 *
 * @return
 *  FBE_STATUS_OK if all goes well.
 *
 * @version
 *  27-Apr-2011: PHE - Copied over from fbe_zrt_internal_insert_voyager_sas_enclosure.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_pp_util_internal_insert_voyager_sas_enclosure (fbe_terminator_api_device_handle_t parent_handle,
                                                      fbe_terminator_sas_encl_info_t *encl_info, 
                                                      fbe_terminator_api_device_handle_t *encl_handle,
                                                      fbe_u32_t *num_handles)
{
    fbe_status_t status;
    fbe_terminator_sas_encl_info_t sas_encl; 
    fbe_terminator_api_device_handle_t term_encl_handle;

    *num_handles = 0;

    memcpy(&sas_encl, encl_info, sizeof(*encl_info));

    /* Insert the VOYAGER ICM into the parent */
    status = fbe_api_terminator_insert_sas_enclosure (parent_handle, encl_info, encl_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    (*num_handles)++;
    mut_printf(MUT_LOG_LOW, "ICM connector_id 0 sas address: %llX\n",
           (unsigned long long)encl_info->sas_address);


    /* Insert the two VOYAGER edge expanders into the ICM enclosure */
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VOYAGER_EE;
    //sas_encl.uid[0] = 1; // some unique ID.

    // Insert the first EE
    sas_encl.connector_id = 4;
    //sas_encl.sas_address = encl_info->sas_address + sas_encl.connector_id; // Add in the connector id to make this unique.
    sas_encl.sas_address = FBE_TEST_BUILD_LCC_SAS_ADDRESS(encl_info->backend_number, encl_info->encl_number, 0);
    status = fbe_api_terminator_insert_sas_enclosure (*encl_handle, &sas_encl, &term_encl_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    (*num_handles)++;
    mut_printf(MUT_LOG_LOW,
           "EE0 connector_id 4 sas address: %llX, ee0 handle %llu\n",
           (unsigned long long)sas_encl.sas_address,
           (unsigned long long)term_encl_handle);

    // Insert the second EE
    sas_encl.connector_id = 5;
    //sas_encl.sas_address = encl_info->sas_address + sas_encl.connector_id; // Add in the connector id to make this unique.
    sas_encl.sas_address = FBE_TEST_BUILD_LCC_SAS_ADDRESS(encl_info->backend_number, encl_info->encl_number, 1);
    status = fbe_api_terminator_insert_sas_enclosure (*encl_handle, &sas_encl, &term_encl_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    (*num_handles)++;
    mut_printf(MUT_LOG_LOW,
               "EE1 connector_id 5 sas address: %llX, ee1 handle %llu\n",
               (unsigned long long)sas_encl.sas_address,
               (unsigned long long)term_encl_handle);

    return status;
}

/*!****************************************************************************
 * @fn fbe_test_pp_util_internal_insert_viking_sas_enclosure(fbe_terminator_api_device_handle_t parent_handle,
 *                                 fbe_terminator_sas_encl_info_t *encl_info, 
 *                                 fbe_terminator_api_device_handle_t *encl_handle,
 *                                 fbe_u32_t *num_handles)
 ******************************************************************************
 * @brief
 *  This function inserts a viking enclosure and the corresponding DRVSXP.
 *
 * @param parent_handle - parent where the new enclosure will be inserted.
 * @param encl_info - characteristics of the enclosure to insert (IOSXP)
 * @param ret_handles - fills up with terminator handles for IOSXP, DRVSXP0-3
 *  The num_handles will be set to 5.
 *
 * @return
 *  FBE_STATUS_OK if all goes well.
 *
 * @version
 *  23-Nov-2-13: Greg Bailey - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_pp_util_internal_insert_viking_sas_enclosure (fbe_terminator_api_device_handle_t parent_handle,
                                                      fbe_terminator_sas_encl_info_t *encl_info, 
                                                      fbe_terminator_api_device_handle_t *encl_handle,
                                                      fbe_u32_t *num_handles)
{
    fbe_status_t status;
    fbe_terminator_sas_encl_info_t sas_encl; 
    fbe_terminator_api_device_handle_t term_encl_handle;

    *num_handles = 0;

    memcpy(&sas_encl, encl_info, sizeof(*encl_info));

    /* Insert the Viking ICM into the parent */
    status = fbe_api_terminator_insert_sas_enclosure (parent_handle, encl_info, encl_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    (*num_handles)++;
    mut_printf(MUT_LOG_LOW, "IOSXP connector_id 0 sas address: %llX\n",
               (unsigned long long)encl_info->sas_address);


    /* Insert the 4 Viking edge expanders into the IOSXP enclosure */
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIKING_DRVSXP;


    // Insert the first EE
    sas_encl.connector_id = 2;
    //sas_encl.sas_address = encl_info->sas_address + sas_encl.connector_id; // Add in the connector id to make this unique.
    sas_encl.sas_address = FBE_TEST_BUILD_LCC_SAS_ADDRESS(encl_info->backend_number, encl_info->encl_number, sas_encl.connector_id);
    status = fbe_api_terminator_insert_sas_enclosure (*encl_handle, &sas_encl, &term_encl_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    (*num_handles)++;
    mut_printf(MUT_LOG_LOW,
               "DRVSXP0 connector_id %d sas address: %llX, SXP0 handle %llu\n",
               sas_encl.connector_id,
               (unsigned long long)sas_encl.sas_address,
               (unsigned long long)term_encl_handle);

    // Insert the second EE
    sas_encl.connector_id = 3;
    //sas_encl.sas_address = encl_info->sas_address + sas_encl.connector_id; // Add in the connector id to make this unique.
    sas_encl.sas_address = FBE_TEST_BUILD_LCC_SAS_ADDRESS(encl_info->backend_number, encl_info->encl_number, sas_encl.connector_id);
    status = fbe_api_terminator_insert_sas_enclosure (*encl_handle, &sas_encl, &term_encl_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    (*num_handles)++;
    mut_printf(MUT_LOG_LOW,
               "DRVSXP1 connector_id %d sas address: %llX, SXP1 handle %llu\n",
               sas_encl.connector_id,
               (unsigned long long)sas_encl.sas_address,
               (unsigned long long)term_encl_handle);

    // Insert the third EE
    sas_encl.connector_id = 4;
    //sas_encl.sas_address = encl_info->sas_address + sas_encl.connector_id; // Add in the connector id to make this unique.
    sas_encl.sas_address = FBE_TEST_BUILD_LCC_SAS_ADDRESS(encl_info->backend_number, encl_info->encl_number, sas_encl.connector_id);
    status = fbe_api_terminator_insert_sas_enclosure (*encl_handle, &sas_encl, &term_encl_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    (*num_handles)++;
    mut_printf(MUT_LOG_LOW,
               "DRVSXP2 connector_id %d sas address: %llX, SXP2 handle %llu\n",
               sas_encl.connector_id,
               (unsigned long long)sas_encl.sas_address,
               (unsigned long long)term_encl_handle);

    // Insert the fourth EE
    sas_encl.connector_id = 5;
    //sas_encl.sas_address = encl_info->sas_address + sas_encl.connector_id; // Add in the connector id to make this unique.
    sas_encl.sas_address = FBE_TEST_BUILD_LCC_SAS_ADDRESS(encl_info->backend_number, encl_info->encl_number, sas_encl.connector_id);
    status = fbe_api_terminator_insert_sas_enclosure (*encl_handle, &sas_encl, &term_encl_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    (*num_handles)++;
    mut_printf(MUT_LOG_LOW,
               "DRVSXP3 connector_id %d sas address: %llX, SXP3 handle %llu\n",
               sas_encl.connector_id,
               (unsigned long long)sas_encl.sas_address,
               (unsigned long long)term_encl_handle);

    return status;
}
/*************************************************************
 * end of fbe_test_pp_util_internal_insert_voyager_sas_enclosure()
 *************************************************************/


/*!****************************************************************************
 * @fn fbe_test_pp_util_internal_insert_naga_sas_enclosure(fbe_terminator_api_device_handle_t parent_handle,
 *                                 fbe_terminator_sas_encl_info_t *encl_info, 
 *                                 fbe_terminator_api_device_handle_t *encl_handle,
 *                                 fbe_u32_t *num_handles)
 ******************************************************************************
 * @brief
 *  This function inserts a naga enclosure and the corresponding DRVSXP.
 *
 * @param parent_handle - parent where the new enclosure will be inserted.
 * @param encl_info - characteristics of the enclosure to insert (IOSXP)
 * @param ret_handles - fills up with terminator handles for IOSXP, DRVSXP0-3
 *  The num_handles will be set to 5.
 *
 * @return
 *  FBE_STATUS_OK if all goes well.
 *
 * @version
 *  23-Nov-2-13: Greg Bailey - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_pp_util_internal_insert_naga_sas_enclosure (fbe_terminator_api_device_handle_t parent_handle,
                                                      fbe_terminator_sas_encl_info_t *encl_info, 
                                                      fbe_terminator_api_device_handle_t *encl_handle,
                                                      fbe_u32_t *num_handles)
{
    fbe_status_t status;
    fbe_terminator_sas_encl_info_t sas_encl; 
    fbe_terminator_api_device_handle_t term_encl_handle;

    *num_handles = 0;

    memcpy(&sas_encl, encl_info, sizeof(*encl_info));

    /* Insert the Naga ICM into the parent */
    status = fbe_api_terminator_insert_sas_enclosure (parent_handle, encl_info, encl_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    (*num_handles)++;
    mut_printf(MUT_LOG_LOW, "IOSXP connector_id 0 sas address: %llX\n",
               (unsigned long long)encl_info->sas_address);


    /* Insert the 4 Naga edge expanders into the IOSXP enclosure */
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_NAGA_DRVSXP;


    // Insert the first EE
    sas_encl.connector_id = 4;
    //sas_encl.sas_address = encl_info->sas_address + sas_encl.connector_id; // Add in the connector id to make this unique.
    sas_encl.sas_address = FBE_TEST_BUILD_LCC_SAS_ADDRESS(encl_info->backend_number, encl_info->encl_number, sas_encl.connector_id);
    status = fbe_api_terminator_insert_sas_enclosure (*encl_handle, &sas_encl, &term_encl_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    (*num_handles)++;
    mut_printf(MUT_LOG_LOW,
               "DRVSXP0 connector_id %d sas address: %llX, SXP0 handle %llu\n",
               sas_encl.connector_id,
               (unsigned long long)sas_encl.sas_address,
               (unsigned long long)term_encl_handle);

    // Insert the second EE
    sas_encl.connector_id = 5;
    //sas_encl.sas_address = encl_info->sas_address + sas_encl.connector_id; // Add in the connector id to make this unique.
    sas_encl.sas_address = FBE_TEST_BUILD_LCC_SAS_ADDRESS(encl_info->backend_number, encl_info->encl_number, sas_encl.connector_id);
    status = fbe_api_terminator_insert_sas_enclosure (*encl_handle, &sas_encl, &term_encl_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    (*num_handles)++;
    mut_printf(MUT_LOG_LOW,
               "DRVSXP1 connector_id %d sas address: %llX, SXP1 handle %llu\n",
               sas_encl.connector_id,
               (unsigned long long)sas_encl.sas_address,
               (unsigned long long)term_encl_handle);

    

    return status;
}
/*************************************************************
 * end of fbe_test_pp_util_internal_insert_voyager_sas_enclosure()
 *************************************************************/

/*!****************************************************************************
 * @fn fbe_test_pp_util_internal_insert_cayenne_sas_enclosure(fbe_terminator_api_device_handle_t parent_handle,
 *                                 fbe_terminator_sas_encl_info_t *encl_info, 
 *                                 fbe_terminator_api_device_handle_t *encl_handle,
 *                                 fbe_u32_t *num_handles)
 ******************************************************************************
 * @brief
 *  This function inserts a Cayenne enclosure and the corresponding DRVSXP.
 *
 * @param parent_handle - parent where the new enclosure will be inserted.
 * @param encl_info - characteristics of the enclosure to insert (IOSXP)
 * @param ret_handles - fills up with terminator handles for IOSXP, DRVSXP0-3
 *  The num_handles will be set to 5.
 *
 * @return
 *  FBE_STATUS_OK if all goes well.
 *
 * @version
 *  4-Apr-14: R. Porteus - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_pp_util_internal_insert_cayenne_sas_enclosure (fbe_terminator_api_device_handle_t parent_handle,
                                                      fbe_terminator_sas_encl_info_t *encl_info, 
                                                      fbe_terminator_api_device_handle_t *encl_handle,
                                                      fbe_u32_t *num_handles)
{
    fbe_status_t status;
    fbe_terminator_sas_encl_info_t sas_encl; 
    fbe_terminator_api_device_handle_t term_encl_handle;

    *num_handles = 0;

    memcpy(&sas_encl, encl_info, sizeof(*encl_info));

    /* Insert the Cayenne ICM into the parent */
    status = fbe_api_terminator_insert_sas_enclosure (parent_handle, encl_info, encl_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    (*num_handles)++;
    mut_printf(MUT_LOG_LOW, "IOSXP connector_id 0 sas address: %llX\n",
               (unsigned long long)encl_info->sas_address);


    /* Insert the 1 CAYENNE edge expanders into the IOSXP enclosure */
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_CAYENNE_DRVSXP;


    // Insert the first EE
    sas_encl.connector_id = 4;
    //sas_encl.sas_address = encl_info->sas_address + sas_encl.connector_id; // Add in the connector id to make this unique.
    sas_encl.sas_address = FBE_TEST_BUILD_LCC_SAS_ADDRESS(encl_info->backend_number, encl_info->encl_number, sas_encl.connector_id);
    status = fbe_api_terminator_insert_sas_enclosure (*encl_handle, &sas_encl, &term_encl_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    (*num_handles)++;
    mut_printf(MUT_LOG_LOW,
               "DRVSXP0 connector_id %d sas address: %llX, SXP0 handle %llu\n",
               sas_encl.connector_id,
               (unsigned long long)sas_encl.sas_address,
               (unsigned long long)term_encl_handle);

    return status;
}
/*************************************************************
 * end of fbe_test_pp_util_internal_insert_cayenne_sas_enclosure()
 *************************************************************/

static fbe_status_t fbe_test_pp_util_insert_sas_expanders (fbe_terminator_api_device_handle_t parent_handle,
                                                      fbe_terminator_sas_encl_info_t *encl_info, 
                                                      fbe_terminator_api_device_handle_t *encl_handle,
                                                      fbe_u32_t *num_handles,
                                                        fbe_sas_enclosure_type_t sasEnclType,
                                                        fbe_u8_t firstDrvExp,
                                                        fbe_u8_t numDrvExp)
{
    fbe_status_t                        status;
    fbe_terminator_sas_encl_info_t      sas_encl; 
    fbe_terminator_api_device_handle_t  term_encl_handle;
    fbe_u8_t                            i, cid;

    *num_handles = 0;

    memcpy(&sas_encl, encl_info, sizeof(*encl_info));

    // insert the IOSXP
    status = fbe_api_terminator_insert_sas_enclosure (parent_handle, encl_info, encl_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    (*num_handles)++;
    mut_printf(MUT_LOG_LOW, "IOSXP connector_id 0 sas address: %llX\n",
               (unsigned long long)encl_info->sas_address);

    // Set the drvsxp type and start adding them 
    sas_encl.encl_type = sasEnclType;

    for(i=0, cid=firstDrvExp; i<numDrvExp ; i++, cid++)
    {
        sas_encl.connector_id = cid;
        //sas_encl.sas_address = encl_info->sas_address + sas_encl.connector_id; // Add in the connector id to make this unique.
        sas_encl.sas_address = FBE_TEST_BUILD_LCC_SAS_ADDRESS(encl_info->backend_number, encl_info->encl_number, sas_encl.connector_id);
        status = fbe_api_terminator_insert_sas_enclosure (*encl_handle, &sas_encl, &term_encl_handle);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        (*num_handles)++;
        mut_printf(MUT_LOG_LOW,
                   "DRVSXP%d connector_id %d sas address: %llX, SXP handle %llu\n",
                    i,
                    sas_encl.connector_id,
                   (unsigned long long)sas_encl.sas_address,
                   (unsigned long long)term_encl_handle);
    }

    return status;
} // end fbe_test_pp_util_insert_sas_expanders

/*!****************************************************************************
 * @fn fbe_test_insert_sas_drive(fbe_terminator_api_device_handle_t encl_handle,
 *                              fbe_u32_t slot_number,
 *                              fbe_terminator_sas_drive_info_t *drive_info, 
 *                              fbe_api_terminator_device_handle_t *drive_handle)
 * ****************************************************************************
 * @brief
 *  This function inserts a sas drive. 
 *
 * @param None
 *
 * @return
 *  None
 *
 * @version
 *  27-Apr-2011: PHE - Copied over from fbe_zrt_insert_sas_drive.
 *
 *****************************************************************************/
fbe_status_t fbe_test_insert_sas_drive (fbe_terminator_api_device_handle_t encl_handle,
                                       fbe_u32_t slot_number,
                                       fbe_terminator_sas_drive_info_t *drive_info, 
                                       fbe_api_terminator_device_handle_t *drive_handle)
{
    fbe_status_t status;
    fbe_term_encl_connector_list_t connector_ids;

    status = fbe_api_terminator_get_connector_id_list_for_enclosure (encl_handle, &connector_ids);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    if (connector_ids.num_connector_ids == 0) // If this is one we have a Viper, Derringer, etc.
    {
        status  = fbe_api_terminator_insert_sas_drive (encl_handle, slot_number, drive_info, drive_handle);
    }
    else // It must be a Voyager. We will need to handle Viking later.
    {
        status  = fbe_test_pp_util_internal_insert_voyager_sas_drive (encl_handle, slot_number, drive_info, drive_handle);
    }
    return status;
}
/*************************************************************
 * end of fbe_test_insert_sas_drive()
 *************************************************************/


/*!****************************************************************************
 * @fn fbe_test_pp_util_internal_insert_voyager_sas_drive(fbe_terminator_api_device_handle_t encl_handle,
 *                                      fbe_u32_t slot_number,
 *                                      fbe_terminator_sas_drive_info_t *drive_info, 
 *                                      fbe_api_terminator_device_handle_t *drive_handle)
 ******************************************************************************
 * @brief
 * This function inserts a drive into the appropriate edge expander of
 * the voyager enclosure.  This is based on the slot number.  It is
 * assumed that the encl_handles is already setup.
 *
 * @param encl_handle - the terminator handle associated with the ICM.
 * @param slot_number - a number between 0 and 59 which indicates
 *                      where the drive is to be inserted.
 * @param drive_info - characteristics of the drive to be inserted.
 * @param drive_handle - used to return the handle for the drive inserted.
 *
 * @return
 *   FBE_STATUS_OK if all goes well.
 *
 * @version
 *  27-Apr-2011: PHE - Copied over from fbe_zrt_internal_insert_voyager_sas_drive.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_pp_util_internal_insert_voyager_sas_drive (fbe_terminator_api_device_handle_t encl_handle,
                                       fbe_u32_t slot_number,
                                       fbe_terminator_sas_drive_info_t *drive_info, 
                                       fbe_api_terminator_device_handle_t *drive_handle)
{
    fbe_status_t status;
    fbe_terminator_api_device_handle_t temp_term_handle;
    fbe_u32_t temp_slot_number = slot_number;

    temp_term_handle = encl_handle;

    status = fbe_api_terminator_get_drive_slot_parent(&temp_term_handle, &temp_slot_number);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return fbe_api_terminator_insert_sas_drive (temp_term_handle, slot_number, drive_info, drive_handle);
}
/*************************************************************
 * end of fbe_test_pp_util_internal_insert_voyager_sas_drive()
 *************************************************************/

/*!**************************************************************
 * fbe_test_pp_utils_get_default_flash_drive_type_based_on_block_size()
 ****************************************************************
 * @brief
 *  Determine the default flash drive type based on the required block size.
 *
 * @param   drive_block_size - The physical block size of the drive
 * @param   drive_type_p - Addresss of drive type to populate
 *
 * @note    Only works in simulation environment.
 * 
 * @return  status
 *
 ****************************************************************/
static fbe_status_t fbe_test_pp_utils_get_default_flash_drive_type_based_on_block_size(fbe_block_size_t drive_block_size,
                                                                                 fbe_sas_drive_type_t *drive_type_p)
{
    /*! @note Currently these are the only block size supported.
     */
    *drive_type_p = FBE_SAS_DRIVE_INVALID;
    switch(drive_block_size)
    {
        case FBE_BE_BYTES_PER_BLOCK:
            *drive_type_p = FBE_SAS_DRIVE_SIM_520_FLASH_HE;
            break;

        default:
            /* The requested block size is not supported.
             */
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "=== %s Requested block size: %d NOT supported !!!",
                       __FUNCTION__, drive_block_size);
            MUT_ASSERT_INT_EQUAL(FBE_BE_BYTES_PER_BLOCK, drive_block_size);
            return FBE_STATUS_GENERIC_FAILURE;
    }
    
    return FBE_STATUS_OK;
}
/*********************************************************************
 * end fbe_test_pp_utils_get_default_flash_drive_type_based_on_block_size()
 ********************************************************************/

/*!**************************************************************
 * fbe_test_get_sas_flash_drive_info()
 ****************************************************************
 * @brief
 *  This function gets SAS Flash drive info.
 *
 * @param sas_drive_p - SAS drive info.
 * @param backend_number - backend number
 * @param encl_number - enclosure number
 * @param slot_number - slot number
 * @param block_size - block size
 * @param capacity - capacity
 * 
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_test_get_sas_flash_drive_info(fbe_terminator_sas_drive_info_t *sas_drive_p,
                                         fbe_u32_t backend_number,
                                         fbe_u32_t encl_number,
                                         fbe_u32_t slot_number,
                                         fbe_block_size_t block_size,
                                         fbe_lba_t capacity)
{
    fbe_status_t            status;
    fbe_sas_drive_type_t    drive_type;

    status = fbe_test_pp_utils_get_default_flash_drive_type_based_on_block_size(block_size, &drive_type);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_test_get_sas_drive_info_extend(sas_drive_p,
                        backend_number,
                        encl_number,
                        slot_number,
                        block_size,
                        capacity,
                        GET_SAS_DRIVE_SAS_ADDRESS(backend_number, encl_number, slot_number),
                        drive_type);

    return FBE_STATUS_OK;
}
/*************************************************************
 * end of fbe_test_get_sas_flash_drive_info()
 *************************************************************/

/*!**************************************************************
 * fbe_test_pp_util_insert_sas_flash_drive()
 ****************************************************************
 * @brief
 *  This function inserts SAS Flash drive info.
 *
 * @param backend_number - backend number
 * @param encl_number - enclosure number
 * @param slot_number - slot number
 * @param block_size - block size
 * @param capacity - capacity
 * @param drive_handle_p - drive handle output
 * 
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_test_pp_util_insert_sas_flash_drive(fbe_u32_t backend_number,
                                                     fbe_u32_t encl_number,
                                                     fbe_u32_t slot_number,
                                                     fbe_block_size_t block_size,
                                                     fbe_lba_t capacity,
                                                     fbe_api_terminator_device_handle_t  *drive_handle_p)
{
    fbe_status_t status;
    fbe_terminator_sas_drive_info_t sas_drive;
    fbe_api_terminator_device_handle_t encl_handle;

    status = fbe_api_terminator_get_enclosure_handle(backend_number, encl_number, &encl_handle);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    if((encl_number == 0) && (slot_number < 4) && (capacity < TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY)){
        capacity = TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY + TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY;
        mut_printf(MUT_LOG_LOW, "=== fbe_test_pp_util Increased drive capacity! ===\n");
    }

    status = fbe_test_get_sas_flash_drive_info(&sas_drive, 
                                               backend_number, encl_number, slot_number, 
                                               block_size, capacity);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }
    mut_printf(MUT_LOG_LOW, "=== _insert_sas_flash_drive: %d_%d_%d serial: %s inserted. blks:0x%llx sz:0x%x cap:%llu MB drive_type:0x%x ===",
               backend_number, encl_number, slot_number, sas_drive.drive_serial_number, sas_drive.capacity, sas_drive.block_size,  
               (sas_drive.block_size*sas_drive.capacity/(1024*1024)), sas_drive.drive_type);


    status  = fbe_api_terminator_insert_sas_drive(encl_handle, slot_number, &sas_drive, drive_handle_p);
    return status;
}
/*************************************************************
 * end of fbe_test_pp_util_insert_sas_flash_drive()
 *************************************************************/

/*!***************************************************************************
 * fbe_test_pp_util_set_terminator_completion_dpc()
 *****************************************************************************
 * @brief   This method will set the terminator I/O completion IRQL based on
 *          b_should_terminator_completion_be_at_dpc:
 *              o FBE_TRUE - The terminator will complete I/Os at DPC
 *              o FBE_FALSE - The terminator will complete I/Os at passive (thread)
 * 
 * @param   b_should_terminator_completion_be_at_dpc - FBE_TRUE - Terminator completion
 *              will be at DPC.
 *                                                   - FBE_FALSE - Change 
 *              terminator completion to be at passive.
 *                      
 * @return none
 *
 ***************************************************************************/
void fbe_test_pp_util_set_terminator_completion_dpc(fbe_bool_t b_should_terminator_completion_be_at_dpc)
{
    char       *value_string;
    fbe_u32_t   value;
    fbe_bool_t  b_is_terminator_completion_dpc_set = FBE_FALSE;

    value_string = mut_get_user_option_value("-term_set_completion_dpc");
    if (value_string != NULL) 
    {
        value = strtoul(value_string, 0, 0);
        b_is_terminator_completion_dpc_set = (value == 0) ? FBE_FALSE : FBE_TRUE;

        mut_printf(MUT_LOG_TEST_STATUS, "setting term completion dpc to: %d", b_is_terminator_completion_dpc_set);

        /* Now invoke the api to set the flags.  Currently we don't check status.
         */
        fbe_api_terminator_set_completion_dpc(b_is_terminator_completion_dpc_set);

    } /* end if the `set term_set_completion_dpc' option is set*/

    return;
}
/*************************************************************
 * end of fbe_test_pp_util_set_terminator_completion_dpc()
 *************************************************************/

/*!**************************************************************
 *          fbe_test_pp_util_get_cmd_option_int()
 ****************************************************************
 * @brief
 *  Return an option value that is an integer.
 *
 * @param option_name_p - name of option to get
 * @param value_p - pointer to value to return.
 *
 * @return fbe_bool_t - FBE_TRUE if value found FBE_FALSE otherwise.
 *
 ****************************************************************/
fbe_bool_t fbe_test_pp_util_get_cmd_option_int(char *option_name_p, fbe_u32_t *value_p)
{
    char *value = mut_get_user_option_value(option_name_p);

    if (value != NULL)
    {
        *value_p = strtoul(value, 0, 0);
        return FBE_TRUE;
    }
    return FBE_FALSE;
}

/*!***************************************************************************
 * fbe_test_pp_util_set_terminator_drive_debug_flags()
 *****************************************************************************
 * @brief  This method will return either the passed (i.e. in_debug_flags) 
 *         or the system value of `fbe_debug_flags'
 * 
 * @param flags - if -term_drive_debug_flags is set, we use that value
 *                 otherwise we use the passed in value.
 *                      
 * @return none
 *
 ***************************************************************************/
void fbe_test_pp_util_set_terminator_drive_debug_flags(fbe_terminator_drive_debug_flags_t in_terminator_drive_debug_flags)
{
    char * CSX_MAYBE_UNUSED drive_range_string_p = mut_get_user_option_value("-term_drive_debug_range");
    fbe_terminator_drive_select_type_t drive_select_type = FBE_TERMINATOR_DRIVE_SELECT_TYPE_INVALID;
    fbe_u32_t first_term_drive_index = FBE_TERMINATOR_DRIVE_SELECT_NO_DRIVES;
    fbe_u32_t last_term_drive_index = FBE_TERMINATOR_DRIVE_SELECT_NO_DRIVES;
    fbe_u32_t backend_bus_number = FBE_TERMINATOR_DRIVE_SELECT_NO_DRIVES;
    fbe_u32_t encl_number = FBE_TERMINATOR_DRIVE_SELECT_NO_DRIVES;
    fbe_u32_t slot_number = FBE_TERMINATOR_DRIVE_SELECT_NO_DRIVES;
    fbe_terminator_drive_debug_flags_t terminator_drive_debug_flags = 0;
    fbe_u32_t cmd_line_flags = 0;

    if (fbe_test_pp_util_get_cmd_option_int("-term_drive_debug_flags", &cmd_line_flags))
    {
        terminator_drive_debug_flags = cmd_line_flags;
        mut_printf(MUT_LOG_TEST_STATUS, "using term drive debug flags of: 0x%x", terminator_drive_debug_flags);

        /*! @todo Need to handle the following strings:
         *          o "<first>-<last>"
         *          o "b/e/s"
         *
         *  @todo Currently if either `all drives' or `single index' is not
         *        specified, we will default to `all drives'.
         *       
         */
        if (fbe_test_pp_util_get_cmd_option_int("-term_drive_debug_range", &cmd_line_flags))
        {
            first_term_drive_index = cmd_line_flags;
            mut_printf(MUT_LOG_TEST_STATUS, "using term drive index range of: 0x%x", first_term_drive_index);
        }
        else
        {
            /*! @todo Currently we default to `all drives'
             */
            first_term_drive_index = FBE_TERMINATOR_DRIVE_SELECT_ALL_DRIVES;
        }

        /*! @todo More work to do to support range.  For now either single terminator
         *        array index or `all drives'.
         */
        if (first_term_drive_index != FBE_TERMINATOR_DRIVE_SELECT_ALL_DRIVES)
        {
            drive_select_type = FBE_TERMINATOR_DRIVE_SELECT_TYPE_TERM_DRIVE_INDEX;
        }
        else
        {
            drive_select_type = FBE_TERMINATOR_DRIVE_SELECT_TYPE_ALL_DRIVES;
        }

        /* Now invoke the api to set the flags.  Currently we don't check status.
         */
        fbe_api_terminator_set_simulated_drive_debug_flags(drive_select_type,
                                                           first_term_drive_index,
                                                           last_term_drive_index,
                                                           backend_bus_number,
                                                           encl_number,
                                                           slot_number,
                                                           terminator_drive_debug_flags);

    } /* end if the `set terminator drive debug flags' option is set*/

    return;
}
/*************************************************************
 * end of fbe_test_pp_util_set_terminator_drive_debug_flags()
 *************************************************************/

/*******************************
 * end file fbe_test_pp_utils.c
 *******************************/
 
 
