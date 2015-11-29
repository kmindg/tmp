#ifndef TERMINATOR_SIMULATED_DISK_PRIVATE_H
#define TERMINATOR_SIMULATED_DISK_PRIVATE_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"

/*************** 
 * LITERALS
 ***************/
#define TERMINATOR_SIMULATED_DISK_UNINITIALIZED_READ_DATA   (0xBAD0DEAD40DA1ACDULL)



fbe_status_t terminator_simulated_disk_generate_zero_buffer(fbe_u8_t * data_buffer_pointer, 
                                                                  fbe_lba_t lba, 
                                                                  fbe_block_count_t block_count, 
                                                                  fbe_block_size_t block_size,
                                                                  fbe_bool_t use_valid_seed);

#endif /*TERMINATOR_SIMULATED_DISK_PRIVATE_H*/

