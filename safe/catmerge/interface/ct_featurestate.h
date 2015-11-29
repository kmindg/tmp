#ifndef CT_FEATURESTATE_H
#define CT_FEATURESTATE_H

#include "csx_ext.h"

#define FEATURESTATE_ENABLED   0

#define FEATURESTATE_HIDDEN    1

#define FEATURESTATE_EXCLUDED  2


#ifdef FEATURESTATE_SELF
#define FUNCTYPE CSX_MOD_EXPORT
#else
#define FUNCTYPE CSX_MOD_IMPORT
#endif

FUNCTYPE csx_status_e get_feature_state( csx_cstring_t feature, csx_u64_t *rv );

#endif
