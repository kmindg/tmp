/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
 
/*!**************************************************************************
 * @file ravana_peer_boot_test.c
 ***************************************************************************
 *
 * @brief
 *  Verify Peer Boot Logging functionality of board_mgmt object.
 * 
 * @version
 *   24-Jan-2011 - Created. Vaibhav Gaonkar
 *
 ***************************************************************************/
 /*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "mut.h"
#include "fbe_test_configurations.h"
#include "esp_tests.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_esp_board_mgmt_interface.h"
#include "fbe/fbe_api_esp_common_interface.h"
#include "fbe/fbe_event_log_utils.h"
#include "spid_types.h"
#include "fbe/fbe_devices.h"
#include "physical_package_tests.h"
#include "fbe/fbe_api_resume_prom_interface.h"
#include "fbe/fbe_peer_boot_utils.h"
#include "fbe/fbe_api_esp_ps_mgmt_interface.h"
#include "fbe/fbe_api_esp_module_mgmt_interface.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "fbe_test_esp_utils.h"
#include "generic_utils_lib.h"
#include "fbe_base_environment_debug.h"

#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"

#define RAVANA_PEER_SP_ID           SP_B
#define RAVANA_FLT_REG_ID           PRIMARY_FLT_REG
#define FBE_API_POLLING_INTERVAL    5000 /*ms*/
#define RAVANA_FLT_REG_FRU_TYPE     17
#define RAVANA_FLT_REG_MAX_SLOT     64
#define RAVANA_FLT_REG_TIMEOUT      180000 /*ms*/
#define RAVANA_FLT_REG_RETRY        10
/*************************
 *   TYPE DEFINITIONS
 *************************/
typedef enum {
    RAVANA_PLATFORM_UNKNOWN,
    RAVANA_PLATFORM_MEGATRON,
    RAVANA_PLATFORM_JETFIRE,
}ravana_platform_type_t;

typedef enum {
    RAVANA_FLTREG_INVALID_FAULT     = 0x0000,
    RAVANA_FLTREG_DIMM_FAULT        = 0x0001,
    RAVANA_FLTREG_DRIVE_FAULT       = 0x0002,    
    RAVANA_FLTREG_SLIC_FAULT        = 0x0004,
    RAVANA_FLTREG_POWER_FAULT       = 0x0008,    
    RAVANA_FLTREG_BATTERY_FAULT     = 0x0010,    
    RAVANA_FLTREG_SUPERCAP_FAULT    = 0x0020,    
    RAVANA_FLTREG_FAN_FAULT         = 0x0040,    
    RAVANA_FLTREG_I2C_FAULT         = 0x0080,    
    RAVANA_FLTREG_CPU_FAULT         = 0x0100,    
    RAVANA_FLTREG_MGMT_FAULT        = 0x0200,    
    RAVANA_FLTREG_BEM_FAULT         = 0x0400,    
    RAVANA_FLTREG_EFLASH_FAULT      = 0x0800,    
    RAVANA_FLTREG_EHORNET_FAULT     = 0x1000,    
    RAVANA_FLTREG_MIDPLANE_FAULT    = 0x2000,    
    RAVANA_FLTREG_CMI_FAULT         = 0x4000,    
    RAVANA_FLTREG_ALLFRUS_FAULT     = 0x8000,    
    RAVANA_FLTREG_EXTERNALFRU_FAULT =0x10000,    
}ravana_fault_reg_fru_type_t;

typedef struct ravana_fault_reg_fru_s {
    ravana_fault_reg_fru_type_t     fruType;
    fbe_u8_t                        slots[RAVANA_FLT_REG_MAX_SLOT];
    fbe_u8_t                        slotCount;
}ravana_rault_reg_fru_t;

typedef struct ravana_fault_reg_test_s {
    ravana_rault_reg_fru_t          faultFrus[RAVANA_FLT_REG_FRU_TYPE];
    fbe_u8_t                        fruFaults;
    fbe_u32_t                       cpuStatus;
}ravana_fault_reg_test_t;

/*************************
 *   GLOBAL DEFINITIONS
 *************************/
char * ravana_fault_reg_megatron_short_desc = "Test for Peer Boot Log(Fault register) functionality on Megatron platform";
char * ravana_fault_reg_jetfire_short_desc = "Test for Peer Boot Log(Fault register) functionality on Jetfire platform";
char * ravana_fault_reg_long_desc ="\
This test validates the Peer Boot Logging(Fault register) by explicitly modifying the fault register status in SPECL \n\
\n\
\n\
STEP 1: Bring up the initial topology for dual SPs.\n\
STEP 2: Validate the Peer Boot Logging ";

fbe_object_id_t             flt_reg_expected_objectId;
fbe_u64_t           flt_reg_device_mask = FBE_DEVICE_TYPE_INVALID;

static ravana_platform_type_t   platformType = RAVANA_PLATFORM_UNKNOWN;
static fbe_semaphore_t          flt_reg_sem;
static fbe_bool_t               isFmea = FBE_FALSE;
static SPECL_SFI_MASK_DATA      orig_sfi_mask_data = {0,};

/*************************
 *   FUNCTION PROTOTYPES
 *************************/
static void ravana_fault_reg_setup(void);
static void ravana_peer_boot_fault_reg_test(void);
static void ravana_fault_reg_test_boot_state(void);
static void ravana_fault_reg_log_test(ravana_fault_reg_test_t * faultRegTest);
static void ravana_fault_reg_set_reg(ravana_fault_reg_test_t        * faultRegTest,
                                     SPECL_FAULT_REGISTER_STATUS    * faultRegStatus);
static fbe_bool_t ravana_check_fault_reg_log_message(ravana_fault_reg_test_t * faultRegTest);
static fbe_bool_t ravana_check_event_boot_state(ravana_fault_reg_test_t * faultRegTest);
static fbe_bool_t ravana_check_event_with_fail(ravana_fault_reg_test_t * faultRegTest,
                                               fbe_u8_t                  faultType,
                                               const char              * faultStr);
static fbe_bool_t ravana_check_event_with_fru(ravana_fault_reg_test_t * faultRegTest,
                                              const char              * faultStr);
static fbe_bool_t ravana_check_event_with_slot(ravana_fault_reg_test_t * faultRegTest,
                                               fbe_u8_t                  faultType,
                                               const char              * faultStr);
static fbe_bool_t ravana_check_event_with_part_number(fbe_u64_t         deviceType,
                                                      ravana_fault_reg_test_t * faultRegTest,
                                                      fbe_u8_t                  faultType,
                                                      const char              * faultStr);
static fbe_bool_t ravana_check_event_with_slot_part_number(fbe_u64_t         deviceType,
                                                           ravana_fault_reg_test_t * faultRegTest,
                                                           fbe_u8_t                  faultType,
                                                           const char              * faultStr);
static fbe_bool_t ravana_check_slic_event(ravana_fault_reg_test_t * faultRegTest,
                                          fbe_u8_t                  typeIndex);
static fbe_bool_t ravana_check_mgmt_event(void);
static fbe_bool_t ravana_check_power_event(ravana_fault_reg_test_t * faultRegTest,
                                           fbe_u8_t                  typeIndex);
static fbe_bool_t ravana_check_fan_event(ravana_fault_reg_test_t * faultRegTest,
                                           fbe_u8_t                  typeIndex);
static fbe_status_t ravana_fault_reg_check_event(ravana_fault_reg_test_t * faultRegTest,
                                                 fbe_u32_t retryCount);
static void ravana_fault_reg_check_board_mgmt(SPECL_FAULT_REGISTER_STATUS   * faultRegStatus);
static void ravana_fault_reg_add_test(ravana_fault_reg_test_t     * faultRegTest,
                                      ravana_fault_reg_fru_type_t      faultType,
                                      fbe_u8_t                         slotNum);
static void ravana_get_tla_num(fbe_u8_t             slot,
                               fbe_u8_t             sp,
                               fbe_u64_t    device_type,
                               char                *tla_num,
                               fbe_u32_t            len);
static fbe_status_t ravana_get_specl_flt_reg(PSPECL_SFI_MASK_DATA  sfi_mask_data);
static fbe_status_t ravana_set_specl_flt_reg(PSPECL_SFI_MASK_DATA sfi_mask_data);
static void ravana_esp_object_data_change_callback_function(update_object_msg_t * update_object_msg, 
                                                            void *context);
static void ravana_fault_reg_test(void);

/*************************
 *   FUNCTION DEFINITIONS
 ************************/
/*!**************************************************************
 *  ravana_fault_reg_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the ravana peer boot fault register test
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  8-Aug-2012 - Chengkai Hu - Created
 *
 ****************************************************************/
