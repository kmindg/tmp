/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
****************************************************************************/

#include "fbe/fbe_physical_drive.h"
#include "fbe/fbe_types.h"
#include "terminator_board.h"
#include "terminator_port.h"
#include "terminator_enclosure.h"
#include "terminator_drive.h"
#include "terminator_enum_conversions.h"
#include <ctype.h>

/* Routines for converting attributes from strings to their actual types and vice versa */
/* May be, sometimes this source will be generated automatically... */

typedef struct string_to_enum_s{
    int enum_value;
    fbe_u8_t *  enum_str;
} string_to_enum_t;

#define STRING_TO_ENUM_RECORD(_enum) {_enum, #_enum}

static string_to_enum_t string_to_component_type [] =
{
    STRING_TO_ENUM_RECORD( TERMINATOR_COMPONENT_TYPE_INVALID ),
    STRING_TO_ENUM_RECORD( TERMINATOR_COMPONENT_TYPE_BOARD ),
    STRING_TO_ENUM_RECORD( TERMINATOR_COMPONENT_TYPE_PORT ),
    STRING_TO_ENUM_RECORD( TERMINATOR_COMPONENT_TYPE_ENCLOSURE ),
    STRING_TO_ENUM_RECORD( TERMINATOR_COMPONENT_TYPE_DRIVE ),
    STRING_TO_ENUM_RECORD( TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY ),
    STRING_TO_ENUM_RECORD( TERMINATOR_COMPONENT_TYPE_LAST )
};

static int string_to_component_type_len = sizeof(string_to_component_type)/sizeof(string_to_enum_t);

static string_to_enum_t string_to_board_type [] =
{
    STRING_TO_ENUM_RECORD( FBE_BOARD_TYPE_X1 ),
    STRING_TO_ENUM_RECORD( FBE_BOARD_TYPE_X1_LITE ),
    STRING_TO_ENUM_RECORD( FBE_BOARD_TYPE_CHAM2 ),
    STRING_TO_ENUM_RECORD( FBE_BOARD_TYPE_X1_SUNLITE ),
    STRING_TO_ENUM_RECORD( FBE_BOARD_TYPE_TARPON ),
    STRING_TO_ENUM_RECORD( FBE_BOARD_TYPE_BARRACUDA ),
    STRING_TO_ENUM_RECORD( FBE_BOARD_TYPE_SNAPPER ),
    STRING_TO_ENUM_RECORD( FBE_BOARD_TYPE_ACADIA ),
    STRING_TO_ENUM_RECORD( FBE_BOARD_TYPE_GRAND_TETON ),
    STRING_TO_ENUM_RECORD( FBE_BOARD_TYPE_FRESHFISH ),
    STRING_TO_ENUM_RECORD( FBE_BOARD_TYPE_YOSEMITE ),
    STRING_TO_ENUM_RECORD( FBE_BOARD_TYPE_BIG_SUR ),
    STRING_TO_ENUM_RECORD( FBE_BOARD_TYPE_HAMMERHEAD ),
    STRING_TO_ENUM_RECORD( FBE_BOARD_TYPE_SLEDGEHAMMER ),
    STRING_TO_ENUM_RECORD( FBE_BOARD_TYPE_JACKHAMMER ),
    STRING_TO_ENUM_RECORD( FBE_BOARD_TYPE_VMWARE ),
    STRING_TO_ENUM_RECORD( FBE_BOARD_TYPE_TACKHAMMER ),
    STRING_TO_ENUM_RECORD( FBE_BOARD_TYPE_BOOMSLANG ),
//  should we keep previous board types?
    STRING_TO_ENUM_RECORD( FBE_BOARD_TYPE_DELL ),
    STRING_TO_ENUM_RECORD( FBE_BOARD_TYPE_FLEET ),
    STRING_TO_ENUM_RECORD( FBE_BOARD_TYPE_MAGNUM ),
    STRING_TO_ENUM_RECORD( FBE_BOARD_TYPE_ARMADA ),
    STRING_TO_ENUM_RECORD( FBE_BOARD_TYPE_LAST )
};

