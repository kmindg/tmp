#ifndef FBE_TOPOLOGY_INTERFACE_H
#define FBE_TOPOLOGY_INTERFACE_H

#include "fbe/fbe_package.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_physical_drive.h"
#include "fbe/fbe_api_common_interface.h"
#include "fbe/fbe_event_interface.h"
#include "fbe/fbe_scsi_interface.h"
#include "fbe/fbe_port.h"
#include "fbe/fbe_transport.h"
#include "fbe_base.h"

enum fbe_topology_constants_e {
    FBE_TOPOLOGY_CONTROL_CODE_CLASS_COMMAND_SIZE = 512,
};
#define FBE_MAX_SPARE_OBJECTS 2048 /* For Megatron large config */

/*!********************************************************************* 
 * @enum fbe_topology_object_type_t 
 *  
 * @brief 
 *  This contains the enumeration for the obejct types.
 * @ingroup fbe_api_common_interface
 **********************************************************************/
enum fbe_topology_object_type_e{
    FBE_TOPOLOGY_OBJECT_TYPE_INVALID = 				0x00000000,
    FBE_TOPOLOGY_OBJECT_TYPE_BOARD = 				0x00000001,/*!< This is a board */
    FBE_TOPOLOGY_OBJECT_TYPE_PORT = 				0x00000002,/*!< This is a port */
    FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE = 			0x00000004,/*!< This is an enclosure */
    FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE = 		0x00000008,/*!< This is a Physical Drive */
    FBE_TOPOLOGY_OBJECT_TYPE_LOGICAL_DRIVE = 		0x00000010,/*!< This is a Logical drive*/
    FBE_TOPOLOGY_OBJECT_TYPE_RAID_GROUP = 			0x00000020,/*!< This is a RAID */
    FBE_TOPOLOGY_OBJECT_TYPE_VIRTUAL_RAID_GROUP = 	0x00000040,/*!< This is a Virtual RAID */
    FBE_TOPOLOGY_OBJECT_TYPE_VIRTUAL_DRIVE = 		0x00000080,/*!< This is a Virtual drive*/
    FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE = 	0x00000100,/*!< This is a Provisioned drive (1:1 match to a logical) */
    FBE_TOPOLOGY_OBJECT_TYPE_LUN = 					0x00000200,/*!< This is a LUN */
    FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT =     0x00000400,/*!< This is an environment management object */
    FBE_TOPOLOGY_OBJECT_TYPE_LCC  =                 0x00000800,/*!< This is an LCC */   // GJF - Can we move this to the middle? 
    FBE_TOPOLOGY_OBJECT_TYPE_EXTENT_POOL = 			0x00001000,/*!< This is a Extent pool */
    FBE_TOPOLOGY_OBJECT_TYPE_EXT_POOL_LUN = 		0x00002000,/*!< This is a Extent pool lun */
    FBE_TOPOLOGY_OBJECT_TYPE_ALL = 					0xFFFFFFFFFFFFFFFFULL /*!< Mask for all objects together */
};

typedef fbe_u64_t fbe_topology_object_type_t;/*to be used with fbe_topology_object_type_e*/

