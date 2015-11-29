/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************/
/** @file fbe_hash_table.c
 ***************************************************************************
 * @brief
 *  This file contains a hash table implementation. This is a basic table
 * using open addressing with a fixed linear probe interval of 1
 * 
 * @ingroup fbe_hash_table
 ***************************************************************************/

#include "fbe_hash_table.h"
#include <stdio.h>
#include <string.h>

#define FBE_HASH_TABLE_HIGH_WATERMARK_PCT 70
#define FBE_HASH_TABLE_LOW_WATERMARK_PCT 20
#define FBE_HASH_SWEET_SPOT_PCT 40

/*********************************************************************/
/** fbe_hash_table_int_hash_with_hint()
 *********************************************************************
 * @brief This function hashes an int with a starting hint
 *
 * @param  key  - the key to hash
 * @param  hint - the hint
 *
 * @return fbe_hash_table_hash - the hashed int
 *
 *********************************************************************/
fbe_hash_table_hash fbe_hash_table_int_hash_with_hint(fbe_u32_t key, fbe_hash_table_hash hint)
{
    key = (key^61) ^ (key >> 16 );
    key = key + ( key << 3 );
    key = key ^ ( key >> 4 );
    key = key * hint;
    key = key ^ ( key >> 15 );
    return key;
}


/*********************************************************************/
/** fbe_hash_table_int_hash()
 *********************************************************************
 * @brief This function hashes an int
 *
 * @param  key  - the key to hash
 *
 * @return fbe_hash_table_hash - the hashed int
 *
 *********************************************************************/
fbe_hash_table_hash fbe_hash_table_int_hash(fbe_u32_t key)
{
    return fbe_hash_table_int_hash_with_hint(key,
                                             0x27d4eb2d /* a large prime that just happens to work rather well */);
}


/*********************************************************************/
/** fbe_hash_table_init()
 *********************************************************************
 * @brief Initialize a hash table
 *
 * @param  fbe_hash_table - the table to initialize
 * @param  num_buckets    - the number of buckets to allocate ( should be around 70% of the expected table size )
 * @param  allocate       - function to call to allocate internal table structures
 * @param  deallocate     - function to call to release internal table structures
 * @param  hash_function  - function to use to hash keys in this table
 * @param  key_compare    - function to use to compare keys in this table.
 *
 * @return fbe_hash_table_status_t
 *
 *********************************************************************/
fbe_hash_table_status_t fbe_hash_table_init(fbe_hash_table_t *fbe_hash_table, fbe_u32_t init_num_buckets,
                                            fbe_hash_table_allocate_func allocate, fbe_hash_table_deallocate_func deallocate,
                                            fbe_hash_table_hash_func hash_function, fbe_hash_table_key_comp key_compare)
{
    fbe_u32_t allocation_size;
    fbe_u32_t i;
    
    if ( fbe_hash_table == NULL)
    {
        return FBE_HASH_TABLE_INVALID_TABLE;
    }

    /* Too many buckets */
    if ( init_num_buckets == BUCKET_NOT_USED ||
         init_num_buckets == BUCKET_DELETED )
    {
        return FBE_HASH_TABLE_FULL;
    }
    
    memset(fbe_hash_table, 0, sizeof(*fbe_hash_table));

    allocation_size = init_num_buckets * sizeof(*fbe_hash_table->buckets);
    fbe_hash_table->buckets = allocate(allocation_size);
    if ( fbe_hash_table->buckets == NULL )
    {
        return FBE_HASH_TABLE_ALLOCATION_ERROR;
    }

    memset(fbe_hash_table->buckets, 0, allocation_size);
    
    fbe_hash_table->num_buckets = init_num_buckets;
    fbe_hash_table->min_buckets = init_num_buckets;

    fbe_hash_table->hash_function = hash_function;
    fbe_hash_table->key_compare = key_compare;

    fbe_hash_table->allocate = allocate;
    fbe_hash_table->deallocate = deallocate;

    for ( i = 0 ; i < init_num_buckets; ++i )
    {
        fbe_hash_table->buckets[i].target_bucket = BUCKET_NOT_USED;
    }

    /* Auto resize is 'on' be default */
    fbe_hash_table->auto_resize = FBE_TRUE;
    
    return FBE_HASH_TABLE_OK;
}

