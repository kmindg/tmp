#include "fbe_hash_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fbe/fbe_emcutil_shell_include.h"

static int allocs;
static int deallocs;

void banner(char* message)
{
    printf("\n");
    printf("************************************\n");
    printf("* %s\n", message);
    printf("************************************\n");
}

/* Track allocation and deallocations */
void* my_alloc(int size)
{
    ++allocs;
    return malloc(size);
}

void* my_strdup(char *str)
{
    ++allocs;
    return _strdup(str);
}

void my_dealloc(void *chunk)
{
    ++deallocs;
    free(chunk);
}

typedef struct {
    unsigned int key;
    unsigned int data;
} key_data;


int int_comp(void* key1, void* key2)
{
    return  ( (int)key1 != (int)key2 );
}

void print_int_hash_entry(int h, int tb, int ab, void* k, void* d)
{
    printf("  %08x %2u->%2u %02u  %u\n", h, tb, ab, (int)k, *(int*)d);
}

void fbe_hash_table_int_print(void* key, void *data, void *context)
{
    printf("%u %u\n", (int)key, *(int*)data);
}

void print_string_hash_entry(int h, int tb, int ab, void* k, void* d)
{
    printf("  %08x %2u->%2u %s  %s\n", h, tb, ab, (char *)k, (char *)d);
}


void print_hash_table_info(fbe_hash_table_t *hash_table)
{
    printf("Num Entries %u\n", fbe_hash_table_get_num_entries(hash_table));
    printf("Num Buckets %u\n", fbe_hash_table_get_num_buckets(hash_table));
    printf("Num MaxChainLen %u\n", fbe_hash_table_get_max_chain_len(hash_table));
}

void validate_hash_table_size(fbe_hash_table_t *hash_table, fbe_u32_t size)
{
    if ( fbe_hash_table_get_num_buckets(hash_table) != size )
    {
        printf("Invalid number of buckets - should be %u\n", size);
        exit(1);
    }
}

