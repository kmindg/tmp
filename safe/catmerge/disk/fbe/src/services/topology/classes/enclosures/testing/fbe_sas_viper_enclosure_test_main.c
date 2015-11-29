/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/**************************************************************************
 * fbe_sas_viper_enclosure_test_main.c
 ***************************************************************************
 *
 *  This file contains test functions for testing the main
 *  functions for the sas viper enclosure
 *
 * HISTORY
 *   7/14/2008:  Created. bphilbin
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_ses.h"
#include "sas_viper_enclosure_private.h"
#include "fbe_enclosure_test_prototypes.h"
#include "fbe_transport_memory.h"
#include "fbe_interface.h"
#include "fbe_enclosure_data_access_public.h"
#include "edal_eses_enclosure_data.h"
#include "mut.h"

#define MAX_VIPER_ENCL_DATA_BLOCK 2
#define FBE_EDAL_FLEX_COMP_TEST_MAX_VIPER_ENCL_DATA_BLOCK 10
#define FBE_EDAL_FLEX_COMP_TEST_MEMORY_CHUNK_SIZE 512

fbe_object_handle_t test_object_handle = EmcpalEmpty;
fbe_u8_t test_encl_comp_data[MAX_VIPER_ENCL_DATA_BLOCK*FBE_MEMORY_CHUNK_SIZE] = EmcpalEmpty;
fbe_eses_elem_group_t test_eses_elem_group[FBE_SAS_VIPER_ENCLOSURE_NUMBER_OF_POSSIBLE_ELEM_GROUPS] = EmcpalEmpty;
fbe_u8_t edal_flex_comp_test_encl_comp_data[FBE_EDAL_FLEX_COMP_TEST_MAX_VIPER_ENCL_DATA_BLOCK*FBE_EDAL_FLEX_COMP_TEST_MEMORY_CHUNK_SIZE];

/*
 * Data & structures used to test Enclosure Data Access
 */
typedef union edal_test_data_value_u
{
    fbe_u64_t       u64Value;
    fbe_u32_t       u32Value;
    fbe_u16_t       u16Value;
    fbe_u8_t        u8Value;
    fbe_u8_t        *test_str;
    fbe_bool_t      boolValue;
} edal_test_data_value_t;

typedef struct edal_test_data_info_s
{
    fbe_enclosure_component_types_t     component;
    fbe_base_enclosure_attributes_t     attribute;
    fbe_base_enclosure_data_type_t      dataType;
//    void                                *getFunctionPtr;
//    void                                *setFunctionPtr;
    fbe_u32_t                           index;
    edal_test_data_value_t              dataValue;
    fbe_bool_t                          successfulOperation;
} edal_test_data_info_t;