static void ravana_fault_reg_test(void)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;  
    fbe_object_id_t object_id = FBE_OBJECT_ID_INVALID;
    fbe_notification_registration_id_t reg_id;

    mut_printf(MUT_LOG_LOW, "Ravana Peer Boot Log(Fault register) test: Started...\n");

    /* Update fema flag */
    if (fbe_test_esp_util_get_cmd_option("-fmea")) 
    {
        isFmea = FBE_TRUE;
    }
    else
    {
        isFmea = FBE_FALSE;
    }
  
	/* Init the Semaphore */
    fbe_semaphore_init(&flt_reg_sem, 0, 1);

    /* Get the original flt reg status from SPECL */
    status = ravana_get_specl_flt_reg(&orig_sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    /* Register to get event notification from ESP */
    status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
                                                                  FBE_PACKAGE_NOTIFICATION_ID_ESP,
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT,
                                                                  ravana_esp_object_data_change_callback_function,
                                                                  &flt_reg_sem,
                                                                  &reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_BOARD_MGMT, &object_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_id != FBE_OBJECT_ID_INVALID);
    flt_reg_expected_objectId = object_id;

    mut_printf(MUT_LOG_LOW, "Ravana test: Started Peer Boot Log(Fault register) test...\n");
    flt_reg_device_mask = FBE_DEVICE_TYPE_FLT_REG;
    /* Test the peer Boot logging */
    ravana_peer_boot_fault_reg_test();
    flt_reg_device_mask = FBE_DEVICE_TYPE_INVALID;
    mut_printf(MUT_LOG_LOW, "Ravana test: Peer Boot Log(Fault register) test passed successfully ...\n");

    /* Unregistered the notification*/
    status = fbe_api_notification_interface_unregister_notification(ravana_esp_object_data_change_callback_function,
                                                                    reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_destroy(&flt_reg_sem);
}
/**********************************
 *  end of ravana_fault_reg_test()
 *********************************/ 
/*!**************************************************************
 *  ravana_fault_reg_mt_test() 
 ****************************************************************
 * @brief
 *  dummy Megatron entry to let cruisecontrol passed with individual test name.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  15-Jan-2013 - Chengkai Hu - Created
 *
 ****************************************************************/
void ravana_fault_reg_mt_test(void)
{
    ravana_fault_reg_test();
}
/**********************************
 *  end of ravana_fault_reg__mt_test()
 *********************************/ 
/*!**************************************************************
 *  ravana_fault_reg_jf_test() 
 ****************************************************************
 * @brief
 *  dummy Jetfire entry to let cruisecontrol passed with individual test name.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  15-Jan-2013 - Chengkai Hu - Created
 *
 ****************************************************************/
void ravana_fault_reg_jf_test(void)
{
    ravana_fault_reg_test();
}
/**********************************
 *  end of ravana_fault_reg__jf_test()
 *********************************/ 
/*!**************************************************************
*  ravana_peer_boot_fault_reg_test() 
****************************************************************
* @brief
*  This function start to validate the Peer Boot fault register 
*
* @param None.
*
* @return None.
*
* @author
*  8-Aug-2012 Chengkai Hu - Created. 
*
****************************************************************/
static void 
ravana_peer_boot_fault_reg_test(void)
{
    ravana_fault_reg_test_t faultRegTest = {0,};

    /* Boot state test */
    ravana_fault_reg_test_boot_state();

    /* IO module fault test (max slot refer to MAX_SLIC_FAULT) */
    //One slot fault test
    memset(&faultRegTest, 0, sizeof(ravana_fault_reg_test_t));
    ravana_fault_reg_add_test(&faultRegTest, RAVANA_FLTREG_SLIC_FAULT, 1);
    faultRegTest.cpuStatus = CSR_POST_IO_MODULE_1;
    ravana_fault_reg_log_test(&faultRegTest);
    fbe_api_sleep(10000);
    
    //Multiple slots fault test
    memset(&faultRegTest, 0, sizeof(ravana_fault_reg_test_t));
    ravana_fault_reg_add_test(&faultRegTest, RAVANA_FLTREG_SLIC_FAULT, 1);
    ravana_fault_reg_add_test(&faultRegTest, RAVANA_FLTREG_SLIC_FAULT, 3);
    ravana_fault_reg_add_test(&faultRegTest, RAVANA_FLTREG_SLIC_FAULT, 4);
    if(RAVANA_PLATFORM_JETFIRE != platformType)
    {
        ravana_fault_reg_add_test(&faultRegTest, RAVANA_FLTREG_SLIC_FAULT, 6);
        faultRegTest.cpuStatus = CSR_POST_IO_MODULE_6;
    }
    else
    {
        faultRegTest.cpuStatus = CSR_POST_IO_MODULE_4;
    }
    ravana_fault_reg_log_test(&faultRegTest);
    fbe_api_sleep(15000);
   
    /* Management module fault test */
    //One slot fault test
    memset(&faultRegTest, 0, sizeof(ravana_fault_reg_test_t));
    ravana_fault_reg_add_test(&faultRegTest, RAVANA_FLTREG_MGMT_FAULT, 0);
    faultRegTest.cpuStatus = CSR_POST_MM_MODULE_B;
    ravana_fault_reg_log_test(&faultRegTest);
    fbe_api_sleep(5000);

    /* Power supply fault test (max slot refer to MAX_POWER_FAULT) */
    //One slot fault test
    memset(&faultRegTest, 0, sizeof(ravana_fault_reg_test_t));
    ravana_fault_reg_add_test(&faultRegTest, RAVANA_FLTREG_POWER_FAULT, 0);
    faultRegTest.cpuStatus = CSR_POST_PS_1;
    ravana_fault_reg_log_test(&faultRegTest);
    fbe_api_sleep(5000);

    //Multiple slots fault test
    if(RAVANA_PLATFORM_JETFIRE != platformType)
    {
        memset(&faultRegTest, 0, sizeof(ravana_fault_reg_test_t));
        ravana_fault_reg_add_test(&faultRegTest, RAVANA_FLTREG_POWER_FAULT, 0);
        ravana_fault_reg_add_test(&faultRegTest, RAVANA_FLTREG_POWER_FAULT, 1);
        faultRegTest.cpuStatus = CSR_POST_PS_1;
        ravana_fault_reg_log_test(&faultRegTest);
        fbe_api_sleep(15000);
    }

    /* Fan fault test */
    memset(&faultRegTest, 0, sizeof(ravana_fault_reg_test_t));
    ravana_fault_reg_add_test(&faultRegTest, RAVANA_FLTREG_FAN_FAULT, 0);
    ravana_fault_reg_add_test(&faultRegTest, RAVANA_FLTREG_FAN_FAULT, 1);
    faultRegTest.cpuStatus = CSR_POST_END;
    ravana_fault_reg_log_test(&faultRegTest);
    fbe_api_sleep(5000);

    if (isFmea)
    {
        /* DIMM fault test (max slot refer to MAX_DIMM_FAULT)*/
        //One slot fault test
        memset(&faultRegTest, 0, sizeof(ravana_fault_reg_test_t));
        ravana_fault_reg_add_test(&faultRegTest, RAVANA_FLTREG_DIMM_FAULT, 5);
        faultRegTest.cpuStatus = CSR_POST_MEMORY;
        ravana_fault_reg_log_test(&faultRegTest);
        fbe_api_sleep(5000);
        
        //Multiple slots fault test
        memset(&faultRegTest, 0, sizeof(ravana_fault_reg_test_t));
        ravana_fault_reg_add_test(&faultRegTest, RAVANA_FLTREG_DIMM_FAULT, 10);
        ravana_fault_reg_add_test(&faultRegTest, RAVANA_FLTREG_DIMM_FAULT, 15);
        ravana_fault_reg_add_test(&faultRegTest, RAVANA_FLTREG_DIMM_FAULT, 3);
        faultRegTest.cpuStatus = CSR_POST_MEMORY;
        ravana_fault_reg_log_test(&faultRegTest);
        fbe_api_sleep(10000);

        /* DRIVE fault test (max slot refer to MAX_DRIVE_FAULT)*/
        //One slot fault test
        memset(&faultRegTest, 0, sizeof(ravana_fault_reg_test_t));
        ravana_fault_reg_add_test(&faultRegTest, RAVANA_FLTREG_DRIVE_FAULT, 3);
        faultRegTest.cpuStatus = CSR_POST_DISK_3;
        ravana_fault_reg_log_test(&faultRegTest);
        fbe_api_sleep(5000);
        
        //Multiple slots fault test
        memset(&faultRegTest, 0, sizeof(ravana_fault_reg_test_t));
        ravana_fault_reg_add_test(&faultRegTest, RAVANA_FLTREG_DRIVE_FAULT, 1);
        ravana_fault_reg_add_test(&faultRegTest, RAVANA_FLTREG_DRIVE_FAULT, 3);
        faultRegTest.cpuStatus = CSR_POST_DISK_3;
        ravana_fault_reg_log_test(&faultRegTest);
        fbe_api_sleep(10000);

        if(RAVANA_PLATFORM_JETFIRE == platformType)
        {
            /* Battery fault test */
            memset(&faultRegTest, 0, sizeof(ravana_fault_reg_test_t));
            ravana_fault_reg_add_test(&faultRegTest, RAVANA_FLTREG_BATTERY_FAULT, 0);
            faultRegTest.cpuStatus = CSR_POST_BBU_B;
            ravana_fault_reg_log_test(&faultRegTest);
            fbe_api_sleep(5000);
        }

        /* SuperCap fault test */
        memset(&faultRegTest, 0, sizeof(ravana_fault_reg_test_t));
        ravana_fault_reg_add_test(&faultRegTest, RAVANA_FLTREG_SUPERCAP_FAULT, 0);
        faultRegTest.cpuStatus = CSR_POST_END;
        ravana_fault_reg_log_test(&faultRegTest);
        fbe_api_sleep(5000);

        /* I2C fault test (max slot refer to MAX_I2C_FAULT)*/
        memset(&faultRegTest, 0, sizeof(ravana_fault_reg_test_t));
        ravana_fault_reg_add_test(&faultRegTest, RAVANA_FLTREG_I2C_FAULT, 0);
        ravana_fault_reg_add_test(&faultRegTest, RAVANA_FLTREG_I2C_FAULT, 4);
        ravana_fault_reg_add_test(&faultRegTest, RAVANA_FLTREG_I2C_FAULT, 5);
        faultRegTest.cpuStatus = CSR_POST_END;
        ravana_fault_reg_log_test(&faultRegTest);
        fbe_api_sleep(15000);

        /* CPU fault test */
        memset(&faultRegTest, 0, sizeof(ravana_fault_reg_test_t));
        ravana_fault_reg_add_test(&faultRegTest, RAVANA_FLTREG_CPU_FAULT, 0);
        faultRegTest.cpuStatus = CSR_POST_START;
        ravana_fault_reg_log_test(&faultRegTest);
        fbe_api_sleep(5000);

        /* BEM only on Jetfire platform */
        if(RAVANA_PLATFORM_JETFIRE == platformType)
        {
            /* BEM fault test */
            memset(&faultRegTest, 0, sizeof(ravana_fault_reg_test_t));
            ravana_fault_reg_add_test(&faultRegTest, RAVANA_FLTREG_BEM_FAULT, 0);
            faultRegTest.cpuStatus = CSR_POST_BACK_END_MODULE;
            ravana_fault_reg_log_test(&faultRegTest);
            fbe_api_sleep(5000);
        }

        /* eFlash fault test */
        memset(&faultRegTest, 0, sizeof(ravana_fault_reg_test_t));
        ravana_fault_reg_add_test(&faultRegTest, RAVANA_FLTREG_EFLASH_FAULT, 0);
        faultRegTest.cpuStatus = CSR_POST_SW_IMAGE_CORRUPT;
        ravana_fault_reg_log_test(&faultRegTest);
        fbe_api_sleep(5000);

        /* eHornet fault test */
        memset(&faultRegTest, 0, sizeof(ravana_fault_reg_test_t));
        ravana_fault_reg_add_test(&faultRegTest, RAVANA_FLTREG_EHORNET_FAULT, 0);
        faultRegTest.cpuStatus = CSR_POST_EHORNET;
        ravana_fault_reg_log_test(&faultRegTest);
        fbe_api_sleep(5000);

        /* midPlane fault test */
        memset(&faultRegTest, 0, sizeof(ravana_fault_reg_test_t));
        ravana_fault_reg_add_test(&faultRegTest, RAVANA_FLTREG_MIDPLANE_FAULT, 0);
        faultRegTest.cpuStatus = CSR_POST_MOTHERBOARD;
        ravana_fault_reg_log_test(&faultRegTest);
        fbe_api_sleep(5000);

        /* CMI fault test */
        memset(&faultRegTest, 0, sizeof(ravana_fault_reg_test_t));
        ravana_fault_reg_add_test(&faultRegTest, RAVANA_FLTREG_CMI_FAULT, 0);
        faultRegTest.cpuStatus = CSR_POST_END;
        ravana_fault_reg_log_test(&faultRegTest);
        fbe_api_sleep(5000);

        /* AllFrus fault test */
        memset(&faultRegTest, 0, sizeof(ravana_fault_reg_test_t));
        ravana_fault_reg_add_test(&faultRegTest, RAVANA_FLTREG_ALLFRUS_FAULT, 0);
        faultRegTest.cpuStatus = CSR_POST_END;
        ravana_fault_reg_log_test(&faultRegTest);
        fbe_api_sleep(5000);

        /* ExternalFru fault test */
        memset(&faultRegTest, 0, sizeof(ravana_fault_reg_test_t));
        ravana_fault_reg_add_test(&faultRegTest, RAVANA_FLTREG_EXTERNALFRU_FAULT, 0);
        faultRegTest.cpuStatus = CSR_POST_END;
        ravana_fault_reg_log_test(&faultRegTest);
        fbe_api_sleep(5000);
        
    }//if(isFmea)
}
/***************************************
 *  end of ravana_peer_boot_fault_reg_test()
 ***************************************/
