/* odb_dbo.h */

#ifndef ODB_DBO_H_INCLUDED
#define ODB_DBO_H_INCLUDED 0x00000001

/***************************************************************************
 * Copyright (C) EMC Corporation 2000-2001
 * All rights reserved
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *
 * DESCRIPTION:
 *   This header file contains DATABASE OBJECT related definitions.
 *
 * NOTES:
 *
 *
 * HISTORY:
 *   09-Aug-00: Created.                               Mike Zelikov (MZ)
 *
 ***************************************************************************/

/* INCLUDE FILES */
#include "odb_defs.h"

#define DBO_INVALID_FRU                        UNSIGNED_MINUS_1


/* The following provides definitions for
 * Host->Network, and Network->Host functions.
 * HTONxx and NTOHxx : xx - short (16 bit), long (32 bit)
 */
#if defined(_LITTLE_ENDIAN) || defined(FL_LITTLE_ENDIAN) /* Little endian machine */

#define dbo_byte_swap_long(x)           \
        ((((x) & 0xff000000) >> 24) |   \
         (((x) & 0x00ff0000) >>  8) |   \
         (((x) & 0x0000ff00) <<  8) |   \
         (((x) & 0x000000ff) << 24))

#define dbo_htonl(x) (dbo_byte_swap_long((x)))
#define dbo_ntohl(x) (dbo_byte_swap_long((x)))

#define dbo_byte_swap_short(x)    \
        ((((x) & 0xff00) >> 8) |  \
         (((x) & 0x00ff) << 8))

#define dbo_htons(x) (dbo_byte_swap_short((x)))
#define dbo_ntohs(x) (dbo_byte_swap_short((x)))

#else /* Big endian machine */

#define dbo_htonl(x)(x)
#define dbo_htons(x)(x)
                     
#define dbo_ntohl(x)(x)
#define dbo_ntohs(x)(x)

#endif /* ifdef _LITTLE_ENDIAN */

/*************************************************************
 * DBO_MB_2_BLOCKS()
 *   This macro converts size in megabytes to blocks
 *************************************************************
 */
#define DBO_MB_2_BLOCKS(value_in_mb) (2048 * (value_in_mb))

/*************************************************************
 * DBO_INDEX
 *   This DBO buffer indexes
 *************************************************************
 */
#define      DBO_INVALID_INDEX           UNSIGNED_MINUS_1
typedef enum _DBO_INDEX
{
     DBO_PRIMARY_INDEX         = 0,
     DBO_SECONDARY_INDEX       = 1

} DBO_INDEX;


/*************************************************************
 * DBO_REVISION: 4 bytes
 *   This defines revision number type.
 *************************************************************
 */
typedef UINT_32E DBO_REVISION;

#define DBO_INVALID_REVISION                 ((DBO_REVISION) UNSIGNED_MINUS_1)


/*************************************************************
 * DBO_UPDATE_COUNT: 4 bytes
 *   This defines update counter.
 *************************************************************
 */
typedef UINT_32E DBO_UPDATE_COUNT;

#define DBO_INVALID_UPDATE_COUNT             ((DBO_UPDATE_COUNT) UNSIGNED_MINUS_1)


/*************************************************************
 * DBO_ALGORITHM: 4 bytes
 *   This defines algorithm that is used to store DBO data.
 *************************************************************
 */
typedef UINT_32E DBO_ALGORITHM;

#define DBO_INVALID_ALGORITHM                ((DBO_ALGORITHM) UNSIGNED_MINUS_1)


/*************************************************************
 * DBO_ID: 8 bytes *
 *   This structure represents Database Object ID: 64 bit number;
 *   low         - low 32 bits;
 *   high        - high 32 bits;
 *************************************************************
 */
typedef struct _DBO_ID
{
     UINT_32E  low;
     UINT_32E  high;
     
} DBO_ID;


/*************************************************************
 * Special IDs :
 *   The following defined Well-Known DB objects. We only
 *   define special low part. The actuall well-know objects
 *   are defined by implementatuion.
 *************************************************************
 */
#define DBO_SPECIAL_ID_LOW              UNSIGNED_MINUS_1
#define DBO_INVALID_ID_HIGH             UNSIGNED_MINUS_1

#define DBO_INVALID_ID                  {DBO_SPECIAL_ID_LOW, DBO_INVALID_ID_HIGH}

#define DBO_ID_IS_SPECIAL(id_ptr) (DBO_SPECIAL_ID_LOW == (id_ptr)->low)
#define DBO_ID_IS_INVALID(id_ptr) (DBO_ID_IS_SPECIAL((id_ptr)) && \
                                   (DBO_INVALID_ID_HIGH == (id_ptr)->high))

#define DBO_ID_IS_EQUAL(id_ptr_A, id_ptr_B) (((id_ptr_A)->low == (id_ptr_B)->low) && \
                                             ((id_ptr_A)->high == (id_ptr_B)->high))


/*************************************************************
 * DBO_TYPE: 4 bytes *
 *  We allow class reservation, s.t. types can be divided into
 *  class. One should use macro to create/reserve class BIT, which
 *  is used later to create ID of a proper class.
 *  12 bits are allocated for classes.
 *************************************************************
 */
typedef UINT_32E DBO_TYPE;

#define DBO_TYPE_CLASS_MASK (0xFFF00000)
#define DBO_TYPE_ALLOCATE_CLASS(class)(1<<(20+(class)))
#define DBO_TYPE_GET_CLASS(dbo_type)((dbo_type)&DBO_TYPE_CLASS_MASK)

