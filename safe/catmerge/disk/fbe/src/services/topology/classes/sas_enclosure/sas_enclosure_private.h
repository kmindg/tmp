#ifndef SAS_ENCLOSURE_PRIVATE_H
#define SAS_ENCLOSURE_PRIVATE_H

#include "fbe/fbe_winddk.h"
#include "fbe_base_enclosure.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_transport.h"
#include "base_enclosure_private.h"

#include "fbe_ssp_transport.h"
#include "fbe_smp_transport.h"
#include "fbe_sas_enclosure.h"

#define SAS_ENCLOSURE_MAX_ENCLOSURES 10

#define SAS_ENCLOSURE_LOG_SUPPRESS_COUNT  100

#define FBE_SAS_TOPOLOGY_DEPTH_ADJUSTMENT 1

extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(sas_enclosure);

/*
 * These are the lifecycle condition ids for a sas enclosure object. 
 * The order of these conditions must match the order in the 
 * FBE_LIFECYCLE_BASE_COND_ARRAY.
 */
typedef enum fbe_sas_enclosure_lifecycle_cond_id_e {
    FBE_SAS_ENCLOSURE_LIFECYCLE_COND_ATTACH_SMP_EDGE = FBE_LIFECYCLE_COND_FIRST_ID(FBE_CLASS_ID_SAS_ENCLOSURE),
    /* Specialize rotary conditions */
    FBE_SAS_ENCLOSURE_LIFECYCLE_COND_GET_SMP_ADDRESS,
    FBE_SAS_ENCLOSURE_LIFECYCLE_COND_OPEN_SMP_EDGE,
    FBE_SAS_ENCLOSURE_LIFECYCLE_COND_GET_VIRTUAL_PORT_ADDRESS,
    FBE_SAS_ENCLOSURE_LIFECYCLE_COND_OPEN_SSP_EDGE,
    FBE_SAS_ENCLOSURE_LIFECYCLE_COND_INQUIRY,

    /* Ready rotary conditions */
    FBE_SAS_ENCLOSURE_LIFECYCLE_COND_GET_ELEMENT_LIST,

	FBE_SAS_ENCLOSURE_LIFECYCLE_COND_DETACH_SSP_EDGE, /* Preset in destroy rotary */
	FBE_SAS_ENCLOSURE_LIFECYCLE_COND_DETACH_SMP_EDGE, /* Preset in destroy rotary */

    FBE_SAS_ENCLOSURE_LIFECYCLE_COND_LAST /* must be last */
} fbe_sas_enclosure_lifecycle_cond_id_t;

/*
 * Data for SAS Enclosure Object (includes Base Enclosure Object data)
 */
typedef struct fbe_sas_enclosure_s{
    fbe_base_enclosure_t        base_enclosure;
    fbe_ssp_edge_t              ssp_edge;
    fbe_smp_edge_t              smp_edge;

//    fbe_u32_t enclosures_number;      // already in base enclosure
    fbe_u32_t                   io_enclosure_number;
    fbe_sas_address_t           sasEnclosureSMPAddress;
    fbe_generation_code_t       smp_address_generation_code;

    fbe_sas_enclosure_type_t    sasEnclosureType; /* This field will be updated by first inquiry */
    fbe_sas_enclosure_product_type_t    sasEnclosureProductType;
    fbe_sas_address_t ses_port_address;
    fbe_generation_code_t ses_port_address_generation_code;

    FBE_LIFECYCLE_DEF_INST_DATA;
    FBE_LIFECYCLE_DEF_INST_COND(FBE_LIFECYCLE_COND_MAX(FBE_SAS_ENCLOSURE_LIFECYCLE_COND_LAST));
}fbe_sas_enclosure_t;


/* All FBE timeouts in miliseconds */
enum fbe_sas_enclosure_timeouts_e {
    FBE_SAS_ENCLOSURE_INQUIRY_TIMEOUT = 10000, /* 10 sec */
    FBE_SAS_ENCLOSURE_CONFIG_PAGE_TIMEOUT = 10000 /* 10 sec */
};

