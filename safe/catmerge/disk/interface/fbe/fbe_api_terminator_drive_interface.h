#ifndef FBE_API_TERMINATOR_DRIVE_INTERFACE_H
#define FBE_API_TERMINATOR_DRIVE_INTERFACE_H

#include "fbe/fbe_types.h"

#if defined(__cplusplus)
#define FBE_API_TERMINATOR_DRIVE_INTERFACE_CALL __stdcall
#else
#define FBE_API_TERMINATOR_DRIVE_INTERFACE_CALL
#endif

/*!**********************************************************************
 * @enum fbe_terminator_drive_select_e
 *  
 *  @brief Determines how the user want's to select the drive
 *
 *  @note Typically these flags should only be set for unit or
 *        intergration testing since they could have SEVERE 
 *        side effects.
 *
*************************************************************************/
typedef enum fbe_terminator_drive_select_e
{
    FBE_TERMINATOR_DRIVE_SELECT_ALL_DRIVES          = 0x0000ffff,
    FBE_TERMINATOR_DRIVE_SELECT_NO_DRIVES           = 0x00eeeeee,

} fbe_terminator_drive_select_t;

/*!**********************************************************************
 * @enum fbe_terminator_drive_select_type_e
 *  
 *  @brief Determines how the user want's to select the drive
 *
 *  @note Typically these flags should only be set for unit or
 *        intergration testing since they could have SEVERE 
 *        side effects.
 *
*************************************************************************/
typedef enum fbe_terminator_drive_select_type_e
{
    FBE_TERMINATOR_DRIVE_SELECT_TYPE_INVALID            =   0x00000000,

    /*! All drives 
     */
    FBE_TERMINATOR_DRIVE_SELECT_TYPE_ALL_DRIVES         =   0x00000001,

    /*! Select by fru
     */
    FBE_TERMINATOR_DRIVE_SELECT_TYPE_TERM_DRIVE_INDEX   =   0x00000002,

    /*! Select by bus/encl/slot
     */
    FBE_TERMINATOR_DRIVE_SELECT_TYPE_BUS_ENCL_SLOT      =   0x00000004,

    /*! Select terminator drive index range 
     */
    FBE_TERMINATOR_DRIVE_SELECT_TYPE_TERM_DRIVE_RANGE   =   0x00000008,

    FBE_TERMINATOR_DRIVE_SELECT_TYPE_NEXT               =   0x00000010,


} fbe_terminator_drive_select_type_t;

/*!**********************************************************************
 * @enum fbe_terminator_drive_debug_flags_t
 *  
 *  @brief Debug flags that allow run-time debug functions for terminator
 *         drive object.
 *
 *  @note Typically these flags should only be set for unit or
 *        intergration testing since they could have SEVERE 
 *        side effects.
 *
*************************************************************************/
typedef enum fbe_terminator_drive_debug_flags_e
{
    FBE_TERMINATOR_DRIVE_DEBUG_FLAG_NONE                =   0x00000000,

    /*! All I/O traffic tracing
     */
    FBE_TERMINATOR_DRIVE_DEBUG_FLAG_ALL_TRAFFIC         =   0x00000001,

    /*! Trace request completion (by default only the request start is traced).
     */
    FBE_TERMINATOR_DRIVE_DEBUG_FLAG_TRACE_END           =   0x00000002,

    /*! @note Trace the first and last 16-bytes of first and last blocks. 
     *        (FBE_TERMINATOR_DRIVE_DEBUG_FLAG_TRACE_END must be set)
     */
    FBE_TERMINATOR_DRIVE_DEBUG_FLAG_TRACE_DATA          =   0x00000004,

    /*! Zero traffic
     */
    FBE_TERMINATOR_DRIVE_DEBUG_FLAG_ZERO_TRAFFIC        =   0x00000008,

    /*! Write traffic
     */
    FBE_TERMINATOR_DRIVE_DEBUG_FLAG_WRITE_TRAFFIC       =   0x00000010,

    /*! Read traffic
     */
    FBE_TERMINATOR_DRIVE_DEBUG_FLAG_READ_TRAFFIC        =   0x00000020,

    /*! Journal record tracing
     */
    FBE_TERMINATOR_DRIVE_DEBUG_FLAG_JOURNAL_TRACE       =   0x00000040,

    /*! @note Terminator I/O tracing for the system drives is disabled by 
     *        default.  This flag enables system drive tracing.
     */
    FBE_TERMINATOR_DRIVE_DEBUG_FLAG_TRACE_SYSTEM_DRIVES =   0x00000080,

    /*! If set dump the first block of the paged metadata on writes. 
     *  (FBE_TERMINATOR_DRIVE_DEBUG_FLAG_TRACE_END  and ..
     *   FBE_TERMINATOR_DRIVE_DEBUG_FLAG_TRACE_DATA must be set)
     */
    FBE_TERMINATOR_DRIVE_DEBUG_FLAG_METADATA_WRITE_DUMP =   0x00000100,

    FBE_TERMINATOR_DRIVE_DEBUG_FLAG_NEXT                =   0x00000200,

    /* Mask of maximum flag value.
     */
    FBE_TERMINATOR_DRIVE_DEBUG_MAX_FLAGS                =   (FBE_TERMINATOR_DRIVE_DEBUG_FLAG_NEXT - 1)


} fbe_terminator_drive_debug_flags_t;

/*Add any drive prototype and functions here*/
fbe_status_t FBE_API_TERMINATOR_DRIVE_INTERFACE_CALL
fbe_api_terminator_set_simulated_drive_debug_flags(fbe_terminator_drive_select_type_t drive_select_type,
                                                   fbe_u32_t first_term_drive_index,
                                                   fbe_u32_t last_term_drive_index,
                                                   fbe_u32_t backend_bus_number,
                                                   fbe_u32_t encl_number,
                                                   fbe_u32_t slot_number,
                                                   fbe_terminator_drive_debug_flags_t terminator_drive_debug_flags);
fbe_status_t FBE_API_CALL fbe_api_terminator_drive_disable_compression(void);

#endif /*FBE_API_TERMINATOR_DRIVE_INTERFACE_H*/