typedef enum fbe_topology_control_code_e {
    FBE_TOPOLOGY_CONTROL_CODE_INVALID = FBE_SERVICE_CONTROL_CODE_INVALID_DEF(FBE_SERVICE_ID_TOPOLOGY),
    FBE_TOPOLOGY_CONTROL_CODE_CREATE_OBJECT,
    FBE_TOPOLOGY_CONTROL_CODE_DESTROY_OBJECT,

    FBE_TOPOLOGY_CONTROL_CODE_ENUMERATE_OBJECTS,
    FBE_TOPOLOGY_CONTROL_CODE_GET_OBJECT_TYPE,

    FBE_TOPOLOGY_CONTROL_CODE_GET_STATISTICS,
    FBE_TOPOLOGY_CONTROL_CODE_GET_ALL_DRIVES_OF_TYPE,
    
    FBE_TOPOLOGY_CONTROL_CODE_GET_DRIVE_BY_LOCATION, /* Returns object id of logical drive in specific location */
    FBE_TOPOLOGY_CONTROL_CODE_GET_PROVISION_DRIVE_BY_LOCATION, /* returns PVD id */
    FBE_TOPOLOGY_CONTROL_CODE_GET_ENCLOSURE_BY_LOCATION, /* Returns object id of enclosure in specific location */
    FBE_TOPOLOGY_CONTROL_CODE_GET_PHYSICAL_DRIVE_BY_LOCATION,
    FBE_TOPOLOGY_CONTROL_CODE_GET_PORT_BY_LOCATION,
    FBE_TOPOLOGY_CONTROL_CODE_GET_PORT_BY_LOCATION_AND_ROLE, /* Get port object by its bus number and role */
    FBE_TOPOLOGY_CONTROL_CODE_ENUMERATE_CLASS, /* Gets list of all objects of this class. */
    FBE_TOPOLOGY_CONTROL_CODE_REGISTER_EVENT_FUNCTION,
    FBE_TOPOLOGY_CONTROL_CODE_UNREGISTER_EVENT_FUNCTION,
    FBE_TOPOLOGY_CONTROL_CODE_GET_SPARE_DRIVE_POOL,
    FBE_TOPOLOGY_CONTROL_CODE_GET_TOTAL_OBJECTS,
    FBE_TOPOLOGY_CONTROL_CODE_GET_TOTAL_OBJECTS_OF_CLASS,
    FBE_TOPOLOGY_CONTROL_CODE_GET_LOGICAL_DRIVE_BY_SERIAL_NUMBER,
    FBE_TOPOLOGY_CONTROL_CODE_GET_BOARD,
    FBE_TOPOLOGY_CONTROL_CODE_ENUMERATE_PORTS_BY_ROLE,
    FBE_TOPOLOGY_CONTROL_CODE_GET_BVD_OBJECT_ID,
    FBE_TOPOLOGY_CONTROL_CODE_GET_OBJECT_ID_OF_SINGLETON_CLASS,
    FBE_TOPOLOGY_CONTROL_CODE_CHECK_OBJECT_EXISTENT,
    FBE_TOPOLOGY_CONTROL_CODE_GET_PHYSICAL_DRIVE_OBJECTS,
    FBE_TOPOLOGY_CONTROL_CODE_SET_PVD_HS_FLAG_CONTROL,
    FBE_TOPOLOGY_CONTROL_CODE_GET_PVD_HS_FLAG_CONTROL,
    FBE_TOPOLOGY_CONTROL_CODE_UPDATE_DISABLE_SPARE_SELECT_UNCONSUMED,
    FBE_TOPOLOGY_CONTROL_CODE_GET_DISABLE_SPARE_SELECT_UNCONSUMED,
	FBE_TOPOLOGY_CONTROL_CODE_GET_PROVISION_DRIVE_BY_LOCATION_FROM_NON_PAGED,
    FBE_TOPOLOGY_CONTROL_CODE_GET_PHYSICAL_DRIVE_BY_SERIAL_NUMBER,
    FBE_TOPOLOGY_CONTROL_CODE_LAST
} fbe_topology_control_code_t;

/* FBE_TOPOLOGY_CONTROL_CODE_ENUMERATE_OBJECTS */
typedef struct fbe_topology_mgmt_enumerate_objects_s{
    fbe_u32_t number_of_objects;/* INPUT */
    fbe_u32_t number_of_objects_copied;/* OUTPUT */
}fbe_topology_mgmt_enumerate_objects_t;

/* FBE_TOPOLOGY_CONTROL_CODE_GET_OBJECT_TYPE */
typedef struct fbe_topology_mgmt_get_object_type_s{
    fbe_object_id_t object_id; /* INPUT */
    fbe_topology_object_type_t topology_object_type; /* OUTPUT */
}fbe_topology_mgmt_get_object_type_t;

/* FBE_TOPOLOGY_CONTROL_CODE_DESTROY_OBJECT */
typedef struct fbe_topology_mgmt_destroy_object_s{
    fbe_object_id_t object_id; /* INPUT */
}fbe_topology_mgmt_destroy_object_t;

