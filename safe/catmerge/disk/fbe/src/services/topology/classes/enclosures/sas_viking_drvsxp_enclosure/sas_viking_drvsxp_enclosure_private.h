#ifndef SAS_VIKING_DRVSXP_ENCLOSURE_PRIVATE_H
#define SAS_VIKING_DRVSXP_ENCLOSURE_PRIVATE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file sas_viking_drvsxp_enclosure_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions for sas Viking DRVSXP enclosure class.
 *  DRVSXP represents the Drive SAS Expander object. This epxander provides
 *  the connect to the drives and their phys and leds.
 * 
 *  Enclosure environmental status monitoring is handled in
 *  Viking (IOSXP) Enclosure and not part of DRVSXP.
 * 
 *
 * @ingroup fbe_enclosure_class
 *
 * HISTORY
 *   22-Nov-2013:  Greg Bailey - Created.
 *
 ***************************************************************************/

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_eses.h"
#include "fbe_eses_enclosure_private.h"

// This is the number of possible element groups for viking_drvsxp.
// Used to allocate the memory for element group.
// The number of actual element groups will be updated while processing the configuration page. 
#define FBE_SAS_VIKING_DRVSXP_ENCLOSURE_NUMBER_OF_ELEM_GROUPS       25 

/*
 * Defines for the actual components of a viking_drvsxp enclosure, used to size
 * fbe_sas_viking_drvsxp_init_data.
 */
#define FBE_SAS_VIKING_DRVSXP_ENCLOSURE_MAX_COMPONENT_TYPE          FBE_ESES_ENCLOSURE_MAX_COMPONENT_TYPE

// These constants are used to init fbe_sas_viking_drvsxp_init_data and
// should correspond to fbe_enclosure_component_types_t.
// Note: These numbers are not the platform limits. They are the number of ESES elements described in EDAL. 
#define FBE_SAS_VIKING_DRVSXP_ENCLOSURE_NUMBER_OF_COMP_TYPES        5
#define FBE_SAS_VIKING_DRVSXP_ENCLOSURE_NUMBER_OF_POWER_SUPPLIES                0 // no power supply components for drvsxp.
#define FBE_SAS_VIKING_DRVSXP_ENCLOSURE_NUMBER_OF_POWER_SUPPLY_SUBELEMS_PER_PS        0 // no power supply subelements for drvsxp.
#define FBE_SAS_VIKING_DRVSXP_ENCLOSURE_NUMBER_OF_COOLING_COMPONENTS            0 // no cooling components for drvsxp. 
#define FBE_SAS_VIKING_DRVSXP_ENCLOSURE_NUMBER_OF_COOLING_COMPONENTS_PER_PS     0  // no cooling components for drvsxp.
#define FBE_SAS_VIKING_DRVSXP_ENCLOSURE_NUMBER_OF_COOLING_COMPONENTS_ON_CHASSIS 0 // no cooling components for drvsxp.
#define FBE_SAS_VIKING_DRVSXP_ENCLOSURE_NUMBER_OF_COOLING_COMPONENTS_EXTERNAL   0 
#define FBE_SAS_VIKING_DRVSXP_ENCLOSURE_NUMBER_OF_SLOTS             30
#define FBE_SAS_VIKING_DRVSXP_ENCLOSURE_MAX_ENCL_SLOTS             120
#define FBE_SAS_VIKING_DRVSXP_ENCLOSURE_NUMBER_OF_SLOTS_PER_BANK    20

#define FBE_SAS_VIKING_DRVSXP_ENCLOSURE_NUMBER_OF_LCCS               2   // one local lcc and one peer lcc.
#define FBE_SAS_VIKING_DRVSXP_ENCLOSURE_NUMBER_OF_EXPANDERS          2   // The expanders on A side are stored first in EDAL, followed by B side.
#define FBE_SAS_VIKING_DRVSXP_ENCLOSURE_NUMBER_OF_EXPANDERS_PER_LCC  1   // just for this instance
#define FBE_SAS_VIKING_DRVSXP_ENCLOSURE_NUMBER_OF_EXPANDER_PHYS     35 // 30 for drives and 5 for the connector

