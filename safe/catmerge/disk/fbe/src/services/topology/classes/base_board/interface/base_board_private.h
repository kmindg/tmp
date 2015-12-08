#ifndef BASE_BOARD_PRIVATE_H
#define BASE_BOARD_PRIVATE_H

#include "fbe_base_transport.h"
#include "fbe_base_board.h"
#include "base_discovering_private.h"
#include "fbe_lifecycle.h"
#include "fbe/fbe_board.h"
#include "fbe/fbe_ps_info.h"
#include "fbe/fbe_sps_info.h"
#include "fbe/fbe_eir_info.h"
#include "fbe/fbe_enclosure_data_access_public.h"
#include "spid_types.h"

#define FBE_MAX_MULTI_FAN_FAULT_TIMEOUT         600

#define FBE_BASE_BOARD_EIR_SAMPLE_COUNT         10

#define FBE_BASE_BOARD_FUP_PERMISSION_DENY_LIMIT_IN_SEC      600      // 10 minutes 

typedef void*   fbe_base_board_pe_buffer_handle_t; 

typedef enum fbe_base_board_cond_id_e{ 
    FBE_BASE_BOARD_LIFECYCLE_COND_INIT_BOARD = FBE_LIFECYCLE_COND_FIRST_ID(FBE_CLASS_ID_BASE_BOARD),
    FBE_BASE_BOARD_LIFECYCLE_COND_UPDATE_HARDWARE_CONFIGURATION,
    FBE_BASE_BOARD_LIFECYCLE_COND_UPDATE_HARDWARE_CONFIGURATION_POLL,
	FBE_BASE_BOARD_LIFECYCLE_COND_PE_STATUS_UNKNOWN,
    FBE_BASE_BOARD_LIFECYCLE_COND_INITIALIZE_PRIVATE_SPACE_LAYOUT,
    FBE_BASE_BOARD_LIFECYCLE_COND_LAST /* Must be last. */
} fbe_base_board_cond_id_t;

typedef struct fbe_base_board_command_s
{
    fbe_queue_head_t command_queue;
    fbe_spinlock_t   command_queue_lock;
}fbe_base_board_command_t;

typedef struct fbe_base_board_command_queue_element_s
{
    fbe_queue_element_t			queue_element; /* MUST be first */
    fbe_packet_t                *packet;
}fbe_base_board_command_queue_element_t;

extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(base_board);

/* This structure is used to keep which enclosure holds the permission to do firmware upgrade*/
typedef struct fbe_base_board_fup_permission_info_s
{
    fbe_bool_t  permissionOccupied;
    fbe_u32_t   permissionGrantTime;
    fbe_device_physical_location_t location;
} fbe_base_board_fup_permission_info_t;

typedef enum
{
    FBE_BASE_BOARD_SSD_TEMP_LOG_NONE = 0,
    FBE_BASE_BOARD_SSD_TEMP_LOG_OT_WARNING,
    FBE_BASE_BOARD_SSD_TEMP_LOG_OT_FAILURE,
    FBE_BASE_BOARD_SSD_TEMP_LOG_SHUTDOWN,
} fbe_base_board_ssdTempLogType;

