#ifndef SAS_VOYAGER_ICM_ENCLOSURE_PRIVATE_H
#define SAS_VOYAGER_ICM_ENCLOSURE_PRIVATE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file sas_voyager_icm_enclosure_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions for sas voyager icm enclosure.
 *
 * @ingroup fbe_enclosure_class
 *
 * HISTORY
 *   26-Feb-2010:  Created from sas_magnum_enclosure_private.h. Dipak Patel
 *
 ***************************************************************************/

#include "fbe/fbe_eses.h"
#include "fbe_eses_enclosure_private.h"

// This is the number of possible element groups for voyager icm.
// Used to allocate the memory for element group.
// The number of actual element groups will be updated while processing the configuration page. 
#define FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_ELEM_GROUPS       36

/*
 * Defines for the actual components of a voyager icm enclosure, used to size
 * fbe_sas_voyager_icm_init_data.
 */
#define FBE_SAS_VOYAGER_ICM_ENCLOSURE_MAX_COMPONENT_TYPE          FBE_ESES_ENCLOSURE_MAX_COMPONENT_TYPE



// These constants are used to init fbe_sas_voyager_icm_init_data and
// should correspond to fbe_enclosure_component_types_t.
// Note: These numbers are not the platform limits. They are the number of ESES elements described in EDAL. 
#define FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_COMP_TYPES        5
#define FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_POWER_SUPPLIES    2

#define FBE_SAS_VOYAGER_ICM_ENCLOSURE_PS_OVERALL_ELEM_SAVED 1  /* Save PS overall element in EDAL */
#define FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_POWER_SUPPLY_SUBELEMS_PER_PS    5  // One overall + 4 invidual elements
#define FBE_SAS_VOYAGER_TOTAL_PS_SUBELEM   (FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_POWER_SUPPLIES * FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_POWER_SUPPLY_SUBELEMS_PER_PS)
#define FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_POWER_SUPPLIES_PER_SIDE    1

#define FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_COOLING_COMPONENTS   \
    ((FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_POWER_SUPPLIES * \
      FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_COOLING_COMPONENTS_PER_PS) + \
     FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_COOLING_COMPONENTS_ON_CHASSIS + \
     (FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_COOLING_COMPONENTS_EXTERNAL * \
      FBE_ESES_NUMBER_OF_ELEMENTS_PER_EXTERNAL_COOLING_UNIT) )

#define FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_COOLING_COMPONENTS_PER_PS  3/* one overall+2 in each of 2 power supplies.*/
#define FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_COOLING_COMPONENTS_ON_CHASSIS   1 // one overall element + zero individual element.
#define FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_COOLING_COMPONENTS_EXTERNAL   3 // 3 groups of (one overall element + one individual element).
#define FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_SLOTS             0 
#define FBE_SAS_VOYAGER_ICM_ENCLOSURE_MAX_ENCL_SLOTS             60
#define FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_SLOTS_PER_BANK             12

// The number of temp sensor elements described in EDAL, including overall elements and individual elements:
#define FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_TEMP_SENSORS    ((FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_EE_LCC * FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_TEMP_SENSORS_PER_LCC) + FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_TEMP_SENSORS_ON_CHASSIS)
#define FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_TEMP_SENSORS_PER_LCC  2
#define FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_TEMP_SENSORS_ON_CHASSIS  3 // one overall element + 2 individual element for Air Inlet temperature sensor.
#define FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_EXPANDERS         2   // The expanders on A side are stored first in EDAL, followed by B side.
#define FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_EXPANDERS_PER_LCC    1   
#define FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_EXPANDER_PHYS     24  // six connector 4x wide ports
#define FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_ENCL              1
#define FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_DISPLAYS          3  // 3 display characters on local LCC.
#define FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_TWO_DIGIT_DISPLAYS         1   // 2 display characters on local LCC.
#define FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_ONE_DIGIT_DISPLAYS         1  // 1 display character on local LCC.
#define FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_IMAGES_PER_LCC         5        
#define FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_LCCS              2   // number of ICMs per voyager (one local and one peer).
#define FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_EE_LCC            2   // number of EE_LCC boards per voyager (one local and one peer).
#define FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_EE_PER_EE_LCC     2   // number of EE Chips per EE_LCC board.
// Defines the number of LCCs (of any type) in the Voyager enclosure (both sides)
#define FBE_SAS_VOYAGER_ENCLOSURE_NUMBER_OF_LCCS                  (FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_LCCS + FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_EE_LCC)
#define FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_POWER_SUPPLY_IMAGE     2	
#define FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_SPS               1
#define FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_DRIVE_MIDPLANE    0

