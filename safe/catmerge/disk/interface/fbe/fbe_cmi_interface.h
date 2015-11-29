#ifndef FBE_CMI_INTERFACE_H
#define FBE_CMI_INTERFACE_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_package.h"

typedef enum fbe_cmi_control_code_e {
	FBE_CMI_CONTROL_CODE_INVALID = FBE_SERVICE_CONTROL_CODE_INVALID_DEF(FBE_SERVICE_ID_CMI),
    FBE_CMI_CONTROL_CODE_GET_INFO,
	FBE_CMI_CONTROL_CODE_DISABLE_TRAFFIC, /*disable all incoming traffic*/
	FBE_CMI_CONTROL_CODE_ENABLE_TRAFFIC, /*enable all incoming traffic*/
    FBE_CMI_CONTROL_CODE_GET_IO_STATISTICS,
    FBE_CMI_CONTROL_CODE_CLEAR_IO_STATISTICS,
	FBE_CMI_CONTROL_CODE_SET_PEER_VERSION, /*disable all incoming traffic*/
	FBE_CMI_CONTROL_CODE_CLEAR_PEER_VERSION, /*enable all incoming traffic*/
	FBE_CMI_CONTROL_CODE_LAST
} fbe_cmi_control_code_t;

//----------------------------------------------------------------
// Define the FBE API CMI Interface for the Usurper Interface. 
// This is where all the data structures defined. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_cmi_interface_usurper_interface FBE API CMI Interface Usurper Interface
 *  @brief 
 *    This is the set of definitions that comprise the FBE API CMI Interface class
 *    usurper interface.
 *
 *  @ingroup fbe_classes_usurper_interface
 *  @{
 */ 
//----------------------------------------------------------------

/*!*******************************************************************
 * @enum fbe_cmi_sp_id_t
 *********************************************************************
 * @brief
 *  This is the enum used to know if we are SPA or SPB
 *
 * @ingroup fbe_api_cmi_interface
 *********************************************************************/

typedef enum fbe_cmi_sp_id_e{
	FBE_CMI_SP_ID_INVALID,/*!< FBE_CMI_SP_ID_INVALID - Illegal value */
	FBE_CMI_SP_ID_A,/*!< FBE_CMI_SP_ID_A - Indicates this is SPA */
	FBE_CMI_SP_ID_B/*!< FBE_CMI_SP_ID_B - Indicates this is SPB  */
}fbe_cmi_sp_id_t;

/*!*******************************************************************
 * @enum fbe_cmi_sp_state_t
 *********************************************************************
 * @brief
 *  This is the enum used to know if we are active or passive
 *
 * @ingroup fbe_api_cmi_interface
 *********************************************************************/
typedef enum fbe_cmi_sp_state_e{
	FBE_CMI_STATE_INVALID,/*!< FBE_CMI_STATE_INVALID - Illegal value */
	FBE_CMI_STATE_ACTIVE,/*!< FBE_CMI_STATE_ACTIVE - Indicates this is the Active SP*/
	FBE_CMI_STATE_PASSIVE,/*!< FBE_CMI_STATE_PASSIVE - Indicates this is the passive SP (no background activity) */
	FBE_CMI_STATE_BUSY,	/*!< FBE_CMI_STATE_BUSY - Indicates the SP is still busy handshaking in CMI level */
	FBE_CMI_STATE_SERVICE_MODE /*!< FBE_CMI_STATE_SERVICE_MODE - Indicates the SP is the service mode */

}fbe_cmi_sp_state_t;

/*!*******************************************************************
 * @struct fbe_cmi_service_get_info_t
 *********************************************************************
 * @brief
 *  This is the strucutre used for FBE_CMI_CONTROL_CODE_GET_INFO opcode
 *  use it to get information about the CMI service status
 *
 * @ingroup fbe_api_cmi_interface
 *********************************************************************/

typedef struct fbe_cmi_service_get_info_s{
	fbe_bool_t			peer_alive;/*!< peer_alive - Is our peer alive */
	fbe_cmi_sp_state_t	sp_state;/*!< sp_state - what state are we in (active or passive) */
	fbe_cmi_sp_id_t		sp_id;/*!< sp_id - are we SPA or SPB */
}fbe_cmi_service_get_info_t;

/*!*******************************************************************
 * @enum fbe_cmi_service_get_io_statistics_t
 *********************************************************************
 * @brief
 *  This is the structure used for FBE_CMI_CONTROL_CODE_GET_IO_STATISTICS
 *  use it to get IO statistics
 *
 * @ingroup fbe_api_cmi_interface
 *********************************************************************/
typedef struct fbe_cmi_service_get_io_statistics_s{
    fbe_u32_t conduit_id;
    fbe_u64_t sent_ios;
    fbe_u64_t sent_bytes;
    fbe_u64_t received_ios;
    fbe_u64_t received_bytes;
}fbe_cmi_service_get_io_statistics_t;

/*! @} */ /* end of group fbe_api_cmi_interface_usurper_interface */


#endif /* FBE_CMI_INTERFACE_H */