#define DBO_INVALID_TYPE                ((DBO_TYPE) -1)


/*************************************************************
 * DBOLB defines logical block which is used to store 
 *       information in DBO format. 
 *************************************************************
 */  
typedef ULONGLONG DBOLB_ADDR;

#define DBOLB_INVALID_ADDRESS           ((DBOLB_ADDR) -1)
#define UINT32_INVALID_ADDRESS         0xFFFFFFFF

#define DBOLB_PAGE_SIZE                 BE_BYTES_PER_BLOCK                         /* Block with OVERHEAD data */
#define DBOLB_USER_PAGE_SIZE            BYTES_PER_BLOCK                            /* Block without OVERHEAD data */
#define DBOLB_MANAGEMENT_DATA_SIZE      (DBOLB_USER_PAGE_SIZE - DBO_HEADER_SIZE)   /* OVERHEAD and DBO overhead data is not counted */
#define DBOLB_OVERHEAD_DATA_SIZE        SECTOR_OVERHEAD_BYTES                      /* Additional overhed */


/*************************************************************
 * DBO_HEADER (FIELDS): (28 | 32) + 8 =  (36 | 40) bytes 
 *   This structure represents Database Object common header;
 *   crc         - Computed CRC;
 *   rev_num     - Revision Number;
 *
 *   type        - DBO Type;
 *   id          - DBO Id;
 *   count       - Current number of DBOLB (multiblock format);
 *   offset      - Offset for data;
 *   length      - Total length of data left;
 *   next        - Next block address  (multiblock format);
 * raw: used to get raw access - needed for htonl and ntohl
 *************************************************************
 */
/* DBO_HEADER: 28 |32 Bytes */
typedef struct DBO_HEADER_FIELDS
{
     DBO_TYPE   type;                                  /* 4 bytes */
     DBO_ID     id;                                    /* 8 bytes */ 
     UINT_32E   count;                                 /* 4 bytes */
     UINT_32E   offset;                                /* 4 bytes */
     UINT_32E   length;                                /* 4 bytes */
     /* Though the following is an address field, we have to define it
      * as a UINT_32E in order to keep the manufacture zero mark
      * consistent. DBOLB_ADDRESS was 32-bit pre-R22 and 64-bit after
      */
     UINT_32E   next;                                  /* 4 | 8 bytes */

} DBO_HEADER_FIELDS;

#define DBO_HEADER_CRC_SIZE             (sizeof(DBO_HEADER_FIELDS))

typedef struct DBO_HEADER
{
     DBO_REVISION rev_num;                             /* 4 bytes */
     UINT_32E     crc;                                 /* 4 bytes */

     union
     {
          DBO_HEADER_FIELDS fields;
          UINT_8E           raw[DBO_HEADER_CRC_SIZE];  /* 28 | 32 bytes */
     } u;
          
} DBO_HEADER;

#define DBO_HEADER_SIZE                 sizeof(DBO_HEADER)

#define DBO_REVISION_ONE                ((DBO_REVISION)  1)
#define DBO_CURRENT_REVISION            DBO_REVISION_ONE


/*************************************************************
 * VALIDITY masks
 *************************************************************
 */  
typedef UINT_32 DBO_VALIDITY_MASK;

#define DBO_ALL_COPIES_INVALID                0x00000000
#define DBO_IS_COPY_VALID(bitmask, copy_n)    ((bitmask) & (1 << (copy_n)))
#define DBO_SET_COPY_INVALID(bitmask, copy_n) ((bitmask) &= ~(1 << (copy_n)))
#define DBO_SET_COPY_VALID(bitmask, copy_n)   ((bitmask) |= (1 << (copy_n)))


/************************
 * PROTOTYPE DEFINITIONS
 ************************/

BOOL dbo_memset(UINT_8E *data, 
                UINT_8E sym, 
                UINT_32 size);
BOOL dbo_memcmp(const void *rd1, 
                const void *rd2, 
                UINT_32 size,
                BOOL *status);
BOOL dbo_memcpy(void *rd_dest, 
                const void *rd_src, 
                UINT_32 size);

BOOL dbo_strcmp(const char *s1, 
                const char *s2, 
                BOOL *status);
BOOL dbo_strcpy(char *s1, 
                const char *s2);

BOOL dbo_DbolbData2UserData(UINT_8E *buf, 
                            UINT_32 *count_ptr, 
                            DBOLB_ADDR start_addr, 
                            DBO_TYPE *dbo_type_ptr, 
                            DBO_ID *dbo_id_ptr,
                            BOOL *status_ptr);
BOOL dbo_UserData2DbolbData(UINT_8E *buf, 
                            UINT_32 *count_ptr, 
                            const DBOLB_ADDR start_addr,
                            const DBO_TYPE *dbo_type_ptr, 
                            const DBO_ID *dbo_id_ptr);

BOOL dbo_DbolbNet2Host(DBO_HEADER *dbo_h, 
                       DBO_REVISION *rev_num_ptr, 
                       BOOL *status_ptr);
BOOL dbo_DbolbHost2Net(DBO_HEADER *dbo_h, 
                       const DBO_REVISION rev_num);

UINT_32 dbo_UserDataCount2DbolbCount(UINT_32 user_data_count);

#endif /* ODB_DBO_H_INCLUDED */