fbe_hash_table_status_t fbe_hash_table_set_auto_resize(fbe_hash_table_t *hash_table, fbe_bool_t auto_resize)
{
    hash_table->auto_resize = auto_resize;

    if ( auto_resize )
    {
        return fbe_hash_table_auto_resize(hash_table);
    }

    return FBE_HASH_TABLE_OK;
}

fbe_hash_table_status_t fbe_hash_table_auto_resize(fbe_hash_table_t *hash_table)
{
    fbe_u32_t load_factor_pct;

    load_factor_pct = 100 * hash_table->num_entries / hash_table->num_buckets;

    /* Do we need to increase the size */
    if ( load_factor_pct > FBE_HASH_TABLE_HIGH_WATERMARK_PCT )
    {
        fbe_u32_t new_size = 100 *  hash_table->num_entries / FBE_HASH_SWEET_SPOT_PCT;

        return fbe_hash_table_resize(hash_table, new_size);
    }

    /* Do we need to decrease the size */
    if ( load_factor_pct < FBE_HASH_TABLE_LOW_WATERMARK_PCT )
    {
        fbe_u32_t new_size = 100 *  hash_table->num_entries / FBE_HASH_SWEET_SPOT_PCT;

        if ( new_size > hash_table->min_buckets )
        {
            return fbe_hash_table_resize(hash_table, new_size);
        }
    }

    return FBE_HASH_TABLE_OK;
}


/* Convert a hash to a bucket number */
static fbe_hash_table_status_t hash_to_bucket(fbe_u32_t num_buckets, 
                                              fbe_hash_table_hash hash,
                                              fbe_u32_t *bucket_no)
{
    if ((fbe_s32_t)num_buckets <= 0)
    {
        return FBE_HASH_TABLE_INVALID_TABLE;
    }
    *bucket_no = hash % num_buckets;
    return FBE_HASH_TABLE_OK;
}

/* Find the next unused bucket */
static fbe_hash_table_status_t hash_table_next_free_bucket(hash_bucket_entry_t *buckets,
                                                           fbe_u32_t            num_buckets,
                                                           fbe_u32_t           *bucket)
{
    fbe_u32_t i;

    /* If the target bucket is empty ( or has been deleted ) then we can use it */
    if ( buckets[*bucket].target_bucket == BUCKET_NOT_USED ||
         buckets[*bucket].target_bucket == BUCKET_DELETED )
    {
        return FBE_HASH_TABLE_OK;
    }

    /* Find the next bucket that is either empty or has been deleted */
    i = (*bucket + 1) % num_buckets;
    while(i != *bucket )
    {
        if ( buckets[i].target_bucket == BUCKET_NOT_USED ||
             buckets[i].target_bucket == BUCKET_DELETED )
        {
            *bucket = i;
            return FBE_HASH_TABLE_OK;
        }

        i = ( i + 1 ) % num_buckets;
    }

    return FBE_HASH_TABLE_FULL;
}

static fbe_hash_table_status_t hash_table_find_bucket_for_key(fbe_hash_table_t   *fbe_hash_table,
                                                              fbe_u32_t          *actual_bucket,
                                                              fbe_hash_table_hash hash,
                                                              void               *key)
{
    fbe_u32_t i;

    /* If the target and actual buckets are the same we could have a match */
    /* If the hashes are the same then it is "very" likely that we have a match */
    /* But we always have to check the key - we do this last as it is a relativly
       expensive operation */
    if ( fbe_hash_table->buckets[*actual_bucket].target_bucket == *actual_bucket &&
         fbe_hash_table->buckets[*actual_bucket].hash          == hash &&
         fbe_hash_table->key_compare(fbe_hash_table->buckets[*actual_bucket].key, key ) == 0 )
    {
        return FBE_HASH_TABLE_OK;
    }

    /* Not found in the expected bucket - try the next ones */
    i = (*actual_bucket + 1) % fbe_hash_table->num_buckets;
    while ( i != *actual_bucket )
    {
        /* If the bucket is empty then we did not find the key */
        if ( fbe_hash_table->buckets[*actual_bucket].target_bucket == BUCKET_NOT_USED )

        {
            return FBE_HASH_TABLE_KEY_NOT_FOUND;
        }

        /* If this is a NOT deleted entry then see if it is the one we want */
        if ( fbe_hash_table->buckets[i].target_bucket == *actual_bucket &&
             fbe_hash_table->buckets[i].hash          == hash &&
             fbe_hash_table->key_compare(fbe_hash_table->buckets[i].key, key ) == 0 )
        {
            *actual_bucket = i;
            return FBE_HASH_TABLE_OK;
        }

        i = (i + 1) % fbe_hash_table->num_buckets;
    }

    return FBE_HASH_TABLE_KEY_NOT_FOUND;
}


