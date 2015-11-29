/***************************************************************************
 * Copyright (C) EMC Corporation 2008-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * andalusia.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the Andalusia scenario.
 * 
 *  This scenario is for a 1 port, 1 enclosure configuration, aka the maui
 *  configuration.
 * 
 *  The test run in this case is:
 *      - Initialize the 1-Port, 1-Enclosure configuration and 
 *          verify that the topology is correct.
 *      - Check the enclosure and the drives are in READY state
 *      - Read the resume prom of all the components in the enclosure. (response_buffer)
 *      - Write the resume prom of all the components in the enclosure. (data_buffer)
 *      - Read the resume prom of all the components in the enclosure (response_buffer)
 *          again to verify the written data is reflected. 
 *      - Shutdown the physical package. 
 *
 * HISTORY
 *   01/27/2009:  Created. ArunKumar Selvapillai
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "physical_package_tests.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_enclosure_data_access_public.h"
#include "fbe/fbe_api_resume_prom_interface.h"


/*!*******************************************************************
 * @enum andalusia_tests_t
 *********************************************************************
 * @brief The list of unique configurations to be tested in 
 * andalusia_test.
 *********************************************************************/

typedef enum andalusia_e
{
    ANDULASIA_TEST_VIPER,
    ANDULASIA_TEST_DERRINGER,
//    ANDULASIA_TEST_VOYAGER,
    ANDULASIA_TEST_BUNKER,
//    ANDULASIA_TEST_FALLBACK,
    ANDULASIA_TEST_BOXWOOD,
    ANDULASIA_TEST_KNOT,
//    ANDULASIA_TEST_CALYPSO,
// ENCL_CLEANUP
//    ANDULASIA_TEST_RHEA,
//    ANDULASIA_TEST_MIRANDA,
//    ANDULASIA_TEST_TABASCO, 
} andalusia_tests_t;


/*!*******************************************************************
 * @var test_table
 *********************************************************************
 * @brief This is a table of set of parameters for which the test has 
 * to be executed.
 *********************************************************************/
