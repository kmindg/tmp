#ifndef DBA_EXPORT_API_H
#define DBA_EXPORT_API_H 0x0000001
#define FLARE__STAR__H
/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  dba_export_api.h
 ***************************************************************************
 *
 *      Description
 *
 *         This file contains that portion of the interface to the
 *         Database Access "dba" library in the Flare driver that is used
 *         outside of Flare.
 *
 *  History:
 *
 *      07/25/01 CJH    Created (from dba_api.h)
 *      10/02/01 LBV    Add fru_flag ODBS_QUEUED_SWAP
 *      10/17/01 CJH    move defines from dba_api.h
 *      11/01/01 HEW    Added ifdefs for C++ compilation
 *      04/17/02 HEW    Added CALL_TYPE to decl for dba_frusig_get_cache
 ***************************************************************************/

#include "generics.h"
#include "k10defs.h"
#include "cm_config_exports.h"
#include "vp_exports.h"
#include "drive_handle_interface.h"

#include "dba_export_types.h"
#include "dba_export_intfc.h"

#endif /* DBA_EXPORT_API_H */