static __forceinline 
fbe_sas_enclosure_type_t fbe_sas_enclosure_get_enclosure_type(fbe_sas_enclosure_t *sas_enclosure)
{
    return (sas_enclosure->sasEnclosureType);
}

static __forceinline 
fbe_status_t fbe_sas_enclosure_set_enclosure_type(fbe_sas_enclosure_t *sas_enclosure, fbe_sas_enclosure_type_t type)
{
    sas_enclosure->sasEnclosureType = type;
    return FBE_STATUS_OK;
}

/* Methods */
fbe_status_t fbe_sas_enclosure_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle);
fbe_status_t fbe_sas_enclosure_destroy_object( fbe_object_handle_t object_handle);
fbe_status_t fbe_sas_enclosure_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_sas_enclosure_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context);
fbe_status_t fbe_sas_enclosure_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_sas_enclosure_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);

fbe_status_t fbe_sas_enclosure_monitor_load_verify(void);

fbe_status_t fbe_sas_enclosure_monitor(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_sas_enclosure_create_edge(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet);
fbe_status_t fbe_sas_enclosure_get_expander_list(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet);

fbe_status_t fbe_sas_enclosure_check_reset_ride_through_needed(fbe_base_object_t * p_object);
fbe_status_t fbe_sas_enclosure_send_ssp_functional_packet(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet);
fbe_status_t fbe_sas_enclosure_send_smp_functional_packet(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet);
fbe_status_t fbe_sas_enclosure_send_ssp_control_packet(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet);
fbe_status_t fbe_sas_enclosure_send_smp_control_packet(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet);
fbe_status_t fbe_sas_enclosure_get_smp_path_attributes(fbe_sas_enclosure_t * sas_enclosure, fbe_path_attr_t * path_attr);
fbe_status_t fbe_sas_enclosure_get_ssp_path_attributes(fbe_sas_enclosure_t * sas_enclosure, fbe_path_attr_t * path_attr);

static __inline fbe_status_t 
fbe_sas_enclosure_get_sas_address(fbe_sas_enclosure_t * sas_enclosure, fbe_sas_address_t * sas_address)
{
    *sas_address = sas_enclosure->sasEnclosureSMPAddress;
	return FBE_STATUS_OK;
}

static __inline fbe_status_t 
fbe_sas_enclosure_get_encl_type(fbe_sas_enclosure_t * sas_enclosure, fbe_sas_enclosure_type_t * encl_type)
{
    *encl_type = sas_enclosure->sasEnclosureType;
	return FBE_STATUS_OK;
}

/* Usurper functions */
fbe_status_t fbe_sas_enclosure_attach_ssp_edge(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet);
fbe_status_t fbe_sas_enclosure_open_ssp_edge(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet);
fbe_status_t fbe_sas_enclosure_detach_ssp_edge(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet);
fbe_status_t fbe_sas_enclosure_attach_smp_edge(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet);
fbe_status_t fbe_sas_enclosure_open_smp_edge(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet);
fbe_status_t fbe_sas_enclosure_detach_smp_edge(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet);

/* Executer functions */
fbe_status_t fbe_sas_enclosure_get_smp_address(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet);
fbe_status_t fbe_sas_enclosure_send_inquiry(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet);
fbe_status_t fbe_sas_enclosure_send_get_element_list_command(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet);

fbe_packet_t * fbe_sas_enclosure_build_inquiry_packet(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet);

fbe_packet_t * fbe_sas_enclosure_build_get_element_list_packet(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet);
fbe_status_t fbe_sas_enclosure_discovery_transport_entry(fbe_sas_enclosure_t * sas_enclosure, fbe_packet_t * packet);


#endif /* SAS_ENCLOSURE_PRIVATE_H */