int int_test(void)
{
    key_data data[32];
    unsigned int i;
    fbe_hash_table_t fbe_hash_table;
    void *del_key;
    void *del_data;

    banner("Tests for integer hash");
    
    banner("Testing hash initialization");
    
    if ( fbe_hash_table_init(&fbe_hash_table, 32 /* number of buckets */,
                             (fbe_hash_table_allocate_func)my_alloc, (fbe_hash_table_deallocate_func)my_dealloc,
                             (fbe_hash_table_hash_func)fbe_hash_table_int_hash, int_comp) != FBE_HASH_TABLE_OK )
    {
        printf("hash table failed to initialize\n");
        return 1;
    }

    fbe_hash_table_set_auto_resize(&fbe_hash_table, FBE_FALSE);
    
    banner("Testing hash insertion");

    /* Add 32 entries into the table */
    for ( i = 0; i < 32; ++i )
    {
        data[i].key = i;
        data[i].data = i + 1000;
        
        if ( fbe_hash_table_insert(&fbe_hash_table, (void *)(fbe_ptrhld_t)i, &data[i].data) != FBE_HASH_TABLE_OK )
        {
            printf("fbe_hash_table_insert failed for key %u\n", i);
            return 1;
        }
    }

    fbe_hash_table_dump(&fbe_hash_table, print_int_hash_entry);

    /* Get some stats */
    print_hash_table_info(&fbe_hash_table);

    if ( fbe_hash_table_get_num_entries(&fbe_hash_table) != 32 )
    {
        printf("Entry count is wrong ( is %u - should be 32 )\n", fbe_hash_table_get_num_entries(&fbe_hash_table) );
        return 1;
    }

    /* Insert another one - it must fail */
    if ( fbe_hash_table_insert(&fbe_hash_table, (void*)32, NULL) == FBE_HASH_TABLE_OK )
    {
        printf("fbe_hash_table_insert succeeded for key 32 - it should fail\n");
        return 1;
    }
    
    
    /* Test hash - get */
    for ( i = 0; i < 32; ++i )
    {
        int *key_data;

        if ( fbe_hash_table_get(&fbe_hash_table, (void *)(fbe_ptrhld_t)i, (void **)&key_data) != FBE_HASH_TABLE_OK )
        {
            printf("get failed for %u\n", i);
            return 1;
        }

        if ( *key_data != data[i].data )
        {
            printf("Invalid data returned for read\n");
        }
    }

    /* Test read of non-existant entry */
    if ( fbe_hash_table_get(&fbe_hash_table, (void*)33, NULL) != FBE_HASH_TABLE_KEY_NOT_FOUND )
    {
        printf("get returned success for non existant entry\n");
        return 1;
    }
    
    banner("Testing hash delete");

    /* Delete an entry from the middle of a list */
    if ( fbe_hash_table_delete(&fbe_hash_table, (void*)5, &del_key, &del_data) != FBE_HASH_TABLE_OK )
    {
        printf("Delete operation failed\n");
        return 1;
    }

    /* Delete an entry from the start of a list */
    if ( fbe_hash_table_delete(&fbe_hash_table, (void*)0, &del_key, &del_data) != FBE_HASH_TABLE_OK )
    {
        printf("Delete operation failed\n");
        return 1;
    }

    /* Delete an entry from the end of a list */
    if ( fbe_hash_table_delete(&fbe_hash_table, (void*)27, &del_key, &del_data) != FBE_HASH_TABLE_OK )
    {
        printf("Delete operation failed\n");
        return 1;
    }

    fbe_hash_table_dump(&fbe_hash_table, print_int_hash_entry);


    /* Get some stats */
    print_hash_table_info(&fbe_hash_table);
    
    if ( fbe_hash_table_get_num_entries(&fbe_hash_table) != 29 )
    {
        printf("Entry count is wrong ( is %u - should be 29 )\n", fbe_hash_table_get_num_entries(&fbe_hash_table) );
        return 1;
    }

    /* Resize the hash */
    banner("Shrinking  the hash");
    if ( fbe_hash_table_resize(&fbe_hash_table, 4) == FBE_HASH_TABLE_OK )
    {
        printf("Invalid Resize operation passed\n");
        return 1;
    }

    if ( fbe_hash_table_resize(&fbe_hash_table, 29) != FBE_HASH_TABLE_OK )
    {
        printf("Resize operation failed\n");
        return 1;
    }

    /* Get some stats */
    fbe_hash_table_dump(&fbe_hash_table, print_int_hash_entry);
    print_hash_table_info(&fbe_hash_table);
    
    if ( fbe_hash_table_get_num_entries(&fbe_hash_table) != 29 )
    {
        printf("Entry count is wrong ( is %u - should be 29 )\n", fbe_hash_table_get_num_entries(&fbe_hash_table) );
        return 1;
    }

    banner("Expanding  the hash");
    if ( fbe_hash_table_resize(&fbe_hash_table, 40) != FBE_HASH_TABLE_OK )
    {
        printf("Resize operation failed\n");
        return 1;
    }

    /* Get some stats */
    fbe_hash_table_dump(&fbe_hash_table, print_int_hash_entry);
    print_hash_table_info(&fbe_hash_table);
    
    if ( fbe_hash_table_get_num_entries(&fbe_hash_table) != 29 )
    {
        printf("Entry count is wrong ( is %u - should be 29 )\n", fbe_hash_table_get_num_entries(&fbe_hash_table) );
        return 1;
    }

    banner("Testing hash get");

    /* Test hash - get */
    for ( i = 0; i < 32; ++i )
    {
        int *key_data;

        /* Ignore the values we deleted earlier */
        if ( i == 5 || i == 0 || i == 27 ) continue;
        
        if ( fbe_hash_table_get(&fbe_hash_table, (void *)(fbe_ptrhld_t)i, (void **)&key_data) != FBE_HASH_TABLE_OK )
        {
            printf("get failed\n");
            return 1;
        }

        if ( *key_data != data[i].data )
        {
            printf("Invalid data returned for read\n");
        }
    }


    /* Test the traversal function */
    banner("Traversal");
    fbe_hash_table_traverse(&fbe_hash_table, fbe_hash_table_int_print, NULL);
    
    fbe_hash_table_clear(&fbe_hash_table, NULL, NULL);

    /* Test auto resize */
    banner("Testing auto resize");
    if ( fbe_hash_table_init(&fbe_hash_table, 5 /* number of buckets */,
                             (fbe_hash_table_allocate_func)my_alloc, (fbe_hash_table_deallocate_func)my_dealloc,
                             (fbe_hash_table_hash_func)fbe_hash_table_int_hash, int_comp) != FBE_HASH_TABLE_OK )
    {
        printf("hash table failed to initialize\n");
        return 1;
    }

    /* Start by adding a couple of entries */
    for ( i = 0; i < 2; ++i )
    {
        if ( fbe_hash_table_insert(&fbe_hash_table, (void *)(fbe_ptrhld_t)i, &data[i].data) != FBE_HASH_TABLE_OK )
        {
            printf("fbe_hash_table_insert failed for key %u\n", i);
            return 1;
        }
    }
    
    fbe_hash_table_dump(&fbe_hash_table, print_int_hash_entry);
    print_hash_table_info(&fbe_hash_table);
    validate_hash_table_size(&fbe_hash_table, 5);


    /* Make sure we cannot add the same key twice */
    if ( fbe_hash_table_insert(&fbe_hash_table, (void*)1, NULL) == FBE_HASH_TABLE_OK )
    {
        printf("fbe_hash_table_insert failed to recognise a duplicate key %u\n", i);
        return 1;
    }
    
    /* This will force a resize */
    for ( i = 2; i < 4; ++i )
    {
        if ( fbe_hash_table_insert(&fbe_hash_table, (void *)(fbe_ptrhld_t)i, &data[i].data) != FBE_HASH_TABLE_OK )
        {
            printf("fbe_hash_table_insert failed for key %u\n", i);
            return 1;
        }
    }

    fbe_hash_table_dump(&fbe_hash_table, print_int_hash_entry);
    print_hash_table_info(&fbe_hash_table);
    validate_hash_table_size(&fbe_hash_table, 10);
    
    /* This will force another resize */
    for ( i = 4; i < 8; ++i )
    {
        if ( fbe_hash_table_insert(&fbe_hash_table, (void *)(fbe_ptrhld_t)i, &data[i].data) != FBE_HASH_TABLE_OK )
        {
            printf("fbe_hash_table_insert failed for key %u\n", i);
            return 1;
        }
    }

    fbe_hash_table_dump(&fbe_hash_table, print_int_hash_entry);
    print_hash_table_info(&fbe_hash_table);
    validate_hash_table_size(&fbe_hash_table, 20);
    
    /* Now remove some entries to force a resize to a smaller array */
    for ( i = 0; i < 5; ++i )
    {
        if ( fbe_hash_table_delete(&fbe_hash_table, (void *)(fbe_ptrhld_t)i, NULL, NULL) != FBE_HASH_TABLE_OK )
        {
            printf("fbe_hash_table_delete failed for key %u\n", i);
            return 1;
        }
    }

    fbe_hash_table_dump(&fbe_hash_table, print_int_hash_entry);
    print_hash_table_info(&fbe_hash_table);
    validate_hash_table_size(&fbe_hash_table, 7);
    
    fbe_hash_table_clear(&fbe_hash_table, NULL, NULL);
    return 0;
}




