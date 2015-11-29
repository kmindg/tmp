
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file denali.c
 ***************************************************************************
 *
 * @brief
 *  This scenario tests PDO error injection.
 *
 * @revision
 *   05/18/2012:  Created kothal
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "physical_package_tests.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe_test_configurations.h"
#include "fbe_test_package_config.h"
#include "fbe_test_io_api.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe_media_error_edge_tap.h"
#include "fbe_test_common_utils.h"
#include "pp_utils.h"
#include "fbe/fbe_api_dest_injection_interface.h"
#include "fbe/fbe_dest_service.h"
#include "neit_utils.h"
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_stat_api.h"
#include "fbe/fbe_drive_configuration_interface.h"
#include "fbe/fbe_api_drive_configuration_service_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_scheduler_interface.h"



/*************************
 *   TEST DESCRIPTION
 *************************/
char * denali_short_desc = "Drive Error Injection via DEST";
char * denali_long_desc =
    "\n"
    "The denali scenario tests the error injection via DEST.\n"    
    "\n";
   
/*! @enum DENALI_NUMBER_OF_OBJECTS 
/*  @brief These are the number of objects used in the scenario. 
 *   1 board 
 *   1 port 
 *   1 enclosure 
 *   1 physical drives 
 *   1 logical drives
 */
enum {DENALI_NUMBER_OF_OBJECTS = 5};

/*! @enum DENALI_MAX_DRIVES  
 *  @brief These are the number of drives in this scenario. 
 */
enum {DENALI_MAX_DRIVES = 1};

#define DENALI_LUNS_PER_RAID_GROUP 1
#define DENALI_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def DENALI_DIEH_ERROR_INJECTION_RANGE
 *********************************************************************
 * @brief This range is used when calculating number of errors to 
 *        inject to test DIEH actions.   Range is needed to cover
 *        rounding issues when calculating the error ratio.
 *
 *********************************************************************/
#define DENALI_DIEH_ERROR_INJECTION_RANGE 4

/*!*******************************************************************
 * @var denali_rdgen_context
 *********************************************************************
 * @brief This contains our context object for running rdgen I/O.
 *
 *********************************************************************/
static fbe_api_rdgen_context_t denali_rdgen_context[DENALI_LUNS_PER_RAID_GROUP * 2];



/* STRUCT & GLOBALS */

typedef struct denali_scsi_error_s
{
    fbe_port_request_status_t       port_status;
    fbe_payload_cdb_scsi_status_t   scsi_status;
    fbe_u8_t                        sk, asc, ascq;  /* only used when scsi_status == CC */
} denali_scsi_error_t;

typedef enum {
    DENALI_ACTION_NONE = 0x0,
    DENALI_ACTION_EOL = 0x1,
    DENALI_ACTION_FAIL = 0x2,
} denali_drive_action_bitmap_t; 

typedef struct {
    denali_scsi_error_t             error;
    fbe_u32_t                       num_inserts;
    denali_drive_action_bitmap_t    drive_actions;
    fbe_bool_t                      restore_pvd;
} denali_hc_record_t;

denali_hc_record_t hc_rec[] =
{ /*   error,                                                                                        num_ins, drive_actions,      restore pvd*/
    { {FBE_PORT_REQUEST_STATUS_SUCCESS, FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION,   -1,   -1,   -1}, 0,  DENALI_ACTION_NONE,                   FBE_TRUE},   // no error injection. tests successful QST on first try.
    { {FBE_PORT_REQUEST_STATUS_SUCCESS, FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION, 0x04, 0x3e, 0x03}, 1,  DENALI_ACTION_EOL,                    FBE_TRUE}, 
    { {FBE_PORT_REQUEST_STATUS_SUCCESS, FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION, 0x04, 0x3e, 0x03}, 3,  DENALI_ACTION_EOL|DENALI_ACTION_FAIL, FBE_TRUE},
    { {FBE_PORT_REQUEST_STATUS_SUCCESS, FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION, 0x01, 0x5d, 0x10}, 1,  DENALI_ACTION_EOL,                    FBE_TRUE},  
    { {FBE_PORT_REQUEST_STATUS_SUCCESS, FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION, 0x04, 0x44, 0x00}, 1,  DENALI_ACTION_NONE,                   FBE_TRUE},
    { {FBE_PORT_REQUEST_STATUS_SUCCESS, FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION, 0x04, 0x44, 0x00}, 2,  DENALI_ACTION_NONE,                   FBE_TRUE},
    { {FBE_PORT_REQUEST_STATUS_SUCCESS, FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION, 0x04, 0x44, 0x00}, 3,  DENALI_ACTION_FAIL,                   FBE_TRUE},                                  
    { {FBE_PORT_REQUEST_STATUS_SUCCESS, FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION, 0x04, 0x80, 0x00}, 1,  DENALI_ACTION_FAIL,                   FBE_TRUE},  
    { {FBE_PORT_REQUEST_STATUS_SUCCESS, FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION, 0x02, 0x4c, 0x00}, 1,  DENALI_ACTION_FAIL,                   FBE_TRUE},  
    { {FBE_PORT_REQUEST_STATUS_SUCCESS, FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION, 0x02, 0x04, 0x05}, 1,  DENALI_ACTION_FAIL,                   FBE_TRUE},  
    { {FBE_PORT_REQUEST_STATUS_SUCCESS, FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION, 0x02, 0x31, 0x00}, 1,  DENALI_ACTION_FAIL,                   FBE_FALSE},
/*! @todo The previous test is leaving the drive in the failed state.*/  
//    { {FBE_PORT_REQUEST_STATUS_ABORT_TIMEOUT, FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD,        -1,   -1,   -1}, 1,  DENALI_ACTION_NONE,                   FBE_FALSE},
    { {                                   -1,                               -1,        -1,   -1,   -1}, -1, -1,                                   -1}   /*port_status=-1 is end marker */
};


#define DENALI_RG_CONFIGS 1

/*!*******************************************************************
 * @var denali_raid_group_config_qual
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/

fbe_test_rg_configuration_array_t denali_raid_group_config_qual[DENALI_RG_CONFIGS+1] =   /* +1 for terminator*/
{
    /* Raid 1 configurations.
     */
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
        {2,       0xE000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,    520,            0,         0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */}
};



/*TODO:  remove the fields that are not needed.  -wayne*/

/*! @struct media_error_test_case_t  
 *  @brief This is the structure that defines a test case for
 *         testing media errors.
 */
typedef struct media_error_test_case_s
{
    /*! Size of block exported in bytes.
     */
    fbe_block_size_t exported_block_size;
    /*! Number of blocks per exported optimal block.
     */
    fbe_block_size_t exported_opt_block_size;
    /*! The size in bytes of the imported block. 
     */
    fbe_block_size_t imported_block_size;
    /*! Number of imported blocks in the optimum block size.
     */
    fbe_block_size_t imported_opt_block_size;

    fbe_lba_t read_lba; /*!< Lba to read from. */
    fbe_block_count_t read_blocks; /*!< Blocks to read. */

    fbe_lba_t write_lba;/*!< Lba to write to. */
    fbe_block_count_t write_blocks;/*!< Blocks to write. */

    fbe_lba_t error_lba; /*!< Start physical lba to inject errors. */
    fbe_block_count_t error_blocks; /*! Number of blocks to inject errors. */

    fbe_lba_t read_error_lba; /*!< media error lba for this read */
    fbe_lba_t write_error_lba; /*!< Start logical lba for write errors. */
}
media_error_test_case_t;

/*! @var media_error_520_io_case_array  
 *  @brief This is the set of test cases for testing 520 byte block drives. 
 */
static media_error_test_case_t media_error_520_io_case_array[] =
{
    /* 
     * exp  exp  imp   imp read read  write  write Error Error Read  Write
     * bl   opt  block opt lba  bls   lba    bls   lba   Bl    Error Error
     * size size size  size                                    Lba   Lba 
     *  
     * Simple small block count error cases. 
     * We try to cover cases where the read starts at the beginning 
     * of the error and where the read starts mid-way through the error. 
     * The write always covers the same range as the read so that the 
     * write will fix all the errors. 
     */


    {520,   1,   520,  1,  0,   1,     0,    1,    0,    1,    0,    0},
    /* ADD MORE TEST CONFIGURATIONS HERE ...*/

    /* This is the terminator record, it always should be zero.
     */
    {FBE_TEST_IO_INVALID_FIELD, 0,         0,    0,    0,       0, 0},
   
};
/* end media_error_520_io_case_array[] */



typedef enum dieh_test_action_e
{
    DIEH_TEST_ACTION_RESET = 0,
    DIEH_TEST_ACTION_EOL,
    DIEH_TEST_ACTION_FAIL,
    DIEH_TEST_ACTION_RESET_REACTIVATION,
}dieh_test_action_t;

/*! @struct dieh_test_case_t  
 *  @brief This is the structure that defines a test case for
 *         testing DIEH.
 */
typedef struct dieh_test_case_s
{
    fbe_u32_t bus;
    fbe_u32_t encl;
    fbe_u32_t slot;
    fbe_object_id_t pdo;  /*PDO object ID will be set after drive is created. */
    fbe_scsi_command_t error_opcode;  /*!< opcode to inject */   
    fbe_rdgen_operation_t rdgen_opcode;  /*!< rdgen opcode to send */    
    fbe_lba_t lba; /*!< Start physical lba to inject errors. */
    fbe_block_count_t blocks; /*! Number of blocks to inject errors. */
    fbe_u8_t sk;
    fbe_u8_t asc;
    fbe_u8_t ascq;
    fbe_port_request_status_t port_status;
    fbe_u32_t  num_to_inject;   
    dieh_test_action_t  action_to_verify;
    fbe_object_death_reason_t death_reason;
}
dieh_test_case_t;

/*! @var dieh_test_case_array  
 *  @brief This is the set of test cases for DIEH actions
 */
static dieh_test_case_t dieh_test_case_array[] =
{
    /* 
     * Bus, Encl, Slot, PDO Object ID,         Opcode,           RDGEN Opcode,                  Error  Error, SK,   ASC,  ASCQ, Port Status,                          Num, Verify,                Death Reason
     *                                                                                          lba    Bl                                                             inj  action
     */
    {  0,   0,    5,    FBE_OBJECT_ID_INVALID, FBE_SCSI_READ_16, FBE_RDGEN_OPERATION_READ_ONLY, 100,   1,     0xff, 0xff, 0xff, FBE_PORT_REQUEST_STATUS_ABORT_TIMEOUT, 100, DIEH_TEST_ACTION_RESET_REACTIVATION, 0},    
    {  0,   0,    5,    FBE_OBJECT_ID_INVALID, FBE_SCSI_READ_16, FBE_RDGEN_OPERATION_READ_ONLY, 100,   1,     0x04, 0x44, 0x00, FBE_PORT_REQUEST_STATUS_SUCCESS,       25,  DIEH_TEST_ACTION_EOL, 0},    
    {  0,   0,    5,    FBE_OBJECT_ID_INVALID, FBE_SCSI_READ_16, FBE_RDGEN_OPERATION_READ_ONLY, 100,   1,     0x03, 0x11, 0x00, FBE_PORT_REQUEST_STATUS_SUCCESS,       50,  DIEH_TEST_ACTION_EOL, 0},
    {  0,   0,    5,    FBE_OBJECT_ID_INVALID, FBE_SCSI_READ_16, FBE_RDGEN_OPERATION_READ_ONLY, 100,   1,     0x01, 0x18, 0x00, FBE_PORT_REQUEST_STATUS_SUCCESS,       250, DIEH_TEST_ACTION_EOL, 0},
    {  0,   0,    5,    FBE_OBJECT_ID_INVALID, FBE_SCSI_READ_16, FBE_RDGEN_OPERATION_READ_ONLY, 100,   1,     0x04, 0x44, 0x00, FBE_PORT_REQUEST_STATUS_SUCCESS,       14,  DIEH_TEST_ACTION_RESET, 0},    
    {  0,   0,    5,    FBE_OBJECT_ID_INVALID, FBE_SCSI_READ_16, FBE_RDGEN_OPERATION_READ_ONLY, 100,   1,     0x03, 0x11, 0x00, FBE_PORT_REQUEST_STATUS_SUCCESS,       28,  DIEH_TEST_ACTION_RESET, 0},
    {  0,   0,    5,    FBE_OBJECT_ID_INVALID, FBE_SCSI_READ_16, FBE_RDGEN_OPERATION_READ_ONLY, 100,   1,     0x01, 0x18, 0x00, FBE_PORT_REQUEST_STATUS_SUCCESS,       140, DIEH_TEST_ACTION_RESET, 0},
    {  0,   0,    5,    FBE_OBJECT_ID_INVALID, FBE_SCSI_READ_16, FBE_RDGEN_OPERATION_READ_ONLY, 100,   1,     0x04, 0x44, 0x00, FBE_PORT_REQUEST_STATUS_SUCCESS,       28,  DIEH_TEST_ACTION_FAIL, FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_HARDWARE_THRESHOLD_EXCEEDED},
    {  0,   0,    5,    FBE_OBJECT_ID_INVALID, FBE_SCSI_READ_16, FBE_RDGEN_OPERATION_READ_ONLY, 100,   1,     0x03, 0x11, 0x00, FBE_PORT_REQUEST_STATUS_SUCCESS,       56,  DIEH_TEST_ACTION_FAIL, FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_MEDIA_THRESHOLD_EXCEEDED},
    {  0,   0,    5,    FBE_OBJECT_ID_INVALID, FBE_SCSI_READ_16, FBE_RDGEN_OPERATION_READ_ONLY, 100,   1,     0x01, 0x18, 0x00, FBE_PORT_REQUEST_STATUS_SUCCESS,       280, DIEH_TEST_ACTION_FAIL, FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_RECOVERED_THRESHOLD_EXCEEDED},

    /* ADD MORE TEST CONFIGURATIONS HERE ...*/

    
    {0xff, 0xff, 0xff, } /* terminator record*/   
};



