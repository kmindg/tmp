#ifndef FBE_ENCLOSURE_INTERFACE_H
#define FBE_ENCLOSURE_INTERFACE_H

#include "fbe/fbe_winddk.h"
#include "familyfruid.h"
#include "fbe/fbe_devices.h"

/*!*************************************************************************
 *  @def FBE_ENCLOSURE_MAX_IMAGE_HEADER_SIZE
 *  @brief Define for maximum microcode image header size
 ***************************************************************************/ 
#define FBE_ENCLOSURE_MAX_IMAGE_HEADER_SIZE     400

/*!*************************************************************************
 *  @def FBE_ENCLOSURE_MAX_SUBENCL_PRODUCT_ID_SIZE
 *  @brief Define for maximum subencl product id size
 ***************************************************************************/ 
#define FBE_ENCLOSURE_MAX_SUBENCL_PRODUCT_ID_SIZE     16

/*!*************************************************************************
 *  @def FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE
 *  @brief Define for maximum firmware rev size
 ***************************************************************************/ 
#define FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE     16

/*!*************************************************************************
 *  @def FBE_ENCLOSURE_MAX_NUM_SUBENCL_PRODUCT_ID_IN_IMAGE
 *  @brief Define the maximum number of subenclosure product id which
 *  the firmware image can be downloaded for.
 ***************************************************************************/ 
#define FBE_ENCLOSURE_MAX_NUM_SUBENCL_PRODUCT_ID_IN_IMAGE     20

/*!*************************************************************************
 *  @def FBE_ENCLOSURE_MAX_REV_COUNT_PER_PS
 *  @brief Define the maximum number of version per PS 
 ***************************************************************************/ 
#define FBE_ENCLOSURE_MAX_REV_COUNT_PER_PS    4 //!< each Cayenne replaceable ps unit really conatins 2 individual ps with 2 micros each

/*!*************************************************************************
 *  @enum fbe_enclosure_firmware_status_t
 *  @brief 
 *    Defines the valid status codes for firmware download process.
 ***************************************************************************/ 
typedef enum fbe_enclosure_firmware_status_e {
    FBE_ENCLOSURE_FW_STATUS_NONE        = 0x00, //!< no operation in progress
    FBE_ENCLOSURE_FW_STATUS_IN_PROGRESS = 0x01, //!< operation in progress
    FBE_ENCLOSURE_FW_STATUS_FAIL        = 0x02, //!< requested operation failed, used internally in PP.
    FBE_ENCLOSURE_FW_STATUS_ABORT       = 0x03,  //!< operation aborted
    FBE_ENCLOSURE_FW_STATUS_DOWNLOAD_FAILED  = 0x04,  //!< operation status returned to the client when download failed.
    FBE_ENCLOSURE_FW_STATUS_ACTIVATE_FAILED  = 0x05,  //!< operation status returned to the client when activate failed.
    FBE_ENCLOSURE_FW_STATUS_UNKNOWN     = 0xFF  //!< status unknown
} fbe_enclosure_firmware_status_t;


/*!*************************************************************************
 *  @enum fbe_enclosure_firmware_ext_status_t
 *  @brief 
 *    Defines the Extended Status codes for firmware download process. These
 *  codes provide additional information to fbe_enclosure_firmware_status_t.
 ***************************************************************************/ 
typedef enum fbe_enclosure_firmware_ext_status_e {
    FBE_ENCLOSURE_FW_EXT_STATUS_NONE          = 0x00,   //!< no extended status
    FBE_ENCLOSURE_FW_EXT_STATUS_REQ           = 0x01,   //!< extended status requested
    FBE_ENCLOSURE_FW_EXT_STATUS_IMAGE_LOADED  = 0x02,   //!< image is downloaded and waiting for activation
    FBE_ENCLOSURE_FW_EXT_STATUS_UNKNOWN       = 0x03,   //!< unknown status
    FBE_ENCLOSURE_FW_EXT_STATUS_NO_ABORT      = 0x04,   //!< can't abort the operation
    FBE_ENCLOSURE_FW_EXT_STATUS_BUSY          = 0x05,   //!< busy status from ema
    FBE_ENCLOSURE_FW_EXT_STATUS_ERR_PAGE      = 0x13,   //!< page field error
    FBE_ENCLOSURE_FW_EXT_STATUS_ERR_CHECKSUM  = 0x14,   //!< checksum error
    FBE_ENCLOSURE_FW_EXT_STATUS_ERR_TO        = 0x15,   //!< image timeout error
    FBE_ENCLOSURE_FW_EXT_STATUS_ERR_ACTIVATE  = 0x16,   //!< image activation error
    FBE_ENCLOSURE_FW_EXT_STATUS_ERR_NO_IMAGE  = 0x17,   //!< no image to activate
    FBE_ENCLOSURE_FW_EXT_STATUS_ERR_LFC       = 0x18,    //!< lifecycle error
    FBE_ENCLOSURE_FW_EXT_STATUS_TUNNEL_PROCESSING = 0x19, //!< processing tunnel command 
    FBE_ENCLOSURE_FW_EXT_STATUS_TUNNEL_FAILED = 0x20     //!< tunnel command failed
} fbe_enclosure_firmware_ext_status_t;

/*!*************************************************************************
 *  @enum fbe_enclosure_port_open_status_t
 *  @brief 
 *    Defines the enclosure's port open status.
 ***************************************************************************/ 
typedef enum fbe_enclosure_port_open_status_e
{
    FBE_ENCLOSURE_PORT_OPEN_STATUS_FALSE    = 0,
    FBE_ENCLOSURE_PORT_OPEN_STATUS_TRUE     = 1,
    FBE_ENCLOSURE_PORT_OPEN_STATUS_UNKNOWN  = 254,
    FBE_ENCLOSURE_PORT_OPEN_STATUS_NA       = 255
}fbe_enclosure_port_open_status_t;