/*********************************************************************/
/** fbe_hash_table_insert()
 *********************************************************************
 * @brief insert an entry into a hash table
 *
 * @param  fbe_hash_table - hash table
 * @param  key            - key to add into the table
 * @param  data           - data associated with the key
 *
 * @return fbe_hash_table_status_t
 *
 *********************************************************************/
fbe_hash_table_status_t fbe_hash_table_insert(fbe_hash_table_t *fbe_hash_table, void *key, void *data)
{
    fbe_hash_table_hash hash;
    unsigned int target_bucket;
    unsigned int actual_bucket;
    fbe_hash_table_status_t status;
    
    if ( fbe_hash_table == NULL)
    {
        return FBE_HASH_TABLE_INVALID_TABLE;
    }
        
    /* get the hash */
    hash = fbe_hash_table->hash_function(key);

    /* Get the bucket */
    status = hash_to_bucket(fbe_hash_table->num_buckets, hash, &target_bucket);
    if ( status != FBE_HASH_TABLE_OK )
    {
        return status;
    }

    /* See if the key already exists */
    actual_bucket = target_bucket;
    status = hash_table_find_bucket_for_key(fbe_hash_table, &actual_bucket, hash, key);
    if ( status == FBE_HASH_TABLE_OK )
    {
        return FBE_HASH_TABLE_KEY_EXISTS;
    }
    
    /* Find the next free bucket */
    actual_bucket = target_bucket;
    status = hash_table_next_free_bucket(fbe_hash_table->buckets, fbe_hash_table->num_buckets, &actual_bucket);
    if ( status != FBE_HASH_TABLE_OK )
    {
        return status;
    }

    fbe_hash_table->buckets[actual_bucket].key = key;
    fbe_hash_table->buckets[actual_bucket].data = data;
    fbe_hash_table->buckets[actual_bucket].hash = hash;
    fbe_hash_table->buckets[actual_bucket].target_bucket = target_bucket;
    
    ++fbe_hash_table->num_entries;

    if ( fbe_hash_table->auto_resize )
    {
        return fbe_hash_table_auto_resize(fbe_hash_table);
    }
    
    return FBE_HASH_TABLE_OK;
}

/*********************************************************************/
/** fbe_hash_table_get()
 *********************************************************************
 * @brief retrieve an entry from a hash table.
 *
 * @param  fbe_hash_table - hash table
 * @param  key            - key to lookup from the table
 * @param  data           - if non NULL then the data associated with "key" will
 *                        - be written here
 *
 * @return fbe_hash_table_status_t : FBE_HASH_TABLE_KEY_NOT_FOUND if the key does not exist
 *
 *********************************************************************/
fbe_hash_table_status_t fbe_hash_table_get(fbe_hash_table_t *fbe_hash_table, void *key, void **key_data)
{
    fbe_hash_table_hash     hash;
    unsigned int            target_bucket;
    unsigned int            actual_bucket;
    fbe_hash_table_status_t status;
    
    if ( fbe_hash_table == NULL)
    {
        return FBE_HASH_TABLE_INVALID_TABLE;
    }
        
    /* get the hash */
    hash = fbe_hash_table->hash_function(key);

    /* Get the bucket */
    status = hash_to_bucket(fbe_hash_table->num_buckets, hash, &target_bucket);
    if ( status != FBE_HASH_TABLE_OK )
    {
        return status;
    }

    /* Get the actual bucket */
    actual_bucket = target_bucket;
    status = hash_table_find_bucket_for_key(fbe_hash_table, &actual_bucket, hash, key);

    if ( status != FBE_HASH_TABLE_OK )
    {
        return status;
    }

    if ( key_data )
    {
        *key_data = fbe_hash_table->buckets[actual_bucket].data;
    }
    
    return FBE_HASH_TABLE_OK;
}

