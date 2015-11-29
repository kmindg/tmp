#ifndef FBE_TRAFFIC_TRACE_H
#define FBE_TRAFFIC_TRACE_H

#ifdef __cplusplus
/*
 * all of these functions need standard C linkage.  Otherwise, they can not be referenced from C.
 * Note, thee implementation of these functions is within a cpp file, which allows these functions to
 * access C++ instances, instance methods and class methods
 */
extern "C" {
#endif
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_traffic_trace_interface.h"
#include "fbe/fbe_payload_block_operation.h"
#include "fbe/fbe_payload_cdb_operation.h"
#include "fbe/fbe_transport.h"
#include "ktrace_structs.h"

fbe_status_t fbe_traffic_trace_init(void);
fbe_status_t fbe_traffic_trace_destroy(void);
fbe_status_t fbe_traffic_trace_block_operation(fbe_class_id_t class_id, 
                                           fbe_payload_block_operation_t *block_op_p, 
                                           fbe_object_id_t object_id,
                                           fbe_u64_t obj_info,/*extra information*/
                                           fbe_bool_t b_start,
                                               fbe_u16_t priority);

fbe_status_t fbe_traffic_trace_block_opcode(fbe_class_id_t class_id, 
                                            fbe_lba_t lba,
                                            fbe_block_count_t block_count,
                                            fbe_payload_block_operation_opcode_t block_opcode,
                                            fbe_u32_t target,
                                            fbe_u64_t obj_info,/*extra information*/
                                            fbe_u64_t obj_info2,
                                            fbe_bool_t b_start,
                                            fbe_u16_t priority);

void fbe_traffic_trace_get_data_crc(fbe_sg_element_t *sg_p,
                                    fbe_block_count_t blocks,
                                    fbe_u32_t offset,
                                    fbe_u64_t *trace_info_p);
fbe_u8_t fbe_traffic_trace_get_priority_from_cdb(fbe_payload_cdb_priority_t cdb_priority);
fbe_u8_t fbe_traffic_trace_get_priority(fbe_packet_t *packet_p);

/*!****************************************************************************
 * fbe_traffic_trace_is_enabled()
 ******************************************************************************
 * @brief
 *  This is the function for checking if RBA is enabled.
 *  This is inlined for performance.
 *
 * @param component_flag - traffic flag.
 *
 * @return True or False
 *
 *****************************************************************************/
static __forceinline fbe_bool_t fbe_traffic_trace_is_enabled (KTRC_tflag_T component_flag)
{
#if defined(UMODE_ENV) || defined(SIMMODE_ENV)
    fbe_bool_t fbe_traffic_trace_is_enabled_sim(KTRC_tflag_T component_flag);
    return fbe_traffic_trace_is_enabled_sim(component_flag);
#else
    return fbe_ktrace_is_rba_logging_enabled((fbe_u64_t)component_flag);
#endif
}

#ifdef __cplusplus
};
#endif

#endif /* FBE_TRAFFIC_TRACE_H */