/*!**************************************************************
 *  ravana_fault_reg_test_boot_state() 
 ****************************************************************
 * @brief
 *  This function includes boot state test cases.
 *  Test result based on fbe_translatePeerBootStatus()
 *
 * @param faultRegTest - handle to test data.
 *
 * @return None.
 *
 * @author
 *  15-Aug-2012 Chengkai Hu - Created
 *
 ****************************************************************/
static void
ravana_fault_reg_test_boot_state(void)
{
    ravana_fault_reg_test_t     faultRegTest = {0,};

    memset(&faultRegTest, 0, sizeof(ravana_fault_reg_test_t));
    faultRegTest.cpuStatus = CSR_OS_DEGRADED_MODE;
    ravana_fault_reg_log_test(&faultRegTest);
    fbe_api_sleep(5000);

    memset(&faultRegTest, 0, sizeof(ravana_fault_reg_test_t));
    faultRegTest.cpuStatus = CSR_OS_APPLICATION_RUNNING;
    ravana_fault_reg_log_test(&faultRegTest);
    fbe_api_sleep(5000);

    memset(&faultRegTest, 0, sizeof(ravana_fault_reg_test_t));
    faultRegTest.cpuStatus = CSR_OS_BLADE_BEING_SERVICED;
    ravana_fault_reg_log_test(&faultRegTest);
    fbe_api_sleep(5000);

}
/***************************************
 *  end of ravana_fault_reg_test_boot_state()
 ***************************************/
/*!**************************************************************
 *  ravana_fault_reg_log_test() 
 ****************************************************************
 * @brief
 *  This function update fault register data in SPECL sfi, and 
 *  validate log.
 *
 * @param faultRegTest - handle to test data.
 *
 * @return None.
 *
 * @author
 *  08-Aug-2012 Chengkai Hu - Created
 *
 ****************************************************************/
static void
ravana_fault_reg_log_test(ravana_fault_reg_test_t * faultRegTest)
{
    DWORD                           dwWaitResult;
    fbe_status_t                    status;
    SPECL_SFI_MASK_DATA             sfi_mask_data;
    SPECL_FAULT_REGISTER_STATUS   * faultRegStatus = NULL;

    /* - Set the fault expander with bootCode
     * - Check event log for respective bootCode message
     */
    /* Get the original flt reg status */
    memcpy(&sfi_mask_data, &orig_sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));

    /* Update fault register with test data */
    faultRegStatus = &sfi_mask_data.sfiSummaryUnions.fltExpStatus.fltExpSummary[RAVANA_PEER_SP_ID].faultRegisterStatus[RAVANA_FLT_REG_ID];
    ravana_fault_reg_set_reg(faultRegTest, faultRegStatus);

    /* Set the flt Reg status to SPECL */
    status = ravana_set_specl_flt_reg(&sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Wait for notification from ESP, 
     * that is bootCode get updated in board_mgmt object
     */
    mut_printf(MUT_LOG_LOW, "Ravana test(fault register): Waiting for fault register update in board_mgmt object...\n");
    dwWaitResult = fbe_semaphore_wait_ms(&flt_reg_sem, RAVANA_FLT_REG_TIMEOUT);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    mut_printf(MUT_LOG_LOW, "Ravana test(fault register): Waiting for check fault register event...\n");
    status = ravana_fault_reg_check_event(faultRegTest, RAVANA_FLT_REG_RETRY);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "Ravana test(fault register): Checking peer boot info in board mgmt...\n");
    ravana_fault_reg_check_board_mgmt(faultRegStatus);

    /* Restore flt reg satus with original info */
    status = ravana_set_specl_flt_reg(&orig_sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "Ravana test(fault register): Verified successfully for CPU status 0x%x, CPU status Description: %s.\n",
                            faultRegStatus->cpuStatusRegister,
                            decodeCpuStatusRegister(faultRegStatus->cpuStatusRegister));
}
/***************************************
 *  end of ravana_fault_reg_log_test()
 ***************************************/
