#ifndef SAS_BULLET_ENCLOSURE_PRIVATE_H
#define SAS_BULLET_ENCLOSURE_PRIVATE_H

#include "sas_enclosure_private.h"

enum fbe_fibre_enclosure_number_of_slots_e{
	FBE_SAS_BULLET_ENCLOSURE_NUMBER_OF_SLOTS = 15 /*!< Indexes 0-14 for slots and 15 for expansion port */
};

enum fbe_fibre_enclosure_first_slot_index_e{
	FBE_SAS_BULLET_ENCLOSURE_FIRST_SLOT_INDEX = 0
};

enum fbe_fibre_enclosure_number_of_expansion_ports_e{
	FBE_SAS_BULLET_ENCLOSURE_NUMBER_OF_EXPANSION_PORTS = 1
};

enum fbe_fibre_enclosure_first_expansion_port_index_e{
	FBE_SAS_BULLET_ENCLOSURE_FIRST_EXPANSION_PORT_INDEX = 25 /* SES has Phy number 24 the next phy hard coded for expansion port*/
};

/* All FBE timeouts in miliseconds */
enum fbe_sas_bullet_enclosure_timeouts_e {
	FBE_SAS_BULLET_ENCLOSURE_STATUS_PAGE_TIMEOUT = 10000 /* 10 sec */
};

/* In this structure we will combine the SES and SMP information */
typedef struct fbe_sas_bullet_enclosure_drive_info_s{
    fbe_sas_address_t sas_address;
	fbe_payload_discovery_element_type_t element_type;
	fbe_generation_code_t generation_code;
	/* Locator information for phy_number */

    /*fbe_ses_slot_status_element_t slot_status_element;*/

} fbe_sas_bullet_enclosure_drive_info_t;

typedef struct fbe_sas_bullet_enclosure_expansion_port_info_s{
    fbe_sas_address_t sas_address;
	fbe_payload_discovery_element_type_t element_type;
	fbe_generation_code_t generation_code;
	fbe_u8_t chain_depth;
} fbe_sas_bullet_enclosure_expansion_port_info_t;

extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(sas_bullet_enclosure);

typedef struct fbe_sas_bullet_enclosure_s{
	fbe_sas_enclosure_t sas_enclosure;
	fbe_u32_t status_page_requiered;
    fbe_sas_bullet_enclosure_drive_info_t * drive_info;

	fbe_sas_bullet_enclosure_expansion_port_info_t expansion_port_info;

	FBE_LIFECYCLE_DEF_INST_DATA;
}fbe_sas_bullet_enclosure_t;

/* Methods */
/* Class methods forward declaration */
fbe_status_t fbe_sas_bullet_enclosure_load(void);
fbe_status_t fbe_sas_bullet_enclosure_unload(void);
fbe_status_t fbe_sas_bullet_enclosure_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle);
fbe_status_t fbe_sas_bullet_enclosure_destroy_object( fbe_object_handle_t object_handle);
fbe_status_t fbe_sas_bullet_enclosure_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_sas_bullet_enclosure_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context);
fbe_status_t fbe_sas_bullet_enclosure_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_sas_bullet_enclosure_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);

fbe_status_t fbe_sas_bullet_enclosure_monitor(fbe_sas_bullet_enclosure_t * sas_bullet_enclosure, fbe_packet_t * packet);
fbe_status_t fbe_sas_bullet_enclosure_monitor_load_verify(void);

fbe_status_t fbe_sas_bullet_enclosure_create_edge(fbe_sas_bullet_enclosure_t * sas_bullet_enclosure, fbe_packet_t * packet);
fbe_status_t fbe_sas_bullet_enclosure_get_parent_address(fbe_sas_bullet_enclosure_t * sas_bullet_enclosure, fbe_packet_t * packet);

fbe_status_t fbe_sas_bullet_enclosure_init(fbe_sas_bullet_enclosure_t * sas_bullet_enclosure);

/* Executer functions */
fbe_status_t fbe_sas_bullet_enclosure_send_get_element_list_command(fbe_sas_bullet_enclosure_t * sas_bullet_enclosure, fbe_packet_t * packet);

#endif /*SAS_BULLET_ENCLOSURE_PRIVATE_H */
