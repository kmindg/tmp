#ifndef FBE_SG_ELEMENT_H
#define FBE_SG_ELEMENT_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * fbe_sg_element.h
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains the definition and api for the fbe sg element.
 *
 * HISTORY
 *   11/09/2007:  Ported to FBE From Flare. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"

/* This is the fixed sg size for the sg embedded in the sg descriptor.
 */
#define FBE_SG_DESCRIPTOR_EMBEDDED_SG_LENGTH 3

/* This structure defines one element of a scatter gather list.
 * A list is logically an array of these structures.
 * Currenty this definition taken typedef _NTBE_SGL 
 */
#pragma pack(4) /*sharel:this is only temporary until we resolve that with Flare, this way we can run nice both on 64 and 32 bits usersim*/
typedef struct fbe_sg_element_s {
    fbe_u32_t			count;/*keep first so it matches the data layout in PSGL we receive from SP cache*/
	fbe_u8_t *		    address;
} fbe_sg_element_t;
#pragma pack()

static __forceinline void fbe_sg_element_init(fbe_sg_element_t * const sg_p,
                                       const fbe_u32_t count,
                                       void * address)
{
    sg_p->count = count;
    sg_p->address = (fbe_u8_t *) address;
    return;
}

static __forceinline fbe_u32_t fbe_sg_element_count(fbe_sg_element_t * sg_p)
{
    return sg_p->count;
}

static __forceinline fbe_u8_t* fbe_sg_element_address(fbe_sg_element_t * sg_p)
{
    return sg_p->address;
}

static __forceinline void fbe_sg_element_terminate(fbe_sg_element_t * const sg_p)
{
    sg_p->count = 0;
    sg_p->address = NULL;
    return;
}

static __forceinline void fbe_sg_element_increment(fbe_sg_element_t ** sg_p)
{
    (*sg_p)++;
    return;
}

static __forceinline fbe_status_t fbe_sg_element_copy(fbe_sg_element_t * const source_sg_p,
                                               fbe_sg_element_t * const dest_sg_p )
{
    fbe_status_t status = FBE_STATUS_OK;
        
    /* Just perform the copy from source to destination.
     */
    *dest_sg_p = *source_sg_p;
    return status;
}

typedef struct fbe_sg_descriptor_with_offset_s
{
    /* Offset of the number of sg entries.
     */
    fbe_u32_t offset;
    /* Total number of sg entries in the list.
     */
    fbe_u32_t length;

    /* Always points to the top of the sg list even
     * when there might be an offset. 
     */
    fbe_sg_element_t *sg_ptr;
} fbe_sg_descriptor_with_offset_t;

/* This is the structure for the FBE_SG_TYPE_COMPRESSED_SG_ENTRY type.
 * 
 * This represents an sg where the contents of a given embedded sg element
 * need to be repeated repeat_count times. 
 */
typedef struct fbe_sg_descriptor_compressed_sg_entry_s
{
    fbe_u32_t repeat_count;

    fbe_sg_element_t sg[2];

} fbe_sg_descriptor_compressed_sg_entry_t;

/* This is the structure for the FBE_SG_TYPE_COMPRESSED_SG sg type. 
 * This represents an sg where the contents of a given sg list 
 * need to be repeated repeat_count times. 
 * We assume this sg list is 0, NULL terminated. 
 */
typedef struct fbe_sg_descriptor_compressed_sg_s
{
    fbe_u32_t repeat_count;

    fbe_sg_element_t *sg_p;

} fbe_sg_descriptor_compressed_sg_t;

/* This defines the type of an sg descriptor. 
 *  
 * The sg descriptor defines many different types so that we can be flexible 
 * about how we represent data with sg lists. 
 */
typedef enum fbe_sg_type_s
{
    FBE_SG_TYPE_INVALID = 0,
    FBE_SG_TYPE_SG_WITH_OFFSET = 1,
    FBE_SG_TYPE_SG_ENTRY,
    FBE_SG_TYPE_COMPRESSED_SG_ENTRY,
    FBE_SG_TYPE_COMPRESSED_SG,
} fbe_sg_type_t;

/* The sg descriptor allows us to define flexible
 * scatter gather lists. 
 *  
 * The setters/getters should be use to access this object. 
 */
typedef struct fbe_sg_descriptor_s
{
    /* Specifies the field in the union we are using.
     */
    fbe_sg_type_t sg_type;

    union
    {
        fbe_sg_descriptor_with_offset_t sg_with_offset;
        fbe_sg_element_t sg_entry[3];
        fbe_sg_descriptor_compressed_sg_entry_t compressed_sg_entry;
        fbe_sg_descriptor_compressed_sg_t compressed_sg;
    } desc;

} fbe_sg_descriptor_t;