/*!**************************************************************
 *  ravana_fault_reg_set_reg() 
 ****************************************************************
 * @brief
 *  This function validate the boot Code which have timer value.
 *  This function update the SPECL fault expander value with boot code
 *  1) It change the bootCode value before timer expired and check event log
 *  2) It wait for timer to expired and check the event log
 *
 * @param bootEntry - handle to fbe_esp_board_mgmt_peerBootEntry_t
 *
 * @return None.
 *
 * @author
 *  08-Aug-2012 Chengkai Hu - Created
 *
 ****************************************************************/
static void
ravana_fault_reg_set_reg(ravana_fault_reg_test_t        * faultRegTest,
                         SPECL_FAULT_REGISTER_STATUS    * faultRegStatus)
{
    fbe_u8_t    type = 0;
    fbe_u8_t    slot = 0;

    faultRegStatus->cpuStatusRegister = faultRegTest->cpuStatus;
    mut_printf(MUT_LOG_LOW, "Ravana test: Verifying CPU status 0x%x, CPU status Description: %s.\n",
                            faultRegStatus->cpuStatusRegister,
                            decodeCpuStatusRegister(faultRegStatus->cpuStatusRegister));
    
    for(type = 0;type < faultRegTest->fruFaults; type++)
    {
        switch(faultRegTest->faultFrus[type].fruType)
        {
            case RAVANA_FLTREG_DIMM_FAULT:
                for(slot = 0;slot < faultRegTest->faultFrus[type].slotCount;slot++)
                {
                    faultRegStatus->dimmFault[faultRegTest->faultFrus[type].slots[slot]] = TRUE;
                }
            break;
            case RAVANA_FLTREG_DRIVE_FAULT:
                for(slot = 0;slot < faultRegTest->faultFrus[type].slotCount;slot++)
                {
                    faultRegStatus->driveFault[faultRegTest->faultFrus[type].slots[slot]] = TRUE;
                }
            break;
            case RAVANA_FLTREG_SLIC_FAULT:
                for(slot = 0;slot < faultRegTest->faultFrus[type].slotCount;slot++)
                {
                    faultRegStatus->slicFault[faultRegTest->faultFrus[type].slots[slot]] = TRUE;
                }
            break;
            case RAVANA_FLTREG_POWER_FAULT:
                for(slot = 0;slot < faultRegTest->faultFrus[type].slotCount;slot++)
                {
                    faultRegStatus->powerFault[faultRegTest->faultFrus[type].slots[slot]] = TRUE;
                }
            break;
            case RAVANA_FLTREG_BATTERY_FAULT:
                for(slot = 0;slot < faultRegTest->faultFrus[type].slotCount;slot++)
                {
                    faultRegStatus->batteryFault[faultRegTest->faultFrus[type].slots[slot]] = TRUE;
                }
            break;
            case RAVANA_FLTREG_SUPERCAP_FAULT:
                faultRegStatus->superCapFault = TRUE;
            break;
            case RAVANA_FLTREG_FAN_FAULT:
                for(slot = 0;slot < faultRegTest->faultFrus[type].slotCount;slot++)
                {
                    faultRegStatus->fanFault[faultRegTest->faultFrus[type].slots[slot]] = TRUE;
                }
            break;
            case RAVANA_FLTREG_I2C_FAULT:
                for(slot = 0;slot < faultRegTest->faultFrus[type].slotCount;slot++)
                {
                    faultRegStatus->i2cFault[faultRegTest->faultFrus[type].slots[slot]] = TRUE;
                }
            break;
            case RAVANA_FLTREG_CPU_FAULT:
                faultRegStatus->cpuFault = TRUE;
            break;
            case RAVANA_FLTREG_MGMT_FAULT:
                faultRegStatus->mgmtFault = TRUE;
            break;
            case RAVANA_FLTREG_BEM_FAULT:
                faultRegStatus->bemFault = TRUE;
            break;
            case RAVANA_FLTREG_EFLASH_FAULT:
                faultRegStatus->eFlashFault = TRUE;
            break;
            case RAVANA_FLTREG_EHORNET_FAULT:
                faultRegStatus->cacheCardFault = TRUE;
            break;
            case RAVANA_FLTREG_MIDPLANE_FAULT:
                faultRegStatus->midplaneFault = TRUE;
            break;
            case RAVANA_FLTREG_CMI_FAULT:
                faultRegStatus->cmiFault = TRUE;
            break;
            case RAVANA_FLTREG_ALLFRUS_FAULT:
                faultRegStatus->allFrusFault = TRUE;
            break;
            case RAVANA_FLTREG_EXTERNALFRU_FAULT:
                faultRegStatus->externalFruFault = TRUE;
            break;
            default:
                mut_printf(MUT_LOG_LOW, "Ravana test: Unknow fault type %d, please check.",
                    faultRegTest->faultFrus[type].fruType);
            break;
        }
    }
    /* HCK TODO: this should be removed while SPECL set it automatically */
    if(faultRegTest->fruFaults > 0)
    {
        faultRegStatus->anyFaultsFound = TRUE;
    }

}
/*************************************************
 *  end of ravana_fault_reg_set_reg()
 ************************************************/
 /*!**************************************************************
 *  ravana_check_fault_reg_log_message() 
 ****************************************************************
 * @brief
 *  This function return whether particular message present in 
 *  event_log or not 
 *
 * @param bootEntry - handle to fbe_esp_board_mgmt_peerBootEntry_t
 *
 * @return None.
 *
 * @author
 *  08-Aug-2012 Chengkai Hu - Created
 *
 ****************************************************************/
