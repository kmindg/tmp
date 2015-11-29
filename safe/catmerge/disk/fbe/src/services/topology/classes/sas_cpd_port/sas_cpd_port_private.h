#ifndef SAS_CPD_PORT_PRIVATE_H
#define SAS_CPD_PORT_PRIVATE_H

#include "fbe/fbe_winddk.h"
#include "fbe_base_port.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_transport.h"
#include "fbe_sas.h"
#include "sas_port_private.h"

enum fbe_sas_cpd_port_max_phy_number_e {
	FBE_SAS_CPD_PORT_MAX_PHY_NUMBER = 96
};

/* These are the lifecycle condition ids for a sas cpd class. */
typedef enum fbe_sas_cpd_lifecycle_cond_id_e {
    FBE_SAS_CPD_PORT_LIFECYCLE_COND_UPDATE_DISCOVERY_TABLE = FBE_LIFECYCLE_COND_FIRST_ID(FBE_CLASS_ID_SAS_CPD_PORT),
	FBE_SAS_CPD_PORT_LIFECYCLE_COND_LAST /* must be last */
} fbe_sas_cpd_lifecycle_cond_id_t;


extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(sas_cpd_port);

typedef struct sas_cps_port_sas_table_element_s{
	fbe_sas_element_type_t	element_type;
	fbe_sas_address_t		sas_address;
	fbe_cpd_device_id_t		cpd_device_id;
	fbe_generation_code_t	generation_code;
	fbe_u8_t				phy_number;
	fbe_u8_t				enclosure_chain_depth;
	fbe_u8_t				reserved2;
	fbe_u8_t				reserved3;

	fbe_sas_address_t		parent_sas_address;
	fbe_cpd_device_id_t		parent_cpd_device_id;

}sas_cps_port_sas_table_element_t;

typedef struct fbe_sas_cpd_port_s{
	fbe_sas_port_t sas_port;
    fbe_bool_t     is_port_up;
    /* fbe_cpd_port_info_t port_info;*/
	/* fbe_u32_t update_phy_table_requiered;*/ /*!< If true - expander list needs to be updated */
	/* fbe_expander_list_t expander_list; *//*!< expander list from CPD driver */

	/* fbe_sas_element_t   phy_table[FBE_SAS_CPD_PORT_MAX_PHY_NUMBER]; */

	fbe_atomic_t generation_code;
	sas_cps_port_sas_table_element_t * sas_table;

	FBE_LIFECYCLE_DEF_INST_DATA;
	FBE_LIFECYCLE_DEF_INST_COND(FBE_LIFECYCLE_COND_MAX(FBE_SAS_CPD_PORT_LIFECYCLE_COND_LAST));

}fbe_sas_cpd_port_t;

/* Methods */
fbe_status_t fbe_sas_cpd_port_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle);
fbe_status_t fbe_sas_cpd_port_destroy_object( fbe_object_handle_t object_handle);
fbe_status_t fbe_sas_cpd_port_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_sas_cpd_port_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context);
fbe_status_t fbe_sas_cpd_port_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_sas_cpd_port_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);

fbe_status_t fbe_sas_cpd_port_monitor_load_verify(void);

fbe_status_t fbe_sas_cpd_port_create_edge(fbe_sas_cpd_port_t * sas_cpd_port, fbe_packet_t * packet);
fbe_status_t fbe_sas_cpd_port_get_expander_list(fbe_sas_cpd_port_t * sas_cpd_port, fbe_packet_t * packet);

/* fbe_status_t fbe_sas_cpd_port_process_phy_table(fbe_sas_cpd_port_t * sas_cpd_port, fbe_sas_element_t * phy_table); */
fbe_status_t fbe_sas_cpd_port_update_sas_table(fbe_sas_cpd_port_t * sas_cpd_port, fbe_packet_t * packet);


#endif /* SAS_CPD_PORT_PRIVATE_H */
