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
#include "sas_magnum_enclosure_private.h"
#include "sas_citadel_enclosure_private.h"
#include "sas_bunker_enclosure_private.h"
#include "sas_derringer_enclosure_private.h"
#include "sas_tabasco_enclosure_private.h"
#include "sas_voyager_icm_enclosure_private.h"
#include "sas_voyager_ee_enclosure_private.h"
#include "sas_viking_iosxp_enclosure_private.h"
#include "sas_viking_drvsxp_enclosure_private.h"
#include "sas_cayenne_iosxp_enclosure_private.h"
#include "sas_cayenne_drvsxp_enclosure_private.h"
#include "sas_naga_iosxp_enclosure_private.h"
#include "sas_naga_drvsxp_enclosure_private.h"
#include "sas_fallback_enclosure_private.h"
#include "sas_boxwood_enclosure_private.h"
#include "sas_knot_enclosure_private.h"
#include "sas_pinecone_enclosure_private.h"
#include "sas_steeljaw_enclosure_private.h"
#include "sas_ramhorn_enclosure_private.h"
#include "sas_ancho_enclosure_private.h"
#include "sas_rhea_enclosure_private.h"
#include "sas_miranda_enclosure_private.h"
#include "sas_calypso_enclosure_private.h"
#include "base_physical_drive_private.h"
#include "sas_physical_drive_private.h"
#include "sas_flash_drive_private.h"
#include "sata_paddlecard_drive_private.h"
#include "sata_physical_drive_private.h"
#include "sata_flash_drive_private.h"
#include "fbe_logical_drive_private.h"
#include "fbe_bvd_interface_private.h"
#include "fbe_lun_private.h"
#include "fbe_striper_private.h"
#include "fbe_parity_private.h"
#include "fbe_virtual_drive_private.h"
#include "fbe_provision_drive_private.h"
#include "fbe_mirror_private.h"
#include "fbe_extent_pool_private.h"
//#include "fbe_ext_pool_lun_private.h"

#include "fbe/fbe_class.h"

extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(ext_pool_lun);

