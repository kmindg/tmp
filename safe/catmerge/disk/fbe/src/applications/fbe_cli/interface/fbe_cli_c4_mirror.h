#ifndef FBE_CLI_C4_MIRROR_H
#define FBE_CLI_C4_MIRROR_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2015
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_c4_mirror.h
 ***************************************************************************
 *
 * @brief
 *  This file contains c4mirror command cli definitions.
 *
 * @version
 *  06/29/2015 - Created. Jamin Kang
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

void fbe_cli_c4_mirror(int argc, char **argv);

#define MIRROR_CMD_USAGE "\n\
mirror -help|h\n\
mirror -rebuild_status\n\
mirror -update_pvd -rg_obj_id rg_obj_id -edge_index 0/1 -sn new_pvd_sn \n\
mirror -get_pvd_map -rg_obj_id rg_obj_id  \n\
mirror -mark_clean \n\
\n\
    -rebuild_status       Get mirror raid groups rebuild status\n\
    \n\
    -update_pvd           Try to modify the PVD SN of this RG \n\
        -rg_obj_id obj_id The RG object_id which you want to change its PVD SN \n\
        -edge_index       Which edge of this RG you want to change \n\
        -sn a2134ws       The new SN of this edge \n\
    \n\
    -get_pvd_map          List all the drive SNs of this RG \n\
        -rg_obj_id rg_obj_id  Tell us the rg_obj_id \n\
    \n\
    -mark_clean           Mark mirror lun clean \n\
    \n\
    NOTE: Be careful to use -update_pvd commands. It is aimed to test the code or fix the corrupt data \n\
\n"

/*************************
 * end file fbe_cli_c4_mirror.h
 *************************/


#endif