static int string_to_board_type_len = sizeof(string_to_board_type)/sizeof(string_to_enum_t);

static string_to_enum_t string_to_platform_type [] =
{
    STRING_TO_ENUM_RECORD( SPID_UNKNOWN_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_X1_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_X1_LITE_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_CHAM2_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_X1_SUNLITE_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_TARPON_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_BARRACUDA_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_SNAPPER_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_ACADIA_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_GRAND_TETON_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_FRESHFISH_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_YOSEMITE_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_BIG_SUR_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_HAMMERHEAD_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_SLEDGEHAMMER_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_JACKHAMMER_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_VMWARE_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_TACKHAMMER_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_DREADNOUGHT_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_TRIDENT_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_IRONCLAD_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_NAUTILUS_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_BOOMSLANG_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_VIPER_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_COMMUP_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_MAGNUM_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_ZODIAC_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_CORSAIR_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_SPITFIRE_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_LIGHTNING_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_HELLCAT_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_HELLCAT_LITE_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_MUSTANG_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_MUSTANG_XM_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_HELLCAT_XM_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_PROMETHEUS_M1_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_PROMETHEUS_S1_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_DEFIANT_M1_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_DEFIANT_M2_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_DEFIANT_M3_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_DEFIANT_M4_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_DEFIANT_M5_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_DEFIANT_S1_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_DEFIANT_S4_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_DEFIANT_K2_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_DEFIANT_K3_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_EVERGREEN_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_SENTRY_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_NOVA1_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_NOVA3_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_NOVA3_XM_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_NOVA_S1_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_ENTERPRISE_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_OBERON_1_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_OBERON_2_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_OBERON_3_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_OBERON_4_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_OBERON_S1_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_HYPERION_1_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_HYPERION_2_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_HYPERION_3_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_TRITON_1_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_MERIDIAN_ESX_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_TUNGSTEN_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_BEARCAT_HW_TYPE ),
    STRING_TO_ENUM_RECORD( SPID_MAXIMUM_HW_TYPE )
};

static int string_to_platform_type_len = sizeof(string_to_platform_type)/sizeof(string_to_enum_t);

static string_to_enum_t string_to_port_type [] =
{
    //STRING_TO_ENUM_RECORD( FBE_PORT_TYPE_FIBRE ),
    STRING_TO_ENUM_RECORD( FBE_PORT_TYPE_SAS_LSI ),
    STRING_TO_ENUM_RECORD( FBE_PORT_TYPE_SAS_PMC ),
    STRING_TO_ENUM_RECORD( FBE_PORT_TYPE_FC_PMC ),
    STRING_TO_ENUM_RECORD( FBE_PORT_TYPE_ISCSI ),
    STRING_TO_ENUM_RECORD( FBE_PORT_TYPE_FCOE ),
    STRING_TO_ENUM_RECORD( FBE_PORT_TYPE_LAST )
};

static int string_to_port_type_len = sizeof(string_to_port_type)/sizeof(string_to_enum_t);

static string_to_enum_t string_to_port_connect_class [] =
{
    STRING_TO_ENUM_RECORD( FBE_PORT_CONNECT_CLASS_SAS ),
    STRING_TO_ENUM_RECORD( FBE_PORT_CONNECT_CLASS_FC ),
    STRING_TO_ENUM_RECORD( FBE_PORT_CONNECT_CLASS_ISCSI ),
    STRING_TO_ENUM_RECORD( FBE_PORT_CONNECT_CLASS_FCOE ),
    STRING_TO_ENUM_RECORD( FBE_PORT_CONNECT_CLASS_LAST )
};

static int string_to_port_connect_class_len = sizeof(string_to_port_connect_class)/sizeof(string_to_enum_t);

