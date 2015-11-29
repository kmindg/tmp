#ifndef FBE_DBGEXT_H
#define FBE_DBGEXT_H
#include "fbe_trace.h"
#include "csx_ext_dbgext.h"

#ifndef I_AM_DEBUG_EXTENSION
#error "fbe_dbgext.h should only be included for code being built for TARGETTYPE CSX_DEBUG_EXTENSION or CSX_DEBUG_EXTENSION_LIBRARY"
#endif

extern csx_bool_t globalCtrlC;
#define CHECK_CONTROL_C() ((csx_dbg_ext_was_abort_requested() || globalCtrlC) ? (globalCtrlC = 1) : 0)

/* Used to determine if control-c was pressed by the user.
 */
extern int pp_debug_check_control_c(void);
 
/*!************************************************** 
 * FBE_CHECK_CONTROL_C()
 **************************************************** 
 * @brief
 *  We should add calls to this macro within every
 *  debug macro loop. This allows us to break out of
 *  loops gracefully when the user hits a control-c.
 ***************************************************/
#define FBE_CHECK_CONTROL_C()\
(pp_debug_check_control_c())

	/* Macro to get offset for a particular field. 
	 */ 
	#define FBE_GET_FIELD_OFFSET(_module, _type, _field, _offset_p) \
	{   											\
			char type[128]; 							\
			_snprintf(type, 128, "%s!%s", _module, #_type); 	\
			csx_dbg_ext_get_field_offset(type, _field, _offset_p);	\
	}

	#define FBE_GET_FIELD_OFFSET_WITHOUT_MODULE_NAME(_type, _field, _offset_p) \
	{   											\
			char type[128]; 							\
			_snprintf(type, 128, "%s", #_type); 	\
			csx_dbg_ext_get_field_offset(type, _field, _offset_p);	\
	}

	#define FBE_GET_FIELD_STRING_OFFSET(_module, _type_string, _field, _offset_p) \
	{   											\
			char type[128]; 							\
			_snprintf(type, 128, "%s!%s", _module, _type_string); 	\
			csx_dbg_ext_get_field_offset(type, _field, _offset_p);	\
	}

	#define FBE_GET_FIELD_DATA(_module, _addr, _type, _field, _size, _ptr)	\
    {																	\
			char expr[128]; 							\
			_snprintf(expr, 128, "%s!%s", _module, #_type); 	\
			csx_dbg_ext_read_in_field((ULONG64)_addr, expr, #_field, _size, _ptr);		\
    }

    #define FBE_GET_FIELD_DATA_WITHOUT_MODULE_NAME(_addr, _type, _field, _size, _ptr)	\
		{																	\
			csx_dbg_ext_read_in_field((ULONG64)_addr, #_type, #_field, _size, _ptr);		\
		}
	#define FBE_GET_EXPRESSION(_module, _type, _out)	\
		{												\
			*( _out )= csx_dbg_ext_lookup_symbol_address(_module, #_type);				\
		}
	#define FBE_GET_STRING_EXPRESSION(_module, _type_string, _out)	\
		{												\
			*( _out )= csx_dbg_ext_lookup_symbol_address(_module, _type_string);				\
		}

    #define FBE_GET_EXPRESSION_WITHOUT_MODULE_NAME(_type, _out)	\
		{												\
			*( _out )= csx_dbg_ext_lookup_symbol_address("", #_type);				\
		}

    #define FBE_GET_TYPE_SIZE(_module, _type, _out)		\
		{												\
			char expr[128]; \
			_snprintf(expr,128,"%s!%s", _module, #_type);		\
			*( _out ) =  (fbe_u32_t)csx_dbg_ext_get_type_size(expr);				\
		}

    #define FBE_GET_TYPE_SIZE_WITHOUT_MODULE_NAME(_type, _out)		\
		{												\
			char expr[128]; \
			_snprintf(expr,128,"%s", #_type);		\
			*( _out )=  (fbe_u32_t)csx_dbg_ext_get_type_size(expr);				\
		}

	#define FBE_GET_TYPE_STRING_SIZE(_module, _type_string, _out)		\
		{												\
			char expr[128]; \
			_snprintf(expr,128,"%s!%s", _module, _type_string);		\
			*( _out )=  (fbe_u32_t)csx_dbg_ext_get_type_size(expr);				\
		}

	#define FBE_GET_TYPE_PTR_SIZE(_module, _type, _out)		\
		{												\
			char expr[128];								\
			_snprintf(expr,128,"%s!%s *", _module, #_type);		\
			*( _out )=  (fbe_u32_t)csx_dbg_ext_get_type_size(expr);				\
		}
    #define FBE_READ_MEMORY(_source, _destination, _size)\
    {\
        if(CSX_STATUS_SUCCESS != csx_dbg_ext_read_in_len(_source, _destination, _size))\
        {\
            csx_dbg_ext_print("csx_dbg_ext_read_in_len at %llx  failed line: %d func: %s\n",_source, __LINE__, __FUNCTION__);\
        }\
    }
    #define FBE_CHECK_CTRL_C()((ExtensionApis.lpCheckControlCRoutine)())

/*!*******************************************************************
 * @def FBE_DEBUG_MAX_FIELD_NAME_CHARS
 *********************************************************************
 * @brief Max Len for the length of a name to specify the
 *        name of a structure/field symbol
 *
 *********************************************************************/
#define FBE_DEBUG_MAX_FIELD_NAME_CHARS 50

/*!*******************************************************************
 * @def FBE_DEBUG_MAX_FIELD_SPECIFIER_CHARS
 *********************************************************************
 * @brief The max length of a format string.
 *
 *********************************************************************/
#define FBE_DEBUG_MAX_FIELD_SPECIFIER_CHARS 50

/*!*******************************************************************
 * @typedef fbe_debug_display_field_fn_t
 *********************************************************************
 * @brief Function used to display a single field of
 *        a fbe_debug_field_info_t
 *
 *********************************************************************/
struct fbe_debug_field_info_s;
typedef fbe_status_t (*fbe_debug_display_field_fn_t)(const fbe_u8_t * module_name,
                                                     fbe_dbgext_ptr object_p,
                                                     fbe_trace_func_t trace_func,
                                                     fbe_trace_context_t trace_context,
                                                     struct fbe_debug_field_info_s *field_info_p,
                                                     fbe_u32_t spaces_to_indent);

/*!*******************************************************************
 * @typedef fbe_debug_display_object_fn_t
 *********************************************************************
 * @brief Function used to display an entire object.
 *        This gets passed into the routine that displays a queue of objects.
 *
 *********************************************************************/
typedef fbe_status_t (*fbe_debug_display_object_fn_t)(const fbe_u8_t * module_name,
                                                      fbe_dbgext_ptr object_ptr,
                                                      fbe_trace_func_t trace_func,
                                                      fbe_trace_context_t trace_context,
                                                      fbe_u32_t spaces_to_indent);
/*!*************************************************
 * @typedef fbe_debug_queue_info_t
 ***************************************************
 * @brief Information for displaying this queue.
 *
 ***************************************************/
typedef struct fbe_debug_queue_info_s
{
    /* Type of object on the queue.
     */
    fbe_u8_t type_name[FBE_DEBUG_MAX_FIELD_NAME_CHARS];
    /* Name of the queue element in the above type used by this queue.
     */
    fbe_u8_t queue_element_name[FBE_DEBUG_MAX_FIELD_NAME_CHARS];

    fbe_debug_display_object_fn_t display_function;    /*!< Display fn for elements of queue. */

    fbe_bool_t b_first_field; /*!< TRUE if the queue element is the first field in this object. */
    fbe_u32_t queue_element_offset; /*!< Offset of queue element from start of object. */
}
fbe_debug_queue_info_t;

/*!*************************************************
 * @typedef fbe_debug_field_info_t
 ***************************************************
 * @brief Information for displaying this field.
 *
 ***************************************************/
typedef struct fbe_debug_field_info_s
{
    /*! Chars for the symbol name.
     */
    fbe_u8_t name[FBE_DEBUG_MAX_FIELD_NAME_CHARS];

    /* Type of object.
     */
    fbe_u8_t type_name[FBE_DEBUG_MAX_FIELD_NAME_CHARS];

    fbe_bool_t b_first_field;    /*! Offset is always zero for first field for validation.  */

    /*! Chars for the output specifier.  
     * Output will be "%s: %s",  name, ouput_format_specifier
     */
    fbe_u8_t output_format_specifier[FBE_DEBUG_MAX_FIELD_SPECIFIER_CHARS];

    fbe_debug_display_field_fn_t display_function;    /*!< Optional display fn. */
    fbe_debug_queue_info_t *queue_info_p; /*!< Optional queue display info. */
    fbe_u32_t size;    /*!< Size of this field. */
    fbe_u32_t offset;    /*!< Offset within the containing struct. */
}
fbe_debug_field_info_t;

/*!*************************************************
 * @typedef fbe_debug_struct_info_t
 ***************************************************
 * @brief Information used for displaying this struct.
 *
 ***************************************************/
typedef struct fbe_debug_struct_info_s
{
    fbe_char_t name[FBE_DEBUG_MAX_FIELD_NAME_CHARS];
    fbe_u32_t field_count;    /*!< Count of fields we are interested in. */
    fbe_debug_field_info_t *field_info_array;
    fbe_u32_t size;
    fbe_bool_t b_initialized;
}
fbe_debug_struct_info_t;

/*!*******************************************************************
 * @def FBE_DEBUG_DECLARE_QUEUE_INFO
 *********************************************************************
 * @brief This defines a struct info structure for a given
 *        type of structure.
 *
 *********************************************************************/
#define FBE_DEBUG_DECLARE_QUEUE_INFO(m_object_name, m_queue_element_name, m_b_first_field, m_struct_name, m_display_fn)\
fbe_debug_queue_info_t m_struct_name =                      \
{                                                           \
    #m_object_name, m_queue_element_name, m_display_fn, m_b_first_field, 0   \
}
/*!*******************************************************************
 * @def FBE_DEBUG_DECLARE_STRUCT_INFO
 *********************************************************************
 * @brief This defines a struct info structure for a given
 *        type of structure.
 *
 *********************************************************************/
#define FBE_DEBUG_DECLARE_STRUCT_INFO(m_name, m_struct_name, m_field_info_array)\
fbe_debug_struct_info_t m_struct_name =                 \
{                                                       \
    #m_name, 0, m_field_info_array, 0, 0,   \
}
/*!*******************************************************************
 * @def FBE_DEBUG_DECLARE_FIELD_INFO
 *********************************************************************
 * @brief This defines a single entry of a field entry array.
 *
 *********************************************************************/
#define FBE_DEBUG_DECLARE_FIELD_INFO(m_name, m_type_name, m_b_first_field, m_format_specifier)\
{                                                                          \
    m_name, #m_type_name, m_b_first_field, m_format_specifier, 0, 0, sizeof(m_type_name), 0  \
}
/*!*******************************************************************
 * @def FBE_DEBUG_DECLARE_FIELD_INFO_FN
 *********************************************************************
 * @brief This defines a single entry of a field entry array
 *        Where we will use a function to display the field.
 *
 *********************************************************************/
#define FBE_DEBUG_DECLARE_FIELD_INFO_FN(m_name, m_type_name, \
                                           m_b_first_field, m_format_specifier,\
                                           m_fn)\
{                                                                             \
    m_name, #m_type_name, m_b_first_field, m_format_specifier, m_fn, 0, sizeof(m_type_name), 0  \
}

/*!*******************************************************************
 * @def FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR
 *********************************************************************
 * @brief This defines a termination field info entry
 *
 *********************************************************************/
#define FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR()\
{                                              \
    "", "", FBE_FALSE, "", 0, 0, 0, 0          \
}
/*!*******************************************************************
 * @def FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE
 *********************************************************************
 * @brief This tells us to start the next entry on the next line.
 *
 *********************************************************************/
#define FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE()     \
{                                                 \
    "newline", "newline", FBE_FALSE, "", 0, 0, 0, 0 \
}
/*!*******************************************************************
 * @def FBE_DEBUG_DECLARE_FIELD_INFO_QUEUE
 *********************************************************************
 * @brief This defines a single entry of a field entry array, which
 *        happens to be a queue.
 *
 *********************************************************************/
#define FBE_DEBUG_DECLARE_FIELD_INFO_QUEUE(m_name, m_type_name, m_b_first_field, m_queue_info_p)\
{                                                                             \
    m_name, #m_type_name, m_b_first_field, "", NULL, m_queue_info_p, 0, 0  \
}

/*!*******************************************************************
 * @enum fbe_debug_display_options_t
 *********************************************************************
 * @brief Enum of options to discribe how we want to display.
 *
 *********************************************************************/
typedef enum fbe_debug_display_options_e
{
    FBE_DEBUG_DISPLAY_OPTIONS_INVALID = 0,
    FBE_DEBUG_DISPLAY_OPTIONS_NONE           = 0x1,
    FBE_DEBUG_DISPLAY_OPTIONS_NO_FIELD_NAMES = 0x2,
    FBE_DEBUG_DISPLAY_OPTIONS_LAST,
}
fbe_debug_display_options_t;

/*!*******************************************************************
 * @struct fbe_debug_enum_trace_info_t
 *********************************************************************
 * @brief Represents a flag and the string representation of the flag.
 *
 *********************************************************************/
typedef struct fbe_debug_flag_trace_info_s
{
    fbe_u32_t flag;
    fbe_char_t *string_p;
}
fbe_debug_enum_trace_info_t;

/*!*******************************************************************
 * @struct fbe_dump_debug_trace_prefix_t
 *********************************************************************
 * @brief represent a prefix used for tracing functions print .
 * @author 08/20/2012 created. Jingcheng Zhang
 *********************************************************************/
#define FBE_MAX_DUMP_DEBUG_PREFIX_LEN       256
typedef struct fbe_dump_debug_trace_prefix_s
{
    char prefix[FBE_MAX_DUMP_DEBUG_PREFIX_LEN];
} fbe_dump_debug_trace_prefix_t;

/*!*******************************************************************
 * @def FBE_DEBUG_ENUM_TRACE_FIELD_INFO
 *********************************************************************
 * @brief Input is an enumeration and it creates an entry in a table
 *        of fbe_debug_enum_trace_info_t.
 *
 *********************************************************************/
#define FBE_DEBUG_ENUM_TRACE_FIELD_INFO(m_name){m_name, #m_name}

/*!*******************************************************************
 * @def FBE_DEBUG_ENUM_TRACE_INFO_TERMINATOR
 *********************************************************************
 * @brief Simply defines the end point for an array of fbe_debug_flag_trace_info_t
 *
 *********************************************************************/
#define FBE_DEBUG_ENUM_TRACE_INFO_TERMINATOR 0, NULL

void fbe_debug_display_enum_string(fbe_trace_func_t trace_func,
                                   fbe_trace_context_t trace_context, 
                                   fbe_u32_t enum_value, 
                                   fbe_u32_t max_enum, const char **string_array );
void fbe_debug_display_flag_string(fbe_trace_func_t trace_func,
                                   fbe_trace_context_t trace_context, 
                                   fbe_u32_t flag, fbe_u32_t max_bit, const char **strArray );
void fbe_debug_set_display_queue_header(fbe_bool_t b_display);
fbe_status_t fbe_debug_display_structure(const fbe_u8_t * module_name,
                                         fbe_trace_func_t trace_func,
                                         fbe_trace_context_t trace_context, 
                                         fbe_dbgext_ptr struct_ptr,
                                         fbe_debug_struct_info_t *struct_info_p,
                                         fbe_u32_t num_per_line,
                                         fbe_u32_t spaces_to_indent);

fbe_status_t fbe_debug_display_structure_with_options(const fbe_u8_t * module_name,
                                                      fbe_trace_func_t trace_func,
                                                      fbe_trace_context_t trace_context, 
                                                      fbe_dbgext_ptr struct_ptr,
                                                      fbe_debug_struct_info_t *struct_info_p,
                                                      fbe_u32_t num_per_line,
                                                      fbe_u32_t spaces_to_indent,
                                                      fbe_debug_display_options_t options);
fbe_status_t fbe_debug_get_ptr_size(const fbe_u8_t * module_name,
                                    fbe_u32_t *ptr_size);
fbe_status_t fbe_debug_display_pointer(const fbe_u8_t * module_name,
                                       fbe_dbgext_ptr base_ptr,
                                       fbe_trace_func_t trace_func,
                                       fbe_trace_context_t trace_context,
                                       fbe_debug_field_info_t *field_info_p,
                                       fbe_u32_t spaces_to_indent);
fbe_status_t fbe_debug_display_field_pointer(const fbe_u8_t * module_name,
                                             fbe_dbgext_ptr base_ptr,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context,
                                             fbe_debug_field_info_t *field_info_p,
                                             fbe_u32_t spaces_to_indent);
fbe_status_t fbe_debug_display_function_ptr(const fbe_u8_t * module_name,
                                            fbe_dbgext_ptr base_ptr,
                                            fbe_trace_func_t trace_func,
                                            fbe_trace_context_t trace_context,
                                            fbe_debug_field_info_t *field_info_p,
                                            fbe_u32_t spaces_to_indent);
fbe_status_t fbe_debug_display_function_symbol(const fbe_u8_t * module_name,
                                               fbe_dbgext_ptr base_ptr,
                                               fbe_trace_func_t trace_func,
                                               fbe_trace_context_t trace_context,
                                               fbe_debug_field_info_t *field_info_p,
                                               fbe_u32_t spaces_to_indent);
fbe_status_t fbe_debug_display_queue(const fbe_u8_t * module_name,
                                     fbe_dbgext_ptr input_queue_head_p,
                                     fbe_debug_field_info_t *field_info_p,
                                     fbe_trace_func_t trace_func,
                                     fbe_trace_context_t trace_context,
                                     fbe_u32_t spaces_to_indent);
fbe_status_t fbe_debug_display_time(const fbe_u8_t * module_name,
                                    fbe_dbgext_ptr base_ptr,
                                    fbe_trace_func_t trace_func,
                                    fbe_trace_context_t trace_context,
                                    fbe_debug_field_info_t *field_info_p,
                                    fbe_u32_t spaces_to_indent);
fbe_status_t fbe_debug_trace_time(const fbe_u8_t * module_name,
                                  fbe_dbgext_ptr base_ptr,
                                  fbe_trace_func_t trace_func,
                                  fbe_trace_context_t trace_context,
                                  fbe_debug_field_info_t *field_info_p,
                                  fbe_u32_t spaces_to_indent);
fbe_status_t fbe_debug_trace_time_brief(const fbe_u8_t * module_name,
                                        fbe_dbgext_ptr base_ptr,
                                        fbe_trace_func_t trace_func,
                                        fbe_trace_context_t trace_context,
                                        fbe_debug_field_info_t *field_info_p,
                                        fbe_u32_t spaces_to_indent);
fbe_status_t fbe_debug_display_function_ptr_symbol(const fbe_u8_t * module_name,
                                                   fbe_dbgext_ptr state,
                                                   fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_u32_t spaces_to_indent);

fbe_status_t fbe_debug_trace_flags_strings(const fbe_u8_t * module_name,
                                                  fbe_dbgext_ptr base_ptr,
                                                  fbe_trace_func_t trace_func,
                                                  fbe_trace_context_t trace_context,
                                                  fbe_debug_field_info_t *field_info_p,
                                                  fbe_u32_t spaces_to_indent,
                                                  fbe_debug_enum_trace_info_t *flags_info_p);
fbe_status_t fbe_debug_trace_enum_strings(const fbe_u8_t * module_name,
                                          fbe_dbgext_ptr base_ptr,
                                          fbe_trace_func_t trace_func,
                                          fbe_trace_context_t trace_context,
                                          fbe_debug_field_info_t *field_info_p,
                                          fbe_u32_t spaces_to_indent,
                                          fbe_debug_enum_trace_info_t *flags_info_p);

fbe_status_t fbe_debug_dump_generate_prefix(fbe_dump_debug_trace_prefix_t *new_prefix, 
                                            fbe_dump_debug_trace_prefix_t *old_prefix,
                                            char *next_prefix);
void fbe_debug_get_idle_thread_time(const fbe_u8_t * module_name, fbe_time_t *time_p);

fbe_u8_t* fbe_get_module_name(void);
#endif /* FBE_DBGEXT_H */
