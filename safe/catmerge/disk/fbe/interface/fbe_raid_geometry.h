#ifndef _FBE_RAID_GEOMETRY_H_
#define _FBE_RAID_GEOMETRY_H_
/*******************************************************************
 * Copyright (C) EMC Corporation 2000, 2006-2007
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 *******************************************************************/

/*!***************************************************************************
 *          fbe_raid_geometry.h
 *****************************************************************************
 *
 * @brief   The raid geometry structure contains complete and sufficient
 *          information that is required by the raid library.  The raid library
 *          will generate the necessary request for the packet received using
 *          the information from the geometry which describes the raid group
 *          the I/O is sent to.
 *
 * @author
 *  11/05/2009  Ron Proulx  - Refactored from fbe_raid_geometry.h
 *
 *******************************************************************/

/*************************
 * INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_raid_group.h"     /* This is need for the raid type */
#include "fbe_block_transport.h"    /* This is needed for the block edge definition */
#include "fbe_metadata.h"
#include "fbe_mirror_geometry.h"
#include "fbe_parity_write_log.h"

/* Define the RAID5 data-parity layout as either
 * Left symmetric or right assymetric.
 */
#define LU_RAID5_RIGHT_SYMMETRIC
/*
 * #define LU_RAID5_RIGHT_SYMMETRIC
 * Below is defined the left symmetric
 * data-parity mapping.
 * Note that parity rotates to the right.
 * This data-parity layout will be used 
 * starting with the Catapult product.
 *
 * P  3  2  1  0
 * 4  P  7  6  5  
 * 9  8  P  11 10
 * 14 13 12 P  15
 * 19 18 17 16 P
 *
 */

/*
 * #define LU_RAID5_LEFT_SYMMETRIC
 * Below is defined the left symmetric
 * data-parity mapping.
 * Note that parity rotates to the left.
 * This mapping was used in the early prototype
 * and is not targeted for release on any product.
 * 0  1  2  3  P
 * 5  6  7  P  4
 * 10 11 P  8  9
 * 15 P  12 13 14
 * P  16 17 18 19
 */    


/* 
 * #define LU_RAID5_RIGHT_ASSYMETRIC
 * Right Asymetric has parity rotating to the right.
 * This is the "Classic" data-parity layout, and
 * is *not* an ideal layout. This will be used
 * through the Longbow product.
 * P  1  2  3  4
 * 5  P  6  7  8 
 * 9  10 P  11 12
 * 13 14 15 P  16
 * 17 18 19 20 P
 */
 

typedef enum _RAID5_DATA_LAYOUT
{
    RIGHT_ASYMMETRIC,
    LEFT_ASYMMETRIC,
    RIGHT_SYMMETRIC,
    LEFT_SYMMETRIC,
}
RAID5_DATA_LAYOUT;


/**************************
 * Examples of geometry structure use.
 **************************/

/**********************************************************************
 * Geometry Interface Macros
 **********************************************************************
 *
 * The below macros are defined for each of 3 different
 * data/parity mappings:
 * LU_RAID5_RIGHT_ASSYMETRIC
 * LU_RAID5_RIGHT_SYMMETRIC - Supports R5 and R6
 * LU_RAID5_LEFT_SYMMETRIC
 *
 * LU_DATA_POSITION macro maps from an actual position to a 
 * data relative logical position.
 *
 * LU_EXTENT_POSITION macro maps from an actual position to a
 * position in the geometry EXTENT structure.
 * The EXTENT structure has data drives in array positions
 * 0..N-2 and N-1 is the parity position, where N is array_width.
 **********************************************************************/

#ifdef LU_RAID5_RIGHT_ASSYMETRIC
/********************************************************
 * LU_DATA_POSITION
 *
 * Convert a data position which includes parity to a
 * data position which does not include parity.
 ********************************************************/
static __inline fbe_status_t LU_DATA_POSITION(fbe_u32_t position,
                                  fbe_u32_t parity_position,
                                  fbe_u32_t width,
                                  fbe_u32_t *logical_data_pos_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    if(position == parity_position)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* The position must be within the width.
     */
    if(position >= width)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if(position >= parity_position)
    {
        *logical_data_pos_p = position - 1;
    }
    else
    {
        *logical_data_pos_p = position;
    }
    return status;
}
/********************************************************
 * LU_EXTENT_POSITION
 *
 * Convert a parity or data position into
 * an index into the extent structure.
 ********************************************************/
static __inline fbe_status_t LU_EXTENT_POSITION(fbe_u32_t position,
                                  fbe_u32_t parity_position,
                                  fbe_u32_t width,
                                  fbe_u32_t *logical_data_pos_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    if ( position == parity_position)
    {
       *logical_data_pos_p = (width) - 1
    }
    else
    {
        status = LU_DATA_POSITION(position, parity_position, width, logical_data_pos_p);
        if(status != FBE_STATUS_OK)
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    return status;
}

#elif defined(LU_RAID5_LEFT_SYMMETRIC)

/********************************************************
 * LU_DATA_POSITION
 *
 * Convert a data position which includes parity to a
 * data position which does not include parity.
 ********************************************************/
static __inline fbe_status_t LU_DATA_POSITION(fbe_u32_t position,
                                  fbe_u32_t parity_position,
                                  fbe_u32_t width,
                                  fbe_u32_t *logical_data_pos_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    if(position == parity_position)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* The position must be within the width.
     */
    if(position >= width)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if(position > parity_position)
    {
        if(( position - (parity_position + 1)) >= (width-1))
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }
        else
        {
            *logical_data_pos_p = (position - (parity_position + 1))
        }
    }
    else
    {
        if(( (position +width)- (parity_position + 1)) >= (width-1))
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }
        else
        {
            *logical_data_pos_p = ((position+width) - (parity_position + 1))
        }
    }
    return status;
}

/********************************************************
 * LU_EXTENT_POSITION
 *
 * Convert a parity or data position into
 * an index into the extent structure.
 ********************************************************/