#define TEST_DATA_TABLE_COUNT   95
#define ENCL_SAS_ESES_BASE_TEST_DATA_TABLE_COUNT 15
edal_test_data_info_t TestDataInfoTable[TEST_DATA_TABLE_COUNT] =
{
    // Valid operations that will succeed
    { FBE_ENCL_ENCLOSURE,            FBE_ENCL_SERVER_SAS_ADDRESS,                EDAL_U64,     0,    -5,                 TRUE},//-5 will be considered as 0xfffffffb    
    { FBE_ENCL_ENCLOSURE,            FBE_ENCL_SERVER_SAS_ADDRESS,             EDAL_U64,     0,    0x11223344,            TRUE},
    { FBE_ENCL_ENCLOSURE,            FBE_ENCL_SMP_SAS_ADDRESS,                EDAL_U64,     0,    0x1,                   TRUE},
    { FBE_ENCL_ENCLOSURE,            FBE_ENCL_SES_SAS_ADDRESS,                EDAL_U64,     0,    0x112,                 TRUE},
    { FBE_ENCL_CONNECTOR,            FBE_ENCL_CONNECTOR_EXP_SAS_ADDRESS,      EDAL_U64,     2,    0x1122334455667788,    TRUE},
    { FBE_ENCL_EXPANDER,             FBE_ENCL_EXP_SAS_ADDRESS,                EDAL_U64,     1,    0x492734847,           TRUE},
    { FBE_ENCL_ENCLOSURE,            FBE_ENCL_UNIQUE_ID,                      EDAL_U32,     0,    0x1,                   TRUE},
    { FBE_ENCL_ENCLOSURE,            FBE_ENCL_POSITION,                       EDAL_U8,      0,    0,                     TRUE},
    { FBE_ENCL_ENCLOSURE,            FBE_ENCL_ADDRESS,                        EDAL_U8,      0,    5,                     TRUE},
    { FBE_ENCL_ENCLOSURE,            FBE_ENCL_PORT_NUMBER,                    EDAL_U8,      0,    1,                     TRUE},
    { FBE_ENCL_DRIVE_SLOT,           FBE_ENCL_DRIVE_PHY_INDEX,                EDAL_U8,      5,    0,                     TRUE},
    { FBE_ENCL_DRIVE_SLOT,           FBE_ENCL_COMP_ELEM_INDEX,                EDAL_U8,      5,    0,                     TRUE},
    { FBE_ENCL_DRIVE_SLOT,           FBE_ENCL_COMP_TYPE,                      EDAL_U8,      5,    0,                     TRUE},
    { FBE_ENCL_CONNECTOR,            FBE_ENCL_COMP_ELEM_INDEX,                EDAL_U8,      2,    0,                     TRUE},
    { FBE_ENCL_CONNECTOR,            FBE_ENCL_COMP_TYPE,                      EDAL_U8,      2,    0,                     TRUE},
    { FBE_ENCL_EXPANDER,             FBE_ENCL_COMP_ELEM_INDEX,                EDAL_U8,      0,    0,                     TRUE},
    { FBE_ENCL_EXPANDER_PHY,         FBE_ENCL_COMP_ELEM_INDEX,                EDAL_U8,      4,    0,                     TRUE},
    { FBE_ENCL_EXPANDER_PHY,         FBE_ENCL_EXP_PHY_EXPANDER_ELEM_INDEX,    EDAL_U8,      4,    0,                     TRUE},
    { FBE_ENCL_EXPANDER_PHY,         FBE_ENCL_EXP_PHY_ID,                     EDAL_U8,      4,    0,                     TRUE},
    { FBE_ENCL_POWER_SUPPLY,         FBE_ENCL_COMP_SUB_ENCL_ID,               EDAL_U8,      1,    0,                     TRUE},
    { FBE_ENCL_POWER_SUPPLY,         FBE_ENCL_COMP_ELEM_INDEX,                EDAL_U8,      1,    0,                     TRUE},
    { FBE_ENCL_COOLING_COMPONENT,    FBE_ENCL_COMP_ELEM_INDEX,                EDAL_U8,      1,    0,                     TRUE},
    { FBE_ENCL_TEMP_SENSOR,          FBE_ENCL_COMP_ELEM_INDEX,                EDAL_U8,      1,    0,                     TRUE},
    { FBE_ENCL_DISPLAY,              FBE_DISPLAY_MODE,                        EDAL_U8,      0,    0,                     TRUE},
    { FBE_ENCL_DISPLAY,              FBE_DISPLAY_CHARACTER,                   EDAL_U8,      0,    1,                     TRUE},
    { FBE_ENCL_ENCLOSURE,            FBE_ENCL_COMP_STATE_CHANGE,              EDAL_BOOL,    0,    TRUE,                  TRUE},
    { FBE_ENCL_ENCLOSURE,            FBE_ENCL_COMP_WRITE_DATA,                EDAL_BOOL,    0,    TRUE,                  TRUE},
    { FBE_ENCL_DRIVE_SLOT,           FBE_ENCL_COMP_INSERTED,                  EDAL_BOOL,    5,    TRUE,                  TRUE},
    { FBE_ENCL_DRIVE_SLOT,           FBE_ENCL_COMP_FAULTED,                   EDAL_BOOL,    5,    FALSE,                 TRUE},
    { FBE_ENCL_DRIVE_SLOT,           FBE_ENCL_COMP_POWERED_OFF,                EDAL_BOOL,    5,    TRUE,                  TRUE},
    { FBE_ENCL_DRIVE_SLOT,           FBE_ENCL_COMP_TURN_ON_FAULT_LED,         EDAL_BOOL,    5,    FALSE,                 TRUE},
    { FBE_ENCL_DRIVE_SLOT,           FBE_ENCL_COMP_MARK_COMPONENT,            EDAL_BOOL,    5,    TRUE,                  TRUE},
    { FBE_ENCL_DRIVE_SLOT,           FBE_ENCL_COMP_STATE_CHANGE,              EDAL_BOOL,    5,    TRUE,                  TRUE},
    { FBE_ENCL_DRIVE_SLOT,           FBE_ENCL_COMP_WRITE_DATA,                EDAL_BOOL,    5,    TRUE,                  TRUE},
    { FBE_ENCL_DRIVE_SLOT,           FBE_ENCL_DRIVE_BYPASSED,                 EDAL_BOOL,    5,    FALSE,                 TRUE},
    { FBE_ENCL_DRIVE_SLOT,           FBE_ENCL_DRIVE_LOGGED_IN,                EDAL_BOOL,    5,    TRUE,                  TRUE},
    { FBE_ENCL_DRIVE_SLOT,           FBE_ENCL_BYPASS_DRIVE,                   EDAL_BOOL,    5,    FALSE,                 TRUE},
    { FBE_ENCL_CONNECTOR,            FBE_ENCL_CONNECTOR_DISABLED,              EDAL_BOOL,    2,    TRUE,                  TRUE},
    { FBE_ENCL_CONNECTOR,            FBE_ENCL_COMP_STATE_CHANGE,              EDAL_BOOL,    2,    TRUE,                  TRUE},
    { FBE_ENCL_CONNECTOR,            FBE_ENCL_COMP_WRITE_DATA,                EDAL_BOOL,    2,    TRUE,                  TRUE},
    { FBE_ENCL_EXPANDER,             FBE_ENCL_COMP_STATE_CHANGE,              EDAL_BOOL,    0,    TRUE,                  TRUE},
    { FBE_ENCL_EXPANDER,             FBE_ENCL_COMP_WRITE_DATA,                EDAL_BOOL,    1,    TRUE,                  TRUE},
    { FBE_ENCL_EXPANDER_PHY,         FBE_ENCL_EXP_PHY_DISABLED,                EDAL_BOOL,    5,    TRUE,                  TRUE},
    { FBE_ENCL_EXPANDER_PHY,         FBE_ENCL_EXP_PHY_READY,                  EDAL_BOOL,    5,    TRUE,                  TRUE},
    { FBE_ENCL_EXPANDER_PHY,         FBE_ENCL_EXP_PHY_LINK_READY,             EDAL_BOOL,    5,    TRUE,                  TRUE},
    { FBE_ENCL_EXPANDER_PHY,         FBE_ENCL_EXP_PHY_FORCE_DISABLED,         EDAL_BOOL,    5,    FALSE,                 TRUE},
    { FBE_ENCL_EXPANDER_PHY,         FBE_ENCL_EXP_PHY_CARRIER_DETECTED,       EDAL_BOOL,    5,    TRUE,                  TRUE},
    { FBE_ENCL_EXPANDER_PHY,         FBE_ENCL_EXP_PHY_SPINUP_ENABLED,         EDAL_BOOL,    5,    TRUE,                  TRUE},
    { FBE_ENCL_EXPANDER_PHY,         FBE_ENCL_EXP_PHY_SATA_SPINUP_HOLD,       EDAL_BOOL,    6,    FALSE,                 TRUE},
    { FBE_ENCL_EXPANDER_PHY,         FBE_ENCL_COMP_STATE_CHANGE,              EDAL_BOOL,    5,    TRUE,                  TRUE},
    { FBE_ENCL_EXPANDER_PHY,         FBE_ENCL_COMP_WRITE_DATA,                EDAL_BOOL,    5,    TRUE,                  TRUE},
    { FBE_ENCL_POWER_SUPPLY,         FBE_ENCL_COMP_INSERTED,                  EDAL_BOOL,    1,    TRUE,                  TRUE},
    { FBE_ENCL_POWER_SUPPLY,         FBE_ENCL_COMP_FAULTED,                   EDAL_BOOL,    1,    FALSE,                 TRUE},
    { FBE_ENCL_POWER_SUPPLY,         FBE_ENCL_COMP_POWERED_OFF,                EDAL_BOOL,    1,    TRUE,                  TRUE},
    { FBE_ENCL_POWER_SUPPLY,         FBE_ENCL_COMP_TURN_ON_FAULT_LED,         EDAL_BOOL,    1,    FALSE,                 TRUE},
    { FBE_ENCL_POWER_SUPPLY,         FBE_ENCL_COMP_MARK_COMPONENT,            EDAL_BOOL,    1,    TRUE,                  TRUE},
    { FBE_ENCL_POWER_SUPPLY,         FBE_ENCL_COMP_STATE_CHANGE,              EDAL_BOOL,    1,    TRUE,                  TRUE},
    { FBE_ENCL_POWER_SUPPLY,         FBE_ENCL_COMP_WRITE_DATA,                EDAL_BOOL,    1,    TRUE,                  TRUE},
    { FBE_ENCL_POWER_SUPPLY,         FBE_ENCL_PS_AC_FAIL,                     EDAL_BOOL,    1,    FALSE,                 TRUE},
    { FBE_ENCL_POWER_SUPPLY,         FBE_ENCL_PS_DC_DETECTED,                 EDAL_BOOL,    1,    FALSE,                 TRUE},
    { FBE_ENCL_COOLING_COMPONENT,    FBE_ENCL_COMP_INSERTED,                  EDAL_BOOL,    1,    TRUE,                  TRUE},
    { FBE_ENCL_COOLING_COMPONENT,    FBE_ENCL_COMP_FAULTED,                   EDAL_BOOL,    1,    FALSE,                 TRUE},
    { FBE_ENCL_COOLING_COMPONENT,    FBE_ENCL_COMP_POWERED_OFF,                EDAL_BOOL,    1,    TRUE,                  TRUE},
    { FBE_ENCL_COOLING_COMPONENT,    FBE_ENCL_COMP_TURN_ON_FAULT_LED,         EDAL_BOOL,    1,    TRUE,                  TRUE},
    { FBE_ENCL_COOLING_COMPONENT,    FBE_ENCL_COMP_MARK_COMPONENT,            EDAL_BOOL,    1,    FALSE,                 TRUE},
    { FBE_ENCL_COOLING_COMPONENT,    FBE_ENCL_COMP_STATE_CHANGE,              EDAL_BOOL,    1,    TRUE,                  TRUE},
    { FBE_ENCL_COOLING_COMPONENT,    FBE_ENCL_COMP_WRITE_DATA,                EDAL_BOOL,    1,    TRUE,                  TRUE},
    { FBE_ENCL_COOLING_COMPONENT,    FBE_ENCL_COOLING_MULTI_FAN_FAULT,        EDAL_BOOL,    1,    FALSE,                 TRUE},
    { FBE_ENCL_TEMP_SENSOR,          FBE_ENCL_COMP_INSERTED,                  EDAL_BOOL,    1,    TRUE,                  TRUE},
    { FBE_ENCL_TEMP_SENSOR,          FBE_ENCL_COMP_FAULTED,                   EDAL_BOOL,    1,    FALSE,                 TRUE},
    { FBE_ENCL_TEMP_SENSOR,          FBE_ENCL_COMP_POWERED_OFF,                 EDAL_BOOL,    1,    TRUE,                 TRUE},
    { FBE_ENCL_TEMP_SENSOR,          FBE_ENCL_COMP_TURN_ON_FAULT_LED,         EDAL_BOOL,    1,    FALSE,                 TRUE},
    { FBE_ENCL_TEMP_SENSOR,          FBE_ENCL_COMP_MARK_COMPONENT,            EDAL_BOOL,    1,    TRUE,                  TRUE},
    { FBE_ENCL_TEMP_SENSOR,          FBE_ENCL_COMP_STATE_CHANGE,              EDAL_BOOL,    1,    TRUE,                  TRUE},
    { FBE_ENCL_TEMP_SENSOR,          FBE_ENCL_COMP_WRITE_DATA,                EDAL_BOOL,    1,    TRUE,                  TRUE},
    { FBE_ENCL_TEMP_SENSOR,          FBE_ENCL_TEMP_SENSOR_OT_WARNING,         EDAL_BOOL,    1,    FALSE,                 TRUE},
    { FBE_ENCL_TEMP_SENSOR,          FBE_ENCL_TEMP_SENSOR_OT_FAILURE,         EDAL_BOOL,    1,    FALSE,                 TRUE},
    { FBE_ENCL_DRIVE_SLOT,    FBE_ENCL_COMP_ADDL_STATUS,            EDAL_U8,  1,  1,               TRUE },
    // Invalid operations that will fail
    { FBE_ENCL_ENCLOSURE,            FBE_ENCL_EXP_SAS_ADDRESS,                EDAL_U64,     1,    0x11223344,            FALSE},
    { FBE_ENCL_ENCLOSURE,            FBE_ENCL_DRIVE_PHY_INDEX,                EDAL_U64,     1,    0x11223344,            FALSE},
    { FBE_ENCL_DRIVE_SLOT,           FBE_ENCL_SERVER_SAS_ADDRESS,             EDAL_U64,     12,   0x11223344,            FALSE},
    { FBE_ENCL_CONNECTOR,            FBE_ENCL_SERVER_SAS_ADDRESS,             EDAL_U64,     2,    0x1122334455667788,    FALSE},
    { FBE_ENCL_EXPANDER,             FBE_ENCL_SERVER_SAS_ADDRESS,             EDAL_U64,     1,    0x1122334455667788,    FALSE},
    { FBE_ENCL_POWER_SUPPLY,         FBE_ENCL_EXP_SAS_ADDRESS,                EDAL_U64,     1,    0x1122334455667788,    FALSE},
    { FBE_ENCL_ENCLOSURE,            FBE_ENCL_SERVER_SAS_ADDRESS,             EDAL_U32,     1,    0x11223344,            FALSE},
    { FBE_ENCL_CONNECTOR,            FBE_ENCL_CONNECTOR_EXP_SAS_ADDRESS,      EDAL_U32,     7,    0x11223344,            FALSE},
    { FBE_ENCL_CONNECTOR,            FBE_ENCL_CONNECTOR_EXP_SAS_ADDRESS,      EDAL_U16,     7,    0x1122,                FALSE},
    { FBE_ENCL_CONNECTOR,            FBE_ENCL_BYPASS_DRIVE,                   EDAL_BOOL,    2,    TRUE,                  FALSE},
    { FBE_ENCL_EXPANDER,             FBE_ENCL_EXP_PHY_LINK_READY,             EDAL_BOOL,    0,    TRUE,                  FALSE},
    { FBE_ENCL_EXPANDER,             FBE_ENCL_EXP_PHY_READY,                  EDAL_BOOL,    1,    TRUE,                  FALSE},
    { FBE_ENCL_POWER_SUPPLY,         FBE_ENCL_EXP_PHY_DISABLED,                EDAL_BOOL,    0,    FALSE,                 FALSE},
    { FBE_ENCL_DRIVE_SLOT,           FBE_ENCL_COMP_INSERTED,                  EDAL_BOOL,    15,   TRUE,                  FALSE}
};

