#ifndef FBE_CLI_PRIVATE_H
#define FBE_CLI_PRIVATE_H
 
#include "csx_ext.h"
#include "fbe/fbe_cli.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_enclosure.h"
#include "fbe/fbe_port.h"
#include "fbe_cli_encl_obj.h"
#include "fbe_cli_physical_drive_obj.h"
#include "fbe_cli_logical_drive_obj.h"
#include "fbe_cli_rdgen_service.h"
#include "fbe_cli_lurg.h"
#include "fbe_cli_lifecycle_state.h"
#include "fbe_cli_pvd_object.h"
#include "fbe/fbe_package.h"
#include "fbe_cli_sps_obj.h"
#include "fbe_cli_drive_mgmt_obj.h"


/*! @def FBE_CLI_MAX_TRACE_CHARS  
 *  @brief This is the max number of chars we will try to print out with
 *         our display function.
 */
#define FBE_CLI_MAX_TRACE_CHARS 256
#define CMD_MAX_ARGC 400 /* Max number of command line arguments. */
#define CMD_MAX_ARG_SIZE 20 /* Max number of chars per argument. */
#define CMD_MAX_SIZE CMD_MAX_ARGC*CMD_MAX_ARG_SIZE

/*! @typedef 
 *  @brief This definition makes it easier to define functions that can be used
 *         to operate against a single object.  We can use this function when
 *         looping across a range of objects also.
 */
typedef fbe_status_t (* fbe_cli_command_fn)(fbe_u32_t context1, fbe_u32_t context2, fbe_package_id_t package_id);

/*! @typedef enum 
 *  @brief Enum with the possible ordering of pdo commands
 */
typedef enum fbe_cli_pdo_order_enum
{
	ORDER_UNDEFINED,
	OBJECT_ORDER,
	PHYSICAL_ORDER
} fbe_cli_pdo_order_t;

/* This function allows the operator to execute commands on object or physical order, calling 
 * fbe_cli_execute_command_for_objects or fbe_cli_execute_command_for_objects_physical_order.
 */
fbe_status_t fbe_cli_execute_command_for_objects_order_defined(fbe_class_id_t filter_class_id,
                                                               fbe_package_id_t package_id,
															   fbe_u32_t port,
                                                               fbe_u32_t enclosure,
                                                               fbe_u32_t slot,
															   fbe_cli_pdo_order_t cmd_order,
                                                               fbe_cli_command_fn command_function_p,
                                                               fbe_u32_t context);

/* This function uses the above in order to allow iterating over a single class 
 * or all classes. 
 */
fbe_status_t fbe_cli_execute_command_for_objects(fbe_class_id_t filter_class_id,
                                                 fbe_package_id_t package_id,
                                                 fbe_u32_t port,
                                                 fbe_u32_t enclosure,
                                                 fbe_u32_t slot,
                                                 fbe_cli_command_fn command_function_p,
                                                 fbe_u32_t context);

/*  This function will iterate over some set of objects
 *  specified by an optional filter based on class, port, enclosure and slot.
 *  Results are ordered on port-enclosure-slot fashion.
 */
fbe_status_t fbe_cli_execute_command_for_objects_physical_order(fbe_class_id_t filter_class_id,
                                                                fbe_package_id_t package_id,
                                                                fbe_u32_t filter_port,
                                                                fbe_u32_t filter_enclosure,
                                                                fbe_u32_t filter_slot,
                                                                fbe_cli_command_fn command_function_p,
                                                                fbe_u32_t context);

void fbe_cli_trace_func(fbe_trace_context_t trace_context, const char * fmt, ...);
fbe_status_t fbe_cli_set_trace_level(fbe_u32_t object_id, fbe_u32_t context, fbe_package_id_t package_id);
fbe_status_t fbe_cli_get_class_info(fbe_u32_t object_id,
                                    fbe_package_id_t package_id,
                                    fbe_const_class_info_t **class_info_p);
fbe_status_t fbe_cli_get_bus_enclosure_slot_info(fbe_object_id_t object_id,
                                                 fbe_class_id_t class_id,
                                                 fbe_port_number_t *port_p,
                                                 fbe_enclosure_number_t *enc_p,
                                                 fbe_enclosure_slot_number_t *slot_p,
                                                 fbe_package_id_t package_id);

fbe_status_t fbe_cli_set_write_uncorrectable(fbe_u32_t object_id, fbe_lba_t lba);
fbe_status_t fbe_cli_create_read_uncorrectable(fbe_u32_t object_id, fbe_lba_t lba);
fbe_status_t fbe_cli_create_read_unc_range(fbe_u32_t object_id, fbe_lba_t start_lba, fbe_lba_t end_lba, fbe_lba_t inc_lba);


void fbe_cli_cmd_print_usage(int argc , char ** argv);
void fbe_cli_cmd_list(int argc, char ** argv);
void fbe_cli_cmd_object_classes(int argc , char ** argv);
void fbe_cli_cmd_display_encl_data(int argc , char ** argv);
void fbe_cli_display_board_info(int argc, char** argv);
void fbe_cli_reboot_sp(int argc, char** argv);