// The number of temp sensor elements described in EDAL, including overall elements and individual elements. 
#define FBE_SAS_VIKING_DRVSXP_ENCLOSURE_NUMBER_OF_TEMP_SENSORS_PER_LCC      0 // no temp sensor components for drvsxp.
#define FBE_SAS_VIKING_DRVSXP_ENCLOSURE_NUMBER_OF_TEMP_SENSORS_ON_CHASSIS   0 // no cooling components for drvsxp.
#define FBE_SAS_VIKING_DRVSXP_ENCLOSURE_NUMBER_OF_TEMP_SENSORS  ((FBE_SAS_VIKING_DRVSXP_ENCLOSURE_NUMBER_OF_LCCS * FBE_SAS_VIKING_DRVSXP_ENCLOSURE_NUMBER_OF_TEMP_SENSORS_PER_LCC) + FBE_SAS_VIKING_DRVSXP_ENCLOSURE_NUMBER_OF_TEMP_SENSORS_ON_CHASSIS) 

#define FBE_SAS_VIKING_DRVSXP_ENCLOSURE_NUMBER_OF_ENCL                  1
#define FBE_SAS_VIKING_DRVSXP_ENCLOSURE_NUMBER_OF_DISPLAYS              0   // no led display components for drvsxp.
#define FBE_SAS_VIKING_DRVSXP_ENCLOSURE_NUMBER_OF_TWO_DIGIT_DISPLAYS    0   // no led display components for drvsxp.
#define FBE_SAS_VIKING_DRVSXP_ENCLOSURE_NUMBER_OF_ONE_DIGIT_DISPLAYS    0   // no led display components for drvsxp.
#define FBE_SAS_VIKING_DRVSXP_ENCLOSURE_NUMBER_OF_IMAGES_PER_LCC        5        
#define FBE_SAS_VIKING_DRVSXP_ENCLOSURE_NUMBER_OF_POWER_SUPPLY_IMAGE    0   
#define FBE_SAS_VIKING_DRVSXP_ENCLOSURE_NUMBER_OF_SPS                   0
#define FBE_SAS_VIKING_DRVSXP_ENCLOSURE_NUMBER_OF_DRIVE_MIDPLANE        0

/* 
 * Each side has two entire connectors and each entire connector has multiple individual lanes. 
 * The following number of connectors is the total number of the entire connectors and individual lanes.
 * The connectors on A side are stored first in EDAL, followed by B side.
 */
#define FBE_SAS_VIKING_DRVSXP_ENCLOSURE_NUMBER_OF_CONNECTORS                    12
#define FBE_SAS_VIKING_DRVSXP_ENCLOSURE_NUMBER_OF_CONNECTORS_PER_LCC            6
#define FBE_SAS_VIKING_DRVSXP_ENCLOSURE_NUMBER_OF_LANES_PER_ENTIRE_CONNECTOR    5

#define FBE_SAS_VIKING_DRVSXP_ENCLOSURE_FIRST_SLOT_INDEX            0

#define FBE_SAS_VIKING_DRVSXP_ENCLOSURE_NUMBER_OF_EXPANSION_PORTS   1  // only incoming port for drvsxp 

#define FBE_SAS_VIKING_DRVSXP_ENCLOSURE_FIRST_EXPANSION_PORT_INDEX  121  // first 120 are for drives

#define FBE_SAS_VIKING_DRVSXP_ENCLOSURE_MAX_NUMBER_OF_DRIVES_SPINNINGUP 30

/*
 * Firmware Update Related Info
 */
/* since viking_drvsxp not supporting SPS */
#define FBE_SAS_VIKING_DRVSXP_ENCLOSURE_FW_DL_RETRY_MAX 5