edal_test_data_info_t Encl_Sas_Eses_base_TestDataInfoTable[ENCL_SAS_ESES_BASE_TEST_DATA_TABLE_COUNT] =
{    
    // Invalid operations that will fail (Nagative test)
    { FBE_ENCL_ENCLOSURE,            FBE_ENCL_EXP_SAS_ADDRESS,                EDAL_U64,     1,    0x11223344,            FALSE},
    { FBE_ENCL_ENCLOSURE,            FBE_ENCL_DRIVE_PHY_INDEX,                EDAL_U64,     1,    0x11223344,            FALSE},
    { FBE_ENCL_DRIVE_SLOT,           FBE_ENCL_SERVER_SAS_ADDRESS,             EDAL_U64,     12,   0x11223344,            FALSE},
    { FBE_ENCL_CONNECTOR,            FBE_ENCL_SERVER_SAS_ADDRESS,             EDAL_U64,     2,    0x1122334455667788,    FALSE},
    { FBE_ENCL_EXPANDER,             FBE_ENCL_SERVER_SAS_ADDRESS,             EDAL_U64,     1,    0x1122334455667788,    FALSE},
    { FBE_ENCL_POWER_SUPPLY,         FBE_ENCL_EXP_SAS_ADDRESS,                EDAL_U64,     1,    0x1122334455667788,    FALSE},
    { FBE_ENCL_ENCLOSURE,            FBE_ENCL_SERVER_SAS_ADDRESS,           EDAL_U32,       0,    0x11223344,            FALSE},
    { FBE_ENCL_CONNECTOR,            FBE_ENCL_CONNECTOR_EXP_SAS_ADDRESS,    EDAL_U32,       7,    0x11223344,            FALSE},
    { FBE_ENCL_CONNECTOR,            FBE_ENCL_CONNECTOR_EXP_SAS_ADDRESS,    EDAL_U16,       7,    0x1122,                FALSE},
    { FBE_ENCL_CONNECTOR,            FBE_ENCL_BYPASS_DRIVE,                 EDAL_BOOL,      2,    TRUE,                  FALSE},
    { FBE_ENCL_EXPANDER,             FBE_ENCL_EXP_PHY_LINK_READY,           EDAL_BOOL,      0,    TRUE,                  FALSE },
    { FBE_ENCL_EXPANDER,             FBE_ENCL_EXP_PHY_READY,                EDAL_BOOL,      1,    TRUE,                  FALSE },
    { FBE_ENCL_POWER_SUPPLY,         FBE_ENCL_EXP_PHY_DISABLED,              EDAL_BOOL,      0,    FALSE,                 FALSE },
    { FBE_ENCL_DRIVE_SLOT,           FBE_ENCL_COMP_INSERTED,                EDAL_BOOL,      15,   TRUE,                  FALSE },
    { FBE_ENCL_DRIVE_SLOT,           FBE_ENCL_PORT_NUMBER,                EDAL_U8,      5,   TRUE,                  FALSE }
};