typedef struct {
    fbe_scsi_error_code_t err_code;
    fbe_u8_t sk;
    fbe_u8_t asc;
    fbe_u8_t ascq;
    fbe_bool_t valid_lba;
} denali_fbe_scsi_xlat_rec_t;

/* This table contains every possible FBE_SCSI CC error our sw handles and an ak/asc/ascq that
   maps to it.
*/
denali_fbe_scsi_xlat_rec_t   g_dest_scsi_err_lookup_a[] =
{
    /* value,                                error_code,  valid LBA*/

    {FBE_SCSI_CC_NOERR,                      0x00,0x00,0x01,   1},
    {FBE_SCSI_CC_AUTO_REMAPPED,              0x01,0x18,0x02,   1},
    {FBE_SCSI_CC_RECOVERED_BAD_BLOCK,        0x01,0x18,0x00,   1},
    {FBE_SCSI_CC_RECOVERED_ERR_CANT_REMAP,   0x01,0x18,0x00,   0},
    {FBE_SCSI_CC_DIE_RETIREMENT_START,       0x01,0x18,0xFD,   1},
    {FBE_SCSI_CC_DIE_RETIREMENT_END,         0x01,0x18,0xFE,   1},
    {FBE_SCSI_CC_PFA_THRESHOLD_REACHED,      0x01,0x5D,0x00,   1},
    {FBE_SCSI_CC_RECOVERED_ERR_NOSYNCH,      0x01,0x5C,0x02,   1},
    {FBE_SCSI_CC_RECOVERED_ERR,              0x01,0x5C,0x00,   1},
    {FBE_SCSI_CC_BECOMING_READY,             0x02,0x04,0x01,   1},
    {FBE_SCSI_CC_NOT_SPINNING,               0x02,0x04,0x02,   1},
    {FBE_SCSI_CC_FORMAT_IN_PROGRESS,         0x02,0x04,0x04,   1},
    {FBE_SCSI_CC_NOT_READY,                  0x02,0x00,0x00,   1},    
    {FBE_SCSI_CC_FORMAT_CORRUPTED,           0x02,0x31,0x00,   1},
    {FBE_SCSI_CC_SANITIZE_INTERRUPTED,       0x03,0x31,0x80,   1},
    {FBE_SCSI_CC_HARD_BAD_BLOCK,             0x03,0x11,0x00,   1},
    {FBE_SCSI_CC_MEDIA_ERR_CANT_REMAP,       0x03,0x11,0x00,   0},
    {FBE_SCSI_CC_HARDWARE_ERROR_PARITY,      0x04,0x47,0x00,   1},
    {FBE_SCSI_CC_HARDWARE_ERROR_SELF_TEST,   0x04,0x3E,0x03,   1},
    {FBE_SCSI_CC_HARDWARE_ERROR,             0x04,0x00,0x00,   1},
    {FBE_SCSI_CC_ILLEGAL_REQUEST,            0x05,0x00,0x00,   1},
    {FBE_SCSI_CC_DEV_RESET,                  0x06,0x29,0x00,   1},
    {FBE_SCSI_CC_MODE_SELECT_OCCURRED,       0x06,0x2A,0x00,   1},
    {FBE_SCSI_CC_SYNCH_SUCCESS,              0x06,0x5C,0x01,   1},
    {FBE_SCSI_CC_SYNCH_FAIL,                 0x06,0x5C,0x02,   1},
    {FBE_SCSI_CC_UNIT_ATTENTION,             0x06,0x00,0x00,   1},
    {FBE_SCSI_CC_ABORTED_CMD_PARITY_ERROR,   0x0B,0x47,0x00,   1},
    {FBE_SCSI_CC_ABORTED_CMD,                0x0B,0x00,0x00,   1},
    {FBE_SCSI_CC_UNEXPECTED_SENSE_KEY,       0x07,0x00,0x00,   1},
    {FBE_SCSI_CC_HARDWARE_ERROR_FIRMWARE,    0x04,0x80,0x00,   1},
    {FBE_SCSI_CC_HARDWARE_ERROR_NO_SPARE,    0x04,0x32,0x00,   1},
    {FBE_SCSI_CC_MEDIA_ERR_WRITE_ERROR,      0x03,0x0C,0x00,   1},
    {FBE_SCSI_CC_DEFECT_LIST_ERROR,          0x03,0x19,0x00,   1},
    {FBE_SCSI_CC_SEEK_POSITIONING_ERROR,     0x04,0x15,0x00,   1},
    {FBE_SCSI_CC_SEL_ID_ERROR,               0x06,0xFF,0x00,   1},
    {FBE_SCSI_CC_RECOVERED_WRITE_FAULT,      0x01,0x03,0x00,   1},
    {FBE_SCSI_CC_MEDIA_ERROR_WRITE_FAULT,    0x03,0x03,0x00,   1},
    {FBE_SCSI_CC_HARDWARE_ERROR_WRITE_FAULT, 0x04,0x03,0x00, 1},
    {FBE_SCSI_CC_INTERNAL_TARGET_FAILURE,    0x04,0x44,0x00,   1},
    {FBE_SCSI_CC_RECOVERED_DEFECT_LIST_ERROR, 0x01,0x19,0x00, 1},
    {FBE_SCSI_CC_INVALID_LUN,                0x05,0x25,0x00,   1},
    {FBE_SCSI_LOGICAL_UNIT_FAILED_SELF_CONFIGURATION, 0x02,0x4C,0x00, 1},    
    {FBE_SCSI_CC_SUPER_CAP_FAILURE,          0x01,0x80,0x30,   1},
    {FBE_SCSI_CC_DRIVE_TABLE_REBUILD,        0x02,0x04,0x05,   1},    
    {FBE_SCSI_CC_WRITE_PROTECT,              0x07,0x27,0x00,   1},
    {FBE_SCSI_CC_SENSE_DATA_MISSING,         0x00,0x00,0x00,   1},

    {-1, -1, -1, -1, -1}   // last record marker
};

/*!*******************************************************************
 * @var denali_test_rdgen_context
 *********************************************************************
 * @brief This contains our context objects for running rdgen I/O.
 *
 *********************************************************************/
static fbe_api_rdgen_context_t denali_test_rdgen_context;



/* EXTERNS */
extern void percy_test_insert_drive(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot);
extern void percy_test_remove_drive(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot);
extern void biff_insert_new_drive(fbe_sas_drive_type_t drive_type, fbe_terminator_sas_drive_type_default_info_t *new_page_info_p, fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot);
extern void biff_verify_drive_is_logically_online(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot);
extern void biff_remove_drive(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot);
extern fbe_status_t sleeping_beauty_wait_for_state(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, fbe_lifecycle_state_t state, fbe_u32_t timeout_ms);


/* PROTOTYPES */
static void denali_test_send_tur_to_abort_rla(void);
static void denali_test_case_health_check(void);
static void denali_test_health_check(denali_hc_record_t * hc_case_p, fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot);
static void denali_test_port_error_incidental_abort(fbe_test_rg_configuration_t *rg_config_p, void * context_p);
static void denali_test_mode_select(void);
static void denali_test_dest(fbe_test_rg_configuration_t *rg_config_p, void * context_p);
void denali_dest_verify_injection_complete(fbe_u32_t num_to_inject, fbe_dest_record_handle_t record_handle, fbe_u32_t wait_time_ms);
void denali_test_all_dieh_buckets(void);
void denali_dieh_test_case_set_pdo_id(dieh_test_case_t * case_p);
void denali_create_dest_record_for_dieh_test(dieh_test_case_t * case_p, fbe_u32_t injection_range, fbe_dest_record_handle_t *record_handle_p);
void denali_verify_death_reason(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, fbe_object_death_reason_t in_reason);
void denali_dieh_test_send_io_until_action(dieh_test_case_t *case_p, fbe_dest_record_handle_t dest_rec_handle, fbe_scheduler_debug_hook_t *reset_hook_p, fbe_bool_t prevent_burst_reduction);
void denali_dieh_test_reset_reactivation(dieh_test_case_t *case_p, fbe_dest_record_handle_t dest_rec_handle, fbe_scheduler_debug_hook_t *reset_hook_p);
fbe_bool_t denali_dieh_test_verify_action_occured(dieh_test_case_t *case_p, fbe_dest_record_handle_t dest_rec_handle, fbe_scheduler_debug_hook_t *reset_hook_p);
void denali_disable_background_services(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot);




/* FUNCTIONS */
/*********************************************************************/

denali_fbe_scsi_xlat_rec_t* denali_get_fbe_scsi_xlat_rec(fbe_scsi_error_code_t err_code)
{
    denali_fbe_scsi_xlat_rec_t *ptr = &g_dest_scsi_err_lookup_a[0];
    fbe_bool_t is_found = FBE_FALSE;

    while(ptr->err_code != (fbe_scsi_error_code_t)-1)
    {
        if (ptr->err_code == err_code)
        {
            is_found = FBE_TRUE;
            break;
        }

        ptr++;
    }

    MUT_ASSERT_TRUE(is_found);
    return ptr++;
}

fbe_status_t
fbe_test_dest_utils_init_error_injection_record(fbe_dest_error_record_t * error_injection_record)
{
    fbe_zero_memory(error_injection_record, sizeof(fbe_dest_error_record_t));
    return FBE_STATUS_OK;
}


/*!**************************************************************
 * @fn denali_add_scsi_cc_error_record()
 ****************************************************************
 * @brief
 *   This function adds a scsi check condition error record. 
 *
 * @param case_p - The test case which contains the area to
 *                 inject errors on, to read from and write to.
 * @param record_p - error record to be inserted.
 * @param pdo_object_id - pdo object on which error will be inserted.
 * @param sk/asc/ascq - scsi check condition error
 * @param port_status - port status, normally set by miniport
 *
 * @return - none
 *
 * @author
 *   11/04/2012:  Wayne Garrett - Created 
 *
 ****************************************************************/
void denali_create_scsi_cc_error_record(media_error_test_case_t *const case_p,
                                        fbe_dest_error_record_t* record_p,
                                        fbe_object_id_t pdo_object_id,
                                        fbe_scsi_command_t opcode,
                                        fbe_scsi_sense_key_t sk,
                                        fbe_scsi_additional_sense_code_t asc,
                                        fbe_scsi_additional_sense_code_qualifier_t ascq,
                                        fbe_port_request_status_t port_status)
{    
    /* Initialize the dest injection record.*/
    fbe_api_dest_injection_init_scsi_error_record(record_p);

    record_p->object_id = pdo_object_id;

    if (NULL != case_p)
    {
        record_p->lba_start = case_p->error_lba;
        record_p->lba_end = (case_p->error_lba + case_p->error_blocks) ; 
    }

    record_p->dest_error.dest_scsi_error.scsi_command[0] = opcode;
    record_p->dest_error.dest_scsi_error.scsi_command_count = 1;

    record_p->dest_error.dest_scsi_error.scsi_sense_key = sk;
    record_p->dest_error.dest_scsi_error.scsi_additional_sense_code = asc;
    record_p->dest_error.dest_scsi_error.scsi_additional_sense_code_qualifier = ascq;

    record_p->dest_error.dest_scsi_error.scsi_status = FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION;
    record_p->dest_error.dest_scsi_error.port_status = port_status;

    record_p->num_of_times_to_insert = 1;        
}
/**************************************
 * end denali_create_scsi_cc_error_record()
 **************************************/

/*!**************************************************************
 * @fn denali_create_scsi_error_record()
 ****************************************************************
 * @brief
 *   This function adds a scsi error record.  This is intended for 
 *   all non-checkcondition type of errors.  Use 
 *   denali_add_scsi_cc_error_record for injecting check conditions
 *
 * @param case_p - The test case which contains the area to
 *                 inject errors on, to read from and write to.
 * @param record_p - error record to be inserted.
 * @param pdo_object_id - pdo object on which error will be inserted.
 * @param scsi_status - scsi status, normally set by miniport
 * @param port_status - port status, normally set by miniport
 *
 * @return - none
 *
 * @author
 *   11/04/2012:  Wayne Garrett - Created 
 *
 ****************************************************************/
