#ifndef SAS_ANCHO_ENCLOSURE_PRIVATE_H
#define SAS_ANCHO_ENCLOSURE_PRIVATE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file sas_ancho_enclosure_private.h
 ***************************************************************************
 * @brief
 *  This file contains definitions for sas ancho enclosure.
 * @ingroup fbe_enclosure_class
 *
 * HISTORY
 *   24-Mar-2012:  Created. Jaleel Kazi
 *    Notes from the system specification TODO: Remove it in the end, This file needs to be 
 *                                              updated with the Ancho component details....
 *    - Ancho supports up to 15 SAS 3.0 12 Gb/S, 6 Gbit/S, 2 .5-inch or 3.5-inch disk drives
 *    - The chassis and Power Supply/Cooling module will be reused from Viper
 *    - Midplane and LCC will be redesigned
 *	  - Enclosure will be managed by host through the ESES in-band interface
 *    - CDES-2 based design
 *    - Ancho DAE is composed of one Midplane, 2 Power Supply/Cooling Module and 2 LCC(LCCA&LCCB)
 *    - Front Bezel could provide cosmetic as well as EMI shielding. reused from Viper.
 *    - Ancho Midplane is leveraged from Viper
 *    - Temperature sensor called sight is reused from Sentry
 *        - A temperature sensor called Sight is reused from Sentry15, EMC part number 303-169-000C
 *    - The Midplane resume (Enclosure resume) consists of a 4KB EEPROM that resides on I2C Global Bus #1 
 *      for the LCC in slot B and Bus #0 for the LCC in slot A.  
 *    - Ancho’s Power Supply is reused from Viper.  
 *          The AC version is 3rd Gen Katina VE PSU (071-000-553 A07 from Acbel and 071-000-554 A07 
 *          from Emerson).  
 *
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_eses.h"
#include "fbe_eses_enclosure_private.h"

// This is the number of possible element groups for anchos.
// Used to allocate the memory for element group.
// The number of actual element groups will be updated while processing the configuration page. 
#define FBE_SAS_ANCHO_ENCLOSURE_NUMBER_OF_POSSIBLE_ELEM_GROUPS       28

/*
 * Defines for the actual components of a Ancho enclosure, used to size
 * fbe_sas_ancho_init_data.
 */
#define FBE_SAS_ANCHO_ENCLOSURE_MAX_COMPONENT_TYPE          FBE_ESES_ENCLOSURE_MAX_COMPONENT_TYPE

// These constants are used to init fbe_sas_ancho_init_data and
// should correspond to fbe_enclosure_component_types_t.
// Note: These numbers are not the platform limits. They are the number of ESES elements described in EDAL.
/* Ancho: Two independent cooling modules inside PSU provide the front-to-rear airflow inside Ancho.  
   Each cooling module houses 2 blowers. */
#define FBE_SAS_ANCHO_ENCLOSURE_NUMBER_OF_COMP_TYPES                       5
#define FBE_SAS_ANCHO_ENCLOSURE_NUMBER_OF_POWER_SUPPLIES                   2 

#define FBE_SAS_ANCHO_ENCLOSURE_PS_OVERALL_ELEM_SAVED 1                       // Save PS overall element in EDAL
#define FBE_SAS_ANCHO_ENCLOSURE_NUMBER_OF_POWER_SUPPLY_SUBELEMS_PER_PS    2   // One overall element + 1 individual element
#define FBE_SAS_ANCHO_TOTAL_PS_SUBELEM   (FBE_SAS_ANCHO_ENCLOSURE_NUMBER_OF_POWER_SUPPLIES * FBE_SAS_ANCHO_ENCLOSURE_NUMBER_OF_POWER_SUPPLY_SUBELEMS_PER_PS)
#define FBE_SAS_ANCHO_ENCLOSURE_NUMBER_OF_POWER_SUPPLIES_PER_SIDE    1

