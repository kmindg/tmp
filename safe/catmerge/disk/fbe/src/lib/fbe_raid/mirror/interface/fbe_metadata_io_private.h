#ifndef METADATA_IO_PRIVATE_H
#define METADATA_IO_PRIVATE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_metadata_io_private.h
 ***************************************************************************
 *
 * @brief   This file contains private defines for the metadata io algorithms.
 *          Since the I/O path isn't concerned with the actual metadata, this
 *          file only contains private data for handle the modification of the
 *          metadata (treated as any othe user's data) not the handling of the
 *          metadata contents.
 * 
 * @ingroup virtual_drive_class_files
 * @ingroup provision_drive_class_files
 * 
 * @version
 *  09/28/2009  Ron Proulx  - Created
 *
 ***************************************************************************/

#include "fbe/fbe_transport.h"
#include "fbe_block_transport.h"

/******************************
 * LITERAL DEFINITIONS.
 ******************************/

/**************************************** 
    fbe_metadata_util.c
 ****************************************/

/**************************************** 
    fbe_metadata_executor.c
 ****************************************/
fbe_status_t fbe_metadata_io_block_transport_entry(fbe_transport_entry_context_t context, 
                                                   fbe_packet_t * packet);


#endif /* METADATA_IO_PRIVATE_H */

/*******************************
 * end fbe_metadata_io_private.h
 *******************************/
