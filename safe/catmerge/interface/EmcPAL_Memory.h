 /*!
 * @file EmcPAL_Memory.h
 * @addtogroup emcpal_memory_utils
 * @{
 * @version 
 *      zubarp Oct 27, 2009
 */

#ifndef _EMCPAL_MEMORY_H_
#define _EMCPAL_MEMORY_H_

#include "EmcPAL.h"

#ifdef __cplusplus
extern "C" {
#endif

//++
// End Includes
//--

/*! @brief Moves memory either forward or backward, aligned or unaligned, in 4-byte blocks, followed by any remaining bytes
 *  @param dest Pointer to destination memory
 *  @param source Pointer to source memory
 *  @param length Length of moved area
 *  @return none
 */
#ifdef I_AM_DEBUG_EXTENSION

#include <string.h>

CSX_LOCAL CSX_FINLINE void EmcpalMoveMemory(void *dest, const void *source, SIZE_T length)
{
        memmove(dest, source, length);
}

CSX_LOCAL CSX_FINLINE void EmcpalCopyMemory(void *dest, const void *source, SIZE_T length)
{
        memcpy(dest, source, length);
}

CSX_LOCAL CSX_FINLINE void EmcpalZeroMemory(void *dest, SIZE_T length)
{
        memset(dest, 0, length);
}

CSX_LOCAL CSX_FINLINE void EmcpalFillMemory(void *dest, SIZE_T length, UCHAR fill)
{
        memset(dest, fill, length);
}

#else
CSX_LOCAL CSX_FINLINE void EmcpalMoveMemory(void *dest, const void *source, SIZE_T length)
{
#ifdef EMCPAL_USE_CSX_MEMORY
	csx_p_memmove(dest, source, length);
#else
	RtlMoveMemory(dest,source,length);
#endif
}

/*! @brief Copy the contents of one buffer to another
 *  @param dest Pointer to destination memory
 *  @param source Pointer to source memory
 *  @param length Length of copied area
 *  @return none
 */
CSX_LOCAL CSX_FINLINE void EmcpalCopyMemory(void *dest, const void *source, SIZE_T length)
{
#ifdef EMCPAL_USE_CSX_MEMORY
	csx_p_memcpy(dest, source, length);
#else
	RtlCopyMemory(dest,source,length);
#endif
}

/*! @brief Routine compares blocks of memory and returns the number of bytes that are equivalent
 *  @param source1 Pointer to the first memory bloc to compare
 *  @param source2 Pointer to the second memory bloc to compare
 *  @param length Length of compared areas
 *  @return Number of bytes that are equivalent
 */
CSX_LOCAL CSX_FINLINE SIZE_T EmcpalCompareMemory(const void *source1, const void *source2, SIZE_T length)
{
#ifdef EMCPAL_USE_CSX_MEMORY
	return csx_p_memmatch(source1, source2, length);
#else
	return RtlCompareMemory(source1, source2, length);
#endif
}

/*! @brief Zero the memory area
 *  @param dest Pointer to target memory
 *  @param length Length of target memory
 *  @return none
 */
CSX_LOCAL CSX_FINLINE void EmcpalZeroMemory(void *dest, SIZE_T length)
{
#ifdef EMCPAL_USE_CSX_MEMORY
	csx_p_memzero(dest, length);
#else
	RtlZeroMemory(dest, length);
#endif
}

/*! @brief Fill a memory with the supplied character
 *  @param dest Pointer to target memory
 *  @param length Length of target memory area
 *  @param fill Value/character to fill the memory
 *  @return none
 */
CSX_LOCAL CSX_FINLINE void EmcpalFillMemory(void *dest, SIZE_T length, UCHAR fill)
{
#ifdef EMCPAL_USE_CSX_MEMORY
	csx_p_memset(dest, fill, length);
#else
	RtlFillMemory(dest, length, fill);
#endif
}

#endif // I_AM_DEBUG_EXTENSION

CSX_LOCAL CSX_FINLINE void EmcpalCopyCharsMemory(char *dest, char *src, SIZE_T length)
{
    EmcpalCopyMemory(dest, src, length);
}

/*! @brief Routine compares two blocks of memory to determine whether the specified number of bytes are identical
 *  @param src1 Pointer to the first memory bloc
 *  @param src2 Pointer to the second memory bloc
 *  @param length Length of compared areas
 *  @return TRUE if memory areas are equivalent; otherwise, it returns FALSE
 */
CSX_LOCAL CSX_FINLINE unsigned int EmcpalEqualMemory(const void * src1, const void * src2, SIZE_T length)
{
#ifdef EMCPAL_USE_CSX_MEMORY
	return csx_p_memcmp(src1, src2, length)==0;
#else
	return RtlEqualMemory(src1, src2, length);
#endif
}

/*! @brief Zero a variable */
#define EmcpalZeroVariable(Variable) EmcpalZeroMemory(&Variable,sizeof(Variable))

/*! @brief Empty value */
#if (!defined(_MSC_VER)) && defined(__cplusplus)
  // This syntax compatible wit ANSI C99 standard
  #define EmcpalEmpty {}
#else
  // MSVS 2005 compiler does not support ANSI C99 standard, so for C89 standard
  // we should insert minimum one initializer between braces
  #define EmcpalEmpty {0}
#endif
#ifdef __cplusplus
}
#endif

/*!
 * @} end group emcpal_memory_utils
 * @} end file EmcPAL_Memory.h
 */

#endif /* _EMCPAL_MEMORY_H_ */