void denali_create_scsi_error_record(media_error_test_case_t *const case_p,
                                     fbe_dest_error_record_t* record_p,
                                     fbe_object_id_t pdo_object_id,
                                     fbe_scsi_command_t opcode,
                                     fbe_payload_cdb_scsi_status_t scsi_status,
                                     fbe_port_request_status_t port_status)
{
    /* Initialize the dest injection record.*/
    fbe_api_dest_injection_init_scsi_error_record(record_p);

    record_p->object_id = pdo_object_id;

    if (NULL != case_p)
    {
        record_p->lba_start = case_p->error_lba;
        record_p->lba_end = (case_p->error_lba + case_p->error_blocks) ; 
    }

    record_p->dest_error.dest_scsi_error.scsi_command[0] = opcode;
    record_p->dest_error.dest_scsi_error.scsi_command_count = 1;

    record_p->dest_error.dest_scsi_error.scsi_status = scsi_status;
    record_p->dest_error.dest_scsi_error.port_status = port_status;

    record_p->num_of_times_to_insert = 1;
}
/**************************************
 * end denali_create_scsi_error_record()
 **************************************/



/*!**************************************************************
 * @fn denali_add_scsi_command_to_record()
 ****************************************************************
 * @brief
 *   This function modifies the error record to add an additional 
 *   command to inject on.
 *
 * @param record_p - error record to be inserted.
 * @param opcode - command to inject on.
 *
 * @return - none
 *
 * @author
 *   11/27/2012:  Wayne Garrett - Created 
 *
 ****************************************************************/
void denali_add_scsi_command_to_record(fbe_dest_error_record_t* record_p,
                                       fbe_scsi_command_t opcode)
{
    MUT_ASSERT_TRUE_MSG(record_p->dest_error.dest_scsi_error.scsi_command_count < MAX_CMDS, "added too many opcodes");

    record_p->dest_error.dest_scsi_error.scsi_command[record_p->dest_error.dest_scsi_error.scsi_command_count] = opcode;
    record_p->dest_error.dest_scsi_error.scsi_command_count++;    
}
/**************************************
 * end denali_add_scsi_command_to_record()
 **************************************/

/*!**************************************************************
 * @fn denali_add_record_to_dest()
 ****************************************************************
 * @brief
 *   This function adds record to DEST
 *
 * @param record_p - error record to be inserted.
 *
 * @return - none
 *
 * @author
 *   11/27/2012:  Wayne Garrett - Created 
 *
 ****************************************************************/
void denali_add_record_to_dest(fbe_dest_error_record_t* record_p, fbe_dest_record_handle_t * record_handle_p)
{
    fbe_status_t status;

    status = fbe_api_dest_injection_add_record(record_p, record_handle_p);   
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}
/**************************************
 * end denali_add_record_to_dest()
 **************************************/


/*!**************************************************************
 * @fn denali_inject_scsi_error_stop()
 ****************************************************************
 * @brief
 *   This function stops the protocol error injection.
 *  
 * @return - fbe_status_t - FBE_STATUS_OK on success,
 *                        any other value implies error.
 *
 * HISTORY:
 *   05/18/2012:  Created kothal
 *
 ****************************************************************/
fbe_status_t denali_inject_scsi_error_stop(void)
{
    fbe_status_t   status = FBE_STATUS_OK;

    /*set the error using NEIT package. */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: stop error injection.", __FUNCTION__);

    status = fbe_api_dest_injection_stop(); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    return status;
}
/**************************************
 * end denali_inject_scsi_error_stop()
 **************************************/

/*!**************************************************************
 * @fn denali_test_case_media_error(media_error_test_case_t *const case_p,
 *                                  fbe_u32_t slot)
 ****************************************************************
 * @brief
 *   This tests a single case of the denali test.
 *   We insert an error, read from an area, write to an area
 *   and then read back the data written.
 *  
 * @param case_p - The test case which contains the area to
 *                 inject errors on, to read from and write to.
 * @param slot - The drive slot to test.  The test setup two
 *               drives on slots 0, 1.
 *
 * @return - None.
 *
 * HISTORY:
 *   05/18/2012:  Created kothal
 *   11/11/2013:  Wayne Garrett - reworked.
 *
 ****************************************************************/

void denali_test_case_media_error(media_error_test_case_t *const case_p,
                          fbe_u32_t slot)
{
    fbe_status_t status;
    fbe_u32_t object_handle;
    fbe_object_id_t pdo_object_id;    
    fbe_dest_error_record_t record;
    fbe_dest_record_handle_t record_handle = NULL;
    fbe_api_rdgen_context_t *context_p = &denali_test_rdgen_context;     
    
    mut_printf(MUT_LOG_LOW, "=== %s: Test Begin ===\n", __FUNCTION__);

    /* Get the physical drive object id.
     */
    status = fbe_api_get_physical_drive_object_id_by_location(0, 0, slot, &object_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);
    pdo_object_id = (fbe_object_id_t)object_handle;

    mut_printf(MUT_LOG_HIGH, "%s %s start error injection!",__FILE__, __FUNCTION__);

    /*** Test Case:  test media error. */

    /* add scsi error record
     */
    denali_create_scsi_cc_error_record(case_p, &record, pdo_object_id, FBE_SCSI_READ_10,  /*TODO: move opcode into testcase table -wayne.*/
                                       0x03, 0x11, 0x00,                  // sk/asc/ascq 
                                       FBE_PORT_REQUEST_STATUS_SUCCESS);  // port status
    denali_add_scsi_command_to_record(&record, FBE_SCSI_READ_12);
    denali_add_scsi_command_to_record(&record, FBE_SCSI_READ_16);
    denali_add_record_to_dest(&record, &record_handle);

    /* Start the error injection 
     */
    status = fbe_api_dest_injection_start(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Send I/O.  We expect this to get a media error because of the error injected
     * above. 
     */  
    status = fbe_api_rdgen_send_one_io(context_p,
                                    pdo_object_id,                                              // object id
                                    FBE_CLASS_ID_INVALID,                             // class id
                                    FBE_PACKAGE_ID_PHYSICAL,                      // package id
                                    FBE_RDGEN_OPERATION_READ_ONLY,          // rdgen opcode
                                    FBE_RDGEN_PATTERN_LBA_PASS,                // data pattern to generate
                                    case_p->read_lba,                                     // start lba
                                    case_p->read_blocks,                                // min blocks
                                    FBE_RDGEN_OPTIONS_INVALID,                   // rdgen options
                                    0,0,
                                    FBE_API_RDGEN_PEER_OPTIONS_INVALID);   
    
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_GENERIC_FAILURE); 
    mut_printf(MUT_LOG_LOW, "=== %s:  expected media error detected. ===", __FUNCTION__);       

    status = denali_inject_scsi_error_stop();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_dest_injection_remove_all_records();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "=== %s: Test End ===\n", __FUNCTION__);    
    return;
}
/**************************************
 * end denali_test_case_media_error()
 **************************************/

/*!**************************************************************
 * @fn denali_test_case_reservation_conflict(media_error_test_case_t *const case_p,
 *                                           fbe_u32_t slot)
 ****************************************************************
 * @brief
 *   This tests injecting a reservation conflict scsi error.
 *  
 * @param case_p - The test case which contains the area to
 *                 inject errors on, to read from and write to.
 * @param slot - The drive slot to test.  The test setup two
 *               drives on slots 0, 1.
 *
 * @return - None.
 *
 * HISTORY:
 *   11/11/3013  Wayne Garrett - Reworked.
 *  
 ****************************************************************/

void denali_test_case_reservation_conflict(media_error_test_case_t *const case_p,
                                           fbe_u32_t slot)
{
    fbe_status_t status;
    fbe_u32_t object_handle;// logical_object_handle;
    fbe_object_id_t pdo_object_id;//, ldo_object_id;    
    fbe_dest_error_record_t record;
    fbe_dest_record_handle_t record_handle = NULL;
    fbe_api_rdgen_context_t *context_p = &denali_test_rdgen_context;     
    
    mut_printf(MUT_LOG_LOW, "=== %s: Test Begin ===\n", __FUNCTION__);

    /* Get the physical drive object id.
     */
    status = fbe_api_get_physical_drive_object_id_by_location(0, 0, slot, &object_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);
    pdo_object_id = (fbe_object_id_t)object_handle;

    /* Verify if the drive exists.
     */
/*    status = fbe_api_get_logical_drive_object_id_by_location(0, 0, slot, &logical_object_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(logical_object_handle != FBE_OBJECT_ID_INVALID);

    ldo_object_id = (fbe_object_id_t)logical_object_handle;
*/
    mut_printf(MUT_LOG_HIGH, "%s start error injection!", __FUNCTION__);



    /*** Test Case:  test reservation conflict error.  This involves:
                     adding record,
                     start DEST,
                     running single IO,
                     verify injection,
                     stop DEST,
                     cleanup
    */

    denali_create_scsi_error_record(case_p, &record, pdo_object_id, FBE_SCSI_READ_10, 
                                    FBE_PAYLOAD_CDB_SCSI_STATUS_RESERVATION_CONFLICT, // scsi status
                                    FBE_PORT_REQUEST_STATUS_DATA_UNDERRUN);           // port status
    denali_add_scsi_command_to_record(&record, FBE_SCSI_READ_12);
    denali_add_scsi_command_to_record(&record, FBE_SCSI_READ_16);
    denali_add_record_to_dest(&record, &record_handle);

    status = fbe_api_dest_injection_start(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_rdgen_send_one_io(context_p,
                                    pdo_object_id,                                              // object id
                                    FBE_CLASS_ID_INVALID,                             // class id
                                    FBE_PACKAGE_ID_PHYSICAL,                      // package id
                                    FBE_RDGEN_OPERATION_READ_ONLY,          // rdgen opcode
                                    FBE_RDGEN_PATTERN_LBA_PASS,                // data pattern to generate
                                    case_p->read_lba,                                     // start lba
                                    case_p->read_blocks,                                // min blocks
                                    FBE_RDGEN_OPTIONS_INVALID,                   // rdgen options
                                    0,0,
                                    FBE_API_RDGEN_PEER_OPTIONS_INVALID);     
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_GENERIC_FAILURE); // expect failure

    mut_printf(MUT_LOG_LOW, "=== %s:  expected reservation conflict error detected. ===", __FUNCTION__); 

    status = denali_inject_scsi_error_stop();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_dest_injection_remove_all_records();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "=== %s: Test End ===\n", __FUNCTION__);    
    return;
}
/**************************************
 * end denali_test_case_reservation_conflict()
 **************************************/

/*!**************************************************************
 * @fn denali_dest_injection()
 ****************************************************************
 * @brief
 *  This is the main test function of the denali test.
 *  This test will test media error handling.
 *
 * @param  - None.
 *
 * @return - None.
 *
 * HISTORY:
 *   05/18/2012:  Created kothal
 *
 ****************************************************************/

void denali_dest_injection(void)
{
    media_error_test_case_t *case_p = NULL; 
    fbe_status_t status;   

    mut_printf(MUT_LOG_LOW, "\n=== %s: Start ===", __FUNCTION__);

    /* Test sending Test Unit Ready (TUR) to abort Read Look Ahead(RLA) only on
     * Seagate drives that aren't running specific revisions of firmware (or later)
     */
    denali_test_send_tur_to_abort_rla();

    /* Test DIEH Bucket actions 
    */
    denali_test_all_dieh_buckets();


    /* Test DEST error injection
     */
    fbe_test_run_test_on_rg_config(&denali_raid_group_config_qual[0][0], 
                                   NULL,
                                   denali_test_dest,
                                   DENALI_LUNS_PER_RAID_GROUP,
                                   DENALI_CHUNKS_PER_LUN);

    status = fbe_test_sep_util_destroy_all_user_luns();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_test_sep_util_destroy_all_user_raid_groups();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

   
    /* Test mode select does not occur if mode pages match overrides
    */
    denali_test_mode_select();


    /* Test INCIDENTAL_ABORT port error
     */
    fbe_test_run_test_on_rg_config(&denali_raid_group_config_qual[0][0], 
                                   NULL,
                                   denali_test_port_error_incidental_abort,
                                   DENALI_LUNS_PER_RAID_GROUP,
                                   DENALI_CHUNKS_PER_LUN);

    status = fbe_test_sep_util_destroy_all_user_luns();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_test_sep_util_destroy_all_user_raid_groups();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Test basic error handling
     */
    case_p = media_error_520_io_case_array;
    while( case_p->exported_block_size != FBE_TEST_IO_INVALID_FIELD )
    {
        denali_test_case_media_error(case_p, 0 /* slot */);
        denali_test_case_reservation_conflict(case_p, 0 /* slot */);
        case_p++;
    }

    /* Test Health Check
    */
    denali_test_case_health_check();

    mut_printf(MUT_LOG_LOW, "=== %s: End ===\n", __FUNCTION__);
    
    return;
}
/**************************************
 * end denali()
 **************************************/


void denali_load_and_verify(void)
{
    fbe_test_rg_configuration_array_t *array_p = NULL;
    fbe_u32_t test_index;

    array_p = &denali_raid_group_config_qual[0];

    for (test_index = 0; test_index < DENALI_RG_CONFIGS; test_index++)
    {
        fbe_test_sep_util_init_rg_configuration_array(&array_p[test_index][0]);
    }

    fbe_test_sep_util_rg_config_array_load_physical_config(array_p);
    elmo_load_sep_and_neit();
}


/*!**************************************************************
 * @fn denali_unload()
 ****************************************************************
 * @brief
 *  Unload packages.
 *
 * @param none.
 *
 * @return fbe_status_t - FBE_STATUS_OK/FBE_STATUS_GENERIC_FAILURE.
 *
 * @author
 *  11/19/2012  Wayne Garrett - Created. 
 *
 ****************************************************************/