static fbe_test_params_t test_table[] = 
{
    {ANDULASIA_TEST_VIPER,
     "VIPER",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_VIPER,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_VIPER,

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
     {ANDULASIA_TEST_DERRINGER,
     "DERRINGER",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_DERRINGER,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_DERRINGER,

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
    /*{ANDULASIA_TEST_VOYAGER,
     "VOYAGER",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_VOYAGER,
     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },*/
     {ANDULASIA_TEST_BUNKER,
     "BUNKER",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_BUNKER,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_BUNKER,

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
 /* 
    Jetfire LCC resum eeprom is not connected to expander, so when this test case try to get that 
    buffer, it faulted. So just skiip this test for jetfire. 
    fbe_api_terminator_set_resume_prom_info(LCC_A_RESUME_PROM) -> ->
    config_page_info_get_buf_id_by_subencl_info() cannot find right item in the for loop.
    {ANDULASIA_TEST_FALLBACK,
     "FALLBACK",
     FBE_BOARD_TYPE_ARMADA,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_FALLBACK,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_FALLBACK,

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
*/
    {ANDULASIA_TEST_BOXWOOD,
     "BOXWOOD",
     FBE_BOARD_TYPE_ARMADA,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_BOXWOOD,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_BOXWOOD,

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
    {ANDULASIA_TEST_KNOT,
     "KNOT",
     FBE_BOARD_TYPE_ARMADA,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_KNOT,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_KNOT,

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
// Moons
// ENCL_CLEANUP
/*
    {ANDULASIA_TEST_RHEA,
     "RHEA",
     FBE_BOARD_TYPE_ARMADA,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_RHEA,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_RHEA,

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
    {ANDULASIA_TEST_MIRANDA,
     "MIRANDA",
     FBE_BOARD_TYPE_ARMADA,
     FBE_PORT_TYPE_SAS_PMC,
     3,
     5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_MIRANDA,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_MIRANDA,

     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
    },
*/
    /* ENCL_CLEANUP
    {ANDULASIA_TEST_TABASCO,
    "TABASCO",
    FBE_BOARD_TYPE_FLEET,
    FBE_PORT_TYPE_SAS_PMC,
    3,
    5,
    0,
    FBE_SAS_ENCLOSURE_TYPE_TABASCO,
    0,
    FBE_SAS_DRIVE_CHEETA_15K,
    INVALID_DRIVE_NUMBER,
    MAX_DRIVE_COUNT_TABASCO,

    MAX_PORTS_IS_NOT_USED,
    MAX_ENCLS_IS_NOT_USED,
    DRIVE_MASK_IS_NOT_USED,
   },*/

} ;

/*!*******************************************************************
 * @def STATIC_TEST_MAX
 *********************************************************************
 * @brief Number of test scenarios to be executed.
 *********************************************************************/
#define STATIC_TEST_MAX     sizeof(test_table)/sizeof(test_table[0])


/*************************
 *   MACRO DEFINITIONS
 *************************/
#define ZRT_RESUME_PROM_REQUEST_SIZE    (2 * KILOBYTE)
fbe_semaphore_t  resume_semaphore;

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
fbe_status_t andalusia_read_resume_and_verify(fbe_u8_t * pCompareBuffer, fbe_u32_t bufferSize);
fbe_status_t analusia_enclosure_resume_read_async(fbe_object_id_t object_id, 
                                                  fbe_resume_prom_get_resume_async_t *pGetResumeProm);

fbe_status_t andalusia_enclosure_resume_read_async_callback(fbe_packet_completion_context_t context, void *envContext);

/*!*******************************************************************
 * @fn andalusia_enclosure_resume_read_async(
 *     fbe_object_id_t object_id, 
 *     fbe_resume_prom_get_resume_async_t *pGetResumeProm)
 *********************************************************************
 * @brief:
 *  This function read the resume prom in sync manner by calling 
  * async read command to physical package.
 *
 * @param object_id - The object id to send request to
 * @param operation - pointer to the information passed from the the client
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    9-Mar-2010: DP  created
 *    13-Jan-2011: Arun S  Modified to fit in the new Async resume read interface
 *                         and making the scenario more generic.
 *********************************************************************/
fbe_status_t andalusia_enclosure_resume_read_async(fbe_object_id_t object_id, 
                                                   fbe_resume_prom_get_resume_async_t *pGetResumeProm)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;    

    fbe_semaphore_init(&resume_semaphore, 0, FBE_PACKET_STACK_SIZE);

    status = fbe_api_resume_prom_read_async(object_id, pGetResumeProm);

    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING && status != FBE_STATUS_BUSY)
    {
         fbe_semaphore_release(&resume_semaphore, 0, 1, FALSE);
         free(pGetResumeProm);
         return status;
    }
    
    fbe_semaphore_wait(&resume_semaphore, NULL);

    fbe_semaphore_destroy(&resume_semaphore); /* SAFEBUG - needs destroy */

    MUT_ASSERT_TRUE(pGetResumeProm->resumeReadCmd.readOpStatus == FBE_RESUME_PROM_STATUS_READ_SUCCESS);
    
    return status;
} // end andalusia_enclosure_resume_read_async

/*!*******************************************************************
 * @fn andalusia_enclosure_resume_read_async_callback()
 *
 *********************************************************************
 * @brief:
 *  callback function of  andalusia_enclosure_resume_read_async function.
 *
 * @param context - context of structure 
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    9-Mar-2010: DP  created
 *********************************************************************/
fbe_status_t andalusia_enclosure_resume_read_async_callback(fbe_packet_completion_context_t context, void *envContext)                                                                                                            
{
    fbe_semaphore_release(&resume_semaphore, 0, 1, FALSE);

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn  andalusia_run
 ****************************************************************
 * @brief:
 *  Function to read/write the resume prom
 *
 * @return 
 *  FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *   01/27/2008:  Created. ArunKumar Selvapillai
 *
 ****************************************************************/

fbe_status_t andalusia_run(fbe_test_params_t *test)
{
    fbe_u8_t            *tmp;
    fbe_u8_t            *write_buf_p;
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_u32_t           request_size = 0;
    fbe_api_terminator_device_handle_t enclosure_handle;
    fbe_resume_prom_cmd_t                      resume_write_cmd;
    char data_buf[] = "Writing the Resume Prom Data";
    fbe_u32_t           object_handle;


    /*******************    Terminator fills resume prom data ******************/
    mut_printf(MUT_LOG_TEST_STATUS, "Terminator fills resume prom data.");
    write_buf_p = (fbe_u8_t *)malloc(ZRT_RESUME_PROM_REQUEST_SIZE);
    if (write_buf_p == NULL)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s Not enough memory to allocate for write_buf_p", __FUNCTION__);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }    

    /* Fill the dummmy data with inc pattern */
    for (tmp = write_buf_p, request_size=0; 
         request_size < ZRT_RESUME_PROM_REQUEST_SIZE; 
         request_size++)
    {
        *tmp++ = (fbe_u8_t) request_size;
    }
    
    status = fbe_api_terminator_get_enclosure_handle (test->backend_number, test->encl_number, &enclosure_handle);

    
    fbe_api_terminator_set_resume_prom_info(enclosure_handle,
                                            LCC_A_RESUME_PROM,  //Resume Prom ID of the component
                                            write_buf_p,
                                            ZRT_RESUME_PROM_REQUEST_SIZE); // Response buffer size

    /*******************    Read resume and verify ******************/
    andalusia_read_resume_and_verify(write_buf_p, ZRT_RESUME_PROM_REQUEST_SIZE);

    /*******************    Write Resume Prom Data ******************/

    mut_printf(MUT_LOG_TEST_STATUS, "%s Writing the Resume Prom Data", __FUNCTION__);

    fbe_zero_memory(&resume_write_cmd, sizeof(fbe_resume_prom_cmd_t));

    resume_write_cmd.deviceType = FBE_DEVICE_TYPE_LCC;     // resume prom identifier (LCC A)
    resume_write_cmd.deviceLocation.sp = 0;
    resume_write_cmd.deviceLocation.bus = 0;
    resume_write_cmd.deviceLocation.enclosure = 0;
    resume_write_cmd.deviceLocation.slot = 0;
    resume_write_cmd.pBuffer = (fbe_u8_t *)data_buf; 
    resume_write_cmd.bufferSize = sizeof(data_buf); 
    resume_write_cmd.resumePromField = RESUME_PROM_ALL_FIELDS;
    resume_write_cmd.offset = 0;   // start from the first byte.

    /* Get handle to enclosure object */
    status = fbe_api_get_enclosure_object_id_by_location(0, 0, &object_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);

    status = fbe_api_enclosure_resume_write_synch(object_handle, &resume_write_cmd);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /************    Read Resume Prom Data to verify the data has been written correctly **********/

    andalusia_read_resume_and_verify(data_buf, sizeof(data_buf));

    mut_printf(MUT_LOG_TEST_STATUS, "%s Resume Prom Data successfully written", __FUNCTION__);

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn  andalusia_read_resume_and_verify
 ****************************************************************
 * @brief:
 *  Function to read the resume prom and compare with the buffer passed in.
 *
 * @return 
 *  FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *   01/27/2008:  Created. ArunKumar Selvapillai
 *
 ****************************************************************/

fbe_status_t andalusia_read_resume_and_verify(fbe_u8_t * pCompareBuffer, fbe_u32_t bufferSize)
{
    fbe_u8_t            *read_buf_p;
    fbe_u32_t           object_handle;
    fbe_u8_t            is_equal;
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_resume_prom_get_resume_async_t      *pGetResumeProm = NULL;

    pGetResumeProm = (fbe_resume_prom_get_resume_async_t *) malloc (sizeof (fbe_resume_prom_get_resume_async_t));
    if(pGetResumeProm == NULL)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s Not enough memory to allocate for pGetResumeProm", __FUNCTION__);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Allocate the read & write buffer */
    read_buf_p = (fbe_u8_t *)malloc(bufferSize);
    if (read_buf_p == NULL)
    {
        free(pGetResumeProm); /* Free the pGetResumeProm buffer if read buffer is NULL */
        mut_printf(MUT_LOG_TEST_STATUS, "%s Not enough memory to allocate for read_buf_p", __FUNCTION__);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    fbe_zero_memory(read_buf_p, bufferSize);
    
    pGetResumeProm->resumeReadCmd.deviceType = FBE_DEVICE_TYPE_LCC;      // Input
    pGetResumeProm->resumeReadCmd.deviceLocation.sp = SP_A;
    pGetResumeProm->resumeReadCmd.deviceLocation.enclosure = 0;
    pGetResumeProm->resumeReadCmd.deviceLocation.port = 0;
    pGetResumeProm->resumeReadCmd.deviceLocation.slot = 0;
    pGetResumeProm->resumeReadCmd.deviceLocation.bus = 0;
    pGetResumeProm->resumeReadCmd.resumePromField = RESUME_PROM_ALL_FIELDS;
    pGetResumeProm->resumeReadCmd.offset = 0;
    pGetResumeProm->resumeReadCmd.pBuffer = read_buf_p;
    pGetResumeProm->resumeReadCmd.bufferSize = bufferSize;

    /* Get handle to enclosure object */
    status = fbe_api_get_enclosure_object_id_by_location(0, 0, &object_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);

    mut_printf(MUT_LOG_TEST_STATUS, "%s Sending Synchronous Resume Read", __FUNCTION__);

    /* Get the Resume Info - Synchronous */
    status = fbe_api_resume_prom_read_sync(object_handle, &pGetResumeProm->resumeReadCmd);    
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Check if the read data matches the fill data */
// ENCL_CLEANUP
/*
    mut_printf(MUT_LOG_TEST_STATUS, "cmpBf 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", 
               *pCompareBuffer,
               *(pCompareBuffer+1),
               *(pCompareBuffer+2),
               *(pCompareBuffer+3),
               *(pCompareBuffer+4),
               *(pCompareBuffer+5),
               *(pCompareBuffer+6),
               *(pCompareBuffer+7));
    mut_printf(MUT_LOG_TEST_STATUS, "pBf 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", 
               *pGetResumeProm->resumeReadCmd.pBuffer,
               *(pGetResumeProm->resumeReadCmd.pBuffer+1),
               *(pGetResumeProm->resumeReadCmd.pBuffer+2),
               *(pGetResumeProm->resumeReadCmd.pBuffer+3),
               *(pGetResumeProm->resumeReadCmd.pBuffer+4),
               *(pGetResumeProm->resumeReadCmd.pBuffer+5),
               *(pGetResumeProm->resumeReadCmd.pBuffer+6),
               *(pGetResumeProm->resumeReadCmd.pBuffer+7));
*/
    is_equal = memcmp(pCompareBuffer, pGetResumeProm->resumeReadCmd.pBuffer, pGetResumeProm->resumeReadCmd.bufferSize);
    MUT_ASSERT_INT_EQUAL(0, is_equal);

    mut_printf(MUT_LOG_TEST_STATUS, "%s Resume Prom Data verified for Synchronous Resume Read.", __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, "%s Sending Asynchronous Resume Read", __FUNCTION__);

    memset(pGetResumeProm->resumeReadCmd.pBuffer, 0, pGetResumeProm->resumeReadCmd.bufferSize);
    pGetResumeProm->resumeReadCmd.readOpStatus = FBE_RESUME_PROM_STATUS_INVALID;

    pGetResumeProm->completion_function = andalusia_enclosure_resume_read_async_callback;
    pGetResumeProm->completion_context = NULL;

    /* Get the Resume Info - Asynchronous */
    status = andalusia_enclosure_resume_read_async(object_handle, pGetResumeProm);
    MUT_ASSERT_TRUE((status == FBE_STATUS_OK) || (status == FBE_STATUS_PENDING) || (status == FBE_STATUS_BUSY));

    /* Check id the read data matches the fill data */
    is_equal = memcmp(pCompareBuffer, pGetResumeProm->resumeReadCmd.pBuffer, pGetResumeProm->resumeReadCmd.bufferSize);
    MUT_ASSERT_INT_EQUAL(0, is_equal);

    mut_printf(MUT_LOG_TEST_STATUS, "%s Resume Prom Data verified for Asynchronous Resume Read.", __FUNCTION__);

    free(read_buf_p);
    free(pGetResumeProm);
    return FBE_STATUS_OK;
}


/*!***************************************************************
 * @fn andalusia()
 ****************************************************************
 * @brief:
 *  Function to load the config and run the Andalusia scenario.
 *
 * @return 
 *  FBE_STATUS_OK if no errors.
 *
 * HISTORY:
 *   01/27/2008:  Created. ArunKumar Selvapillai
 *   02/8/2011 - Updated to support atble driven approach
 ****************************************************************/

void andalusia(void)
{
    fbe_status_t run_status = FBE_STATUS_OK;
    fbe_u32_t test_num;

    for(test_num = 0; test_num < STATIC_TEST_MAX; test_num++)
    {
        /* Load the configuration for test
         */
        maui_load_and_verify_table_driven(&test_table[test_num]);
        run_status = andalusia_run(&test_table[test_num]);
    MUT_ASSERT_TRUE(run_status == FBE_STATUS_OK);

        /* Unload the configuration
        */
        fbe_test_physical_package_tests_config_unload();
    }
}

