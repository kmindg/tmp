/*******************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 *******************************************************************/

/*!*****************************************************************
 * @file fbe_logical_error_injection_block_operations.c
 *******************************************************************
 *
 * @brief
 *    This file contains functions which are used to inject errors
 *    into block operations.
 *
 * @author
 *    05/12/2010 - Created. Randy Black
 *
 *******************************************************************/


/*************************
 * INCLUDE FILES
 *************************/

#include "fbe_logical_error_injection_private.h"
#include "fbe_logical_error_injection_proto.h"
#include "base_object_private.h"


/*****************************************************
 *  Global Variables
 *****************************************************/

/*****************************************************
 *  static FUNCTIONS DECLARED FOR VISIBILITY
 *****************************************************/

static fbe_status_t fbe_logical_error_injection_block_operation_inject(fbe_payload_block_operation_t *block_operation_p,
                                                                       fbe_logical_error_injection_record_t *rec_p,
                                                                       fbe_payload_ex_t *payload_p,
                                                                       fbe_base_edge_t *base_edge_p);

static void fbe_logical_error_injection_block_io_inject_media_error(fbe_payload_block_operation_t *const block_operation_p,
                                                                    fbe_logical_error_injection_record_t *const in_rec_p,
                                                                    fbe_logical_error_injection_object_t *const object_p,
                                                                    fbe_payload_ex_t *const payload_p,
                                                                    fbe_bool_t *const b_media_error_injected_p);

static void fbe_logical_error_injection_block_io_get_bad_lba_range(fbe_payload_block_operation_t *const block_operation_p,
                                                                   fbe_lba_t*         const out_lba_p,
                                                                   fbe_block_count_t* const out_blocks_p );


static fbe_bool_t fbe_logical_error_injection_block_io_inject_media_err_read(fbe_payload_block_operation_t *const block_operation_p,
                                                                       fbe_logical_error_injection_record_t* const in_rec_p,
                                                                       fbe_logical_error_injection_object_t *const object_p,
                                                                       fbe_lba_t*                            io_bad_lba_p,
                                                                       fbe_block_count_t                     in_bad_blocks );

static fbe_bool_t fbe_logical_error_injection_block_io_inject_media_err_write(fbe_payload_block_operation_t *const block_operation_p,
                                                                              fbe_logical_error_injection_record_t* const in_rec_p,
                                                                              fbe_logical_error_injection_object_t *const object_p,
                                                                              fbe_lba_t*                            io_bad_lba_p,
                                                                              fbe_block_count_t                     in_bad_blocks );

static fbe_payload_block_operation_t* fbe_logical_error_injection_packet_to_block_operation( fbe_packet_t* in_packet_p );

static fbe_object_id_t fbe_logical_error_injection_packet_to_object_id( fbe_packet_t* in_packet_p );

static void fbe_logical_error_injection_invalidate_media_error_lba( fbe_packet_t* in_packet_p );


/*!**************************************************************
 * fbe_logical_error_injection_block_io_inject_all()
 ****************************************************************
 * 
 * @brief
 *    This function handles all aspects of injecting errors  on
 *    a block i/o operation as directed by its associated error
 *    injection record.
 * 
 * @param   in_packet_p  -  pointer to control packet
 *
 * @return  void
 *
 * @author
 *    04/16/2010 - Created. Randy Black
 *
 ****************************************************************/

