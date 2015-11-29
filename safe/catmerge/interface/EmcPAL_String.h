/*!
 * @file EmcPAL_String.h
 * @addtogroup emcpal_misc
 * @{
 *  @version
 *   Oct 27, 2009 zubarp
 */

#ifndef _EMCPAL_STRING_H_
#define _EMCPAL_STRING_H_

//++
// Include files
//--

#include "EmcPAL.h"

#ifdef __cplusplus
extern "C" {
#endif

//++
// End Includes
//--
#ifdef EMCPAL_USE_CSX_STRINGS

typedef csx_rt_mx_nt_unicode_string_t 	EMCPAL_UNICODE_STRING;		/*!< EmcPAL Unicode string */
typedef csx_rt_mx_pnt_unicode_string_t 	EMCPAL_PUNICODE_STRING;		/*!< Ptr to EmcPAL Unicode string */
typedef csx_rt_mx_nt_ansi_string_t		EMCPAL_ANSI_STRING;			/*!< EmcPAL Ansi string */
typedef csx_rt_mx_pnt_ansi_string_t		EMCPAL_PANSI_STRING;		/*!< Ptr to EmcPAL Ansi string */

typedef csx_rt_mx_nt_string_t			EMCPAL_STRING;				/*!< EmcPAL string */
typedef csx_rt_mx_pnt_string_t			EMCPAL_PSTRING;				/*!< Ptr to EmcPAL string */

typedef	csx_wchar_t						EMCPAL_WCHAR;				/*!< EmcPAL Unicode character */
typedef	csx_pwchar_t					EMCPAL_PWCHAR;				/*!< ptr to EmcPAL Unicode character */
typedef csx_cpwchar_t					EMCPAL_PCWSTR;				/*!< Ptr to constant EmcPAL Unicode string */

typedef csx_cstring_t					EMCPAL_PCSZ;				/*!< EmcPAL constant string */

#else

typedef UNICODE_STRING					EMCPAL_UNICODE_STRING;		/*!< EmcPAL Unicode string */
typedef PUNICODE_STRING					EMCPAL_PUNICODE_STRING;		/*!< Ptr to EmcPAL Unicode string */
typedef ANSI_STRING						EMCPAL_ANSI_STRING;			/*!< EmcPAL Ansi string */
typedef PANSI_STRING					EMCPAL_PANSI_STRING;		/*!< Ptr to EmcPAL Ansi string */

typedef STRING							EMCPAL_STRING;				/*!< EmcPAL string */
typedef PSTRING							EMCPAL_PSTRING;				/*!< Ptr to EmcPAL string */

typedef	WCHAR							EMCPAL_WCHAR;				/*!< EmcPAL Unicode character */
typedef	PWCHAR							EMCPAL_PWCHAR;				/*!< ptr to EmcPAL Unicode character */
typedef	PCWSTR							EMCPAL_PCWSTR;				/*!< Ptr to constant EmcPAL Unicode string */

typedef PCSZ							EMCPAL_PCSZ;				/*!< EmcPAL constant string */
#endif

/*! @brief Initialize Unicode sting with other Unicode sting
 *  @param dest Pointer to destination sting
 *  @param source Pointer to source sting
 *  @return none
 */
CSX_LOCAL CSX_FINLINE void EmcpalInitUnicodeString(
								EMCPAL_PUNICODE_STRING dest,
									  EMCPAL_PCWSTR source)
{
#ifdef EMCPAL_USE_CSX_STRINGS
	csx_rt_mx_nt_RtlInitUnicodeString(dest,source);
	//RtlInitUnicodeString(dest, source);
#else
	RtlInitUnicodeString(dest, source);
#endif
}

CSX_LOCAL CSX_FINLINE void EmcpalCopyUnicodeString(
								EMCPAL_PUNICODE_STRING dest,
		                                            EMCPAL_PUNICODE_STRING source)
{
#ifdef EMCPAL_USE_CSX_STRINGS
	csx_rt_mx_nt_RtlCopyUnicodeString(dest,source);
	//RtlCopyUnicodeString(dest, source);
#else
	RtlCopyUnicodeString(dest, source);
#endif
}

/*! @brief Initialize a string with other string
 *  @param dest Pointer to destination sting
 *  @param source Pointer to source sting
 *  @return none
 */
CSX_LOCAL CSX_FINLINE void EmcpalInitString(
								EMCPAL_PSTRING dest,
									  EMCPAL_PCSZ source)
{
#ifdef EMCPAL_USE_CSX_STRINGS
	csx_rt_mx_nt_RtlInitString(dest,source);
	//RtlInitString(dest, source);
#else
	RtlInitString(dest, source);
#endif
}

/*! @brief Initialize a Ansi string with other string
 *  @param dest Pointer to destination sting
 *  @param source Pointer to source sting
 *  @return none
 */
CSX_LOCAL CSX_FINLINE void EmcpalInitAnsiString(
								EMCPAL_PANSI_STRING dest,
                                                                          EMCPAL_PCSZ source)
{
#ifdef EMCPAL_USE_CSX_STRINGS
	csx_rt_mx_nt_RtlInitAnsiString(dest,source);
	//RtlInitAnsiString(dest, source);
#else
	RtlInitAnsiString(dest, source);
#endif
}

/*! @brief Converts the given ANSI source string into a Unicode string
 *  @param dest Pointer to destination Unicode sting
 *  @param source Pointer to source Ansi sting
 *  @param needAllocate If True the routine should allocate the buffer space for the destination string
 *  @return STATUS_SUCCESS if success, appropriate error-code otherwise
 */
CSX_LOCAL CSX_FINLINE EMCPAL_STATUS EmcpalAnsiStringToUnicodeString(
									EMCPAL_PUNICODE_STRING dest,
									EMCPAL_PANSI_STRING source,
									BOOLEAN needAllocate)
{
#ifdef EMCPAL_USE_CSX_STRINGS
	return csx_rt_mx_nt_RtlAnsiStringToUnicodeString(dest, source, needAllocate);
	//return RtlAnsiStringToUnicodeString(dest, source, needAllocate);
#else
	return RtlAnsiStringToUnicodeString(dest, source, needAllocate);
#endif
}

/*! @brief Converts a given Unicode string into an ANSI string
 *  @param dest Pointer to destination Ansi sting
 *  @param source Pointer to source Unicode sting
 *  @param needAllocate If True the routine should allocate the buffer space for the destination string
 *  @return STATUS_SUCCESS if success, appropriate error-code otherwise
 */
CSX_LOCAL CSX_FINLINE EMCPAL_STATUS EmcpalUnicodeStringToAnsiString(
									EMCPAL_PANSI_STRING dest,
									EMCPAL_PUNICODE_STRING source,
									BOOLEAN needAllocate)
{
#ifdef EMCPAL_USE_CSX_STRINGS
	return csx_rt_mx_nt_RtlUnicodeStringToAnsiString(dest, source, needAllocate);
#else
	return RtlUnicodeStringToAnsiString(dest, source, needAllocate);
#endif
}


/*! @brief Compare two Unicode strings
 *  @param dest Pointer to destination Unicode sting
 *  @param source Pointer to source Unicode sting
 *  @param CaseInSensative If TRUE, case should be ignored when doing the comparison
 *  @return Zero: stings are equal, negative: dest is less than source, positive: dest is more than source
 */
CSX_LOCAL CSX_FINLINE LONG EmcpalCompareUnicodeString(
								EMCPAL_PUNICODE_STRING dest,
								EMCPAL_PUNICODE_STRING source,
								BOOLEAN CaseInSensative)
{
#ifdef EMCPAL_USE_CSX_STRINGS
	return csx_rt_mx_nt_RtlCompareUnicodeString(dest, source, CaseInSensative);
#else
	return RtlCompareUnicodeString(dest, source, CaseInSensative);
#endif
}

/*! @brief 
 *  @param dest Pointer to destination Unicode sting
 *  @param source Pointer to source Unicode sting
 *  @param CaseInSensative If TRUE, case should be ignored when doing the comparison
 *  @return TRUE if the two Unicode strings are equal; otherwise, it returns FALSE
 */
CSX_LOCAL CSX_FINLINE BOOLEAN EmcpalEqualUnicodeString(
									EMCPAL_PUNICODE_STRING dest,
									EMCPAL_PUNICODE_STRING source,
									BOOLEAN CaseInSensative)
{
#ifdef EMCPAL_USE_CSX_STRINGS
	return csx_rt_mx_nt_RtlEqualUnicodeString(dest, source, CaseInSensative);
#else
	return RtlEqualUnicodeString(dest, source, CaseInSensative);
#endif
}

/*! @brief Free an Unicode string memory
 *  @param dest Pointer to destination Unicode sting
 *  @return none
 */
CSX_LOCAL CSX_FINLINE void EmcpalFreeUnicodeString(EMCPAL_PUNICODE_STRING dest)
{
#ifdef EMCPAL_USE_CSX_STRINGS
	csx_rt_mx_nt_RtlFreeUnicodeString(dest);
#else
	RtlFreeUnicodeString(dest);
#endif
}

/*! @brief Free an Ansi sting memory
 *  @param dest Pointer to destination Ansi sting
 *  @return none
 */
CSX_LOCAL CSX_FINLINE void EmcpalFreeAnsiString(EMCPAL_PANSI_STRING dest)
{
#ifdef EMCPAL_USE_CSX_STRINGS
	csx_rt_mx_nt_RtlFreeAnsiString(dest);
#else
	RtlFreeAnsiString(dest);
#endif
}


#ifdef __cplusplus
}
#endif

/*!
 * @} end group emcpal_misc
 * @} end file EmcPAL_String.h
 */
#endif /* _EMCPAL_STRING_H_ */
