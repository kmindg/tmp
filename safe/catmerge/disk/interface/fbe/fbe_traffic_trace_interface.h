#ifndef FBE_TRAFFIC_TRACE_INTERFACE_H
#define FBE_TRAFFIC_TRACE_INTERFACE_H

#include "fbe/fbe_service.h"
//#include "fbe/fbe_winddk.h"

/*!************************************************************
* @enum fbe_traffic_trace_control_code_t
*
* @brief 
*    This contains the enum data info for traffic trace control code.
*
***************************************************************/
typedef enum fbe_traffic_trace_control_code_e {
    FBE_TRAFFIC_TRACE_CONTROL_CODE_INVALID = FBE_SERVICE_CONTROL_CODE_INVALID_DEF(FBE_SERVICE_ID_TRAFFIC_TRACE),
    FBE_TRAFFIC_TRACE_CONTROL_CODE_ENABLE_RBA_TRACE, /*! Enable RBA tracing. */
    FBE_TRAFFIC_TRACE_CONTROL_CODE_DISABLE_RBA_TRACE, /*! Disable RBA tracing. */
    FBE_TRAFFIC_TRACE_CONTROL_CODE_LAST
} fbe_traffic_trace_control_code_t;

/*!************************************************************
* @enum fbe_traffic_trace_rba_set_class_t
*
* @brief 
*    This contains the enum data info setting traffic service correspond to objects
*
***************************************************************/
typedef enum fbe_traffic_trace_rba_set_class_e {
    FBE_TRAFFIC_TRACE_CLASS_INAVLID= 0,
    FBE_TRAFFIC_TRACE_CLASS_LUN,
    FBE_TRAFFIC_TRACE_CLASS_RG,
    FBE_TRAFFIC_TRACE_CLASS_RG_FRU,
    FBE_TRAFFIC_TRACE_CLASS_VD,
    FBE_TRAFFIC_TRACE_CLASS_PVD,
    FBE_TRAFFIC_TRACE_CLASS_PD,
    FBE_TRAFFIC_TRACE_CLASS_LAST
} fbe_traffic_trace_rba_set_class_t;


void  fbe_traffic_trace_enable(fbe_traffic_trace_rba_set_class_t tag);
void  fbe_traffic_trace_disable(fbe_traffic_trace_rba_set_class_t tag);
#endif /* FBE_TRAFFIC_TRACE_INTERFACE_H */

