#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_package.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_library_interface.h"
#include "fbe/fbe_traffic_trace_interface.h"
#include "fbe_traffic_trace.h"
#include "fbe_base.h"
#include "fbe_base_service.h"
#include "fbe_service_manager.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe_traffic_trace_private.h"
#include "fbe_database.h"
#include "ktrace_structs.h"

fbe_status_t fbe_traffic_trace_control_entry(fbe_packet_t * packet);
fbe_service_methods_t fbe_traffic_trace_service_methods = {FBE_SERVICE_ID_TRAFFIC_TRACE, fbe_traffic_trace_control_entry};

typedef struct fbe_traffic_trace_service_s {
    fbe_base_service_t base_service;
} fbe_traffic_trace_service_t;

static fbe_traffic_trace_service_t traffic_trace_service;
static fbe_bool_t       fbe_rba_trace_set[FBE_TRAFFIC_TRACE_CLASS_LAST] = {FBE_FALSE};


#define FBE_RBA_TRACING_NEW_FORMAT 0

/*!***************************************************************************
 *          fbe_traffic_trace_enable()
 *****************************************************************************
 *
 * @brief   Simply enable rba traces for simulator 
 *  
 * @param   None
 *
 * @return  None
 *
 * @author
 *  03/28/2011  Swati Fursule - Created
 *
 *****************************************************************************/
void  fbe_traffic_trace_enable(fbe_traffic_trace_rba_set_class_t tag)
{
    if(tag<FBE_TRAFFIC_TRACE_CLASS_LAST)
    {
        fbe_rba_trace_set[tag] = FBE_TRUE;
    }
}
/* end of fbe_traffic_trace_enable() */

/*!***************************************************************************
 *          fbe_traffic_trace_disable()
 *****************************************************************************
 *
 * @brief   Simply disable rba traces for simulator 
 *  
 * @param   None
 *
 * @return  None
 *
 * @author
 *  03/28/2011  Swati Fursule - Created
 *
 *****************************************************************************/
void  fbe_traffic_trace_disable(fbe_traffic_trace_rba_set_class_t tag)
{
    if(tag<FBE_TRAFFIC_TRACE_CLASS_LAST)
    {
        fbe_rba_trace_set[tag] = FBE_FALSE;
    }
}
/* end of fbe_traffic_trace_disable() */

/*!***************************************************************************
 *          fbe_traffic_trace_get_setting()
 *****************************************************************************
 *
 * @brief   Simply get rba traces setting for simulator if it has been enabled or
 *          disabled.
 *  
 * @param   None
 *
 * @return  FBE_TRUE or FBE_FALSE
 *
 * @author
 *  03/28/2011  Swati Fursule - Created
 *
 *****************************************************************************/
fbe_bool_t  fbe_traffic_trace_get_setting(fbe_traffic_trace_rba_set_class_t tag)
{
    if(tag<FBE_TRAFFIC_TRACE_CLASS_LAST)
    {
        return fbe_rba_trace_set[tag];
    }
    return FBE_FALSE;
}
/* end of fbe_traffic_trace_get_setting() */

/*!***************************************************************************
 *          fbe_traffic_trace_control_enable()
 *****************************************************************************
 *
 * @brief   Enable the RBA trace
 *  
 * @param   packet
 *
 * @return  FBE_TRUE or FBE_FALSE
 *
 * @author
 *  04/10/2011  Swati Fursule - Created
 *
 *****************************************************************************/
