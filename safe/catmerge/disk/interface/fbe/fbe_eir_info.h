/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

#ifndef FBE_EIR_INFO_H
#define FBE_EIR_INFO_H

/*!**************************************************************************
 * @file fbe_eir_info.h
 ***************************************************************************
 *
 * @brief
 *  This file EIR (Energy Information Reporting) defines & data structures.
 * 
 * @ingroup 
 * 
 * @revision
 *   06/18/2010:  Created. Joe Perry
 *
 ***************************************************************************/

#include "fbe/fbe_types.h"


// defines for Sample Data
#define FBE_STATS_WINDOW_SIZE            3600        // 1 hour
#define FBE_STATS_SUB_SAMPLE_INTERVAL    3           // 3 seconds
#define FBE_STATS_SAMPLE_INTERVAL        30          // 30 seconds
#define FBE_STATS_SUB_SAMPLE_COUNT       (FBE_STATS_SAMPLE_INTERVAL / FBE_STATS_SUB_SAMPLE_INTERVAL)
#define FBE_STATS_SUB_POLL_INTERVAL      (FBE_STATS_SUB_SAMPLE_INTERVAL * FBE_TIME_MILLISECONDS_PER_SECOND)
#define FBE_STATS_POLL_INTERVAL          (FBE_STATS_SAMPLE_INTERVAL * FBE_TIME_MILLISECONDS_PER_SECOND)
#define FBE_SAMPLE_DATA_SIZE         \
        (FBE_STATS_WINDOW_SIZE / FBE_STATS_SAMPLE_INTERVAL)

// defines for Statistics status
#define FBE_ENERGY_STAT_INVALID              0xFFFFFFFF  // invalid data
#define FBE_ENERGY_STAT_UNINITIALIZED        0x0         // not set yet
#define FBE_ENERGY_STAT_VALID                0x1         // good PS stats
#define FBE_ENERGY_STAT_UNSUPPORTED          0x2         // PS stats is unsupported
#define FBE_ENERGY_STAT_FAILED               0x4         // failed to retrieve PS stats
#define FBE_ENERGY_STAT_NOT_PRESENT          0x8         // PS is not available/present
#define FBE_ENERGY_STAT_SAMPLE_TOO_SMALL     0x10        // PS is not available/present

// JAP - make smaller for now so it fits in one 2K buffer
//#define FBE_EIR_MAX_ENCL_COUNT      80
#define FBE_EIR_MAX_ENCL_COUNT      10

/*
 * Input Power structures
 */
typedef struct fbe_eir_input_power_sample_s
{
    fbe_u32_t		status;
    fbe_u32_t       inputPower;
} fbe_eir_input_power_sample_t;

/*
 * Air Inlet Temperature structures
 */
typedef struct fbe_eir_temp_sample_s
{
    fbe_u32_t       status;
    fbe_u32_t       airInletTemp;
} fbe_eir_temp_sample_t;

/*
 * Array Utilization structures
 */
typedef struct fbe_eir_util_sample_s
{
    fbe_u32_t       status;
    fbe_u32_t       utilization;
} fbe_eir_util_sample_t;

// FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_EIR_INFO  << Usurper command to the eses enclosure object to bring this down to the 
// enclosure management object.

/*!********************************************************************* 
 * @struct fbe_esp_encl_eir_data_t 
 *  
 * @brief 
 *   Contains the enclosure environmental data. This data is a compilation of
 *   a 30-second interval for one enclosure.
 *   
 * @ingroup fbe_api_esp_sps_mgmt_interface
 * @ingroup fbe_esp_sps_mgmt_spsInputPower
 * 
 **********************************************************************/
typedef struct
{
    // calculated values per enclosure, also used to summarize all enclosures.
    fbe_eir_input_power_sample_t   enclCurrentInputPower ; 
    fbe_eir_input_power_sample_t   enclMaxInputPower     ;
    fbe_eir_input_power_sample_t   enclAverageInputPower ;
                                                      
    fbe_eir_temp_sample_t          currentAirInletTemp   ;                       
    fbe_eir_temp_sample_t          maxAirInletTemp       ;                          
    fbe_eir_temp_sample_t          averageAirInletTemp   ;                      

} fbe_esp_encl_eir_data_t;


/*!********************************************************************* 
 * @struct fbe_esp_indiv_sps_eir_data_t 
 *  
 * @brief 
 *   Contains the sps environmental data. This data is a compilation of
 *   a 30-second interval for all sps's.  Also used for collecting individual sps data.
 *   
 * @ingroup fbe_api_esp_sps_mgmt_interface
 * @ingroup fbe_esp_sps_mgmt_spsInputPower
 * 
 **********************************************************************/
typedef struct
{
    // calculated values per SPS, and SPS summarized data.
    fbe_eir_input_power_sample_t   spsCurrentInputPower ; 
    fbe_eir_input_power_sample_t   spsMaxInputPower     ;
    fbe_eir_input_power_sample_t   spsAverageInputPower ;
                                                      
} fbe_esp_indiv_sps_eir_data_t;


/*!********************************************************************* 
 * @struct fbe_esp_indiv_encl_eir_data_t 
 *  
 * @brief 
 *   Contains the Enclosure environmental data.
 *   
 * @ingroup fbe_api_esp_encl_mgmt_interface
 * @ingroup fbe_esp_encl_mgmt_enclInputPower
 * 
 **********************************************************************/
typedef struct
{
    fbe_eir_input_power_sample_t   enclCurrentInputPower; 
    fbe_eir_input_power_sample_t   enclMaxInputPower;
    fbe_eir_input_power_sample_t   enclAverageInputPower;
                                                      
    fbe_eir_temp_sample_t          enclCurrentAirInletTemp;
    fbe_eir_temp_sample_t          enclMaxAirInletTemp;                          
    fbe_eir_temp_sample_t          enclAverageAirInletTemp;                      
                                                          
} fbe_esp_indiv_encl_eir_data_t;


/*!********************************************************************* 
 * @struct fbe_esp_current_encl_eir_data_t 
 *  
 * @brief 
 *   Contains the Enclosure environmental data.
 *   
 * @ingroup fbe_api_esp_encl_mgmt_interface
 * @ingroup fbe_esp_encl_mgmt_enclInputPower
 * 
 **********************************************************************/
typedef struct
{
    fbe_eir_input_power_sample_t   enclCurrentInputPower; 
                                                      
    fbe_eir_temp_sample_t          enclCurrentAirInletTemp;
                                                          
} fbe_esp_current_encl_eir_data_t;

#endif /* FBE_EIR_INFO_H */

/*******************************
 * end fbe_eir_info.h
 *******************************/