fbe_status_t
fbe_get_class_lifecycle_const_data(fbe_class_id_t class_id, fbe_lifecycle_const_t ** pp_class_const)
{
    *pp_class_const = NULL;
    switch (class_id) {
        case FBE_CLASS_ID_BASE_OBJECT:
            *pp_class_const = (fbe_lifecycle_const_t*)&FBE_LIFECYCLE_CONST_DATA(base_object);
            break;
        case FBE_CLASS_ID_BASE_DISCOVERED:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(base_discovered);
            break;
        case FBE_CLASS_ID_BASE_DISCOVERING:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(base_discovering);
            break;
        case FBE_CLASS_ID_BASE_BOARD:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(base_board);
            break;
        case FBE_CLASS_ID_FLEET_BOARD:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(fleet_board);
            break;
        case FBE_CLASS_ID_MAGNUM_BOARD:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(magnum_board);
            break;
        case FBE_CLASS_ID_ARMADA_BOARD:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(armada_board);
            break;
        case FBE_CLASS_ID_BASE_PORT:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(base_port);
            break;
        case FBE_CLASS_ID_SAS_PORT:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(sas_port);
            break;
        case FBE_CLASS_ID_SAS_PMC_PORT:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(sas_pmc_port);
            break;
        case FBE_CLASS_ID_BASE_PHYSICAL_DRIVE:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(base_physical_drive);
            break;
        case FBE_CLASS_ID_SAS_PHYSICAL_DRIVE:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(sas_physical_drive);
            break;
        case FBE_CLASS_ID_SAS_FLASH_DRIVE:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(sas_flash_drive);
            break;
        case FBE_CLASS_ID_SATA_PADDLECARD_DRIVE:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(sata_paddlecard_drive);
            break;
        case FBE_CLASS_ID_SATA_PHYSICAL_DRIVE:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(sata_physical_drive);
            break;
        case FBE_CLASS_ID_SATA_FLASH_DRIVE:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(sata_flash_drive);
            break;
        case FBE_CLASS_ID_BASE_ENCLOSURE:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(base_enclosure);
            break;
        case FBE_CLASS_ID_SAS_ENCLOSURE:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(sas_enclosure);
            break;          
        case FBE_CLASS_ID_ESES_ENCLOSURE:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(eses_enclosure);
            break;
        case FBE_CLASS_ID_SAS_VIPER_ENCLOSURE:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(sas_viper_enclosure);
            break;
        case FBE_CLASS_ID_SAS_MAGNUM_ENCLOSURE:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(sas_magnum_enclosure);
            break;
        case FBE_CLASS_ID_SAS_CITADEL_ENCLOSURE:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(sas_citadel_enclosure);
            break;
        case FBE_CLASS_ID_SAS_BUNKER_ENCLOSURE:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(sas_bunker_enclosure);
            break;
        case FBE_CLASS_ID_SAS_DERRINGER_ENCLOSURE:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(sas_derringer_enclosure);
            break;
        case FBE_CLASS_ID_SAS_TABASCO_ENCLOSURE:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(sas_tabasco_enclosure);
            break;
        case FBE_CLASS_ID_SAS_VOYAGER_ICM_ENCLOSURE:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(sas_voyager_icm_enclosure);
            break;
        case FBE_CLASS_ID_SAS_VOYAGER_EE_LCC:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(sas_voyager_ee_enclosure);
            break;
        case FBE_CLASS_ID_SAS_VIKING_IOSXP_ENCLOSURE:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(sas_viking_iosxp_enclosure);
            break;
        case FBE_CLASS_ID_SAS_VIKING_DRVSXP_LCC:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(sas_viking_drvsxp_enclosure);
            break;
        case FBE_CLASS_ID_SAS_CAYENNE_IOSXP_ENCLOSURE:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(sas_cayenne_iosxp_enclosure);
            break;
        case FBE_CLASS_ID_SAS_CAYENNE_DRVSXP_LCC:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(sas_cayenne_drvsxp_enclosure);
            break;
        case FBE_CLASS_ID_SAS_NAGA_IOSXP_ENCLOSURE:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(sas_naga_iosxp_enclosure);
            break;
        case FBE_CLASS_ID_SAS_NAGA_DRVSXP_LCC:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(sas_naga_drvsxp_enclosure);
            break;
        case FBE_CLASS_ID_SAS_FALLBACK_ENCLOSURE:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(sas_fallback_enclosure);
            break;
        case FBE_CLASS_ID_SAS_BOXWOOD_ENCLOSURE:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(sas_boxwood_enclosure);
            break;
        case FBE_CLASS_ID_SAS_KNOT_ENCLOSURE:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(sas_knot_enclosure);
            break;		
        case FBE_CLASS_ID_SAS_ANCHO_ENCLOSURE:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(sas_ancho_enclosure);
            break;
        case FBE_CLASS_ID_SAS_PINECONE_ENCLOSURE:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(sas_pinecone_enclosure);
            break;
        case FBE_CLASS_ID_SAS_STEELJAW_ENCLOSURE:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(sas_steeljaw_enclosure);
            break;
        case FBE_CLASS_ID_SAS_RAMHORN_ENCLOSURE:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(sas_ramhorn_enclosure);
            break;
        case FBE_CLASS_ID_SAS_RHEA_ENCLOSURE:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(sas_rhea_enclosure);
            break;
        case FBE_CLASS_ID_SAS_MIRANDA_ENCLOSURE:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(sas_miranda_enclosure);
            break;
        case FBE_CLASS_ID_SAS_CALYPSO_ENCLOSURE:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(sas_calypso_enclosure);
            break;

        case FBE_CLASS_ID_RAID_GROUP:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(raid_group);
            break;
        case FBE_CLASS_ID_MIRROR:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(mirror);
            break;
        case FBE_CLASS_ID_STRIPER:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(striper);
            break;
        case FBE_CLASS_ID_PARITY:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(parity);
            break;
        case FBE_CLASS_ID_VIRTUAL_DRIVE:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(virtual_drive);
            break;
        case FBE_CLASS_ID_LUN:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(lun);
            break;
        case FBE_CLASS_ID_BVD_INTERFACE:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(bvd_interface);
            break;
        case FBE_CLASS_ID_PROVISION_DRIVE:
            *pp_class_const = &FBE_LIFECYCLE_CONST_DATA(provision_drive);
            break;

        default:
            return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