/* Initialize the sg descriptor to use type sg with offset. 
 * This initializes all the members that are needed for 
 * this sg descriptor type. 
 */
static __forceinline void fbe_sg_descriptor_init_with_offset(fbe_sg_descriptor_t *sg_desc_p,
                                                      fbe_u32_t offset,
                                                      fbe_u32_t length,
                                                      fbe_sg_element_t *sg_p)
{
    sg_desc_p->sg_type = FBE_SG_TYPE_SG_WITH_OFFSET;
    sg_desc_p->desc.sg_with_offset.offset = offset;
    sg_desc_p->desc.sg_with_offset.length = length;
    sg_desc_p->desc.sg_with_offset.sg_ptr = sg_p;
    return;
}

/* Initialize the sg descriptor to use type SG_ENTRY.
 * This initializes all the members that are needed for this sg descriptor type.
 */
static __forceinline void fbe_sg_descriptor_init_sg_entry(fbe_sg_descriptor_t *sg_desc_p,
                                                   fbe_sg_element_t *sg_p)
{
    /* For now we assume a single entry sg.
     */
    sg_desc_p->sg_type = FBE_SG_TYPE_SG_ENTRY;
    sg_desc_p->desc.sg_entry[0] = *sg_p;
    fbe_sg_element_terminate(&sg_desc_p->desc.sg_entry[1]);
    return;
}

/* Initialize the sg descriptor to use type compressed sg entry. This
 * initializes all the members that are needed for this sg descriptor type. 
 */
static __forceinline void 
fbe_sg_descriptor_init_compressed_sg_entry(fbe_sg_descriptor_t *sg_desc_p,
                                           fbe_u32_t repeat_count,
                                           fbe_sg_element_t *sg_p)
{
    sg_desc_p->sg_type = FBE_SG_TYPE_COMPRESSED_SG_ENTRY;
    sg_desc_p->desc.compressed_sg_entry.repeat_count = repeat_count;

    /* Copy over just this one sg entry.
     * We always terminate this sg so that we can use standard sg list routines 
     * to traverse it. 
     */
    sg_desc_p->desc.compressed_sg_entry.sg[0] = *sg_p;
    fbe_sg_element_terminate(&sg_desc_p->desc.compressed_sg_entry.sg[1]); 
    return;
}

/* Initialize the sg descriptor to use type compressed sg.
 * This initializes all the members that are needed for this sg descriptor type.
 */
static __forceinline void 
fbe_sg_descriptor_init_compressed_sg(fbe_sg_descriptor_t *sg_desc_p,
                                     fbe_u32_t repeat_count,
                                     fbe_sg_element_t *sg_p)
{
    sg_desc_p->sg_type = FBE_SG_TYPE_COMPRESSED_SG;
    sg_desc_p->desc.compressed_sg.repeat_count = repeat_count;
    sg_desc_p->desc.compressed_sg.sg_p = sg_p;
    return;
}

static __forceinline fbe_sg_type_t 
fbe_sg_descriptor_get_sg_type(fbe_sg_descriptor_t *sg_desc_p)
{
    return sg_desc_p->sg_type;
}
static __forceinline void
fbe_sg_descriptor_set_sg_type(fbe_sg_descriptor_t *sg_desc_p, fbe_sg_type_t sg_type)
{
    sg_desc_p->sg_type = sg_type;
}

/* Getter functions for each of the different sg descriptor types.
 */
static __forceinline fbe_sg_descriptor_with_offset_t *
fbe_sg_descriptor_get_sg_with_offset(fbe_sg_descriptor_t *sg_desc_p)
{
    return &sg_desc_p->desc.sg_with_offset;
}
static __forceinline fbe_sg_descriptor_compressed_sg_t *
fbe_sg_descriptor_get_compressed_sg(fbe_sg_descriptor_t *sg_desc_p)
{
    return &sg_desc_p->desc.compressed_sg;
}
static __forceinline fbe_sg_descriptor_compressed_sg_entry_t *
fbe_sg_descriptor_get_compressed_sg_entry(fbe_sg_descriptor_t *sg_desc_p)
{
    return &sg_desc_p->desc.compressed_sg_entry;
}

static __forceinline fbe_sg_element_t *
fbe_sg_descriptor_get_sg_ptr(fbe_sg_descriptor_t *sg_desc_p)
{
    fbe_sg_element_t *sg_p = NULL;

    switch (sg_desc_p->sg_type)
    {
        case FBE_SG_TYPE_SG_WITH_OFFSET:
            sg_p = sg_desc_p->desc.sg_with_offset.sg_ptr;
            if (sg_p != NULL)
            {
                sg_p += sg_desc_p->desc.sg_with_offset.offset;
            }
            break;

        case FBE_SG_TYPE_SG_ENTRY:
            sg_p = sg_desc_p->desc.sg_entry;
            break;
        case FBE_SG_TYPE_COMPRESSED_SG:
            sg_p = sg_desc_p->desc.compressed_sg.sg_p;
            break;
        case FBE_SG_TYPE_COMPRESSED_SG_ENTRY:
            sg_p = sg_desc_p->desc.compressed_sg_entry.sg;
            break;
        case FBE_SG_TYPE_INVALID:
        default:
            break;
    };
    return sg_p;
}

