#include "sep_tests.h"
#include "fbe/fbe_api_common.h"


/*!**************************************************************
 * @fn fbe_test_verify_storage_extent_package_no_trace_error()
 ****************************************************************
 * @brief
 *  Check the trace errors from storage_extent_package.  Assert if not zero.  
 *
 * @param none.
 *
 * @return fbe_status_t - FBE_STATUS_OK/FBE_STATUS_GENERIC_FAILURE.
 *
 * HISTORY:
 *  8/26/2009 - Created. VG
 *
 ****************************************************************/
fbe_status_t fbe_test_verify_storage_extent_package_no_trace_error(void)
{
#if 0 // FBE30
    fbe_api_trace_error_counters_t trace_error_counters;
    fbe_status_t status;
    status = fbe_api_trace_get_error_counter(&trace_error_counters, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "failed to get trace error counters.")

    MUT_ASSERT_INT_EQUAL_MSG(SEP_EXPECTED_ERROR, trace_error_counters.trace_critical_error_counter, "Storage Extent Package has critical error trace")
    MUT_ASSERT_INT_EQUAL_MSG(SEP_EXPECTED_ERROR, trace_error_counters.trace_error_counter, "Storage Extent Package has error trace")
#endif
    return FBE_STATUS_OK;
}

