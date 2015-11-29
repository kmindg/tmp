#ifndef EDAL_SAS_ENCLOSURE_DATA_H
#define EDAL_SAS_ENCLOSURE_DATA_H

/***************************************************************************
 * Copyright (C) EMC Corporation  2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 * Unpublished - all rights reserved under the copyright laws
 * of the United States
 ***************************************************************************/

/***************************************************************************
 *  edal_sas_enclosure_data.h
 ***************************************************************************
 *
 * DESCRIPTION:
 *      This module is a header file for Enclosure Data Access Library
 *      and it defined data that is in a SAS Enclosure object.
 *
 *  FUNCTIONS:
 *
 *  LOCAL FUNCTIONS:
 *
 *  NOTES:
 *
 *  HISTORY:
 *      16-Sep-08   Joe Perry - Created
 *
 *
 **************************************************************************/

//#include "fbe/fbe_types.h"
#include "edal_base_enclosure_data.h"

// Pack the structures to prevent padding.
#pragma pack(push, fbe_sas_encl, 1) 

/*
 * SAS Enclosure data
 */
typedef struct fbe_sas_enclosure_info_s
{
    fbe_base_enclosure_info_t   baseEnclosureInfo;

    fbe_sas_address_t           smpSasAddress;
    fbe_sas_address_t           sesSasAddress;
    fbe_sas_address_t           serverSasAddress;
    fbe_u32_t                   uniqueId;

} fbe_sas_enclosure_info_t;

/*
 * SAS Expander Data
 */
typedef struct fbe_sas_expander_info_s
{
    fbe_encl_component_general_info_t    generalExpanderInfo;
} fbe_sas_expander_info_t;

/*
 * SAS Expander PHY Data
 */
typedef struct fbe_sas_expander_phy_info_s
{
    fbe_encl_component_general_info_t    generalExpanderPhyInfo;
} fbe_sas_expander_phy_info_t;

#pragma pack(pop, fbe_sas_encl) /* Go back to default alignment.*/


/*
 * SAS Enclosure function prototypes
 */
// Get function prototypes
fbe_edal_status_t 
sas_enclosure_access_getBool(fbe_edal_general_comp_handle_t generalDataPtr,
                             fbe_base_enclosure_attributes_t attribute,
                             fbe_enclosure_component_types_t component,
                             fbe_bool_t *returnDataPtr);
fbe_edal_status_t 
sas_enclosure_access_getU8(fbe_edal_general_comp_handle_t generalDataPtr,
                           fbe_base_enclosure_attributes_t attribute,
                           fbe_enclosure_component_types_t component,
                           fbe_u8_t *returnDataPtr);
fbe_edal_status_t 
sas_enclosure_access_getU16(fbe_edal_general_comp_handle_t generalDataPtr,
                            fbe_base_enclosure_attributes_t attribute,
                            fbe_enclosure_component_types_t component,
                            fbe_u16_t *returnDataPtr);
fbe_edal_status_t 
sas_enclosure_access_getU64(fbe_edal_general_comp_handle_t generalDataPtr,
                            fbe_base_enclosure_attributes_t attribute,
                            fbe_enclosure_component_types_t component,
                            fbe_u64_t *returnDataPtr);
fbe_edal_status_t 
sas_enclosure_access_getU32(fbe_edal_general_comp_handle_t generalDataPtr,
                            fbe_base_enclosure_attributes_t attribute,
                            fbe_enclosure_component_types_t component,
                            fbe_u32_t *returnDataPtr);
fbe_edal_status_t 
sas_enclosure_access_getEnclosureU64(fbe_edal_general_comp_handle_t generalDataPtr,
                                     fbe_base_enclosure_attributes_t attribute,
                                     fbe_u64_t *returnDataPtr);
fbe_edal_status_t 
sas_enclosure_access_getEnclosureU32(fbe_edal_general_comp_handle_t generalDataPtr,
                                     fbe_base_enclosure_attributes_t attribute,
                                     fbe_u32_t *returnDataPtr);

// Set function prototypes
fbe_edal_status_t 
sas_enclosure_access_setBool(fbe_edal_general_comp_handle_t generalDataPtr,
                             fbe_base_enclosure_attributes_t attribute,
                             fbe_enclosure_component_types_t component,
                             fbe_bool_t newValue);
fbe_edal_status_t 
sas_enclosure_access_setU8(fbe_edal_general_comp_handle_t generalDataPtr,
                           fbe_base_enclosure_attributes_t attribute,
                           fbe_enclosure_component_types_t component,
                           fbe_u8_t newValue);
fbe_edal_status_t 
sas_enclosure_access_setU16(fbe_edal_general_comp_handle_t generalDataPtr,
                            fbe_base_enclosure_attributes_t attribute,
                            fbe_enclosure_component_types_t component,
                            fbe_u16_t newValue);
fbe_edal_status_t 
sas_enclosure_access_setEnclosureU64(fbe_edal_general_comp_handle_t generalDataPtr,
                                     fbe_base_enclosure_attributes_t attribute,
                                     fbe_u64_t newValue);
fbe_edal_status_t 
sas_enclosure_access_setEnclosureU32(fbe_edal_general_comp_handle_t generalDataPtr,
                                     fbe_base_enclosure_attributes_t attribute,
                                     fbe_u32_t newValue);
fbe_edal_status_t 
sas_enclosure_access_setU64(fbe_edal_general_comp_handle_t generalDataPtr,
                            fbe_base_enclosure_attributes_t attribute,
                            fbe_enclosure_component_types_t component,
                            fbe_u64_t newValue);
fbe_edal_status_t 
sas_enclosure_access_setU32(fbe_edal_general_comp_handle_t generalDataPtr,
                            fbe_base_enclosure_attributes_t attribute,
                            fbe_enclosure_component_types_t component,
                            fbe_u32_t newValue);

#endif  // ifndef EDAL_SAS_ENCLOSURE_DATA_H
