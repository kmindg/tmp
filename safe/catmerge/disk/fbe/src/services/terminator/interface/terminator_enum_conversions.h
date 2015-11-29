/***************************************************************************
 * Copyright (C) EMC Corporation 2008-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
****************************************************************************/

/* Routines for converting attributes from strings to their actual types and vice versa */
/* May be, sometimes this source will be generated automatically... */

/* It is enough for the first time (XML parser, class management, etc) */
/* feel free to add any Terminator enumeration you need */

fbe_status_t terminator_string_to_component_type(
    const char * component_type_string,
    terminator_component_type_t * result);

fbe_status_t terminator_component_type_to_string(
	terminator_component_type_t component_type,
    const char ** result);

fbe_status_t terminator_string_to_board_type(
    const char * board_type_string, 
    fbe_board_type_t * result);

fbe_status_t terminator_board_type_to_string(
    fbe_board_type_t board_type,
    const char ** result);

fbe_status_t terminator_string_to_platform_type(
    const char * platform_type_string, 
    SPID_HW_TYPE * result);

fbe_status_t terminator_platform_type_to_string(
    SPID_HW_TYPE platform_type,
    const char ** result);

fbe_status_t terminator_string_to_port_type(
    const char * port_type_string,
    fbe_port_type_t * result);


fbe_status_t terminator_port_type_to_string(
    fbe_port_type_t port_type,
    const char ** result);

fbe_status_t terminator_string_to_port_connect_class(
    const char * port_connect_class_string,
    fbe_port_connect_class_t * result);

fbe_status_t terminator_port_connect_class_to_string(
    fbe_port_connect_class_t port_connect_class,
    const char ** result);

fbe_status_t terminator_string_to_port_role(
    const char * port_role_string,
    fbe_port_role_t * result);

fbe_status_t terminator_port_role_to_string(
    fbe_port_role_t port_role,
    const char ** result);

fbe_status_t terminator_string_to_enclosure_type(
    const char * enclosure_type_string,
    fbe_enclosure_type_t * result);

fbe_status_t terminator_enclosure_type_to_string(
    fbe_enclosure_type_t enclosure_type_string,
    const char ** result);

fbe_status_t terminator_string_to_enclosure_type(
    const char * enclosure_type_string,
    fbe_enclosure_type_t * result);

fbe_status_t terminator_enclosure_type_to_string(
    fbe_enclosure_type_t enclosure_type_string,
    const char ** result);

fbe_status_t terminator_string_to_sas_enclosure_type(
    const char * enclosure_type_string,
    fbe_sas_enclosure_type_t * result);

fbe_status_t terminator_sas_enclosure_type_to_string(
    fbe_sas_enclosure_type_t sas_enclosure_type,
    const char ** result);

fbe_status_t terminator_string_to_fc_enclosure_type(
    const char * fc_enclosure_type_string,
    fbe_fc_enclosure_type_t * result);

fbe_status_t terminator_fc_enclosure_type_to_string(
    fbe_fc_enclosure_type_t fc_enclosure_type,
    const char ** result);

fbe_status_t terminator_string_to_sas_drive_type(
    const char * sas_drive_type_string,
    fbe_sas_drive_type_t * result);

fbe_status_t terminator_sas_drive_type_to_string(
    fbe_sas_drive_type_t sas_drive_type,
    const char ** result);

fbe_status_t terminator_fc_drive_type_to_string(
    fbe_fc_drive_type_t fc_drive_type,
    const char ** result);

fbe_status_t terminator_string_to_sata_drive_type(
    const char * sas_drive_type_string,
    fbe_sata_drive_type_t * result);

fbe_status_t terminator_sata_drive_type_to_string(
    fbe_sata_drive_type_t sas_drive_type,
    const char ** result);

fbe_status_t terminator_string_to_fc_drive_type(
    const char * fc_drive_type_string,
    fbe_fc_drive_type_t * result);

fbe_status_t terminator_string_to_fbe_drive_type(
    const char * fbe_drive_type_string,
    fbe_drive_type_t * result);

fbe_status_t terminator_fbe_drive_type_to_string(
    fbe_drive_type_t fbe_drive_type,
    const char ** result);

/*  As far as strtol/strtoul causes build to be broken for some strange reason, 
    we will have to reinvent a wheel implementing such conversion routines.
    Note: these routines are not compliant with strtol.
    decimal_string_to_u64 input can be described as:
    
        begin-of-string spaces [0-9]+ spaces end-of-string
   
    hexadecimal_string_to_u64 input can be described as:

        begin-of-string spaces {0x|0X} [0-9A-Fa-f]+ spaces end-of-string
   */
fbe_status_t terminator_decimal_string_to_u64(
    const char * string,
    fbe_u64_t * result);

fbe_status_t terminator_hexadecimal_string_to_u64(
    const char * string,
    fbe_u64_t * result);

fbe_status_t terminator_platform_type_to_board_type(
    SPID_HW_TYPE platform_type, 
    fbe_board_type_t *board_type);