fbe_status_t denali_unload(void)
{
    fbe_status_t status = FBE_STATUS_OK;

#if 0
    status = fbe_api_sim_server_unload_package(FBE_PACKAGE_ID_NEIT);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "unload neit failed");

    status = fbe_api_sim_server_unload_package(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "unload sep failed");

    status = fbe_test_physical_package_tests_config_unload();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
#endif

    fbe_test_sep_util_destroy_neit_sep_physical();

    return status;
}


/*!**************************************************************
 * @fn denali_test_send_tur_to_abort_rla()
 ****************************************************************
 * @brief
 * Verify that sending Test Unit Ready (TUR) to abort Read Look Ahead(RLA)
 * only occurs on Seagate drives that aren't running specific revisions of
 * firmware (or later) by testing the following three things: 
 *   1) VERIFY is still successful even if TUR is not successful.
 *   2) TUR is not sent to abort RLA if VERIFY is not successful.
 *   3) TUR is never sent to abort RLA when the PDO has outstanding IO.
 *
 * @param none.
 *
 * @author
 *   07/24/2014:  Darren Insko - Created.
 *
 ****************************************************************/
static void denali_test_send_tur_to_abort_rla(void)
{
    fbe_api_rdgen_context_t rdgen_contexts[2];
    fbe_terminator_sas_drive_type_default_info_t drive_page_info = {0};
    fbe_sas_drive_type_t drive_type = FBE_SAS_DRIVE_SIM_520;
    fbe_u8_t tla[FBE_SCSI_INQUIRY_TLA_SIZE] = "005049197PWR";
    fbe_u8_t fw_rev[FBE_SCSI_INQUIRY_REVISION_SIZE] = {"CS19"};
    const fbe_u32_t bus = 0;
    const fbe_u32_t encl = 0;
    const fbe_u32_t slot = 5;
    fbe_object_id_t pdo_object_id;
    fbe_dest_error_record_t tur_record_in = {0};    
    fbe_dest_error_record_t verify_record_in = {0};    
    fbe_dest_record_handle_t tur_record_handle = NULL;        
    fbe_dest_record_handle_t verify_record_handle = NULL;        
    fbe_status_t status;  

    mut_printf(MUT_LOG_LOW, "=== %s: Test Begin ===\n", __FUNCTION__);

    /* Insert test drive and get its PDO's object ID */
    mut_printf(MUT_LOG_LOW, "=== %s: Get PDO of inserted %d_%d_%d whose firmware indicates RLAs must be aborted ===\n",
               __FUNCTION__, bus, encl, slot);
    fbe_api_terminator_sas_drive_get_default_page_info(drive_type, &drive_page_info);
    fbe_copy_memory(&drive_page_info.inquiry[FBE_SCSI_INQUIRY_TLA_OFFSET], tla, sizeof(tla));
    fbe_copy_memory(&drive_page_info.inquiry[FBE_SCSI_INQUIRY_REVISION_OFFSET], fw_rev, sizeof(fw_rev));
    biff_insert_new_drive(drive_type, &drive_page_info,
                          bus, encl, slot);

    status = fbe_test_pp_util_verify_pdo_state(bus, encl, slot,
                                              FBE_LIFECYCLE_STATE_READY, 10000 /* ms */);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &pdo_object_id);          
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(pdo_object_id != FBE_OBJECT_ID_INVALID);

    fbe_api_physical_drive_clear_error_counts(pdo_object_id, FBE_PACKET_FLAG_NO_ATTRIB);

    /* TEST 1 - VERIFY is still successful even if TUR is not successful */
    /* setup DEST record for TUR */
    mut_printf(MUT_LOG_LOW, "=== %s: Test case 1 - VERIFY is still successful even if TUR is not successful ===", __FUNCTION__);
    denali_create_scsi_cc_error_record(NULL, &tur_record_in, pdo_object_id, FBE_SCSI_TEST_UNIT_READY,  
                                       0x02, 0x04, 0x01,                  // sk/asc/ascq 
                                       FBE_PORT_REQUEST_STATUS_SUCCESS);  // port status
    tur_record_in.num_of_times_to_insert = 1;                
    denali_add_record_to_dest(&tur_record_in, &tur_record_handle);
    mut_printf(MUT_LOG_LOW, "=== %s: Test case 1 - Setup to inject an error on TUR ===", __FUNCTION__);

    /* Start the error injection */
    status = fbe_api_dest_injection_start(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        
    /* Generate just one VERIFY, which should succeed */
    mut_printf(MUT_LOG_LOW, "=== %s: Test case 1 - Sending 1 VERIFY IO to PDO of %d_%d_%d ===",
               __FUNCTION__, bus, encl, slot);
    status = fbe_api_rdgen_send_one_io(&rdgen_contexts[0],                  // Pointer to the rdgen context
                                       pdo_object_id,                       // PDO object ID
                                       FBE_CLASS_ID_INVALID,                // Class ID not needed
                                       FBE_PACKAGE_ID_PHYSICAL,             // Testing physical package and down
                                       FBE_RDGEN_OPERATION_VERIFY,          // rdgen opcode
                                       FBE_RDGEN_PATTERN_LBA_PASS,          // Data pattern to generate
                                       0x100,                               // Starting LBA offset
                                       1,                                   // min blocks
                                       FBE_RDGEN_OPTIONS_INVALID,           // rdgen options
                                       0,0,
                                       FBE_API_RDGEN_PEER_OPTIONS_INVALID); 
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't send 1 VERIFY IO");  /* Expect VERIFY IO to succeed */

    /* Determine that a single error was injected because the TUR was sent */
    denali_dest_verify_injection_complete(1, tur_record_handle, 10000 /*ms*/);
    mut_printf(MUT_LOG_LOW, "=== %s: Test case 1 - TUR was sent but was not successful due to injected error ===", __FUNCTION__);
    
    /* Cleanup the error injection */
    status = fbe_api_dest_injection_stop(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    status = fbe_api_dest_injection_remove_all_records();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_api_physical_drive_clear_error_counts(pdo_object_id, FBE_PACKET_FLAG_NO_ATTRIB);

    /* TEST 2 - TUR is not sent to abort RLA if VERIFY is not successful. */
    /* setup DEST record for TUR */
    mut_printf(MUT_LOG_LOW, "=== %s: Test case 2 - TUR is not sent to abort RLA if VERIFY is not successful ===", __FUNCTION__);
    denali_create_scsi_cc_error_record(NULL, &tur_record_in, pdo_object_id, FBE_SCSI_TEST_UNIT_READY,  
                                       0x02, 0x04, 0x01,                  // sk/asc/ascq 
                                       FBE_PORT_REQUEST_STATUS_SUCCESS);  // port status
    tur_record_in.num_of_times_to_insert = 1;                
    denali_add_record_to_dest(&tur_record_in, &tur_record_handle);

    /* setup DEST record for VERIFY */
    denali_create_scsi_cc_error_record(NULL, &verify_record_in, pdo_object_id, FBE_SCSI_VERIFY_16,  
                                       0x03, 0x11, 0x00,                  // sk/asc/ascq 
                                       FBE_PORT_REQUEST_STATUS_SUCCESS);  // port status
    denali_add_scsi_command_to_record(&verify_record_in, FBE_SCSI_VERIFY_10);
    verify_record_in.num_of_times_to_insert = 3;
    denali_add_record_to_dest(&verify_record_in, &verify_record_handle);
    mut_printf(MUT_LOG_LOW, "=== %s: Test case 2 - Setup to inject an error on TUR & VERIFY ===", __FUNCTION__);

    /* Start the error injection */
    status = fbe_api_dest_injection_start(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
 
    /* Generate just one VERIFY, which should fail because of the media error injected above */
    mut_printf(MUT_LOG_LOW, "=== %s: Test case 2 - Sending 1 VERIFY IO to PDO of %d_%d_%d ===",
               __FUNCTION__, bus, encl, slot);
    status = fbe_api_rdgen_send_one_io(&rdgen_contexts[0],                  // Pointer to the rdgen context
                                       pdo_object_id,                       // PDO object ID
                                       FBE_CLASS_ID_INVALID,                // Class ID not needed
                                       FBE_PACKAGE_ID_PHYSICAL,             // Testing physical package and down
                                       FBE_RDGEN_OPERATION_VERIFY,          // rdgen opcode
                                       FBE_RDGEN_PATTERN_LBA_PASS,          // Data pattern to generate
                                       0x100,                               // Starting LBA offset
                                       1,                                   // min blocks
                                       FBE_RDGEN_OPTIONS_INVALID,           // rdgen options
                                       0,0,
                                       FBE_API_RDGEN_PEER_OPTIONS_INVALID); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_GENERIC_FAILURE); /* Expect VERIFY IO to fail */
    mut_printf(MUT_LOG_LOW, "=== %s: Test case 2 - Expected media error detected. ===", __FUNCTION__);       

    /* Determine that a single error was injected because the VERIFY was sent */
    denali_dest_verify_injection_complete(3, verify_record_handle, 10000 /*ms*/);
    mut_printf(MUT_LOG_LOW, "=== %s: Test case 2 - VERIFY was retried but was never successful due to injected error ===", __FUNCTION__);
    /* Determine that no error was injected because the TUR was never sent */
    denali_dest_verify_injection_complete(0, tur_record_handle, 0 /*ms*/);
    mut_printf(MUT_LOG_LOW, "=== %s: Test case 2 - TUR was not sent because error was not injected ===", __FUNCTION__);
    
    /* Cleanup the error injection */
    status = fbe_api_dest_injection_stop(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    status = fbe_api_dest_injection_remove_all_records();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_api_physical_drive_clear_error_counts(pdo_object_id, FBE_PACKET_FLAG_NO_ATTRIB);

    /* TEST 3 - TUR is never sent to abort RLA when the PDO has outstanding IO. */
    /* setup DEST record for TUR */
    mut_printf(MUT_LOG_LOW, "=== %s: Test case 3 - TUR is never sent to abort RLA when the PDO has outstanding IO ===", __FUNCTION__);
    denali_create_scsi_cc_error_record(NULL, &tur_record_in, pdo_object_id, FBE_SCSI_TEST_UNIT_READY,  
                                       0x02, 0x04, 0x01,                  // sk/asc/ascq 
                                       FBE_PORT_REQUEST_STATUS_SUCCESS);  // port status
    tur_record_in.num_of_times_to_insert = 1;                
    denali_add_record_to_dest(&tur_record_in, &tur_record_handle);
    mut_printf(MUT_LOG_LOW, "=== %s: Test case 3 - Setup to inject an error on TUR ===", __FUNCTION__);

    /* Start the error injection */
    status = fbe_api_dest_injection_start(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        
    /* Setup rdgen context */
    mut_printf(MUT_LOG_LOW, "=== %s: Test case 3 - Initializing rdgen context for PDO of %d_%d_%d ===",
               __FUNCTION__, bus, encl, slot);
    status = fbe_api_rdgen_test_context_init(&rdgen_contexts[1],             // Pointer to the rdgen context
                                             pdo_object_id,                  // PDO object ID
                                             FBE_CLASS_ID_INVALID,           // Class ID not needed
                                             FBE_PACKAGE_ID_PHYSICAL,        // Testing physical package and down
                                             FBE_RDGEN_OPERATION_WRITE_ONLY, // rdgen opcode
                                             FBE_RDGEN_PATTERN_LBA_PASS,     // Data pattern to generate
                                             0,                              // Number of passes (not needed because of manual stop)
                                             0,                              // Number of IOs (not used)
                                             0,                              // Time before stop (not needed because of manual stop)
                                             30,                             // Thread count
                                             FBE_RDGEN_LBA_SPEC_RANDOM,      // Seek pattern
                                             0x100,                          // Starting LBA offset
                                             0x0,                            // Minimum LBA is 0
                                             FBE_LBA_INVALID,                // Stroke the entirety of the PDO
                                             FBE_RDGEN_BLOCK_SPEC_RANDOM,    // Random IO size
                                             16,                             // Minimum IO size
                                             16);                            // Maximum IO size, same as Min for constancy
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't initialize rdgen context!");

    /* Kick off WRITE IOs */
    mut_printf(MUT_LOG_LOW, "=== %s: Test case 3 - Start sending WRITE IOs to PDO of %d_%d_%d ===",
               __FUNCTION__, bus, encl, slot);
    status = fbe_api_rdgen_start_tests(&rdgen_contexts[1],
                                       FBE_PACKAGE_ID_NEIT,
                                       1);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't start WRITE IOs!");
    fbe_api_sleep(5000);  /* give some time for IO to crank up just in case */

    /* Generate just one VERIFY, which should succeed */
    mut_printf(MUT_LOG_LOW, "=== %s: Test case 3 - Sending 1 VERIFY IO to PDO of %d_%d_%d ===",
               __FUNCTION__, bus, encl, slot);
    status = fbe_api_rdgen_send_one_io(&rdgen_contexts[0],                  // Pointer to the rdgen context
                                       pdo_object_id,                       // PDO object ID
                                       FBE_CLASS_ID_INVALID,                // Class ID not needed
                                       FBE_PACKAGE_ID_PHYSICAL,             // Testing physical package and down
                                       FBE_RDGEN_OPERATION_VERIFY,          // rdgen opcode
                                       FBE_RDGEN_PATTERN_LBA_PASS,          // Data pattern to generate
                                       0x100,                               // Starting LBA offset
                                       1,                                   // min blocks
                                       FBE_RDGEN_OPTIONS_INVALID,           // rdgen options
                                       0,0,
                                       FBE_API_RDGEN_PEER_OPTIONS_INVALID); 
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't send 1 VERIFY IO");  /* Expect VERIFY IO to succeed */

    /* Determine that the TUR was never sent by verifying that no error was inserted  */
    denali_dest_verify_injection_complete(0, tur_record_handle, 10000 /*ms*/);
    mut_printf(MUT_LOG_LOW, "=== %s: Test case 3 - TUR was never sent because error was never injected ===", __FUNCTION__);
    
    /* Stop sending WRITE IOs */
    mut_printf(MUT_LOG_LOW, "=== %s: Test case 3 - Stop sending WRITE IOs to PDO of %d_%d_%d ===",
               __FUNCTION__, bus, encl, slot);
    status = fbe_api_rdgen_stop_tests(&rdgen_contexts[1], 1);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't stop WRITE IOs!");

    /* Cleanup the error injection */
    status = fbe_api_dest_injection_stop(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    status = fbe_api_dest_injection_remove_all_records();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_api_physical_drive_clear_error_counts(pdo_object_id, FBE_PACKET_FLAG_NO_ATTRIB);

    /* Remove test drive */
    biff_remove_drive(bus, encl, slot);

    mut_printf(MUT_LOG_LOW, "=== %s: Test End ===\n", __FUNCTION__);
}
/*****************************************
 * end denali_test_send_tur_to_abort_rla()
 *****************************************/


/*!**************************************************************
 * @fn denali_test_health_check_success()
 ****************************************************************
 * @brief
 *   Test health check works.
 *
 * @author
 *   11/15/2012:  Wayne Garrett - Created.
 *
 ****************************************************************/
static void denali_test_health_check_success(void)
{
    fbe_object_id_t pdo_obj;
    fbe_physical_drive_dieh_stats_t dieh_stats = {0};
    fbe_dest_error_record_t record;
    fbe_dest_record_handle_t record_handle = NULL;
    fbe_status_t status;
    const fbe_u32_t SLEEP_INTERVAL_MSEC = 2000;
    const fbe_u32_t MAX_SLEEP_MSEC = 20000;
    fbe_u32_t sleep_total = 0;
    const fbe_u32_t service_time_limit_ms = 500;
    /* For now hardcoding to 0_0_0.  Eventually we should get this from RG info.*/
    const fbe_u32_t bus = 0;
    const fbe_u32_t encl = 0;
    const fbe_u32_t slot = 0;


    /* Get the physical drive object id.
     */
    status = fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &pdo_obj);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(pdo_obj != FBE_OBJECT_ID_INVALID);


    /* Enable HC and clear error counts. 
    */
    fbe_api_drive_configuration_set_control_flag(FBE_DCS_HEALTH_CHECK_ENABLED, FBE_TRUE /*enable*/);
    fbe_api_drive_configuration_set_service_timeout(service_time_limit_ms);

    fbe_api_physical_drive_clear_error_counts(pdo_obj, FBE_PACKET_FLAG_NO_ATTRIB);


    /* Setup error injection 
    */
    fbe_test_dest_utils_init_error_injection_record(&record);
    record.object_id = pdo_obj;
    record.dest_error_type = FBE_DEST_ERROR_TYPE_PORT;
    record.dest_error.dest_scsi_error.port_status = FBE_PORT_REQUEST_STATUS_ABORT_TIMEOUT;
    record.lba_start = 0;
    record.lba_end = 0x1000 ; 
    record.dest_error.dest_scsi_error.scsi_command[0] = FBE_SCSI_WRITE_10;
    record.dest_error.dest_scsi_error.scsi_command[1] = FBE_SCSI_WRITE_12;
    record.dest_error.dest_scsi_error.scsi_command[2] = FBE_SCSI_WRITE_16;
    record.dest_error.dest_scsi_error.scsi_command_count = 3;
    record.num_of_times_to_insert = 1;
    record.delay_io_msec = service_time_limit_ms + 100; /* just long enough to trip HC*/

    status = fbe_api_dest_injection_add_record(&record, &record_handle);    
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_dest_injection_start(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    /* Generate IO which will trigger health check.  IO should be failed back.
    */
    /* TODO: a better way is to bind a lun and send IO through RAID, then way we can injected multiple
       errors into the same IO.  For now just sending a single IO which will be delayed longer then the
       service time.*/
    status = fbe_api_rdgen_send_one_io(&denali_test_rdgen_context,
                                    pdo_obj,                                              // object id
                                    FBE_CLASS_ID_INVALID,                             // class id
                                    FBE_PACKAGE_ID_PHYSICAL,                      // package id
                                    FBE_RDGEN_OPERATION_WRITE_ONLY,          // rdgen opcode
                                    FBE_RDGEN_PATTERN_LBA_PASS,                // data pattern to generate
                                    0x100,                                     // start lba
                                    1,                                // min blocks
                                    FBE_RDGEN_OPTIONS_INVALID,                   // rdgen options
                                    0,0,
                                    FBE_API_RDGEN_PEER_OPTIONS_INVALID); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_GENERIC_FAILURE);  /* IO failed */

    /* Verify health check occurred
    */
    do
    {        

        status = fbe_api_physical_drive_get_dieh_info(pdo_obj, &dieh_stats, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);            
        MUT_ASSERT_INT_NOT_EQUAL_MSG(dieh_stats.error_counts.healthCheck_error_ratio, FBE_STAT_INVALID_RATIO, "DIEH table not loaded");

        if (dieh_stats.error_counts.healthCheck_error_ratio > 0)
        {
            break;
        }

        mut_printf(MUT_LOG_LOW, "%s health check hasn't completed yet. waiting...", __FUNCTION__); 
        fbe_api_sleep(SLEEP_INTERVAL_MSEC);
        sleep_total += SLEEP_INTERVAL_MSEC;

        MUT_ASSERT_INT_EQUAL_MSG( (sleep_total < MAX_SLEEP_MSEC), FBE_TRUE, "Timeout wating for table update to complete");

    } while (1);


    /* cleanup 
    */
    status = fbe_api_dest_injection_stop(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_dest_injection_remove_all_records();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}


/*!**************************************************************
 * @fn denali_test_case_health_check()
 ****************************************************************
 * @brief
 *   Loops over all health check test cases.
 *
 * @author
 *   11/11/2013:  Wayne Garrett - Created.
 *
 ****************************************************************/
static void denali_test_case_health_check(void)
{
    denali_hc_record_t * hc_case_p = &hc_rec[0];
    const fbe_u32_t bus = 0;  
    const fbe_u32_t encl = 0;
    const fbe_u32_t slot = 4;
    fbe_status_t status;

    /* Do Health Check testing.*/
    mut_printf(MUT_LOG_LOW, "=== %s: starting health check test cases ===\n", __FUNCTION__);

    percy_test_insert_drive(bus, encl, slot);  /* use this drive for testing.*/

    status = fbe_test_pp_util_verify_pdo_state(bus, encl, slot, FBE_LIFECYCLE_STATE_READY, 30000 /* wait 30sec */);                                                
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "first time insert - PDO not Ready");

    status = fbe_test_sep_drive_verify_pvd_state(bus, encl, slot, FBE_LIFECYCLE_STATE_READY, 30000 /* wait 30sec */);  
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "first time insert - PVD not Ready");

    while (hc_case_p->error.port_status != (fbe_port_request_status_t)-1)
    {        
        denali_test_health_check(hc_case_p, bus, encl, slot);                                 
        hc_case_p++;
    }
    mut_printf(MUT_LOG_LOW, "=== %s: ending health check test cases ===\n", __FUNCTION__);
}


/*!**************************************************************
 * @fn denali_test_health_check()
 ****************************************************************
 * @brief
 *   Test health check works.
 *
 * @param sk,asc,ascq - error to inject on QST cmd.  
 * @param num_inserts - number times to inject to test retry logic
 * @param expected_state - lifecycle state expected after HC completes.
 *
 * @author
 *   03/26/2013:  Wayne Garrett - Created.
 *
 ****************************************************************/
static void denali_test_health_check(denali_hc_record_t * hc_case_p, fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot)
{
    fbe_object_id_t pdo_obj, pvd_obj;
    fbe_dest_error_record_t record;
    fbe_dest_record_handle_t record_handle = NULL;
    fbe_status_t status;
    const fbe_u32_t service_time_limit_ms = 500;
    fbe_lifecycle_state_t pdo_expected_state = FBE_LIFECYCLE_STATE_READY;
    fbe_lifecycle_state_t pvd_expected_state = FBE_LIFECYCLE_STATE_READY;
    fbe_api_terminator_device_handle_t drive_handle;

    mut_printf(MUT_LOG_LOW, "--- %s prtSt=%d scsiSt=%d cc=%02x|%02x|%02x num=%d expct_drive_actions=0x%x", 
               __FUNCTION__, hc_case_p->error.port_status, hc_case_p->error.scsi_status, 
               hc_case_p->error.sk, hc_case_p->error.asc, hc_case_p->error.ascq, hc_case_p->num_inserts, 
               hc_case_p->drive_actions);

    if (hc_case_p->drive_actions == DENALI_ACTION_NONE)
    {
        pdo_expected_state = FBE_LIFECYCLE_STATE_READY;
        pvd_expected_state = FBE_LIFECYCLE_STATE_READY;
    }
    
    if (hc_case_p->drive_actions & DENALI_ACTION_EOL)
    {
        pdo_expected_state = FBE_LIFECYCLE_STATE_READY;
        pvd_expected_state = FBE_LIFECYCLE_STATE_FAIL;
    }

    if (hc_case_p->drive_actions & DENALI_ACTION_FAIL)
    {
        pdo_expected_state = FBE_LIFECYCLE_STATE_FAIL;
        pvd_expected_state = FBE_LIFECYCLE_STATE_FAIL;
    }
    

    /* Get the physical drive object id.
     */
    status = fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &pdo_obj);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(pdo_obj != FBE_OBJECT_ID_INVALID);

    status = fbe_api_provision_drive_get_obj_id_by_location_from_topology(bus, encl, slot, &pvd_obj);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(pvd_obj != FBE_OBJECT_ID_INVALID);

    /* Enable HC and clear error counts. 
    */
    fbe_api_drive_configuration_set_control_flag(FBE_DCS_HEALTH_CHECK_ENABLED, FBE_TRUE /*enable*/);
    fbe_api_drive_configuration_set_service_timeout(service_time_limit_ms);

    fbe_api_physical_drive_clear_error_counts(pdo_obj, FBE_PACKET_FLAG_NO_ATTRIB);


    /* Setup error injection.  Inject a delay in one IO to trip service timeout.  Then
       Inject an error on the QST diag command.  Verify this fails the drive.
    */

    /* 1. Inject delay to trigger HC.
    */
    fbe_test_dest_utils_init_error_injection_record(&record);
    record.object_id = pdo_obj;
    record.dest_error_type = FBE_DEST_ERROR_TYPE_PORT;
    record.dest_error.dest_scsi_error.port_status = FBE_PORT_REQUEST_STATUS_ABORT_TIMEOUT;
    record.lba_start = 0;
    record.lba_end = 0x1000; 
    record.dest_error.dest_scsi_error.scsi_command[0] = FBE_SCSI_WRITE_10;
    record.dest_error.dest_scsi_error.scsi_command[1] = FBE_SCSI_WRITE_12;
    record.dest_error.dest_scsi_error.scsi_command[2] = FBE_SCSI_WRITE_16;
    record.dest_error.dest_scsi_error.scsi_command_count = 3;
    record.num_of_times_to_insert = 1;
    record.delay_io_msec = service_time_limit_ms + 100; /* just long enough to trip HC*/

    status = fbe_api_dest_injection_add_record(&record, &record_handle);    
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);



    /* 2. Inject QST error
    */
    if (hc_case_p->num_inserts > 0)
    {    
        if (hc_case_p->error.scsi_status == FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION)
        {
            denali_create_scsi_cc_error_record(NULL, &record, pdo_obj, FBE_SCSI_SEND_DIAGNOSTIC,  
                                               hc_case_p->error.sk, hc_case_p->error.asc, hc_case_p->error.ascq,                 
                                               FBE_PORT_REQUEST_STATUS_SUCCESS);  // port status
        }
        else  /* inject port error */
        {
            denali_create_scsi_error_record(NULL, &record, pdo_obj, FBE_SCSI_SEND_DIAGNOSTIC, 
                                            hc_case_p->error.scsi_status, hc_case_p->error.port_status);
        }

        record.num_of_times_to_insert = hc_case_p->num_inserts;
        denali_add_record_to_dest(&record, &record_handle);
    }
    

    status = fbe_api_dest_injection_start(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    /* Generate IO which will trigger health check.  IO should be failed back.
    */
    /* TODO: a better way is to bind a lun and send IO through RAID, then way we can injected multiple
       errors into the same IO.  For now just sending a single IO which will be delayed longer then the
       service time.*/
    status = fbe_api_rdgen_send_one_io(&denali_test_rdgen_context,
                                    pdo_obj,                            // object id
                                    FBE_CLASS_ID_INVALID,               // class id
                                    FBE_PACKAGE_ID_PHYSICAL,            // package id
                                    FBE_RDGEN_OPERATION_WRITE_ONLY,     // rdgen opcode
                                    FBE_RDGEN_PATTERN_LBA_PASS,         // data pattern to generate
                                    0x100,                              // start lba
                                    1,                                  // min blocks
                                    FBE_RDGEN_OPTIONS_INVALID,          // rdgen options
                                    0,0,
                                    FBE_API_RDGEN_PEER_OPTIONS_INVALID);
    mut_printf(MUT_LOG_LOW, "%s rdgen io status = %d\n", __FUNCTION__, status);
    //MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_GENERIC_FAILURE, "unexpected RDGEN IO status");  /* IO failed */

    /* 3. verify PDO transitioned to correct state
    */
    if (pdo_expected_state == FBE_LIFECYCLE_STATE_READY)
    {   
        fbe_api_sleep(200);  /* give some time in case there's state transition */
    }

    status = fbe_api_wait_for_object_lifecycle_state(pdo_obj, pdo_expected_state, 30000,  /* 30 sec*/ FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "PDO not in expected state");

    /* Pull and re-insert drive
    */
    mut_printf(MUT_LOG_LOW, "%s pull drive: %d_%d_%d", __FUNCTION__, bus, encl, slot);
    status = fbe_test_pp_util_pull_drive(bus, 
                                         encl, 
                                         slot,
                                         &drive_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    /* wait for pvd to transition to fail */
    status = fbe_api_wait_for_object_lifecycle_state(pvd_obj, FBE_LIFECYCLE_STATE_FAIL, 30000 /*30 sec*/, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "failed - pvd not failed");
    mut_printf(MUT_LOG_LOW, "%s drive: %d_%d_%d in failed state", __FUNCTION__, bus, encl, slot);


    /* Re-insert the same drive
     */
    mut_printf(MUT_LOG_LOW, "%s re-insert drive: %d_%d_%d", __FUNCTION__, bus, encl, slot);
    status = fbe_test_pp_util_reinsert_drive(bus, 
                                             encl, 
                                             slot,
                                             drive_handle);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    if (hc_case_p->restore_pvd)
    {    
        /* wait for pvd to transition to expected state */
        status = fbe_api_wait_for_object_lifecycle_state(pvd_obj, pvd_expected_state, 30000 /*30 sec*/, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "failed - pvd not in expected state");
    
        if (hc_case_p->drive_actions & DENALI_ACTION_FAIL)
        {
            /* make sure PVD persisted faults are cleared before starting next test*/
            status = fbe_api_provision_drive_clear_drive_fault(pvd_obj);
            MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "failed clearing pvd fault flag");

            /* If EOL is not set then PVD will transition to ready.   Change the expected state.*/
            if (!(hc_case_p->drive_actions & DENALI_ACTION_EOL))
            {
                pvd_expected_state = FBE_LIFECYCLE_STATE_READY;
            }

            /* wait for PDO and PVD to transition to expected state */
            fbe_api_sleep(200);
            status = fbe_test_pp_util_verify_pdo_state(bus, encl, slot, FBE_LIFECYCLE_STATE_READY, 30000 /* wait 30sec */);                                                
            MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "PDO not Ready after drive fault cleared.");

            status = fbe_api_wait_for_object_lifecycle_state(pvd_obj, pvd_expected_state, 30000 /*30 sec*/, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "failed - pvd not in expected state after drive fault cleared");
        }       
    
        /* clear EOL */
        if (hc_case_p->drive_actions & DENALI_ACTION_EOL)
        {
            fbe_u32_t i;
            for (i=0; i<2; i++) 
            {
                status = fbe_api_provision_drive_clear_eol_state(pvd_obj, FBE_PACKET_FLAG_NO_ATTRIB);
                MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "failed clearing pvd eol flag");
    
                status = fbe_api_wait_for_object_lifecycle_state(pvd_obj, FBE_LIFECYCLE_STATE_READY, 10000 /*10 sec*/, FBE_PACKAGE_ID_SEP_0);
                if (status == FBE_STATUS_OK)
                {
                    break;
                }
            }
            MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "failed - pvd not ready");
        }
    }


    /* commenting out.  We need at least 2 errors for dieh ratio to show accurate value */
