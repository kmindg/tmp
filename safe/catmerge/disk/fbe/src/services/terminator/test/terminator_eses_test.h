#ifndef TERMINATOR_ESES_TEST_H
#define TERMINATOR_ESES_TEST_H

#define MAX_SLOT_NUMBER 15
#define MAX_PHY_NUMBER 36
#define MAX_PS_NUMBER 2
#define MAX_COOLING_NUMBER 4
#define MAX_TEMP_SENSOR_NUMBER 1

/**************************************************
 * some local defines for testing internal routines
 * These are for testing purposes, so not calling any
 * terminator config page parsing routines. So change
 * them when we get a new config. page from Exp group.
 ***************************************************/
#define ARRAY_DEVICE_SLOT_ELEM_OFFSET  0xD8
#define PHY_ELEM_OFFSET 0x10
#define PS_A_OFFSET 0x214
#define PS_B_OFFSET 0x228
#define COOLING_0_OFFSET 0x21C
// Cooling element 1 should be immediately next
// to cooling element with index 0, as they 
// belong to the same PS subenclosure. So it
// does not need a seperate define.
#define COOLING_2_OFFSET 0x230
//There is only 1 temp sensor element for now
#define TEMP_SENSOR_0_OFFSET 0x118

#define CONNECTOR_ELEM_OFFSET   0xAC


// encl status elements
#define LOCAL_LCC_ENCL_STAT_ELEM_OFFSET 0x8
#define PEER_LCC_ENCL_STAT_ELEM_OFFSET 0x134
#define CHASSIS_ENCL_STAT_ELEM_OFFSET 0x20C

// This should be changes whenever the 
// configuration page changes.
#define STAT_PAGE_LENGTH 0x238

// This has information about the device
// to which IO is sent, the current phy,
// drive--etc that are being tested. This
// will be used in IO completion function. 
// Pointer to this will be passed in the
// "Completion Context"
typedef struct eses_test_io_completion_context_s
{
    fbe_u8_t port;
    fbe_u8_t encl_id;
    fbe_u8_t phy;
}eses_test_io_completion_context_t;

typedef struct read_buf_test_cdb_param_s
{
    fbe_u8_t buf_id;
    fbe_u32_t buf_offset;
    fbe_u32_t alloc_len;
}read_buf_test_cdb_param_t;

typedef struct write_buf_test_cdb_param_s
{
    fbe_u8_t buf_id;
    fbe_u32_t buf_offset;
    fbe_u32_t param_list_len;
}write_buf_test_cdb_param_t;

typedef struct mode_cmds_test_mode_sense_cdb_param_s
{
    fbe_u8_t pc;
    fbe_u8_t pg_code;
    fbe_u8_t subpg_code;
}mode_cmds_test_mode_sense_cdb_param_t;

typedef struct mode_cmds_test_mode_select_cdb_param_s
{
    fbe_u16_t param_list_len;
}mode_cmds_test_mode_select_cdb_param_t;

extern ses_pg_code_enum ses_pg_code_encl_stat;
extern ses_pg_code_enum ses_pg_code_emc_encl_stat;
extern ses_pg_code_enum ses_pg_code_config;
extern ses_pg_code_enum ses_pg_code_addl_elem_stat;
extern ses_pg_code_enum ses_pg_code_download_microcode_stat;
extern ses_pg_code_enum ses_pg_code_encl_ctrl;
extern ses_pg_code_enum ses_pg_code_emc_encl_ctrl;
extern ses_pg_code_enum ses_pg_code_str_out;

fbe_sg_element_t * fbe_ldo_test_alloc_memory_with_sg( fbe_u32_t bytes );
void fbe_ldo_test_free_memory_with_sg( fbe_sg_element_t **sg_p );
fbe_status_t terminator_miniport_api_issue_eses_cmd (
    fbe_u32_t port_number, 
    fbe_cpd_device_id_t device_id, 
    fbe_scsi_command_t scsi_cmd,
    PVOID scsi_cmd_input_param,
    fbe_sg_element_t *sg_p);
void init_dev_phy_elems(void);


#endif /* TERMINATOR_ESES_TEST_H */