/*********************************************************************/
/** fbe_hash_table_delete()
 *********************************************************************
 * @brief delete an entry from a hash table.
 *
 * @param  fbe_hash_table - hash table
 * @param  key            - key to delete from the table
 * @param  del_key        - if non NULL then the stored key will be written here
 * @param  del_data       - if non NULL then the stored data will be written here
 *
 * @return fbe_hash_table_status_t
 *
 *********************************************************************/
fbe_hash_table_status_t fbe_hash_table_delete(fbe_hash_table_t* fbe_hash_table, void *key, void **del_key, void **del_data)
{
    fbe_hash_table_status_t status;
    fbe_hash_table_hash     hash;
    unsigned int            target_bucket;
    unsigned int            actual_bucket;
    
    if ( fbe_hash_table == NULL)
    {
        return FBE_HASH_TABLE_INVALID_TABLE;
    }

    /* get the hash */
    hash = fbe_hash_table->hash_function(key);
    
    /* Get the bucket */
    status = hash_to_bucket(fbe_hash_table->num_buckets, hash, &target_bucket);
    if ( status != FBE_HASH_TABLE_OK )
    {
        return status;
    }

    /* Get the actual bucket */
    actual_bucket = target_bucket;
    status = hash_table_find_bucket_for_key(fbe_hash_table, &actual_bucket, hash, key);

    if ( status != FBE_HASH_TABLE_OK )
    {
        return status;
    }

    if ( del_key )
    {
        *del_key = fbe_hash_table->buckets[actual_bucket].key;
    }

    if ( del_data )
    {
        *del_data = fbe_hash_table->buckets[actual_bucket].data;
    }
    
    fbe_hash_table->buckets[actual_bucket].target_bucket = BUCKET_DELETED;

    --fbe_hash_table->num_entries;
    
    if ( fbe_hash_table->auto_resize )
    {
        return fbe_hash_table_auto_resize(fbe_hash_table);
    }

    return FBE_HASH_TABLE_OK;
}

/*********************************************************************/
/** fbe_hash_table_iterator_init()
 *********************************************************************
 * @brief delete an entry from a hash table.
 *
 * @param  fbe_hash_table - hash table
 * @param  iterator       - iterator to initialize
 *********************************************************************/
void fbe_hash_table_iterator_init(fbe_hash_table_t *fbe_hash_table, fbe_hash_table_iterator_t *iterator)
{
    iterator->hash_table = fbe_hash_table;
    iterator->bucket_no = BUCKET_NOT_USED;
}

/*********************************************************************/
/** fbe_hash_table_iterator_next()
 *********************************************************************
 * @brief Move an iterator to the next element
 *
 * @param  iterator       - iterator
 * @param  key            - if non NULL the next key is written here
 * @param  data           - if non NULL the data associated with the
 *                          next key is written here
 *
 * @return fbe_hash_table_status_t : FBE_HASH_TABLE_ITERATOR_LAST if there
 *                                   are no more entries
 *********************************************************************/
fbe_hash_table_status_t fbe_hash_table_iterator_next(fbe_hash_table_iterator_t *iterator, void** key, void** data)
{
    if ( iterator->bucket_no == BUCKET_NOT_USED )
    {
        iterator->bucket_no = 0;
    }
    else
    {
        iterator->bucket_no = ( iterator->bucket_no + 1 ) % iterator->hash_table->num_buckets;

        if ( iterator->bucket_no == 0 )
        {
            return FBE_HASH_TABLE_ITERATOR_LAST;
        }
    }

    do
    {
        if ( iterator->hash_table->buckets[iterator->bucket_no].target_bucket != BUCKET_NOT_USED &&
             iterator->hash_table->buckets[iterator->bucket_no].target_bucket != BUCKET_DELETED )
        {
            break;
        }

        iterator->bucket_no = ( iterator->bucket_no + 1 ) % iterator->hash_table->num_buckets;

        if ( iterator->bucket_no == 0 )
        {
            return FBE_HASH_TABLE_ITERATOR_LAST;
        }

    } while(1);

    if ( key )
    {
        *key = iterator->hash_table->buckets[iterator->bucket_no].key;
    }
    
    if ( data )
    {
        *data = iterator->hash_table->buckets[iterator->bucket_no].data;
    }

    return FBE_HASH_TABLE_OK;
}