/* Function to test EDAL functions and data*/
void test_edal_function(edal_test_data_info_t * TestDataInfoTableTest ,fbe_enclosure_component_block_t     *sas_viper_component_block, fbe_u16_t  max_index);
// utility function
fbe_enclosure_component_block_t *fbe_sas_viper_init_component_block(fbe_sas_viper_enclosure_t * viperEnclPtr, fbe_u8_t *buffer);

static void fbe_sas_viper_enclosure_test_edal(fbe_enclosure_component_block_t * sas_viper_component_block);
static fbe_enclosure_component_block_t * 
fbe_edal_flex_comp_test_sas_viper_init_component_block(fbe_sas_viper_enclosure_t * viperEnclPtr, fbe_u8_t *buffer);
/**************************************************************************
 * fbe_sas_viper_enclosure_test_create_object_function
 *************************************************************************
 *
 *  This function tests the viper enclosure object creation and initialization
 *  functions.
 *
 * HISTORY
 *   7/14/2008:   Created.  bphilbin
 *************************************************************************/
VOID fbe_sas_viper_enclosure_test_create_object_function(void)
{
#if 0
    fbe_object_handle_t * p_object_handle;
    fbe_packet_t * p_packet;
    fbe_status_t status;

    p_object_handle = &test_object_handle;
    p_packet = NULL;

    p_packet = fbe_transport_allocate_packet();

    MUT_ASSERT_NOT_NULL(p_packet);
    
    status = fbe_sas_viper_enclosure_create_object(p_packet, p_object_handle);

    MUT_ASSERT_INT_EQUAL(status,  FBE_STATUS_OK);

    fbe_transport_release_packet(p_packet);
#endif
    return;
    
}