static __forceinline fbe_sg_element_t *
fbe_sg_descriptor_get_sg_ptr_without_offset(fbe_sg_descriptor_t *sg_desc_p)
{
    fbe_sg_element_t *sg_p = NULL;

    switch (sg_desc_p->sg_type)
    {
        case FBE_SG_TYPE_SG_WITH_OFFSET:
            sg_p = sg_desc_p->desc.sg_with_offset.sg_ptr;
            break;

        case FBE_SG_TYPE_SG_ENTRY:
            sg_p = sg_desc_p->desc.sg_entry;
            break;
        case FBE_SG_TYPE_COMPRESSED_SG:
            sg_p = sg_desc_p->desc.compressed_sg_entry.sg;
            break;
        case FBE_SG_TYPE_COMPRESSED_SG_ENTRY:
            sg_p = sg_desc_p->desc.compressed_sg.sg_p;
            break;
        case FBE_SG_TYPE_INVALID:
        default:
            break;
    };
    return sg_p;
}
static __forceinline fbe_u32_t
fbe_sg_descriptor_get_length(fbe_sg_descriptor_t *sg_desc_p)
{
    fbe_u32_t length = 0;

    switch (sg_desc_p->sg_type)
    {
        case FBE_SG_TYPE_SG_WITH_OFFSET:
            /* Offset for this sg type is settable.
             */
            length = sg_desc_p->desc.sg_with_offset.length;
            break;

        case FBE_SG_TYPE_SG_ENTRY:
            /* Cannot specify an offset with this type of sg yet.
             */
            length = FBE_SG_DESCRIPTOR_EMBEDDED_SG_LENGTH;
            break;
        case FBE_SG_TYPE_INVALID:
        default:
            break;
    };
    return length;
}
static __forceinline fbe_u32_t
fbe_sg_descriptor_get_offset(fbe_sg_descriptor_t *sg_desc_p)
{
    fbe_u32_t offset = 0;

    switch (sg_desc_p->sg_type)
    {
        case FBE_SG_TYPE_SG_WITH_OFFSET:
            /* Offset for this sg type is settable.
             */
            offset = sg_desc_p->desc.sg_with_offset.offset;
            break;

        case FBE_SG_TYPE_COMPRESSED_SG:
        case FBE_SG_TYPE_COMPRESSED_SG_ENTRY:
        case FBE_SG_TYPE_SG_ENTRY:
            /* Cannot specify an offset with this type of sg.
             */
            offset = 0;
            break;
        case FBE_SG_TYPE_INVALID:
        default:
            break;
    };
    return offset;
}

/* Get the repeat count of the sg descriptor. 
 * This is the number of times the data in the sg list is repeated. 
 */
static __forceinline fbe_u32_t
fbe_sg_descriptor_get_repeat_count(fbe_sg_descriptor_t *sg_desc_p)
{
    /* Only two sg descriptor types can specify a repeat count. 
     * For the rest the default repeat count is simply 1. 
     */
    fbe_u32_t repeat_count = 0; 

    switch (sg_desc_p->sg_type)
    {
        /* The first two types support a repeat count.
         */
        case FBE_SG_TYPE_COMPRESSED_SG:
            repeat_count = sg_desc_p->desc.compressed_sg.repeat_count;
            break;
        case FBE_SG_TYPE_COMPRESSED_SG_ENTRY:
            repeat_count = sg_desc_p->desc.compressed_sg_entry.repeat_count;
            break;
        case FBE_SG_TYPE_SG_WITH_OFFSET:
        case FBE_SG_TYPE_SG_ENTRY:
        case FBE_SG_TYPE_INVALID:
        default:
            /* These cannot specify a repeat count, but
             * implicitly the data is repeated once. 
             */
            repeat_count = 1;
            break;
    };
    return repeat_count;
}
/* Set the repeat count of the sg descriptor. This is the number of times the
 * data in the sg list is repeated. 
 */