void
fbe_logical_error_injection_block_io_inject_all( fbe_packet_t* in_packet_p )
{

	fbe_payload_ex_t                       *payload_p;
    fbe_block_count_t                       block_count;
    fbe_lba_t                               block_lba;
    fbe_payload_block_operation_opcode_t    block_opcode;
    fbe_payload_block_operation_t          *block_operation_p;
    fbe_bool_t                              enabled_b;
    fbe_u32_t                               rec_index;
    fbe_logical_error_injection_record_t   *rec_p;

    /* Get the payload
     */
	payload_p = fbe_transport_get_payload_ex(in_packet_p);

    /* get block operation payload from this packet
     */
    block_operation_p = fbe_logical_error_injection_packet_to_block_operation( in_packet_p );

    /* extract block operation payload's opcode, lba, and block count
     */
    fbe_payload_block_get_opcode( block_operation_p, &block_opcode );
    fbe_payload_block_get_lba( block_operation_p, &block_lba );
    fbe_payload_block_get_block_count( block_operation_p, &block_count );

    /* if the block operation status is not already success then do
     * not inject error(s) since in some cases the error  injection
     * service sets status to good when injecting soft media errors
     */
    if ( block_operation_p->status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS )
    {
        return;
    }


    /* only inject errors if logical error injection service is enabled
     */
    fbe_logical_error_injection_get_enable( &enabled_b );
    if ( !enabled_b )
    {
        return;
    }

    /* only inject errors on block operations listed below
     */
    switch(block_opcode)
    {
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ERROR_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE:
            /*! @note We only inject `block operation' errors for the opcodes 
             *        listed above.
             */
            break;

        default:
            /*! @note We will not inject for other opcodes. 
             */
            fbe_logical_error_injection_service_trace( FBE_TRACE_LEVEL_DEBUG_HIGH,
                                               FBE_TRACE_MESSAGE_ID_INFO,
                                               "%s: skip opcode: %d lba: 0x%llx blocks: 0x%llx \n",
                                               __FUNCTION__, block_opcode, (unsigned long long)block_lba, (unsigned long long)block_count);
            return;
    }

    /* loop through error injection records
     */
    for ( rec_index = 0; rec_index < FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS; rec_index++ )
    {
        /* get pointer to selected error injection record
         */
        fbe_logical_error_injection_get_error_info( &rec_p, rec_index );

        /* nothing to inject if there is no overlap between current
         * block i/o's range and the error injection record's range
         */
        if ( !fbe_logical_error_injection_overlap(block_lba, 
                                                  block_count, 
                                                  rec_p->lba, 
                                                  rec_p->blocks) )
        {
            /* if there's no overlap then invalidate the last media
             * error lba when injecting errors at the same lba
             */
            if ( rec_p->err_mode == FBE_LOGICAL_ERROR_INJECTION_MODE_INJECT_SAME_LBA )
            {
                fbe_logical_error_injection_invalidate_media_error_lba( in_packet_p );
            }
            continue;
        }
        /* inject errors based on record's error injection mode
         */
        switch ( rec_p->err_mode )
        {
        
            case FBE_LOGICAL_ERROR_INJECTION_MODE_ALWAYS:
            case FBE_LOGICAL_ERROR_INJECTION_MODE_INJECT_SAME_LBA:
            case FBE_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED:
                {
                    rec_p->err_count++;
                    fbe_logical_error_injection_block_operation_inject(block_operation_p, rec_p, payload_p, in_packet_p->base_edge);
                }
                break;

            case FBE_LOGICAL_ERROR_INJECTION_MODE_COUNT:
                {
                    if ( rec_p->err_count < rec_p->err_limit )
                    {
                        rec_p->err_count++;
                        fbe_logical_error_injection_block_operation_inject(block_operation_p, rec_p, payload_p, in_packet_p->base_edge);
                    }
                }
                break;

            case FBE_LOGICAL_ERROR_INJECTION_MODE_SKIP:
                {
                    if ( rec_p->skip_count++ >= rec_p->skip_limit )
                    {
                        rec_p->skip_count = 0;
                        rec_p->err_mode = FBE_LOGICAL_ERROR_INJECTION_MODE_SKIP_INSERT;
                    }
                }
                break;

            case FBE_LOGICAL_ERROR_INJECTION_MODE_SKIP_INSERT:
                {
                    rec_p->err_count++;
                    fbe_logical_error_injection_block_operation_inject(block_operation_p, rec_p, payload_p, in_packet_p->base_edge);

                    if ( rec_p->skip_count++ >= rec_p->err_limit )
                    {
                        rec_p->skip_count = 0;
                        rec_p->err_mode = FBE_LOGICAL_ERROR_INJECTION_MODE_SKIP;
                    }
                }
                break;
        }
    }

    return;
}   /* end fbe_logical_error_injection_block_io_inject_all() */

/*****************************************************************************
 *          fbe_logical_error_injection_block_operation_inject()
 ***************************************************************************** 
 * 
 *  @brief  Corrupt all overlap between the block operation and the input error
 *          record with the error(s) indicated in the error record.
 * 
 * @param   block_operation_p - Pointer to block operation to inject
 * @param   rec_p - ptr to current error record
 * @param   payload_p - Pointer to payload so that media lba can be modified.
 * @param   block_edge_p - Pointer to block edge this operation was injected on
 *
 *  @return fbe_status_t
 *
 *  @author
 *  02/21/2012  Ron Proulx  - Created from fbe_logical_error_injection_fruts_inject()
 *
 *****************************************************************************/