/***************************************************************
 *  fbe_sas_viper_enclosure_test_lifecycle()
 ****************************************************************
 * 
 *  Validates the enclosure's lifecycle.
 *
 * @param  none.          
 *
 * @return none
 *
 * HISTORY:
 *  7/22/2008 - Created. bphilbin
 *
 ****************************************************************/

void fbe_sas_viper_enclosure_test_lifecycle(void)
{
    fbe_lifecycle_const_t * p_class_const;
    fbe_status_t status;

    /* Get the logical drive's constant data.
     */
    status = fbe_get_class_lifecycle_const_data(FBE_CLASS_ID_SAS_VIPER_ENCLOSURE, &p_class_const);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Make sure this data is valid.
     */
    status = fbe_lifecycle_class_const_verify(p_class_const);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    return;
}
/******************************************
 * end fbe_sas_viper_enclosure_test_lifecycle()
 ******************************************/


/***************************************************************
 *  fbe_sas_viper_enclosure_test_publicDataAccess()
 ****************************************************************
 * @brief
 *  This function tests the EDAL with the normal memory chunk size.
 *
 * @param  none.          
 *
 * @return none
 *
 * HISTORY:
 *  7/22/2008 - Created. Joe Perry
 *
 ****************************************************************/

void fbe_sas_viper_enclosure_test_publicDataAccess(void)
{
    fbe_sas_viper_enclosure_t           viperEnclosure, *viperEnclPtr;
    fbe_enclosure_component_block_t     *sas_viper_component_block;

    /*
     * Initialize Viper Enclosure
     */
    memset(&viperEnclosure, 0, sizeof(fbe_sas_viper_enclosure_t));
    viperEnclPtr = &viperEnclosure;

    sas_viper_component_block = fbe_sas_viper_init_component_block(viperEnclPtr, test_encl_comp_data);

    fbe_sas_viper_enclosure_test_edal(sas_viper_component_block);

    return;   
}
/******************************************
 * end fbe_sas_viper_enclosure_test_publicDataAccess()
 ******************************************/