static string_to_enum_t string_to_port_role [] =
{
    STRING_TO_ENUM_RECORD( FBE_PORT_ROLE_FE ),
    STRING_TO_ENUM_RECORD( FBE_PORT_ROLE_BE ),
    STRING_TO_ENUM_RECORD( FBE_PORT_ROLE_UNC),
    STRING_TO_ENUM_RECORD( FBE_PORT_ROLE_BOOT),
    STRING_TO_ENUM_RECORD( FBE_PORT_ROLE_SPECIAL),
    STRING_TO_ENUM_RECORD( FBE_PORT_ROLE_MAX)
};

static int string_to_port_role_len = sizeof(string_to_port_role)/sizeof(string_to_enum_t);

static string_to_enum_t string_to_enclosure_type [] =
{
    STRING_TO_ENUM_RECORD( FBE_ENCLOSURE_TYPE_FIBRE ),
    STRING_TO_ENUM_RECORD( FBE_ENCLOSURE_TYPE_SAS),    
    STRING_TO_ENUM_RECORD( FBE_ENCLOSURE_TYPE_LAST )
};

static int string_to_enclosure_type_len = sizeof(string_to_enclosure_type)/sizeof(string_to_enum_t);

static string_to_enum_t string_to_sas_enclosure_type [] =
{
    STRING_TO_ENUM_RECORD( FBE_SAS_ENCLOSURE_TYPE_BULLET ),
    STRING_TO_ENUM_RECORD( FBE_SAS_ENCLOSURE_TYPE_VIPER ),
    STRING_TO_ENUM_RECORD( FBE_SAS_ENCLOSURE_TYPE_MAGNUM ),
    STRING_TO_ENUM_RECORD( FBE_SAS_ENCLOSURE_TYPE_BUNKER ),
    STRING_TO_ENUM_RECORD( FBE_SAS_ENCLOSURE_TYPE_CITADEL ),
    STRING_TO_ENUM_RECORD( FBE_SAS_ENCLOSURE_TYPE_DERRINGER),
    STRING_TO_ENUM_RECORD( FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM),
    STRING_TO_ENUM_RECORD( FBE_SAS_ENCLOSURE_TYPE_VOYAGER_EE),    
    STRING_TO_ENUM_RECORD( FBE_SAS_ENCLOSURE_TYPE_FALLBACK ),
    STRING_TO_ENUM_RECORD( FBE_SAS_ENCLOSURE_TYPE_BOXWOOD ),
    STRING_TO_ENUM_RECORD( FBE_SAS_ENCLOSURE_TYPE_KNOT ),
    STRING_TO_ENUM_RECORD( FBE_SAS_ENCLOSURE_TYPE_PINECONE ),
    STRING_TO_ENUM_RECORD( FBE_SAS_ENCLOSURE_TYPE_STEELJAW ),
    STRING_TO_ENUM_RECORD( FBE_SAS_ENCLOSURE_TYPE_RAMHORN ),
    STRING_TO_ENUM_RECORD( FBE_SAS_ENCLOSURE_TYPE_ANCHO ),
    STRING_TO_ENUM_RECORD( FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP),
    STRING_TO_ENUM_RECORD( FBE_SAS_ENCLOSURE_TYPE_VIKING_DRVSXP),
    STRING_TO_ENUM_RECORD( FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP),
    STRING_TO_ENUM_RECORD( FBE_SAS_ENCLOSURE_TYPE_CAYENNE_DRVSXP),
    STRING_TO_ENUM_RECORD( FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP),
    STRING_TO_ENUM_RECORD( FBE_SAS_ENCLOSURE_TYPE_NAGA_DRVSXP),
    STRING_TO_ENUM_RECORD( FBE_SAS_ENCLOSURE_TYPE_RHEA ),
    STRING_TO_ENUM_RECORD( FBE_SAS_ENCLOSURE_TYPE_MIRANDA ),
    STRING_TO_ENUM_RECORD( FBE_SAS_ENCLOSURE_TYPE_CALYPSO ),
    STRING_TO_ENUM_RECORD( FBE_SAS_ENCLOSURE_TYPE_TABASCO),
    STRING_TO_ENUM_RECORD( FBE_SAS_ENCLOSURE_TYPE_LAST )
};