static fbe_bool_t
ravana_check_fault_reg_log_message(ravana_fault_reg_test_t * faultRegTest)
{
	fbe_u8_t                    tla_num[RESUME_PROM_EMC_TLA_PART_NUM_SIZE+1];
    fbe_bool_t                  isMsgPresent = FALSE;
    fbe_u8_t                    type = 0;
    SPECL_FAULT_REGISTER_STATUS faultRegStatus;

    faultRegStatus.cpuStatusRegister = faultRegTest->cpuStatus;
    fbe_zero_memory(tla_num, RESUME_PROM_EMC_TLA_PART_NUM_SIZE+1); 

    /* Check log based on Boot state */
    if (!(isMsgPresent = ravana_check_event_boot_state(faultRegTest)))
    {
        return FBE_FALSE;
    }
    
    /* Check log based on FRU status */
    if (faultRegTest->fruFaults > 0)
    {
        for(type = 0;type < faultRegTest->fruFaults; type++)
        {
            switch(faultRegTest->faultFrus[type].fruType)
            {
                case RAVANA_FLTREG_DIMM_FAULT:
                    isMsgPresent = ravana_check_event_with_slot(faultRegTest, type, fbe_pbl_decodeFru(PBL_FRU_DIMM));
                break;
                case RAVANA_FLTREG_DRIVE_FAULT:
                    isMsgPresent = ravana_check_event_with_slot(faultRegTest, type, fbe_pbl_decodeFru(PBL_FRU_DRIVE));
                break;
                case RAVANA_FLTREG_SLIC_FAULT:
                    isMsgPresent = ravana_check_event_with_slot_part_number(FBE_DEVICE_TYPE_IOMODULE,
                                                                            faultRegTest,
                                                                            type,
                                                                            fbe_pbl_decodeFru(PBL_FRU_SLIC));
                break;
                case RAVANA_FLTREG_POWER_FAULT:
                    isMsgPresent = ravana_check_event_with_slot_part_number(FBE_DEVICE_TYPE_PS,
                                                                            faultRegTest,
                                                                            type,
                                                                            fbe_pbl_decodeFru(PBL_FRU_POWER));
                break;
                case RAVANA_FLTREG_BATTERY_FAULT:
                    isMsgPresent = ravana_check_event_with_part_number(FBE_DEVICE_TYPE_BATTERY,
                                                                       faultRegTest,
                                                                       type,
                                                                       fbe_pbl_decodeFru(PBL_FRU_BATTERY));
                break;
                case RAVANA_FLTREG_SUPERCAP_FAULT:
                    isMsgPresent = ravana_check_event_with_fru(faultRegTest, fbe_pbl_decodeFru(PBL_FRU_SUPERCAP));
                break;
                case RAVANA_FLTREG_FAN_FAULT:
                    isMsgPresent = ravana_check_event_with_slot(faultRegTest, type, fbe_pbl_decodeFru(PBL_FRU_FAN));
                break;
                case RAVANA_FLTREG_I2C_FAULT:
                    isMsgPresent = ravana_check_event_with_fail(faultRegTest, type, NULL);
                break;
                case RAVANA_FLTREG_CPU_FAULT:
                    isMsgPresent = ravana_check_event_with_part_number(FBE_DEVICE_TYPE_SP,
                                                                       faultRegTest,
                                                                       type,
                                                                       fbe_pbl_decodeFru(PBL_FRU_CPU));
                break;
                case RAVANA_FLTREG_MGMT_FAULT:
                    isMsgPresent = ravana_check_event_with_part_number(FBE_DEVICE_TYPE_MGMT_MODULE,
                                                                       faultRegTest,
                                                                       type,
                                                                       fbe_pbl_decodeFru(PBL_FRU_MGMT));
                break;
                case RAVANA_FLTREG_BEM_FAULT:
                    isMsgPresent = ravana_check_event_with_part_number(FBE_DEVICE_TYPE_BACK_END_MODULE,
                                                                       faultRegTest,
                                                                       type,
                                                                       fbe_pbl_decodeFru(PBL_FRU_BM));
                break;
                case RAVANA_FLTREG_EFLASH_FAULT:
                    isMsgPresent = ravana_check_event_with_fru(faultRegTest, fbe_pbl_decodeFru(PBL_FRU_EFLASH));
                break;
                case RAVANA_FLTREG_EHORNET_FAULT:
                    isMsgPresent = ravana_check_event_with_fru(faultRegTest, fbe_pbl_decodeFru(PBL_FRU_CACHECARD));
                break;
                case RAVANA_FLTREG_MIDPLANE_FAULT:
                    isMsgPresent = ravana_check_event_with_part_number(FBE_DEVICE_TYPE_ENCLOSURE,
                                                                       faultRegTest,
                                                                       type,
                                                                       fbe_pbl_decodeFru(PBL_FRU_MIDPLANE));
                break;
                case RAVANA_FLTREG_CMI_FAULT:
                    isMsgPresent = ravana_check_event_with_fru(faultRegTest, fbe_pbl_decodeFru(PBL_FRU_CMI));
                break;
                case RAVANA_FLTREG_ALLFRUS_FAULT:
                    isMsgPresent = ravana_check_event_with_fru(faultRegTest, fbe_pbl_decodeFru(PBL_FRU_ALL_FRU));
                break;
                case RAVANA_FLTREG_EXTERNALFRU_FAULT:
                    isMsgPresent = ravana_check_event_with_fru(faultRegTest, fbe_pbl_decodeFru(PBL_FRU_EXTERNAL_FRU));
                break;
                default:
                    mut_printf(MUT_LOG_LOW, "Ravana test: Unknow fault type %d, please check.",
                        faultRegTest->faultFrus[type].fruType);
                    return FBE_FALSE;
                break;
            }
        }
    }

    return isMsgPresent;    
}
/***************************************************
 *  end of ravana_check_fault_reg_log_message()
 **************************************************/
/*!**************************************************************
 *  ravana_check_event_boot_state() 
 ****************************************************************
 * @brief
 *  This function return whether particular message present in 
 *  event_log or not 
 *
 * @param faultRegTest  - handle to fault test info
 * @param bootState     - peer boot state
 *
 * @return None.
 *
 * @author
 *  14-Aug-2012 Chengkai Hu - Created
 *
 ****************************************************************/
static fbe_bool_t
ravana_check_event_boot_state(ravana_fault_reg_test_t * faultRegTest)
{
    fbe_bool_t                  isMsgPresent = FBE_FALSE;
    fbe_status_t                status;
    fbe_u32_t                   cpuRegStatus;

    cpuRegStatus = faultRegTest->cpuStatus;
    
    switch(cpuRegStatus)
    {
        case CSR_OS_DEGRADED_MODE:
        case CSR_OS_APPLICATION_RUNNING:
        case CSR_OS_BLADE_BEING_SERVICED:
            status  = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_ESP,
                                                  &isMsgPresent,
                                                  ESP_INFO_PEER_SP_BOOT_OTHER,
                                                  decodeSpId(RAVANA_PEER_SP_ID),
                                                  decodeCpuStatusRegister(cpuRegStatus),
                                                  faultRegTest->cpuStatus);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            mut_printf(MUT_LOG_LOW, "Ravana test: Verifying bootState: %s(0x%x), event:0x%x....%s",
                        decodeCpuStatusRegister(cpuRegStatus),
                        cpuRegStatus,
                        ESP_INFO_PEER_SP_BOOT_OTHER,
                        isMsgPresent?"OK":".");
        break;
        default:
            mut_printf(MUT_LOG_LOW, "Ravana test: Skip bootState check for: %s(0x%x) ",
                        decodeCpuStatusRegister(cpuRegStatus),
                        cpuRegStatus);
            isMsgPresent = FBE_TRUE;
        break;
    }
    
        
    return isMsgPresent;
}
/*************************************************
 *  end of ravana_check_event_boot_state()
 ************************************************/
/*!**************************************************************
 *  ravana_check_event_with_fail() 
 ****************************************************************
 * @brief
 *  This function return whether particular message present in 
 *  event_log or not 
 *
 * @param faultRegTest  - handle to fault test info
 * @param faultType     - fault type index
 * @param faultStr      - fault type string
 *
 * @return None.
 *
 * @author
 *  17-Feb-2013 Chengkai Hu - Created
 *
 ****************************************************************/
static fbe_bool_t
ravana_check_event_with_fail(ravana_fault_reg_test_t * faultRegTest,
                             fbe_u8_t                  faultType,
                             const char              * faultStr)
{
    fbe_bool_t                  isMsgPresent = FBE_FALSE;
    fbe_u8_t                    slot = 0;
    fbe_status_t                status;
    fbe_u8_t                    tempStr[128];

    if(faultRegTest->faultFrus[faultType].fruType == RAVANA_FLTREG_I2C_FAULT)
    {
        for(slot = 0;slot < faultRegTest->faultFrus[faultType].slotCount;slot++)
        {               
            _snprintf(tempStr, sizeof(tempStr), "Detected a fault isolated to I2C Bus %d",
                faultRegTest->faultFrus[faultType].slots[slot]);
            tempStr[sizeof(tempStr)-1] = '\0';
        }
        status  = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_ESP,
                                              &isMsgPresent,
                                              ESP_CRIT_PEERSP_POST_FAIL,
                                              decodeSpId(RAVANA_PEER_SP_ID),
                                              tempStr);
        mut_printf(MUT_LOG_LOW, "Ravana test: Verifying %s fault, ....%s",
                    tempStr,
                    isMsgPresent?"OK":".");
    }
    else if(faultStr != NULL)
    {
        status  = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_ESP,
                                              &isMsgPresent,
                                              ESP_CRIT_PEERSP_POST_FAIL,
                                              decodeSpId(RAVANA_PEER_SP_ID),
                                              faultStr);
        mut_printf(MUT_LOG_LOW, "Ravana test: Verifying %s fault, ....%s",
                    faultStr,
                    isMsgPresent?"OK":".");
    }
    
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        
    return isMsgPresent;
}
/*************************************************
 *  end of ravana_check_event_with_fail()
 ************************************************/
 /*!**************************************************************
 *  ravana_check_event_with_fru() 
 ****************************************************************
 * @brief
 *  This function return whether particular message present in 
 *  event_log or not 
 *
 * @param faultRegTest  - handle to fault test info
 * @param faultType     - fault type index
 * @param faultStr      - fault type string
 *
 * @return None.
 *
 * @author
 *  14-Aug-2012 Chengkai Hu - Created
 *
 ****************************************************************/
static fbe_bool_t
ravana_check_event_with_fru(ravana_fault_reg_test_t * faultRegTest,
                            const char              * faultStr)
{
    fbe_bool_t                  isMsgPresent = FBE_FALSE;
    fbe_status_t                status;
    
    status  = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_ESP,
                                          &isMsgPresent,
                                          ESP_CRIT_PEERSP_POST_FAIL_FRU,
                                          decodeSpId(RAVANA_PEER_SP_ID),
                                          faultStr);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "Ravana test: Verifying %s fault, ....%s",
                faultStr,
                isMsgPresent?"OK":".");
        
    return isMsgPresent;
}
/*************************************************
 *  end of ravana_check_event_with_fru()
 ************************************************/
