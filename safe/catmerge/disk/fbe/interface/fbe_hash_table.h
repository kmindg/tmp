/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*! @defgroup fbe_hash_table FBE Hash Table 
 *  @brief This is the set of definitions for FBE Hash Tables
 * @{
 */ 

/************************************************************************/
/** @file fbe_hash_table.h
 ************************************************************************
 *
 *  DESCRIPTION:
 *  This file contains declaration for a hash table
 *
 *  HISTORY:
 *  01/07/10 - Lewis Alderton Created.
 *************************************************************************
 */

#ifndef FBE_HASH_TABLE_H
#define FBE_HASH_TABLE_H

#include "fbe/fbe_types.h"


#define BUCKET_NOT_USED ( (fbe_u32_t) -1 )
#define BUCKET_DELETED  ( (fbe_u32_t) -2 )


/*********************************************************************/
/** @enum fbe_hash_table_status_t
 *********************************************************************
 * @brief Enums for hash table function return codes. If these values
 *        are altered/appended to then the fbe_hash_table_strerror function
 *        MUST also be changed appropriatly 
 *
 *********************************************************************/
typedef enum { FBE_HASH_TABLE_OK = 0,
               FBE_HASH_TABLE_ALREADY_INITIALIZED,
               FBE_HASH_TABLE_KEY_EXISTS,
               FBE_HASH_TABLE_KEY_NOT_FOUND,
               FBE_HASH_TABLE_ALLOCATION_ERROR,
               FBE_HASH_TABLE_INVALID_TABLE,
               FBE_HASH_TABLE_FULL,
               FBE_HASH_TABLE_ITERATOR_LAST,
               
               FBE_HASH_TABLE_LAST /* last enum */
} fbe_hash_table_status_t;

/*********************************************************************/
/** @typedef fbe_hash_table_hash
 *********************************************************************
 * @brief typedef to store a hashed key ( 32 bits )
 *********************************************************************/
typedef fbe_u32_t fbe_hash_table_hash;

/*********************************************************************/
/** @typedef fbe_hash_table_hash_func
 *********************************************************************
 * @brief typedef of a function that generates hashes
 *********************************************************************/
typedef fbe_hash_table_hash(*fbe_hash_table_hash_func)(void *key);

/*********************************************************************/
/** @typedef fbe_hash_table_key_comp
 *********************************************************************
 * @brief typedef of a function that compares keys
 *********************************************************************/
typedef int(*fbe_hash_table_key_comp)(void *key1, void *key2);


/*********************************************************************/
/** @typedef fbe_hash_table_allocate_func
 *********************************************************************
 * @brief allocate function for internal hash structures
 *********************************************************************/
typedef void* (*fbe_hash_table_allocate_func)(fbe_u32_t size);

/*********************************************************************/
/** @typedef fbe_hash_table_deallocate_func
 *********************************************************************
 * @brief release funtion for internal hash structures
 *********************************************************************/
typedef void (*fbe_hash_table_deallocate_func)(void* chunk);

/*********************************************************************/
/** @typedef fbe_hash_table_traversal_func
 *********************************************************************
 * @brief Traversal function. This function will be called for every
 *        table entry during a traversal
 *********************************************************************/
typedef void (*fbe_hash_table_traversal_func)(void *key, void *data, void *context);

typedef struct hash_bucket_entry_s
{
    /* Pointer to the key */
    void           *key;

    /* The data we are storing */
    void           *data;

    /* We keep the hash around so we don't have to recalulate them during a rebuild */
    fbe_hash_table_hash hash;

    fbe_u32_t target_bucket;
} hash_bucket_entry_t;

typedef struct
{
    fbe_u32_t              num_entries;
    fbe_u32_t              num_buckets;
    fbe_u32_t              min_buckets;
    hash_bucket_entry_t   *buckets;
    
    fbe_hash_table_hash_func   hash_function;
    fbe_hash_table_key_comp    key_compare;

    // Allocate deallocate functions
    fbe_hash_table_allocate_func   allocate;
    fbe_hash_table_deallocate_func deallocate;

    fbe_bool_t auto_resize;
} fbe_hash_table_t;

typedef struct
{
    fbe_hash_table_t  *hash_table;
    fbe_u32_t          bucket_no;
} fbe_hash_table_iterator_t;

fbe_hash_table_status_t fbe_hash_table_init(fbe_hash_table_t*, fbe_u32_t init_num_buckets,
                                            fbe_hash_table_allocate_func allocate, fbe_hash_table_deallocate_func deallocate,
                                            fbe_hash_table_hash_func hash_function, fbe_hash_table_key_comp key_compare);

fbe_hash_table_status_t fbe_hash_table_set_auto_resize(fbe_hash_table_t* hash_table, fbe_bool_t auto_resize);

fbe_hash_table_status_t fbe_hash_table_auto_resize(fbe_hash_table_t *hash_table);

fbe_hash_table_status_t fbe_hash_table_insert(fbe_hash_table_t*, void *key, void *data);

fbe_hash_table_status_t fbe_hash_table_get(fbe_hash_table_t*, void *key, void **get_data);

fbe_hash_table_status_t fbe_hash_table_delete(fbe_hash_table_t*, void *key, void **del_key, void **del_data);

fbe_hash_table_status_t fbe_hash_table_resize(fbe_hash_table_t*, fbe_u32_t num_buckets);

fbe_hash_table_status_t fbe_hash_table_clear(fbe_hash_table_t *fbe_hash_table,
                                     fbe_hash_table_deallocate_func del_key, fbe_hash_table_deallocate_func del_data);

fbe_u32_t fbe_hash_table_get_num_entries(fbe_hash_table_t*);
fbe_u32_t fbe_hash_table_get_num_buckets(fbe_hash_table_t*);
fbe_u32_t fbe_hash_table_get_max_chain_len(fbe_hash_table_t*);

void fbe_hash_table_dump(fbe_hash_table_t* fbe_hash_table, void (*print_hash_entry)(int, int, int, void*, void*) );

void fbe_hash_table_iterator_init(fbe_hash_table_t *fbe_hash_table, fbe_hash_table_iterator_t *iterator);
fbe_hash_table_status_t fbe_hash_table_iterator_next(fbe_hash_table_iterator_t *iterator, void** key, void** data);

void fbe_hash_table_traverse(fbe_hash_table_t *fbe_hash_table, fbe_hash_table_traversal_func traverse_function, void *context);

/* create a hash key from an integer */
fbe_hash_table_hash fbe_hash_table_int_hash(fbe_u32_t key);
fbe_hash_table_hash fbe_hash_table_int_hash_with_hint(fbe_u32_t key, fbe_hash_table_hash hint);

char* fbe_hash_table_strerror(fbe_hash_table_status_t status);

/*! @} */ /* end of group fbe_hash_table */

#endif /* FBE_HASH_TABLE_H */