/* FBE_TOPOLOGY_CONTROL_CODE_GET_STATISTICS */
typedef struct fbe_topology_control_get_statistics_s{
    fbe_u32_t number_of_created_objects;
    fbe_u32_t number_of_destroyed_objects;
}fbe_topology_control_get_statistics_t;

/*FBE_TOPOLOGY_CONTROL_CODE_GET_ALL_DRIVES_OF_TYPE*/
typedef struct fbe_topology_mgmt_get_all_drive_of_type_s{
    fbe_drive_type_t	drive_type;
    fbe_u32_t number_of_objects;
    fbe_object_id_t object_id_array[FBE_MAX_PHYSICAL_OBJECTS];
}fbe_topology_mgmt_get_all_drive_of_type_t;

/* FBE_TOPOLOGY_CONTROL_CODE_GET_PHYSICAL_DRIVE_OBJECTS */
typedef struct fbe_topology_control_get_physical_drive_objects_s{
    fbe_u32_t number_of_objects;
    fbe_object_id_t pdo_list[FBE_MAX_PHYSICAL_OBJECTS];
}fbe_topology_control_get_physical_drive_objects_t;

/* FBE_TOPOLOGY_CONTROL_CODE_GET_DRIVE_BY_LOCATION */
typedef struct fbe_topology_control_get_drive_by_location_s{
    fbe_u32_t port_number; /* IN */
    fbe_u32_t enclosure_number; /* IN */
    fbe_u32_t slot_number; /* IN */

    fbe_object_id_t logical_drive_object_id; /* OUT */
}fbe_topology_control_get_drive_by_location_t;

/* FBE_TOPOLOGY_CONTROL_CODE_GET_PVD_ID_BY_LOCATION */
typedef struct fbe_topology_control_get_provision_drive_id_by_location_s {
    fbe_u32_t port_number; /* IN */
    fbe_u32_t enclosure_number; /* IN */
    fbe_u32_t slot_number; /* IN */
    fbe_object_id_t object_id; /* OUT */
}fbe_topology_control_get_provision_drive_id_by_location_t;

/* FBE_TOPOLOGY_CONTROL_CODE_GET_ENCLOSURE_BY_LOCATION */ 
typedef struct fbe_topology_control_get_enclosure_by_location_s{
       fbe_u32_t port_number; /* IN */
       fbe_u32_t enclosure_number; /* IN */

       fbe_object_id_t enclosure_object_id; /* OUT */
       fbe_u32_t component_count; /*OUT */
       fbe_object_id_t comp_object_id[FBE_API_MAX_ENCL_COMPONENTS]; /* OUT */
}fbe_topology_control_get_enclosure_by_location_t;

typedef enum fbe_topology_control_get_spare_drive_type_e
{
    FBE_TOPOLOGY_GET_SPARE_DRIVE_TYPE_INVALID   = 0,
    FBE_TOPOLOGY_GET_SPARE_DRIVE_TYPE_UNCONSUMED,
    FBE_TOPOLOGY_GET_SPARE_DRIVE_TYPE_TEST_SPARE
} fbe_topology_control_get_spare_drive_type_t;

/* FBE_TOPOLOGY_CONTROL_CODE_GET_CONFIGURED_SPARE_DRIVE */
typedef struct fbe_topology_control_get_spare_drive_pool_s {
    fbe_topology_control_get_spare_drive_type_t desired_spare_drive_type;
    fbe_u32_t       number_of_spare;
    fbe_object_id_t spare_object_list[FBE_MAX_SPARE_OBJECTS]; /* OUT */ 
}fbe_topology_control_get_spare_drive_pool_t;

/* FBE_TOPOLOGY_CONTROL_CODE_GET_CONFIGURED_SPARES */
typedef struct fbe_topology_control_get_physical_drive_by_location_s{
    fbe_u32_t port_number; /* IN */
    fbe_u32_t enclosure_number; /* IN */
    fbe_u32_t slot_number; /* IN */
    fbe_object_id_t physical_drive_object_id; /* OUT */
}fbe_topology_control_get_physical_drive_by_location_t;

