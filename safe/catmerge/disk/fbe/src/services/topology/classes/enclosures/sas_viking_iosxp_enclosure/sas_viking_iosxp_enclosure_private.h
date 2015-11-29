#ifndef SAS_VIKING_IOSXP_ENCLOSURE_PRIVATE_H
#define SAS_VIKING_IOSXP_ENCLOSURE_PRIVATE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file sas_viking_iosxp_enclosure_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions for sas viking enclosure.
 *
 * @ingroup fbe_enclosure_class
 *
 * HISTORY
 *   22-Nov-2013:  Greg Bailey - Created.
 *
 ***************************************************************************/

#include "fbe/fbe_eses.h"
#include "fbe_eses_enclosure_private.h"

// This is the number of possible element groups for viking.
// Used to allocate the memory for element group.
// The number of actual element groups will be updated while processing the configuration page. 
#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_NUMBER_OF_ELEM_GROUPS       36

/*
 * Defines for the actual components of a viking enclosure, used to size
 * fbe_sas_viking_init_data.
 */
#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_MAX_COMPONENT_TYPE          FBE_ESES_ENCLOSURE_MAX_COMPONENT_TYPE



// These constants are used to init fbe_sas_viking_init_data and
// should correspond to fbe_enclosure_component_types_t.
// Note: These numbers are not the platform limits. They are the number of ESES elements described in EDAL. 
#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_NUMBER_OF_COMP_TYPES        5
#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_NUMBER_OF_POWER_SUPPLIES    4

#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_PS_OVERALL_ELEM_SAVED 1  /* Save PS overall element in EDAL */
#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_NUMBER_OF_POWER_SUPPLY_SUBELEMS_PER_PS    3   // One overall element + 2 individual elements
#define FBE_SAS_VIKING_IOSXP_TOTAL_PS_SUBELEM   (FBE_SAS_VIKING_IOSXP_ENCLOSURE_NUMBER_OF_POWER_SUPPLIES * FBE_SAS_VIKING_IOSXP_ENCLOSURE_NUMBER_OF_POWER_SUPPLY_SUBELEMS_PER_PS)
#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_NUMBER_OF_POWER_SUPPLIES_PER_SIDE    2

#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_NUMBER_OF_COOLING_COMPONENTS               11
#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_NUMBER_OF_COOLING_COMPONENTS_PER_PS        0  // There are no cooling elements in PS.
#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_NUMBER_OF_COOLING_COMPONENTS_ON_CHASSIS    11 // One overall element + 10 individual element.
#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_NUMBER_OF_COOLING_COMPONENTS_EXTERNAL      0  // The fans on Viking are not individual subenclosures.
#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_NUMBER_OF_SLOTS            0
#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_MAX_ENCL_SLOTS             120
#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_NUMBER_OF_SLOTS_PER_BANK   20

#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_NUMBER_OF_LCCS                 2   // one local and one peer
#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_NUMBER_OF_TEMP_SENSORS_PER_LCC 2   // 2 per lcc
#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_NUMBER_OF_TEMP_SENSORS_ON_CHASSIS  10 // one overall element + 9 individual element for chassis temperature sensor.
#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_NUMBER_OF_TEMP_SENSORS    ((FBE_SAS_VIKING_IOSXP_ENCLOSURE_NUMBER_OF_LCCS * FBE_SAS_VIKING_IOSXP_ENCLOSURE_NUMBER_OF_TEMP_SENSORS_PER_LCC) + FBE_SAS_VIKING_IOSXP_ENCLOSURE_NUMBER_OF_TEMP_SENSORS_ON_CHASSIS)
#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_NUMBER_OF_EXPANDERS            2   // The expanders on A side are stored first in EDAL, followed by B side.
#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_NUMBER_OF_EXPANDERS_PER_LCC    1   
#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_NUMBER_OF_DRVSXP_PER_LCC       4   // number of DRVSXP per LCC board.
#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_NUMBER_OF_EXPANDER_PHYS        36  // 4 connectors(connector id 0,1,6, 7) have 4 lanes and 4 connecters (connector id 2,3,4, 5) have 5 lanes
#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_NUMBER_OF_ENCL                 1
#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_NUMBER_OF_DISPLAYS             3  // 3 display characters
#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_NUMBER_OF_TWO_DIGIT_DISPLAYS   1  // 2 chars for bus number
#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_NUMBER_OF_ONE_DIGIT_DISPLAYS   1  // 1 char for encl position
#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_NUMBER_OF_IMAGES_PER_LCC       5        
#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_NUMBER_OF_POWER_SUPPLY_IMAGE   2 
#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_NUMBER_OF_SPS                  1
#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_NUMBER_OF_DRIVE_MIDPLANE       0
#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_NUMBER_OF_SSC                  1