static fbe_status_t fbe_logical_error_injection_block_operation_inject(fbe_payload_block_operation_t *block_operation_p,
                                                                       fbe_logical_error_injection_record_t * rec_p,
                                                                       fbe_payload_ex_t *payload_p,
                                                                       fbe_base_edge_t *base_edge_p)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_u32_t           position = FBE_RAID_INVALID_DISK_POSITION;
    fbe_block_count_t   block_count;
    fbe_lba_t           block_lba;
    fbe_payload_block_operation_opcode_t block_opcode;
    fbe_object_id_t     object_id = 0;
    fbe_logical_error_injection_object_t *object_p = NULL;
    fbe_raid_group_type_t raid_type;
    fbe_bool_t          b_is_proactive_sparing;
    fbe_u32_t           width;
    fbe_raid_position_bitmask_t parity_bitmask;
    fbe_bool_t          b_is_parity_position;
    fbe_u64_t           phys_addr_local = 0;
    fbe_bool_t          b_media_error_injected = FBE_FALSE;

    /* extract block operation payload's opcode, lba, and block count
     */
    fbe_payload_block_get_opcode( block_operation_p, &block_opcode );
    fbe_payload_block_get_lba( block_operation_p, &block_lba );
    fbe_payload_block_get_block_count( block_operation_p, &block_count );

    /* Get the destination object from the edge.
     */
    object_id = base_edge_p->client_id;
    position = base_edge_p->client_index;

    /*! @todo Need to populate the following fields with the correct
     *        values:
     *          o raid_type
     *          o b_is_proactive_sparing
     *          o width
     *          o parity_bitmask
     *          o b_is_parity_position
     */
    raid_type = FBE_RAID_GROUP_TYPE_UNKNOWN;
    b_is_proactive_sparing = FBE_FALSE;
    width = rec_p->width;
    parity_bitmask = 0;
    b_is_parity_position = FBE_FALSE;

    /* Fetch the object ptr.
     */
    object_p = fbe_logical_error_injection_find_object(object_id);
    if (object_p == NULL)
    {
        /* We are not injecting for this object, so not sure why we are here.
         */
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
             "%s object_id: 0x%x was not found"
             "while inject 0x%x on pos: %d lba: %llx blocks: 0x%llx opcode: %d\n",
             __FUNCTION__, object_id, rec_p->err_type, position, 
             (unsigned long long)block_lba, (unsigned long long)block_count, block_opcode);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /*! @note Currently we do not increment the per object counters.
     */

    /* If enabled trace the invocation.
     */
    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                                              "LEI: block 0x%x err_type 0x%x on obj: 0x%x pos: %d lba: 0x%llx blks: 0x%llx op: %d\n",
                                              (fbe_u32_t)rec_p, rec_p->err_type, object_id, position, 
                                              (unsigned long long)block_lba, (unsigned long long)block_count, block_opcode);


    /*! @note There are cases where the error type is `none'.
     *        Need to determine why this is the case.
     */
    switch(rec_p->err_type)
    {
        /* The following error types are currently supported.
         */
        case FBE_XOR_ERR_TYPE_SOFT_MEDIA_ERR:
        case FBE_XOR_ERR_TYPE_HARD_MEDIA_ERR:   
        case FBE_XOR_ERR_TYPE_CRC:        
        case FBE_XOR_ERR_TYPE_KLOND_CRC:       
        case FBE_XOR_ERR_TYPE_DH_CRC:         
        case FBE_XOR_ERR_TYPE_RAID_CRC:     
        case FBE_XOR_ERR_TYPE_CORRUPT_CRC:      
        case FBE_XOR_ERR_TYPE_WS:               
        case FBE_XOR_ERR_TYPE_TS:        
        case FBE_XOR_ERR_TYPE_LBA_STAMP:   
        case FBE_XOR_ERR_TYPE_SINGLE_BIT_CRC:   
        case FBE_XOR_ERR_TYPE_MULTI_BIT_CRC:  
        case FBE_XOR_ERR_TYPE_MULTI_BIT_WITH_LBA_STAMP: 
        case FBE_XOR_ERR_TYPE_KEY_ERROR: 
        case FBE_XOR_ERR_TYPE_KEY_NOT_FOUND: 
        case FBE_XOR_ERR_TYPE_ENCRYPTION_NOT_ENABLED: 
            break;
             
        /*! @todo The following error types are not currently supported.
         */
        case FBE_XOR_ERR_TYPE_NONE: /*! @note There are race conditions where this is set.*/
        case FBE_XOR_ERR_TYPE_SS:
        case FBE_XOR_ERR_TYPE_BOGUS_WS:  
        case FBE_XOR_ERR_TYPE_BOGUS_TS:  
        case FBE_XOR_ERR_TYPE_BOGUS_SS:           
        case FBE_XOR_ERR_TYPE_1NS:                
        case FBE_XOR_ERR_TYPE_1S:                 
        case FBE_XOR_ERR_TYPE_1R:                 
        case FBE_XOR_ERR_TYPE_1D:                 
        case FBE_XOR_ERR_TYPE_1COD:               
        case FBE_XOR_ERR_TYPE_1COP:               
        case FBE_XOR_ERR_TYPE_1POC:               
        case FBE_XOR_ERR_TYPE_COH:                
        case FBE_XOR_ERR_TYPE_CORRUPT_DATA:       
        case FBE_XOR_ERR_TYPE_N_POC_COH:          
        case FBE_XOR_ERR_TYPE_POC_COH:            
        case FBE_XOR_ERR_TYPE_COH_UNKNOWN:        
        case FBE_XOR_ERR_TYPE_RB_FAILED:          
        case FBE_XOR_ERR_TYPE_TIMEOUT_ERR:
        case FBE_XOR_ERR_TYPE_CORRUPT_CRC_INJECTED:
        case FBE_XOR_ERR_TYPE_CORRUPT_DATA_INJECTED:
        case FBE_XOR_ERR_TYPE_RND_MEDIA_ERR:   
        case FBE_XOR_ERR_TYPE_RAW_MIRROR_BAD_MAGIC_NUM: 
        case FBE_XOR_ERR_TYPE_RAW_MIRROR_BAD_SEQ_NUM:   
        case FBE_XOR_ERR_TYPE_SILENT_DROP:              
        case FBE_XOR_ERR_TYPE_INVALIDATED:              
        case FBE_XOR_ERR_TYPE_BAD_CRC:     
        case FBE_XOR_ERR_TYPE_IO_UNEXPECTED:
        case FBE_XOR_ERR_TYPE_UNKNOWN:                  
        default:
            /* Trace the fact that we received an unsupported type.
             */
            fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
             "%s object_id: 0x%x unexpected err_type: 0x%x on pos: %u lba: %llx blocks: 0x%llx opcode: %d\n",
             __FUNCTION__, object_id, rec_p->err_type, position, 
             (unsigned long long)block_lba, (unsigned long long)block_count, block_opcode);
            return FBE_STATUS_OK;

    } /* end switch on error type */

    /* If the error injection opcode isn't invalid and it doesn't match the
     * block operation code don't inject.
     */
    if ((rec_p->opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID) &&
        (rec_p->opcode <  FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_LAST)    &&
        (rec_p->opcode != block_opcode)                                  )
    {
        fbe_logical_error_injection_service_trace( FBE_TRACE_LEVEL_DEBUG_LOW,
                                                   FBE_TRACE_MESSAGE_ID_INFO,
                                                   "LEI: block do not inject: 0x%x on obj: 0x%x "
                                                   "op: %d lba: 0x%llx blks: 0x%08llx inject op: %d doesn't match\n",
                                                   rec_p->err_type, object_p->object_id,
                                                   block_opcode, (unsigned long long)block_lba, (unsigned long long)block_count, rec_p->opcode);
        return FBE_FALSE;
    }

    /* First inject a media error if it is enabled.  If we injected a media
     * error return.
     */
    fbe_logical_error_injection_block_io_inject_media_error(block_operation_p, rec_p, 
                                                            object_p, payload_p, &b_media_error_injected);
    if (b_media_error_injected == FBE_TRUE)
    {
        return FBE_STATUS_OK;
    }
    if (rec_p->err_type == FBE_XOR_ERR_TYPE_KEY_ERROR) {
        block_operation_p->status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED;
        block_operation_p->status_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_KEY_WRAP_ERROR;
        return FBE_STATUS_OK;
    } else if (rec_p->err_type == FBE_XOR_ERR_TYPE_KEY_NOT_FOUND) {
        block_operation_p->status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED;
        block_operation_p->status_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_BAD_KEY_HANDLE;
        return FBE_STATUS_OK;
    } else if (rec_p->err_type == FBE_XOR_ERR_TYPE_ENCRYPTION_NOT_ENABLED) {
        block_operation_p->status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED;
        block_operation_p->status_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_ENCRYPTION_NOT_ENABLED;
        return FBE_STATUS_OK;
    }  
    /* We always inject errors into the sectors,
     * even for hard errors.
     */
    {
        fbe_u32_t block_offset;   /* Block offset into fruts sg list */
        fbe_block_count_t modify_blocks = rec_p->blocks;  /* Total blocks in fruts list to modify */
        fbe_sg_element_with_offset_t sg_desc;
        fbe_lba_t table_lba;
        fbe_lba_t seed;
        fbe_sg_element_t *sg_p = NULL;
        fbe_u32_t   sg_list_count = 0;

        table_lba = fbe_logical_error_injection_get_table_lba(NULL, block_lba, block_count);
        
        {
            /* Based upon the amount of overlap, we may need to decrease the number
             * of blocks we are modifying.
             */            
            fbe_lba_t end_lba = table_lba + block_count - 1;   /* End of fruts range. */
            fbe_lba_t end_rec = rec_p->lba + rec_p->blocks - 1;    /* End of record range. */

            /* If record ends before this fruts,
             * decrease range to end at this fruts.
             */
            modify_blocks -= (end_rec > end_lba) ? (fbe_u32_t)(end_rec - end_lba) : 0;

            /* If fruts begins after the record start,
             * then decrease blocks to begin at this fruts.
             */
            modify_blocks -= (table_lba > rec_p->lba) ? (fbe_u32_t)(table_lba - rec_p->lba): 0;
            if(LEI_COND((modify_blocks == 0) || (modify_blocks > block_count)) )
            {
                fbe_logical_error_injection_service_trace(
                    FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: logical error injection: modify_blocks 0x%llx == 0 Or"
                    "modify_blocks 0x%llx > fruts_p->blocks 0x%llx"
                    "object_id:%d pos:0x%x, Lba:0x%llX Blks:0x%llx \n",
                    __FUNCTION__, (unsigned long long)modify_blocks, (unsigned long long)modify_blocks,
                    (unsigned long long)block_count, object_id, position, 
                    (unsigned long long)block_lba, (unsigned long long)block_count);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }

        /* The area being modified begins after the start of the block operation.
         * We have an offset into the area to be modified.
         */
        block_offset = (rec_p->lba <= table_lba) ? 0 : (fbe_u32_t)(rec_p->lba - table_lba);

        /* Init our sg descriptor
         */
        fbe_payload_ex_get_sg_list(payload_p, &sg_p, &sg_list_count);
        fbe_sg_element_with_offset_init(&sg_desc,  
                                        block_offset * FBE_BE_BYTES_PER_BLOCK, /* offset */
                                        sg_p,
                                        NULL /* default increment */);
        seed = block_lba + block_offset;

        /* Initially:
         *  sg_desc.offset is the byte offset into the entire sg "list",
         *    so offset does not respect sg element boundaries.
         *  sg_desc.sg_element is pointing at the head of the sg list.
         *
         * Call macro to sync up sg_element ptr and sg_desc offset.
         */
        fbe_raid_adjust_sg_desc(&sg_desc);

        /* Modify the blocks.
         */
        while (modify_blocks > 0)
        {
            fbe_sector_t * sector_p;

            /*! @todo Currently we don't mark the block operation to know thay
             *        an error was injected.
             */
            if(LEI_COND(sg_desc.offset >= sg_desc.sg_element->count) )
            {
                fbe_logical_error_injection_service_trace(
                    FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: logical error injection: sg_desc.offset 0x%x >= sg_desc.sg_element->count 0x%x \n",
                    __FUNCTION__, sg_desc.offset, sg_desc.sg_element->count);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* Map in the memory.
             */
            sector_p = (fbe_sector_t *) FBE_LOGICAL_ERROR_SGL_MAP_MEMORY(sg_desc.sg_element, 
                                                                         sg_desc.offset, 
                                                                         FBE_BE_BYTES_PER_BLOCK);

            /* Corrupt the sector.
             */
            status = fbe_logical_error_injection_corrupt_sector(sector_p,
                                                                rec_p,
                                                                position,
                                                                seed,
                                                                raid_type,
                                                                b_is_proactive_sparing,
                                                                block_lba,
                                                                block_count,
                                                                width,
                                                                parity_bitmask,
                                                                b_is_parity_position);
            if(LEI_COND(status != FBE_STATUS_OK))
            {
                fbe_logical_error_injection_service_trace(
                    FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: logical error injection: Bad status from fbe_logical_error_injection_corrupt_sector"
                    "sector_p %p rec_p %p position 0x%x parity_pos 0x%x seed 0x%llx\n",
                    __FUNCTION__, sector_p, rec_p, position, b_is_parity_position, (unsigned long long)seed);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            
            /* Done with the memory, if we had mapped in a virtual address
             * then unmap it now.
             */
            FBE_LOGICAL_ERROR_SGL_GET_PHYSICAL_ADDRESS(sg_desc.sg_element, phys_addr_local);
            if(phys_addr_local != 0)
            {
                FBE_LOGICAL_ERROR_SGL_UNMAP_MEMORY(sg_desc.sg_element, FBE_BE_BYTES_PER_BLOCK);
            }
            modify_blocks--;
            seed++;

            /* Increment our sg descriptor.
             */
            if(LEI_COND((sg_desc.sg_element->count % FBE_BE_BYTES_PER_BLOCK) != 0) )
            {
                fbe_logical_error_injection_service_trace(
                    FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: logical error injection: sg_desc.sg_element->count 0x%x %% FBE_BE_BYTES_PER_BLOCK) != 0\n",
                    __FUNCTION__, sg_desc.sg_element->count);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            if (modify_blocks)
            {
                if ((sg_desc.offset + FBE_BE_BYTES_PER_BLOCK) == sg_desc.sg_element->count)
                {
                    /* At end of an sg, goto next.
                     */
                    sg_desc.sg_element++,
                        sg_desc.offset = 0;

                    /* After incrementing the SG element ptr
                     * make sure we didn't fall off into the weeds.
                     */
                    if(LEI_COND( (sg_desc.sg_element->count == 0) ||
                                  (sg_desc.sg_element->address == NULL) ) )
                    {
                        fbe_logical_error_injection_service_trace(
                            FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: logical error injection: sg_desc.sg_element->count 0x%x == 0 Or"
                            "sg_desc.sg_element->address 0x%llx == NULL\n",
                            __FUNCTION__, sg_desc.sg_element->count, (unsigned long long)sg_desc.sg_element->address);
                        return FBE_STATUS_GENERIC_FAILURE;
                    }
                }
                else
                {
                    /* Blocks remaining in the current
                     * scatter gather element.
                     */
                    sg_desc.offset += FBE_BE_BYTES_PER_BLOCK;
                }
            } /* end if modify_blocks */
        } /* end while modify_blocks > 0 */
    }
    return FBE_STATUS_OK;
}
/*********************************************************
 * end fbe_logical_error_injection_block_operation_inject()
 *********************************************************/

/*!**************************************************************
 * fbe_logical_error_injection_block_io_inject_media_error()
 ****************************************************************
 *
 * @brief
 *    This function injects a media error on the specified block
 *    operation.
 *
 * @param   block_operation_p  -  pointer to block operation
 * @param   in_rec_p     -  pointer to error injection record
 * @param   object_p - pointer to error injeciton object information
 * @param   payload_p - pointer to paylaod in case we modify media lba
 * @param   b_media_error_injected_p - Pointer to media error injected
 *              or not.
 *
 * @return  void
 *
 * @author
 *    04/16/2010 - Created. Randy Black
 *
 ****************************************************************/

void fbe_logical_error_injection_block_io_inject_media_error(fbe_payload_block_operation_t *const block_operation_p,
                                                             fbe_logical_error_injection_record_t *const in_rec_p,
                                                             fbe_logical_error_injection_object_t *const object_p,
                                                             fbe_payload_ex_t *const payload_p,
                                                             fbe_bool_t *const b_media_error_injected_p)
{
    fbe_block_count_t                      bad_blocks;
    fbe_lba_t                              bad_lba;
    fbe_payload_block_operation_opcode_t   block_opcode;

    /* Mark that we haven't injected an error.
     */
    *b_media_error_injected_p = FBE_FALSE;

    /* get block operation opcode
     */
    fbe_payload_block_get_opcode( block_operation_p, &block_opcode );

    /* Return if error injection record doesn't select a media error
     */ 
    if ( (in_rec_p->err_type != FBE_XOR_ERR_TYPE_HARD_MEDIA_ERR) &&
         (in_rec_p->err_type != FBE_XOR_ERR_TYPE_SOFT_MEDIA_ERR)     )
    {
        return;
    }

    /* determine current lba range for error injection
     */
    fbe_logical_error_injection_block_io_get_bad_lba_range(block_operation_p, &bad_lba, &bad_blocks );

    /* Allow media errors to be injected on `read' operations.
     */
    if ((block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ)         ||
        (block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY)       ||
        (block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ERROR_VERIFY)    )
    {
        /* Inject for a read type of request.
         */
        if ( !fbe_logical_error_injection_block_io_inject_media_err_read(block_operation_p, in_rec_p, object_p, &bad_lba, bad_blocks) )
        {
            /* If this is a media error and we decided not to inject we return
             * True so that we don't inject else where.
             */
            *b_media_error_injected_p = FBE_TRUE;
            return;
        }
    }
    else if ( block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY )
    {
        /* if an error should not be injected on this
         * "write verify" block operation, then return
         */
        if ( !fbe_logical_error_injection_block_io_inject_media_err_write(block_operation_p, in_rec_p, object_p, &bad_lba, bad_blocks) )
        {
            /* If this is a media error and we decided not to inject we return
             * True so that we don't inject else where.
             */
            *b_media_error_injected_p = FBE_TRUE;
            return;
        }
    }
    else
    {
        /*! @todo Else for all other operations we will not do anything. 
         *        We may need to add support for other operation types
         *        in the future.  We flag the error as injected so that
         *        we don't attempt to inject other error types.
         */
        *b_media_error_injected_p = FBE_TRUE;
        return;
    }

    /* increment count of errors injected so far
     */
    fbe_logical_error_injection_lock();
    fbe_logical_error_injection_inc_num_errors_injected();
    fbe_logical_error_injection_unlock();

    /* set block status, block qualifier, and media error lba
     * for block operations when injecting a hard media error
     */
    if ( in_rec_p->err_type == FBE_XOR_ERR_TYPE_HARD_MEDIA_ERR )
    {
        fbe_payload_block_set_status( block_operation_p,
                                      FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR,
                                      FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST
                                    );

        fbe_payload_ex_set_media_error_lba(payload_p, bad_lba );
    }

    /* set block status, block qualifier, and media error lba
     * for block operations when injecting a soft media error
     */
    if ( in_rec_p->err_type == FBE_XOR_ERR_TYPE_SOFT_MEDIA_ERR )
    {
        fbe_payload_block_set_status( block_operation_p,
                                      FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                                      FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_REMAP_REQUIRED
                                    );

        fbe_payload_ex_set_media_error_lba(payload_p, bad_lba );
    }

    /* Flag the fact that we injected an error.
     */
    *b_media_error_injected_p = FBE_TRUE;

    /* Always succeeds
     */
    return;
}   /* end fbe_logical_error_injection_block_io_inject_media_error() */


/*!**************************************************************
 * fbe_logical_error_injection_block_io_inject_media_err_read()
 ****************************************************************
 *
 * @brief
 *    This function injects a media error on a "verify" block
 *    operation.
 *
 * @param   block_operation_p  -  pointer to block operation
 * @param   in_rec_p       -  pointer to error injection record
 * @param   object_p - Pointer to object injection information
 * @param   io_bad_lba_p   -  pointer to lba of bad i/o range
 * @param   in_bad_blocks  -  size of bad i/o range
 *
 * @return  void
 *
 * @author
 *    04/16/2010 - Created. Randy Black
 *
 ****************************************************************/

fbe_bool_t fbe_logical_error_injection_block_io_inject_media_err_read(fbe_payload_block_operation_t *const block_operation_p, 
                                                           fbe_logical_error_injection_record_t *const in_rec_p,
                                                           fbe_logical_error_injection_object_t *const object_p,
                                                           fbe_lba_t *io_bad_lba_p,
                                                           fbe_block_count_t in_bad_blocks)
{
    fbe_block_count_t   block_count;
    fbe_lba_t           block_lba;
    fbe_payload_block_operation_opcode_t block_opcode;
    fbe_lba_t           last_error_lba;

    /* extract block operation payload's opcode, lba, and block count
     */
    fbe_payload_block_get_opcode( block_operation_p, &block_opcode );
    fbe_payload_block_get_lba( block_operation_p, &block_lba );
    fbe_payload_block_get_block_count( block_operation_p, &block_count );

    /* Get the last media error lba.
     */
    last_error_lba = object_p->last_media_error_lba[0];

    /* true if last media error is in error injection range
     */
    if ( (last_error_lba >= *io_bad_lba_p) &&
         (last_error_lba <  (*io_bad_lba_p + in_bad_blocks)) )
    {
        /* inject new error at lba of last media error
         */
        *io_bad_lba_p = last_error_lba;
    }
    else
    {
        /* didn't find a match with the last media error; simply
         * set the last media error lba so that a "write verify"
         * operation will inject a media error on the next lba
         */
        object_p->last_media_error_lba[0]  = *io_bad_lba_p;
        object_p->first_media_error_lba[0] = *io_bad_lba_p;
    }


    /* increment count of read media errors injected so far
     */
    fbe_logical_error_injection_object_inc_num_errors( object_p );
    fbe_logical_error_injection_object_inc_num_rd_media_errors( object_p );

    /* trace injection of media error on this block operation
     */
    fbe_logical_error_injection_service_trace( FBE_TRACE_LEVEL_DEBUG_HIGH,
                                               object_p->object_id,
                                               "LEI: me read err type:0x%x op: %d lba: 0x%llx blks: 0x%08llx me-lba: 0x%llx\n",
                                               in_rec_p->err_type, block_opcode, (unsigned long long)block_lba, (unsigned long long)block_count, *io_bad_lba_p);

    return FBE_TRUE;

}   /* end fbe_logical_error_injection_block_io_inject_media_err_read() */


/*!**************************************************************
 * fbe_logical_error_injection_block_io_inject_media_err_write()
 ****************************************************************
 *
 * @brief
 *    This function injects a media error on a "write verify"
 *    block operation.
 * 
 * @param   block_operation_p - Pointer to block operation
 * @param   in_rec_p -  pointer to error injection record
 * @param   object_p - Poninter to error injection object information
 * @param   io_bad_lba_p   -  pointer to lba of bad i/o range
 * @param   in_bad_blocks  -  size of bad i/o range
 *
 * @return  fbe_bool_t     -  FBE_FALSE there's no error to inject
 *                            FBE_TRUE there's an error to inject
 *
 * @author
 *    04/16/2010 - Created. Randy Black
 *
 ****************************************************************/

fbe_bool_t
fbe_logical_error_injection_block_io_inject_media_err_write(fbe_payload_block_operation_t *const block_operation_p,
                                                            fbe_logical_error_injection_record_t *const in_rec_p,
                                                            fbe_logical_error_injection_object_t *const object_p,
                                                            fbe_lba_t *io_bad_lba_p,
                                                            fbe_block_count_t in_bad_blocks)
{
    fbe_block_count_t                      block_count;
    fbe_lba_t                              block_lba;
    fbe_payload_block_operation_opcode_t   block_opcode;
    fbe_lba_t                              first_error_lba;
    fbe_lba_t                              last_error_lba;

    /* extract block operation payload's opcode, lba, and block count
     */
    fbe_payload_block_get_opcode( block_operation_p, &block_opcode );
    fbe_payload_block_get_lba( block_operation_p, &block_lba );
    fbe_payload_block_get_block_count( block_operation_p, &block_count );

    /* now, use the object id to locate the associated error
     * injection object and the first/last media error lba's
     */
    first_error_lba    = object_p->first_media_error_lba[0];
    last_error_lba     = object_p->last_media_error_lba[0];

    /* true if last media error lba is in error injection range
     */
    if ( (last_error_lba >= *io_bad_lba_p)                   &&
         (last_error_lba <  (*io_bad_lba_p + in_bad_blocks )))
    {

        fbe_logical_error_injection_object_inc_num_wr_remaps( object_p );

        if ((in_rec_p->err_mode != FBE_LOGICAL_ERROR_INJECTION_MODE_INJECT_SAME_LBA ) &&
            ((last_error_lba == (*io_bad_lba_p + in_bad_blocks - 1)) ||
             (last_error_lba == (in_rec_p->lba + in_rec_p->blocks - 1))))
        {
            object_p->last_media_error_lba[0]++;
        }
        else
        {
            /* advance last media error lba to the next block if
             * the error mode is not "inject same lba"
             */
            if ( in_rec_p->err_mode != FBE_LOGICAL_ERROR_INJECTION_MODE_INJECT_SAME_LBA )
            {
                object_p->last_media_error_lba[0]++;
            }


            /* inject new error at lba of last media error
             */
            *io_bad_lba_p = object_p->last_media_error_lba[0];

            /* increment count of write media errors injected so far
             */
            fbe_logical_error_injection_object_inc_num_errors( object_p );

            /* trace injection of media error on this block operation
             */
            fbe_logical_error_injection_service_trace( FBE_TRACE_LEVEL_DEBUG_HIGH,
                                                       object_p->object_id,
                                                       "LEI: inject type: 0x%x op: 0x%x lba: 0x%llx bl: 0x%x me-lba: 0x%llx\n",
                                                       in_rec_p->err_type, block_opcode, (unsigned long long)block_lba, (unsigned int)block_count, (unsigned long long)last_error_lba);

            /* Return the fact that an error was injected.
             */
            return FBE_TRUE;
        }
    }

    /* trace the lack of injection of media error on this block operation
     */
    fbe_logical_error_injection_service_trace( FBE_TRACE_LEVEL_DEBUG_HIGH, object_p->object_id,
                                               "LEI: no write injection 0x%x op: 0x%x lba: 0x%llx bl: 0x%x me-lba: 0x%llx\n",
                                               in_rec_p->err_type, block_opcode, (unsigned long long)block_lba, (unsigned int)block_count, (unsigned long long)last_error_lba);

    /* disable this error injection record if all of following apply:
     * (1) start of error injection range matches the starting lba
     *     where errors were injected
     * (2) end of error injection range matches end of this transfer
     * (3) error mode is "inject until remapped"
     */
    if ( (in_rec_p->lba == object_p->first_media_error_lba[0]) &&
         ((in_rec_p->lba + in_rec_p->blocks) == (*io_bad_lba_p + in_bad_blocks)) &&
         (in_rec_p->err_mode == FBE_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED) )
    {
        in_rec_p->err_type = FBE_XOR_ERR_TYPE_NONE;
    }

    /* If error mode is inject until remapped and we remapped a certain block,
     * we want to prevent injecting an error to that lba again
     */
    if(in_rec_p->err_mode == FBE_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED)
    {
    
        in_rec_p->blocks -= (fbe_block_count_t)(object_p->last_media_error_lba[0]-in_rec_p->lba); 
        in_rec_p->lba = object_p->last_media_error_lba[0];
         
    }

    /* the media error lba is beyond the block operation's range, so
     * don't inject an error and set last media error lba to invalid
     */
    object_p->last_media_error_lba[0] = FBE_LBA_INVALID;
    
    /* We didn't inject an error.  Return False
     */
    return FBE_FALSE;

}   /* end fbe_logical_error_injection_block_io_inject_media_err_write() */


/*!**************************************************************
 * fbe_logical_error_injection_block_io_get_bad_lba_range()
 ****************************************************************
 *
 * @brief
 *    Determine the range within this block operation where
 *    errors can be injected.
 *
 * @param   block_operation_p - pointer to block operation
 * @param   out_lba_p     -  bad lba range start output parameter
 * @param   out_blocks_p  -  bad lba range size output parameter
 *
 * @return  void
 *
 * @author
 *    04/16/2010 - Created. Randy Black
 *
 ****************************************************************/

void
fbe_logical_error_injection_block_io_get_bad_lba_range( fbe_payload_block_operation_t *const block_operation_p,
                                                        fbe_lba_t *const out_lba_p,
                                                        fbe_block_count_t *const out_blocks_p )
{
    fbe_block_count_t                      block_count;
    fbe_lba_t                              block_lba;
    fbe_block_count_t                      blocks;
    fbe_lba_t                              lba;
    fbe_u32_t                              rec_index;
    fbe_logical_error_injection_record_t*  rec_p;
    
    /* extract block operation payload's lba and block count
     */
    fbe_payload_block_get_lba( block_operation_p, &block_lba );
    fbe_payload_block_get_block_count( block_operation_p, &block_count );

    /* set default values for bad lba range's start and size
     */
    lba    = block_lba;
    blocks = block_count;

    /* loop through error injection records
     */
    for ( rec_index = 0; rec_index < FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS; rec_index++ )
    {
        /* get pointer to selected error injection record
         */
        fbe_logical_error_injection_get_error_info( &rec_p, rec_index );

        /* insure that error injection record selects a media error
         */
        if ( (rec_p->err_type == FBE_XOR_ERR_TYPE_HARD_MEDIA_ERR) ||
             (rec_p->err_type == FBE_XOR_ERR_TYPE_SOFT_MEDIA_ERR) )
        {
            /* nothing to do if there is no overlap between the current
             * block i/o's range and the error injection record's range
             */
            if ( fbe_logical_error_injection_overlap(block_lba, 
                                                     block_count,
                                                     rec_p->lba,
                                                     rec_p->blocks) )
            {
                /* found a region to inject errors into
                 */
                if ( block_lba > rec_p->lba )
                {
                    /* inject at the first lba for this block operation;  the bad
                     * lba range ends at the minimum  of  either the total blocks
                     * in block operation or the end of the error injection range
                     */
                    blocks = (fbe_block_count_t) ( (rec_p->lba + rec_p->blocks) - block_lba );
                    blocks = FBE_MIN( blocks, block_count );
                }
                else
                {
                    /* the bad lba range is somewhere after the start of the block
                     * operation, so inject at first lba of error injection range
                     */
                    lba = rec_p->lba;

                    /* the bad lba range ends at the minimum of either the total blocks
                     * in the block operation or the end of the error injection range
                     */
                    if ( (rec_p->lba + rec_p->blocks) >= (block_lba + block_count) )
                    {
                        /* the bad lba range extends from the start of the injection to
                         * the end of the block operation
                         */
                        blocks = (fbe_block_count_t) ( (block_lba + block_count) - rec_p->lba );
                    }
                    else
                    {
                        /* the bad lba range ends at the end of error injection range 
                         */
                        blocks = rec_p->blocks;
                    }
                }

                /* output start and size of bad lba range
                 */
                *out_lba_p    = lba;
                *out_blocks_p = blocks;

                return;
            }
        }
    }

    /* output start and size of bad lba range
     */
    *out_lba_p    = lba;
    *out_blocks_p = blocks;

    return;
}   /* end fbe_logical_error_injection_block_io_get_bad_lba_range() */


/*!**************************************************************
 * fbe_logical_error_injection_invalidate_media_error_lba()
 ****************************************************************
 *
 * @brief
 *    This function invalidates the last media error lba in the
 *    error injection object associated with this packet.
 *
 * @param   in_packet_p      -  pointer to control packet
 *
 * @return  void
 *
 * @author
 *    04/16/2010 - Created. Randy Black
 *
 ****************************************************************/

void
fbe_logical_error_injection_invalidate_media_error_lba( fbe_packet_t* in_packet_p )
{
    fbe_logical_error_injection_object_t*  injection_object_p;
    fbe_object_id_t                        object_id;

    /* get id of object associated with this packet
     */
    object_id = fbe_logical_error_injection_packet_to_object_id( in_packet_p );

    /* use object id to locate associated error injection object
     */
    injection_object_p = fbe_logical_error_injection_find_object( object_id );

    /* invalidate last media error lba
     */
    injection_object_p->last_media_error_lba[0] = FBE_LBA_INVALID;

    return;
}   /* end fbe_logical_error_injection_invalidate_media_error_lba() */


/*!**************************************************************
 * fbe_logical_error_injection_packet_to_block_operation()
 ****************************************************************
 *
 * @brief
 *    This function returns the block operation payload from the
 *    specified packet.
 *
 * @param   in_packet_p  -  pointer to control packet
 *
 * @return  fbe_payload_block_operation_t
 *
 * @author
 *    04/16/2010 - Created. Randy Black
 *
 ****************************************************************/

fbe_payload_block_operation_t*
fbe_logical_error_injection_packet_to_block_operation( fbe_packet_t* in_packet_p )
{
    fbe_payload_block_operation_t*  block_operation_p;
    fbe_payload_ex_t*               payload_p;

    /* get payload type for this packet
     */

    /* get the block operation from payload */
    payload_p         = fbe_transport_get_payload_ex( in_packet_p );
    block_operation_p = fbe_payload_ex_get_block_operation( payload_p );

    /* now, return block operation
     */
    return block_operation_p;

}   /* end fbe_logical_error_injection_packet_to_block_operation() */


/*!**************************************************************
 * fbe_logical_error_injection_packet_to_object_id()
 ****************************************************************
 *
 * @brief
 *    This function returns the id for the associated object in
 *    the specified packet's completion context.
 *
 * @param   in_packet_p      -  pointer to control packet
 *
 * @return  fbe_object_id_t  -  object id
 *
 * @author
 *    04/16/2010 - Created. Randy Black
 *
 ****************************************************************/

fbe_object_id_t
fbe_logical_error_injection_packet_to_object_id( fbe_packet_t* in_packet_p )
{
    fbe_object_id_t client_id;

    if (in_packet_p->base_edge != NULL)
    {
        fbe_base_transport_get_client_id(in_packet_p->base_edge, &client_id);
        return client_id;
    }
    else
    {
        return FBE_OBJECT_ID_INVALID;
    }
}   /* end fbe_logical_error_injection_packet_to_object_id() */

/*!**************************************************************
 * fbe_logical_error_injection_get_class_id()
 ****************************************************************
 * @brief
 *   Get the class id of the object
 *
 * @param  packet_p - pointer to packet 
 *         object_id - pointer to object  
 *
 * @return fbe_status_t   
 *
 * @author
 *  2/24/2012 - Created. Deanna Heng
 *
 ****************************************************************/
void fbe_logical_error_injection_get_client_id(fbe_packet_t * packet_p, fbe_object_id_t * object_id) 
{
    fbe_payload_ex_t                        *payload_p = NULL;
    fbe_payload_block_operation_t           *block_operation_p = NULL;
    fbe_raid_fruts_t                        *fruts_p = NULL;
    fbe_status_t                            status = FBE_STATUS_OK;

    status = fbe_logical_error_injection_validate_fruts(packet_p, &fruts_p);
    if (status == FBE_STATUS_OK) 
    {
        payload_p         = fbe_transport_get_payload_ex( packet_p );
        block_operation_p = fbe_payload_ex_get_block_operation( payload_p );
        *object_id = block_operation_p->block_edge_p->base_edge.client_id;
    } else 
    {
        *object_id = fbe_logical_error_injection_packet_to_object_id(packet_p);
    }
} /* end fbe_logical_error_injection_object_id() */

/*****************************************
 * end of fbe_logical_error_injection_block_operations.c
 *****************************************/