/*!********************************************************************* 
 * @enum fbe_esp_lcc_type_t 
 *  
 * @brief 
 *   The enclosure LCC types.
 *
 **********************************************************************/
typedef enum fbe_lcc_type_s
{
    FBE_LCC_TYPE_UNKNOWN = 0x00,
    FBE_LCC_TYPE_6G_VIPER,
    FBE_LCC_TYPE_6G_DERRINGER,
    FBE_LCC_TYPE_6G_VOYAGER_ICM,
    FBE_LCC_TYPE_6G_VOYAGER_EE,
    FBE_LCC_TYPE_6G_SENTRY_15,
    FBE_LCC_TYPE_6G_SENTRY_25,
    FBE_LCC_TYPE_6G_JETFIRE,
    FBE_LCC_TYPE_6G_BEACHCOMBER_12,
    FBE_LCC_TYPE_6G_BEACHCOMBER_25,
    FBE_LCC_TYPE_6G_PINECONE,
    FBE_LCC_TYPE_6G_SILVERBOLT_12,
    FBE_LCC_TYPE_6G_SILVERBOLT_25,
    FBE_LCC_TYPE_6G_VIKING,
    FBE_LCC_TYPE_12G_CAYENNE,
    FBE_LCC_TYPE_12G_NAGA,
    FBE_LCC_TYPE_12G_ANCHO,
    FBE_LCC_TYPE_12G_TABASCO,
    FBE_LCC_TYPE_12G_CALYPSO,
    FBE_LCC_TYPE_12G_MIRANDA,
    FBE_LCC_TYPE_12G_RHEA,
    FBE_LCC_TYPE_MERIDIAN,   //vVNX VPE
    FBE_LCC_TYPE_MAX
} fbe_lcc_type_t;

/*!********************************************************************* 
 * @struct fbe_lcc_info_t
 *  
 * @brief 
 *   The struct of lcc info.  
 **********************************************************************/
typedef struct fbe_lcc_info_s{
    fbe_bool_t      inserted;
    fbe_bool_t      faulted;
    fbe_u8_t        additionalStatus;
    fbe_bool_t      bDownloadable;
    fbe_bool_t      hasResumeProm;
    fbe_bool_t      isLocal;
    fbe_bool_t      isCru;
    fbe_bool_t      ecbFault;
    fbe_u16_t       eses_version_desc;
    /* For CDES-1, firmwareRev is the bundled firmware revision for LCC. 
     * For CDES-2, there is no more bundled firmware revision for LCC.
     * So it is used to save the expander firmware (i.e. primary firmware) revision.
     */
    fbe_char_t      firmwareRev[FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1];
    fbe_char_t      initStrFirmwareRev[FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1];
    fbe_char_t      fpgaFirmwareRev[FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1];
    fbe_char_t      subenclProductId[FBE_ENCLOSURE_MAX_SUBENCL_PRODUCT_ID_SIZE + 1];
    HW_MODULE_TYPE  uniqueId;
    fbe_bool_t      shunted;
    fbe_enclosure_port_open_status_t    expansionPortOpen;
    fbe_lcc_type_t    lccType;
    fbe_u64_t       lccActualHwType;  // represents the real hw type of this lcc (eg. beachcomber lcc is actually SP board)
    fbe_u32_t       currentSpeed;
    fbe_u32_t       maxSpeed;
    fbe_bool_t      resumePromReadFailed;
    fbe_bool_t      fupFailure;
} fbe_lcc_info_t;

/*!********************************************************************* 
 * @struct fbe_ccable_status_t
 *  
 * @brief 
 *   The cable status values  
 **********************************************************************/
typedef enum fbe_cable_status_e{
    FBE_CABLE_STATUS_UNKNOWN = 0,
    FBE_CABLE_STATUS_GOOD,
    FBE_CABLE_STATUS_DEGRADED,
    FBE_CABLE_STATUS_DISABLED,
    FBE_CABLE_STATUS_MISSING,
    FBE_CABLE_STATUS_CROSSED,
    FBE_CABLE_STATUS_INVALID = 0xFF,
} fbe_cable_status_t;

/*!********************************************************************* 
 * @enum fbe_connector_type_t 
 *  
 * @brief 
 *   The enclosure Connector types.
 *
 **********************************************************************/
typedef enum fbe_connector_type_e
{
    FBE_CONNECTOR_TYPE_UNKNOWN = 0,
    FBE_CONNECTOR_TYPE_PRIMARY,
    FBE_CONNECTOR_TYPE_EXPANSION,
    FBE_CONNECTOR_TYPE_UNUSED,
    FBE_CONNECTOR_TYPE_INTERNAL,
} fbe_connector_type_t;

/*!********************************************************************* 
 * @struct fbe_connector_info_t
 *  
 * @brief 
 *   The struct of connector info.  
 **********************************************************************/
typedef struct fbe_connector_info_s{
    fbe_bool_t              isLocalFru;
    fbe_bool_t              inserted;
    fbe_cable_status_t      cableStatus;
    fbe_connector_type_t    connectorType;
    fbe_u32_t               connectorId;
} fbe_connector_info_t;

typedef struct fbe_rev_info_s{
    fbe_u8_t                       firmwareRevValid;
    fbe_bool_t                     bDownloadable;
    fbe_u8_t                       firmwareRev[FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1];
} fbe_rev_info_t;


typedef struct fbe_ssc_info_s{
    fbe_bool_t      inserted;
    fbe_bool_t      faulted;
} fbe_ssc_info_t;

#endif /* FBE_ENCLOSURE_INTERFACE_H */
