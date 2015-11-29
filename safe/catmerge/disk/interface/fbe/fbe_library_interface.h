#ifndef FBE_LIBRARY_H
#define FBE_LIBRARY_H

#include "csx_ext.h"
#include "fbe/fbe_trace_interface.h"

typedef enum fbe_library_id_e{
    FBE_LIBRARY_ID_INVALID,

    FBE_LIBRARY_ID_LIFECYCLE,
    FBE_LIBRARY_ID_FLARE_SHIM,
    FBE_LIBRARY_ID_ENCL_DATA_ACCESS,
    FBE_LIBRARY_ID_FBE_API,
    FBE_LIBRARY_ID_FBE_NEIT_API,
    FBE_LIBRARY_ID_FBE_DEST_API,
    FBE_LIBRARY_ID_PMC_SHIM,
    FBE_LIBRARY_ID_DRIVE_STAT,
	FBE_LIBRARY_ID_FIS,
	FBE_LIBRARY_ID_FBE_SEP_SHIM,
	FBE_LIBRARY_ID_CPD_SHIM,
    FBE_LIBRARY_ID_DRIVE_SPARING,
    FBE_LIBRARY_ID_XOR,
    FBE_LIBRARY_ID_RAID,
    FBE_LIBRARY_ID_REGISTRY,
    FBE_LIBRARY_ID_DATA_PATTERN,
	FBE_LIBRARY_ID_SYSTEM_LIMITS,
	FBE_LIBRARY_ID_MULTICORE_QUEUE,
    FBE_LIBRARY_ID_PACKET_SERIALIZE,
    FBE_LIBRARY_ID_LAST
} fbe_library_id_t;

typedef const struct fbe_const_library_info {
    const char * library_name;
    fbe_library_id_t library_id;
} fbe_const_library_info_t;

void fbe_base_library_trace(fbe_library_id_t library_id,
                            fbe_trace_level_t trace_level,
                            fbe_trace_message_id_t message_id,
                            const fbe_char_t * fmt, ...)
                            __attribute__((format(__printf_func__,4,5)));

fbe_status_t fbe_get_library_by_id(fbe_library_id_t library_id, fbe_const_library_info_t ** pp_library_info);
fbe_status_t fbe_get_library_by_name(const char * library_name, fbe_const_library_info_t ** pp_library_info);

#endif /* FBE_LIBRARY_H */