static __forceinline fbe_status_t
fbe_sg_descriptor_set_repeat_count(fbe_sg_descriptor_t *sg_desc_p,
                                   fbe_u32_t repeat_count)
{
    switch (sg_desc_p->sg_type)
    {
        /* The first two types support a repeat count.
         */
        case FBE_SG_TYPE_COMPRESSED_SG:
            sg_desc_p->desc.compressed_sg.repeat_count = repeat_count;
            break;
        case FBE_SG_TYPE_COMPRESSED_SG_ENTRY:
            sg_desc_p->desc.compressed_sg_entry.repeat_count = repeat_count;
            break;
        case FBE_SG_TYPE_SG_WITH_OFFSET:
        case FBE_SG_TYPE_SG_ENTRY:
        case FBE_SG_TYPE_INVALID:
        default:
            /* These cannot specify a repeat count.
             */
            return FBE_STATUS_GENERIC_FAILURE;
            break;
    };
    return FBE_STATUS_OK;
}

/*!*******************************************************************
 * @type fbe_sg_element_get_next_fn_t
 *********************************************************************
 * @brief This type defines the function to fetch the next
 *        sg element.
 *
 *********************************************************************/
typedef fbe_sg_element_t * (*fbe_sg_element_get_next_fn_t) (fbe_sg_element_t *);

/*!*******************************************************************
 * @struct fbe_sg_element_with_offset_t
 *********************************************************************
 * @brief This struct allows us to keep track of a precise
 *        position within an sg element by adding an offset.
 *        We also can vary how we transition to the next
 *        sg element with the use of a get next function.
 *
 *********************************************************************/
typedef struct fbe_sg_element_with_offset_s
{
    fbe_sg_element_t *sg_element;
    fbe_u32_t offset;
    /* Function to fetch the next sg element.
     */
    fbe_sg_element_get_next_fn_t get_next_sg_fn;
} 
fbe_sg_element_with_offset_t;

fbe_sg_element_t * fbe_sg_element_with_offset_get_next_sg(fbe_sg_element_t *sg_p);
fbe_sg_element_t * fbe_sg_element_with_offset_get_current_sg(fbe_sg_element_t *sg_p);

static __forceinline void fbe_sg_element_with_offset_init(fbe_sg_element_with_offset_t *sgd_p,
                                                          fbe_u32_t offset,
                                                          fbe_sg_element_t *sg_element_p,
                                                          fbe_sg_element_get_next_fn_t get_next_sg_fn)
{
    sgd_p->offset = offset;
    sgd_p->sg_element = sg_element_p;

    /* By default we fill in the fn to just get the next sg.
     */
    if (get_next_sg_fn == NULL)
    {
        sgd_p->get_next_sg_fn = fbe_sg_element_with_offset_get_next_sg;
    }
    else
    {
        /* Otherwise use what the client passed in.
         */
        sgd_p->get_next_sg_fn = get_next_sg_fn;
    }
    return;
}
/* Other api functions defined in fbe_sg_element.c
 */
fbe_status_t fbe_sg_element_copy_bytes(fbe_sg_element_t * const source_sg_p,
                                                     fbe_sg_element_t * const dest_sg_p,
                                       fbe_u32_t bytes );
fbe_u32_t fbe_sg_element_count_entries(fbe_sg_element_t *sg_element_p);
fbe_u32_t fbe_sg_element_count_entries_for_bytes(fbe_sg_element_t *sg_element_p,
                                       fbe_u32_t bytes);
fbe_u32_t fbe_sg_element_count_entries_with_offset(fbe_sg_element_t *sg_element_p,
                                                   fbe_u32_t bytes,
                                                   fbe_u32_t offset_bytes);
fbe_sg_element_t * fbe_sg_element_copy_list(fbe_sg_element_t *source_sg_p,
                                            fbe_sg_element_t *dest_sg_p,
                                            fbe_u32_t bytes);
fbe_u32_t fbe_sg_element_count_list_bytes(fbe_sg_element_t * sg_p);
fbe_bool_t fbe_sg_element_bytes_is_exact_match(fbe_sg_element_t *sg_element_p, fbe_u64_t expected_byte_count);

fbe_sg_element_t * fbe_sg_element_copy_list_with_offset(fbe_sg_element_t *source_sg_p,
                                                        fbe_sg_element_t *dest_sg_p,
                                                        fbe_u32_t bytes,
                                                        fbe_u32_t offset_bytes);fbe_u32_t 
fbe_sg_element_count_entries_for_bytes_with_remainder(fbe_sg_element_t *sg_element_p,
                                                      fbe_u32_t bytes,
                                                      fbe_bool_t *b_remainder_p);

fbe_u32_t fbe_sg_element_count_entries_and_bytes(fbe_sg_element_t * sg_element_p,
                                                 fbe_u64_t max_bytes,
                                                 fbe_u64_t *bytes_found_p);

void fbe_sg_element_adjust_offset(fbe_sg_element_t **sg_pp, fbe_u32_t *offset_p);
#endif /* FBE_SG_ELEMENT_H */