typedef struct fbe_base_board_s{
    fbe_base_discovering_t base_discovering;
    /* fbe_edge_index_t first_io_port_index; I am not shure that we need it*/
    fbe_u32_t           number_of_io_ports;

    /* Command Queue */
    fbe_base_board_command_t            board_command_queue;

    /* The pointer to the buffer used to get status of the processor enclosure components.*/
    fbe_u32_t                           pe_buffer_size;
    fbe_base_board_pe_buffer_handle_t   pe_buffer_handle;
    
    /* The pointer to the first block of EDAL data for the processor enclosure components.*/
    fbe_edal_block_handle_t             edal_block_handle;
	SP_ID                               spid;
    fbe_u32_t                           localIoModuleCount;
    fbe_u32_t                           localAltIoDevCount; //Hack to allow NDU of changes to single array of IO devices in SPECL
    fbe_u32_t                           psCountPerBlade;
	SPID_PLATFORM_INFO                  platformInfo;
    fbe_bool_t                          mfgMode;
    /* Whether this array is a single SP array. Eg single SP beachcomber */
    fbe_bool_t                          isSingleSpSystem;    

    // EIR relate info
    fbe_u32_t                       sampleCount;
    fbe_u32_t                       sampleIndex;
    fbe_eir_input_power_sample_t    inputPower;
    fbe_eir_input_power_sample_t    inputPowerSamples[FBE_ESP_PS_MAX_COUNT_PER_ENCL][FBE_BASE_BOARD_EIR_SAMPLE_COUNT];

    /* In ESP, board_mgmt and module_mgmt is in charge of upgrading enclosure 0_0 on some DPE.
     * Meanwhile encl_mgmt is do upgrade for all other enclosures. So there is possiblity they upgrade
     * enclosures at the same time, which will cause problems. To solve that, we use this lock to 
     * ensure only one enclosure to upgrade at one time. 
     * on DPE board_mgmt moudle_mgmt and encl_mgmt should send request to board object to race for this permission.
     */
    fbe_spinlock_t  enclGetLocalFupPermissionLock;
    fbe_base_board_fup_permission_info_t  enclosureFupPermission;

    fbe_u32_t                       ssdPollCounter;
    fbe_base_board_ssdTempLogType   logSsdTemperature;
    
    FBE_LIFECYCLE_DEF_INST_DATA;
    FBE_LIFECYCLE_DEF_INST_COND(FBE_LIFECYCLE_COND_MAX(FBE_BASE_BOARD_LIFECYCLE_COND_LAST));    
}fbe_base_board_t;

typedef struct fbe_base_board_pe_init_data_s{
    fbe_enclosure_component_types_t component_type; 
    fbe_u32_t           component_size; // The individual component size.
    fbe_u32_t           component_count; // The number of the components with the corresponding type.
    fbe_u32_t           max_status_timeout; // Max timeout value (in secs) for the bad status read.
}fbe_base_board_pe_init_data_t;

enum fbe_base_board_invalid_diplex_port_e{
    FBE_BASE_BOARD_DIPLEX_PORT_INVALID = 0xFFFFFFFF
};

/* Methods */
fbe_status_t fbe_base_board_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle);
fbe_status_t fbe_base_board_destroy_object( fbe_object_handle_t object_handle);
fbe_status_t fbe_base_board_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_base_board_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context);
fbe_status_t fbe_base_board_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);

fbe_status_t fbe_base_board_monitor_load_verify(void);


/* XXX FIXME XXX This function is deprecated by the LIFECYCLE state. */
fbe_status_t fbe_base_board_monitor(fbe_base_board_t * base_board, fbe_packet_t * packet);
fbe_status_t fbe_base_board_init(fbe_base_board_t * base_board);
fbe_edal_block_handle_t * fbe_base_board_alloc_and_init_edal_block(void);
fbe_status_t fbe_base_board_format_edal_block(fbe_base_board_t * base_board,
                                              fbe_base_board_pe_init_data_t pe_init_data[]);
fbe_status_t fbe_base_board_release_edal_block(fbe_edal_block_handle_t edal_block_handle);

fbe_status_t fbe_base_board_get_first_io_port_index(fbe_base_board_t * base_board, fbe_edge_index_t * first_io_port_index);
fbe_status_t fbe_base_board_set_first_io_port_index(fbe_base_board_t * base_board, fbe_edge_index_t  first_io_port_index);

fbe_status_t fbe_base_board_get_number_of_io_ports(fbe_base_board_t * base_board, fbe_u32_t * number_of_io_ports);
fbe_status_t fbe_base_board_set_number_of_io_ports(fbe_base_board_t * base_board, fbe_u32_t number_of_io_ports);

fbe_status_t fbe_base_board_set_edal_block_handle(fbe_base_board_t * base_board, fbe_edal_block_handle_t edal_block_handle);
fbe_status_t fbe_base_board_get_edal_block_handle(fbe_base_board_t * base_board, fbe_edal_block_handle_t * edal_block_handle);

fbe_status_t fbe_base_board_set_pe_buffer_handle(fbe_base_board_t * base_board, fbe_base_board_pe_buffer_handle_t pe_buffer_handle);
fbe_status_t fbe_base_board_get_pe_buffer_handle(fbe_base_board_t * base_board, fbe_base_board_pe_buffer_handle_t * pe_buffer_handle);

