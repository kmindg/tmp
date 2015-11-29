#ifndef ENABLER_VALUES_H
#define ENABLER_VALUES_H 0x00000001	/* from common dir */

/***************************************************************************
 * Copyright (C) EMC Corporation 2004
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/****************************************************************
 *
 *  Description:
 *  This header file defines registry values that enable tiers and
 *  optional features.
 *
 *  Notes:
 *  Updates to this file should occur in the Main source branch,
 *  then be propagated to all other branches.
 *
 *  Registry value definitions should be wide character strings.
 *
 ****************************************************************/

// Piranha LC tier enabler
// Registry location: \Registry\Machine\System\CurrentControlSet\Services\K10DriverConfig\PlatformTier
// (The id is for internal driver use and not stored in the registry.)

#define TIER_SELECTOR_AX_SERIES_LC1     L"AX_SERIES_LC1"
#define TIER_SELECTOR_AX_SERIES_LC1_ID  99


// Piranha (non-LC) and Grand Teton tier enabler
// Registry location: \Registry\Machine\System\CurrentControlSet\Services\K10DriverConfig\PlatformTier
// (The id is for internal driver use and not stored in the registry.)

#define TIER_SELECTOR_AX_SERIES_STD1    L"AX_SERIES_STD1"
#define TIER_SELECTOR_AX_SERIES_STD1_ID 100

// AX Family Dual-SP key.
#define K10DualSPKey    "DualSP"

#endif	/* ndef ENABLER_VALUES_H */