#if 0  
    /* Verify health check occurred
    */
    do
    {        

        status = fbe_api_physical_drive_get_dieh_info(pdo_obj, &dieh_stats, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);            
        MUT_ASSERT_INT_NOT_EQUAL_MSG(dieh_stats.error_counts.healthCheck_error_ratio, FBE_STAT_INVALID_RATIO, "DIEH table not loaded");

        if (dieh_stats.error_counts.healthCheck_error_ratio > 0)
        {
            break;
        }

        mut_printf(MUT_LOG_LOW, "%s health check hasn't completed yet. waiting...", __FUNCTION__); 
        fbe_api_sleep(SLEEP_INTERVAL_MSEC);
        sleep_total += SLEEP_INTERVAL_MSEC;

        MUT_ASSERT_INT_EQUAL_MSG( (sleep_total < MAX_SLEEP_MSEC), FBE_TRUE, "Timeout wating for table update to complete");

    } while (1);
#endif

    /* Stop error injection
    */
    status = fbe_api_dest_injection_stop(); 
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Failed DEST stop");

    status = fbe_api_dest_injection_remove_all_records();
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Failed DEST remove all rec");

}


/*!**************************************************************
 * @fn denali_test_port_error_incidental_abort()
 ****************************************************************
 * @brief
 *   Test that the port error INCIDENTAL_ABORT causes PDO to go
 *   into Activate on first occurrence.  This is a callback function
 *   from fbe_test_run_test_on_rg_config(), which runs this test
 *   against a provided set of RG configs.
 *
 * @param rg_config_p -   RAID group configuration
 * @param context_p -  Context that was passed to fbe_test_run_test_on_rg_config().
 *
 * @author
 *   03/26/2013:  Wayne Garrett - Created.
 *
 ****************************************************************/