#define FBE_SAS_ANCHO_ENCLOSURE_NUMBER_OF_COOLING_COMPONENTS               7 // The number of cooling elements described in EDAL, including overall elements and individual elements. 
#define FBE_SAS_ANCHO_ENCLOSURE_NUMBER_OF_COOLING_COMPONENTS_PER_PS        3 // one overall element + two individual elements.
#define FBE_SAS_ANCHO_ENCLOSURE_NUMBER_OF_COOLING_COMPONENTS_ON_CHASSIS    1 // one overall element + zero individual element.
#define FBE_SAS_ANCHO_ENCLOSURE_NUMBER_OF_COOLING_COMPONENTS_EXTERNAL      0
#define FBE_SAS_ANCHO_ENCLOSURE_NUMBER_OF_SLOTS                            15 /*!< Indexes 0-14 for slots and 15 for expansion port */
#define FBE_SAS_ANCHO_ENCLOSURE_MAX_ENCL_SLOTS                             15
#define FBE_SAS_ANCHO_ENCLOSURE_NUMBER_OF_SLOTS_PER_BANK                   0

// The number of temp sensor elements described in EDAL, including overall elements and individual elements. 
/* Ancho: Temperature sensor called Sight is reused from Sentry15 - EMC part number 303-169-000C    */
#define FBE_SAS_ANCHO_ENCLOSURE_NUMBER_OF_LCCS_WITH_TEMP_SENSORS 2
#define FBE_SAS_ANCHO_ENCLOSURE_NUMBER_OF_TEMP_SENSORS_PER_LCC     3  // one overall element + two individual element.
#define FBE_SAS_ANCHO_ENCLOSURE_NUMBER_OF_TEMP_SENSORS_ON_CHASSIS  11
#define FBE_SAS_ANCHO_ENCLOSURE_NUMBER_OF_TEMP_SENSORS      ((FBE_SAS_ANCHO_ENCLOSURE_NUMBER_OF_LCCS_WITH_TEMP_SENSORS * FBE_SAS_ANCHO_ENCLOSURE_NUMBER_OF_TEMP_SENSORS_PER_LCC) + FBE_SAS_ANCHO_ENCLOSURE_NUMBER_OF_TEMP_SENSORS_ON_CHASSIS) 

/* Ancho LCC details */
#define FBE_SAS_ANCHO_ENCLOSURE_NUMBER_OF_EXPANDERS                2  // The expanders on A side are stored first in EDAL, followed by B side.
#define FBE_SAS_ANCHO_ENCLOSURE_NUMBER_OF_EXPANDERS_PER_LCC        1   
#define FBE_SAS_ANCHO_ENCLOSURE_NUMBER_OF_EXPANDER_PHYS            23 // 15 (drives) + 8 (two connector wide ports)
#define FBE_SAS_ANCHO_ENCLOSURE_NUMBER_OF_ENCL                     1
#define FBE_SAS_ANCHO_ENCLOSURE_NUMBER_OF_DISPLAY_CHARS            3  // 3 display characters on local LCC.
#define FBE_SAS_ANCHO_ENCLOSURE_NUMBER_OF_TWO_DIGIT_DISPLAYS       1  // 2 display characters on local LCC.
#define FBE_SAS_ANCHO_ENCLOSURE_NUMBER_OF_ONE_DIGIT_DISPLAYS       1  // 1 display character on local LCC.
#define FBE_SAS_ANCHO_ENCLOSURE_NUMBER_OF_IMAGES_PER_LCC           5        
#define FBE_SAS_ANCHO_ENCLOSURE_NUMBER_OF_LCCS                     2  // one local lcc and one peer lcc.
#define FBE_SAS_ANCHO_ENCLOSURE_NUMBER_OF_POWER_SUPPLY_IMAGE       2   
#define FBE_SAS_ANCHO_ENCLOSURE_NUMBER_OF_SPS                      0
#define FBE_SAS_ANCHO_ENCLOSURE_NUMBER_OF_DRIVE_MIDPLANE           0

/* 
 * Each side has two entire connectors and each entire connector has multiple individual lanes. 
 * The following number of connectors is the total number of the entire connectors and individual lanes.
 * The connectors on A side are stored first in EDAL, followed by B side.
 */
#define FBE_SAS_ANCHO_ENCLOSURE_NUMBER_OF_CONNECTORS                    20
#define FBE_SAS_ANCHO_ENCLOSURE_NUMBER_OF_CONNECTORS_PER_LCC            10 
#define FBE_SAS_ANCHO_ENCLOSURE_NUMBER_OF_LANES_PER_ENTIRE_CONNECTOR    4

