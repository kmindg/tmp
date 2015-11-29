#include "fbe/fbe_types.h"
#include "fbe/fbe_port.h"
#include "fbe_sas_port.h"

#define FBE_SAS_LSI_SHIM_MAX_PORTS 24
#define FBE_SAS_LSI_SHIM_MAX_EXPANDERS 8
#define FBE_SAS_LSI_SHIM_MAX_DRIVES 120

#define FBE_SAS_LSI_SHIM_MAX_DEVICES_PER_PORT (FBE_SAS_LSI_SHIM_MAX_EXPANDERS + FBE_SAS_LSI_SHIM_MAX_DRIVES)

typedef struct fbe_sas_lsi_shim_enumerate_backend_ports_s {
	fbe_u32_t number_of_io_ports;
	fbe_u32_t io_port_array[FBE_SAS_LSI_SHIM_MAX_PORTS];
}fbe_sas_lsi_shim_enumerate_backend_ports_t;

typedef enum sas_lsi_port_state_e {
	SAS_LSI_PORT_STATE_INVALID,
	SAS_LSI_PORT_STATE_UNINITIALIZED,
	SAS_LSI_PORT_STATE_INITIALIZED,
	SAS_LSI_PORT_STATE_LAST
}sas_lsi_port_state_t;

typedef struct fbe_sas_lsi_device_info_s {
    fbe_u64_t device_type;
    fbe_u32_t bus_id;
    fbe_u32_t target_id;
    fbe_u32_t lun_id;
    fbe_sas_address_t sas_address;
    fbe_sas_device_type_t product_type;
}fbe_sas_lsi_device_info_t;

typedef struct fbe_sas_lsi_port_info_s {
	fbe_port_number_t port_number;
	sas_lsi_port_state_t state;
    fbe_u32_t max_buses;
    fbe_u32_t max_targets;
    fbe_u32_t max_luns;
    fbe_u16_t payload_offset;
    fbe_sas_address_t *sas_address;
    fbe_u32_t num_phys;
    fbe_sas_lsi_device_info_t device_info[FBE_SAS_LSI_SHIM_MAX_DEVICES_PER_PORT];
}fbe_sas_lsi_port_info_t;

fbe_status_t fbe_sas_lsi_shim_enumerate_backend_ports(fbe_sas_lsi_shim_enumerate_backend_ports_t * sas_lsi_enumerate_backend_ports);
fbe_status_t fbe_sas_lsi_port_init(void);
fbe_status_t fbe_sas_lsi_get_expander_list(fbe_port_number_t port_number, fbe_sas_lsi_device_info_t *expander_info, fbe_u32_t max_count);
fbe_status_t fbe_sas_lsi_get_all_attached_devices(fbe_port_number_t port_number, 
                                                  fbe_sas_lsi_device_info_t *expander_info, 
                                                  fbe_u32_t max_count,
                                                  fbe_u32_t *count);
fbe_status_t fbe_sas_lsi_port_info_init(fbe_sas_lsi_port_info_t *port_info);
fbe_status_t fbe_sas_lsi_port_device_info_init(fbe_sas_lsi_device_info_t *device_info);
fbe_status_t fbe_sas_lsi_shim_convert_inq_data_device_type(fbe_u8_t *inq_data, fbe_u64_t *device_type);
fbe_status_t fbe_sas_lsi_shim_convert_inq_data_product_type(fbe_u8_t *inq_data, fbe_sas_device_type_t *product_type);
fbe_status_t fbe_sas_lsi_shim_send_scsi_command(fbe_u32_t port_number, fbe_u32_t bus_id, 
                                                fbe_u32_t target_id, fbe_u32_t lun_id,
                                                fbe_u8_t *cdb, fbe_u32_t cdb_size, 
                                                fbe_u8_t *resp_buffer, fbe_u32_t resp_buffer_size,
                                                fbe_u8_t *cmd_buffer, fbe_u32_t cmd_buffer_size);
fbe_status_t fbe_sas_lsi_shim_send_ioctl (fbe_u32_t  portNumber, void  *pIoCtl, fbe_u32_t Size);
fbe_status_t fbe_sas_lsi_port_destroy(fbe_u32_t portNumber);