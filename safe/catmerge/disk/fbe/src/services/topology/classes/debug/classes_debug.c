#include "fbe_interface.h"

#include "base_discovered_private.h"
#include "base_discovering_private.h"
#include "base_board_private.h"
#include "fleet_board_private.h"
#include "magnum_board_private.h"
#include "armada_board_private.h"
#include "base_port_private.h"
#include "sas_port_private.h"
#include "sas_pmc_port_private.h"
#include "base_enclosure_private.h"
#include "sas_enclosure_private.h"
#include "fbe_eses_enclosure_private.h"
#include "sas_viper_enclosure_private.h"
#include "base_physical_drive_private.h"
#include "sas_physical_drive_private.h"
#include "sata_physical_drive_private.h"
#include "sas_ancho_enclosure_private.h"
#include "fbe_logical_drive_private.h"

#include "fbe/fbe_class.h"
#include "fbe_classes_debug.h"

fbe_status_t
fbe_class_debug_get_lifecycle_const_data(char * p_module_name, fbe_class_id_t class_id, fbe_dbgext_ptr * pp_class_const)
{
#   define EXPR_SIZE 128
    fbe_dbgext_ptr p_class_const;
    const char * p_class_const_label;

    *pp_class_const = NULL;
    switch (class_id) {
        case FBE_CLASS_ID_BASE_OBJECT:
            p_class_const_label = "fbe_base_object_lifecycle_const";
            break;
        case FBE_CLASS_ID_BASE_DISCOVERED:
            p_class_const_label = "fbe_base_discovered_lifecycle_const";
            break;
        case FBE_CLASS_ID_BASE_DISCOVERING:
            p_class_const_label = "fbe_base_discovering_lifecycle_const";
            break;
        case FBE_CLASS_ID_BASE_BOARD:
            p_class_const_label = "fbe_base_board_lifecycle_const";
            break;
        case FBE_CLASS_ID_FLEET_BOARD:
            p_class_const_label = "fbe_fleet_board_lifecycle_const";
            break;
        case FBE_CLASS_ID_MAGNUM_BOARD:
            p_class_const_label = "fbe_magnum_board_lifecycle_const";
            break;
        case FBE_CLASS_ID_ARMADA_BOARD:
            p_class_const_label = "fbe_armada_board_lifecycle_const";
            break;
        case FBE_CLASS_ID_BASE_PORT:
            p_class_const_label = "fbe_base_port_lifecycle_const";
            break;
        case FBE_CLASS_ID_SAS_PORT:
            p_class_const_label = "fbe_sas_port_lifecycle_const";
            break;
        case FBE_CLASS_ID_SAS_PMC_PORT:
            p_class_const_label = "fbe_sas_pmc_port_lifecycle_const";
            break;
        case FBE_CLASS_ID_BASE_PHYSICAL_DRIVE:
            p_class_const_label = "fbe_base_physical_drive_lifecycle_const";
            break;
        case FBE_CLASS_ID_SAS_PHYSICAL_DRIVE:
            p_class_const_label = "fbe_sas_physical_drive_lifecycle_const";
            break;
        case FBE_CLASS_ID_SAS_FLASH_DRIVE:
            p_class_const_label = "fbe_sas_flash_drive_lifecycle_const";
            break;
        case FBE_CLASS_ID_SATA_PADDLECARD_DRIVE:
            p_class_const_label = "fbe_sata_paddlecard_drive_lifecycle_const";
            break;
        case FBE_CLASS_ID_SATA_PHYSICAL_DRIVE:
            p_class_const_label = "fbe_sata_physical_drive_lifecycle_const";
            break;
        case FBE_CLASS_ID_SATA_FLASH_DRIVE:
            p_class_const_label = "fbe_sata_flash_drive_lifecycle_const";
            break;
        case FBE_CLASS_ID_BASE_ENCLOSURE:
            p_class_const_label = "fbe_base_enclosure_lifecycle_const";
            break;
        case FBE_CLASS_ID_SAS_ENCLOSURE:
            p_class_const_label = "fbe_sas_enclosure_lifecycle_const";
            break;          
        case FBE_CLASS_ID_ESES_ENCLOSURE:
            p_class_const_label = "fbe_eses_enclosure_lifecycle_const";
            break;
        case FBE_CLASS_ID_SAS_VIPER_ENCLOSURE:
            p_class_const_label = "fbe_sas_viper_enclosure_lifecycle_const";
            break;
        case FBE_CLASS_ID_SAS_MAGNUM_ENCLOSURE:
            p_class_const_label = "fbe_sas_magnum_enclosure_lifecycle_const";
            break;
        case FBE_CLASS_ID_SAS_CITADEL_ENCLOSURE:
            p_class_const_label = "fbe_sas_citadel_enclosure_lifecycle_const";
            break;
        case FBE_CLASS_ID_SAS_BUNKER_ENCLOSURE:
            p_class_const_label = "fbe_sas_bunker_enclosure_lifecycle_const";
            break;  
        case FBE_CLASS_ID_SAS_DERRINGER_ENCLOSURE:
            p_class_const_label = "fbe_sas_derringer_enclosure_lifecycle_const";
            break;
        case FBE_CLASS_ID_SAS_TABASCO_ENCLOSURE:
            p_class_const_label = "fbe_sas_tabasco_enclosure_lifecycle_const";
            break;
        case FBE_CLASS_ID_SAS_VOYAGER_ICM_ENCLOSURE:
            p_class_const_label = "fbe_sas_voyager_icm_enclosure_lifecycle_const";
            break;
        case FBE_CLASS_ID_SAS_VOYAGER_EE_LCC:
            p_class_const_label = "fbe_sas_voyager_ee_enclosure_lifecycle_const";
            break;
        case FBE_CLASS_ID_SAS_VIKING_IOSXP_ENCLOSURE:
            p_class_const_label = "fbe_sas_viking_iosxp_enclosure_lifecycle_const";
            break;
        case FBE_CLASS_ID_SAS_VIKING_DRVSXP_LCC:
            p_class_const_label = "fbe_sas_viking_drvsxp_enclosure_lifecycle_const";
            break;
        case FBE_CLASS_ID_SAS_CAYENNE_IOSXP_ENCLOSURE:
            p_class_const_label = "fbe_sas_cayenne_iosxp_enclosure_lifecycle_const";
            break;
        case FBE_CLASS_ID_SAS_CAYENNE_DRVSXP_LCC:
            p_class_const_label = "fbe_sas_cayenne_drvsxp_enclosure_lifecycle_const";
            break;
        case FBE_CLASS_ID_SAS_NAGA_IOSXP_ENCLOSURE:
            p_class_const_label = "fbe_sas_naga_iosxp_enclosure_lifecycle_const";
            break;
        case FBE_CLASS_ID_SAS_NAGA_DRVSXP_LCC:
            p_class_const_label = "fbe_sas_naga_drvsxp_enclosure_lifecycle_const";
            break;
        case FBE_CLASS_ID_SAS_FALLBACK_ENCLOSURE:
            p_class_const_label = "fbe_sas_fallback_enclosure_lifecycle_const";
            break;
        case FBE_CLASS_ID_SAS_BOXWOOD_ENCLOSURE:
            p_class_const_label = "fbe_sas_boxwood_enclosure_lifecycle_const";
            break;
        case FBE_CLASS_ID_SAS_KNOT_ENCLOSURE:
            p_class_const_label = "fbe_sas_knot_enclosure_lifecycle_const";
            break;		
        case FBE_CLASS_ID_SAS_ANCHO_ENCLOSURE:
            p_class_const_label = "fbe_sas_ancho_enclosure_lifecycle_const";
            break;
        case FBE_CLASS_ID_SAS_RHEA_ENCLOSURE:
            p_class_const_label = "fbe_sas_rhea_enclosure_lifecycle_const";
            break;
        case FBE_CLASS_ID_SAS_MIRANDA_ENCLOSURE:
            p_class_const_label = "fbe_sas_miranda_enclosure_lifecycle_const";
            break;
        case FBE_CLASS_ID_SAS_CALYPSO_ENCLOSURE:
            p_class_const_label = "fbe_sas_calypso_enclosure_lifecycle_const";
            break;


			/* Logical package classes */
        case FBE_CLASS_ID_BASE_CONFIG:
            p_class_const_label = "fbe_base_config_lifecycle_const";
            break;

        case FBE_CLASS_ID_RAID_GROUP:
            p_class_const_label = "fbe_raid_group_lifecycle_const";
            break;

        case FBE_CLASS_ID_STRIPER:
            p_class_const_label = "fbe_striper_lifecycle_const";
            break;

        case FBE_CLASS_ID_PARITY:
            p_class_const_label = "fbe_parity_lifecycle_const";
            break;

        case FBE_CLASS_ID_MIRROR:
            p_class_const_label = "fbe_mirror_lifecycle_const";
            break;

        case FBE_CLASS_ID_VIRTUAL_DRIVE:
            p_class_const_label = "fbe_virtual_drive_lifecycle_const";
            break;

        case FBE_CLASS_ID_PROVISION_DRIVE:
            p_class_const_label = "fbe_provision_drive_lifecycle_const";
            break;

        case FBE_CLASS_ID_LUN:
            p_class_const_label = "fbe_lun_lifecycle_const";
            break;

        case FBE_CLASS_ID_EXTENT_POOL:
            p_class_const_label = "fbe_extent_pool_lifecycle_const";
            break;

        case FBE_CLASS_ID_EXTENT_POOL_LUN:
            p_class_const_label = "fbe_ext_pool_lun_lifecycle_const";
            break;

        case FBE_CLASS_ID_EXTENT_POOL_METADATA_LUN:
            p_class_const_label = "fbe_ext_pool_lun_lifecycle_const";
            break;

            /* ESP classes */
        case FBE_CLASS_ID_BASE_ENVIRONMENT:
            p_class_const_label = "fbe_base_environment_lifecycle_const";
            break;

        case FBE_CLASS_ID_PS_MGMT:
            p_class_const_label = "fbe_ps_mgmt_lifecycle_const";
            break;

        case FBE_CLASS_ID_SPS_MGMT:
            p_class_const_label = "fbe_sps_mgmt_lifecycle_const";
            break;

        case FBE_CLASS_ID_ENCL_MGMT:
            p_class_const_label = "fbe_encl_mgmt_lifecycle_const";
            break;

        case FBE_CLASS_ID_DRIVE_MGMT:
            p_class_const_label = "fbe_drive_mgmt_lifecycle_const";
            break;

        case FBE_CLASS_ID_BOARD_MGMT:
            p_class_const_label = "fbe_board_mgmt_lifecycle_const";
            break;

        case FBE_CLASS_ID_COOLING_MGMT:
            p_class_const_label = "fbe_cooling_mgmt_lifecycle_const";
            break;

        case FBE_CLASS_ID_MODULE_MGMT:
            p_class_const_label = "fbe_module_mgmt_lifecycle_const";
            break;

        default:
            return FBE_STATUS_GENERIC_FAILURE;
    }
    p_class_const = NULL;
    FBE_GET_STRING_EXPRESSION(p_module_name, p_class_const_label, &p_class_const);
    if (p_class_const == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *pp_class_const = p_class_const;
    return FBE_STATUS_OK;
#   undef EXPR_SIZE
}

fbe_status_t
fbe_class_debug_get_lifecycle_inst_cond(char * p_module_name,
                                        fbe_dbgext_ptr p_object,
                                        fbe_class_id_t class_id,
                                        fbe_dbgext_ptr * pp_inst_cond)
{
#   define EXPR_SIZE 128
    const char * p_class_type_label;
    fbe_u32_t offset;

    *pp_inst_cond = 0;
    p_class_type_label = NULL;
    switch (class_id) {
        case FBE_CLASS_ID_BASE_OBJECT:
            p_class_type_label = "fbe_base_object_t";
            break;
        case FBE_CLASS_ID_BASE_DISCOVERED:
            p_class_type_label = "fbe_base_discovered_t";
            break;
        case FBE_CLASS_ID_BASE_DISCOVERING:
            p_class_type_label = "fbe_base_discovering_t";
            break;
        case FBE_CLASS_ID_BASE_BOARD:
            p_class_type_label = "fbe_base_board_t";
            break;
        case FBE_CLASS_ID_FLEET_BOARD:
            p_class_type_label = "fbe_fleet_board_t";
            break;
        case FBE_CLASS_ID_MAGNUM_BOARD:
            p_class_type_label = "fbe_magnum_board_t";
            break;
        case FBE_CLASS_ID_ARMADA_BOARD:
            p_class_type_label = "fbe_armada_board_t";
            break;
        case FBE_CLASS_ID_BASE_PORT:
            p_class_type_label = "fbe_base_port_t";
            break;
        case FBE_CLASS_ID_SAS_PORT:
            p_class_type_label = "fbe_sas_port_t";
            break;
        case FBE_CLASS_ID_SAS_PMC_PORT:
            p_class_type_label = "fbe_sas_pmc_port_t";
            break;
        case FBE_CLASS_ID_BASE_PHYSICAL_DRIVE:
            p_class_type_label = "fbe_base_physical_drive_t";
            break;
        case FBE_CLASS_ID_SAS_PHYSICAL_DRIVE:
            p_class_type_label = "fbe_sas_physical_drive_t";
            break;
        case FBE_CLASS_ID_SAS_FLASH_DRIVE:
            p_class_type_label = "fbe_sas_flash_drive_t";
            break;
        case FBE_CLASS_ID_SATA_PADDLECARD_DRIVE:
            p_class_type_label = "fbe_sata_paddlecard_drive_t";
            break;
        case FBE_CLASS_ID_SATA_PHYSICAL_DRIVE:
            p_class_type_label = "fbe_sata_physical_drive_t";
            break;
        case FBE_CLASS_ID_SATA_FLASH_DRIVE:
            p_class_type_label = "fbe_sata_flash_drive_t";
            break;
        case FBE_CLASS_ID_BASE_ENCLOSURE:
            p_class_type_label = "fbe_base_enclosure_t";
            break;
        case FBE_CLASS_ID_SAS_ENCLOSURE:
            p_class_type_label = "fbe_sas_enclosure_t";
            break;          
        case FBE_CLASS_ID_ESES_ENCLOSURE:
            p_class_type_label = "fbe_eses_enclosure_t";
            break;
        case FBE_CLASS_ID_SAS_VIPER_ENCLOSURE:
            p_class_type_label = "fbe_sas_viper_enclosure_t";
            break;
        case FBE_CLASS_ID_SAS_MAGNUM_ENCLOSURE:
            p_class_type_label = "fbe_sas_magnum_enclosure_t";
            break;
        case FBE_CLASS_ID_SAS_CITADEL_ENCLOSURE:
            p_class_type_label = "fbe_sas_citadel_enclosure_t";
            break;
        case FBE_CLASS_ID_SAS_BUNKER_ENCLOSURE:
            p_class_type_label = "fbe_sas_bunker_enclosure_t";
            break;
        case FBE_CLASS_ID_SAS_DERRINGER_ENCLOSURE:
            p_class_type_label = "fbe_sas_derringer_enclosure_t";
            break;  
        case FBE_CLASS_ID_SAS_TABASCO_ENCLOSURE:
            p_class_type_label = "fbe_sas_tabasco_enclosure_t";
            break;
        case FBE_CLASS_ID_SAS_FALLBACK_ENCLOSURE:
            p_class_type_label = "fbe_sas_fallback_enclosure_t";
            break;
        case FBE_CLASS_ID_SAS_BOXWOOD_ENCLOSURE:
            p_class_type_label = "fbe_sas_boxwood_enclosure_t";
            break;
        case FBE_CLASS_ID_SAS_KNOT_ENCLOSURE:
            p_class_type_label = "fbe_sas_knot_enclosure_t";
            break;		
        case FBE_CLASS_ID_SAS_ANCHO_ENCLOSURE:
            p_class_type_label = "fbe_sas_ancho_enclosure_t";
            break;
        case FBE_CLASS_ID_SAS_PINECONE_ENCLOSURE:
            p_class_type_label = "fbe_sas_pinecone_enclosure_t";
            break;
        case FBE_CLASS_ID_SAS_STEELJAW_ENCLOSURE:
            p_class_type_label = "fbe_sas_steeljaw_enclosure_t";
            break;
        case FBE_CLASS_ID_SAS_RAMHORN_ENCLOSURE:
            p_class_type_label = "fbe_sas_ramhorn_enclosure_t";
            break;
        case FBE_CLASS_ID_SAS_RHEA_ENCLOSURE:
            p_class_type_label = "fbe_sas_rhea_enclosure_t";
            break;
        case FBE_CLASS_ID_SAS_MIRANDA_ENCLOSURE:
            p_class_type_label = "fbe_sas_miranda_enclosure_t";
            break;
        case FBE_CLASS_ID_SAS_CALYPSO_ENCLOSURE:
            p_class_type_label = "fbe_sas_calypso_enclosure_t";
            break;
        case FBE_CLASS_ID_LOGICAL_DRIVE:
            p_class_type_label = "fbe_logical_drive_t";
            break;

        case FBE_CLASS_ID_BASE_CONFIG:
            p_class_type_label = "fbe_base_config_t";
            break;
        case FBE_CLASS_ID_RAID_GROUP:
            p_class_type_label = "fbe_raid_group_t";
            break;
        case FBE_CLASS_ID_STRIPER:
            p_class_type_label = "fbe_striper_t";
            break;
        case FBE_CLASS_ID_PARITY:
            p_class_type_label = "fbe_parity_t";
            break;
        case FBE_CLASS_ID_MIRROR:
            p_class_type_label = "fbe_mirror_t";
            break;
        case FBE_CLASS_ID_VIRTUAL_DRIVE:
            p_class_type_label = "fbe_virtual_drive_t";
            break;
        case FBE_CLASS_ID_PROVISION_DRIVE:
            p_class_type_label = "fbe_provision_drive_t";
            break;
        case FBE_CLASS_ID_LUN:
            p_class_type_label = "fbe_lun_t";
            break;
        case FBE_CLASS_ID_EXTENT_POOL:
            p_class_type_label = "fbe_extent_pool_t";
            break;
        case FBE_CLASS_ID_EXTENT_POOL_LUN:
            p_class_type_label = "fbe_ext_pool_lun_t";
            break;
        case FBE_CLASS_ID_EXTENT_POOL_METADATA_LUN:
            p_class_type_label = "fbe_ext_pool_metadata_lun_t";
            break;

        /* ESP Class id Starts. */
        case FBE_CLASS_ID_BASE_ENVIRONMENT:
            p_class_type_label = "fbe_base_environment_t";
            break;
        case FBE_CLASS_ID_ENCL_MGMT:
            p_class_type_label = "fbe_encl_mgmt_t";
            break;
        case FBE_CLASS_ID_SPS_MGMT:
            p_class_type_label = "fbe_sps_mgmt_t";
            break;
        case FBE_CLASS_ID_DRIVE_MGMT:
            p_class_type_label = "fbe_drive_mgmt_t";
            break;
        case FBE_CLASS_ID_MODULE_MGMT:
            p_class_type_label = "fbe_module_mgmt_t";
            break;
        case FBE_CLASS_ID_PS_MGMT:
            p_class_type_label = "fbe_ps_mgmt_t";
            break;
        case FBE_CLASS_ID_BOARD_MGMT:
            p_class_type_label = "fbe_board_mgmt_t";
            break;
        case FBE_CLASS_ID_COOLING_MGMT:
            p_class_type_label = "fbe_cooling_mgmt_t";
            break;
        /* ESP Class id Ends. */
        default:
            return FBE_STATUS_GENERIC_FAILURE;
    }
    offset = 0;
    FBE_GET_FIELD_STRING_OFFSET(p_module_name, p_class_type_label, "lifecycle_cond", &offset)
    if (offset == 0) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *pp_inst_cond = p_object + offset;
    return FBE_STATUS_OK;
#   undef EXPR_SIZE
}
fbe_status_t
fbe_get_lifecycle_state_name(fbe_lifecycle_state_t state, const char ** pp_state_name)
{
    *pp_state_name = NULL;
    switch (state) {
        case FBE_LIFECYCLE_STATE_SPECIALIZE:
            *pp_state_name = "SPECIALIZE";
            break;
        case FBE_LIFECYCLE_STATE_ACTIVATE:
            *pp_state_name = "ACTIVATE";
            break;
        case FBE_LIFECYCLE_STATE_READY:
            *pp_state_name = "READY";
            break;
        case FBE_LIFECYCLE_STATE_HIBERNATE:
            *pp_state_name = "HIBERNATE";
            break;
        case FBE_LIFECYCLE_STATE_OFFLINE:
            *pp_state_name = "OFFLINE";
            break;
        case FBE_LIFECYCLE_STATE_FAIL:
            *pp_state_name = "FAIL";
            break;
        case FBE_LIFECYCLE_STATE_DESTROY:
            *pp_state_name = "DESTROY";
            break;
        case FBE_LIFECYCLE_STATE_PENDING_READY:
            *pp_state_name = "PENDING_READY";
            break;
        case FBE_LIFECYCLE_STATE_PENDING_ACTIVATE:
            *pp_state_name = "PENDING_ACTIVATE";
            break;
        case FBE_LIFECYCLE_STATE_PENDING_HIBERNATE:
            *pp_state_name = "PENDING_HIBERNATE";
            break;
        case FBE_LIFECYCLE_STATE_PENDING_OFFLINE:
            *pp_state_name = "PENDING_OFFLINE";
            break;
        case FBE_LIFECYCLE_STATE_PENDING_FAIL:
            *pp_state_name = "PENDING_FAIL";
            break;
        case FBE_LIFECYCLE_STATE_PENDING_DESTROY:
            *pp_state_name = "PENDING_DESTROY";
            break;
        case FBE_LIFECYCLE_STATE_NOT_EXIST:
            *pp_state_name = "STATE_NOT_EXIST";
            break;
        case FBE_LIFECYCLE_STATE_INVALID:
            *pp_state_name = "INVALID";
            break;
        default:
            return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}


fbe_status_t
fbe_get_class_name(fbe_class_id_t class_id, const char ** pp_class_name)
{
    *pp_class_name = NULL;
    switch (class_id) {
        case FBE_CLASS_ID_BASE_OBJECT:
            *pp_class_name = "BASE_OBJECT";
            break;
        case FBE_CLASS_ID_BASE_DISCOVERED:
            *pp_class_name = "BASE_DISCOVERED";
            break;
        case FBE_CLASS_ID_BASE_DISCOVERING:
            *pp_class_name = "BASE_DISCOVERING";
            break;
        case FBE_CLASS_ID_BASE_BOARD:
            *pp_class_name = "BASE_BOARD";
            break;
        case FBE_CLASS_ID_FLEET_BOARD:
            *pp_class_name = "FLEET_BOARD";
            break;
        case FBE_CLASS_ID_MAGNUM_BOARD:
            *pp_class_name = "MAGNUM_BOARD";
            break;
        case FBE_CLASS_ID_ARMADA_BOARD:
            *pp_class_name = "ARMADA_BOARD";
            break;
        case FBE_CLASS_ID_BASE_PORT:
            *pp_class_name = "BASE_PORT";
            break;
        case FBE_CLASS_ID_SAS_PORT:
            *pp_class_name = "SAS_PORT";
            break;
        case FBE_CLASS_ID_SAS_PMC_PORT:
            *pp_class_name = "SAS_PMC_PORT";
            break;
        case FBE_CLASS_ID_BASE_PHYSICAL_DRIVE:
            *pp_class_name = "BASE_PHYSICAL_DRIVE";
            break;
        case FBE_CLASS_ID_SAS_PHYSICAL_DRIVE:
            *pp_class_name = "SAS_PHYSICAL_DRIVE";
            break;
        case FBE_CLASS_ID_SAS_FLASH_DRIVE:
            *pp_class_name = "SAS_FLASH_DRIVE";
            break;
        case FBE_CLASS_ID_SATA_PADDLECARD_DRIVE:
            *pp_class_name = "SATA_PADDLECARD_DRIVE";
            break;
        case FBE_CLASS_ID_SATA_PHYSICAL_DRIVE:
            *pp_class_name = "SATA_PHYSICAL_DRIVE";
            break;
        case FBE_CLASS_ID_SATA_FLASH_DRIVE:
            *pp_class_name = "SATA_FLASH_DRIVE";
            break;
	case FBE_CLASS_ID_SAS_VOYAGER_EE_LCC:
            *pp_class_name = "SAS_VOYAGER_EE_LCC";
            break;	 
        case FBE_CLASS_ID_SAS_VIKING_DRVSXP_LCC:
            *pp_class_name = "SAS_VIKING_DRVSXP_LCC";
            break;	  		
        case FBE_CLASS_ID_SAS_CAYENNE_DRVSXP_LCC:
            *pp_class_name = "SAS_CAYENNE_DRVSXP_LCC";
            break;	  		
            break; 	
        case FBE_CLASS_ID_SAS_NAGA_DRVSXP_LCC:
            *pp_class_name = "SAS_NAGA_DRVSXP_LCC";
            break; 		
        case FBE_CLASS_ID_BASE_ENCLOSURE:
            *pp_class_name = "BASE_ENCLOSURE";
            break;
        case FBE_CLASS_ID_SAS_ENCLOSURE:
            *pp_class_name = "SAS_ENCLOSURE";
            break;          
        case FBE_CLASS_ID_ESES_ENCLOSURE:
            *pp_class_name = "ESES_ENCLOSURE";
            break;
        case FBE_CLASS_ID_SAS_VIPER_ENCLOSURE:
            *pp_class_name = "SAS_VIPER_ENCLOSURE";
            break;
        case FBE_CLASS_ID_SAS_MAGNUM_ENCLOSURE:
            *pp_class_name = "SAS_MAGNUM_ENCLOSURE";
            break;
        case FBE_CLASS_ID_SAS_CITADEL_ENCLOSURE:
            *pp_class_name = "SAS_CITADEL_ENCLOSURE";
            break;
        case FBE_CLASS_ID_SAS_BUNKER_ENCLOSURE:
            *pp_class_name = "SAS_BUNKER_ENCLOSURE";
            break;
        case FBE_CLASS_ID_SAS_DERRINGER_ENCLOSURE:
            *pp_class_name = "SAS_DERRINGER_ENCLOSURE";
            break;
        case FBE_CLASS_ID_SAS_TABASCO_ENCLOSURE:
            *pp_class_name = "SAS_TABASCO_ENCLOSURE";
            break;
	case FBE_CLASS_ID_SAS_VOYAGER_ICM_ENCLOSURE:
            *pp_class_name = "SAS_VOYAGER_ICM_ENCLOSURE";
            break;  
        case FBE_CLASS_ID_SAS_VIKING_IOSXP_ENCLOSURE:
            *pp_class_name = "SAS_VIKING_IOSXP_ENCLOSURE";
            break;    
        case FBE_CLASS_ID_SAS_CAYENNE_IOSXP_ENCLOSURE:
            *pp_class_name = "SAS_CAYENNE_IOSXP_ENCLOSURE";
            break;    
            break; 
        case FBE_CLASS_ID_SAS_NAGA_IOSXP_ENCLOSURE:
            *pp_class_name = "SAS_NAGA_IOSXP_ENCLOSURE";
            break; 
        case FBE_CLASS_ID_SAS_FALLBACK_ENCLOSURE:
            *pp_class_name = "SAS_FALLBACK_ENCLOSURE";
            break;
        case FBE_CLASS_ID_SAS_BOXWOOD_ENCLOSURE:
            *pp_class_name = "SAS_BOXWOOD_ENCLOSURE";
            break;
        case FBE_CLASS_ID_SAS_KNOT_ENCLOSURE:
            *pp_class_name = "SAS_KNOT_ENCLOSURE";
            break;		
        case FBE_CLASS_ID_SAS_ANCHO_ENCLOSURE:
            *pp_class_name = "SAS_ANCHO_ENCLOSURE";
            break;
        case FBE_CLASS_ID_SAS_RHEA_ENCLOSURE:
            *pp_class_name = "SAS_RHEA_ENCLOSURE";
            break;
        case FBE_CLASS_ID_SAS_MIRANDA_ENCLOSURE:
            *pp_class_name = "SAS_MIRANDA_ENCLOSURE";
            break;
        case FBE_CLASS_ID_SAS_CALYPSO_ENCLOSURE:
            *pp_class_name = "SAS_CALYPSO_ENCLOSURE";
            break;
        case FBE_CLASS_ID_LOGICAL_DRIVE:
            *pp_class_name = "LOGICAL_DRIVE";
            break;
        case FBE_CLASS_ID_BASE_CONFIG:
            *pp_class_name = "BASE_CONFIG";
            break;
        case FBE_CLASS_ID_RAID_GROUP:
            *pp_class_name = "RAID_GROUP";
            break;
        case FBE_CLASS_ID_PARITY:
            *pp_class_name = "PARITY";
            break;
        case FBE_CLASS_ID_STRIPER:
            *pp_class_name = "STRIPER";
            break;
        case FBE_CLASS_ID_MIRROR:
            *pp_class_name = "MIRROR";
            break;
        case FBE_CLASS_ID_VIRTUAL_DRIVE:
            *pp_class_name = "VIRTUAL_DRIVE";
            break;
        case FBE_CLASS_ID_PROVISION_DRIVE:
            *pp_class_name = "PROVISION_DRIVE";
            break;
        case FBE_CLASS_ID_LUN:
            *pp_class_name = "LUN";
            break;
        case FBE_CLASS_ID_EXTENT_POOL:
            *pp_class_name = "EXTENT_POOL";
            break;
        case FBE_CLASS_ID_EXTENT_POOL_LUN:
            *pp_class_name = "EXTENT_POOL_LUN";
            break;
        case FBE_CLASS_ID_EXTENT_POOL_METADATA_LUN:
            *pp_class_name = "EXTENT_POOL_METADATA_LUN";
            break;

        /* ESP Class id Starts. */
        case FBE_CLASS_ID_BASE_ENVIRONMENT:
            *pp_class_name = "BASE_ENVIRONMENT";
            break;
        case FBE_CLASS_ID_ENCL_MGMT:
            *pp_class_name = "ENCL_MGMT";
            break;
        case FBE_CLASS_ID_SPS_MGMT:
            *pp_class_name = "SPS_MGMT";
            break;
        case FBE_CLASS_ID_DRIVE_MGMT:
            *pp_class_name = "DRIVE_MGMT";
            break;
        case FBE_CLASS_ID_MODULE_MGMT:
            *pp_class_name = "MODULE_MGMT";
            break;
        case FBE_CLASS_ID_PS_MGMT:
            *pp_class_name = "PS_MGMT";
            break;
        case FBE_CLASS_ID_BOARD_MGMT:
            *pp_class_name = "BOARD_MGMT";
            break;
        case FBE_CLASS_ID_COOLING_MGMT:
            *pp_class_name = "COOLING_MGMT";
            break;
        /* ESP Class id Ends. */

        default:
            return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