/*********************************************************************/
/** fbe_hash_table_traverse()
 *********************************************************************
 * @brief Traverse a hash table - the traversal function is called for
 *        every element
 *
 * @param  fbe_hash_table     - hash_table
 * @param  traversal_function - the function to call for every element
 * @param  context            - opaque contextual data passed to the traversal function
 *********************************************************************/
void fbe_hash_table_traverse(fbe_hash_table_t *fbe_hash_table, fbe_hash_table_traversal_func traversal_function, void *context)
{
    fbe_hash_table_iterator_t i;
    void *data;
    void *key;
    
    fbe_hash_table_iterator_init(fbe_hash_table, &i);

    while( (fbe_hash_table_iterator_next(&i, &key, &data) == FBE_HASH_TABLE_OK) )
    {
        traversal_function(key, data, context);
    }
}

/* Traversal callback function type for internal use */
typedef fbe_hash_table_status_t (*fbe_hash_table_internal_traversal_func)(fbe_hash_table_iterator_t *iterator, void *context);


/* Internally used traversal function - passes the traversal function the iterator ( instead */
/* of the key/data pair used by the external traversal function ) */
static
fbe_hash_table_status_t fbe_hash_table_traverse_internal(fbe_hash_table_t *fbe_hash_table,
                                                         fbe_hash_table_internal_traversal_func traversal_function,
                                                         void *context)
{
    fbe_hash_table_iterator_t i;
    fbe_hash_table_iterator_init(fbe_hash_table, &i);

    while(fbe_hash_table_iterator_next(&i, NULL, NULL) == FBE_HASH_TABLE_OK )
    {
        fbe_hash_table_status_t status = traversal_function(&i, context);
        if ( status != FBE_HASH_TABLE_OK )
        {
            return status;
        }
    }

    return FBE_HASH_TABLE_OK;
}

/* This is the type of the context data we pass for a hash table dump */
typedef struct
{
    void (*print_hash_entry)(int, int, int, void*, void*);
} hash_dump_traversal_funcs_t;

/* this function is called for each element when doing a dump */
static
fbe_hash_table_status_t fbe_hash_table_dump_traversal_func(fbe_hash_table_iterator_t *iterator,
                                                           hash_dump_traversal_funcs_t *context)
{
    if ( iterator->hash_table->buckets[iterator->bucket_no].target_bucket != BUCKET_NOT_USED &&
         iterator->hash_table->buckets[iterator->bucket_no].target_bucket != BUCKET_DELETED )
    {
        context->print_hash_entry(iterator->hash_table->buckets[iterator->bucket_no].hash,
                                  iterator->hash_table->buckets[iterator->bucket_no].target_bucket,
                                  iterator->bucket_no,
                                  iterator->hash_table->buckets[iterator->bucket_no].key,
                                  iterator->hash_table->buckets[iterator->bucket_no].data);
    }

    return FBE_HASH_TABLE_OK;
}

void fbe_hash_table_dump(fbe_hash_table_t* fbe_hash_table, void (*print_hash_entry)(int, int, int, void*, void*) )
{
    hash_dump_traversal_funcs_t funcs = { print_hash_entry };
    fbe_hash_table_traverse_internal(fbe_hash_table, (fbe_hash_table_internal_traversal_func)&fbe_hash_table_dump_traversal_func, &funcs);
}

/* This typedef is used when traversing the hash table to find the max chain length */
typedef struct
{
    fbe_u32_t max_chain_length;
} hash_get_max_chain_length_t;

static
fbe_hash_table_status_t fbe_hash_table_max_chain_length_traversal_func(fbe_hash_table_iterator_t *iterator,
                                                                       hash_get_max_chain_length_t *context)
{
    fbe_u32_t current_chain_length;

    current_chain_length = (iterator->bucket_no - iterator->hash_table->buckets[iterator->bucket_no].target_bucket )
        % iterator->hash_table->num_buckets;
    
    if ( current_chain_length > context->max_chain_length )
    {
        context->max_chain_length = current_chain_length;
    }

    return FBE_HASH_TABLE_OK;
}