#define FBE_SAS_ANCHO_ENCLOSURE_FIRST_SLOT_INDEX                0
#define FBE_SAS_ANCHO_ENCLOSURE_NUMBER_OF_EXPANSION_PORTS       2  /* 1 incoming, 1 outgoing */
#define FBE_SAS_ANCHO_ENCLOSURE_FIRST_EXPANSION_PORT_INDEX      15 /* 0-14 are for drives*/
#define FBE_SAS_ANCHO_ENCLOSURE_MAX_NUMBER_OF_DRIVES_SPINNINGUP 4

/*
 * Firmware Update Related Info
 */
/* since ancho not supporting SPS */
#define FBE_SAS_ANCHO_ENCLOSURE_FW_DL_RETRY_MAX                 5

// Constants used to size the data structure that stores
// the firmware revision information.
#define FBE_ANCHO_FW_NUM_EXPANDER_ELEMENTS                      1   // expanders per lcc
#define FBE_ANCHO_FW_NUM_SUBENCL_SIDES                          1   // number of lcc to size array
#define FBE_ANCHO_FW_LCC_VERSION_DESCS                          5   // version descriptors in the lcc subencl
#define FBE_ANCHO_FW_PS_VERSION_DESCS                           1   // version descriptors in the ps subencl
#define FBE_SAS_ANCHO_ENCLOSURE_DEFAULT_RIDE_THROUGH_PERIOD     12 /* 12 seconds */

/* These are the lifecycle condition ids for a sas ancho enclosure object. */
typedef enum fbe_sas_ancho_enclosure_lifecycle_cond_id_e {
    FBE_SAS_ANCHO_LIFECYCLE_COND_FOO = FBE_LIFECYCLE_COND_FIRST_ID(FBE_CLASS_ID_SAS_ANCHO_ENCLOSURE),
    FBE_SAS_ANCHO_ENCLOSURE_LIFECYCLE_COND_LAST 
     /* must be last */
} fbe_sas_ancho_enclosure_lifecycle_cond_id_t;

extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(sas_ancho_enclosure);

typedef struct fbe_sas_ancho_enclosure_s
{
    fbe_eses_enclosure_t                            eses_enclosure;

    FBE_LIFECYCLE_DEF_INST_DATA; 
    FBE_LIFECYCLE_DEF_INST_COND(FBE_LIFECYCLE_COND_MAX(FBE_SAS_ANCHO_ENCLOSURE_LIFECYCLE_COND_LAST));
}fbe_sas_ancho_enclosure_t;

/* Methods */
/* Class methods forward declaration */
fbe_status_t fbe_sas_ancho_enclosure_load(void);
fbe_status_t fbe_sas_ancho_enclosure_unload(void);
fbe_status_t fbe_sas_ancho_enclosure_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle);
fbe_status_t fbe_sas_ancho_enclosure_destroy_object( fbe_object_handle_t object_handle);
fbe_status_t fbe_sas_ancho_enclosure_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_sas_ancho_enclosure_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context);
fbe_status_t fbe_sas_ancho_enclosure_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_sas_ancho_enclosure_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);

fbe_status_t fbe_sas_ancho_enclosure_monitor(fbe_sas_ancho_enclosure_t * sas_ancho_enclosure, fbe_packet_t * packet);
fbe_status_t fbe_sas_ancho_enclosure_monitor_load_verify(void);

fbe_status_t fbe_sas_ancho_enclosure_create_edge(fbe_sas_ancho_enclosure_t * sas_ancho_enclosure, fbe_packet_t * packet);
fbe_status_t fbe_sas_ancho_enclosure_get_parent_address(fbe_sas_ancho_enclosure_t * sas_ancho_enclosure, fbe_packet_t * packet);

fbe_status_t fbe_sas_ancho_enclosure_init(fbe_sas_ancho_enclosure_t * sas_ancho_enclosure);

/* Executer functions */

/* fbe_sas_ancho_enclosure_read.c */

/* fbe_sas_ancho_enclosure_main.c*/
fbe_status_t fbe_sas_ancho_format_encl_data(fbe_enclosure_component_block_t   *sas_ancho_component_block);
void fbe_sas_ancho_enclosure_init_eses_parameters(fbe_sas_ancho_enclosure_t *sas_ancho_enclosure);

/* fbe_sas_ancho_enclosure_build_pages.c */
#endif /*SAS_ANCHO_ENCLOSURE_PRIVATE_H */
