/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file baloo_test.c
 ***************************************************************************
 *
 * @brief
 *  This file is for testing the registry infrastructre
 *
 * @version
 *   21-April-2010 - Created. Vaibhav Gaonkar
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "esp_tests.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_file.h"
#include "fbe_test_esp_utils.h"

#define REGISTRY_LENGTH   256

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * baloo_short_desc = "Test for Registry Infrastructure code";
char * baloo_long_desc = "\
This test validates the registry read, write and check operations\n\
The registry is created as esp_reg_pid<sp_pid>.txt file.\n\
1) Write and read the registry for REG_SZ type\n\
2) Write and read the registry for REG_DWORD type\n\
3) Write and read the registry for REG_BINARY type";

/*!**************************************************************
 * baloo_test() 
 ****************************************************************
 * @brief
 *  Main entry point to baloo_test 
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  21-April-2010 - Created. Vaibhav Gaonkar
 *
 ****************************************************************/
void baloo_test(void)
{
    /*Variable for registy REG_SZ type*/
    fbe_u8_t *reg_wr_sz_key = {"ESP_REG_SZ"};                     //Key
    fbe_u8_t reg_wr_sz_key_value[50] = {"ESP_REG_SZ_KEY_VALUE"};  //Key Value
    fbe_u8_t  reg_rd_sz_key_value[REGISTRY_LENGTH];              //place to retrieve key value
    
    /*Variable for registy REG_DWORD type*/
    fbe_u8_t *reg_wr_dword_key  = {"ESP_REG_DWORD"};
    fbe_u32_t reg_wr_dword_key_value = 55;
    fbe_u32_t  reg_rd_dword_key_value;

    /*Variable for registy REG_BINARY type*/
    fbe_u8_t *reg_wr_bool_key = {"ESP_REG_BINARY"};
    fbe_bool_t reg_wr_bool_key_value =  FBE_TRUE;
    fbe_bool_t reg_rd_bool_key_value;
    
    mut_printf(MUT_LOG_LOW, "Write the REG_SZ registry value ...\n");
    fbe_test_esp_registry_write(ESP_REG_PATH, reg_wr_sz_key, FBE_REGISTRY_VALUE_SZ, reg_wr_sz_key_value,0);
    fbe_test_esp_registry_read(ESP_REG_PATH,reg_wr_sz_key,&reg_rd_sz_key_value, REGISTRY_LENGTH, FBE_REGISTRY_VALUE_SZ,NULL,0,FALSE);
    MUT_ASSERT_STRING_EQUAL(reg_rd_sz_key_value, reg_wr_sz_key_value);

    mut_printf(MUT_LOG_LOW, "Write the REG_DWORD registry value ...\n");
    fbe_test_esp_registry_write(ESP_REG_PATH, reg_wr_dword_key, FBE_REGISTRY_VALUE_DWORD,&reg_wr_dword_key_value,0);
    fbe_test_esp_registry_read(ESP_REG_PATH,reg_wr_dword_key,&reg_rd_dword_key_value, sizeof(fbe_u32_t),FBE_REGISTRY_VALUE_DWORD,NULL,0,FALSE);
    MUT_ASSERT_INT_EQUAL(reg_rd_dword_key_value,reg_wr_dword_key_value);

    mut_printf(MUT_LOG_LOW, "Write the REG_BINARY registry value ...\n");
    fbe_test_esp_registry_write(ESP_REG_PATH, reg_wr_bool_key, FBE_REGISTRY_VALUE_BINARY,&reg_wr_bool_key_value,0);
    fbe_test_esp_registry_read(ESP_REG_PATH,reg_wr_bool_key,&reg_rd_bool_key_value, sizeof(fbe_bool_t) ,FBE_REGISTRY_VALUE_BINARY,NULL,0,FALSE);
    MUT_ASSERT_TRUE(reg_rd_bool_key_value);

    mut_printf(MUT_LOG_LOW, "Baloo test passed...\n");
    return;
}
/******************************************
 * end baloo_test()
 ******************************************/
void baloo_setup(void)
{
    /*  Create esp_reg_pid<sp_pid>.txt registry file */
    fbe_test_esp_create_registry_file(FBE_TRUE);
}

void baloo_destroy(void)
{
    fbe_test_esp_delete_registry_file();
}
/*************************
 * end file baloo_test.c
 *************************/