/***************************************************************
 *  fbe_sas_viper_enclosure_test_edalFlexComp()
 ****************************************************************
 * @brief
 *  This function tests the EDAL with the component type 
 *  spanning multiple blocks by reducing the memory chunk size.
 *
 * @param  none.          
 *
 * @return none
 *
 * HISTORY:
 *  29-Mar-2010 PHE - Created.
 *
 ****************************************************************/

void fbe_sas_viper_enclosure_test_edalFlexComp(void)
{
    fbe_sas_viper_enclosure_t           viperEnclosure, *viperEnclPtr;
    fbe_enclosure_component_block_t     *sas_viper_component_block;

    /*
     * Initialize Viper Enclosure
     */
    memset(&viperEnclosure, 0, sizeof(fbe_sas_viper_enclosure_t));
    viperEnclPtr = &viperEnclosure;

    sas_viper_component_block = fbe_edal_flex_comp_test_sas_viper_init_component_block(viperEnclPtr, 
                                 edal_flex_comp_test_encl_comp_data);

    fbe_sas_viper_enclosure_test_edal(sas_viper_component_block);
    
    return;   
}
/******************************************
 * end fbe_sas_viper_enclosure_test_edalFlexComp()
 ******************************************/

/***************************************************************
 *  fbe_sas_viper_enclosure_test_edal()
 ****************************************************************
 * @brief
 *   This function tests the EDAL data for sas viper enclosure.
 *
 * @param  
 *   sas_viper_component_block - pointer to the first EDAL block.
 *
 * @return none
 *
 * HISTORY:
 *  29-Mar-2010 PHE - Moved from fbe_sas_viper_enclosure_test_publicDataAccess.
 *
 ****************************************************************/
void fbe_sas_viper_enclosure_test_edal(fbe_enclosure_component_block_t * sas_viper_component_block)
{
    edal_test_data_info_t *edalTablePtr;
        
    /* test edal data and functions with enclosure type = FBE_ENCL_SAS_VIPER*/
    sas_viper_component_block->enclosureType = FBE_ENCL_SAS_VIPER;
    edalTablePtr = TestDataInfoTable;
    test_edal_function(edalTablePtr , sas_viper_component_block, TEST_DATA_TABLE_COUNT);
    
    /* Now test edal data and functions with enclosure type = FBE_ENCL_SAS*/
    sas_viper_component_block->enclosureType = FBE_ENCL_SAS;
    edalTablePtr = Encl_Sas_Eses_base_TestDataInfoTable;
    test_edal_function(edalTablePtr , sas_viper_component_block, ENCL_SAS_ESES_BASE_TEST_DATA_TABLE_COUNT);


    /* Now test edal data and functions with enclosure type = FBE_ENCL_ESES*/
    sas_viper_component_block->enclosureType = FBE_ENCL_ESES;
    edalTablePtr = Encl_Sas_Eses_base_TestDataInfoTable;
    test_edal_function(edalTablePtr , sas_viper_component_block, ENCL_SAS_ESES_BASE_TEST_DATA_TABLE_COUNT);

    /* Now test edal data and functions with enclosure type = FBE_ENCL_BASE*/
    sas_viper_component_block->enclosureType = FBE_ENCL_BASE;
    edalTablePtr = Encl_Sas_Eses_base_TestDataInfoTable;
    test_edal_function(edalTablePtr , sas_viper_component_block, ENCL_SAS_ESES_BASE_TEST_DATA_TABLE_COUNT);
    return;
}
/******************************************
 * end fbe_sas_viper_enclosure_test_edal()
 ******************************************/