static int string_to_sas_enclosure_type_len = sizeof(string_to_sas_enclosure_type)/sizeof(string_to_enum_t);
static string_to_enum_t string_to_fc_enclosure_type [] =
{
    STRING_TO_ENUM_RECORD( FBE_FC_ENCLOSURE_TYPE_KATANA ),
    STRING_TO_ENUM_RECORD( FBE_FC_ENCLOSURE_TYPE_STILLETO ),
    STRING_TO_ENUM_RECORD( FBE_FC_ENCLOSURE_TYPE_KLONDIKE ),
    STRING_TO_ENUM_RECORD( FBE_SAS_ENCLOSURE_TYPE_LAST )
};

static int string_to_fc_enclosure_type_len = sizeof(string_to_fc_enclosure_type)/sizeof(string_to_enum_t);

static string_to_enum_t string_to_sas_drive_type [] =
{
    STRING_TO_ENUM_RECORD( FBE_SAS_DRIVE_CHEETA_15K ),
    STRING_TO_ENUM_RECORD( FBE_SAS_DRIVE_UNICORN_512 ),
    STRING_TO_ENUM_RECORD( FBE_SAS_DRIVE_UNICORN_4096 ),
    STRING_TO_ENUM_RECORD( FBE_SAS_DRIVE_UNICORN_4160 ),
    STRING_TO_ENUM_RECORD( FBE_SAS_DRIVE_SIM_520 ),
    STRING_TO_ENUM_RECORD( FBE_SAS_NL_DRIVE_SIM_520 ),
    STRING_TO_ENUM_RECORD( FBE_SAS_DRIVE_SIM_512 ),
    STRING_TO_ENUM_RECORD( FBE_SAS_DRIVE_SIM_520_FLASH_HE ),
    STRING_TO_ENUM_RECORD( FBE_SAS_DRIVE_SIM_520_FLASH_UNMAP ),
    STRING_TO_ENUM_RECORD( FBE_SAS_DRIVE_SIM_4160_FLASH_UNMAP ),
    STRING_TO_ENUM_RECORD( FBE_SAS_DRIVE_SIM_520_UNMAP ),
    STRING_TO_ENUM_RECORD( FBE_SAS_DRIVE_UNICORN_4160_UNMAP ),
    STRING_TO_ENUM_RECORD( FBE_SAS_DRIVE_COBRA_E_10K ),
    STRING_TO_ENUM_RECORD( FBE_SAS_DRIVE_SIM_520_12G),
    STRING_TO_ENUM_RECORD( FBE_SAS_DRIVE_SIM_520_FLASH_ME ),
    STRING_TO_ENUM_RECORD( FBE_SAS_DRIVE_SIM_520_FLASH_LE ),
    STRING_TO_ENUM_RECORD( FBE_SAS_DRIVE_SIM_520_FLASH_RI ),

    STRING_TO_ENUM_RECORD( FBE_SAS_DRIVE_LAST )
};

static int string_to_sas_drive_type_len = sizeof(string_to_sas_drive_type)/sizeof(string_to_enum_t);
static string_to_enum_t string_to_fc_drive_type [] =
{
    STRING_TO_ENUM_RECORD( FBE_FC_DRIVE_CHEETA_15K ),
    STRING_TO_ENUM_RECORD( FBE_FC_DRIVE_UNICORN_512 ),
    STRING_TO_ENUM_RECORD( FBE_FC_DRIVE_UNICORN_4096 ),
    STRING_TO_ENUM_RECORD( FBE_FC_DRIVE_UNICORN_4160 ),
    STRING_TO_ENUM_RECORD( FBE_FC_DRIVE_SIM_520 ),
    STRING_TO_ENUM_RECORD( FBE_FC_DRIVE_SIM_512 ),
    STRING_TO_ENUM_RECORD( FBE_FC_DRIVE_LAST )
};

static int string_to_fc_drive_type_len = sizeof(string_to_fc_drive_type)/sizeof(string_to_enum_t);

