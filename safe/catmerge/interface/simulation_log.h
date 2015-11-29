#ifndef _SIMULATION_LOG_H_
#define _SIMULATION_LOG_H_

#include <stdarg.h>
#include "csx_ext.h"

/*
 * Enum for output stream type.
*/
typedef enum
{
    SIMULATION_LOG_DESTINATION_CONSOLE,
    SIMULATION_LOG_DESTINATION_FILE,
    SIMULATION_LOG_DESTINATION_LAST
}simulation_log_destination_t;

#define TIME_DATE_STAMP_SIZE 20

/*
 * Pointer to function that implements simulation log formatted output.
 */
typedef void (*simulation_log_formatter_t)(char *buffer, const char *content);
typedef void (*suite_log_callback_func_t)(const char *log_path, const char *log_file_name);
typedef void (*simulation_log_trace_func_t)(const char *format, ...) __attribute__((format(__printf_func__,1,2)));
typedef void (*simulation_log_set_formatter_func_t)(simulation_log_formatter_t func);
typedef void (*simulation_log_set_name_prefix_func_t)(const char *prefix);
typedef void (*simulation_log_set_common_name_func_t)(const char *common_name);
typedef void (*simulation_log_init_func_t)(void);
typedef void (*simulation_get_timedate_stamp_func_t)(char *, int bufferSize);
typedef void (*simulation_log_destroy_func_t)(void);
typedef void (*simulation_log_add_destination_func_t)(simulation_log_destination_t destination);
typedef void (*simulation_log_remove_destination_func_t)(simulation_log_destination_t destination);
typedef void (*simulation_log_set_name_suffix_func_t)(const char *suffix);
typedef void (*simulation_log_get_name_suffix_func_t)(char *buffer, int buffersize);
typedef suite_log_callback_func_t (*simulation_log_get_suite_log_callback_func_func_t)(void);
typedef void (*simulation_log_set_logdir_func_t)(char *log_path);

/*
 * Init function.
 */
CSX_MOD_EXPORT
void __cdecl simulation_log_init(void);

/*
 * Get timestamp.
 */
CSX_MOD_EXPORT
void __cdecl simulation_get_timedate_stamp(char *, int bufferSize);

/*
 * Destroy function.
 */
CSX_MOD_EXPORT
void __cdecl simulation_log_destroy(void);

/*
 * Print function.
 */
CSX_MOD_EXPORT
void __cdecl simulation_log_trace(const char *format, ...) __attribute__((format(__printf_func__,1,2)));

/*
 * Set simulation log converter.
 */
CSX_MOD_EXPORT
void __cdecl simulation_log_set_formatter(simulation_log_formatter_t func);

/*
 * Set log file name.
 */
CSX_MOD_EXPORT
void __cdecl simulation_log_set_name_prefix(const char *prefix);

/*
 * Set suffix of the name.
 */
CSX_MOD_EXPORT
void __cdecl simulation_log_set_name_suffix(const char *suffix);

/*
 * Get suffix of the name.
 */
CSX_MOD_EXPORT
void __cdecl simulation_log_get_name_suffix(char *buffer, int buffersize);

/*
 * Add simulation log output destination.
 */
CSX_MOD_EXPORT
void __cdecl simulation_log_add_destination(simulation_log_destination_t destination);

/*
 * Remove simulation log output destination.
 */
CSX_MOD_EXPORT
void __cdecl simulation_log_remove_destination(simulation_log_destination_t destination);

/*
 * Return a pointer to log suite callback function
 */
CSX_MOD_EXPORT
suite_log_callback_func_t __cdecl simulation_log_get_suite_log_callback_func(void);

/*
 * Set current log name
 */
CSX_MOD_EXPORT
void __cdecl simulation_log_set_logdir(char *log_path);
#endif // _SIMULATION_LOG_H_
