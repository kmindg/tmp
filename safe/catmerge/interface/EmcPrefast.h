#ifndef __EMCPREFAST_H__
#define __EMCPREFAST_H__

/*******************************************************************************
 * Copyright (C) EMC Corporation, 2005,2006
 * All rights reserved.
 * Licensed material - Property of EMC Corporation
 ******************************************************************************/

/*******************************************************************************
 * EmcPrefast.h
 *
 * This file contains compiler directives that suppress specific Prefast
 * warnings that have been approved for global suppression. These warnings
 * are informational only, or are not relavent to the ASE code base.
 *
 * Author:
 * Greg Mogavero 11/08/2005
 *
 * Revision History:
 * 11/08/2005  Created		   									            GRM 
 * 01/19/2006  Added #382,changed ifdef                             Chetan Loke 
 ******************************************************************************/

/* We disable the 4068 warning so that compiler does not generate unknown pragma warnings 
 * for #pragma prefast.
 */
#pragma warning(disable: 4068)

/* Suppress Prefast Warning 382:Shutdown resource ID <variable> used.
 * This warning indicates that a resource ID has been used for a shutdown message. 
 * The resource ID contains the string 'reboot' or 'restartwindows.'
 * Microsoft has replied that the Prefast heuristics engine may be looking for macros/strings that 
 * would cause the system to reboot and raise this warning.
 */

#pragma prefast(disable: 382)

/* Suppress Prefast warning 8101: The Drivers module has inferred that the current function is a <Function 
 * Name Type> function: This is informational only. No problem has been detected.
 */
#pragma prefast(disable: 8101)

/* Suppress Prefast warning 8126: The AccessMode parameter to ObReferenceObject* should be IRP->RequestorMode..
 * I've asked Microsoft about this one. They agree that this Prefast warning is not consistent with the DDK
 * documentation on this issue. The DDK is correct and they've filed a bug on the Prefast documentation for
 * this warning. It's only appropriate for top level drivers to specify IRP->RequestorMode. We're using this 
 * function correctly in our lower level drivers when we set the AccessMode parameter to a constant
 * value of KernelMode.
 */
#pragma prefast(disable: 8126)


/* We re-enable the 4068 warning so that compiler will now generate unknown pragma warnings
 */
#pragma warning(default: 4068)


#endif
