#ifndef BGSL_CLASS_H
#define BGSL_CLASS_H

#include "fbe/bgsl_types.h"

typedef enum bgsl_class_id_e {
    BGSL_CLASS_ID_INVALID,
    BGSL_CLASS_ID_VERTEX,

    BGSL_CLASS_ID_BASE_OBJECT,
    BGSL_CLASS_ID_BASE_DISCOVERED,
    BGSL_CLASS_ID_BASE_DISCOVERING,

    BGSL_CLASS_ID_BOARD_FIRST,
    BGSL_CLASS_ID_BASE_BOARD,
    BGSL_CLASS_ID_HAMMERHEAD_BOARD,
    BGSL_CLASS_ID_SLEDGEHAMMER_BOARD,
    BGSL_CLASS_ID_JACKHAMMER_BOARD,
    BGSL_CLASS_ID_BOOMSLANG_BOARD,
    BGSL_CLASS_ID_DELL_BOARD,
    BGSL_CLASS_ID_FLEET_BOARD,
    BGSL_CLASS_ID_MAGNUM_BOARD,
    BGSL_CLASS_ID_ARMADA_BOARD,
    BGSL_CLASS_ID_BOARD_LAST,

    BGSL_CLASS_ID_PORT_FIRST,
    BGSL_CLASS_ID_BASE_PORT,
    BGSL_CLASS_ID_FIBRE_PORT,
    BGSL_CLASS_ID_SAS_PORT,
    BGSL_CLASS_ID_SAS_LSI_PORT,
    BGSL_CLASS_ID_SAS_CPD_PORT,
    BGSL_CLASS_ID_SAS_PMC_PORT,
    BGSL_CLASS_ID_FIBRE_XPE_PORT,
    BGSL_CLASS_ID_FIBRE_DPE_PORT,
    BGSL_CLASS_ID_PORT_LAST,

    BGSL_CLASS_ID_LCC_FIRST,
    BGSL_CLASS_ID_BASE_LCC,
    BGSL_CLASS_ID_FIBRE_LCC,
    BGSL_CLASS_ID_FIBRE_KOTO_LCC,
    BGSL_CLASS_ID_FIBRE_YUKON_LCC,
    BGSL_CLASS_ID_FIBRE_BROADSWORD_2G_LCC,
    BGSL_CLASS_ID_FIBRE_BONESWORD_4G_LCC,
    BGSL_CLASS_ID_SAS_LCC,
    BGSL_CLASS_ID_SAS_BULLET_LCC,
    BGSL_CLASS_ID_LCC_LAST,

    BGSL_CLASS_ID_LOGICAL_DRIVE_FIRST,
    BGSL_CLASS_ID_BASE_LOGICAL_DRIVE,

    BGSL_CLASS_ID_LOGICAL_DRIVE,
    BGSL_CLASS_ID_LOGICAL_DRIVE_LAST,
    
    BGSL_CLASS_ID_PHYSICAL_DRIVE_FIRST,
    BGSL_CLASS_ID_BASE_PHYSICAL_DRIVE,
    BGSL_CLASS_ID_FIBRE_PHYSICAL_DRIVE_FIRST,
    BGSL_CLASS_ID_FIBRE_PHYSICAL_DRIVE,
    BGSL_CLASS_ID_FIBRE_PHYSICAL_DRIVE_LAST,

    BGSL_CLASS_ID_SAS_PHYSICAL_DRIVE_FIRST,
    BGSL_CLASS_ID_SAS_PHYSICAL_DRIVE,
    BGSL_CLASS_ID_SAS_FLASH_DRIVE,
    /* All other SAS drives */
    BGSL_CLASS_ID_SAS_PHYSICAL_DRIVE_LAST,

    BGSL_CLASS_ID_SATA_PHYSICAL_DRIVE_FIRST,
    BGSL_CLASS_ID_SATA_PHYSICAL_DRIVE,
    BGSL_CLASS_ID_SATA_FLASH_DRIVE,
    /* All other SATA drives */
    BGSL_CLASS_ID_SATA_PHYSICAL_DRIVE_LAST,

    BGSL_CLASS_ID_PHYSICAL_DRIVE_LAST,

    BGSL_CLASS_ID_ENCLOSURE_FIRST,
    BGSL_CLASS_ID_BASE_ENCLOSURE,
    BGSL_CLASS_ID_SAS_ENCLOSURE_FIRST,
    BGSL_CLASS_ID_SAS_ENCLOSURE,
    BGSL_CLASS_ID_ESES_ENCLOSURE,
    BGSL_CLASS_ID_SAS_BULLET_ENCLOSURE,
    BGSL_CLASS_ID_SAS_VIPER_ENCLOSURE,
    BGSL_CLASS_ID_SAS_MAGNUM_ENCLOSURE,
    BGSL_CLASS_ID_SAS_CITADEL_ENCLOSURE,
    BGSL_CLASS_ID_SAS_BUNKER_ENCLOSURE,
    BGSL_CLASS_ID_SAS_DERRINGER_ENCLOSURE,
    BGSL_CLASS_ID_SAS_TABASCO_ENCLOSURE,
    /* Add all other SAS Enclosures here */
    BGSL_CLASS_ID_SAS_ENCLOSURE_LAST,

    BGSL_CLASS_ID_ENCLOSURE_LAST,

    // FLARE_Service Classes
    BGSL_L_CLASS_ID_EVENT_HANDLER,
    BGSL_L_CLASS_ID_DATABASE_HANDLER,
    BGSL_L_CLASS_ID_SP_HANDLER,
    BGSL_L_CLASS_ID_MISC_ADAPTER,
    BGSL_L_CLASS_ID_ADAPTER_CLASS,

    // BGS_Service Classes
    BGSL_L_CLASS_ID_BGS_SCHEDULER,
    BGSL_L_CLASS_ID_BGS_MONITOR,

	BGSL_CLASS_ID_DRIVE_LAST,
    BGSL_CLASS_ID_LAST
} bgsl_class_id_t;

typedef const struct bgsl_const_class_info {
    const char * class_name;
    bgsl_class_id_t class_id;
} bgsl_const_class_info_t;

bgsl_status_t bgsl_get_class_by_id(bgsl_class_id_t class_id, bgsl_const_class_info_t ** pp_class_info);
bgsl_status_t bgsl_get_class_by_name(const char * class_name, bgsl_const_class_info_t ** pp_class_info);

#endif /* BGSL_CLASS_H */