/*********************************************************************
* Copyright (C) EMC Corporation, 1989 - 2011
* All Rights Reserved.
* Licensed Material-Property of EMC Corporation.
* This software is made available solely pursuant to the terms
* of a EMC license agreement which governs its use.
*********************************************************************/

/*********************************************************************
*
*  Description:
*      This file defines methods of the Ioctl utility class. 
*
*  Notes:
*       These methods handle various operations that can be useful
*       for the entire application. 
*       
*  History:
*      07-March-2011 : initial version
*
*********************************************************************/

#ifndef T_IOCTL_UTIL_CLASS_H
#include "ioctl_util.h"
#endif

/*********************************************************************
* iUtilityClass :: EditWWn
**********************************************************************
*
*   Description:
*       Converts wwn into K10_LU_ID data type.
*
*   Input:
*       Wwn*    - Converted Wwn 
*       argv[]  - argument string (wwn)
*       temp*   - output messages
*
*   Returns:
*       FBE_STATUS_OK or FBE_STATUS_INVALID
*  
*   History
*       5-March-2012 : initial version
*
**********************************************************************/

fbe_status_t iUtilityClass::EditWWn(
    K10_LU_ID *Wwn, char **argv, char *temp)
{
    
    ULONGLONG port, node;
    int rtn = sscanf(*argv, "%016llX:%016llX", (unsigned long long*)&port, (unsigned long long *)&node);
    
    sprintf(temp, "%s  port %016llX node %016llX \n", 
        "iUtilityClass::EditWWn", port, node);

    if (rtn != 2) {
        sprintf(temp, "Incorrect format: WWN ID must be in the "
            "format: xxxxxxxxxxxxxxxx:xxxxxxxxxxxxxxxx\n%s\n", 
            *argv);
        return FBE_STATUS_INVALID;   
    }
    
    char* pPort = (char*)&port; 
	char* pNode = (char*)&node;
	
    for (int i = 7; i >= 0; i--){
		memcpy(&Wwn->nPort.bytes[i], pPort++, 1);
		memcpy(&Wwn->node.bytes[i],  pNode++, 1);         
	}
    
    return FBE_STATUS_OK;
};

/*********************************************************************
* ioctl_util.cpp end of file
*********************************************************************/