static fbe_status_t
fbe_traffic_trace_control_enable(fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload;
    fbe_payload_control_operation_t * control_operation;
    fbe_u32_t * object_tag_p = NULL;
    fbe_payload_control_buffer_length_t buffer_length;
    fbe_status_t status;

    payload = fbe_transport_get_payload_ex(packet);
    if (payload == NULL) {
        fbe_base_service_trace((fbe_base_service_t*)&traffic_trace_service,
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s, can't get packet payload\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if (control_operation == NULL) {
        fbe_base_service_trace((fbe_base_service_t*)&traffic_trace_service,
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s, can't get control operation\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_payload_control_get_buffer(control_operation, (fbe_u32_t*)&object_tag_p);
    if (status != FBE_STATUS_OK) {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    buffer_length = 0;
    fbe_payload_control_get_buffer_length(control_operation, &buffer_length); 
    if (buffer_length != sizeof(fbe_u32_t)) {
        fbe_base_service_trace((fbe_base_service_t*)&traffic_trace_service,
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s, can't get control operation buffer size\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_traffic_trace_enable(*object_tag_p);

    if (status != FBE_STATUS_OK) {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }
    else {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/*!***************************************************************************
 *          fbe_traffic_trace_control_disable()
 *****************************************************************************
 *
 * @brief   Disable the RBA trace
 *  
 * @param   packet
 *
 * @return  FBE_TRUE or FBE_FALSE
 *
 * @author
 *  04/10/2011  Swati Fursule - Created
 *
 *****************************************************************************/
static fbe_status_t
fbe_traffic_trace_control_disable(fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload;
    fbe_payload_control_operation_t * control_operation;
    fbe_u32_t * object_tag_p = NULL;
    fbe_payload_control_buffer_length_t buffer_length;
    fbe_status_t status;

    payload = fbe_transport_get_payload_ex(packet);
    if (payload == NULL) {
        fbe_base_service_trace((fbe_base_service_t*)&traffic_trace_service,
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s, can't get packet payload\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if (control_operation == NULL) {
        fbe_base_service_trace((fbe_base_service_t*)&traffic_trace_service,
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s, can't get control operation\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_payload_control_get_buffer(control_operation, (fbe_u32_t*)&object_tag_p);
    if (status != FBE_STATUS_OK) {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    buffer_length = 0;
    fbe_payload_control_get_buffer_length(control_operation, &buffer_length); 
    if (buffer_length != sizeof(fbe_u32_t)) {
        fbe_base_service_trace((fbe_base_service_t*)&traffic_trace_service,
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s, can't get control operation buffer size\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_traffic_trace_disable(*object_tag_p);

    if (status != FBE_STATUS_OK) {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }
    else {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/*!***************************************************************************
 *          fbe_traffic_trace_control_entry()
 *****************************************************************************
 *
 * @brief   This is the entry fucntion for all traffic trace control codes
 *  
 * @param   packet
 *
 * @return  FBE_TRUE or FBE_FALSE
 *
 * @author
 *  04/10/2011  Swati Fursule - Created
 *
 *****************************************************************************/
fbe_status_t
fbe_traffic_trace_control_entry(fbe_packet_t * packet)
{    
    fbe_payload_control_operation_opcode_t control_code;
    fbe_status_t status;

    control_code = fbe_transport_get_control_code(packet);
    if (control_code == FBE_BASE_SERVICE_CONTROL_CODE_INIT) {
        fbe_traffic_trace_init();
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    if (fbe_base_service_is_initialized((fbe_base_service_t*)&traffic_trace_service) == FALSE) {
        fbe_transport_set_status(packet, FBE_STATUS_NOT_INITIALIZED, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_NOT_INITIALIZED;
    }

    switch(control_code) {
        case FBE_BASE_SERVICE_CONTROL_CODE_DESTROY:
            fbe_traffic_trace_destroy();
            fbe_base_service_destroy((fbe_base_service_t*)&traffic_trace_service);
            fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet);
            status = FBE_STATUS_OK;
            break;
        case FBE_TRAFFIC_TRACE_CONTROL_CODE_ENABLE_RBA_TRACE:
            status = fbe_traffic_trace_control_enable(packet);
            break;
        case FBE_TRAFFIC_TRACE_CONTROL_CODE_DISABLE_RBA_TRACE:
            status = fbe_traffic_trace_control_disable(packet);
            break;
        default:
            fbe_base_service_trace((fbe_base_service_t*)&traffic_trace_service,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s, unknown trace control code: %X\n", __FUNCTION__, control_code);

            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }
    return status;
}
/*!****************************************************************************
 * fbe_traffic_trace_init()
 ******************************************************************************
 * @brief
 *  This is the function for Initializing RBA tracing.
 *
 * @param None
 *
 * @return None
 *
 *****************************************************************************/
fbe_status_t fbe_traffic_trace_init(void)
{

    /* Initialize the base service
     */
    fbe_base_service_set_service_id(&traffic_trace_service.base_service, 
                                    FBE_SERVICE_ID_TRAFFIC_TRACE);
    fbe_base_service_set_trace_level(&traffic_trace_service.base_service, 
                                     fbe_trace_get_default_trace_level());
    fbe_base_service_init(&traffic_trace_service.base_service);

    /* RBA uses the traffic trace get a pointer to the "traffic" trace
       info buffer if we are the first place to initialize it. It may
       have been initialized somewhere else and would thus not be NULL. */
    if (fbe_ktrace_is_initialized() == FBE_FALSE)
    {
        fbe_ktrace_initialize();
    }
    return FBE_STATUS_OK;
}


/*!****************************************************************************
 * fbe_traffic_trace_destroy()
 ******************************************************************************
 * @brief
 *  This is the function for Initializing RBA tracing.
 *
 * @param None
 *
 * @return None
 *
 *****************************************************************************/
fbe_status_t 
fbe_traffic_trace_destroy(void)
{
    /* Must destroy the fbe ktrace also */
    fbe_ktrace_destroy();
    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * fbe_traffic_trace_is_enabled_sim()
 ******************************************************************************
 * @brief
 *  This is the function for checking if RBA is enabled.
 *
 * @param component_flag - traffic flag.
 *
 * @return True or False
 *
 *****************************************************************************/
#if defined(UMODE_ENV) || defined(SIMMODE_ENV)
fbe_bool_t fbe_traffic_trace_is_enabled_sim(KTRC_tflag_T component_flag)
{
    if (fbe_ktrace_is_initialized())
    {
        fbe_rba_trace_set[FBE_TRAFFIC_TRACE_CLASS_INAVLID] = FBE_FALSE;

        switch (component_flag)
        {
            case KTRC_TFBE_LUN: return fbe_rba_trace_set[FBE_TRAFFIC_TRACE_CLASS_LUN];
            case KTRC_TFBE_RG: return fbe_rba_trace_set[FBE_TRAFFIC_TRACE_CLASS_RG];
            case KTRC_TFBE_RG_FRU: return fbe_rba_trace_set[FBE_TRAFFIC_TRACE_CLASS_RG_FRU];
            case KTRC_TVD:      return fbe_rba_trace_set[FBE_TRAFFIC_TRACE_CLASS_VD];
            case KTRC_TPVD:     return fbe_rba_trace_set[FBE_TRAFFIC_TRACE_CLASS_PVD];
            case KTRC_TPDO:     return fbe_rba_trace_set[FBE_TRAFFIC_TRACE_CLASS_PD];
        }
    }
    return false;
}
#endif
/*!**************************************************************
 * fbe_traffic_trace_get_priority()
 ****************************************************************
 * @brief
 *  Fetch the priority for a given packet and convert it
 *  to an rba trace priority
 *
 * @param packet_p - Packet to get priority for.
 *
 * @return fbe_u8_t - rba trace priority
 *
 * @author
 *  9/6/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_u8_t fbe_traffic_trace_get_priority(fbe_packet_t *packet_p)
{
    fbe_packet_priority_t priority;
    fbe_u8_t rba_priority;

    fbe_transport_get_packet_priority(packet_p, &priority);

    switch (priority)
    {
        case FBE_PACKET_PRIORITY_LOW:
            rba_priority = KT_FBE_PRIORITY_LOW;
            break;
        case FBE_PACKET_PRIORITY_NORMAL:
            rba_priority = KT_FBE_PRIORITY_NORMAL;
            break;
        case FBE_PACKET_PRIORITY_URGENT:
            rba_priority = KT_FBE_PRIORITY_URGENT;
            break;
        default:
            rba_priority = 0;
            break;
    };
    return rba_priority;
}
/******************************************
 * end fbe_traffic_trace_get_priority()
 ******************************************/

/*!**************************************************************
 * fbe_traffic_trace_get_priority_from_cdb()
 ****************************************************************
 * @brief
 *  Fetch the cdb priority and convert it to an RBA trace priority.
 *
 * @param cdb_priority - The priority to convert.
 *
 * @return fbe_u8_t the rba trace priority.
 *
 * @author
 *  9/6/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_u8_t fbe_traffic_trace_get_priority_from_cdb(fbe_payload_cdb_priority_t cdb_priority)
{
    fbe_u8_t rba_priority;

    switch (cdb_priority)
    {
        case FBE_PAYLOAD_CDB_PRIORITY_LOW:
            rba_priority = KT_FBE_PRIORITY_LOW;
            break;
        case FBE_PAYLOAD_CDB_PRIORITY_NORMAL:
            rba_priority = KT_FBE_PRIORITY_NORMAL;
            break;
        case FBE_PAYLOAD_CDB_PRIORITY_URGENT:
            rba_priority = KT_FBE_PRIORITY_URGENT;
            break;
        default:
            rba_priority = 0;
            break;
    };
    return rba_priority;
}
/******************************************
 * end fbe_traffic_trace_get_priority_from_cdb()
 ******************************************/

/*!****************************************************************************
 * fbe_traffic_trace_block_operation()
 ******************************************************************************
 * @brief
 *  This is the function for tracing block operations in RBA buffer.
 *  LUN. fruts traffic along with VD and PVD traffic can be traced.
 *
 * @param class_id - required to distinguish between multiple traffics.
 * @param block_op_p  - block operation structures
 * @param target - caller's device number. Could be an RG, LUN, etc
 * @param info - specific infomration related to specific object.
 * @param b_start  - indicates start or end for RBA tracing
 * @param priority - The priority to trace for this operation.
 *
 * @return fbe_status_t
 *
 * @author
 *  12/21/2010 - Created. Swati Fursule
 *
 *****************************************************************************/
fbe_status_t fbe_traffic_trace_block_operation(fbe_class_id_t class_id, 
                                               fbe_payload_block_operation_t *block_op_p, 
                                               fbe_u32_t target,
                                               fbe_u64_t obj_info,/*extra information*/
                                               fbe_bool_t b_start,
                                               fbe_u16_t priority)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t    traffic = KT_LUN_TRAFFIC;
    fbe_char_t   target_string[50] = {'\0'};
    fbe_char_t   opcode_string[15] = {'\0'};
    fbe_u64_t    op_code = 0;

    /* It is expected that block operation is not NULL
     */
    if (block_op_p == NULL)
    {
        fbe_KvTrace("trace_rba: %s - block operation is NULL for class_id: %d target: 0x%x info: 0x%llx b_start: %d\n",
                    __FUNCTION__, class_id, target, obj_info, b_start);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if ((class_id >= FBE_CLASS_ID_RAID_FIRST) && 
        (class_id <= FBE_CLASS_ID_RAID_LAST) )
    {
        traffic = KT_FBE_RG_FRU_TRAFFIC;
        strcpy(target_string, "RG FRU");
    }else if (class_id == FBE_CLASS_ID_LUN)
    {
        traffic = KT_FBE_LUN_TRAFFIC;
        strcpy(target_string, "LUN");
    }else if (class_id == FBE_CLASS_ID_VIRTUAL_DRIVE)
    {
        traffic = KT_FBE_VD_TRAFFIC;
        strcpy(target_string, "VD");
    }else if(class_id == FBE_CLASS_ID_PROVISION_DRIVE)
    {
        traffic = KT_FBE_PVD_TRAFFIC;
        strcpy(target_string, "PVD");
    }
    if (b_start)
    {
        if (block_op_p->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE ||
            block_op_p->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED ||
            block_op_p->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY ||
            block_op_p->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE)         
        {
            op_code = KT_TRAFFIC_WRITE_START;
            strcpy(opcode_string, "w:st");
        }
        else if (block_op_p->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO)
        {
            op_code = KT_TRAFFIC_ZERO_START;
            strcpy(opcode_string, "z:st");
            
        }
        else /* Vamsi: We have to handle other opcodes. */
        {
            op_code = KT_TRAFFIC_READ_START;
            strcpy(opcode_string, "r:st");
        }
    }
    else
    {
        if (block_op_p->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE ||
            block_op_p->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED ||
            block_op_p->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY ||
            block_op_p->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE) 
        {
            op_code = KT_TRAFFIC_WRITE_DONE;
            strcpy(opcode_string, "w:d");
        }
        else if (block_op_p->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO)
        {
            op_code = KT_TRAFFIC_ZERO_DONE;
            strcpy(opcode_string, "z:d");
        }
        else
        {
            op_code = KT_TRAFFIC_READ_DONE;
            strcpy(opcode_string, "r:d");
        }
    }
    op_code |= priority;
#if defined(UMODE_ENV) || defined(SIMMODE_ENV)
    /* In user mode, this trace is displayed on console window. commneting it out temporarily
       modified to trace by two lines to fix KTRACE_STRING_TOO_LONG_ERROR*/
    fbe_KvTrace(" %s %s: lba: 0x%llx Info: 0x%llx OBJ/BL: 0x%llx opcode: 0x%llx\n",
                target_string, opcode_string, 
                /* LBA */
                ((unsigned long long)block_op_p->lba),
                /* Private Use */
                obj_info, 
                /* Send the number of blocks */
                KT_TRAFFIC_LU1(target) | KT_TRAFFIC_BLOCKS(block_op_p->block_count),
                /* Opcode */
                op_code);
#else
    /* tr_id : 
      ULONG id     : 8;  // new KT_*_TRAFFIC? (TCD, SFE, LUN, DBE, etc)
     
      ULONG action : 1;  // KT_START, KT_DONE
      ULONG iotype : 7;  // KT_READ,WRITE,ZERO, etc. (128)
     
    Remaining 48 bits to be interpreted by each layered id, roughly:
      ULONG label  : 8;  // KT_PRIMARY,SECONDARY,SOURCE,DEST, etc.
      ULONG layer  : 8;  // KT_FLARE,AGGREGATE,MIGRATION, etc.
      ULONG session: 16;
      ULONG remain : 16;
    Labels KT_SOURCE, KT_DESTINATION no longer required with dual LU/LBA;  Layer might largely be implied from id?
     
    tr_a0 : LBA1
    tr_a1 : LBA2 (if applicable, or layer specific)
    tr_a2 : LU1, LU2, block_count:  
    tr_a3 : 64-bit tag (or interpret differently).
    */

    KTRACEX(TRC_K_TRAFFIC,
            /* Indicate that it's from the  LUN / Raid group /VD /PVD */
            traffic,
            /* LBA */
            ((fbe_u64_t)block_op_p->lba),
            /* Private Use */
            obj_info, 
            /* Send the number of blocks */
            KT_TRAFFIC_LU1(target) | KT_TRAFFIC_BLOCKS(block_op_p->block_count),
            /* Opcode */
            op_code);
#endif
    return status;  
}
/*!****************************************************************************
 * fbe_traffic_trace_block_opcode()
 ******************************************************************************
 * @brief
 *  This function traces the block operation for a given class id.
 *
 * @param class_id - required to distinguish between multiple traffics.
 * @param block_op_p  - block operation structures
 * @param target - caller's device number. Could be an RG, LUN, etc
 * @param info - specific infomration related to specific object.
 * @param b_start  - indicates start or end for RBA tracing
 * @param priority - The priority to trace for this operation.
 *
 * @return fbe_status_t
 *
 * @author
 *  5/24/2013 - Created. Rob Foley
 *
 *****************************************************************************/
fbe_status_t fbe_traffic_trace_block_opcode(fbe_class_id_t class_id, 
                                            fbe_lba_t lba,
                                            fbe_block_count_t block_count,
                                            fbe_payload_block_operation_opcode_t block_opcode,
                                            fbe_u32_t target,
                                            fbe_u64_t obj_info,/*extra information*/
                                            fbe_u64_t obj_info2,
                                            fbe_bool_t b_start,
                                            fbe_u16_t priority)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t    traffic = KT_LUN_TRAFFIC;
    fbe_char_t   target_string[50] = {'\0'};
    fbe_char_t   opcode_string[15] = {'\0'};
    fbe_u16_t    op_code;

    if ((class_id >= FBE_CLASS_ID_RAID_FIRST) && 
        (class_id <= FBE_CLASS_ID_RAID_LAST) )
    {
        traffic = KT_FBE_RG_TRAFFIC;
        strcpy(target_string, "RG");
    }else if (class_id == FBE_CLASS_ID_LUN)
    {
        traffic = KT_FBE_LUN_TRAFFIC;
        strcpy(target_string, "LUN");
    }else if (class_id == FBE_CLASS_ID_VIRTUAL_DRIVE)
    {
        traffic = KT_FBE_VD_TRAFFIC;
        strcpy(target_string, "VD");
    }else if(class_id == FBE_CLASS_ID_PROVISION_DRIVE)
    {
        traffic = KT_FBE_PVD_TRAFFIC;
        strcpy(target_string, "PVD");
    }
    if (b_start)
    {
        if (block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE ||
            block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED ||
            block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY ||
            block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE)         
        {
            op_code = KT_TRAFFIC_WRITE_START;
            strcpy(opcode_string, "w:st");
        }
        else if (block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO)
        {
            op_code = KT_TRAFFIC_ZERO_START;
            strcpy(opcode_string, "z:st");
            
        }
        else /* Vamsi: We have to handle other opcodes. */
        {
            op_code = KT_TRAFFIC_READ_START;
            strcpy(opcode_string, "r:st");
        }
    }
    else
    {
        if (block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE ||
            block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED ||
            block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY ||
            block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE) 
        {
            op_code = KT_TRAFFIC_WRITE_DONE;
            strcpy(opcode_string, "w:d");
        }
        else if (block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO)
        {
            op_code = KT_TRAFFIC_ZERO_DONE;
            strcpy(opcode_string, "z:d");
        }
        else
        {
            op_code = KT_TRAFFIC_READ_DONE;
            strcpy(opcode_string, "r:d");
        }
    }
    op_code |= priority;

#if defined(UMODE_ENV) || defined(SIMMODE_ENV)
    /* In user mode, this trace is displayed on console window. commneting it out temporarily
       modified to trace by two lines to fix KTRACE_STRING_TOO_LONG_ERROR*/
    fbe_KvTrace(" %s %s: lba: 0x%llx Info: 0x%llx OBJ/BL: 0x%llx opcode: 0x%llx\n",
                target_string, opcode_string, 
                /* LBA */
                ((unsigned long long)lba),
                /* Private Use */
                obj_info, 
                /* Send the number of blocks */
                KT_TRAFFIC_LU1(target) | KT_TRAFFIC_BLOCKS(block_count),
                /* Opcode */
                obj_info2 | op_code);
#else
    /* tr_id : 
      ULONG id     : 8;  // new KT_*_TRAFFIC? (TCD, SFE, LUN, DBE, etc)
     
      ULONG action : 1;  // KT_START, KT_DONE
      ULONG iotype : 7;  // KT_READ,WRITE,ZERO, etc. (128)
     
    Remaining 48 bits to be interpreted by each layered id, roughly:
      ULONG label  : 8;  // KT_PRIMARY,SECONDARY,SOURCE,DEST, etc.
      ULONG layer  : 8;  // KT_FLARE,AGGREGATE,MIGRATION, etc.
      ULONG session: 16;
      ULONG remain : 16;
    Labels KT_SOURCE, KT_DESTINATION no longer required with dual LU/LBA;  Layer might largely be implied from id?
     
    tr_a0 : LBA1
    tr_a1 : LBA2 (if applicable, or layer specific)
    tr_a2 : LU1, LU2, block_count:  
    tr_a3 : 64-bit tag (or interpret differently).
    */

    KTRACEX(TRC_K_TRAFFIC,
            /* Indicate that it's from the  LUN / Raid group /VD /PVD */
            traffic,
            /* LBA */
            ((fbe_u64_t)lba),
            /* Private Use */
            obj_info, 
            /* Send the number of blocks */
            KT_TRAFFIC_LU1(target) | KT_TRAFFIC_BLOCKS(block_count),
            /* Opcode */
            obj_info2 | op_code);
#endif
    return status;  
}
/*!**************************************************************
 * fbe_traffic_trace_get_crc()
 ****************************************************************
 * @brief
 *  Get the crc that we want to log to the traffic trace.
 *
 * @param sector_p - The sector to log for.               
 *
 * @return The word that we would like to log.
 *
 * @author
 *  6/18/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_u64_t fbe_traffic_trace_get_crc(fbe_sector_t *sector_p)
{
    fbe_u64_t crc;
    fbe_u64_t data_word;

    /* To construct the CRC that we trace, we will fold the last word of the block
     * on itself and then add it to the CRC. 
     *  
     * This is needed since with some host patterns we will not be able to tell 
     * the difference between old/new data without this change since in many cases 
     * the patterns include an even number of words which cancel themselves out. 
     */
    data_word = sector_p->data_word[FBE_WORDS_PER_BLOCK - 1];
    data_word = (0xFFFFu & ((data_word >> 16) ^ data_word));
    crc = sector_p->crc ^ data_word;
    return crc;
}
/******************************************
 * end fbe_traffic_trace_get_crc()
 ******************************************/
/*!**************************************************************
 * fbe_traffic_trace_get_data_crc()
 ****************************************************************
 * @brief
 *  Calculate a word that contains a set of data crs.
 * 
 *  This is used for tracing
 *
 * @param sg_p - Pointer to the sg to inspect.
 * @param blocks - Total blocks in sg.
 * @param block_offset - offset in bytes into above sg where I/O starts.
 * @param trace_info_p - A word that is comprised of up to 4
 *                       sector crcs that we choose from the sg list.
 *
 * @return None.   
 *
 * @author
 *  5/16/2013 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_traffic_trace_get_data_crc(fbe_sg_element_t *sg_p,
                                    fbe_block_count_t blocks,
                                    fbe_u32_t block_offset,
                                    fbe_u64_t *trace_info_p)
{
    fbe_u64_t crc0;
    fbe_u64_t crc1 = 0;
    fbe_u64_t crc2 = 0;
    fbe_u64_t crc3 = 0;
    fbe_u32_t offset1 = 0;
    fbe_u32_t offset2 = 0;
    fbe_u32_t offset3 = 0;
    fbe_sector_t *sector_p = NULL;
    fbe_u32_t offset = block_offset * FBE_BE_BYTES_PER_BLOCK;
    fbe_u32_t block_count = (fbe_u32_t)blocks;

    if (trace_info_p == NULL){
        return;
    }
    *trace_info_p = 0;

    /* We only have half and quarter if we have more than 2 blocks. 
     * At 3 blocks, the quarter (offset 1) is 0. 
     */
    if (block_count > 2){
        offset1 += (block_count >> 2); // 1/4
        offset2 += (block_count >> 1); // 1/2
    }

    /* Offset 3 is the last block.  We only have a last block for 2 or more total blocks.
     */
    if (block_count > 1){
        offset3 += block_count - 1;
    }

    if ((sg_p == NULL) ||
        (sg_p->address == NULL)){
        return;
    }
    /* this is the layout of the 64 bit word:  crc3 = last | crc2 = 1/2 | crc1 = 1/4 | crc0 = first
     *  1/2 is halfway through the transfer (blocks >> 1)
     *  1/4 is a quarter of the way through the transfer (blocks >> 2).
     *  We always have a first.
     *   If we have at least 2 blocks we have a last.
     *   If we have at least 3 blocks we have a 1/2.
     *   If we have at least 4 blocks we have a 1/4.
     *  
     * We always have one sector.  Get the crc of the first sector.
     */
    fbe_sg_element_adjust_offset(&sg_p, &offset);
    sector_p = (fbe_sector_t*)(sg_p->address + offset);
    crc0 = fbe_traffic_trace_get_crc(sector_p);

    /* If our offset is valid, then simply get the sg ptr and offset to that sector.
     */
    if (offset1 > 0){
        /* Add on the offset to the next sector, then adjust the sg/offset to point to the correct sg.
         */
        offset += offset1 * FBE_BE_BYTES_PER_BLOCK;
        fbe_sg_element_adjust_offset(&sg_p, &offset);
        sector_p = (fbe_sector_t*)(sg_p->address + offset);
        crc1 = fbe_traffic_trace_get_crc(sector_p);
    }
    if (offset2 > 0){
        /* Offset of the 3rd sector is just the difference between the 2nd and 3rd offsets..
         */
        offset += (offset2 - offset1) * FBE_BE_BYTES_PER_BLOCK;
        fbe_sg_element_adjust_offset(&sg_p, &offset);
        sector_p = (fbe_sector_t*)(sg_p->address + offset);
        crc2 = fbe_traffic_trace_get_crc(sector_p);
    }
    if (offset3 > 0){
        /* Offset of the 4th sector is just the difference between the 3rd and 4th offsets..
         */
        offset += (offset3 - offset2) * FBE_BE_BYTES_PER_BLOCK;
        fbe_sg_element_adjust_offset(&sg_p, &offset);
        sector_p = (fbe_sector_t*)(sg_p->address + offset);
        crc3 = fbe_traffic_trace_get_crc(sector_p);
    }
    /* Combine the 4 values into one 64 bit word.
     */
    *trace_info_p = (crc3 << 48) | (crc2 << 32) | (crc1 << 16) | crc0;
}
/******************************************
 * end fbe_traffic_trace_get_data_crc()
 ******************************************/