static __inline fbe_status_t LU_EXTENT_POSITION(fbe_u32_t position,
                                  fbe_u32_t parity_position,
                                  fbe_u32_t width,
                                  fbe_u32_t *logical_data_pos_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    if ( position == parity_position)
    {
       *logical_data_pos_p = (width) - 1
    }
    else
    {
        status = LU_DATA_POSITION(position, parity_position, width, logical_data_pos_p);
        if(status != FBE_STATUS_OK)
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    return status;
}
#elif defined(LU_RAID5_RIGHT_SYMMETRIC)

/********************************************************
 * LU_DATA_POSITION
 *
 * Convert a physical data position which includes parity to a
 * logical data position which does not include parity.
 *
 * This macro converts from a physical position in the array
 * to a logical data position within the EXTENT structure.
 *
 ********************************************************/

static __inline fbe_status_t LU_DATA_POSITION(fbe_u32_t position,
                                  fbe_u32_t parity_position,
                                  fbe_u32_t width,
                                  fbe_u32_t parities,
                                  fbe_u32_t *logical_data_pos_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    
    if(position == parity_position)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* We either have a single parity drive,
     * or we have two parity drives and
     * our position is not equal to the second parity position.
     */
    if((parities != 1) && (position == (parity_position + 1) % width) )
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* The position must be within the width.
     */
    if(position >= width)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Depending on which side of parity we are, we'll calculate differently.
     */
    if (position > parity_position)
    {
        /* Beyond parity, add on the width.
         */
        *logical_data_pos_p = parity_position - 1 + width - position;
    }
    else 
    {
        *logical_data_pos_p = parity_position - 1 - position;
    }
    
    /* Make sure our data position is within the range of valid
     * valid data positions.
     */
    if(*logical_data_pos_p >= (width - parities))
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/* end LU_DATA_POSITION */

/********************************************************
 * LU_EXTENT_POSITION
 *
 * Convert a parity or data position into
 * an index into the extent structure.
 * Convert a physical data or parity position which includes 
 * parity to a logical data position which does not include parity.
 *
 * If we have a parity position, then we will
 * simply give one of the last 2 positions where parity
 * will be located.  R5 uses the last position for parity
 * and R6 uses that last 2 extent positions for parity.
 ********************************************************/

static __inline fbe_status_t LU_EXTENT_POSITION(fbe_u32_t position,
                                  fbe_u32_t parity_position,
                                  fbe_u32_t width,
                                  fbe_u32_t parities,
                                  fbe_u32_t *logical_data_pos_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    
    if( position == parity_position)
    {
        *logical_data_pos_p  = width - parities;
    }
    else
    {
        if( (parities > 1) && (position == ((parity_position + 1) % width )) ) 
        {
            *logical_data_pos_p = width - 1;
        }
        else
        {
            status = LU_DATA_POSITION(position, parity_position, width, parities, logical_data_pos_p);
            if(status != FBE_STATUS_OK)
            {
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
    }
    return status;
}
#else
#error "fbe_raid_geometry.h: No Data Layout Defined!"
#endif

/***************************************************
 *  FBE_RAID_MAX_BACKEND_IO_BLOCKS defines the largest size IO
 *  in sectors that can be sent to a drive.
 ***************************************************/
#define FBE_RAID_MAX_BACKEND_IO_BLOCKS 1024

/********************************************************
 * FBE_RAID_DATA_POSITION
 *
 * Convert a data position which includes parity to a
 * data position which does not include parity.
 ********************************************************/

#define FBE_RAID_DATA_POSITION(m_position,m_parity_position,m_width,m_parities,m_logical_data_pos)\
LU_DATA_POSITION(m_position, m_parity_position, m_width, m_parities, m_logical_data_pos)


/********************************************************
 * FBE_RAID_EXTENT_POSITION
 *
 * Convert a parity or data position into
 * an index into the extent structure.
 ********************************************************/

#define FBE_RAID_EXTENT_POSITION(m_position,m_parity_position,m_width,m_parities, m_logical_data_pos) \
LU_EXTENT_POSITION(m_position, m_parity_position, m_width, m_parities, m_logical_data_pos)

/*! @enum fbe_raid_geometry_flags_t
 *  
 *  @brief These flags indicate the state of the geometry data.
 *         The geometry must first be initialized and then configured
 *         etc.
 */
typedef enum fbe_raid_geometry_flags_e
{
    /*! No flags set.
     */
    FBE_RAID_GEOMETRY_FLAG_INVALID              = 0x00000000,

    /*! The raid geometry has been initialized (still needs
     *  to be configured before I/O is allowed).
     */
    FBE_RAID_GEOMETRY_FLAG_INITIALIZED          = 0x00000001,

    /*! The raid geometry has been configued.
     */
    FBE_RAID_GEOMETRY_FLAG_CONFIGURED           = 0x00000002,

    /*! The raid geometry metadata has been configured
     */
    FBE_RAID_GEOMETRY_FLAG_METADATA_CONFIGURED  = 0x00000004,

    /*! At least (1) block size negotiate has completed successfully.
     */
    FBE_RAID_GEOMETRY_FLAG_NEGOTIATE_COMPLETE   = 0x00000008,

    /*! The block size information is valid (i.e. negotiation has
     *  has completed successfully).
     */
    FBE_RAID_GEOMETRY_FLAG_BLOCK_SIZE_VALID     = 0x00000010,

    /*! The negotiate failed for at least one position.
     */
    FBE_RAID_GEOMETRY_FLAG_NEGOTIATE_FAILED     = 0x00000020,

    /*! This flag set by RAW MIRROR only to not send Usurper to PDO in case
     *  of RAW MIRROR IO complete with CRC error
     */
    FBE_RAID_GEOMETRY_FLAG_RAW_MIRROR = 0x00000040,

    /*! There is no raid group object for this raid geometry. 
     * Raid is being used strictly as a library. 
     */
     FBE_RAID_GEOMETRY_NO_OBJECT  = 0x00000080,
     FBE_RAID_GEOMETRY_FLAG_MIRROR_SEQUENTIAL_DISABLED  = 0x00000100,

    /*! The block size information is invalid.
     */
     FBE_RAID_GEOMETRY_FLAG_BLOCK_SIZE_INVALID  = 0x00000200,

}fbe_raid_geometry_flags_t;

/*! @enum fbe_raid_attribute_flags_t
 *  
 *  @brief These are the different attributes of the raid group
 *
 *  @note This is needed so that we can validate that the correct
 *        algorithms are working on the correct raid group type.
 */

typedef enum fbe_raid_attribute_flags_e
{
    FBE_RAID_ATTRIBUTE_NONE                 = 0x00000000,

    /*! This raid group has an optimal size that is not 1.
     */
    FBE_RAID_ATTRIBUTE_NON_OPTIMAL          = 0x00000001, 

    /*! This raid group is pro-actively sparing
     */
    FBE_RAID_ATTRIBUTE_PROACTIVE_SPARING    = 0x00000002,

    /*! Write_logging is enabled for this raid group
     */
    FBE_RAID_ATTRIBUTE_VAULT                = 0x00000004,

    /*! Tells RAID to use extent metadata for edges and offsets.
     */
    FBE_RAID_ATTRIBUTE_EXTENT_POOL          = 0x00000008,

    /*! This raid group is external used.
     */
    FBE_RAID_ATTRIBUTE_C4_MIRROR            = 0x00000010,

}fbe_raid_attribute_flags_t;

typedef enum fbe_mirror_prefered_position_e
{
    /*! By default mirror prefered position will be invalid.
     */
    FBE_MIRROR_PREFERED_POSITION_INVALID        = 0x0,

    /*! Mirror prefered position will be set to one for the virtual drive
     *  object if it is mirror with spared edge as one.
     */
    FBE_MIRROR_PREFERED_POSITION_ONE            = 0x1,

    /*! Mirror prefered position will be set to two for the virtual drive
     *  object if it is mirror with spared edge as two.
     */
    FBE_MIRROR_PREFERED_POSITIOM_TWO            = 0x2

} fbe_mirror_prefered_position_t;

/*!*******************************************************************
 * @def FBE_RAID_GROUP_WIDTH_INVALID
 *********************************************************************
 * @brief
 *  This represents a raid group width.
 *********************************************************************/
#define FBE_RAID_GROUP_WIDTH_INVALID ((fbe_u32_t)-1)


/*!*******************************************************************
 * @struct fbe_raid_geometry_journal_info_t
 *********************************************************************
 * @brief This is the raid geo info related to journaling,
 *        which is only used on parity raid groups.
 *
 *********************************************************************/
typedef struct fbe_raid_geometry_journal_info_s
{
    /*! This is a pointer to the write log structure.  This is
     *  required so that the raid algorithms can do journaling.
     */
    fbe_parity_write_log_info_t       *write_log_info_p;
}
fbe_raid_geometry_journal_info_t;
/*!*******************************************************************
 * @struct fbe_raid_geometry_raw_mirror_info_t
 *********************************************************************
 * @brief This is the raid geo info related to journaling,
 *        which is only used on parity raid groups.
 *
 *********************************************************************/
typedef struct fbe_raid_geometry_raw_mirror_info_s
{
    /*! Offset to a physical address.
     */
    fbe_lba_t raw_mirror_physical_offset;

    /*! Offset on the raid group needed for generating correct lba stamp.
     */
    fbe_lba_t raw_mirror_rg_offset; 
}
fbe_raid_geometry_raw_mirror_info_t;
/*!*******************************************************************
 * @struct fbe_raid_geometry_t
 *********************************************************************
 * @brief
 *  This structure contains the methods neccessary to execute raid I/O
 *
 *********************************************************************/
typedef struct fbe_raid_geometry_s
{
    fbe_object_id_t         object_id;  /*!< Object id of object for logging only. */
    fbe_class_id_t          class_id;   /*!< Class id for sanity checking against the raid type */

    fbe_raid_geometry_flags_t   geometry_flags;

    fbe_raid_library_debug_flags_t debug_flags; /*!< Debugging flags. */

    fbe_raid_group_type_t   raid_type;

    fbe_raid_attribute_flags_t attribute_flags;

    fbe_mirror_prefered_position_t prefered_mirror_pos; /*!< Prefered position from where we like to read in case of mirror(VD). */
    fbe_mirror_optimization_t *mirror_optimization_p; /*!< Mirror read optimization structure. */

    /*! This is the generate state for new siots. 
     */
    fbe_raid_common_state_t generate_state;

    fbe_raid_position_bitmask_t bitmask_4k; /*!< Bitmask of 4k aligned positions. */

    fbe_block_size_t        exported_block_size;    /*!< The logical block size that RAID uses */

    /*! The number of logical blocks that will result in an aligned physical request. 
     */  
    fbe_block_size_t        optimal_block_size;     

    /*! The worst case (i.e. largest optimal block size) imported block size for this RAID
     *  group.  
     */
    fbe_block_size_t        imported_block_size;

    /*! The maximum number of blocks that we can send per drive request.
     */
    fbe_block_count_t       max_blocks_per_drive;

    /*! This is a pointer to the base object downstream block edges.  This is
     *  required so that the raid algorithms can send packets.
     */
    fbe_block_edge_t       *block_edge_p;

    /*! @note The following fields cannot be set until the owning object is 
     *        configured.
     */
    
    /*! This is the number of configured components for the raid group.
     */
    fbe_u32_t               width;

    /*! This is the configured capacity for the object (includes user and the
     *  metadata capacity).
     */
    fbe_lba_t               configured_capacity;
    
    /*! Size of each data element.
     */
    fbe_element_size_t       element_size;

    /*! Number of elements before parity rotates.
     */
    fbe_elements_per_parity_t    elements_per_parity;

    /*! @note The following fields are used for metadata I/O.
     */

    /*! This is the starting lba for the metadata of this object.
     */
    fbe_lba_t               metadata_start_lba;
    
    /*! This is the metadata capacity (for all copies)
     */
    fbe_lba_t               metadata_capacity;
    
    /*! This is the offset for each copy of the metadata.  If there is only
     *  a single copy this field will be FBE_LBA_INVALID.
     */
    fbe_lba_t               metadata_copy_offset;

    union {
        fbe_raid_geometry_journal_info_t journal_info; /*!< All journal related info for parity groups. */
        fbe_raid_geometry_raw_mirror_info_t raw_mirror_info;
    } raid_type_specific;
    /* MetaData Element pointer for operations with the meta data service.
     */
    fbe_metadata_element_t  *metadata_element_p;
}
fbe_raid_geometry_t;

/*!*******************************************************************
 * @struct fbe_raid_small_read_geometry_t
 *********************************************************************
 * @brief
 *  This structure contains the information neccessary to execute small
 *  read requests.
 *
 *********************************************************************/
typedef struct fbe_raid_small_read_geometry_s
{
    fbe_u32_t position;
    fbe_block_count_t start_offset_rel_parity_stripe;
    fbe_lba_t logical_parity_start;
}
fbe_raid_small_read_geometry_t;

/*************************
 *  METHOD ACCESSORS     *
 *************************/
/* Accessors for object_id.
 */
static __forceinline fbe_object_id_t fbe_raid_geometry_get_object_id(fbe_raid_geometry_t *raid_geometry_p)
{
    return raid_geometry_p->object_id;
}
static __forceinline void fbe_raid_geometry_set_object_id(fbe_raid_geometry_t *raid_geometry_p, fbe_object_id_t object_id) 
{
    raid_geometry_p->object_id = object_id;
    return;
}

/* Accessors for class_id.
 */
static __forceinline fbe_class_id_t fbe_raid_geometry_get_class_id(fbe_raid_geometry_t *raid_geometry_p)
{
    return raid_geometry_p->class_id;
}
static __forceinline void fbe_raid_geometry_set_class_id(fbe_raid_geometry_t *raid_geometry_p, 
                                                   fbe_class_id_t class_id) 
{
    raid_geometry_p->class_id = class_id;
    return;
}

/* Accessors for raid geometry flags.
 */
static __forceinline fbe_bool_t fbe_raid_geometry_is_flag_set(fbe_raid_geometry_t *raid_geometry_p,
                                                       fbe_raid_geometry_flags_t flag)
{
    return ((raid_geometry_p->geometry_flags & flag) == flag);
}
static __forceinline void fbe_raid_geometry_init_flags(fbe_raid_geometry_t *raid_geometry_p)
{
    raid_geometry_p->geometry_flags = FBE_RAID_GEOMETRY_FLAG_INITIALIZED;
    return;
}
static __forceinline void fbe_raid_geometry_set_flag(fbe_raid_geometry_t *raid_geometry_p,
                                              fbe_raid_geometry_flags_t flag)
{
    raid_geometry_p->geometry_flags |= flag;
}
static __forceinline void fbe_raid_geometry_clear_flag(fbe_raid_geometry_t *raid_geometry_p,
                                                fbe_raid_geometry_flags_t flag)
{
    raid_geometry_p->geometry_flags &= ~flag;
}

/* Accessors for the generate state.
 */
static __forceinline void fbe_raid_geometry_set_generate_state(fbe_raid_geometry_t *raid_geometry_p, 
                                                        fbe_raid_common_state_t generate_state)
{
    raid_geometry_p->generate_state = generate_state;
    return;
}
static __forceinline void fbe_raid_geometry_get_generate_state(fbe_raid_geometry_t *raid_geometry_p,
                                                     fbe_raid_common_state_t *generate_state_p)
{
    *generate_state_p = raid_geometry_p->generate_state;
    return;
}

static __forceinline void fbe_raid_geometry_get_attribute_flags(fbe_raid_geometry_t *raid_geometry_p,
                                                         fbe_raid_attribute_flags_t *attribute_p)
{
    *attribute_p = raid_geometry_p->attribute_flags;
}
static __forceinline fbe_bool_t fbe_raid_geometry_is_attribute_set(fbe_raid_geometry_t *raid_geometry_p,
                                                            fbe_raid_attribute_flags_t attribute)
{
    return ((raid_geometry_p->attribute_flags & attribute) == attribute);
}
static __forceinline void fbe_raid_geometry_init_attributes(fbe_raid_geometry_t *raid_geometry_p)
{
    raid_geometry_p->attribute_flags = 0;
    return;
}
static __forceinline void fbe_raid_geometry_set_attribute(fbe_raid_geometry_t *raid_geometry_p,
                                                   fbe_raid_attribute_flags_t attribute)
{
    raid_geometry_p->attribute_flags |= attribute;
}

static __forceinline void fbe_raid_geometry_clear_attribute(fbe_raid_geometry_t *raid_geometry_p,
                                                     fbe_raid_attribute_flags_t attribute)
{
    raid_geometry_p->attribute_flags &= ~attribute;
}
/* Debug flag accessors.
 */
static __forceinline fbe_bool_t fbe_raid_geometry_is_debug_flag_set(fbe_raid_geometry_t *raid_p,
                                                          fbe_raid_library_debug_flags_t debug_flags)
{
    if (raid_p == NULL)
    {
        return FBE_FALSE;
    }
    return ((raid_p->debug_flags & debug_flags) == debug_flags);
}
static __forceinline void fbe_raid_geometry_get_debug_flags(fbe_raid_geometry_t *raid_p,
                                                     fbe_raid_library_debug_flags_t *debug_flags_p)
{
    *debug_flags_p = raid_p->debug_flags;
}
static __forceinline void fbe_raid_geometry_init_debug_flags(fbe_raid_geometry_t *raid_p,
                                                      fbe_raid_library_debug_flags_t debug_flags)
{
    raid_p->debug_flags = debug_flags;
}
static __forceinline void fbe_raid_geometry_set_debug_flags(fbe_raid_geometry_t *raid_p,
                                                     fbe_raid_library_debug_flags_t debug_flags)
{
    raid_p->debug_flags = debug_flags;
}
static __forceinline void fbe_raid_geometry_set_debug_flag(fbe_raid_geometry_t *raid_p,
                                                fbe_raid_library_debug_flags_t debug_flags)
{
    raid_p->debug_flags |= debug_flags;
}

static __forceinline void fbe_raid_geometry_clear_debug_flag(fbe_raid_geometry_t *raid_p,
                                                   fbe_raid_library_debug_flags_t debug_flags)
{
    raid_p->debug_flags &= ~debug_flags;
}

static __forceinline void fbe_raid_geometry_init_raid_type(fbe_raid_geometry_t *raid_geometry_p)
{
    raid_geometry_p->raid_type = FBE_RAID_GROUP_TYPE_UNKNOWN;
    return;
}

static __forceinline void fbe_raid_geometry_set_raid_type(fbe_raid_geometry_t *raid_geometry_p,
                                                   fbe_raid_group_type_t raid_type)
{
    raid_geometry_p->raid_type = raid_type;
    return;
}

static __forceinline void fbe_raid_geometry_get_raid_type(fbe_raid_geometry_t *raid_geometry_p,
                                                   fbe_raid_group_type_t *raid_type)
{
    *raid_type = raid_geometry_p->raid_type;
    return;
}

static __forceinline fbe_bool_t fbe_raid_geometry_is_type_unknown(fbe_raid_geometry_t *raid_geometry_p)
{
    if (raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_UNKNOWN)
    {
        return(FBE_TRUE);
    }
    
    return(FBE_FALSE);
}
static __forceinline fbe_bool_t fbe_raid_geometry_is_raid1(fbe_raid_geometry_t *raid_geometry_p)
{
    if (raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1)
    {
        return(FBE_TRUE);
    }
    
    return(FBE_FALSE);
}

static __forceinline fbe_bool_t fbe_raid_geometry_is_raw_mirror(fbe_raid_geometry_t *raid_geometry_p)
{
    if (raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_RAW_MIRROR)
    {
        return(FBE_TRUE);
    }
    
    return(FBE_FALSE);
}
static __forceinline fbe_bool_t fbe_raid_geometry_has_paged_metadata(fbe_raid_geometry_t *raid_geometry_p)
{
    if (raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_RAW_MIRROR)
    {
        return(FBE_FALSE);
    }
    
    return(FBE_TRUE);
}

static __forceinline fbe_bool_t fbe_raid_geometry_no_object(fbe_raid_geometry_t *raid_geometry_p)
{
    return fbe_raid_geometry_is_flag_set(raid_geometry_p, FBE_RAID_GEOMETRY_NO_OBJECT);
}
static __forceinline fbe_bool_t fbe_raid_geometry_is_raid10(fbe_raid_geometry_t *raid_geometry_p)
{
    if (raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        return(FBE_TRUE);
    }
    
    return(FBE_FALSE);
}
static __forceinline fbe_bool_t fbe_raid_geometry_is_raid3(fbe_raid_geometry_t *raid_geometry_p)
{
    if (raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_RAID3)
    {
        return(FBE_TRUE);
    }
    
    return(FBE_FALSE);
}
static __forceinline fbe_bool_t fbe_raid_geometry_is_raid0(fbe_raid_geometry_t *raid_geometry_p)
{
    if (raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0)
    {
        return(FBE_TRUE);
    }
    
    return(FBE_FALSE);
}
static __forceinline fbe_bool_t fbe_raid_geometry_is_raid5(fbe_raid_geometry_t *raid_geometry_p)
{
    if (raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_RAID5)
    {
        return(FBE_TRUE);
    }
    
    return(FBE_FALSE);
}
static __forceinline fbe_bool_t fbe_raid_geometry_is_raid6(fbe_raid_geometry_t *raid_geometry_p)
{
    if (raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6)
    {
        return(FBE_TRUE);
    }
    
    return(FBE_FALSE);
}
static __forceinline void fbe_raid_geometry_get_num_parity(fbe_raid_geometry_t *raid_geometry_p,
                                                           fbe_u32_t *num_parity_p)
{
    *num_parity_p = fbe_raid_geometry_is_raid6(raid_geometry_p) ? 2 : 1;
}
static __forceinline fbe_bool_t fbe_raid_geometry_is_striper_type(fbe_raid_geometry_t *raid_geometry_p)
{
    if ((raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0)  ||
        (raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)    )
    {
        return(FBE_TRUE);
    }
    
    return(FBE_FALSE);
}
static __forceinline fbe_bool_t fbe_raid_geometry_is_striped_mirror(fbe_raid_geometry_t *raid_geometry_p)
{
    if (raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        return(FBE_TRUE);
    }
    
    return(FBE_FALSE);
}
static __forceinline fbe_bool_t fbe_raid_geometry_is_mirror_type(fbe_raid_geometry_t *raid_geometry_p)
{
    if ((raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1)                         ||
        (raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_SPARE)                         ||
        (raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER) ||
        (raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_INTERNAL_METADATA_MIRROR)      ||
        (raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_RAW_MIRROR)  )
    {
        return(FBE_TRUE);
    }
    
    return(FBE_FALSE);
}
static __forceinline fbe_bool_t fbe_raid_geometry_is_mirror_under_striper(fbe_raid_geometry_t *raid_geometry_p)
{
    if (raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER)
    {
        return(FBE_TRUE);
    }
    
    return(FBE_FALSE);
}
static __forceinline fbe_bool_t fbe_raid_geometry_is_child_raid_group(fbe_raid_geometry_t *raid_geometry_p)
{
    if (raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER)
    {
        return(FBE_TRUE);
    }
    
    return(FBE_FALSE);
}
static __forceinline fbe_bool_t fbe_raid_geometry_is_triple_mirror(fbe_raid_geometry_t *raid_geometry_p)
{
    if (((raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1) ||
         (raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_RAW_MIRROR)) &&
        (raid_geometry_p->width     == 3) )
    {
        return(FBE_TRUE);
    }
    
    return(FBE_FALSE);
}
static __forceinline fbe_bool_t fbe_raid_geometry_is_parity_type(fbe_raid_geometry_t *raid_geometry_p)
{
    if ((raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_RAID5) ||
        (raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_RAID3) ||
        (raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6)    )
    {
        return(FBE_TRUE);
    }
    
    return(FBE_FALSE);
}

/*! @note When a raid group is in `sparing' mode it's only purpose is to serve 
 *        as a `copy machine'.  This means it should not check or modify:
 *          o lba_stamp - `Owned' by parent raid group (could be shed_stamp etc.)
 *          o write_stamp - Can only be generated and validated by parent
 *          o time_stamp - Can only be generated and validated by parent
 *  
 *        It is required and ok to validate and generate the crc.
 */
static __forceinline fbe_bool_t fbe_raid_geometry_is_sparing_type(fbe_raid_geometry_t *raid_geometry_p)
{
    if (raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_SPARE)
    {
        return(FBE_TRUE);
    }
    
    return(FBE_FALSE);
}
static __forceinline fbe_bool_t fbe_raid_geometry_is_hot_sparing_type(fbe_raid_geometry_t *raid_geometry_p)
{
    if ( (raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_SPARE)                                   && 
        !(fbe_raid_geometry_is_attribute_set(raid_geometry_p, FBE_RAID_ATTRIBUTE_PROACTIVE_SPARING))    )  
    {
        return(FBE_TRUE);
    }
    
    return(FBE_FALSE);
}
static __forceinline fbe_bool_t fbe_raid_geometry_is_proactive_sparing_type(fbe_raid_geometry_t *raid_geometry_p)
{
    if ( (raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_SPARE)                                   && 
         (fbe_raid_geometry_is_attribute_set(raid_geometry_p, FBE_RAID_ATTRIBUTE_PROACTIVE_SPARING))    )  
    {
        return(FBE_TRUE);
    }
    
    return(FBE_FALSE);
}

/* Accessor for optimal size.
 */
static __forceinline void fbe_raid_geometry_get_optimal_size(fbe_raid_geometry_t *raid_geometry_p,
                                                   fbe_block_size_t *size_p)
{
    *size_p = raid_geometry_p->optimal_block_size;
    return;
}
static __forceinline void fbe_raid_geometry_set_optimal_size(fbe_raid_geometry_t *raid_geometry_p,
                                                   fbe_block_size_t size)
{
    raid_geometry_p->optimal_block_size = size;
    return;
}
/* Accessor for exported size.
 */
static __forceinline void fbe_raid_geometry_get_exported_size(fbe_raid_geometry_t *raid_geometry_p,
                                                       fbe_block_size_t *size_p)
{
    *size_p = raid_geometry_p->exported_block_size;
    return;
}
static __forceinline void fbe_raid_geometry_set_exported_size(fbe_raid_geometry_t *raid_geometry_p,
                                                       fbe_block_size_t size)
{
    raid_geometry_p->exported_block_size = size;
    return;
}
/* Accessor for imported size.
 */
static __forceinline void fbe_raid_geometry_get_imported_size(fbe_raid_geometry_t *raid_geometry_p,
                                                    fbe_block_size_t *size_p)
{
    *size_p = raid_geometry_p->imported_block_size;
    return;
}
static __forceinline void fbe_raid_geometry_set_imported_size(fbe_raid_geometry_t *raid_geometry_p,
                                                    fbe_block_size_t size)
{
    raid_geometry_p->imported_block_size = size;
    return;
}

/* Accessor for maximum number of blocks per drive.  I.E. The largest number of
 * `logical block size' blocks that we can send to any drive in a raid group.
 */
static __forceinline void fbe_raid_geometry_get_max_blocks_per_drive(fbe_raid_geometry_t *raid_geometry_p,
                                                              fbe_block_count_t *max_blocks_p)
{
    *max_blocks_p = raid_geometry_p->max_blocks_per_drive;
    return;
}
static __forceinline void fbe_raid_geometry_set_max_blocks_per_drive(fbe_raid_geometry_t *raid_geometry_p,
                                                              fbe_block_count_t max_blocks_per_drive)
{
    raid_geometry_p->max_blocks_per_drive = max_blocks_per_drive;
    return;
}

static __forceinline void fbe_raid_geometry_get_block_edge(fbe_raid_geometry_t *raid_geometry_p,
                                                    fbe_block_edge_t **block_edge_p,
                                                    fbe_u32_t position)
{
    if (fbe_raid_geometry_no_object(raid_geometry_p))
    {
        /* This type of raid group uses a pointer array, since it is
         * fetching the edges from the objects.  
         */
        fbe_block_edge_t **block_edge_array = (fbe_block_edge_t **)raid_geometry_p->block_edge_p;
        *block_edge_p = block_edge_array[position];
    }
    else
    {
        /* Return the fru info for a given position.
         */
        *block_edge_p = &raid_geometry_p->block_edge_p[position];
    }
    return;
}
static __forceinline void fbe_raid_geometry_set_block_edge(fbe_raid_geometry_t *raid_geometry_p,
                                                    fbe_block_edge_t *block_edge_p)
{
    raid_geometry_p->block_edge_p = block_edge_p;
    return;
}

static __forceinline void fbe_raid_geometry_init_width(fbe_raid_geometry_t *raid_geometry_p)
{
    raid_geometry_p->width = 0;
    return;
}
static __forceinline void fbe_raid_geometry_set_width(fbe_raid_geometry_t *raid_geometry_p,
                                               fbe_u32_t width)
{
    raid_geometry_p->width = width;
    return;
}
static __forceinline void fbe_raid_geometry_get_width(fbe_raid_geometry_t *raid_geometry_p,
                                               fbe_u32_t *width_p)
{
    *width_p = raid_geometry_p->width;
    return;
}

static __forceinline void fbe_raid_geometry_init_configured_capacity(fbe_raid_geometry_t *raid_geometry_p)
{
    raid_geometry_p->configured_capacity = 0;
    return;
}
static __forceinline void fbe_raid_geometry_set_configured_capacity(fbe_raid_geometry_t *raid_geometry_p,
                                                             fbe_lba_t configured_capacity)
{
    raid_geometry_p->configured_capacity = configured_capacity;
    return;
}
static __forceinline void fbe_raid_geometry_get_configured_capacity(fbe_raid_geometry_t *raid_geometry_p,
                                                             fbe_lba_t *configured_capacity_p)
{
    *configured_capacity_p = raid_geometry_p->configured_capacity;
    return;
}
/* Accessor for element size.
 */
static __forceinline void fbe_raid_geometry_get_element_size(fbe_raid_geometry_t *raid_geometry_p,
                                                      fbe_element_size_t *size_p)
{
    *size_p = raid_geometry_p->element_size;
    return;
}
static __forceinline void fbe_raid_geometry_set_element_size(fbe_raid_geometry_t *raid_geometry_p,
                                                      fbe_element_size_t size)
{
    raid_geometry_p->element_size = size;
    return;
}

/* Accessor for elements per parity.
 */
static __forceinline void fbe_raid_geometry_get_elements_per_parity(fbe_raid_geometry_t *raid_geometry_p,
                                                             fbe_elements_per_parity_t *size_p)
{
    *size_p = raid_geometry_p->elements_per_parity;
    return;
}
static __forceinline void fbe_raid_geometry_set_elements_per_parity(fbe_raid_geometry_t *raid_geometry_p,
                                                             fbe_elements_per_parity_t size)
{
    raid_geometry_p->elements_per_parity = size;
    return;
}

static __forceinline void fbe_raid_geometry_init_metadata_start_lba(fbe_raid_geometry_t *const raid_geometry_p)
{
    raid_geometry_p->metadata_start_lba = FBE_LBA_INVALID;
    return;
}
static __forceinline void fbe_raid_geometry_set_metadata_start_lba(fbe_raid_geometry_t *const raid_geometry_p,
                                                            const fbe_lba_t metadata_start_lba)
{
    raid_geometry_p->metadata_start_lba = metadata_start_lba;
    return;
}
static __forceinline void fbe_raid_geometry_get_metadata_start_lba(fbe_raid_geometry_t *const raid_geometry_p,
                                                            fbe_lba_t  *const metadata_start_lba_p)
{
    *metadata_start_lba_p = raid_geometry_p->metadata_start_lba;
    return;
}

static __forceinline void fbe_raid_geometry_init_metadata_capacity(fbe_raid_geometry_t *raid_geometry_p)
{
    raid_geometry_p->metadata_capacity = 0;
    return;
}
static __forceinline void fbe_raid_geometry_set_metadata_capacity(fbe_raid_geometry_t *raid_geometry_p,
                                                           fbe_lba_t metadata_capacity)
{
    raid_geometry_p->metadata_capacity = metadata_capacity;
    return;
}
static __forceinline void fbe_raid_geometry_get_metadata_capacity(fbe_raid_geometry_t *raid_geometry_p,
                                                           fbe_lba_t *metadata_capacity_p)
{
    *metadata_capacity_p = raid_geometry_p->metadata_capacity;
    return;
}
static __forceinline fbe_bool_t fbe_raid_geometry_is_user_io(fbe_raid_geometry_t *raid_geometry_p,
                                                             fbe_lba_t io_start_lba)
{
    fbe_lba_t metadata_lba = FBE_LBA_INVALID;

    fbe_raid_geometry_get_metadata_start_lba(raid_geometry_p, &metadata_lba);

    if (io_start_lba < metadata_lba)
    {
        /* Yes the I/O is within the user area.
         */
        return FBE_TRUE;
    }
    return FBE_FALSE;
}
static __forceinline fbe_bool_t fbe_raid_geometry_is_metadata_io(fbe_raid_geometry_t *raid_geometry_p,
                                                                 fbe_lba_t io_start_lba)
{
    fbe_lba_t metadata_lba = FBE_LBA_INVALID;
    fbe_lba_t metadata_capacity = 0;
    
    fbe_raid_geometry_get_metadata_start_lba(raid_geometry_p, &metadata_lba);
    fbe_raid_geometry_get_metadata_capacity(raid_geometry_p, &metadata_capacity);

    if ((io_start_lba >= metadata_lba) && 
        (io_start_lba < (metadata_lba + metadata_capacity)))
    {
        /* Yes the I/O is within the metadata area.
         */
        return FBE_TRUE;
    }
    return FBE_FALSE;
}


static __forceinline void fbe_raid_geometry_init_metadata_copy_offset(fbe_raid_geometry_t *const raid_geometry_p)
{
    raid_geometry_p->metadata_copy_offset = FBE_LBA_INVALID;
    return;
}
static __forceinline void fbe_raid_geometry_set_metadata_copy_offset(fbe_raid_geometry_t *const raid_geometry_p,
                                                              const fbe_lba_t  copy_offset)
{
    raid_geometry_p->metadata_copy_offset = copy_offset;
    return;
}
static __forceinline void fbe_raid_geometry_get_metadata_copy_offset(fbe_raid_geometry_t *const raid_geometry_p,
                                                              fbe_lba_t  *const copy_offset_p)
{
    *copy_offset_p = raid_geometry_p->metadata_copy_offset;
    return;
}
static __forceinline void fbe_raid_geometry_set_journal_log_start_lba(fbe_raid_geometry_t *const raid_geometry_p,
                                                               const fbe_lba_t  start_lba)
{
    if (FBE_TRUE == fbe_raid_geometry_is_parity_type(raid_geometry_p))
    {
        raid_geometry_p->raid_type_specific.journal_info.write_log_info_p->start_lba = start_lba;
    }
    return;
}
static __forceinline void fbe_raid_geometry_get_journal_log_start_lba(fbe_raid_geometry_t *const raid_geometry_p,
                                                                      fbe_lba_t  *const start_lba_p)
{
    
    if (fbe_raid_geometry_is_parity_type(raid_geometry_p) &&
        (raid_geometry_p->raid_type_specific.journal_info.write_log_info_p != NULL))
    {
        *start_lba_p = raid_geometry_p->raid_type_specific.journal_info.write_log_info_p->start_lba;
    }
    else
    {
        *start_lba_p = FBE_LBA_INVALID;
    }
    return;
}
static __forceinline void fbe_raid_geometry_set_journal_log_slot_size(fbe_raid_geometry_t *const raid_geometry_p,
                                                               const fbe_u32_t  slot_size)
{
    if (FBE_TRUE == fbe_raid_geometry_is_parity_type(raid_geometry_p))
    {
        raid_geometry_p->raid_type_specific.journal_info.write_log_info_p->slot_size = slot_size;
    }
    return;
}
static __forceinline void fbe_raid_geometry_get_journal_log_slot_size(fbe_raid_geometry_t *const raid_geometry_p,
                                                                      fbe_u32_t  *const slot_size_p)
{
    if (FBE_TRUE == fbe_raid_geometry_is_parity_type(raid_geometry_p))
    {
        *slot_size_p = raid_geometry_p->raid_type_specific.journal_info.write_log_info_p->slot_size;
    }
    else
    {
        *slot_size_p = 0;
    }
    return;
}
static __forceinline void fbe_raid_geometry_set_write_log_slot_count(fbe_raid_geometry_t *const raid_geometry_p,
                                                                     const fbe_u32_t  slot_count)
{
    if (FBE_TRUE == fbe_raid_geometry_is_parity_type(raid_geometry_p))
    {
        raid_geometry_p->raid_type_specific.journal_info.write_log_info_p->slot_count = slot_count;
    }
    return;
}
static __forceinline void fbe_raid_geometry_get_write_log_slot_count(fbe_raid_geometry_t *const raid_geometry_p,
                                                                     fbe_u32_t  *const slot_count_p)
{
    if (FBE_TRUE == fbe_raid_geometry_is_parity_type(raid_geometry_p))
    {
        *slot_count_p = raid_geometry_p->raid_type_specific.journal_info.write_log_info_p->slot_count;
    }
    else
    {
        *slot_count_p = 0;
    }
    return;
}
static __forceinline void fbe_raid_geometry_get_journal_log_end_lba(fbe_raid_geometry_t *const raid_geometry_p,
                                                                    fbe_lba_t  *const end_lba_p)
{
    fbe_lba_t journal_start_lba;
    fbe_lba_t journal_end_lba;

    fbe_raid_geometry_get_journal_log_start_lba(raid_geometry_p, &journal_start_lba);

    journal_end_lba = journal_start_lba + (FBE_RAID_GROUP_WRITE_LOG_SIZE) - 1;
    *end_lba_p = journal_end_lba;
}


static __forceinline fbe_bool_t fbe_raid_geometry_journal_is_flag_set(fbe_raid_geometry_t *raid_geometry_p,
                                                                      fbe_parity_write_log_flags_t flag)
{
    return fbe_parity_write_log_is_flag_set(raid_geometry_p->raid_type_specific.journal_info.write_log_info_p, flag);
}
static __forceinline void fbe_raid_geometry_journal_set_flag(fbe_raid_geometry_t *raid_geometry_p,
                                                             fbe_parity_write_log_flags_t flag)
{
    fbe_parity_write_log_set_flag(raid_geometry_p->raid_type_specific.journal_info.write_log_info_p, flag);
}
static __forceinline void fbe_raid_geometry_journal_clear_flag(fbe_raid_geometry_t *raid_geometry_p,
                                                               fbe_parity_write_log_flags_t flag)
{
    fbe_parity_write_log_clear_flag(raid_geometry_p->raid_type_specific.journal_info.write_log_info_p, flag);
}

static __forceinline void fbe_raid_geometry_get_journal_log_hdr_blocks(fbe_raid_geometry_t *const raid_geometry_p,
                                                                       fbe_u32_t  *const hdr_size_p)
{
    if (FBE_TRUE == fbe_raid_geometry_is_parity_type(raid_geometry_p)){
        if (fbe_raid_geometry_journal_is_flag_set(raid_geometry_p, FBE_PARITY_WRITE_LOG_FLAGS_4K_SLOT)) {
            *hdr_size_p = FBE_520_BLOCKS_PER_4K;
        } else {
            *hdr_size_p = 1;
        }
    }
    else {
        *hdr_size_p = 0;
    }
    return;
}


static __forceinline void fbe_raid_geometry_set_raw_mirror_offset(fbe_raid_geometry_t *const raid_geometry_p,
                                                                  const fbe_lba_t  offset)
{
    raid_geometry_p->raid_type_specific.raw_mirror_info.raw_mirror_physical_offset = offset;
    return;
}
static __forceinline void fbe_raid_geometry_get_raw_mirror_offset(fbe_raid_geometry_t *const raid_geometry_p,
                                                                  fbe_lba_t  *const offset_p)
{
    *offset_p = raid_geometry_p->raid_type_specific.raw_mirror_info.raw_mirror_physical_offset;
    return;
}
static __forceinline void fbe_raid_geometry_set_raw_mirror_rg_offset(fbe_raid_geometry_t *const raid_geometry_p,
                                                                     const fbe_lba_t  offset)
{
    raid_geometry_p->raid_type_specific.raw_mirror_info.raw_mirror_rg_offset = offset;
    return;
}
static __forceinline void fbe_raid_geometry_get_raw_mirror_rg_offset(fbe_raid_geometry_t *const raid_geometry_p,
                                                                     fbe_lba_t  *const offset_p)
{
    *offset_p = raid_geometry_p->raid_type_specific.raw_mirror_info.raw_mirror_rg_offset;
    return;
}
static __forceinline void fbe_raid_geometry_set_metadata_element(fbe_raid_geometry_t *const raid_geometry_p,
                                                          fbe_metadata_element_t *metadata_element_p)
{
    raid_geometry_p->metadata_element_p = metadata_element_p;
    return;
}
static __forceinline void fbe_raid_geometry_get_metadata_element(fbe_raid_geometry_t *const raid_geometry_p,
                                                          fbe_metadata_element_t **const metadata_element_p)
{
    *metadata_element_p = raid_geometry_p->metadata_element_p;
    return;
}

static __forceinline void fbe_raid_geometry_set_mirror_prefered_position(fbe_raid_geometry_t *const raid_geometry_p,
                                                                 fbe_mirror_prefered_position_t prefered_mirror_pos)
{
    raid_geometry_p->prefered_mirror_pos = prefered_mirror_pos;
    return;
}
static __forceinline void fbe_raid_geometry_get_mirror_prefered_position(fbe_raid_geometry_t * const raid_geometry_p,
                                                                  fbe_mirror_prefered_position_t * const prefered_mirror_pos_p)
{
    *prefered_mirror_pos_p = raid_geometry_p->prefered_mirror_pos;
    return;
}
/*fbe_mirror_optimization_t mirror_optimization;*/
static __forceinline void fbe_raid_geometry_get_mirror_opt_db(fbe_raid_geometry_t *raid_geometry_p, fbe_mirror_optimization_t **mirror_db_p)
{
    *mirror_db_p = raid_geometry_p->mirror_optimization_p;
}
static __forceinline void fbe_raid_geometry_set_mirror_opt_db(fbe_raid_geometry_t *raid_geometry_p, fbe_mirror_optimization_t *mirror_db_p) 
{
    raid_geometry_p->mirror_optimization_p = mirror_db_p;
    return;
}

/* Accessors for write_logging */
static __forceinline void fbe_raid_geometry_set_vault(fbe_raid_geometry_t *raid_geometry_p)
{
    raid_geometry_p->attribute_flags |= FBE_RAID_ATTRIBUTE_VAULT;
    return;
}
static __forceinline void fbe_raid_geometry_clear_vault(fbe_raid_geometry_t *raid_geometry_p)
{
    raid_geometry_p->attribute_flags &= ~FBE_RAID_ATTRIBUTE_VAULT;
    return;
}

static __forceinline fbe_bool_t fbe_raid_geometry_is_vault(fbe_raid_geometry_t *raid_geometry_p)
{
    if (fbe_raid_geometry_is_attribute_set(raid_geometry_p, FBE_RAID_ATTRIBUTE_VAULT))
    {
        return(FBE_TRUE);
    }
    
    return(FBE_FALSE);
}

/* Accessors for c4mirror */
static __forceinline void fbe_raid_geometry_set_c4_mirror(fbe_raid_geometry_t *raid_geometry_p)
{
    raid_geometry_p->attribute_flags |= FBE_RAID_ATTRIBUTE_C4_MIRROR;
    return;
}

static __forceinline fbe_bool_t fbe_raid_geometry_is_c4_mirror(fbe_raid_geometry_t *raid_geometry_p)
{
    if (fbe_raid_geometry_is_attribute_set(raid_geometry_p, FBE_RAID_ATTRIBUTE_C4_MIRROR))
    {
        return(FBE_TRUE);
    }
    
    return(FBE_FALSE);
}

static __forceinline fbe_bool_t fbe_raid_geometry_is_write_logging_enabled(fbe_raid_geometry_t *raid_geometry_p)
{
    if ((!fbe_raid_geometry_is_attribute_set(raid_geometry_p, FBE_RAID_ATTRIBUTE_VAULT)) &&
        (FBE_PARITY_WRITE_LOG_ENABLE) &&                    /* Compile time switch to disable write_logging */
        (!fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_DISABLE_WRITE_LOGGING)))
                                                            /* Debug switch to disable write logging */ 
    {
        return(FBE_TRUE);
    }
    
    return(FBE_FALSE);
}

static __forceinline void fbe_raid_geometry_set_write_logging_blob_p(fbe_raid_geometry_t *raid_geometry_p,
                                                                     fbe_parity_write_log_info_t * blob_p)
{
    raid_geometry_p->raid_type_specific.journal_info.write_log_info_p = blob_p;
    return;
}
static __forceinline void fbe_raid_geometry_get_4k_bitmask(fbe_raid_geometry_t *raid_geometry_p,
                                                           fbe_raid_position_bitmask_t *bitmask_4k_p)
{
    *bitmask_4k_p = raid_geometry_p->bitmask_4k;
    return;
}
static __forceinline void fbe_raid_geometry_set_bitmask_4k(fbe_raid_geometry_t *raid_geometry_p,
                                                           fbe_raid_position_bitmask_t bitmask_4k)
{
    raid_geometry_p->bitmask_4k = bitmask_4k;
    return;
}
static __forceinline fbe_bool_t fbe_raid_geometry_is_position_4k_aligned(fbe_raid_geometry_t *raid_geometry_p,
                                                                         fbe_raid_position_t position)
{
    if (raid_geometry_p->bitmask_4k & (1 << position)){
        return FBE_TRUE;
    } else {
        return FBE_FALSE;
    }
}

static __forceinline fbe_bool_t fbe_raid_geometry_needs_alignment(fbe_raid_geometry_t *raid_geometry_p)
{
    if (raid_geometry_p->bitmask_4k != 0){
        return FBE_TRUE;
    } else {
        return FBE_FALSE;
    }
}

static __forceinline fbe_bool_t fbe_raid_geometry_position_needs_alignment(fbe_raid_geometry_t *raid_geometry_p,
                                                                           fbe_u32_t position)
{
    if (raid_geometry_p->bitmask_4k & (1 << position)){
        return FBE_TRUE;
    } else {
        return FBE_FALSE;
    }
}

static __forceinline fbe_u32_t fbe_raid_geometry_get_default_imported_block_size(fbe_raid_geometry_t *raid_geometry_p)
{
    if (raid_geometry_p->bitmask_4k != 0){
        return FBE_4K_BYTES_PER_BLOCK;
    } else {
        return FBE_BE_BYTES_PER_BLOCK;
    }
}
static __forceinline fbe_u32_t fbe_raid_geometry_get_default_optimal_block_size(fbe_raid_geometry_t *raid_geometry_p)
{
    if (raid_geometry_p->bitmask_4k != 0){
        return FBE_520_BLOCKS_PER_4K;
    } else {
        return 1;
    }
}
fbe_status_t fbe_raid_geometry_refresh_block_sizes(fbe_raid_geometry_t *raid_geometry_p);
fbe_status_t fbe_raid_geometry_should_refresh_block_sizes(fbe_raid_geometry_t *raid_geometry_p,
                                                          fbe_bool_t *b_refresh_p,
                                                          fbe_raid_position_bitmask_t *refresh_bitmap_p);
static __forceinline void fbe_raid_geometry_get_write_logging_blob(fbe_raid_geometry_t *raid_geometry_p,
                                                                   fbe_parity_write_log_info_t ** blob_p)
{
    *blob_p = raid_geometry_p->raid_type_specific.journal_info.write_log_info_p;
}
static __forceinline fbe_bool_t fbe_raid_geometry_journal_flag_is_set(fbe_raid_geometry_t *const raid_geometry_p,
                                                                      fbe_parity_write_log_flags_t flag)
{
    fbe_parity_write_log_info_t *blob_p = NULL;
    fbe_raid_geometry_get_write_logging_blob(raid_geometry_p, &blob_p);

    return fbe_parity_write_log_is_flag_set(blob_p, flag);
}
static __forceinline void fbe_raid_geometry_journal_flag_set(fbe_raid_geometry_t *const raid_geometry_p,
                                                             fbe_parity_write_log_flags_t flag)
{
    fbe_parity_write_log_info_t *blob_p = NULL;
    fbe_raid_geometry_get_write_logging_blob(raid_geometry_p, &blob_p);

    fbe_parity_write_log_set_flag(blob_p, flag);
}
static __forceinline void fbe_raid_geometry_journal_flag_clear(fbe_raid_geometry_t *const raid_geometry_p,
                                                               fbe_parity_write_log_flags_t flag)
{
    fbe_parity_write_log_info_t *blob_p = NULL;
    fbe_raid_geometry_get_write_logging_blob(raid_geometry_p, &blob_p);

    fbe_parity_write_log_clear_flag(blob_p, flag);
}

/*!***************************************************************
 *          fbe_raid_geometry_get_max_dead_positions()
 *****************************************************************
 *
 * @brief   Based on raid type and width, return the maximum number
 *          `dead/degraded' positions allowed.
 * 
 * @param   raid_geometry_p - Pointer to raid information
 * @param   max_dead_positions_p - Address of max dead postions to udpate
 *
 * @return fbe_status_t - Typically FBE_STATUS_OK
 *
 * @author
 *  01/15/2010  Ron Proulx  - Created
 *
 ****************************************************************/
static __forceinline fbe_status_t fbe_raid_geometry_get_max_dead_positions(fbe_raid_geometry_t *raid_geometry_p, 
                                                      fbe_u32_t *max_dead_positions_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       width; 
        
    /* This routine shouldn't be invoked unless the raid geometry has been
     * configured.
     */
    if (fbe_raid_geometry_is_flag_set(raid_geometry_p, FBE_RAID_GEOMETRY_FLAG_CONFIGURED))
    {
        /* Get the raid group width and switch on the raid type.
         */
        fbe_raid_geometry_get_width(raid_geometry_p, &width);
        switch(raid_geometry_p->raid_type)
        {
            case FBE_RAID_GROUP_TYPE_UNKNOWN:
                /* The raid group hasn't been configured.
                 */
                status = FBE_STATUS_NOT_INITIALIZED;
                break;
    
            case FBE_RAID_GROUP_TYPE_RAID1:
            case FBE_RAID_GROUP_TYPE_SPARE:
            case FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER:
            case FBE_RAID_GROUP_TYPE_INTERNAL_METADATA_MIRROR:
            case FBE_RAID_GROUP_TYPE_RAW_MIRROR:
                /* The maximum number of dead positions is the width minus 1.
                 */
                *max_dead_positions_p = width - 1;
                break;

            case FBE_RAID_GROUP_TYPE_RAID0:
            case FBE_RAID_GROUP_TYPE_RAID10:
                /* For striped raid group there cannot be any dead positions.
                 */                       
                *max_dead_positions_p = 0;
                break;
    
            case FBE_RAID_GROUP_TYPE_RAID3:
            case FBE_RAID_GROUP_TYPE_RAID5:
                /* Normal parity raid groups can only have 1 dead position.
                 */
                *max_dead_positions_p = 1;
                break;

            case FBE_RAID_GROUP_TYPE_RAID6:
                /* For RAID-6 2 dead positions are allowed.
                 */
                *max_dead_positions_p = 2;
                break;

            default:
                /* Unsupported raid type.
                 */
                *max_dead_positions_p = 0;
                status = FBE_STATUS_GENERIC_FAILURE;
                break;
        }
    }
    else
    {
        /* Else flag the fact the the geometry hasn't been configured
         * and set the maximum dead positions allowed to 0.
         */
        *max_dead_positions_p = 0;
        status = FBE_STATUS_NOT_INITIALIZED; 
    }
    
    /* Return the status of the request.
     */
    return(status);
}

/*****************************************
 * end of fbe_raid_geometry.h
 *****************************************/

#endif // _FBE_RAID_GEOMETRY_H_

