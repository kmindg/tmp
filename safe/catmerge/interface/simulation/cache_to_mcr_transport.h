#ifndef CACHE_TO_MCR_TRANSPORT_H
#define CACHE_TO_MCR_TRANSPORT_H

#include "fbe/fbe_types.h"

#if defined(__cplusplus) || defined(c_plusplus)
#define CACHE_TO_MCR_PUBLIC __stdcall 
#else
#define CACHE_TO_MCR_PUBLIC 
#endif

#define CACHE_TO_MCR_PATH_MAX 64
#define CACHE_TO_MCR_CDR_LUN_NUMBER 997
#define CACHE_TO_MCR_PSM_LUN_NUMBER 998
#define CACHE_TO_MCR_VAULT_LUN_NUMBER 999
#define CACHE_TO_MCR_MAX_LU 1000

typedef enum cache_to_mcr_server_target_e{
	CACHE_TO_MCR_SP_A = 0,
	CACHE_TO_MCR_SP_B,
	CACHE_TO_MCR_LAST_TARGET
}cache_to_mcr_target_t;

typedef enum cache_to_mcr_sp_hw_config_e {
    HW_CONFIG_1ENCLOSURE_15DRIVES,
    HW_CONFIG_2ENCLOSURE_30DRIVES,
    HW_CONFIG_4ENCLOSURE_60DRIVES,
} cache_to_mcr_sp_hw_config_t;

typedef struct cache_to_mcr_send_context_s{
    fbe_u32_t	total_msg_length;/*MUST BE FIRST for performance reasons, we read it on the server side as one field since we don't need the rest*/
    void *		user_buffer;/*opaque*/
	void *		completion_function;
	void *  	completion_context;
    fbe_u32_t	packet_lengh;
}cache_to_mcr_send_context_t;

typedef struct cache_to_mcr_server_mode_to_port_str_s{
	cache_to_mcr_target_t server_target;
	fbe_u8_t	 port_str[16];
}cache_to_mcr_server_mode_to_port_str_t;

typedef enum cache_to_mcr_operation_code_e {
	CACHE_TO_MCR_TRANSPORT_GET_LUN_DEVICE_INFO,
	CACHE_TO_MCR_TRANSPORT_CREATE_LUN,
	CACHE_TO_MCR_TRANSPORT_DESTROY_LUN,
	CACHE_TO_MCR_TRANSPORT_MJ_READ,
	CACHE_TO_MCR_TRANSPORT_MJ_WRITE,
	CACHE_TO_MCR_TRANSPORT_DCA_READ,
	CACHE_TO_MCR_TRANSPORT_DCA_WRITE,
	CACHE_TO_MCR_TRANSPORT_BUILD_HW_CONFIG,
	CACHE_TO_MCR_TRANSPORT_LOAD_SEP,
	CACHE_TO_MCR_TRANSPORT_UNLOAD_SEP,
	CACHE_TO_MCR_TRANSPORT_DESTROY_HW_CONFIG,
	CACHE_TO_MCR_TRANSPORT_SGL_READ,
	CACHE_TO_MCR_TRANSPORT_SGL_WRITE,
}cache_to_mcr_operation_code_t;

typedef struct cache_to_mcr_lu_info_s{
	void * mcr_block_edge;
	fbe_object_id_t bvd_object_id;
}cache_to_mcr_lu_info_t;

typedef void (*get_dev_info_completion)(void * block_edge, fbe_object_id_t bvd_object_id, void * contex);

/*CACHE_TO_MCR_TRANSPORT_GET_LUN_DEVICE_INFO*/
typedef struct cache_to_mcr_get_lun_device_info_s{
	fbe_u8_t      	device_path[CACHE_TO_MCR_PATH_MAX]; //IN path to device /dev/CLARiiONDiskN
	get_dev_info_completion completion_function;
	void * completion_context;
}cache_to_mcr_get_lun_device_info_t;

typedef struct cache_to_mcr_sgl_read_write_cmd_s{
	fbe_u32_t sgl_entries;
	fbe_u64_t lba;
	fbe_u32_t byte_count;
}cache_to_mcr_sgl_read_write_cmd_t;

/*CACHE_TO_MCR_TRANSPORT_MJ_READ*/
/*CACHE_TO_MCR_TRANSPORT_MJ_WRITE*/
/*CACHE_TO_MCR_TRANSPORT_DCA_READ*/
/*CACHE_TO_MCR_TRANSPORT_DCA_WRITE*/
typedef struct cache_to_mcr_mj_read_write_cmd_s{
	fbe_u32_t byte_count;
	fbe_u64_t lba;
	fbe_u8_t * buffer;
}cache_to_mcr_mj_read_write_cmd_t;

/*CACHE_TO_MCR_TRANSPORT_CREATE_LUN*/
typedef struct cache_to_mcr_create_lun_s{
    fbe_lun_number_t        lun_number;        /*!< LU Number */
    fbe_lba_t               capacity;          /*!< LU Capacity in blocks*/
}cache_to_mcr_create_lun_t;

/*CACHE_TO_MCR_TRANSPORT_BUILD_HW_CONFIG*/
typedef struct cache_to_mcr_build_hw_config_s{
    cache_to_mcr_sp_hw_config_t hw_config;
}cache_to_mcr_build_hw_config_t;


typedef struct cache_to_mcr_serialized_io_s{
	cache_to_mcr_operation_code_t	user_control_code;/*IN*/
	void *		block_edge;/*IN (on which edge on the BVD object we send the IO)*/
	fbe_u32_t	sg_elements_count;/*IN*/
	fbe_irp_stack_flags_t	irp_flags;/*IN*/
	fbe_object_id_t bvd_object_id;/*IN*/
	fbe_status_t	status;/*OUT*/
	fbe_u64_t	status_information;/*OUT*/
	void * completion_function;/*opaque*/
	void * completion_context;
}cache_to_mcr_serialized_io_t;

fbe_status_t CACHE_TO_MCR_PUBLIC cache_to_mcr_test_init(cache_to_mcr_target_t target_sp);
fbe_status_t CACHE_TO_MCR_PUBLIC cache_to_mcr_test_destroy(cache_to_mcr_target_t target_sp);
fbe_status_t CACHE_TO_MCR_PUBLIC cache_to_mcr_load_mcr_hw(cache_to_mcr_target_t target_sp, cache_to_mcr_sp_hw_config_t config);
fbe_status_t CACHE_TO_MCR_PUBLIC cache_to_mcr_load_mcr_logical(cache_to_mcr_target_t target_sp);
fbe_status_t CACHE_TO_MCR_PUBLIC cache_to_mcr_unload_mcr_hw(cache_to_mcr_target_t target_sp);
fbe_status_t CACHE_TO_MCR_PUBLIC cache_to_mcr_unload_mcr_logical(cache_to_mcr_target_t target_sp);
fbe_bool_t CACHE_TO_MCR_PUBLIC fbe_test_environment_active(void);
void CACHE_TO_MCR_PUBLIC fbe_test_environment_enable(void);
void CACHE_TO_MCR_PUBLIC fbe_test_environment_disable(void);


#endif /*CACHE_TO_MCR_TRANSPORT_H*/