/* 
 * Each side has two entire connectors and each entire connector has multiple individual lanes. 
 * The following number of connectors is the total number of the entire connectors and individual lanes.
 * The connectors on A side are stored first in EDAL, followed by B side.
 */
#define FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_CONNECTORS                    60
#define FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_CONNECTORS_PER_LCC            30
#define FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_LANES_PER_ENTIRE_CONNECTOR    4

#define FBE_SAS_VOYAGER_ICM_ENCLOSURE_FIRST_SLOT_INDEX  0

#define FBE_SAS_VOYAGER_ICM_ENCLOSURE_NUMBER_OF_EXPANSION_PORTS  6 /* 1 incoming, 1 outgoing, 2 internal, 2 unused */

#define FBE_SAS_VOYAGER_ICM_ENCLOSURE_FIRST_EXPANSION_PORT_INDEX 1 /* 0 drives*/

#define FBE_SAS_VOYAGER_ICM_ENCLOSURE_MAX_NUMBER_OF_DRIVES_SPINNINGUP 60

/*
 * Firmware Update Related Info
 */
/* since magnum not supporting SPS */
#define FBE_SAS_VOYAGER_ICM_ENCLOSURE_FW_DL_RETRY_MAX          5



// Constants used to size the data structure that stores
// the firmware revision information.
#define FBE_VOYAGER_ICM_FW_NUM_EXPANDER_ELEMENTS  1   // expanders per lcc
#define FBE_VOYAGER_ICM_FW_NUM_SUBENCL_SIDES      1   // number of lcc to size array
#define FBE_VOYAGER_ICM_FW_LCC_VERSION_DESCS      5   // version descriptors in the lcc subencl
#define FBE_VOYAGER_ICM_FW_PS_VERSION_DESCS       1   // version descriptors in the ps subencl


#define FBE_SAS_VOYAGER_ICM_ENCLOSURE_DEFAULT_RIDE_THROUGH_PERIOD 12 /* 12 seconds */



/* These are the lifecycle condition ids for a sas magnum enclosure object. */
typedef enum fbe_sas_voyager_icm_enclosure_lifecycle_cond_id_e {
    FBE_SAS_VOYAGER_ICM_LIFECYCLE_COND_FOO = FBE_LIFECYCLE_COND_FIRST_ID(FBE_CLASS_ID_SAS_VOYAGER_ICM_ENCLOSURE),
    FBE_SAS_VOYAGER_ICM_ENCLOSURE_LIFECYCLE_COND_LAST 
     /* must be last */
} fbe_sas_voyager_icm_enclosure_lifecycle_cond_id_t;



extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(sas_voyager_icm_enclosure);

typedef struct fbe_sas_voyager_icm_enclosure_s
{
    fbe_eses_enclosure_t                            eses_enclosure;
    

    FBE_LIFECYCLE_DEF_INST_DATA;
 
    FBE_LIFECYCLE_DEF_INST_COND(FBE_LIFECYCLE_COND_MAX(FBE_SAS_VOYAGER_ICM_ENCLOSURE_LIFECYCLE_COND_LAST));
}fbe_sas_voyager_icm_enclosure_t;

/* Methods */
/* Class methods forward declaration */
fbe_status_t fbe_sas_voyager_icm_enclosure_load(void);
fbe_status_t fbe_sas_voyager_icm_enclosure_unload(void);
fbe_status_t fbe_sas_voyager_icm_enclosure_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle);
fbe_status_t fbe_sas_voyager_icm_enclosure_destroy_object( fbe_object_handle_t object_handle);
fbe_status_t fbe_sas_voyager_icm_enclosure_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_sas_voyager_icm_enclosure_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context);
fbe_status_t fbe_sas_voyager_icm_enclosure_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_sas_voyager_icm_enclosure_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);


fbe_status_t fbe_sas_voyager_icm_enclosure_monitor_load_verify(void);

fbe_status_t fbe_sas_voyager_icm_enclosure_init(fbe_sas_voyager_icm_enclosure_t * sas_voyager_icm_enclosure);

/* fbe_sas_voyager_icm_enclosure_main.c*/
fbe_status_t fbe_sas_voyager_icm_format_encl_data(fbe_enclosure_component_block_t   *sas_voyager_icm_component_block);
void fbe_sas_voyager_icm_enclosure_init_eses_parameters(fbe_sas_voyager_icm_enclosure_t *sas_voyager_icm_enclosure);

#endif /*SAS_VOYAGER_ICM_ENCLOSURE_PRIVATE_H */