/* FBE_TOPOLOGY_CONTROL_CODE_ENUMERATE_CLASS */
typedef struct fbe_topology_control_enumerate_class_s{
    fbe_class_id_t class_id; /* IN class id to enumerate */
    fbe_u32_t number_of_objects;/* INPUT */
    fbe_u32_t number_of_objects_copied;/* OUTPUT */
}fbe_topology_control_enumerate_class_t;

/* FBE_TOPOLOGY_CONTROL_CODE_ENUMERATE_PORTS_BY_ROLE */
typedef struct fbe_topology_control_enumerate_ports_by_role_s{
    fbe_port_role_t port_role;/* IN ports of this role will be enumerated */
    fbe_u32_t number_of_objects;/* INPUT */
    fbe_u32_t number_of_objects_copied;/* OUTPUT */
}fbe_topology_control_enumerate_ports_by_role_t;

/* Number of sg entries needed for FBE_TOPOLOGY_CONTROL_CODE_ENUMERATE_CLASS.
 * The packet's sg gets a ptr to this sg list. 
 * One sg entry is to point to an array of fbe_object_id_t, and the second sg entry is 
 * simply a terminator. 
 */
#define FBE_TOPLOGY_CONTROL_ENUMERATE_CLASS_SG_COUNT 2

typedef fbe_status_t (* fbe_topology_send_event_function_t)(fbe_object_id_t object_id, fbe_event_type_t event_type, fbe_event_context_t event_context);

/* FBE_TOPOLOGY_CONTROL_CODE_REGISTER_EVENT_FUNCTION */
typedef struct fbe_topology_control_register_event_function_s{
    fbe_topology_send_event_function_t event_function;
    fbe_package_id_t package_id;
}fbe_topology_control_register_event_function_t;

/* FBE_TOPOLOGY_CONTROL_CODE_UNREGISTER_EVENT_FUNCTION */
typedef struct fbe_topology_control_unregister_event_function_s{
    fbe_package_id_t package_id;
}fbe_topology_control_unregister_event_function_t;

/* FBE_TOPOLOGY_CONTROL_CODE_GET_PORT_BY_LOCATION */
typedef struct fbe_topology_control_get_port_by_location_s{
    fbe_u32_t port_number; /* IN */
    fbe_object_id_t port_object_id; /* OUT */
}fbe_topology_control_get_port_by_location_t;

/* FBE_TOPOLOGY_CONTROL_CODE_GET_PORT_BY_LOCATION_AND_ROLE */
typedef struct fbe_topology_control_get_port_by_location_and_role_s{
    fbe_u32_t port_number; /* IN */
    fbe_port_role_t port_role;/* IN */
    fbe_object_id_t port_object_id; /* OUT */
}fbe_topology_control_get_port_by_location_and_role_t;

// FBE_TOPOLOGY_CONTROL_CODE_GET_BOARD
typedef struct fbe_topology_control_get_board_s{
    fbe_object_id_t board_object_id; /* OUT */
}fbe_topology_control_get_board_t;

// FBE_TOPOLOGY_CONTROL_CODE_GET_ESP_OBJECT_ID
typedef struct fbe_topology_control_get_esp_object_id_s{
    fbe_class_id_t      esp_class_id;           // IN
    fbe_object_id_t     esp_object_id;          // OUT
}fbe_topology_control_get_esp_object_id_t;

/*FBE_TOPOLOGY_CONTROL_CODE_GET_TOTAL_OBJECTS*/
typedef struct fbe_topology_control_get_total_objects_s{
    fbe_u32_t total_objects; /* OUT */
}fbe_topology_control_get_total_objects_t;

/*FBE_TOPOLOGY_CONTROL_CODE_GET_TOTAL_OBJECTS_OF_CLASS*/
typedef struct fbe_topology_control_get_total_objects_of_class_s{
    fbe_class_id_t class_id;
    fbe_u32_t total_objects; /* OUT */
}fbe_topology_control_get_total_objects_of_class_t;

/*FBE_TOPOLOGY_CONTROL_CODE_GET_BVD_OBJECT_ID*/
typedef struct fbe_topology_control_get_bvd_id_s{
    fbe_object_id_t bvd_object_id; /* OUT */
}fbe_topology_control_get_bvd_id_t;