static void denali_test_port_error_incidental_abort(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
        fbe_dest_error_record_t record = {0};       
        fbe_dest_record_handle_t record_handle; 
        fbe_api_rdgen_context_t context = {0};
        fbe_object_id_t pdo_object_id, pvd_object_id;
        fbe_api_provision_drive_info_t provision_drive_info = {0};
        fbe_api_get_block_edge_info_t block_edge_info = {0};
        fbe_u32_t bus = 0;
        fbe_u32_t encl = 0;
        fbe_u32_t slot = 0;
        fbe_status_t status;


        mut_printf(MUT_LOG_LOW, "=== %s: Test Begin ===\n", __FUNCTION__);
        
        status = fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &pdo_object_id);          
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        status = fbe_api_provision_drive_get_obj_id_by_location_from_topology(bus, encl, slot, &pvd_object_id);        
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);


        /* Setup DEST for error injection 
        */
        fbe_api_dest_injection_init_scsi_error_record(&record);
              
        record.object_id = pdo_object_id;
    
        record.lba_start = 0x100;
        record.lba_end =   0x100; 
                    
        record.dest_error.dest_scsi_error.scsi_command[0] = FBE_SCSI_WRITE_10;
        record.dest_error.dest_scsi_error.scsi_command[1] = FBE_SCSI_WRITE_16;
        record.dest_error.dest_scsi_error.scsi_command_count = 2;
    
        record.dest_error_type = FBE_DEST_ERROR_TYPE_PORT;        
        record.dest_error.dest_port_error.port_status = FBE_PORT_REQUEST_STATUS_INCIDENTAL_ABORT;
        record.num_of_times_to_insert = 1;

        status = fbe_api_dest_injection_add_record(&record, &record_handle);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Start DEST 
        */
        status = fbe_api_dest_injection_start(); 
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


        /* Add debug hook to hold PDO in Activate state so we can verify PDO and PVD states 
        */
        status = fbe_api_scheduler_add_debug_hook_pp(pdo_object_id,
                                                     SCHEDULER_MONITOR_STATE_PHYSICAL_DRIVE_OBJECT_ACTIVATE,
                                                     FBE_PHYSICAL_DRIVE_OBJECT_SUBSTATE_EDGES_NOT_READY,
                                                     0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Send IO which an INCIDENTAL_ABORT will be injected
        */
        status = fbe_api_rdgen_send_one_io(&context,
                                        pdo_object_id,                    // object id
                                        FBE_CLASS_ID_INVALID,             // class id
                                        FBE_PACKAGE_ID_PHYSICAL,          // package id
                                        FBE_RDGEN_OPERATION_WRITE_ONLY,   // rdgen opcode
                                        FBE_RDGEN_PATTERN_LBA_PASS,       // data pattern to generate
                                        0x100,                            // start lba
                                        1,                                // min blocks
                                        FBE_RDGEN_OPTIONS_INVALID,        // rdgen options
                                        0,0,
                                        FBE_API_RDGEN_PEER_OPTIONS_INVALID);           
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_GENERIC_FAILURE); 


        /* Verify States
        */
        status = fbe_test_pp_util_verify_pdo_state(bus, encl, slot, FBE_LIFECYCLE_STATE_ACTIVATE, 30000 /* wait 30sec */);                                                
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "PDO didn't transition to ACTIVATE");
        
        fbe_api_sleep(3000);      /* give some time for info to propagate */

        /* verify Link Fault set
        */
        status = fbe_api_get_block_edge_info(pvd_object_id, 0, &block_edge_info, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status); 
        MUT_ASSERT_TRUE(block_edge_info.path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_LINK_FAULT);        

        /* verify PVD set SLF
        */
        status = fbe_api_provision_drive_get_info(pvd_object_id, &provision_drive_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status); 
        MUT_ASSERT_INT_NOT_EQUAL(provision_drive_info.slf_state, FBE_PROVISION_DRIVE_SLF_NONE); 


        /* cleanup 
        */
        status = fbe_api_scheduler_del_debug_hook_pp(pdo_object_id, 
                                                     SCHEDULER_MONITOR_STATE_PHYSICAL_DRIVE_OBJECT_ACTIVATE, 
                                                     FBE_PHYSICAL_DRIVE_OBJECT_SUBSTATE_EDGES_NOT_READY, 
                                                     0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE); 
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status); 

        status = fbe_api_dest_injection_stop(); 
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_api_dest_injection_remove_all_records();
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        mut_printf(MUT_LOG_LOW, "=== %s: Test End ===\n", __FUNCTION__);
}


/*!**************************************************************
 * @fn denali_test_mode_select()
 ****************************************************************
 * @brief
 *   This function tests two things; 
 *   1) mode select is not issued if mode pages match overrides.
 *   2) mode select is issued if mode pages don't match.
 *
 * @param void
 *
 * @author
 *   11/20/2013:  Wayne Garrett - Created.
 *
 ****************************************************************/
static void denali_test_mode_select(void)
{
    fbe_status_t status;
    fbe_scheduler_debug_hook_t hook = {0};
    fbe_object_id_t pdo_object_id;
    fbe_u32_t wait_time;
    const fbe_u32_t sleep_interval = 100; /*msec*/

    mut_printf(MUT_LOG_LOW, "\n=== %s: Test Begin ===", __FUNCTION__);

    status = fbe_api_get_physical_drive_object_id_by_location(0, 0, 0, &pdo_object_id);  /* test drive 0_0_0 */
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_api_scheduler_add_debug_hook_pp(pdo_object_id,
                                                 SCHEDULER_MONITOR_STATE_PHYSICAL_DRIVE_OBJECT_ACTIVATE,
                                                 FBE_PHYSICAL_DRIVE_OBJECT_SUBSTATE_MODE_SELECT,
                                                 0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_CONTINUE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* TEST 1 - verify mode select is not issued when mode pages haven't changed. 
    */

    /* This will force a mode select only if pages have changed.  Since pages
       haven't change, it's expeccted the debug hook will NOT be hit.
    */
    fbe_api_physical_drive_mode_select(pdo_object_id);

    fbe_api_sleep(4000);

    hook.object_id = pdo_object_id;    
    hook.monitor_state = SCHEDULER_MONITOR_STATE_PHYSICAL_DRIVE_OBJECT_ACTIVATE;
    hook.monitor_substate = FBE_PHYSICAL_DRIVE_OBJECT_SUBSTATE_MODE_SELECT;
    hook.check_type = SCHEDULER_CHECK_STATES;
    hook.action = SCHEDULER_DEBUG_ACTION_CONTINUE;
    
    status = fbe_api_scheduler_get_debug_hook_pp(&hook);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    if (hook.counter != 0)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "hook counter is %lld", hook.counter);
        MUT_FAIL_MSG("hook counter is not zero.");
    }

    mut_printf(MUT_LOG_LOW, "=== %s: Test 1 passed - Mode Select not called  ===", __FUNCTION__);


    /* TEST 2 - verify mode select is issued when mode pages have changed. 
    */

    status = fbe_api_physical_drive_set_enhanced_queuing_timer(pdo_object_id, 1000, 1000);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait for debug hook to be hit*/
    for(wait_time=0; wait_time<10000; wait_time+=sleep_interval)
    {
        status = fbe_api_scheduler_get_debug_hook_pp(&hook);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (hook.counter > 0){
            break;
        }
        fbe_api_sleep(sleep_interval);
    }
    MUT_ASSERT_INT_EQUAL_MSG((int)hook.counter, 1, "hook counter incorrect");

    mut_printf(MUT_LOG_LOW, "=== %s: Test 2 passed - Mode Select called  ===", __FUNCTION__);


    /* CLEAN UP
    */
    status = fbe_api_scheduler_del_debug_hook_pp(pdo_object_id,
                                                 SCHEDULER_MONITOR_STATE_PHYSICAL_DRIVE_OBJECT_ACTIVATE,
                                                 FBE_PHYSICAL_DRIVE_OBJECT_SUBSTATE_MODE_SELECT,
                                                 0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_CONTINUE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== %s: Test End ===", __FUNCTION__);
}