fbe_status_t fbe_base_board_set_pe_buffer_size(fbe_base_board_t * base_board, fbe_u32_t pe_buffer_size);
fbe_status_t fbe_base_board_get_pe_buffer_size(fbe_base_board_t * base_board, fbe_u32_t * pe_buffer_size);

fbe_status_t fbe_base_board_get_platform_info(SPID_PLATFORM_INFO * platform_info);
fbe_status_t fbe_base_board_set_FlushFilesAndReg(SP_ID sp_id);
fbe_status_t fbe_base_board_check_local_sp_reboot(fbe_base_board_t *base_board);
fbe_status_t fbe_base_board_check_peer_sp_reboot(fbe_base_board_t *base_board);
fbe_status_t fbe_base_board_is_mfgMode(fbe_bool_t *mfgModePtr);
fbe_bool_t fbe_base_board_is_simulated_hw(void);
fbe_status_t fbe_base_board_get_ssd_self_test_passed(fbe_base_board_t *base_board, fbe_bool_t *self_test_passed);
fbe_status_t fbe_base_board_get_ssd_spare_blocks(fbe_base_board_t *base_board, fbe_u32_t *spare_blocks);
fbe_status_t fbe_base_board_get_ssd_life_used(fbe_base_board_t *base_board, fbe_u32_t *life_used);
fbe_status_t fbe_base_board_get_ssd_count(fbe_base_board_t *base_board, fbe_u32_t *ssd_count);
fbe_status_t fbe_base_board_get_ssd_serial_number(fbe_base_board_t *base_board, fbe_char_t *pSsdSerialNumber);
fbe_status_t fbe_base_board_get_ssd_part_number(fbe_base_board_t *base_board, fbe_char_t *pSsdPartNumber);
fbe_status_t fbe_base_board_get_ssd_assembly_name(fbe_base_board_t *base_board, fbe_char_t *pSsdAssemblyName);
fbe_status_t fbe_base_board_get_ssd_firmware_revision(fbe_base_board_t *base_board, fbe_char_t *pSsdFirmwareRevision);
fbe_status_t fbe_base_board_get_ssd_temperature(fbe_base_board_t *base_board, fbe_u32_t *pSsdTemperature);
fbe_status_t fbe_base_board_logSsdTemperatureToPmp(fbe_base_board_t *base_board, 
                                                   fbe_base_board_ssdTempLogType ssdTempLogType,
                                                   fbe_u32_t ssdTemperature);

fbe_status_t fbe_base_board_add_command_element(fbe_base_board_t *base_board, fbe_packet_t *packet);
fbe_status_t fbe_base_board_check_command_queue(fbe_base_board_t *base_board);
fbe_status_t fbe_base_board_check_outstanding_command(fbe_base_board_t *base_board, fbe_packet_t *packet);
fbe_status_t fbe_base_board_peerBoot_fltReg_checkFru(fbe_base_board_t *pBaseBoard, fbe_peer_boot_flt_exp_info_t  *fltexpInfoPtr);
fbe_u32_t fbe_base_board_pe_get_fault_reg_status(fbe_base_board_t *base_board);
fbe_status_t fbe_base_board_is_single_sp_system(fbe_bool_t *singleSpPtr);
fbe_status_t fbe_base_board_get_serial_num_from_pci(fbe_base_board_t * base_board,
                                                    fbe_u32_t bus, fbe_u32_t function, fbe_u32_t slot,
                                                    fbe_u32_t phy_map, fbe_u8_t *serial_num, fbe_u32_t buffer_size);
fbe_status_t fbe_base_board_get_resume_field_data(fbe_base_board_t * base_board,
                                                  SMB_DEVICE smb_device,
                                                  fbe_u8_t *buffer,
                                                  fbe_u32_t offset,
                                                  fbe_u32_t buffer_size);
fbe_status_t fbe_base_board_get_smb_device_from_pci(fbe_u32_t pci_bus, fbe_u32_t pci_function, fbe_u32_t pci_slot, 
                                                    fbe_u32_t phy_map, SMB_DEVICE *smb_device);

extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(base_board);



#endif /* FBE_BASE_BOARD_PRIVATE_H */