/*FBE_TOPOLOGY_CONTROL_CODE_GET_LOGICAL_DRIVE_BY_SERIAL_NUMBER*/
typedef struct fbe_topology_control_get_logical_drive_by_sn_s{
    fbe_u8_t		product_serial_num[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1];/*IN*/
    fbe_u32_t		sn_size;
    fbe_object_id_t object_id; /* OUT */
}fbe_topology_control_get_logical_drive_by_sn_t;

/*FBE_TOPOLOGY_CONTROL_CODE_GET_LOGICAL_DRIVE_BY_SERIAL_NUMBER*/
typedef struct fbe_topology_control_get_physical_drive_by_sn_s{
    fbe_u8_t		product_serial_num[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1];/*IN*/
    fbe_u32_t		sn_size;
    fbe_object_id_t object_id; /* OUT */
}fbe_topology_control_get_physical_drive_by_sn_t;

/*FBE_TOPOLOGY_CONTROL_CODE_GET_OBJECT_ID_OF_SINGLETON_CLASS*/
typedef struct fbe_topology_control_get_object_id_of_singleton_s{
    fbe_class_id_t 	class_id; /*IN*/
    fbe_object_id_t object_id; /* OUT */
}fbe_topology_control_get_object_id_of_singleton_t;

/*FBE_TOPOLOGY_CONTROL_CODE_CHECK_OBJECT_EXISTENT*/
typedef struct fbe_topology_mgmt_check_object_existent_s{
    fbe_object_id_t object_id;  /* IN */
    fbe_bool_t exists_b;        /* OUT */
}fbe_topology_mgmt_check_object_existent_t;

/*FBE_TOPOLOGY_CONTROL_CODE_SET_PVD_HS_FLAG_CONTORL*/
typedef struct fbe_topology_mgmt_set_pvd_hs_flag_s{
    fbe_bool_t enable;        /* IN*/
}fbe_topology_mgmt_set_pvd_hs_flag_t;

/*FBE_TOPOLOGY_CONTROL_CODE_GET_PVD_HS_FLAG_CONTORL*/
typedef struct fbe_topology_mgmt_get_pvd_hs_flag_s{
    fbe_bool_t b_is_test_spare_enabled;        /* OUT */
}fbe_topology_mgmt_get_pvd_hs_flag_t;

/*FBE_TOPOLOGY_CONTROL_CODE_UPDATE_DISABLE_SPARE_SELECT_UNCONSUMED */
typedef struct fbe_topology_mgmt_update_disable_spare_select_unconsumed_s{
    fbe_bool_t  b_disable_spare_select_unconsumed;  /* IN */
}fbe_topology_mgmt_update_disable_spare_select_unconsumed_t;

/*FBE_TOPOLOGY_CONTROL_CODE_GET_DISABLE_SPARE_SELECT_UNCONSUMED*/
typedef struct fbe_topology_mgmt_get_disable_spare_select_unconsumed_s{
    fbe_bool_t b_disable_spare_select_unconsumed;        /* OUT */
}fbe_topology_mgmt_get_disable_spare_select_unconsumed_t;

typedef enum fbe_topology_object_status_e {
    FBE_TOPOLOGY_OBJECT_STATUS_NOT_EXIST,
    FBE_TOPOLOGY_OBJECT_STATUS_RESERVED,
    FBE_TOPOLOGY_OBJECT_STATUS_EXIST,
    FBE_TOPOLOGY_OBJECT_STATUS_READY
}fbe_topology_object_status_t;

typedef struct topology_object_table_entry_s{    
    fbe_object_handle_t           control_handle;
    fbe_object_handle_t           io_handle;
    fbe_topology_object_status_t  object_status;
    fbe_atomic_t               reference_count;
}topology_object_table_entry_t;

fbe_status_t fbe_topology_set_physical_package_io_entry(fbe_io_entry_function_t io_entry);
fbe_status_t fbe_topology_set_sep_io_entry(fbe_io_entry_function_t io_entry);

/*FBE3, sharel:temporary here just so we can build the shim, to be removed once we flare is removed*/
fbe_status_t fbe_topology_send_io_packet(struct fbe_packet_s * packet);

#endif /*FBE_TOPOLOGY_INTERFACE_H */