/*!**************************************************************
 *  ravana_check_event_with_slot() 
 ****************************************************************
 * @brief
 *  This function return whether particular message present in 
 *  event_log or not 
 *
 * @param faultRegTest  - handle to fault test info
 * @param faultType     - fault type index
 * @param faultStr      - fault type string
 *
 * @return None.
 *
 * @author
 *  14-Aug-2012 Chengkai Hu - Created
 *
 ****************************************************************/
static fbe_bool_t
ravana_check_event_with_slot(ravana_fault_reg_test_t * faultRegTest,
                             fbe_u8_t                  faultType,
                             const char              * faultStr)
{
    fbe_bool_t                  isMsgPresent = FBE_FALSE;
    fbe_u8_t                    slot = 0;
    fbe_status_t                status;
    
    for(slot = 0;slot < faultRegTest->faultFrus[faultType].slotCount;slot++)
    {
        status  = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_ESP,
                                              &isMsgPresent,
                                              ESP_CRIT_PEERSP_POST_FAIL_FRU_SLOT_NUM,
                                              decodeSpId(RAVANA_PEER_SP_ID),
                                              faultStr,
                                              faultRegTest->faultFrus[faultType].slots[slot]);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
        mut_printf(MUT_LOG_LOW, "Ravana test: Verifying %s fault, slot: %d....%s",
                    faultStr,
                    faultRegTest->faultFrus[faultType].slots[slot],
                    isMsgPresent?"OK":".");
        
        if(!isMsgPresent)
        {
            return isMsgPresent;
        }
    }
    return isMsgPresent;
}
/*************************************************
 *  end of ravana_check_event_with_slot()
 ************************************************/
/*!**************************************************************
 *  ravana_check_event_with_part_number() 
 ****************************************************************
 * @brief
 *  This function return whether particular message present in 
 *  event_log or not 
 *
 * @param faultRegTest  - handle to fault test info
 * @param faultType     - fault type index
 * @param faultStr      - fault type string
 *
 * @return None.
 *
 * @author
 *  14-Aug-2012 Chengkai Hu - Created
 *
 ****************************************************************/
static fbe_bool_t
ravana_check_event_with_part_number(fbe_u64_t         deviceType,
                                    ravana_fault_reg_test_t * faultRegTest,
                                    fbe_u8_t                  faultType,
                                    const char              * faultStr)
{
    fbe_bool_t                  isMsgPresent = FBE_FALSE;
    fbe_status_t                status;
	fbe_char_t                  tla_num[RESUME_PROM_EMC_TLA_PART_NUM_SIZE+1];

    
    ravana_get_tla_num(0, RAVANA_PEER_SP_ID, deviceType, tla_num, sizeof(tla_num));
    status  = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_ESP,
                                          &isMsgPresent,
                                          ESP_CRIT_PEERSP_POST_FAIL_FRU_PART_NUM,
                                          decodeSpId(RAVANA_PEER_SP_ID),
                                          faultStr,
                                          tla_num);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "Ravana test: Verifying %s fault, part number:%s, slot: %d....%s",
                faultStr,
                tla_num,
                0,
                isMsgPresent?"OK":".");
        
    return isMsgPresent;
}
/*************************************************
 *  end of ravana_check_event_with_part_number()
 ************************************************/
/*!**************************************************************
 *  ravana_check_event_with_slot_part_number() 
 ****************************************************************
 * @brief
 *  This function return whether particular message present in 
 *  event_log or not 
 *
 * @param faultRegTest  - handle to fault test info
 * @param faultType     - fault type index
 * @param faultStr      - fault type string
 *
 * @return None.
 *
 * @author
 *  14-Aug-2012 Chengkai Hu - Created
 *
 ****************************************************************/
static fbe_bool_t
ravana_check_event_with_slot_part_number(fbe_u64_t         deviceType,
                                         ravana_fault_reg_test_t * faultRegTest,
                                         fbe_u8_t                  faultType,
                                         const char              * faultStr)
{
    fbe_bool_t                  isMsgPresent = FBE_FALSE;
    fbe_u8_t                    slot = 0;
    fbe_status_t                status;
    fbe_char_t                  tla_num[RESUME_PROM_EMC_TLA_PART_NUM_SIZE+1];


    for(slot = 0;slot < faultRegTest->faultFrus[faultType].slotCount;slot++)
    {
        ravana_get_tla_num(faultRegTest->faultFrus[faultType].slots[slot],
            RAVANA_PEER_SP_ID, deviceType, tla_num, sizeof(tla_num));
    
        status  = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_ESP,
                                              &isMsgPresent,
                                              ESP_CRIT_PEERSP_POST_FAIL_FRU_SLOT_PART_NUM,
                                              decodeSpId(RAVANA_PEER_SP_ID),
                                              faultStr,
                                              faultRegTest->faultFrus[faultType].slots[slot],
                                              tla_num);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        mut_printf(MUT_LOG_LOW, "Ravana test: Verifying %s fault, slot: %d, part number:%s....%s",
                    faultStr,
                    faultRegTest->faultFrus[faultType].slots[slot],
                    tla_num,
                    isMsgPresent?"OK":".");
    }
        
    return isMsgPresent;
}
/*************************************************
 *  end of ravana_check_event_with_slot_part_number()
 ************************************************/
/*!**************************************************************
 *  ravana_check_slic_event() 
 ****************************************************************
 * @brief
 *  This function return whether particular message present in 
 *  event_log or not 
 *
 * @param faultRegTest  - handle to fault test info
 * @param typeIndex     - fault type index
 *
 * @return None.
 *
 * @author
 *  14-Aug-2012 Chengkai Hu - Created
 *
 ****************************************************************/
static fbe_bool_t
ravana_check_slic_event(ravana_fault_reg_test_t * faultRegTest,
                        fbe_u8_t                  typeIndex)
{
    fbe_bool_t                  isMsgPresent = FBE_FALSE;
    fbe_u8_t                    slot = 0;
    fbe_status_t                status;
    fbe_u64_t           deviceType;

    switch(faultRegTest->faultFrus[typeIndex].fruType)
    {
        case RAVANA_FLTREG_SLIC_FAULT:
            deviceType = FBE_DEVICE_TYPE_IOMODULE;
        break;
        case RAVANA_FLTREG_BEM_FAULT:
            deviceType = FBE_DEVICE_TYPE_BACK_END_MODULE;
        break;
        default:
            mut_printf(MUT_LOG_LOW, "Ravana test: Unsupported slic type: %d.",
                faultRegTest->faultFrus[typeIndex].fruType);
            return FBE_FALSE;
        break;
    }
    
    for(slot = 0;slot < faultRegTest->faultFrus[typeIndex].slotCount;slot++)
    {
        status  = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_ESP,
                                              &isMsgPresent,
                                              ESP_ERROR_IO_MODULE_FAULTED,
                                              fbe_module_mgmt_device_type_to_string(deviceType),
                                              faultRegTest->faultFrus[typeIndex].slots[slot],
                                              fbe_module_mgmt_module_substate_to_string(MODULE_SUBSTATE_FAULT_REG_FAILED));
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
        mut_printf(MUT_LOG_LOW, "Ravana test: Verifying SLIC fault, slot: %d....%s",
                    faultRegTest->faultFrus[typeIndex].slots[slot],
                    isMsgPresent?"OK":".");
        
        if(!isMsgPresent)
        {
            return isMsgPresent;
        }
    }
    return isMsgPresent;
}
/*************************************************
 *  end of ravana_check_slic_event()
 ************************************************/

/*!**************************************************************
 *  ravana_check_power_event() 
 ****************************************************************
 * @brief
 *  This function return whether particular message present in 
 *  event_log or not 
 *
 * @param faultRegTest  - handle to fault test info
 * @param typeIndex     - fault type index
 *
 * @return None.
 *
 * @author
 *  14-Aug-2012 Chengkai Hu - Created
 *
 ****************************************************************/