/*********************************************************************/
/** fbe_hash_table_get_max_chain_len()
 *********************************************************************
 * @brief Find the longest hash chain length
 *
 * @param  fbe_hash_table     - hash_table
 *
 * @return fbe_u32_t : The longest chain length
 *********************************************************************/
fbe_u32_t fbe_hash_table_get_max_chain_len(fbe_hash_table_t* fbe_hash_table)
{
    hash_get_max_chain_length_t lengths = {0};
    fbe_hash_table_traverse_internal(fbe_hash_table, (fbe_hash_table_internal_traversal_func)&fbe_hash_table_max_chain_length_traversal_func, &lengths);
    return lengths.max_chain_length;
}

/*********************************************************************/
/** fbe_hash_table_get_num_entries()
 *********************************************************************
 * @brief Returns the number of table entries
 *
 * @param  fbe_hash_table     - hash_table
 *
 * @return fbe_u32_t : The longest number of entries
 *********************************************************************/
fbe_u32_t fbe_hash_table_get_num_entries(fbe_hash_table_t* fbe_hash_table)
{
    return fbe_hash_table->num_entries;
}

/*********************************************************************/
/** fbe_hash_table_get_num_buckets()
 *********************************************************************
 * @brief Returns the number of buckets
 *
 * @param  fbe_hash_table     - hash_table
 *
 * @return fbe_u32_t : The longest number of buckets
 *********************************************************************/
fbe_u32_t fbe_hash_table_get_num_buckets(fbe_hash_table_t* fbe_hash_table)
{
    return fbe_hash_table->num_buckets;
}

/* typedefs for resize context */
typedef struct
{
    hash_bucket_entry_t *new_buckets;
    fbe_u32_t            num_new_buckets;
} fbe_hash_table_resize_context_t;
    

fbe_hash_table_status_t fbe_hash_table_resize_traversal_func(fbe_hash_table_iterator_t *iterator,
                                                             fbe_hash_table_resize_context_t *context)
{
    fbe_hash_table_status_t status;
    fbe_u32_t new_target_bucket;
    fbe_u32_t new_actual_bucket;

    status = hash_to_bucket(context->num_new_buckets, 
                            iterator->hash_table->buckets[iterator->bucket_no].hash, 
                            &new_target_bucket);
    if ( status != FBE_HASH_TABLE_OK )
    {
        return status;
    }

    new_actual_bucket = new_target_bucket;
    status = hash_table_next_free_bucket(context->new_buckets, context->num_new_buckets, &new_actual_bucket);

    context->new_buckets[new_actual_bucket] = iterator->hash_table->buckets[iterator->bucket_no];
    context->new_buckets[new_actual_bucket].target_bucket = new_target_bucket;
    
    return status;
}


/*********************************************************************/
/** fbe_hash_table_resize()
 *********************************************************************
 * @brief Resize the hash table - increase/decrease the number of buckets
 *
 * @param  fbe_hash_table     - hash_table
 * @param  num_buckets        - the number of buckets to resize to
 *
 * @return fbe_hash_table_status_t
 *********************************************************************/
fbe_hash_table_status_t fbe_hash_table_resize(fbe_hash_table_t* fbe_hash_table, fbe_u32_t num_buckets)
{
    fbe_u32_t                       allocation_size;
    fbe_hash_table_status_t         status;
    fbe_hash_table_resize_context_t context = { NULL, num_buckets };
    fbe_u32_t                       i;
    
    if ( fbe_hash_table == NULL)
    {
        return FBE_HASH_TABLE_INVALID_TABLE;
    }

    // Do nothing if the no change in the number of buckets
    if ( num_buckets == fbe_hash_table->num_buckets )
    {
        return FBE_HASH_TABLE_OK;
    }

    allocation_size = num_buckets * sizeof(hash_bucket_entry_t);
    context.new_buckets = fbe_hash_table->allocate(allocation_size);
    if ( fbe_hash_table->buckets == NULL )
    {
        return FBE_HASH_TABLE_ALLOCATION_ERROR;
    }

    memset(context.new_buckets, 0, allocation_size);

    for ( i = 0 ; i < num_buckets; ++i )
    {
        context.new_buckets[i].target_bucket = BUCKET_NOT_USED;
    }

    status = fbe_hash_table_traverse_internal(fbe_hash_table, (fbe_hash_table_internal_traversal_func)&fbe_hash_table_resize_traversal_func, &context);
    
    if ( status != FBE_HASH_TABLE_OK )
    {
        fbe_hash_table->deallocate(context.new_buckets);
        return status;
    }
    
    /* Get rid of the old buckets */
    fbe_hash_table->deallocate(fbe_hash_table->buckets);
    fbe_hash_table->buckets = context.new_buckets;
    fbe_hash_table->num_buckets = num_buckets;
    
    return FBE_HASH_TABLE_OK;
}