static string_to_enum_t string_to_sata_drive_type [] =
{
    STRING_TO_ENUM_RECORD( FBE_SATA_DRIVE_INVALID ),
    STRING_TO_ENUM_RECORD( FBE_SATA_DRIVE_HITACHI_HUA ),
    STRING_TO_ENUM_RECORD( FBE_SATA_DRIVE_SIM_512 ),
    STRING_TO_ENUM_RECORD( FBE_SATA_DRIVE_SIM_512_FLASH ),
    STRING_TO_ENUM_RECORD( FBE_SATA_DRIVE_LAST )
};

static int string_to_sata_drive_type_len = sizeof(string_to_sata_drive_type)/sizeof(string_to_enum_t);

static string_to_enum_t string_to_fbe_drive_type [] =
{
    STRING_TO_ENUM_RECORD( FBE_DRIVE_TYPE_INVALID ),
    STRING_TO_ENUM_RECORD( FBE_DRIVE_TYPE_FIBRE ),
    STRING_TO_ENUM_RECORD( FBE_DRIVE_TYPE_SAS ),
    STRING_TO_ENUM_RECORD( FBE_DRIVE_TYPE_SAS_FLASH_HE ),
    STRING_TO_ENUM_RECORD( FBE_DRIVE_TYPE_SATA ),
    STRING_TO_ENUM_RECORD( FBE_DRIVE_TYPE_SATA_FLASH_HE ),
    STRING_TO_ENUM_RECORD( FBE_DRIVE_TYPE_SAS_FLASH_ME),
    STRING_TO_ENUM_RECORD( FBE_DRIVE_TYPE_SAS_FLASH_LE),
    STRING_TO_ENUM_RECORD( FBE_DRIVE_TYPE_SAS_FLASH_RI ),
    STRING_TO_ENUM_RECORD( FBE_DRIVE_TYPE_LAST )
};

static int string_to_fbe_drive_type_len = sizeof(string_to_fbe_drive_type)/sizeof(string_to_enum_t);