static fbe_bool_t
ravana_check_power_event(ravana_fault_reg_test_t * faultRegTest,
                        fbe_u8_t                  typeIndex)
{
    fbe_bool_t                      isMsgPresent = FBE_FALSE;
    fbe_u8_t                        slot = 0;
    fbe_status_t                    status;
    fbe_char_t                      deviceStr[FBE_DEVICE_STRING_LENGTH];
    fbe_device_physical_location_t  pLocation;
    fbe_u32_t                       psCount;


    if(RAVANA_PLATFORM_MEGATRON == platformType)
    {
        pLocation.bus = FBE_XPE_PSEUDO_BUS_NUM;
        pLocation.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
        status = fbe_api_esp_ps_mgmt_getPsCount(&pLocation, &psCount);
    }
    /* DPE PS is in enclosure mgmt */
    else if(RAVANA_PLATFORM_JETFIRE == platformType)
    {
        pLocation.bus = 0;
        pLocation.enclosure = 0;
        status = fbe_api_esp_encl_mgmt_get_ps_count(&pLocation, &psCount);
    }
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "psCount %d\n", psCount); 
    MUT_ASSERT_TRUE(psCount <= FBE_ESP_PS_MAX_COUNT_PER_ENCL && psCount >0);

    for(slot = 0;slot < faultRegTest->faultFrus[typeIndex].slotCount;slot++)
    {
        fbe_zero_memory(&deviceStr, (FBE_DEVICE_STRING_LENGTH));
        if(SP_B == RAVANA_PEER_SP_ID)
        {
            pLocation.slot = faultRegTest->faultFrus[typeIndex].slots[slot]+psCount/SP_ID_MAX;
        }
        else
        {
            pLocation.slot = faultRegTest->faultFrus[typeIndex].slots[slot];
        }
        /* Get the device string */
        status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_PS, 
                                                   &pLocation, 
                                                   deviceStr, 
                                                   sizeof(deviceStr));
        status  = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_ESP,
                                              &isMsgPresent,
                                              ESP_ERROR_PS_FAULTED,
                                              deviceStr,
                                              "FaultReg Fault ");
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
        mut_printf(MUT_LOG_LOW, "Ravana test: Verifying Power fault, slot: %d....%s",
                    faultRegTest->faultFrus[typeIndex].slots[slot],
                    isMsgPresent?"OK":".");
        
        if(!isMsgPresent)
        {
            return isMsgPresent;
        }
    }
    return isMsgPresent;
}
/*************************************************
 *  end of ravana_check_power_event()
 ************************************************/ 
/*!**************************************************************
 *  ravana_check_fan_event() 
 ****************************************************************
 * @brief
 *  This function return whether particular message present in 
 *  event_log or not 
 *
 * @param faultRegTest  - handle to fault test info
 * @param typeIndex     - fault type index
 *
 * @return None.
 *
 * @author
 *  12-Sep-2012 Chengkai Hu - Created
 *
 ****************************************************************/
static fbe_bool_t
ravana_check_fan_event(ravana_fault_reg_test_t * faultRegTest,
                        fbe_u8_t                  typeIndex)
{
    fbe_bool_t                      isMsgPresent = FBE_FALSE;
    fbe_u8_t                        slot = 0;
    fbe_status_t                    status;
    fbe_char_t                      deviceStr[FBE_DEVICE_STRING_LENGTH];
    fbe_device_physical_location_t  pLocation;
    fbe_u32_t                       fanCount = 0;


    if(RAVANA_PLATFORM_MEGATRON == platformType)
    {
        pLocation.bus = FBE_XPE_PSEUDO_BUS_NUM;
        pLocation.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
    }
    /* DPE PS is in enclosure mgmt */
    else if(RAVANA_PLATFORM_JETFIRE == platformType)
    {
        pLocation.bus = 0;
        pLocation.enclosure = 0;
    }    
    status = fbe_api_esp_encl_mgmt_get_fan_count(&pLocation, &fanCount);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "fanCount %d", fanCount); 
    MUT_ASSERT_TRUE(fanCount > 0);

    for(slot = 0;slot < faultRegTest->faultFrus[typeIndex].slotCount;slot++)
    {
        fbe_zero_memory(&deviceStr, (FBE_DEVICE_STRING_LENGTH));
        if(SP_B == RAVANA_PEER_SP_ID)
        {
            pLocation.slot = faultRegTest->faultFrus[typeIndex].slots[slot]+fanCount/SP_ID_MAX;
        }
        else
        {
            pLocation.slot = faultRegTest->faultFrus[typeIndex].slots[slot];
        }
        /* Get the device string */
        status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_FAN, 
                                                   &pLocation, 
                                                   deviceStr, 
                                                   sizeof(deviceStr));
        status  = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_ESP,
                                              &isMsgPresent,
                                              ESP_ERROR_FAN_FAULTED,
                                              deviceStr,
                                              "FaultReg Fault");
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
        mut_printf(MUT_LOG_LOW, "Ravana test: Verifying Fan fault, slot: %d....%s\n",
                    faultRegTest->faultFrus[typeIndex].slots[slot],
                    isMsgPresent?"OK":".");
        
        if(!isMsgPresent)
        {
            return isMsgPresent;
        }
    }
    return isMsgPresent;
}
/*************************************************
 *  end of ravana_check_fan_event()
 ************************************************/ 

 /*!**************************************************************
 *  ravana_fault_reg_check_event() 
 ****************************************************************
 * @brief
 *  Destroy function for Ravana peer boot Test.
 *
 * @param bootEntry 
 * @param timeoutMs 
 *
 * @return None.
 *
 * @author
 *  08-Aug-2012 Chengkai Hu - Created 
 *
 ****************************************************************/
static fbe_status_t
ravana_fault_reg_check_event(ravana_fault_reg_test_t * faultRegTest,
                             fbe_u32_t retryCount)
{
    fbe_u32_t   retry = 0;
    fbe_bool_t  isMsgPresent = FBE_FALSE;

    while(retry < retryCount)
    {
       isMsgPresent = ravana_check_fault_reg_log_message(faultRegTest);

       if (isMsgPresent == FBE_TRUE){
            return FBE_STATUS_OK;
        }
        retry++;
        fbe_api_sleep (FBE_API_POLLING_INTERVAL);
     }
    return FBE_STATUS_GENERIC_FAILURE;
}
/*************************************************
 *  end of ravana_fault_reg_check_event()
 ************************************************/
 /*!**************************************************************
 *  ravana_fault_reg_check_board_mgmt() 
 ****************************************************************
 * @brief
 *  Destroy function for Ravana peer boot Test.
 *
 * @param faultRegStatus - Fault register status handle.
 * @param  
 *
 * @return status value.
 *
 * @author
 *  08-Aug-2012 Chengkai Hu - Created 
 *
 ****************************************************************/
static void
ravana_fault_reg_check_board_mgmt(SPECL_FAULT_REGISTER_STATUS * faultRegStatus)
{
    fbe_esp_board_mgmt_peer_boot_info_t peerBootInfo = {0};
    fbe_status_t                        status;
    
    status = fbe_api_esp_board_mgmt_getPeerBootInfo(&peerBootInfo);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(peerBootInfo.peerBootState == fbe_pbl_getPeerBootState(faultRegStatus));
    return;
}
/*************************************************
 *  end of ravana_fault_reg_check_board_mgmt()
 ************************************************/
/*!**************************************************************
*  ravana_fault_reg_add_test() 
****************************************************************
* @brief
*  This function add particular fault type and slot to test 
*
* @param faultRegTest - fault register test handler
* @param faultType - fault type to update
* @param slotNum - fault fru slot to update
*
* @return None.
*
* @author
*  8-Aug-2012 Chengkai Hu - Created. 
*
****************************************************************/
static void
ravana_fault_reg_add_test(ravana_fault_reg_test_t     * faultRegTest,
                       ravana_fault_reg_fru_type_t      faultType,
                       fbe_u8_t                         slotNum)
{
    fbe_u8_t                 index = 0;
    ravana_rault_reg_fru_t * faultFru = NULL;

    /* Update fault type entry */
    for(index = 0;index < faultRegTest->fruFaults;index++)
    {
        if(faultType == faultRegTest->faultFrus[index].fruType)
        {
            faultFru = &faultRegTest->faultFrus[index];
            break;
        }
    }
    /* New fault type */
    if(index == faultRegTest->fruFaults)
    {
        if(index >= RAVANA_FLT_REG_FRU_TYPE)
        {
            mut_printf(MUT_LOG_LOW, "Ravana test: too many fault fru types, type:%d.\n", faultType);
            return;
        }
        faultRegTest->fruFaults++;
        faultFru = &faultRegTest->faultFrus[index];
        faultFru->fruType = faultType;
    }

    /* Update slot info */
    for(index = 0;index < faultFru->slotCount;index++)
    {
        if(slotNum == faultFru->slots[index])
        {
            mut_printf(MUT_LOG_LOW, "Ravana test: ignore duplicated slot, slot:%d.\n", slotNum);
            break;
        }
    }
    /* New slot */
    if(index == faultFru->slotCount)
    {
        if(index >= RAVANA_FLT_REG_MAX_SLOT)
        {
            mut_printf(MUT_LOG_LOW, "Ravana test: too many fault slot added, type:%d, slot:%d.\n",
                faultType, slotNum);
            return;
        }
        faultFru->slotCount++;
        faultFru->slots[index] = slotNum;
    }    
}
/**********************************
*  end of ravana_fault_reg_add_test()
*********************************/
/*!**************************************************************
 *  ravana_get_tla_num() 
 ****************************************************************
 * @brief
 *  This function return whether particular message present in 
 *  event_log or not 
 *
 * @param bootEntry - handle to fbe_esp_board_mgmt_peerBootEntry_t
 *
 * @return None.
 *
 * @author
 *  08-Aug-2012 Chengkai Hu - Created
 *
 ****************************************************************/