typedef struct
{
    fbe_hash_table_deallocate_func del_key;
    fbe_hash_table_deallocate_func del_data;
} fbe_hash_table_clear_context_t;

static fbe_hash_table_status_t fbe_hash_table_clear_func(fbe_hash_table_iterator_t *iterator,
                                                         fbe_hash_table_clear_context_t *context )
{
    /* Delete the key if we specified a callback */
    if (context->del_key != NULL )
    {
        context->del_key(iterator->hash_table->buckets[iterator->bucket_no].key);
    }

    /* Delete the data if we specified a callback */
    if (context->del_data != NULL )
    {
        context->del_data(iterator->hash_table->buckets[iterator->bucket_no].data);
    }
    
    return FBE_HASH_TABLE_OK;
}

/*********************************************************************/
/** fbe_hash_table_clear()
 *********************************************************************
 * @brief Clear out a hash table - delete all keys and associated data
 *
 * @param  fbe_hash_table - hash_table
 * @param  del_key        - function to call to delete keys
 * @param  del_data       - function to call to delete data
 *
 * @return fbe_hash_table_status_t
 *********************************************************************/
fbe_hash_table_status_t fbe_hash_table_clear(fbe_hash_table_t *fbe_hash_table,
                                             fbe_hash_table_deallocate_func del_key, fbe_hash_table_deallocate_func del_data)
{
    fbe_hash_table_clear_context_t context = { del_key, del_data};
    fbe_hash_table_status_t status = fbe_hash_table_traverse_internal(fbe_hash_table, (fbe_hash_table_internal_traversal_func)&fbe_hash_table_clear_func, &context);
    
    fbe_hash_table->deallocate(fbe_hash_table->buckets);
    fbe_hash_table->num_buckets = 0;
    fbe_hash_table->num_entries = 0;

    return status;
}

typedef struct fbe_hash_table_status_message_s {
    fbe_hash_table_status_t status;
    char                   *message;
} fbe_hash_table_status_message_t;


static fbe_hash_table_status_message_t fbe_hash_table_messages[] = {
    { FBE_HASH_TABLE_OK, "Ok" },
    { FBE_HASH_TABLE_ALREADY_INITIALIZED, "Table already initialized" },
    { FBE_HASH_TABLE_KEY_EXISTS, "Key exists" },
    { FBE_HASH_TABLE_KEY_NOT_FOUND, "Key not found" },
    { FBE_HASH_TABLE_ALLOCATION_ERROR, "Allocation Error" },
    { FBE_HASH_TABLE_INVALID_TABLE, "Invalid table" },
    { FBE_HASH_TABLE_ITERATOR_LAST, "Iterator finished" },
    { FBE_HASH_TABLE_LAST, "Invalid status code" } };
                   
/*********************************************************************/
/** fbe_hash_table_strerror()
 *********************************************************************
 * @brief Get a textual message from an error code
 *
 * @param  status - status code to convert to a text message
 *
 * @return a text message
 *********************************************************************/
char* fbe_hash_table_strerror(fbe_hash_table_status_t status)
{
    fbe_u32_t i = 0;

    for ( ; fbe_hash_table_messages[i].status != FBE_HASH_TABLE_LAST; ++i )
    {
        if ( fbe_hash_table_messages[i].status == status )
        {
            break;
        }
    }

    return fbe_hash_table_messages[i].message;
}