// Constants used to size the data structure that stores
// the firmware revision information.
#define FBE_VIKING_DRVSXP_FW_NUM_EXPANDER_ELEMENTS  1   // expanders per lcc
#define FBE_VIKING_DRVSXP_FW_NUM_SUBENCL_SIDES      1   // number of lcc to size array
#define FBE_VIKING_DRVSXP_FW_LCC_VERSION_DESCS      5   // version descriptors in the lcc subencl
#define FBE_VIKING_DRVSXP_FW_PS_VERSION_DESCS       1   // version descriptors in the ps subencl


#define FBE_SAS_VIKING_DRVSXP_ENCLOSURE_DEFAULT_RIDE_THROUGH_PERIOD 12 /* 12 seconds */



/* These are the lifecycle condition ids for a sas viking_drvsxp enclosure object. */
typedef enum fbe_sas_viking_drvsxp_enclosure_lifecycle_cond_id_e {
    FBE_SAS_VIKING_DRVSXP_LIFECYCLE_COND_FOO = FBE_LIFECYCLE_COND_FIRST_ID(FBE_CLASS_ID_SAS_VIKING_DRVSXP_LCC),
    FBE_SAS_VIKING_DRVSXP_ENCLOSURE_LIFECYCLE_COND_LAST 
     /* must be last */
} fbe_sas_viking_drvsxp_enclosure_lifecycle_cond_id_t;



extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(sas_viking_drvsxp_enclosure);

typedef struct fbe_sas_viking_drvsxp_enclosure_s
{
    fbe_eses_enclosure_t                            eses_enclosure;
    

    FBE_LIFECYCLE_DEF_INST_DATA;
 
    FBE_LIFECYCLE_DEF_INST_COND(FBE_LIFECYCLE_COND_MAX(FBE_SAS_VIKING_DRVSXP_ENCLOSURE_LIFECYCLE_COND_LAST));
}fbe_sas_viking_drvsxp_enclosure_t;

/* Methods */
/* Class methods forward declaration */
fbe_status_t fbe_sas_viking_drvsxp_enclosure_load(void);
fbe_status_t fbe_sas_viking_drvsxp_enclosure_unload(void);
fbe_status_t fbe_sas_viking_drvsxp_enclosure_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle);
fbe_status_t fbe_sas_viking_drvsxp_enclosure_destroy_object( fbe_object_handle_t object_handle);
fbe_status_t fbe_sas_viking_drvsxp_enclosure_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_sas_viking_drvsxp_enclosure_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context);
fbe_status_t fbe_sas_viking_drvsxp_enclosure_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_sas_viking_drvsxp_enclosure_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);

fbe_status_t fbe_sas_viking_drvsxp_enclosure_monitor(fbe_sas_viking_drvsxp_enclosure_t * sas_viking_drvsxp_enclosure, fbe_packet_t * packet);
fbe_status_t fbe_sas_viking_drvsxp_enclosure_monitor_load_verify(void);

fbe_status_t fbe_sas_viking_drvsxp_enclosure_create_edge(fbe_sas_viking_drvsxp_enclosure_t * sas_viking_drvsxp_enclosure, fbe_packet_t * packet);
fbe_status_t fbe_sas_viking_drvsxp_enclosure_get_parent_address(fbe_sas_viking_drvsxp_enclosure_t * sas_viking_drvsxp_enclosure, fbe_packet_t * packet);

fbe_status_t fbe_sas_viking_drvsxp_enclosure_init(fbe_sas_viking_drvsxp_enclosure_t * sas_viking_drvsxp_enclosure);

/* Executer functions */

/* fbe_sas_viking_drvsxp_enclosure_read.c */

/* fbe_sas_viking_drvsxp_enclosure_main.c*/
fbe_status_t fbe_sas_viking_drvsxp_format_encl_data(fbe_enclosure_component_block_t   *sas_viking_drvsxp_component_block);
void fbe_sas_viking_drvsxp_enclosure_init_eses_parameters(fbe_sas_viking_drvsxp_enclosure_t *sas_viking_drvsxp_enclosure);

/* fbe_sas_viking_drvsxp_enclosure_build_pages.c */


#endif /*SAS_VIKING_DRVSXP_ENCLOSURE_PRIVATE_H */
