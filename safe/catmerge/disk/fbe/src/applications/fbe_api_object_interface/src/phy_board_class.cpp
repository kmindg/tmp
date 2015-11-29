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
*      This file defines the methods of the PHYSICAL BOARD class.
*
*  Notes:
*      The PHYSICAL BOARD class (PhyBoard) is a derived class of 
*      the base class (bPhysical).
*
*  History:
*      05-Aug-2011 : inital version
*
*********************************************************************/

#ifndef T_PHY_BOARD_CLASS_H
#include "phy_board_class.h"
#endif

/*********************************************************************
* PhyBoard class :: Constructor
*********************************************************************/

PhyBoard::PhyBoard() : bPhysical()
{
    // Store Object number
    idnumber = (unsigned) PHY_BOARD_INTERFACE;
    phyBoardCount = ++gPhyBoardCount;

    // Store Object name
    idname = new char [IDNAME_LENGTH];
    strcpy(idname, "PHY_BOARD_INTERFACE");

    //Initializing variables
     phy_board_initializing_variable();
    
    if (Debug) {
        sprintf(temp, "PhyBoard class constructor (%d) %s\n", 
                idnumber, idname);
        vWriteFile(dkey, temp);
    }

    // <List of FBE API PHYSICAL BOARD Interface function calls>
    phyBoardInterfaceFuncs = 
        "[function call]\n" \
        "[short name]       [FBE API PHYSICAL BOARD Interface]\n" \
        " ------------------------------------------------------------\n" \
        " getBoardObjId     fbe_api_get_board_object_id\n" \
        " ------------------------------------------------------------\n";
    
    // Define common usage for PHYSICAL BOARD commands
    usage_format = 
        " Usage: %s\n"
        " For example: %s";
};

/*********************************************************************
* PhyBoard class : Accessor methods
*********************************************************************/

unsigned PhyBoard::MyIdNumIs(void)
{
    return idnumber;
};

char * PhyBoard::MyIdNameIs(void)
{
    return idname;
};

void PhyBoard::dumpme(void)
{ 
    strcpy (key, "PhyBoard::dumpme");
    sprintf(temp, "Object id: (%d) count: (%d)\n", 
            idnumber, phyBoardCount);
    vWriteFile(key, temp);
}

/*********************************************************************
* PhyBoard Class :: select()
*********************************************************************
*
*  Description:
*      This function looks up the function short name to validate
*      it and then calls the function with the index passed back 
*      from the compare.
*            
*  Input: Parameters
*      index - index into arguments
*      argc  - argument count 
*      *argv - pointer to arguments 
*    
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h 
*
*  History
*      28-Jun-2011 : inital version
*
*********************************************************************/

fbe_status_t PhyBoard::select(int index, int argc, char *argv[]) 
{
    // Assign default values
    status = FBE_STATUS_GENERIC_FAILURE;
    strcpy (key, "Select PHY interface"); 

    // List phyBoardCount Interface calls if help option and 
    // less than 6 arguments were entered.
    if (help && argc <= 6) {
        strcpy (key, "Argument [short name]");
        HelpCmds( (char *) phyBoardInterfaceFuncs);

        // get board object id
    } else if (strcmp (argv[index], "GETBOARDOBJID") == 0) {
        status = get_board_object_id(argc, &argv[index]);

        // can't find match on short name
    } else {
        sprintf(temp, "<ERROR!> Argument [short name] %s [%s]\n", 
                BAD_INPUT, argv[index]);
        vWriteFile(ukey, temp);
        HelpCmds( (char *) phyBoardInterfaceFuncs );
    } 

    return status;
}

/*********************************************************************
* PhyBoard Class :: get_board_object_id()
*********************************************************************
*
*  Description:
*      Gets the board object id
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*********************************************************************/

fbe_status_t PhyBoard::get_board_object_id(int argc, char ** argv)
{
    // Define specific usage string  
    usage_format =
        " Usage: %s \n"
        " For example: %s ";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getBoardObjId",
        "PhyBoard::get_board_object_id",
        "fbe_api_get_board_object_id",
        usage_format,
        argc, 6);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Make api call: 
    status = fbe_api_get_board_object_id(&object_id);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Failed to get the board object id");
    } else {
        edit_board_object_id(object_id); 
    }
    
    params[0] = '\0';

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* PhyBoard Class :: edit_board_object_id() (private method)
*********************************************************************
*
*  Description:
*      Format the board object id to display
*
*  Input: Parameters
*      object_id  - board object id

*  Returns:
*      void
*********************************************************************/

void PhyBoard::edit_board_object_id(fbe_object_id_t object_id)
{
    // board object id
    sprintf(temp,         "\n "
        "<Board Object Id>       0x%X\n ",
        object_id);
}

/*********************************************************************
* PhyBoard Class :: phy_board_initializing_variable() (private method)
*********************************************************************/
void PhyBoard :: phy_board_initializing_variable()
{
    object_id = FBE_OBJECT_ID_INVALID;
}

/*********************************************************************
* PhyBoard end of file
*********************************************************************/