/*!**************************************************************
 * @fn denali_test_dest()
 ****************************************************************
 * @brief
 *   This function tests DEST error injection.  It does the
 *   following
 *   1) verify num_to_inject is not exceeded with no delay.
 *   2) verify num_to_inject is not exceeded with delay.
 *
 * @param void
 *
 * @author
 *   01/13/2014:  Wayne Garrett - Created.
 *
 ****************************************************************/
static void denali_test_dest(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_api_rdgen_context_t *rdgen_context_p = &denali_rdgen_context[0];
    fbe_object_id_t pdo;
    fbe_dest_error_record_t record_in = {0};    
    fbe_dest_record_handle_t record_handle = NULL;        
    fbe_status_t status;  
    fbe_u32_t i;

    mut_printf(MUT_LOG_LOW, "\n=== %s: Test Start ===", __FUNCTION__);

    status = fbe_api_get_physical_drive_object_id_by_location(rg_config_p->rg_disk_set[0].bus, 
                                                              rg_config_p->rg_disk_set[0].enclosure, 
                                                              rg_config_p->rg_disk_set[0].slot, 
                                                              &pdo);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* setup DEST record */
    denali_create_scsi_cc_error_record(NULL, &record_in, pdo, FBE_SCSI_READ_16,  
                                       0x01, 0x18, 0x00,                  // sk/asc/ascq 
                                       FBE_PORT_REQUEST_STATUS_SUCCESS);  // port status
    denali_add_scsi_command_to_record(&record_in, FBE_SCSI_WRITE_16);
    denali_add_scsi_command_to_record(&record_in, FBE_SCSI_VERIFY_16);
    denali_add_scsi_command_to_record(&record_in, FBE_SCSI_WRITE_SAME_16);
    denali_add_scsi_command_to_record(&record_in, FBE_SCSI_READ_10);
    denali_add_scsi_command_to_record(&record_in, FBE_SCSI_WRITE_10);
    denali_add_scsi_command_to_record(&record_in, FBE_SCSI_VERIFY_10);
    denali_add_scsi_command_to_record(&record_in, FBE_SCSI_WRITE_SAME_10);

    

    /* run heavy IO */
    big_bird_start_io(rg_config_p, &rdgen_context_p, 30, 4096, 
                      FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                      FBE_TEST_RANDOM_ABORT_TIME_INVALID, FBE_API_RDGEN_PEER_OPTIONS_INVALID);

    fbe_api_sleep(3000);  /* give some time for IO to crank up.  not sure if I need to, but just in case*/

    for (i=1; i<=10; i++)
    {
        record_in.num_of_times_to_insert = i;                
        denali_add_record_to_dest(&record_in, &record_handle);

        /* Start the error injection 
         */
        status = fbe_api_dest_injection_start(); 
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        
        /* Verify 
        */
        denali_dest_verify_injection_complete(record_in.num_of_times_to_insert, record_handle, 10000 /*ms*/);
    
        /* cleanup 
        */
        status = fbe_api_dest_injection_stop(); 
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
        status = fbe_api_dest_injection_remove_all_records();
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    for (i=1; i<=10; i++)
    {
        record_in.num_of_times_to_insert = i;  
        record_in.delay_io_msec = 200;  

        denali_add_record_to_dest(&record_in, &record_handle);

        /* Start the error injection 
         */
        status = fbe_api_dest_injection_start(); 
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Verify 
        */
        denali_dest_verify_injection_complete(record_in.num_of_times_to_insert, record_handle, 10000 /*ms*/);

        /* cleanup 
        */
        status = fbe_api_dest_injection_stop(); 
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_api_dest_injection_remove_all_records();
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }


    mut_printf(MUT_LOG_LOW, "=== %s: Test End ===\n", __FUNCTION__);
}

/*!**************************************************************
 * @fn denali_dest_verify_injection_complete()
 ****************************************************************
 * @brief
 *   Verify error injection has completed and correct number of
 *   errors injected.
 *
 * @param num_to_inject - number to verify
 * @param record_handle - handle to DEST record that will be verified.
 * @param wait_time_ms  - millisecods to wait for error injection to complete.
 *
 * @author
 *   01/13/2014:  Wayne Garrett - Created.
 *
 ****************************************************************/
void denali_dest_verify_injection_complete(fbe_u32_t num_to_inject, fbe_dest_record_handle_t record_handle, fbe_u32_t wait_time_ms)
{
    fbe_dest_error_record_t record_out = {0};
    fbe_u32_t time;
    const fbe_u32_t sleep_interval_ms = 200;

    for (time=0; time<wait_time_ms;  time+=sleep_interval_ms)
    {
        fbe_api_dest_injection_get_record(&record_out, record_handle);
        if (!record_out.is_active_record && record_out.times_inserted > 0)
        {
            /* record finished.  it's now deactivated*/
            break;
        }
        fbe_api_sleep(sleep_interval_ms);
    }    
    MUT_ASSERT_INT_EQUAL(record_out.times_inserted, num_to_inject);   /* verify num_to_inject not exceeded */
    MUT_ASSERT_INT_EQUAL(record_out.is_active_record, FBE_FALSE);  /* verify record injecting done */
}

/*!**************************************************************
 * @fn denali_dest_verify_num_injected()
 ****************************************************************
 * @brief
 *   Verify error injection has completed and correct number of
 *   errors injected.
 *
 * @param num_to_inject - number to verify
 * @param range -  range within num_to_inject which is considered valid.
 * @param record_handle - handle to DEST record that will be verified.
 * @param wait_time_ms  - millisecods to wait for error injection to complete.
 *
 * @author
 *   01/28/2014:  Wayne Garrett - Created.
 *
 ****************************************************************/
void denali_dest_verify_num_injected(fbe_u32_t num_to_inject, fbe_u32_t range, fbe_dest_record_handle_t record_handle, fbe_u32_t wait_time_ms)
{
    fbe_dest_error_record_t record_out = {0};
    fbe_u32_t time;
    const fbe_u32_t sleep_interval_ms = 200;

    for (time=0; time<wait_time_ms;  time+=sleep_interval_ms)
    {
        fbe_api_dest_injection_get_record(&record_out, record_handle);
        if (!record_out.is_active_record && record_out.times_inserted > 0)
        {
            /* record finished.  it's now deactivated*/
            break;
        }
        fbe_api_sleep(sleep_interval_ms);
    }    

    if (record_out.times_inserted < num_to_inject-range  || record_out.times_inserted > num_to_inject+range)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Failed.  num_injected:%d expected:%d +/-:%d", __FUNCTION__, record_out.times_inserted, num_to_inject, range);
        MUT_FAIL();
    }   
}

/*!**************************************************************
 * @fn denali_test_all_dieh_buckets()
 ****************************************************************
 * @brief
 *   Test all DIEH bucket actions.  Test will loop over all dieh
 *   test cases and do the following:
 *   1. create new drive 
 *   2. disable background services to control IO to drive
 *   3. Send IO to PDO until action occurs, then verifies it.
 *   4. remove drive and cleanup for next test.
 *
 * @param none.
 *
 * @author
 *   01/28/2014:  Wayne Garrett - Created.
 *
 ****************************************************************/
void denali_test_all_dieh_buckets(void)
{
    dieh_test_case_t * case_p = &dieh_test_case_array[0];
    fbe_dest_record_handle_t dest_rec_handle;
    fbe_u32_t case_num = 1;
    fbe_status_t status;
    fbe_scheduler_debug_hook_t  reset_hook;

    /* initialize hook state */
    reset_hook.monitor_state = SCHEDULER_MONITOR_STATE_PHYSICAL_DRIVE_OBJECT_ACTIVATE;
    reset_hook.monitor_substate = FBE_PHYSICAL_DRIVE_OBJECT_SUBSTATE_RESET;
    reset_hook.check_type = SCHEDULER_CHECK_STATES;
    reset_hook.action = SCHEDULER_DEBUG_ACTION_CONTINUE;
    reset_hook.val1 = 0;
    reset_hook.val2 = 0;


    while (case_p->bus != 0xFF)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Test Begin - case %d ===", __FUNCTION__, case_num);

        biff_insert_new_drive(FBE_SAS_DRIVE_SIM_520, NULL, case_p->bus, case_p->encl, case_p->slot);
        biff_verify_drive_is_logically_online(case_p->bus, case_p->encl, case_p->slot);

        denali_dieh_test_case_set_pdo_id(case_p);

        /* setup debug hook */
        reset_hook.object_id = case_p->pdo;  
        status = fbe_api_scheduler_add_debug_hook_pp(reset_hook.object_id, reset_hook.monitor_state, reset_hook.monitor_substate,
                                                     reset_hook.val1, reset_hook.val2, 
                                                     reset_hook.check_type, reset_hook.action);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        denali_disable_background_services(case_p->bus, case_p->encl, case_p->slot);

        denali_create_dest_record_for_dieh_test(case_p, DENALI_DIEH_ERROR_INJECTION_RANGE, &dest_rec_handle);        

        status = fbe_api_dest_injection_start(); 
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        if (case_p->action_to_verify == DIEH_TEST_ACTION_RESET_REACTIVATION)
        {
            denali_dieh_test_reset_reactivation(case_p, dest_rec_handle, &reset_hook);
        }
        else
        {
            denali_dieh_test_send_io_until_action(case_p, dest_rec_handle, &reset_hook, FBE_TRUE);        
        }

        /* cleanup 
        */
        status = fbe_api_dest_injection_stop(); 
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_api_dest_injection_remove_all_records();
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_api_scheduler_del_debug_hook_pp(reset_hook.object_id, reset_hook.monitor_state, reset_hook.monitor_substate,
                                                     reset_hook.val1, reset_hook.val2, 
                                                     reset_hook.check_type, reset_hook.action);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        biff_remove_drive(case_p->bus, case_p->encl, case_p->slot);

        mut_printf(MUT_LOG_TEST_STATUS, "=== %s: Test End - case %d ===", __FUNCTION__, case_num);
        case_p++;
        case_num++;
    }
}

/*!**************************************************************
 * @fn denali_dieh_test_case_set_pdo_id()
 ****************************************************************
 * @brief
 *   Utility function to set PDO ID in the dieh_test_case.
 * 
 * @param case_p - dieh test case
 *
 * @author
 *   01/28/2014:  Wayne Garrett - Created.
 *
 ****************************************************************/
void denali_dieh_test_case_set_pdo_id(dieh_test_case_t * case_p)
{
    fbe_object_id_t pdo;
    fbe_status_t status;

    status = fbe_api_get_physical_drive_object_id_by_location(case_p->bus, case_p->encl, case_p->slot, &pdo);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_TRUE(pdo != FBE_OBJECT_ID_INVALID);

    case_p->pdo = pdo;
}


/*!**************************************************************
 * @fn denali_create_dest_record_for_dieh_test()
 ****************************************************************
 * @brief
 *   Utility function to create DEST record for dieh_test_case.
 * 
 * @param case_p - dieh test case
 * @param injection_range - to account for rounding errors when calculating DIEH error ratio.
 * @param record_handle_p - handle to record created.
 *
 * @author
 *   01/28/2014:  Wayne Garrett - Created.
 *
 ****************************************************************/
void denali_create_dest_record_for_dieh_test(dieh_test_case_t * case_p, fbe_u32_t injection_range, fbe_dest_record_handle_t *record_handle_p)
{
    fbe_dest_error_record_t record = {0};

    if (case_p->sk == 0xff)
    {
        denali_create_scsi_error_record(NULL, &record, case_p->pdo, case_p->error_opcode, 
                                        FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD, case_p->port_status);
    }
    else
    {
        denali_create_scsi_cc_error_record(NULL, &record, case_p->pdo, case_p->error_opcode,  
                                           case_p->sk, case_p->asc, case_p->ascq,  
                                           case_p->port_status);  
    }

    record.lba_start = case_p->lba;
    record.lba_end = case_p->lba + case_p->blocks -1;
    record.num_of_times_to_insert = case_p->num_to_inject + injection_range;

    denali_add_record_to_dest(&record, record_handle_p);
}


/*!**************************************************************
 * @fn denali_verify_death_reason()
 ****************************************************************
 * @brief
 *   Utility function to verify PDO death reason
 * 
 * @param bus, encl, slot - drive location
 * @param in_reason - death reason
 *
 * @author
 *   01/28/2014:  Wayne Garrett - Created.
 *
 ****************************************************************/