/* 
 * Each side has two entire connectors and each entire connector has multiple individual lanes. 
 * The following number of connectors is the total number of the entire connectors and individual lanes.
 * The connectors on A side are stored first in EDAL, followed by B side.
 */
#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_NUMBER_OF_CONNECTORS             88
#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_NUMBER_OF_CONNECTORS_PER_LCC     44
#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_NUMBER_OF_LANES_PER_ENTIRE_INTERNAL_CONNECTOR     5
#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_NUMBER_OF_LANES_PER_ENTIRE_EXTERNAL_CONNECTOR     4


#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_FIRST_SLOT_INDEX                   0

#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_NUMBER_OF_EXPANSION_PORTS          8 /* 1 incoming, 1 outgoing, 4 internal, 2 either unused or used for x8 connector */

#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_FIRST_EXPANSION_PORT_INDEX         1 /* 0 drives*/

#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_MAX_NUMBER_OF_DRIVES_SPINNINGUP    120

/*
 * Firmware Update Related Info
 */
/* since magnum not supporting SPS */
#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_FW_DL_RETRY_MAX          5



// Constants used to size the data structure that stores
// the firmware revision information.
#define FBE_VIKING_FW_NUM_EXPANDER_ELEMENTS  (FBE_SAS_VIKING_IOSXP_ENCLOSURE_NUMBER_OF_EXPANDERS_PER_LCC)   // expanders per lcc
#define FBE_VIKING_FW_NUM_SUBENCL_SIDES      1   // number of lcc to size array
#define FBE_VIKING_FW_LCC_VERSION_DESCS      5   // version descriptors in the lcc subencl
#define FBE_VIKING_FW_PS_VERSION_DESCS       1   // version descriptors in the ps subencl


#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_DEFAULT_RIDE_THROUGH_PERIOD 12 /* 12 seconds */

#define FBE_SAS_VIKING_IOSXP_ENCLOSURE_PS_RESUME_FORMAT_SPECIAL  1 /* Viking PS uses PMBUS format */

/* These are the lifecycle condition ids for a sas magnum enclosure object. */
typedef enum fbe_sas_viking_iosxp_enclosure_lifecycle_cond_id_e {
    FBE_SAS_VIKING_IOSXP_LIFECYCLE_COND_FOO = FBE_LIFECYCLE_COND_FIRST_ID(FBE_CLASS_ID_SAS_VIKING_IOSXP_ENCLOSURE),
    FBE_SAS_VIKING_IOSXP_ENCLOSURE_LIFECYCLE_COND_LAST 
     /* must be last */
} fbe_sas_viking_iosxp_enclosure_lifecycle_cond_id_t;



extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(sas_viking_iosxp_enclosure);

typedef struct fbe_sas_viking_iosxp_enclosure_s
{
    fbe_eses_enclosure_t    eses_enclosure;
    FBE_LIFECYCLE_DEF_INST_DATA;
    FBE_LIFECYCLE_DEF_INST_COND(FBE_LIFECYCLE_COND_MAX(FBE_SAS_VIKING_IOSXP_ENCLOSURE_LIFECYCLE_COND_LAST));
}fbe_sas_viking_iosxp_enclosure_t;

/* Methods */
/* Class methods forward declaration */
fbe_status_t fbe_sas_viking_iosxp_enclosure_load(void);
fbe_status_t fbe_sas_viking_iosxp_enclosure_unload(void);
fbe_status_t fbe_sas_viking_iosxp_enclosure_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle);
fbe_status_t fbe_sas_viking_iosxp_enclosure_destroy_object( fbe_object_handle_t object_handle);
fbe_status_t fbe_sas_viking_iosxp_enclosure_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_sas_viking_iosxp_enclosure_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context);
fbe_status_t fbe_sas_viking_iosxp_enclosure_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_sas_viking_iosxp_enclosure_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);


fbe_status_t fbe_sas_viking_iosxp_enclosure_monitor_load_verify(void);
fbe_status_t fbe_sas_viking_iosxp_enclosure_init(fbe_sas_viking_iosxp_enclosure_t * sas_viking_enclosure);

// fbe_sas_viking_enclosure_main.c
fbe_status_t fbe_sas_viking_iosxp_format_encl_data(fbe_enclosure_component_block_t   *sas_viking_component_block);
void fbe_sas_viking_iosxp_enclosure_init_eses_parameters(fbe_sas_viking_iosxp_enclosure_t *sas_viking_enclosure);

#endif /*SAS_VIKING_IOSXP_ENCLOSURE_PRIVATE_H */