/* convert symbolic repsentation of enumerated constant to its numeric value */
static fbe_status_t convert_string_to_enum_value(const char * str, const string_to_enum_t * table,
                                                     int table_size, int * result)
{
    int i = 0;
    char buffer[256];
    if(str == NULL || table == NULL || result == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /*remove leading and trailing spaces*/
    while(*str && isspace(*str))
    {
        str++;
    }
    while(*str && !isspace(*str))
    {
        buffer[i++] = *str++;
        if(i == sizeof(buffer) - 1) break;
    }
    buffer[i] = '\0';
    for(i = 0; i < table_size; i++)
    {
        if(!strcmp(buffer, table[i].enum_str))
        {
            *result = table[i].enum_value;
            return FBE_STATUS_OK;
        }
    }
    return FBE_STATUS_GENERIC_FAILURE;
}

/* convert enumerated constant to its symbolic representation */
static fbe_status_t convert_enum_value_to_string(int enum_value, const string_to_enum_t * table,
                                                 int table_size, const char ** result)
{
    int i;
    if(result == NULL || table == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    for(i = 0; i < table_size; i++)
    {
        if(table[i].enum_value == enum_value )
        {
            *result = table[i].enum_str;
            return FBE_STATUS_OK;
        }
    }
    *result = NULL;
    return FBE_STATUS_GENERIC_FAILURE;
}

fbe_status_t terminator_string_to_component_type(
    const char * component_type_string,
    terminator_component_type_t * result)
{
    return convert_string_to_enum_value(
    	component_type_string,
    	string_to_component_type,
        string_to_component_type_len,
        (int *)result);
}

fbe_status_t terminator_component_type_to_string(
	terminator_component_type_t component_type,
    const char ** result)
{
    return convert_enum_value_to_string(
    	component_type,
        string_to_component_type,
        string_to_component_type_len,
        result);
}

fbe_status_t terminator_string_to_board_type(
    const char * board_type_string,
    fbe_board_type_t * result)
{
    return convert_string_to_enum_value(
        board_type_string,
        string_to_board_type,
        string_to_board_type_len,
        (int *)result);
}

fbe_status_t terminator_board_type_to_string(
    fbe_board_type_t board_type,
    const char ** result)
{
    return convert_enum_value_to_string(
        board_type,
        string_to_board_type,
        string_to_board_type_len,
        result);
}

fbe_status_t terminator_string_to_platform_type(
    const char * platform_type_string,
    SPID_HW_TYPE * result)
{
    return convert_string_to_enum_value(
        platform_type_string,
        string_to_platform_type,
        string_to_platform_type_len,
        (int *)result);
}

fbe_status_t terminator_platform_type_to_string(
    SPID_HW_TYPE platform_type,
    const char ** result)
{
    return convert_enum_value_to_string(
        platform_type,
        string_to_platform_type,
        string_to_platform_type_len,
        result);
}

fbe_status_t terminator_string_to_port_type(
    const char * port_type_string,
    fbe_port_type_t * result)
{
    return convert_string_to_enum_value(
        port_type_string,
        string_to_port_type,
        string_to_port_type_len,
        (int *)result);
}

fbe_status_t terminator_port_type_to_string(
    fbe_port_type_t port_type,
    const char ** result)
{
    return convert_enum_value_to_string(
        port_type,
        string_to_port_type,
        string_to_port_type_len,
        result);
}

fbe_status_t terminator_string_to_port_connect_class(
    const char * port_connect_class_string,
    fbe_port_connect_class_t * result)
{
    return convert_string_to_enum_value(
        port_connect_class_string,
        string_to_port_connect_class,
        string_to_port_connect_class_len,
        (int *)result);
}

fbe_status_t terminator_port_connect_class_to_string(
    fbe_port_connect_class_t port_connect_class,
    const char ** result)
{
    return convert_enum_value_to_string(
        port_connect_class,
        string_to_port_connect_class,
        string_to_port_connect_class_len,
        result);
}

fbe_status_t terminator_string_to_port_role(
    const char * port_role_string,
    fbe_port_role_t * result)
{
    return convert_string_to_enum_value(
        port_role_string,
        string_to_port_role,
        string_to_port_role_len,
        (int *)result);
}

fbe_status_t terminator_port_role_to_string(
    fbe_port_role_t port_role,
    const char ** result)
{
    return convert_enum_value_to_string(
        port_role,
        string_to_port_role,
        string_to_port_role_len,
        result);
}

fbe_status_t terminator_string_to_enclosure_type(
    const char * enclosure_type_string,
    fbe_enclosure_type_t * result)
{
    return convert_string_to_enum_value(
        enclosure_type_string,
        string_to_enclosure_type,
        string_to_enclosure_type_len,
        (int *)result);
}

fbe_status_t terminator_enclosure_type_to_string(
    fbe_enclosure_type_t enclosure_type,
    const char ** result)
{
    return convert_enum_value_to_string(
        enclosure_type,
        string_to_enclosure_type,
        string_to_enclosure_type_len,
        result);
}

fbe_status_t terminator_string_to_sas_enclosure_type(
    const char * sas_enclosure_type_string,
    fbe_sas_enclosure_type_t * result)
{
    return convert_string_to_enum_value(
        sas_enclosure_type_string,
        string_to_sas_enclosure_type,
        string_to_sas_enclosure_type_len,
        (int *)result);
}

fbe_status_t terminator_sas_enclosure_type_to_string(
    fbe_sas_enclosure_type_t sas_enclosure_type,
    const char ** result)
{
    return convert_enum_value_to_string(
        sas_enclosure_type,
        string_to_sas_enclosure_type,
        string_to_sas_enclosure_type_len,
        result);
}

fbe_status_t terminator_string_to_fc_enclosure_type(
    const char * fc_enclosure_type_string,
    fbe_fc_enclosure_type_t * result)
{
    return convert_string_to_enum_value(
        fc_enclosure_type_string,
        string_to_fc_enclosure_type,
        string_to_fc_enclosure_type_len,
        (int *)result);
}

fbe_status_t terminator_fc_enclosure_type_to_string(
    fbe_fc_enclosure_type_t fc_enclosure_type,
    const char ** result)
{
    return convert_enum_value_to_string(
        fc_enclosure_type,
        string_to_fc_enclosure_type,
        string_to_fc_enclosure_type_len,
        result);
}

fbe_status_t terminator_string_to_sas_drive_type(
    const char * sas_drive_type_string,
    fbe_sas_drive_type_t * result)
{
    return convert_string_to_enum_value(
        sas_drive_type_string,
        string_to_sas_drive_type,
        string_to_sas_drive_type_len,
        (int *)result);
}

fbe_status_t terminator_sas_drive_type_to_string(
    fbe_sas_drive_type_t sas_drive_type,
    const char ** result)
{
    return convert_enum_value_to_string(
        sas_drive_type,
        string_to_sas_drive_type,
        string_to_sas_drive_type_len,
        result);
}

fbe_status_t terminator_fc_drive_type_to_string(
    fbe_fc_drive_type_t fc_drive_type,
    const char ** result)
{
    return convert_enum_value_to_string(
        fc_drive_type,
        string_to_fc_drive_type,
        string_to_fc_drive_type_len,
        result);
}

fbe_status_t terminator_string_to_sata_drive_type(
    const char * sata_drive_type_string,
    fbe_sata_drive_type_t * result)
{
    return convert_string_to_enum_value(
        sata_drive_type_string,
        string_to_sata_drive_type,
        string_to_sata_drive_type_len,
        (int *)result);
}

fbe_status_t terminator_sata_drive_type_to_string(
    fbe_sata_drive_type_t sata_drive_type,
    const char ** result)
{
    return convert_enum_value_to_string(
        sata_drive_type,
        string_to_sata_drive_type,
        string_to_sata_drive_type_len,
        result);
}

fbe_status_t terminator_string_to_fc_drive_type(
    const char * fc_drive_type_string,
    fbe_fc_drive_type_t * result)
{
    return convert_string_to_enum_value(
        fc_drive_type_string,
        string_to_fc_drive_type,
        string_to_fc_drive_type_len,
        (int *)result);
}

fbe_status_t terminator_string_to_fbe_drive_type(
    const char * fbe_drive_type_string,
    fbe_drive_type_t * result)
{
    return convert_string_to_enum_value(
        fbe_drive_type_string,
        string_to_fbe_drive_type,
        string_to_fbe_drive_type_len,
        (int *)result);
}

fbe_status_t terminator_fbe_drive_type_to_string(
    fbe_drive_type_t fbe_drive_type,
    const char ** result)
{
    return convert_enum_value_to_string(
        fbe_drive_type,
        string_to_fbe_drive_type,
        string_to_fbe_drive_type_len,
        result);
}

fbe_status_t terminator_decimal_string_to_u64(
    const char * s,
    fbe_u64_t * result)
{
    /*skip leading spaces*/
    while(isspace(*s))
    {
        s++;
    }
    if(isdigit(*s))
    {
        fbe_u64_t r = 0;
        do
        {
            r = 10*r + (fbe_u64_t)(*s - '0');
        }
        while(isdigit(*++s));
        /*skip trailing spaces*/
        while(isspace(*s))
        {
            s++;
        }
        /*check that we get into the end of line*/
        if(!(*s))
        {
            *result = r;
            return FBE_STATUS_OK;
        }
    }
    return FBE_STATUS_GENERIC_FAILURE;
}

fbe_status_t terminator_hexadecimal_string_to_u64(
    const char * s,
    fbe_u64_t * result)
{
    /*skip leading spaces*/
    while(isspace(*s))
    {
        s++;
    }
    /*skip 0x */
    if(*s == '0' && (*(s+1) == 'x' || *(s+1) == 'X'))
    {
        s+=2;
    }
    if(isxdigit(*s))
    {
        fbe_u64_t r = 0;
        do
        {
            if(isdigit(*s))
            {
                r = 16*r + (fbe_u64_t)(*s - '0');
            }
            else
            {
                r = 16*r + (fbe_u64_t)((toupper(*s) - 55));
            }
        }
        while(isxdigit(*++s));
        /*skip trailing spaces*/
        while(isspace(*s))
        {
            s++;
        }
        /*check that we get into the end of line*/
        if(!(*s))
        {
            *result = r;
            return FBE_STATUS_OK;
        }
    }
    return FBE_STATUS_GENERIC_FAILURE;
}


fbe_status_t terminator_platform_type_to_board_type(SPID_HW_TYPE platform_type, fbe_board_type_t *board_type)
{
    fbe_status_t status = FBE_STATUS_OK;

    switch(platform_type)
    {
        case SPID_DREADNOUGHT_HW_TYPE:
        case SPID_TRIDENT_HW_TYPE:
        case SPID_IRONCLAD_HW_TYPE:
        case SPID_NAUTILUS_HW_TYPE:
            *board_type = FBE_BOARD_TYPE_FLEET;
            break;

        case SPID_ZODIAC_HW_TYPE:
            *board_type = FBE_BOARD_TYPE_MAGNUM;
            break;

        case SPID_LIGHTNING_HW_TYPE:
        case SPID_CORSAIR_HW_TYPE:
        case SPID_HELLCAT_HW_TYPE:
        case SPID_HELLCAT_LITE_HW_TYPE:
        case SPID_HELLCAT_XM_HW_TYPE:
        case SPID_SPITFIRE_HW_TYPE:
        case SPID_MUSTANG_HW_TYPE:
        case SPID_MUSTANG_XM_HW_TYPE:

        case SPID_ENTERPRISE_HW_TYPE:/* HACKS!!! */

        case SPID_PROMETHEUS_M1_HW_TYPE:/* HACKS!!! */
        case SPID_PROMETHEUS_S1_HW_TYPE:
        case SPID_DEFIANT_M1_HW_TYPE:
        case SPID_DEFIANT_M2_HW_TYPE:
        case SPID_DEFIANT_M3_HW_TYPE:
        case SPID_DEFIANT_M4_HW_TYPE:
        case SPID_DEFIANT_M5_HW_TYPE:
        case SPID_DEFIANT_S1_HW_TYPE:
        case SPID_DEFIANT_S4_HW_TYPE:
        case SPID_DEFIANT_K2_HW_TYPE:
        case SPID_DEFIANT_K3_HW_TYPE:
        case SPID_NOVA1_HW_TYPE:
        case SPID_NOVA3_HW_TYPE:
        case SPID_NOVA3_XM_HW_TYPE:
        case SPID_NOVA_S1_HW_TYPE:
        case SPID_SENTRY_HW_TYPE:
        case SPID_EVERGREEN_HW_TYPE:
        case SPID_BEARCAT_HW_TYPE:
// Moons
        case SPID_OBERON_1_HW_TYPE:
        case SPID_OBERON_2_HW_TYPE:
        case SPID_OBERON_3_HW_TYPE:
        case SPID_OBERON_4_HW_TYPE:
        case SPID_OBERON_S1_HW_TYPE:
        case SPID_HYPERION_1_HW_TYPE:
        case SPID_HYPERION_2_HW_TYPE:
        case SPID_HYPERION_3_HW_TYPE:
        case SPID_TRITON_1_HW_TYPE:
            *board_type = FBE_BOARD_TYPE_ARMADA;
            break;

        /* VVNX Virtual; TODO: Revisit; Same as KH sim */
        case SPID_MERIDIAN_ESX_HW_TYPE:
        case SPID_TUNGSTEN_HW_TYPE:
            *board_type = FBE_BOARD_TYPE_ARMADA;
            break;

        case SPID_UNKNOWN_HW_TYPE:
        default:
            *board_type = FBE_BOARD_TYPE_INVALID;
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    return status;
}