void denali_verify_death_reason(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, fbe_object_death_reason_t in_reason)
{
    fbe_status_t status;
    fbe_object_death_reason_t reason;
    fbe_object_id_t pdo;

    status = fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &pdo);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_TRUE(pdo != FBE_OBJECT_ID_INVALID);
    sleeping_beauty_wait_for_state(bus, encl, slot, FBE_LIFECYCLE_STATE_FAIL, 10000);
    status = fbe_api_get_object_death_reason(pdo, &reason, NULL, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(reason, in_reason);
}


/*!**************************************************************
 * @fn denali_dieh_test_send_io_until_action()
 ****************************************************************
 * @brief
 *   Sends IOs one at a time until DIEH takes an action.   Action
 *   is then verified. 
 * 
 * @param case_p - dieh test case
 * @param dest_rec_handle - handle to DEST error injection record
 * @param reset_hook_p - debug hook to verify
 * @param prevent_burst_reduction - if TRUE a sleep is added to prevent
 *            DIEH Burst Reduction.
 * @author
 *   01/28/2014:  Wayne Garrett - Created.
 *
 ****************************************************************/
void denali_dieh_test_send_io_until_action(dieh_test_case_t *case_p, fbe_dest_record_handle_t dest_rec_handle, fbe_scheduler_debug_hook_t *reset_hook_p, fbe_bool_t prevent_burst_reduction)
{
    fbe_api_rdgen_context_t *rdgen_context_p = &denali_test_rdgen_context;  
    fbe_u32_t ios_sent = 0;   
    fbe_bool_t has_action_occured = FBE_FALSE;
    fbe_lifecycle_state_t life_state;

    mut_printf(MUT_LOG_LOW, "=== %s: ===\n", __FUNCTION__);
    
    /* send IO until expected action occurs.*/

    while (!has_action_occured)
    {
        /* Guarantee we are always ready before sending IO */
        fbe_api_get_object_lifecycle_state(case_p->pdo, &life_state, FBE_PACKAGE_ID_PHYSICAL);
        if (life_state == FBE_LIFECYCLE_STATE_READY)
        {
            fbe_api_rdgen_send_one_io(rdgen_context_p,
                                      case_p->pdo,                        // object id
                                      FBE_CLASS_ID_INVALID,               // class id
                                      FBE_PACKAGE_ID_PHYSICAL,            // package id
                                      case_p->rdgen_opcode,               // rdgen opcode
                                      FBE_RDGEN_PATTERN_LBA_PASS,         // data pattern to generate
                                      case_p->lba,                        // start lba
                                      case_p->blocks,                     // min blocks
                                      FBE_RDGEN_OPTIONS_INVALID,          // rdgen options
                                      0,0,
                                      FBE_API_RDGEN_PEER_OPTIONS_INVALID); 
            ios_sent++; 
        }
        else
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s: PDO not Ready. state:%d, ios:%d, action:%d", __FUNCTION__, life_state, ios_sent, case_p->action_to_verify);  
            /*continue on.  PDO must be processing a DIEH action we're not looking for. */
        }
        
        has_action_occured = denali_dieh_test_verify_action_occured(case_p, dest_rec_handle, reset_hook_p);

        if (prevent_burst_reduction)
        {
            fbe_api_sleep(101); 
        }
    }

    mut_printf(MUT_LOG_TEST_STATUS, "%s: IOs sent [%d] before action occurred [%d]", __FUNCTION__, ios_sent, case_p->action_to_verify);  
}

/*!**************************************************************
 * @fn denali_dieh_test_reset_reactivation()
 ****************************************************************
 * @brief
 *   Verifies reset reactivation by doing the following...
 *   1) Injects IO errors until reset occurs.
 *   2) Send successful IO until ratio fads below low water mark.
 *   3) Inject more errors until reset occurs again.
 * 
 * @param case_p - dieh test case
 * @param dest_rec_handle - handle to DEST error injection record
 * @param reset_hook_p - debug hook to verify
 * @author
 *   01/28/2014:  Wayne Garrett - Created.
 *
 ****************************************************************/
void denali_dieh_test_reset_reactivation(dieh_test_case_t *case_p, fbe_dest_record_handle_t dest_rec_handle, fbe_scheduler_debug_hook_t *reset_hook_p)
{
    fbe_api_rdgen_context_t *rdgen_context_p = &denali_test_rdgen_context;  
    fbe_u32_t ios_sent = 0;   
    fbe_physical_drive_dieh_stats_t dieh_stats;
    fbe_lifecycle_state_t life_state;
    fbe_status_t status;
    fbe_bool_t has_test_finished = FBE_FALSE;
    fbe_bool_t has_reset_occurred = FBE_FALSE;     /* step 1*/
    fbe_bool_t has_ratio_faded = FBE_FALSE;       /* step 2*/
    

    mut_printf(MUT_LOG_LOW, "=== %s: ===\n", __FUNCTION__);
    

    while (!has_test_finished)
    {
        /* Guarantee we are always ready before sending IO */
        fbe_api_get_object_lifecycle_state(case_p->pdo, &life_state, FBE_PACKAGE_ID_PHYSICAL);
        if (life_state == FBE_LIFECYCLE_STATE_READY)
        {
            fbe_api_rdgen_send_one_io(rdgen_context_p,
                                      case_p->pdo,                        // object id
                                      FBE_CLASS_ID_INVALID,               // class id
                                      FBE_PACKAGE_ID_PHYSICAL,            // package id
                                      case_p->rdgen_opcode,               // rdgen opcode
                                      FBE_RDGEN_PATTERN_LBA_PASS,         // data pattern to generate
                                      case_p->lba,                        // start lba
                                      case_p->blocks,                     // min blocks
                                      FBE_RDGEN_OPTIONS_INVALID,          // rdgen options
                                      0,0,
                                      FBE_API_RDGEN_PEER_OPTIONS_INVALID); 
            ios_sent++; 

            if (ios_sent%1000==0)
            {
                mut_printf(MUT_LOG_TEST_STATUS, "%s: ios:%d, action:%d", __FUNCTION__, ios_sent, case_p->action_to_verify);  
            }
        }
        else
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s: PDO not Ready. state:%d, ios:%d, action:%d", __FUNCTION__, life_state, ios_sent, case_p->action_to_verify);  
            fbe_api_sleep(200);  /* give time to come back to ready */
        }
         
        /* 1.  check for first reset. */       
        if (!has_reset_occurred)
        {
            status = fbe_api_scheduler_get_debug_hook_pp(reset_hook_p);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
            if (reset_hook_p->counter == 1)
            {                          
                 mut_printf(MUT_LOG_TEST_STATUS, "%s: 1st Reset Hit. IOs sent:%d action:%d", __FUNCTION__, ios_sent, case_p->action_to_verify);   
                /* stop error injection so ratio will start fading. */
                status = fbe_api_dest_injection_stop(); 
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                has_reset_occurred = FBE_TRUE;
                  
            }
            continue; 
        }

        /* 2. check that ratio faded below low watermark. */
        if (!has_ratio_faded)
        {
            status = fbe_api_physical_drive_get_dieh_info(case_p->pdo, &dieh_stats, FBE_PACKET_FLAG_NO_ATTRIB);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);  
            if (dieh_stats.error_counts.link_error_ratio <= 10)   /* hardcoded to always use link bucket for testing this */
            {
                mut_printf(MUT_LOG_TEST_STATUS, "%s: Ratio Faded. IOs sent:%d action:%d", __FUNCTION__, ios_sent, case_p->action_to_verify); 
                /* start error injection again, to check for reset activation again */
                status = fbe_api_dest_injection_start(); 
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                has_ratio_faded = FBE_TRUE;               
            }   
            continue;         
        }

        /* 3. check that 2nd reset occurs. */
        status = fbe_api_scheduler_get_debug_hook_pp(reset_hook_p);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        if (reset_hook_p->counter == 2)
        {              
            mut_printf(MUT_LOG_TEST_STATUS, "%s: 2nd Reset Hit. IOs sent:%d action:%d", __FUNCTION__, ios_sent, case_p->action_to_verify);              
            has_test_finished = FBE_TRUE;
        }
    }    
}



/*!**************************************************************
 * @fn denali_dieh_test_verify_action_occured()
 ****************************************************************
 * @brief
 *   Utility function to verify DIEH test case action. 
 * 
 * @param case_p - dieh test case
 * @param dest_rec_handle - handle to DEST error injection record
 * @param reset_hook_p - debug hook to verify
 *
 * @author
 *   01/28/2014:  Wayne Garrett - Created.
 *
 ****************************************************************/
fbe_bool_t denali_dieh_test_verify_action_occured(dieh_test_case_t *case_p, fbe_dest_record_handle_t dest_rec_handle, fbe_scheduler_debug_hook_t *reset_hook_p)
{
    fbe_bool_t has_action_occured = FBE_FALSE;
    const fbe_u32_t sleep_interval_ms = 200;
    fbe_u32_t time;
    fbe_status_t status;
    fbe_object_id_t pvd;
    fbe_api_get_block_edge_info_t block_edge_info;

    fbe_lifecycle_state_t life_state = FBE_LIFECYCLE_STATE_INVALID;   
    switch(case_p->action_to_verify)
    {
        case DIEH_TEST_ACTION_FAIL:
            fbe_api_get_object_lifecycle_state(case_p->pdo, &life_state, FBE_PACKAGE_ID_PHYSICAL);
            if (life_state != FBE_LIFECYCLE_STATE_READY)  /* we are transitioning. is this the expected action? */
            {
                if (life_state != FBE_LIFECYCLE_STATE_PENDING_FAIL &&
                    life_state != FBE_LIFECYCLE_STATE_FAIL)
                {                    
                    break; /* not the expected action.  keep going. */
                }
                sleeping_beauty_wait_for_state(case_p->bus, case_p->encl, case_p->slot, FBE_LIFECYCLE_STATE_FAIL, 5000 /*ms*/);
                denali_dest_verify_num_injected(case_p->num_to_inject, DENALI_DIEH_ERROR_INJECTION_RANGE, dest_rec_handle, 1000 /*wait ms*/);
                denali_verify_death_reason(case_p->bus, case_p->encl, case_p->slot, case_p->death_reason);
                has_action_occured = FBE_TRUE;
            }        
            break;

        case DIEH_TEST_ACTION_RESET:
            fbe_api_get_object_lifecycle_state(case_p->pdo, &life_state, FBE_PACKAGE_ID_PHYSICAL);
            if (life_state != FBE_LIFECYCLE_STATE_READY)  /* we are transitioning. is this the expected action? */
            {
                if (life_state != FBE_LIFECYCLE_STATE_PENDING_ACTIVATE &&
                    life_state != FBE_LIFECYCLE_STATE_ACTIVATE)
                {                    
                    mut_printf(MUT_LOG_TEST_STATUS, "%s: Not an expected state transition for reset action. state:%d", __FUNCTION__, life_state);
                    MUT_FAIL();                    
                }            
                /* wait for reset. If it doesn't occur then some other action happened, so continue on. */
                for (time=0; time<5000/*ms*/; time+=sleep_interval_ms)
                {
                    /* verify reset occured */      
                    status = fbe_api_scheduler_get_debug_hook_pp(reset_hook_p);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                    
                    if (reset_hook_p->counter == 1)
                    {
                        denali_dest_verify_num_injected(case_p->num_to_inject, DENALI_DIEH_ERROR_INJECTION_RANGE, dest_rec_handle, 1000 /*wait ms*/);
                        has_action_occured = FBE_TRUE;
                        break;
                    }
                    else if (reset_hook_p->counter > 1)
                    {
                        mut_printf(MUT_LOG_TEST_STATUS, "%s: Verify Reset Action - Too many resets = %llx", __FUNCTION__, reset_hook_p->counter);
                        MUT_FAIL();   
                    }
                    fbe_api_sleep(sleep_interval_ms);
                }
            }
            break;

        case DIEH_TEST_ACTION_EOL:
            status = fbe_api_provision_drive_get_obj_id_by_location_from_topology(case_p->bus, case_p->encl, case_p->slot, &pvd);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            /* check if EOL set */
            status = fbe_api_get_block_edge_info(pvd, 0, &block_edge_info, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status); 
            if (block_edge_info.path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE)
            {
                denali_dest_verify_num_injected(case_p->num_to_inject, DENALI_DIEH_ERROR_INJECTION_RANGE, dest_rec_handle, 1000 /*wait ms*/);
                has_action_occured = FBE_TRUE;
            }                        
            break;

        default:
            mut_printf(MUT_LOG_TEST_STATUS, "%s: action not implemented %d", __FUNCTION__, case_p->action_to_verify);
            MUT_FAIL();
    }

    return has_action_occured;
}


/*!**************************************************************
 * @fn denali_disable_background_services()
 ****************************************************************
 * @brief
 *   Disables PVD's background services
 * 
 * @param bus, encl, slot - drive location
 * 
 * @author
 *   01/28/2014:  Wayne Garrett - Created.
 *
 ****************************************************************/
void denali_disable_background_services(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot)
{
    fbe_status_t status;
    fbe_object_id_t pvd;

    status = fbe_api_provision_drive_get_obj_id_by_location_from_topology(bus, encl, slot, &pvd);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_test_sep_util_provision_drive_disable_zeroing(pvd);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_test_sep_util_provision_drive_disable_verify (pvd);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

}


/*!**************************************************************
 * end file denali.c
 ***************************************************************/                                  