fbe_hash_table_hash string_hash(void* void_key)
{
    char *str = void_key;
    fbe_hash_table_hash hash = 5381;
    fbe_hash_table_hash c;

    while(c = *str++)
    {
        hash = hash * 33 ^ c;
    }

    return hash;
}

int string_comp(void* key1, void* key2)
{
    return  ( strcmp((char*)key1, (char*)key2 ));
}

void print_string(void* s)
{
    printf("%s", (char*)s);
}


char* keys[] = { "Hello", "World", "Wild", "Thing", NULL };
char* data[] = { "World", "Hello", "Thing", "Wild", NULL };


int string_test(void)
{
    unsigned int i;
    fbe_hash_table_t fbe_hash_table;
    char *key_data;

    banner("Tests for string hash");


    printf("Testing hash initialization\n");
    
    if ( fbe_hash_table_init(&fbe_hash_table, 16, (fbe_hash_table_allocate_func)my_alloc,
                             (fbe_hash_table_deallocate_func)my_dealloc,
                             string_hash, string_comp) != FBE_HASH_TABLE_OK )
    {
        printf("hash table failed to initialize\n");
        return 1;
    }

    printf("Testing hash insertion\n");

    i = 0;
    while(keys[i] != NULL )
    {
        if ( fbe_hash_table_insert(&fbe_hash_table, keys[i], my_strdup(data[i])) != FBE_HASH_TABLE_OK )
        {
            printf("fbe_hash_table_insert failed for key %u\n", i);
            return 1;
        }

        ++i;
    }

    fbe_hash_table_dump(&fbe_hash_table, print_string_hash_entry);
    
    if ( fbe_hash_table_get(&fbe_hash_table, "Hello", (void **)&key_data) != FBE_HASH_TABLE_OK )
    {
        printf("get failed\n");
        return 1;
    }

    if ( strcmp(key_data, "World") )
    {
        printf("Invalid data returned for get\n");
        return 1;
    }

    printf("Looking up key \"Hello\" ... retrieved value \"%s\"\n", key_data);
    
    /* Clear all entries and call free for all the data items */
    fbe_hash_table_clear(&fbe_hash_table, NULL, (fbe_hash_table_deallocate_func)my_dealloc);

    return 0;
}


int __cdecl main(int argc, char** argv)
{
    int r;

#include "fbe/fbe_emcutil_shell_maincode.h"

    r = int_test();
    if ( r != 0 )
    {
        return r;
    }

    r = string_test();

    printf("Total number of allocs - %u\n", allocs);
    printf("Total number of deallocs - %u\n", deallocs);
    
    return r;
}