/* Functions defined in fbe_cli_src_utils.c */ 
void fbe_cli_cmd_base_object(int argc , char ** argv);
void fbe_cli_cmd_base_config(int argc , char ** argv);
void fbe_cli_cmd_trace(int argc, char** argv);
void fbe_cli_x_run_script(int argc , char ** argv);
void fbe_cli_x_access(int argc , char ** argv);
fbe_u32_t fbe_atoh(fbe_char_t * s_p);          /* For use in NEIT and Rdgen*/
fbe_status_t  fbe_atoh64(fbe_char_t * s_p,
                         fbe_u64_t * result);

void fbe_cli_sleep_at_prompt(fbe_s32_t argc, fbe_s8_t ** argv);
fbe_char_t * fbe_cli_lib_SP_ID_to_string(SP_ID spid);
void fbe_cli_cmd_proactive_spare(int argc , char ** argv);
fbe_u32_t fbe_atoi(fbe_u8_t * pData);
char * fbe_cli_convert_metadata_element_state_to_string(fbe_metadata_element_state_t state);
void fbe_cli_display_metadata_element_state(fbe_object_id_t object_id);
void fbe_cli_display_sfp_media_type(fbe_port_sfp_media_type_t media_type);
void fbe_cli_display_sfp_condition_type(fbe_port_sfp_condition_type_t condition_type);
void fbe_cli_display_sfp_sub_condition_type(fbe_port_sfp_sub_condition_type_t sub_condition_type);

void fbe_cli_get_speed_string(fbe_u32_t speed, char **stringBuf);

void fbe_cli_decodeElapsedTime(fbe_u32_t elapsedTimeInSecs, fbe_system_time_t *decodedTime);
fbe_u32_t fbe_cli_convertStringToComponentType(char **componentString);

fbe_status_t fbe_cli_strtoui64(const char *str, fbe_u64_t *value);
fbe_status_t fbe_cli_strtoui32(const char *str, fbe_u32_t *value);
fbe_status_t fbe_cli_strtoui16(const char *str, fbe_u16_t *value);
fbe_status_t fbe_cli_strtoui8(const char *str, fbe_u8_t *value);

extern int  operating_mode; // USER_MODE;
extern fbe_bool_t cmd_not_executed;		/* TRUE - command found */

#define ENG_MODE               1
#define USER_MODE              2



#define LIST_USAGE  "\
list - display various lists.\n\
usage:  list -help                          - display this help information.\n\
        list -trace                         - display trace levels.\n\
        list -classes                       - display a list of classes.\n\
        list -objects                       - display a list of objects.\n\
        list -services                      - display a list of services.\n\
        list -libraries                     - display a list of libraries.\n\
"
#define TRACE_USAGE  "\
trace - display and change trace levels.\n\
usage:trace -help                  - display this help information.\n\
      trace -default               - display (or change) the default trace level.\n\
      trace -set_trace [level]     - change the trace level \n\
      trace -object <object_id>    - display (or change) the trace level of an object.\n\
      trace -service <service_id>  - display (or change) the trace level of a service.\n\
      trace -library <library_id>  - display (or change) the trace level of a library.\n\
      trace -package<package name> - command issued for this package<valid packages are phy,sep,esp>.\n\
      trace -default_level (level) - Change the default trace level for the cli.\n\
examples:\n\
  trace -default -package phy   (displays the default trace level\n\
  trace -default -set_trace 2 -p sep   (sets the default trace level to \"2\"\n\
  trace -object 1e -p esp   (displays the trace level of the object that has the id \"1e\")\n\
  trace -object 1e -st 6 -p phy   (sets the trace level of the object to level \"6\")\n\
"
#define BASE_OBJECT_USAGE  "\
base_object - Display and change base object attributes.\n\
usage:  base_object -help                   - For this help information.\n\
        base_object -object object_id - Target a single object with the command\n\
            -all                    - Issue command to all objects.\n\
            -class class_id         - Issue command to all objects of this class.\n\
            -display                - Display table output of base object attributes.\n\
            -trace_level level      - Change the trace level to a new value.\n\
            -set_state state        - Set the lifecycle state of the object.\n\
            -package<package name> - command issued for this package<valid packages are phy,sep,esp>.\n\
examples:\n\
    base_object -all -display -p phy     (displays a table of attributes for all physical package objects)\n\
    base_object -all -trace_level 5 -p phy  (sets trace level to 5 for all objects)          \n\
    base_object -class 0x45 -display -p Sep (displays a table of all objects with class id 0x25)\n\
    base_object -object 5 -trace_level 6 -p phy (sets object id 9 trace level to 6) \n\
    base_object -object 0x15 -set_state 4 -p phy (sets object id 0x15 state to 6) \n\
"

#define BASE_CONFIG_USAGE  "\
base_config - Display base config attributes.\n\
usage:  base_config -help                   - For this help information.\n\
        base_config -object object_id - Target a single object with the command\n\
            -class class_id         - Issue command to all objects of this class.\n\
examples:\n\
    base_config -class 0x25 (displays a table of all objects with class id 0x25)\n\
    base_config -object 5 (displays a table of object 5) \n\
"
#endif /* FBE_CLI_PRIVATE_H */
