#if !defined(PROCESSOR_COUNT_H)
#define	PROCESSOR_COUNT_H 0x00000001

/***********************************************************************
 *  Copyright (C)  EMC Corporation 1989-2014
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 ***********************************************************************/

/***************************************************************************
 * processorCount.h
 ***************************************************************************
 *
 * DESCRIPTION:
 *      All this provides is the maximum number of processors our code
 *      supports, in a single tiny, clean, header file. This is so it can be
 *      used everywhere and we dont need the same number in the code 3000 times
 *
 * NOTES:
 *      Some modules still require more in depth changes to support more
 *      processors, they include:
 *
 *       safe\catmerge\interface\CmiUpperInterface.h:
 *        Cmi_SEP_IO_Conduit_CoreXX - and all uses thereof (other files)
 *        Cmi_SPCacheReferenceSenderConduitCoreXX - and all uses thereof (other files)
 *        CMI_MAX_CONDUITS - will probably need to increase
 *
 *       safe\catmerge\disk\fbe\interface\fbe_cmi.h:
 *        FBE_CMI_CONDUIT_ID_SEP_IO_COREXX - and all uses thereof (other files)
 *        FBE_CMI_CONDUIT_ID_SEP_IO_LAST - should be updated
 *
 *       safe\catmerge\layered\MLU\krnl\Dart\server\src\kernel\min_kernel\include\sade_os_misc.h
 *         (which is also sade\src\dart\Dart\server\src\kernel\min_kernel\include\sade_os_misc.h):
 *        MP_MAX_NBR_CPUS
 *
 *       safe\catmerge\layered\MLU\krnl\Dart\server\src\d2c\cbfs\include\m_percpu.hxx
 *         (which is also sade\src\dart\Dart\server\src\d2c\cbfs\include\m_percpu.hxx):
 *        MP_MAX_NBR_CPUS (AGAIN!!!)
 *
 *       safe\catmerge\layered\MLU\krnl\Dart\server\src\lib\smallpool.cxx
 *         (which is also sade\src\dart\Dart\server\src\lib\smallpool.cxx):
 *        NCPUS - there are asserts for this, but it actually comes from MP_MAX_NBR_CPUS
 *        gMagPoolLockXX - and all uses thereof (should only be this file)
 *
 * HISTORY:
 *      12-Aug-2014     Created.       -Joseph Ash
 *
 ***************************************************************************/

#define MAX_CORES 64

#endif //PROCESSOR_COUNT_H
