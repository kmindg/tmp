#ifndef FBE_CLI_LIB_RESUM_PROM_CMDS_H
#define FBE_CLI_LIB_RESUM_PROM_CMDS_H

#include "fbe/fbe_api_resume_prom_interface.h"
#include "fbe/fbe_limits.h"

#define FBE_SP_A 0
#define FBE_SP_B 1
#define FBE_PS_0 0
#define FBE_PS_1 1
#define MAX_BUFSIZE 128

#define GP_USAGE "\
Usages\n \
resumeprom <operation> <switches>\n\n\
    -r -b <bus> -e <enclosure> -s <slot> -d <device type> -sp <0|1> \n\
    -w -b <bus> -e <enclosure> -s <slot> -d <device type> -f <field>  -bs <buffer size> \n\n\
Operation:\n\
    -r      - Read\n\
    -w      - Write\n\n\
Switches:\n\
    -b      - bus number, must specified for device type midplane, drivemidplane, ps, lcc, fan.\n\
                 e.g. -b 1 (-b 254 for xpe)\n\
    -e      - enclosure number, must specified for device type midplane, drivemidplane, ps, lcc, fan.\n\
                 e.g. -e 1 (-e 254 for xpe)\n\
    -s      - slot number, must specified for all device types.\n\
                 e.g. -s 1\n\
    -c      - component id\n\
    -d      - device type: midplane, lcc, fan, iom, ps, mezz, mgmt, sp, drivemidplane, bbu, bm, cachecard. \n\
    -f      - field type: prodpn, prodsn, prodrev, chksum, wwn, emcsn, emcpn\n\
            Note: chksum is for all the devices.Other field types are for processor enclosure midplane resume prom only.\n\
    -sp     - 0 for SPA and 1 for SPB, must specified for device type sp, iom, bm, mezz, bbu and mgmt.\n\
    -bs     - Buffersize, required for the fields other than checksum for the write.\n\n\
Example:\n\
    Read:  rp -r -b 0 -e 0 -s 0 -d lcc\n\n\
    Write: rp -w -b 254 -e 254 -s 0 -d midplane -f chksum\n\
           rp -w -b 0 -e 0 -s 0 -d midplane -f chksum\n\
           rp -w -b 0 -e 0 -s 0 -d midplane -f wwn -bs 8\n\
           0x0860022D\n\
           rp -w -s 1 -d iom -sp 0 -f prodpn -bs 16\n\
           100-502-503\n\
"
void fbe_cli_get_prom_info(int argc, char**argv);

void fbe_cli_lib_resume_prom_read(fbe_u32_t argc, fbe_char_t**argv);
void fbe_cli_lib_resume_prom_write(fbe_u32_t argc, fbe_char_t**argv);
fbe_status_t fbe_cli_get_device_type(fbe_char_t* argv, fbe_u64_t  *dev_type);
fbe_status_t fbe_cli_get_field_type(fbe_char_t* argv, RESUME_PROM_FIELD* field_type);
fbe_status_t get_and_verify_bus_number( char ** argv,
                                       fbe_device_physical_location_t*  location_p );
fbe_status_t get_and_verify_enclosure_number(char ** argv,
                                             fbe_device_physical_location_t*  location_p );
fbe_status_t get_slot_number(char ** argv,
                             fbe_device_physical_location_t*  location_p );
void fbe_cli_lib_resume_prom_display_info (RESUME_PROM_STRUCTURE *prom_info);
fbe_status_t     fbe_cli_lib_resume_prom_get_data(fbe_resume_prom_cmd_t*  writeResume);
const char *get_trace_status(fbe_status_t status);
void fbe_cli_lib_get_field_type(fbe_u32_t field_id, RESUME_PROM_FIELD* resumePromField);
fbe_status_t fbe_cli_getline(fbe_char_t* buffer, fbe_u32_t lim);
const char* get_device_name_string(fbe_u32_t dev_id);

/* FORCERPREAD Function prototypes*/
void fbe_cli_cmd_force_rp_read(fbe_s32_t argc,char ** argv);
static void fbe_cli_force_rp_read(fbe_u64_t deviceType,
                                  fbe_device_physical_location_t deviceLocation);
fbe_status_t fbe_cli_get_number(fbe_char_t* buffer);

void fbe_cli_lib_resume_prom_read_all(fbe_esp_base_env_get_resume_prom_info_cmd_t *pGetResumePromInfo);
fbe_status_t fbe_cli_lib_resume_prom_gather_and_display_info(fbe_esp_base_env_get_resume_prom_info_cmd_t *pGetResumePromInfo);
void fbe_cli_lib_resume_prom_gather_and_display_iom_info(fbe_esp_base_env_get_resume_prom_info_cmd_t *pGetResumePromInfo);
void fbe_cli_lib_resume_prom_display_all_enclosure_resume(fbe_u32_t bus, fbe_u32_t encl);

/* Usage*/
#define FORCE_RP_READ_USAGE                               \
" forcerpread/frr  - Forces resume prom reading." \
" Usage:  \n"\
"       forcerpread -h\n"\
"       forcerpread -b<bus no> -e<enclosure no.> -s<slot no.> "\
"-d <device type> -sp <sp id(0|1)>\n"\
"       device type: midplane, lcc, fan, iom, ps, mezz, mgmt, sp, drivemidplane, bbu, bm, cachecard\n"\
"       Example:\n"\
"       forcerpread -b 0 -e 0 -s 0 -d ps -sp 0\n"



#endif

