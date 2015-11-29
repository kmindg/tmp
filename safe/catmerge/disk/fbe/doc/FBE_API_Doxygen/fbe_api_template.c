/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_esp_encl_mgmt_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the encl_mgmt interface for the Environmental Services package.
 *
 * @ingroup fbe_api_esp_interface_class_files
 * @ingroup fbe_api_esp_encl_mgmt_interface
 *
 * @note
 *   Two @ingroup are needed.  One is for the top level class files
 *   and the other is the file specific interface (.h) file.
 *
 * @version
 *   03/24/2010:  Created. Ashok Tamilarasan
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
>>>> Add included here
>>>>

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!***************************************************************
 * @fn fbe_api_esp_encl_mgmt_get_encl_info(
 *         fbe_esp_encl_mgmt_encl_info_t *encl_info)
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve enclosure
 *  information from the ESP ENCL Mgmt object.
 *
 * @param
 *   encl_info - Buffer to store the return information
 *
 * @return
 *   fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @note
 *  The caller needs to fill in the location value in
 *  the fbe_esp_encl_mgmt_encl_info_t structure before calling
 *  this function
 *
 * @version
 *  03/24/10 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_esp_encl_mgmt_get_encl_info(fbe_esp_encl_mgmt_encl_info_t *encl_info)
{
	...
}