/***************************************************************
 *  test_edal_function()
 ****************************************************************
 * 
 *  Validates the edal functions and data.
 *
 * @param           
 *                TestDataInfoTableTest -Pointer for table  
 *                sas_viper_component_block - Component block pointer
 *                max_index - Max. Number of element in table
 *
 * @return none
 *
 * HISTORY:
 *  11/12/2008 - Created. Joe Perry
 *
 ****************************************************************/
void test_edal_function(edal_test_data_info_t * TestDataInfoTableTest, fbe_enclosure_component_block_t     *sas_viper_component_block, fbe_u16_t  max_index)
{
    fbe_u16_t                           index;
    edal_test_data_value_t              returnValue;
    fbe_edal_status_t                   edalStatus;
    fbe_edal_block_handle_t         edalBlockPtr;
    char get_str[5];

   edalBlockPtr = (fbe_edal_block_handle_t)sas_viper_component_block;

    for (index = 0; index < max_index; index++)
    {
        /* The if..block is removed as it serves no purpose */

        // set the specified attribute
        switch (TestDataInfoTableTest[index].dataType)
        {
        case EDAL_U64:
            edalStatus = fbe_edal_setU64(edalBlockPtr,
                                         TestDataInfoTableTest[index].attribute,
                                         TestDataInfoTableTest[index].component,
                                         TestDataInfoTableTest[index].index,
                                         TestDataInfoTableTest[index].dataValue.u64Value);
            break;
        case EDAL_U32:
            edalStatus = fbe_edal_setU32(edalBlockPtr,
                                         TestDataInfoTableTest[index].attribute,
                                         TestDataInfoTableTest[index].component,
                                         TestDataInfoTableTest[index].index,
                                         TestDataInfoTableTest[index].dataValue.u32Value);
            break;
        case EDAL_U16:
            edalStatus = fbe_edal_setU16(edalBlockPtr,
                                         TestDataInfoTableTest[index].attribute,
                                         TestDataInfoTableTest[index].component,
                                         TestDataInfoTableTest[index].index,
                                         TestDataInfoTableTest[index].dataValue.u16Value);
            break;
        case EDAL_U8:
            edalStatus = fbe_edal_setU8(edalBlockPtr,
                                        TestDataInfoTableTest[index].attribute,
                                        TestDataInfoTableTest[index].component,
                                        TestDataInfoTableTest[index].index,
                                        TestDataInfoTableTest[index].dataValue.u8Value);
            break;
        case EDAL_BOOL:
            edalStatus = fbe_edal_setBool(edalBlockPtr,
                                          TestDataInfoTableTest[index].attribute,
                                          TestDataInfoTableTest[index].component,
                                          TestDataInfoTableTest[index].index,
                                          TestDataInfoTableTest[index].dataValue.boolValue);
            break;
        case EDAL_STR:
            edalStatus = fbe_edal_setStr(edalBlockPtr,
                                          TestDataInfoTableTest[index].attribute,
                                          TestDataInfoTableTest[index].component,
                                          TestDataInfoTableTest[index].index,
                                          FBE_ESES_FW_REVISION_SIZE,
                                          TestDataInfoTableTest[index].dataValue.test_str);
            break;
        default:
            break;
        }
        // check for correct result of the operation (some are meant to fail)
        if (TestDataInfoTableTest[index].successfulOperation)
        {
            if(EDAL_STAT_OK(edalStatus))
            {
                edalStatus = FBE_EDAL_STATUS_OK;
            }

            MUT_ASSERT_INT_EQUAL(edalStatus, FBE_EDAL_STATUS_OK);
        }
        else
        {
            MUT_ASSERT_INT_NOT_EQUAL(edalStatus, FBE_EDAL_STATUS_OK);
        }
        // get the specified attribute & verify the value
        switch (TestDataInfoTableTest[index].dataType)
        {
        case EDAL_U64:
            edalStatus = fbe_edal_getU64(edalBlockPtr,
                                         TestDataInfoTableTest[index].attribute,
                                         TestDataInfoTableTest[index].component,
                                         TestDataInfoTableTest[index].index,
                                         &(returnValue.u64Value));
            break;
        case EDAL_U32:
            edalStatus = fbe_edal_getU32(edalBlockPtr,
                                         TestDataInfoTableTest[index].attribute,
                                         TestDataInfoTableTest[index].component,
                                         TestDataInfoTableTest[index].index,
                                         &returnValue.u32Value);
            break;
        case EDAL_U16:
            edalStatus = fbe_edal_getU16(edalBlockPtr,
                                         TestDataInfoTableTest[index].attribute,
                                         TestDataInfoTableTest[index].component,
                                         TestDataInfoTableTest[index].index,
                                         &returnValue.u16Value);
            break;
        case EDAL_U8:
            edalStatus = fbe_edal_getU8(edalBlockPtr,
                                        TestDataInfoTableTest[index].attribute,
                                        TestDataInfoTableTest[index].component,
                                        TestDataInfoTableTest[index].index,
                                        &returnValue.u8Value);
            break;
        case EDAL_BOOL:
            edalStatus = fbe_edal_getBool(edalBlockPtr,
                                          TestDataInfoTableTest[index].attribute,
                                          TestDataInfoTableTest[index].component,
                                          TestDataInfoTableTest[index].index,
                                          &returnValue.boolValue);
            break;
        case EDAL_STR:
            edalStatus = fbe_edal_getStr(edalBlockPtr,
                                          TestDataInfoTableTest[index].attribute,
                                          TestDataInfoTableTest[index].component,
                                          TestDataInfoTableTest[index].index,
                                          FBE_ESES_FW_REVISION_SIZE,
                                          get_str);
            break;
        default:
            break;
        }
        // check for correct result of the operation (some are meant to fail)
        if (TestDataInfoTableTest[index].successfulOperation)
        {
            MUT_ASSERT_INT_EQUAL(edalStatus,  FBE_EDAL_STATUS_OK);
            switch (TestDataInfoTableTest[index].dataType)
            {
                case EDAL_U64:
                    MUT_ASSERT_TRUE(returnValue.u64Value == TestDataInfoTableTest[index].dataValue.u64Value);
                break;
                case EDAL_U32:
                    MUT_ASSERT_TRUE(returnValue.u32Value == TestDataInfoTableTest[index].dataValue.u32Value);
                break;
                case EDAL_U16:
                    MUT_ASSERT_TRUE(returnValue.u16Value == TestDataInfoTableTest[index].dataValue.u16Value);
                break;
                case EDAL_U8:
                    MUT_ASSERT_TRUE(returnValue.u8Value == TestDataInfoTableTest[index].dataValue.u8Value);
                break;
                case EDAL_BOOL:
                    MUT_ASSERT_TRUE(returnValue.boolValue == TestDataInfoTableTest[index].dataValue.boolValue);
                break;
                case EDAL_STR:
                    MUT_ASSERT_STRING_EQUAL((TestDataInfoTableTest[index].dataValue.test_str), (get_str));
                break;
                default:
                    //Do Nothing
                break;
             }
        }
        else
        {
            MUT_ASSERT_INT_NOT_EQUAL(edalStatus,  FBE_EDAL_STATUS_OK);
        }
    }
    fbe_edal_printAllComponentData((void *)sas_viper_component_block, fbe_edal_trace_func);
}
/************************
 * end test_edal_function()
 ************************/