static void
ravana_get_tla_num(fbe_u8_t             slot,
                   fbe_u8_t             sp,
                   fbe_u64_t    device_type,
                   char                *tla_num,
                   fbe_u32_t            len)
{
    fbe_esp_base_env_get_resume_prom_info_cmd_t *pGetResumePromInfo = NULL;
    fbe_status_t                                 status;

    if (len < RESUME_PROM_EMC_TLA_PART_NUM_SIZE)
    {
        mut_printf(MUT_LOG_LOW, "%s, len too small %d\n", __FUNCTION__, len);
        return;
    }
    fbe_zero_memory(tla_num, len); 
    pGetResumePromInfo = malloc(sizeof(fbe_esp_base_env_get_resume_prom_info_cmd_t));
    fbe_zero_memory(pGetResumePromInfo, sizeof(fbe_esp_base_env_get_resume_prom_info_cmd_t));

    if(pGetResumePromInfo == NULL) 
    {
        mut_printf(MUT_LOG_LOW, "Mem alloc failed for pGetResumePromInfo.\n");
        tla_num[0] = '\0';
        return ;
    }

    pGetResumePromInfo->deviceType = device_type;
    pGetResumePromInfo->deviceLocation.slot = slot;
    pGetResumePromInfo->deviceLocation.sp = sp;
    pGetResumePromInfo->op_status = FBE_RESUME_PROM_STATUS_INVALID;

    status = fbe_api_esp_common_get_resume_prom_info(pGetResumePromInfo);

    if (!strncmp(tla_num, "", len) || status != FBE_STATUS_OK || pGetResumePromInfo->op_status != FBE_RESUME_PROM_STATUS_READ_SUCCESS)
    {
        strncpy(tla_num, "Not Found", len);
        tla_num[len-1] = '\0';
    }
    else
    {
        fbe_copy_memory(tla_num, 
                        pGetResumePromInfo->resume_prom_data.emc_tla_part_num,
                        len);
    }
    
    free(pGetResumePromInfo);
}
/*************************************************
 *  end of ravana_get_tla_num()
 ************************************************/
 /*!**************************************************************
*  ravana_get_specl_flt_reg() 
****************************************************************
* @brief
*  This function get the Fault Expander info from SPECL.
*
* @param sfi_mask_data - Handle to SPECL_SFI_MASK_DATA
*
* @return fbe_status_t
*
* @author
*  08-Aug-2012 Chengkai Hu - Created
*
****************************************************************/
static fbe_status_t
ravana_get_specl_flt_reg(PSPECL_SFI_MASK_DATA  sfi_mask_data)
{
    fbe_status_t status;
    sfi_mask_data->structNumber = SPECL_SFI_FLT_EXP_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.fltExpStatus.sfiMaskStatus = TRUE;

    status = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    return status;
}
/**************************************
 *  end of ravana_get_specl_flt_reg()
 *************************************/
/*!**************************************************************
 *  ravana_set_specl_flt_reg() 
 ****************************************************************
 * @brief
 *  This function set the Fault Expander info to SPECL.
 *
 * @param sfi_mask_data - Handle to SPECL_SFI_MASK_DATA
  *
 * @return fbe_status_t
 *
 * @author
 *  08-Aug-2012 Chengkai Hu - Created
 *
 ****************************************************************/
fbe_status_t
ravana_set_specl_flt_reg(PSPECL_SFI_MASK_DATA sfi_mask_data)
{
    fbe_status_t status;
    
    sfi_mask_data->structNumber = SPECL_SFI_FLT_EXP_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.fltExpStatus.sfiMaskStatus = TRUE;

    status = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    return status;
}
/**************************************
 *  end of ravana_set_specl_flt_reg()
 *************************************/
 /*!**************************************************************
 *  ravana_esp_object_data_change_callback_function() 
 ****************************************************************
 * @brief
 *  Notification callback function for data change
 *
 * @param update_object_msg: Object message
 * @param context: Callback context.               
 *
 * @return None.
 *
 * @author
 *  08-Aug-2012 Chengkai Hu - Created
 *
 ****************************************************************/
static void 
ravana_esp_object_data_change_callback_function(update_object_msg_t * update_object_msg, 
                                                void *context)
{
    fbe_semaphore_t *sem = (fbe_semaphore_t *)context;
    fbe_u64_t device_mask;

    device_mask = update_object_msg->notification_info.notification_data.data_change_info.device_mask;
    if (update_object_msg->object_id == flt_reg_expected_objectId) 
    {
        if((device_mask & flt_reg_device_mask) == flt_reg_device_mask) 
        {
            mut_printf(MUT_LOG_LOW, "Ravana test(fault register): Get the notification...\n");
            fbe_semaphore_release(sem, 0, 1 ,FALSE);
        }
    }
    return;
}
/*************************************************************
 *  end of ravana_esp_object_data_change_callback_function()
 ************************************************************/

 /*!**************************************************************
 *  ravana_fault_reg_setup() 
 ****************************************************************
 * @brief
 *  Set up function for Ravana peer boot Test.
 *
 * @param None
 *
 * @return None.
 *
 * @author
 *  08-Aug-2012 Chengkai Hu - Created
 *
 ****************************************************************/
static void ravana_fault_reg_setup()
{
    if(RAVANA_PLATFORM_MEGATRON == platformType)
    {
        fbe_test_startEspAndSep_with_common_config(FBE_SIM_SP_A,
                                            FBE_ESP_TEST_FP_CONIG,
                                            SPID_PROMETHEUS_M1_HW_TYPE,
                                            NULL,
                                            NULL);
    }
    else if(RAVANA_PLATFORM_JETFIRE == platformType)
    {
        fbe_test_startEspAndSep_with_common_config(FBE_SIM_SP_A,
                                            FBE_ESP_TEST_FP_CONIG,
                                            SPID_DEFIANT_M1_HW_TYPE,
                                            NULL,
                                            NULL);
    }
    else
    {
        mut_printf(MUT_LOG_LOW, "Unkown type platform:%d\n", platformType);
    }
}
/*************************
 *  end of ravana_setup()
 *************************/

 /*!**************************************************************
 *  ravana_fault_reg_destroy() 
 ****************************************************************
 * @brief
 *  Destroy function for Ravana peer boot Test.
 *
 * @param None
 *
 * @return None.
 *
 * @author
 *  08-Aug-2012 Chengkai Hu - Created
 *
 ****************************************************************/
void ravana_fault_reg_destroy(void)
{
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_delete_esp_lun();
    fbe_test_sep_util_destroy_neit_sep_physical();
    return;
}
/*************************
 *  end of ravana_destroy()
 *************************/
/*!**************************************************************
 *  ravana_fault_reg_megatron_setup() 
 ****************************************************************
 * @brief
 *  Set up function for Ravana peer boot Test for Megatron platform.
 *
 * @param None
 *
 * @return None.
 *
 * @author
 *  08-Aug-2012 Chengkai Hu - Created
 *
 ****************************************************************/
void ravana_fault_reg_megatron_setup()
{
    platformType = RAVANA_PLATFORM_MEGATRON;
    ravana_fault_reg_setup();
}
/*******************************************
 *  end of ravana_fault_reg_megatron_setup()
 ********************************************/
/*!**************************************************************
 *  ravana_fault_reg_jetfire_setup() 
 ****************************************************************
 * @brief
 *  Set up function for Ravana peer boot Test for Jetfire platform.
 *
 * @param None
 *
 * @return None.
 *
 * @author
 *  08-Aug-2012 Chengkai Hu - Created
 *
 ****************************************************************/
void ravana_fault_reg_jetfire_setup()
{
    platformType = RAVANA_PLATFORM_JETFIRE;
    ravana_fault_reg_setup();
}
/*******************************************
 *  end of ravana_fault_reg_jetfire_setup()
 ********************************************/