/***************************************************************
 * fbe_edal_flex_comp_test_sas_viper_init_component_block()
 ****************************************************************
 * @brief
 *  This function initializes the headers for all the sas viper EDAL blocks
 *  for the EDAL flexible component type test. 
 *   
 * @param   
 *   viperEnclPtr - pointer to the sas viper enclosure
 *   buffer - pointer to the first EDAL block.
 *                        
 * @return 
 *   the pointer to the first EDAL block.
 *
 * HISTORY:
 *  29-Mar-2010 PHE - Created.
 *
 ****************************************************************/
static fbe_enclosure_component_block_t * 
fbe_edal_flex_comp_test_sas_viper_init_component_block(fbe_sas_viper_enclosure_t * viperEnclPtr, fbe_u8_t *buffer)
{
   fbe_enclosure_component_block_t *sas_viper_component_block = NULL;
   
   sas_viper_component_block = fbe_sas_viper_init_component_block_with_block_size(viperEnclPtr, 
                                                   buffer,
                                                   FBE_EDAL_FLEX_COMP_TEST_MAX_VIPER_ENCL_DATA_BLOCK,
                                                   FBE_EDAL_FLEX_COMP_TEST_MEMORY_CHUNK_SIZE);

 
    return (sas_viper_component_block);  
}
/************************
 * end fbe_edal_flex_comp_test_sas_viper_init_component_block()
 ************************/
