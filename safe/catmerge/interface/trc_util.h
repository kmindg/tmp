/* copyright */

/*
 * trc_util.h
 *
 * DESCRIPTION: This header file contains definitions of preprocessor
 *   based macro functions that implement the TRC facilities.
 */
#if !defined(TRC_UTIL_H)
#define TRC_UTIL_H

/*
 * CONDITIONALS
 *
 * DO_BIST       Controls whether printing is done by various functions
 *               and system services or by a test function
 *   !defined -- generate normally 
 *    defined -- value of DO_BIST is substituted for output functions
 *
 * TRC_COMP      Must be defined. Its definition is the name of the component
 *               that is invoking this header file.
 *
 * TRC_UTIL_H    Guard symbol that limits this header file to one inclusion.
 *   !defined -- include the body of this file
 *    defined -- skip the body of this file
 *
 * UMODE_ENV     Controls whether a user- or kernel-mode version is generated
 *   !defined -- generate kernel-mode version
 *    defined -- generate   user-mode version
 */

/*
 * ERROR CHECKING
 */
#if !defined(TRC_COMP)
#error TRC_COMP must be defined prior to including trc_util.h
#endif

/*
 * INCLUDES
 */
#if !defined(_K_TRACE_H_)
#include "ktrace.h"             /* all ktrace components */
#endif

/*
 * FUNCTION PROTOTYPES
 */
#if (defined(UMODE_ENV) || defined(SIMMODE_ENV)) && !defined(DO_BIST)
void TRC_OutputDebugString_(const void* buffer);
#endif


/*
 * INTERNAL MACRO FUNCTION DEFINITIONS
 */
/*
 * TRC_CAT3(m1, m2, m3)
 *
 * DESCRIPTION: This macro concatinates its three arguments
 *  into a single string / symbol
 *
 * PARAMETERS:
 *  m1, m2, m3  -- the values if these 3 strings are concatinated
 *
 * GLOBALS: none.
 *
 * CALLS: nothing.
 *
 * RETURN VALUES:
 *  the generated string
 *
 */
#define TRC_CAT3(m1, m2, m3) m1 ## m2 ## m3

/*
 * TRC_UNPREN1 (m_arg)
 *
 * DESCRIPTION: This macro strips off the parentheses from its
 *  argument and returns the bare argument.
 *
 * PARAMETERS:
 *  m_arg
 *
 * GLOBALS: none.
 *
 * CALLS: nothing.
 *
 * RETURN VALUE:
 *  m_arg with no surrounding parentheses
 */
#define TRC_UNPREN1 (m_arg) m_arg

/*
 * TRC_VALUE (m_arg)
 *
 * DESCRIPTION: This macro stringifies its argument
 *
 * PARAMETER:
 *
 * GLOBALS:
 *
 * CALLS:
 *
 * RETURNS:
 *
 */
#define TRC_VALUE_(m_arg) #m_arg
#define TRC_VALUE(m_arg) TRC_VALUE_(m_arg)
/*
 * TRC_XCAT3(m1, m2, m3)
 *
 * DESCRIPTION: This macro passes the values of its three
 *  arguments to TRC_CAT3() which concatinates the values into
 *  a single string.
 *
 * PARAMETERS:
 *  m1, m2, m3  -- the values of these 3 strings are passed to TRC_CAT3()
 *
 * GLOBALS: none.
 *
 * CALLS: TRC_CAT3()
 *
 * RETURN VALUES:
 *  the result of TRC_CAT3()
 *
 */
#define TRC_XCAT3(m1, m2, m3) TRC_CAT3(m1, m2, m3)

/*
 * MACRO SYMBOL DEFINITIONS
 */
/*
 * TRC_local_FLAG
 *
 * DESCRIPTION: This symbol is set to the trace compilation 
 *  flag symbol for the component that invoked this header
 *  file.
 *
 *
 * EXAMPLE: if this file was invoked with TRC_COMP set to JFW
 *  TRC_local_FLAG would have the value TRC_JFW_FLAG
 *
 */
#define TRC__FLAG 1
#define TRC_local_FLAG TRC_XCAT3 (TRC_, TRC_COMP, _FLAG)

#if TRC_local_FLAG == 1
#error TRC_COMP must be defined with a string value, empty definition found.
#endif

#undef TRC__FLAG

/*
 * TRC_local_flag
 *
 * DESCRIPTION: This symbol is set to the trace active flag name
 *  for the component that invoked this header file.
 *
 * EXAMPLE: if this file was invoked with TRC_COMP set to JFW
 *  TRC_local_flag would have the value TRC_JFW_flag
 *
 */
#define TRC_local_flag TRC_XCAT3 (TRC_, TRC_COMP, _flag)

/*
 * EXTERNAL MACRO FUNCTION DEFINITIONS
 */
/*
 * TRC_arg          (m_name, m_spec)
 * TRCA_arg         (m_name, m_spec)
 * TRCAX_arg (m_buf, m_name, m_spec)
 * TRCR_arg         (m_name, m_spec)
 * TRCRX_arg (m_buf, m_name, m_spec)
 *
 * DESCRIPTION: trace an argument in namelist form.
 *
 *  for TRC_arg():
 *  if TRC_ARGS is cleared in TRC_local_FLAG then
 *    a dummy definition is generated.
 *  if TRC_ARGS is set in TRC_local_FLAG then
 *    a definition is generated which is a TRC_printf() 
 *    enclosed in an "if (TRC_args & TRC_local_flag).
 *
 *  for TRCR_arg():
 *  if TRCR_ARGS is cleared in TRC_local_FLAG then
 *    a dummy definition is generated.
 *  if TRCR_ARGS is set in TRC_local_FLAG then
 *    a definition is generated which is a KTRACE() 
 *    enclosed in an "if (TRCR_args & TRC_local_flag).
 *
 *  for TRCA_arg():
 *  if TRC_ARGS or TRCR_ARGS is set in TRC_local_FLAG then
 *    TRC_arg() and TRCR_arg() are generated inside a do{}while(0) loop
 *  If neither TRC_ARGS nor TRCR_ARGS are set then
 *    a dummy definition is generated. 
 *
 * PARAMETERS:
 *  m_name -- Argument's name.
 *  m_spec -- printf format for argument.
 *
 * GLOBALS:
 *
 * RETURN VALUES: treat as void
 *
 * GENERATED: nothing, one or both of the following
 *  TRC_printf (("<TRC_COMP>:    arg:<m_name>=<m_spec>\n", <m_name>))
 *  KTRACE      ("<TRC_COMP>:    arg:<m_name>=<m_spec>",   <m_name>, DUMMY_VALUE ...)
 */
#if TRC_local_FLAG & TRC_ARGS
#define TRC_arg(m_name, m_spec)				\
do {							\
  if (TRC_local_flag & TRC_args)			\
  {							\
    TRC_printf ((TRC_VALUE(TRC_COMP) ":    arg:"        \
                 #m_name "="  m_spec "\n", m_name));	\
  };							\
} while (0)
#else /* TRC_local_FLAG & TRC_ARGS */
#define TRC_arg(m_dummy, m_dummy1)
#endif /* TRC_local_FLAG & TRC_ARGS */

#if TRC_local_FLAG & TRCR_ARGS
#define TRCR_arg(m_name, m_spec)					\
do {									\
  if (TRC_local_flag & TRCR_args)					\
  {									\
    KTRACE (TRC_VALUE(TRC_COMP) ":    arg:" #m_name "="  m_spec,	\
            m_name,							\
            DUMMY_VALUE,						\
            DUMMY_VALUE,						\
            DUMMY_VALUE);						\
  };									\
} while (0)
#else
#define TRCR_arg(m_dummy, m_dummy1)
#endif 

#if TRC_local_FLAG & TRCR_ARGS
#define TRCRX_arg(m_buf, m_name, m_spec)				 \
do {									 \
  if (TRC_local_flag & TRCR_args)					 \
  {									 \
    KTRACEX (m_buf, TRC_VALUE(TRC_COMP) ":    arg:" #m_name "="  m_spec, \
            m_name,							 \
            DUMMY_VALUE,						 \
            DUMMY_VALUE,						 \
            DUMMY_VALUE);						 \
  };									 \
} while (0)
#else /* TRC_local_FLAG & TRCR_ARGS */
#define TRCR_arg(m_dummy, m_dummy1)
#define TRCRX_arg(m_dummy0, m_dummy, m_dummy1)
#endif /* TRC_local_FLAG & TRC_ARGS */

#if (TRC_local_FLAG & (TRC_ARGS|TRCR_ARGS))
#define TRCA_arg(m_name, m_spec)                \
do {                                            \
  TRC_arg  (m_name, m_spec);                    \
  TRCR_arg (m_name, m_spec);                    \
} while (0)

#define TRCAX_arg(m_buf, m_name, m_spec)	\
do {						\
  TRC_arg          (m_name, m_spec);		\
  TRCRX_arg (m_buf, m_name, m_spec);		\
} while (0)
#else /* (TRC_local_FLAG & (TRC_ARGS|TRCR_ARGS)) */
#define TRCA_arg(m_dummy, m_dummy1)
#define TRCAX_arg(m_dummy0, m_dummy, m_dummy1)
#endif /* (TRC_local_FLAG & (TRC_ARGS|TRCR_ARGS)) */

/*
 * TRC_do_return()
 * TRCA_do_return()
 * TRCAX_do_return()
 * TRCR_do_return()
 * TRCRX_do_return()
 *
 * DESCRIPTION: receive control from all TRC_return() macros in the
 *  current function and trace the function name and exit value.
 *
 *  for TRC_do_return():
 *  if TRC_EXIT is cleared in TRC_local_FLAG then
 *    a dummy definition is generated.
 *  if TRC_EXIT is set in TRC_local_FLAG then
 *    a definition is generated which is a TRC_printf() 
 *    enclosed in an "if (TRC_exit & TRC_local_flag)".
 *    this is followed by "return (TRC_retval)"
 *
 *  for TRCR_do_return():
 *  if TRCR_EXIT is cleared in TRC_local_FLAG then
 *    a dummy definition is generated.
 *  if TRCR_EXIT is set in TRC_local_FLAG then
 *    a definition is generated which is a KTRACE() 
 *    enclosed in an "if (TRCR_exit & TRC_local_flag)".
 *    this is followed by "return (TRC_retval)"
 *
 *  for TRCA_do_return():
 *  If neither TRC_EXIT nor TRCR_EXIT are set then
 *    a dummy definition is generated. 
 *  if TRC_EXIT or TRCR_EXIT is set in TRC_local_FLAG then
 *    a definition is generated which is a TRC_printf() 
 *    enclosed in an "if (TRC_exit & TRC_local_flag)".
 *    and a definition is generated which is a KTRACE() 
 *    enclosed in an "if (TRCR_exit & TRC_local_flag)".
 *    this is followed by "return (TRC_retval)"
 *    
 * NOTE: one of TRC_do_return(), TRCA_do_return(), or TRCR_do_return()
 *  must be used precisely once in a function that contains
 *  one or more TRC_return() macros.
 *  Precisely one TRC_return_init() be present in the function.
 *
 * PARAMETERS:
 *  m_func -- name of the current function
 *  m_spec -- printf format specifier for the return value
 *
 * GLOBALS:
 *  TRC_retval -- variable setup by TRC_return_init(), set by
 *                TRC_return()
 *  TRC_ret:   -- label for this block of generated code, transferred
 *                to by TRC_return().
 *
 * RETURN VALUES: treat as void
 *
 * GENERATES: nothing, one, or both of the following:
 *  TRC_printf (("<TRC_COMP>:exitted:<m_func> returns <m_spec>\n", TRC_retval))
 *  KTRACE      ("<TRC_COMP>:exitted:<m_func> returns <m_spec>",   TRC_retval,
 *               DUMMY_VALUE, DUMMY_VALUE, DUMMY_VALUE)
 */

#if (TRC_local_FLAG & (TRC_EXIT|TRCR_EXIT)) == 0
#define TRC_do_return(m_dummy, m_dummy1)
#define TRCR_do_return(m_dummy, m_dummy1)
#define TRCRX_do_return(m_dummy0, m_dummy, m_dummy1)
#define TRCA_do_return(m_dummy, m_dummy1)
#define TRCAX_do_return(m_dummy0, m_dummy, m_dummy1)
#endif  /* (TRC_local_FLAG & (TRC_EXIT|TRCR_EXIT)) == 0 */


#if (TRC_local_FLAG & (TRC_EXIT|TRCR_EXIT)) == TRC_EXIT
#define TRC_do_return(m_func, m_spec)					\
 TRC_ret:								\
  if (TRC_local_flag & TRC_exit)					\
  {									\
    TRC_printf ((TRC_VALUE(TRC_COMP) ":exitted:" m_func " returns "	\
                 m_spec "\n", TRC_retval));				\
  };									\
  return (TRC_retval)

#define TRCR_do_return(m_dummy, m_dummy1)	\
 TRC_ret:					\
  return (TRC_retval)

#define TRCRX_do_return(m_dummy0, m_dummy1, m_dummy2)	\
 TRC_ret:						\
  return (TRC_retval)

#define TRCA_do_return(m_func, m_spec)		\
   TRC_do_return(m_func, m_spec)

#define TRCAX_do_return(m_dummy0, m_func, m_spec)	\
   TRC_do_return(m_func, m_spec)

#endif /* (TRC_local_FLAG & (TRC_EXIT|TRCR_EXIT) == TRC_EXIT */

#if (TRC_local_FLAG & (TRC_EXIT|TRCR_EXIT)) == TRCR_EXIT
#define TRC_do_return(m_dummy, m_dummy1)	\
 TRC_ret:					\
  return (TRC_retval)

#define TRCR_do_return(m_func, m_spec)					\
 TRC_ret:								\
  if (TRC_local_flag & TRCR_exit)					\
  {									\
    KTRACE (TRC_VALUE(TRC_COMP) ":exitted:" m_func " returns "  m_spec,	\
             TRC_retval, DUMMY_VALUE, DUMMY_VALUE, DUMMY_VALUE);	\
  };									\
  return (TRC_retval)

#define TRCRX_do_return(m_buf, m_func, m_spec)				\
 TRC_ret:								\
  if (TRC_local_flag & TRCR_exit)					\
  {									\
    KTRACEX (m_buf,							\
	     TRC_VALUE(TRC_COMP) ":exitted:" m_func " returns " m_spec,	\
             TRC_retval, DUMMY_VALUE, DUMMY_VALUE, DUMMY_VALUE);	\
  };									\
  return (TRC_retval)

#define TRCA_do_return(m_func, m_spec)		\
   TRCR_do_return(m_func, m_spec)

#define TRCAX_do_return(m_buf, m_func, m_spec)	\
   TRCRX_do_return(m_buf, m_func, m_spec)

#endif /* (TRC_local_FLAG & (TRC_EXIT|TRCR_EXIT)) == TRCR_EXIT */

#if (TRC_local_FLAG & (TRC_EXIT|TRCR_EXIT)) == (TRC_EXIT|TRCR_EXIT)
#define TRC_do_return(m_func, m_spec)					\
 TRC_ret:								\
  if (TRC_local_flag & TRC_exit)					\
  {									\
    TRC_printf ((TRC_VALUE(TRC_COMP) ":exitted:" m_func " returns "	\
                 m_spec "\n", TRC_retval));				\
  };									\
  return (TRC_retval)

#define TRCR_do_return(m_func, m_spec)					\
 TRC_ret:								\
  if (TRC_local_flag & TRCR_exit)					\
  {									\
    KTRACE (TRC_VALUE(TRC_COMP) ":exitted:" m_func " returns "  m_spec,	\
             TRC_retval, DUMMY_VALUE, DUMMY_VALUE, DUMMY_VALUE);	\
  };									\
  return (TRC_retval)

#define TRCRX_do_return(m_buf, m_func, m_spec)				 \
 TRC_ret:								 \
  if (TRC_local_flag & TRCR_exit)					 \
  {									 \
    KTRACEX (m_buf,							 \
	     TRC_VALUE(TRC_COMP) ":exitted:" m_func " returns "  m_spec, \
             TRC_retval, DUMMY_VALUE, DUMMY_VALUE, DUMMY_VALUE);	 \
  };									 \
  return (TRC_retval)

#define TRCA_do_return(m_func, m_spec)					\
 TRC_ret:								\
do {									\
  if (TRC_local_flag & TRC_exit)					\
  {									\
    TRC_printf ((TRC_VALUE(TRC_COMP) ":exitted:" m_func " returns "	\
                 m_spec "\n", TRC_retval));				\
  };									\
  if (TRC_local_flag & TRCR_exit)					\
  {									\
    KTRACE (TRC_VALUE(TRC_COMP) ":exitted:" m_func " returns "  m_spec,	\
             TRC_retval, DUMMY_VALUE, DUMMY_VALUE, DUMMY_VALUE);	\
  };									\
  return (TRC_retval);							\
} while (0)

#define TRCAX_do_return(m_buf, m_func, m_spec)				 \
 TRC_ret:								 \
do {									 \
  if (TRC_local_flag & TRC_exit)					 \
  {									 \
    TRC_printf ((TRC_VALUE(TRC_COMP) ":exitted:" m_func " returns "	 \
                 m_spec "\n", TRC_retval));				 \
  };									 \
  if (TRC_local_flag & TRCR_exit)					 \
  {									 \
    KTRACEX (m_buf,							 \
	     TRC_VALUE(TRC_COMP) ":exitted:" m_func " returns "  m_spec, \
             TRC_retval, DUMMY_VALUE, DUMMY_VALUE, DUMMY_VALUE);	 \
  };									 \
  return (TRC_retval);							 \
} while (0)
#endif /* (TRC_local_FLAG & (TRC_EXIT | TRCR_EXIT)) == TRC_EXIT|TRCR_EXIT */

/*
 * TRC_do_return_void()
 * TRCA_do_return_void()
 * TRCR_do_return_void()
 *
 * DESCRIPTION: receive control from all TRC_return_void() macros in the
 *  current function and trace the function name.
 *
 *  for TRC_do_return_void():
 *  if TRC_EXIT is cleared in TRC_local_FLAG then
 *    a dummy definition is generated.
 *  if TRC_EXIT is set in TRC_local_FLAG then
 *    a definition is generated which is a TRC_printf() 
 *    enclosed in an "if (TRC_exit & TRC_local_flag)".
 *    this is followed by "return"
 *
 *  for TRCR_do_return():
 *  if TRCR_EXIT is cleared in TRC_local_FLAG then
 *    a dummy definition is generated.
 *  if TRCR_EXIT is set in TRC_local_FLAG then
 *    a definition is generated which is a KTRACE() 
 *    enclosed in an "if (TRCR_exit & TRC_local_flag)".
 *    this is followed by "return"
 *
 *  for TRCA_do_return():
 *  If neither TRC_EXIT nor TRCR_EXIT are set then
 *    a dummy definition is generated. 
 *  if TRC_EXIT or TRCR_EXIT is set in TRC_local_FLAG then
 *    a definition is generated which is a TRC_printf() 
 *    enclosed in an "if (TRC_exit & TRC_local_flag)".
 *    and a definition is generated which is a KTRACE() 
 *    enclosed in an "if (TRCR_exit & TRC_local_flag)".
 *    this is followed by "return"
 *    
 * NOTE: one of TRC_do_return_void(), TRCA_do_return_void(), 
 *  or TRCR_do_return_void() must be used precisely once in a function
 *  that contains one or more TRC_return() macros.
 *
 * PARAMETERS:
 *  m_func -- name of the current function
 *
 * GLOBALS:
 *  TRC_ret_void: -- label for this block of generated code, transferred
 *                   to by TRC_return().
 *
 * RETURN VALUES: treat as void
 *
 * GENERATES: nothing, one, or both of tehfollowing:
 *  TRC_print  ("<TRC_COMP>:exitted:<m_func>\n")
 *  KTRACE     ("<TRC_COMP>:exitted:<m_func>\n",
 *              DUMMY_VALUE, DUMMY_VALUE, DUMMY_VALUE, DUMMY_VALUE)
 *
 */

/*
 * neither flag is on
 */
#if (TRC_local_FLAG & (TRC_EXIT|TRCR_EXIT)) == 0
#define TRC_do_return_void(m_dummy)	\
    return
#define TRCA_do_return_void(m_dummy)	\
    return
#define TRCAX_do_return_void(m_dummy0, m_dummy)	\
    return
#define TRCR_do_return_void(m_dummy)	\
    return
#define TRCRX_do_return_void(m_dummy0, m_dummy)	\
    return
#endif /* (TRC_local_FLAG & (TRC_EXIT|TRCR_EXIT))  == 0*/

/*
 * TRC_EXIT flag only
 */
#if (TRC_local_FLAG & (TRC_EXIT|TRCR_EXIT)) == TRC_EXIT
#define TRC_do_return_void(m_func)				\
 TRC_ret_void:							\
  if (TRC_local_flag & TRC_exit)				\
  {								\
    TRC_print (TRC_VALUE(TRC_COMP) ":exitted:" m_func "\n");	\
  };								\
  return;

#define TRCR_do_return_void(m_dummy)		\
 TRC_ret_void:					\
  return;

#define TRCRX_do_return_void(m_dummy0, m_dummy1)	\
 TRC_ret_void:						\
  return;

#define TRCA_do_return_void(m_func)		\
  TRC_do_return_void(m_func)

#define TRCAX_do_return_void(m_dummy, m_func)	\
  TRC_do_return_void(m_func)

#endif /* TRC_local_FLAG & TRC_EXIT */

/*
 * TRCR_EXIT flag only
 */
#if (TRC_local_FLAG & (TRC_EXIT|TRCR_EXIT)) == TRCR_EXIT
#define TRC_do_return_void(m_func)		\
 TRC_ret_void:					\
  return;

#define TRCR_do_return_void(m_func)					\
 TRC_ret_void:								\
  if (TRC_local_flag & TRCR_exit)					\
  {									\
    KTRACE (TRC_VALUE(TRC_COMP) ":exitted:" m_func "\n",		\
            DUMMY_VALUE, DUMMY_VALUE, DUMMY_VALUE, DUMMY_VALUE);	\
  };									\
  return;

#define TRCRX_do_return_void(m_buf, m_func)				\
 TRC_ret_void:								\
  if (TRC_local_flag & TRCR_exit)					\
  {									\
    KTRACEX (m_buf, TRC_VALUE(TRC_COMP) ":exitted:" m_func "\n",	\
            DUMMY_VALUE, DUMMY_VALUE, DUMMY_VALUE, DUMMY_VALUE);	\
  };									\
  return;

#define TRCA_do_return_void(m_func)					\
  TRCR_do_return_void(m_func)

#define TRCAX_do_return_void(m_buf, m_func)				\
  TRCRX_do_return_void(m_buf, m_func)

#endif /* TRC_local_FLAG & TRCR_EXIT */

/*
 *  both flags are on
 */
#if (TRC_local_FLAG & (TRC_EXIT|TRCR_EXIT)) == (TRC_EXIT|TRCR_EXIT)

#define TRC_do_return_void(m_func)				\
 TRC_ret_void:							\
  if (TRC_local_flag & TRC_exit)				\
  {								\
    TRC_print (TRC_VALUE(TRC_COMP) ":exitted:" m_func "\n");	\
  };								\
  return;

#define TRCR_do_return_void(m_func)					\
 TRC_ret_void:								\
  if (TRC_local_flag & TRCR_exit)					\
  {									\
    KTRACE (TRC_VALUE(TRC_COMP) ":exitted:" m_func "\n",		\
            DUMMY_VALUE, DUMMY_VALUE, DUMMY_VALUE, DUMMY_VALUE);	\
  };									\
  return;

#define TRCRX_do_return_void(m_buf, m_func)				\
 TRC_ret_void:								\
  if (TRC_local_flag & TRCR_exit)					\
  {									\
    KTRACEX (m_buf, TRC_VALUE(TRC_COMP) ":exitted:" m_func "\n",	\
            DUMMY_VALUE, DUMMY_VALUE, DUMMY_VALUE, DUMMY_VALUE);	\
  };									\
  return;

#define TRCA_do_return_void(m_func)					\
 goto TRC_ret_void;                     \
 TRC_ret_void:								\
do {									\
  if (TRC_local_flag & TRC_exit)					\
  {									\
    TRC_printf ((TRC_VALUE(TRC_COMP) ":exitted:" m_func "\n"));		\
  };									\
  if (TRC_local_flag & TRCR_exit)					\
  {									\
    KTRACE (TRC_VALUE(TRC_COMP) ":exitted:" m_func "\n",		\
             DUMMY_VALUE, DUMMY_VALUE, DUMMY_VALUE, DUMMY_VALUE);	\
  };									\
  return;								\
} while (0)

#define TRCAX_do_return_void(m_buf, m_func)				\
 TRC_ret_void:								\
do {									\
  if (TRC_local_flag & TRC_exit)					\
  {									\
    TRC_printf ((TRC_VALUE(TRC_COMP) ":exitted:" m_func "\n"));		\
  };									\
  if (TRC_local_flag & TRCR_exit)					\
  {									\
    KTRACEX (m_buf, TRC_VALUE(TRC_COMP) ":exitted:" m_func "\n",	\
             DUMMY_VALUE, DUMMY_VALUE, DUMMY_VALUE, DUMMY_VALUE);	\
  };									\
  return;								\
} while (0)
#endif /* (TRC_local_FLAG & (TRC_EXIT|TRCR_EXIT)) == (TRC_EXIT|TRCR_EXIT) */

/*
 * TRC_func()
 * TRCA_func()
 * TRCR_func()
 *
 * DESCRIPTION: trace the entry to a function.
 *
 *  for TRC_func():
 *  if TRC_ENTRY is cleared in TRC_local_FLAG then
 *    a dummy definition is generated.
 *  if TRC_ENTRY is set in TRC_local_FLAG then
 *    a definition is generated which is a TRC_printf() 
 *    enclosed in an "if (TRC_entry & TRC_local_flag).
 *
 *  for TRCR_func():
 *  if TRCR_ENTRY is cleared in TRC_local_FLAG then
 *    a dummy definition is generated.
 *  if TRCR_ENTRY is set in TRC_local_FLAG then
 *    a definition is generated which is a KTRACE() 
 *    enclosed in an "if (TRCR_entry & TRC_local_flag).
 *
 *  for TRCA_func():
 *  if TRC_ENTRY or TRCR_ENTRY is set in TRC_local_FLAG then
 *    TRC_func() and TRCR_func() are generated inside a do{}while(0) loop
 *  If neither TRC_ENTRY nor TRCR_ENTRY are set then
 *    a dummy definition is generated. 
 *
 *
 * PARAMETERS:
 *  m_func  -- name of the current function
 *
 * GLOBALS: none
 *
 * RETURN VALUES: treat as void
 *
 * GENERATES: nothing, one, or both of the following:
 *  TRC_printf ((TRC_VALUE(TRC_COMP) ":entered:<m_func>\n"))
 *  KTRACE      (TRC_VALUE(TRC_COMP) ":entered:<m_func>\n",
 *              DUMMY_VALUE, DUMMY_VALUE, DUMMY_VALUE, DUMMY_VALUE);
 */

#if TRC_local_FLAG & TRC_ENTRY
#define TRC_func(m_func)					\
do {								\
  if (TRC_local_flag & TRC_entry)				\
  {								\
    TRC_print (TRC_VALUE(TRC_COMP) ":entered:" m_func "\n");	\
  };								\
} while (0)
#else /* TRC_local_FLAG & TRCR_ENTRY */
#define TRC_func(m_dummy)
#endif /* TRC_local_FLAG & TRCR_ENTRY */

#if TRC_local_FLAG & TRCR_ENTRY
#define TRCR_func(m_func)						\
do {									\
  if (TRC_local_flag & TRCR_entry)					\
  {									\
    KTRACE (TRC_VALUE(TRC_COMP) ":entered:" m_func "\n",		\
            DUMMY_VALUE, DUMMY_VALUE, DUMMY_VALUE, DUMMY_VALUE);	\
  };									\
} while (0)

#define TRCRX_func(m_buf, m_func)					\
do {									\
  if (TRC_local_flag & TRCR_entry)					\
  {									\
    KTRACEX (m_buf, TRC_VALUE(TRC_COMP) ":entered:" m_func "\n",	\
            DUMMY_VALUE, DUMMY_VALUE, DUMMY_VALUE, DUMMY_VALUE);	\
  };									\
} while (0)
#else /* TRC_local_FLAG & TRCR_ENTRY */
#define TRCR_func(m_dummy)
#define TRCRX_func(m_dummy0, m_dummy)
#endif /* TRC_local_FLAG & TRCR_ENTRY */

#if TRC_local_FLAG & (TRC_ENTRY|TRCR_ENTRY)
#define TRCA_func(m_func)                       \
 do {                                           \
 TRC_func  (m_func);                            \
 TRCR_func (m_func);                            \
 } while(0)

#define TRCAX_func(m_buf, m_func)		\
 do {						\
 TRC_func  (m_func);				\
 TRCRX_func (m_buf, m_func);			\
 } while(0)
#else /* TRC_local_FLAG & (TRC_ENTRY|TRCR_ENTRY) */
#define TRCA_func(m_dummy)
#define TRCAX_func(m_dummy0, m_dummy)
#endif /* TRC_local_FLAG & (TRC_ENTRY|TRCR_ENTRY) */
 
/*
 * TRC_func1()
 * TRCA_func1()
 * TRCR_func1()
 *
 * DESCRIPTION: trace the entry to a function and trace the values
 *  of 1 parameter.
 *
 *  for TRC_func():
 *  if TRC_ENTRY is cleared in TRC_local_FLAG then
 *    a dummy definition is generated.
 *  if TRC_ENTRY is set in TRC_local_FLAG then
 *    a definition is generated which is a TRC_printf() 
 *    enclosed in an "if (TRC_entry & TRC_local_flag).
 *
 *  for TRCR_func():
 *  if TRCR_ENTRY is cleared in TRC_local_FLAG then
 *    a dummy definition is generated.
 *  if TRCR_ENTRY is set in TRC_local_FLAG then
 *    a definition is generated which is a KTRACE() 
 *    enclosed in an "if (TRCR_ENTRY & TRC_local_flag).
 *
 *  for TRCA_func():
 *  if TRC_ENTRY or TRCR_ENTRY is set in TRC_local_FLAG then
 *    TRC_func() and TRCR_func() are generated inside a do{}while(0) loop
 *  If neither TRC_ENTRY nor TRCR_ENTRY are set then
 *    a dummy definition is generated. 
 *
 *
 * PARAMETERS:
 *  m_func  -- name of the current function
 *  m_arg1  -- first argument
 *  m_spec1 -- first argument format
 *
 * GLOBALS: none
 *
 * RETURN VALUES: treat as void
 *
 * GENERATES: nothing, one, or both of the following:
 *  TRC_printf ((TRC_VALUE(TRC_COMP) ":entered:<m_func>; <m_arg1>=<m_spec1>\n")
 *  KTRACE      (TRC_VALUE(TRC_COMP) ":entered:<m_func>; <m_arg1>=<m_spec1>")
 */

#if TRC_local_FLAG & TRC_ENTRY
#define TRC_func1(m_func, m_arg1, m_spec1)		\
do {							\
  if (TRC_local_flag & TRC_entry)			\
  {							\
    TRC_printf ((TRC_VALUE(TRC_COMP) ":entered:" 	\
                 m_func "; " #m_arg1 "="		\
                 m_spec1 "\n",  m_arg1));		\
  };							\
} while (0)
#else /* TRC_local_FLAG & TRC_ENTRY */
#define TRC_func1(m_dummy, m_dummy1, m_dummy2)
#endif /* TRC_local_FLAG & TRC_ENTRY */

#if TRC_local_FLAG & TRCR_ENTRY
#define TRCR_func1(m_func, m_arg1, m_spec1)			\
do {								\
  if (TRC_local_flag & TRCR_entry)				\
  {								\
    KTRACE (TRC_VALUE(TRC_COMP) ":entered:" m_func "; " 	\
           #m_arg1 "=" m_spec1,					\
            m_arg1, DUMMY_VALUE, DUMMY_VALUE, DUMMY_VALUE);	\
  };								\
} while (0)

#define TRCRX_func1(m_buf, m_func, m_arg1, m_spec1)			\
do {									\
  if (TRC_local_flag & TRCR_entry)					\
  {									\
    KTRACEX (m_buf, TRC_VALUE(TRC_COMP) ":entered:"			\
             m_func "; " #m_arg1 "="					\
	     m_spec1,  m_arg1, DUMMY_VALUE, DUMMY_VALUE, DUMMY_VALUE);	\
  };									\
} while (0)
#else /* TRC_local_FLAG & TRCR_ENTRY */
#define TRCR_func1(m_dummy, m_dummy1, m_dummy2)
#define TRCRX_func1(m_dummy0, m_dummy, m_dummy1, m_dummy2)
#endif /* TRC_local_FLAG & TRCR_ENTRY */

#if TRC_local_FLAG & (TRC_ENTRY|TRCR_ENTRY)
#define TRCA_func1(m_func, m_arg1, m_spec1)     \
 do {                                           \
 TRC_func1  (m_func, m_arg1, m_spec1);          \
 TRCR_func1 (m_func, m_arg1, m_spec1);          \
 } while(0)
#else /* TRC_local_FLAG & (TRC_ENTRY|TRCR_ENTRY) */
#define TRCA_func1(m_dummy, m_dummy1, m_dummy2)
#endif /* TRC_local_FLAG & (TRC_ENTRY|TRCR_ENTRY) */
#if TRC_local_FLAG & (TRC_ENTRY|TRCR_ENTRY)
#define TRCA_func1(m_func, m_arg1, m_spec1)     \
 do {                                           \
 TRC_func1  (m_func, m_arg1, m_spec1);          \
 TRCR_func1 (m_func, m_arg1, m_spec1);          \
 } while(0)

#define TRCAX_func1(m_buf, m_func, m_arg1, m_spec1)	\
 do {							\
 TRC_func1          (m_func, m_arg1, m_spec1);		\
 TRCRX_func1 (m_buf, m_func, m_arg1, m_spec1);		\
 } while(0)
#else /* TRC_local_FLAG & (TRC_ENTRY|TRCR_ENTRY) */
#define TRCA_func1(m_dummy, m_dummy1, m_dummy2)
#define TRCAX_func1(m_dummy0, m_dummy, m_dummy1, m_dummy2)
#endif /* TRC_local_FLAG & (TRC_ENTRY|TRCR_ENTRY) */

/*
 * TRC_func2()
 * TRCA_func2()
 * TRCR_func2()
 *
 * DESCRIPTION: trace the entry to a function and trace the values
 *  of 2 parameters.
 *
 *  for TRC_func():
 *  if TRC_ENTRY is cleared in TRC_local_FLAG then
 *    a dummy definition is generated.
 *  if TRC_ENTRY is set in TRC_local_FLAG then
 *    a definition is generated which is a TRC_printf() 
 *    enclosed in an "if (TRC_entry & TRC_local_flag).
 *
 *  for TRCR_func():
 *  if TRCR_ENTRY is cleared in TRC_local_FLAG then
 *    a dummy definition is generated.
 *  if TRCR_ENTRY is set in TRC_local_FLAG then
 *    a definition is generated which is a KTRACE() 
 *    enclosed in an "if (TRCR_ENTRY & TRC_local_flag).
 *
 *  for TRCA_func():
 *  if TRC_ENTRY or TRCR_ENTRY is set in TRC_local_FLAG then
 *    TRC_func() and TRCR_func() are generated inside a do{}while(0) loop
 *  If neither TRC_ENTRY nor TRCR_ENTRY are set then
 *    a dummy definition is generated. 
 *
 *
 * PARAMETERS:
 *  m_func  -- name of the current function
 *  m_arg1  -- first argument
 *  m_spec1 -- first argument format
 *  m_arg2  -- second argument
 *  m_spec2 -- second argument format
 *
 * GLOBALS: none
 *
 * RETURN VALUES: treat as void
 *
 * GENERATES: nothing, one, or both of the following:
 *  TRC_printf ((TRC_VALUE(TRC_COMP) ":entered:<m_func>; <m_arg1>=<m_spec1>, "
 *                                     "<m_arg2>=<m_spec2>\n"))
 *  KTRACE      (TRC_VALUE(TRC_COMP) ":entered:<m_func>; <m_arg1>=<m_spec1>, "
 *                                     "<m_arg2>=<m_spec2>")
 */

#if TRC_local_FLAG & TRC_ENTRY
#define TRC_func2(m_func, m_arg1, m_spec1, m_arg2, m_spec2)	\
do {								\
  if (TRC_local_flag & TRC_entry)				\
  {								\
    TRC_printf ((TRC_VALUE(TRC_COMP)				\
		 ":entered:" m_func "; " #m_arg1 "=" m_spec1	\
		 ", " #m_arg2 "=" m_spec2			\
		 "\n",						\
		 m_arg1, m_arg2));				\
  };								\
} while (0)
#else /* TRC_local_FLAG & TRC_ENTRY */
#define TRC_func2(m_dummy, m_dummy1, m_dummy2,  \
                           m_dummy3, m_dummy4)
#endif /* TRC_local_FLAG & TRC_ENTRY */

#if TRC_local_FLAG & TRCR_ENTRY
#define TRCR_func2(m_func, m_arg1, m_spec1, m_arg2, m_spec2)	\
do {								\
  if (TRC_local_flag & TRCR_entry)				\
  {								\
    KTRACE (TRC_VALUE(TRC_COMP) ":entered:" m_func 		\
                                  "; " #m_arg1 "=" m_spec1	\
                                  ", " #m_arg2 "=" m_spec2,	\
            m_arg1, m_arg2, DUMMY_VALUE, DUMMY_VALUE);		\
  };								\
} while (0)

#define TRCRX_func2(m_buf, m_func, m_arg1, m_spec1, m_arg2, m_spec2)	\
do {									\
  if (TRC_local_flag & TRCR_entry)					\
  {									\
    KTRACEX (m_buf, TRC_VALUE(TRC_COMP) ":entered:"			\
             m_func "; " #m_arg1 "="					\
	     m_spec1  ", " #m_arg2 "=" m_spec2,				\
             m_arg1, m_arg2, DUMMY_VALUE, DUMMY_VALUE);			\
  };									\
} while (0)
#else /* TRC_local_FLAG & TRCR_ENTRY */
#define TRCR_func2(m_dummy, m_dummy1, m_dummy2,  \
                            m_dummy3, m_dummy4)
#define TRCRX_func2(m_dummy0, m_dummy, m_dummy1, m_dummy2,	\
                            m_dummy3, m_dummy4)
#endif /* TRC_local_FLAG & TRCR_ENTRY */

#if TRC_local_FLAG & (TRC_ENTRY|TRCR_ENTRY)
#define TRCA_func2(m_func, m_arg1, m_spec1,     \
                           m_arg2, m_spec2)     \
 do {                                           \
 TRC_func2  (m_func, m_arg1, m_spec1,           \
                     m_arg2, m_spec2);          \
 TRCR_func2 (m_func, m_arg1, m_spec1,           \
                     m_arg2, m_spec2);          \
 } while(0)

#define TRCAX_func2(m_buf, m_func, m_arg1, m_spec1,	\
                           m_arg2, m_spec2)		\
 do {							\
 TRC_func2          (m_func, m_arg1, m_spec1,		\
                     m_arg2, m_spec2);			\
 TRCRX_func2 (m_buf, m_func, m_arg1, m_spec1,		\
                     m_arg2, m_spec2);			\
 } while(0)
#else /* TRC_local_FLAG & (TRC_ENTRY|TRCR_ENTRY) */
#define TRCA_func2(m_dummy, m_dummy1, m_dummy2, m_dummy3, m_dummy4)
#define TRCAX_func2(m_dummy0, m_dummy, m_dummy1, m_dummy2, m_dummy3, m_dummy4)
#endif /* TRC_local_FLAG & (TRC_ENTRY|TRCR_ENTRY) */

/*
 * TRC_func3()
 * TRCA_func3()
 * TRCR_func3()
 *
 * DESCRIPTION: trace the entry to a function and trace the values
 *  of 3 parameters.
 *
 *  for TRC_func():
 *  if TRC_ENTRY is cleared in TRC_local_FLAG then
 *    a dummy definition is generated.
 *  if TRC_ENTRY is set in TRC_local_FLAG then
 *    a definition is generated which is a TRC_printf() 
 *    enclosed in an "if (TRC_entry & TRC_local_flag).
 *
 *  for TRCR_func():
 *  if TRCR_ENTRY is cleared in TRC_local_FLAG then
 *    a dummy definition is generated.
 *  if TRCR_ENTRY is set in TRC_local_FLAG then
 *    a definition is generated which is a KTRACE() 
 *    enclosed in an "if (TRCR_ENTRY & TRC_local_flag).
 *
 *  for TRCA_func():
 *  if TRC_ENTRY or TRCR_ENTRY is set in TRC_local_FLAG then
 *    TRC_func() and TRCR_func() are generated inside a do{}while(0) loop
 *  If neither TRC_ENTRY nor TRCR_ENTRY are set then
 *    a dummy definition is generated. 
 *
 *
 * PARAMETERS:
 *  m_func  -- name of the current function
 *  m_arg1  -- first argument
 *  m_spec1 -- first argument format
 *  m_arg2  -- second argument
 *  m_spec2 -- second argument format
 *  m_arg3  -- third argument
 *  m_spec3 -- third argument format
 *
 * GLOBALS: none
 *
 * RETURN VALUES: treat as void
 *
 * GENERATES: nothing, one, or both of the following:
 *  TRC_printf ((TRC_VALUE(TRC_COMP) ":entered:<m_func>; <m_arg1>=<m_spec1>, "
 *                                     "<m_arg2>=<m_spec2>, "
 *                                     "<m_arg3>=<m_spec3>\n"))
 *  KTRACE      (TRC_VALUE(TRC_COMP) ":entered:<m_func>; <m_arg1>=<m_spec1>, "
 *                                     "<m_arg2>=<m_spec2>, "
 *                                     "<m_arg3>=<m_spec3>")
 */

#if TRC_local_FLAG & TRC_ENTRY
#define TRC_func3(m_func, m_arg1, m_spec1,				\
                          m_arg2, m_spec2,				\
                          m_arg3, m_spec3)				\
do {									\
  if (TRC_local_flag & TRC_entry)					\
  {									\
    TRC_printf ((TRC_VALUE(TRC_COMP) ":entered:" m_func			\
                                       "; " #m_arg1 "=" m_spec1		\
                                       ", " #m_arg2 "=" m_spec2		\
                                       ", " #m_arg3 "=" m_spec3		\
                                       "\n",				\
                                       m_arg1, m_arg2, m_arg3));	\
  };									\
} while (0)
#else /* TRC_local_FLAG & TRC_ENTRY */
#define TRC_func3(m_dummy, m_dummy1, m_dummy2, \
                           m_dummy3, m_dummy4, \
                           m_dummy5, m_dummy6)
#endif /* TRC_local_FLAG & TRC_ENTRY */

#if TRC_local_FLAG & TRCR_ENTRY
#define TRCR_func3(m_func, m_arg1, m_spec1,			\
                           m_arg2, m_spec2,			\
                           m_arg3, m_spec3)			\
do {								\
  if (TRC_local_flag & TRCR_entry)				\
  {								\
    KTRACE (TRC_VALUE(TRC_COMP) ":entered:" m_func		\
                                  "; " #m_arg1 "=" m_spec1	\
                                  ", " #m_arg2 "=" m_spec2	\
                                  ", " #m_arg3 "=" m_spec3,	\
            m_arg1, m_arg2, m_arg3, DUMMY_VALUE);		\
  };								\
} while (0)

#define TRCRX_func3(m_buf, m_func, m_arg1, m_spec1,		\
                                   m_arg2, m_spec2,		\
                                   m_arg3, m_spec3)		\
do {								\
  if (TRC_local_flag & TRCR_entry)				\
  {								\
    KTRACEX (m_buf, TRC_VALUE(TRC_COMP) ":entered:" m_func	\
                                  "; " #m_arg1 "=" m_spec1	\
                                  ", " #m_arg2 "=" m_spec2	\
                                  ", " #m_arg3 "=" m_spec3,	\
            m_arg1, m_arg2, m_arg3, DUMMY_VALUE);		\
  };								\
} while (0)
#else /* TRC_local_FLAG & TRCR_ENTRY */
#define TRCR_func3(m_dummy, m_dummy1, m_dummy2, \
                            m_dummy3, m_dummy4, \
                            m_dummy5, m_dummy6)
#define TRCRX_func3(m_dummy0, m_dummy,		\
                    m_dummy1, m_dummy2,		\
                    m_dummy3, m_dummy4,		\
                    m_dummy5, m_dummy6)
#endif /* TRC_local_FLAG & TRCR_ENTRY */

#if TRC_local_FLAG & (TRC_ENTRY|TRCR_ENTRY)
#define TRCA_func3(m_func, m_arg1, m_spec1,     \
                           m_arg2, m_spec2,     \
                           m_arg3, m_spec3)     \
 do {                                           \
 TRC_func3  (m_func, m_arg1, m_spec1,           \
                     m_arg2, m_spec2,           \
                     m_arg3, m_spec3);          \
 TRCR_func3 (m_func, m_arg1, m_spec1,           \
                     m_arg2, m_spec2,           \
                     m_arg3, m_spec3);          \
 } while(0)

#define TRCAX_func3(m_buf, m_func, m_arg1, m_spec1,	\
                           m_arg2, m_spec2,		\
                           m_arg3, m_spec3)		\
 do {							\
 TRC_func3  (m_func, m_arg1, m_spec1,			\
                     m_arg2, m_spec2,			\
                     m_arg3, m_spec3);			\
 TRCRX_func3 (m_buf, m_func, m_arg1, m_spec1,		\
                     m_arg2, m_spec2,			\
                     m_arg3, m_spec3);			\
 } while(0)
#else /* TRC_local_FLAG & (TRC_ENTRY|TRCR_ENTRY) */
#define TRCA_func3(m_dummy, m_dummy1, m_dummy2,			\
                            m_dummy3, m_dummy4,			\
                            m_dymmy5, m_dummy6)
#define TRCAX_func3(m_dummy0, m_dummy, m_dummy1, m_dummy2,	\
                            m_dummy3, m_dummy4,			\
                            m_dymmy5, m_dummy6)
#endif /* TRC_local_FLAG & (TRC_ENTRY|TRCR_ENTRY) */
 
/*
 * TRC_func4()
 * TRCA_func4()
 * TRCR_func4()
 *
 * DESCRIPTION: trace the entry to a function and trace the values
 *  of 4 parameters.
 *
 *  for TRC_func():
 *  if TRC_ENTRY is cleared in TRC_local_FLAG then
 *    a dummy definition is generated.
 *  if TRC_ENTRY is set in TRC_local_FLAG then
 *    a definition is generated which is a TRC_printf() 
 *    enclosed in an "if (TRC_entry & TRC_local_flag).
 *
 *  for TRCR_func():
 *  if TRCR_ENTRY is cleared in TRC_local_FLAG then
 *    a dummy definition is generated.
 *  if TRCR_ENTRY is set in TRC_local_FLAG then
 *    a definition is generated which is a KTRACE() 
 *    enclosed in an "if (TRCR_ENTRY & TRC_local_flag).
 *
 *  for TRCA_func():
 *  if TRC_ENTRY or TRCR_ENTRY is set in TRC_local_FLAG then
 *    TRC_func() and TRCR_func() are generated inside a do{}while(0) loop
 *  If neither TRC_ENTRY nor TRCR_ENTRY are set then
 *    a dummy definition is generated. 
 *
 *
 * PARAMETERS:
 *  m_func  -- name of the current function
 *  m_arg1  -- first argument
 *  m_spec1 -- first argument format
 *  m_arg2  -- second argument
 *  m_spec2 -- second argument format
 *  m_arg3  -- third argument
 *  m_spec3 -- third argument format
 *  m_arg4  -- fourth argument
 *  m_spec4 -- fourth argument format
 *
 * GLOBALS: none
 *
 * RETURN VALUES: treat as void
 *
 * GENERATES: nothing, one, or both of the following:
 *  TRC_printf ((TRC_VALUE(TRC_COMP) ":entered:<m_func>; <m_arg1>=<m_spec1>, "
 *                                     "<m_arg2>=<m_spec2>, "
 *                                     "<m_arg3>=<m_spec3>, "
 *                                     "<m_arg4>=<m_spec4>\n"))
 *  KTRACE      (TRC_VALUE(TRC_COMP) ":entered:<m_func>; <m_arg1>=<m_spec1>, "
 *                                     "<m_arg2>=<m_spec2>, "
 *                                     "<m_arg3>=<m_spec3>, "
 *                                     "<m_arg4>=<m_spec4>")
 */

#define foobar int
#if TRC_local_FLAG & TRC_ENTRY
#define TRC_func4(m_func, m_arg1, m_spec1,			\
                          m_arg2, m_spec2,			\
                          m_arg3, m_spec3,			\
                          m_arg4, m_spec4)			\
do {								\
  if (TRC_local_flag & TRC_entry)				\
  {								\
    TRC_printf ((TRC_VALUE(TRC_COMP) ":entered:" m_func		\
                                       "; " #m_arg1 "=" m_spec1	\
                                       ", " #m_arg2 "=" m_spec2	\
                                       ", " #m_arg3 "=" m_spec3	\
                                       ", " #m_arg4 "=" m_spec4	\
                                       "\n",			\
                 m_arg1, m_arg2, m_arg3, m_arg4));		\
  };								\
} while (0)
#else /* TRC_local_FLAG & TRC_ENTRY */
#define TRC_func4(m_dummy, m_dummy1, m_dummy2,	\
                           m_dummy3, m_dummy4,	\
                           m_dummy5, m_dummy6,	\
                           m_dummy7, m_dummy8)
#endif /* TRC_local_FLAG & TRC_ENTRY */

#if TRC_local_FLAG & TRCR_ENTRY
#define TRCR_func4(m_func, m_arg1, m_spec1,			\
                          m_arg2, m_spec2,			\
                          m_arg3, m_spec3,			\
                          m_arg4, m_spec4)			\
do {								\
  if (TRC_local_flag & TRCR_entry)				\
  {								\
    KTRACE (TRC_VALUE(TRC_COMP) ":entered:" m_func		\
                                  "; " #m_arg1 "=" m_spec1	\
                                  ", " #m_arg2 "=" m_spec2	\
                                  ", " #m_arg3 "=" m_spec3	\
                                  ", " #m_arg4 "=" m_spec4,	\
            m_arg1, m_arg2, m_arg3, m_arg4);			\
  };								\
} while (0)

#define TRCRX_func4(m_buf, m_func, m_arg1, m_spec1,		\
                          m_arg2, m_spec2,			\
                          m_arg3, m_spec3,			\
                          m_arg4, m_spec4)			\
do {								\
  if (TRC_local_flag & TRCR_entry)				\
  {								\
    KTRACEX (m_buf, TRC_VALUE(TRC_COMP) ":entered:" m_func	\
	                          "; " #m_arg1 "=" m_spec1	\
                                  ", " #m_arg2 "=" m_spec2	\
                                  ", " #m_arg3 "=" m_spec3	\
                                  ", " #m_arg4 "=" m_spec4,	\
            m_arg1, m_arg2, m_arg3, m_arg4);			\
  };								\
} while (0)
#else /* TRC_local_FLAG & TRCR_ENTRY */
#define TRCR_func4(m_dummy, m_dummy1, m_dummy2, \
                            m_dummy3, m_dummy4, \
                            m_dummy5, m_dummy6, \
                            m_dummy7, m_dummy8)
#define TRCRX_func4(m_dummy0, m_dummy, m_dummy1, m_dummy2,	\
                            m_dummy3, m_dummy4,			\
                            m_dummy5, m_dummy6,			\
                            m_dummy7, m_dummy8)
#endif /* TRC_local_FLAG & TRCR_ENTRY */

#if TRC_local_FLAG & (TRC_ENTRY|TRCR_ENTRY)
#define TRCA_func4(m_func, m_arg1, m_spec1,     \
                           m_arg2, m_spec2,     \
                           m_arg3, m_spec3,     \
                           m_arg4, m_spec4)     \
 do {                                           \
 TRC_func4  (m_func, m_arg1, m_spec1,           \
                     m_arg2, m_spec2,           \
                     m_arg3, m_spec3,           \
                     m_arg4, m_spec4);          \
 TRCR_func4 (m_func, m_arg1, m_spec1,           \
                     m_arg2, m_spec2,           \
                     m_arg3, m_spec3,           \
                     m_arg4, m_spec4);          \
 } while(0)

#define TRCAX_func4(m_buf, m_func, m_arg1, m_spec1,	\
                           m_arg2, m_spec2,		\
                           m_arg3, m_spec3,		\
                           m_arg4, m_spec4)		\
 do {							\
 TRC_func4  (m_func, m_arg1, m_spec1,			\
                     m_arg2, m_spec2,			\
                     m_arg3, m_spec3,			\
                     m_arg4, m_spec4);			\
 TRCRX_func4 (m_buf, m_func, m_arg1, m_spec1,		\
                     m_arg2, m_spec2,			\
                     m_arg3, m_spec3,			\
                     m_arg4, m_spec4);			\
 } while(0)
#else /* TRC_local_FLAG & (TRC_ENTRY|TRCR_ENTRY) */
#define TRCA_func4(m_dummy, m_dummy1, m_dummy2,	\
                            m_dummy3, m_dummy4,	\
                            m_dymmy5, m_dummy6,	\
                            m_dummy7, m_dummy8)
#define TRCAX_func4(m_dummy0, m_dummy, m_dummy1, m_dummy2,	\
                            m_dummy3, m_dummy4,			\
                            m_dymmy5, m_dummy6,			\
                            m_dummy7, m_dummy8)
#endif /* TRC_local_FLAG & (TRC_ENTRY|TRCR_ENTRY) */

/*
 * TRC_if_arg ()
 * TRCA_if_arg ()
 * TRCR_if_arg ()
 */
#if TRC_local_FLAG & TRC_ARGS
#define TRC_if_arg(m_stuff)			\
do {						\
if (TRC_local_flag & TRC_args)			\
{						\
  m_stuff					\
};						\
} while (0)
#else
#define TRC_if_arg(m_dummy)
#endif

#if TRC_local_FLAG & TRCR_ARGS
#define TRCR_if_arg(m_stuff)			\
do {						\
if (TRC_local_flag & TRCR_args)			\
{						\
  m_stuff					\
};						\
} while (0)
#else
#define TRCR_if_arg(m_dummy)
#endif

#if TRC_local_FLAG & (TRC_ARGS|TRCR_ARGS)
#define TRCA_if_arg(m_stuff)			\
do {						\
if (TRC_local_flag & (TRC_args|TRCR_args))	\
{						\
  m_stuff					\
};						\
} while (0)
#else
#define TRCA_if_arg(m_dummy)
#endif

/*
 * TRC_if_args ()
 * TRCA_if_args ()
 * TRCR_if_args ()
 */
#if TRC_local_FLAG & TRC_ARGS
#define TRC_if_args(m_stuff)			\
do {						\
if (TRC_local_flag & TRC_args)			\
{						\
  m_stuff					\
};						\
} while (0)
#else
#define TRC_if_args(m_dummy)
#endif

#if TRC_local_FLAG & TRCR_ARGS
#define TRCR_if_args(m_stuff)			\
do {						\
if (TRC_local_flag & TRCR_args)			\
{						\
  m_stuff					\
};						\
} while (0)
#else
#define TRCR_if_args(m_dummy)
#endif

#if TRC_local_FLAG & (TRC_ARGS|TRCR_ARGS)
#define TRCA_if_args(m_stuff)			\
do {						\
if (TRC_local_flag & (TRC_args|TRCR_args))	\
{						\
  m_stuff					\
};						\
} while (0)
#else
#define TRCA_if_args(m_dummy)
#endif

/*
 * TRC_if_entry ()
 * TRCA_if_entry ()
 * TRCR_if_entry ()
 */
#if TRC_local_FLAG & TRC_ENTRY
#define TRC_if_entry(m_stuff)			\
do {						\
if (TRC_local_flag & TRC_entry)			\
{						\
  m_stuff					\
};						\
} while (0)
#else
#define TRC_if_entry(m_dummy)
#endif

#if TRC_local_FLAG & TRCR_ENTRY
#define TRCR_if_entry(m_stuff)			\
do {						\
if (TRC_local_flag & TRCR_entry)		\
{						\
  m_stuff					\
};						\
} while (0)
#else
#define TRCR_if_entry(m_dummy)
#endif

#if TRC_local_FLAG & (TRC_ENTRY|TRCR_ENTRY)
#define TRCA_if_entry(m_stuff)			\
do {						\
if (TRC_local_flag & (TRC_entry|TRCR_entry))	\
{						\
  m_stuff					\
};						\
} while (0)
#else
#define TRCA_if_entry(m_dummy)
#endif

/*
 * TRC_if_envs ()
 */
#if TRC_local_FLAG & TRC_ENVS
#define TRC_if_envs(m_stuff)			\
do {						\
if (TRC_local_flag & TRC_envs)			\
{						\
  m_stuff					\
};						\
} while (0)
#else
#define TRC_if_envs(m_dummy)
#endif

/*
 * TRC_if_error ()
 * TRCA_if_error ()
 * TRCR_if_error ()
*/
#if TRC_local_FLAG & TRC_LEVEL_ERROR
#define TRC_if_error(m_stuff)			\
do {						\
if (TRC_local_flag & TRC_level_error)		\
{						\
  m_stuff					\
};						\
} while (0)
#else
#define TRC_if_error(m_dummy)
#endif

#if TRC_local_FLAG & TRCR_LEVEL_ERROR
#define TRCR_if_error(m_stuff)			\
do {						\
if (TRC_local_flag & TRCR_level_error)		\
{						\
  m_stuff					\
};						\
} while (0)
#else
#define TRCR_if_error(m_dummy)
#endif

#if TRC_local_FLAG & (TRC_LEVEL_ERROR|TRCR_LEVEL_ERROR)
#define TRCA_if_error(m_stuff)					\
do {								\
if (TRC_local_flag & (TRC_level_error|TRCR_level_error))	\
{								\
  m_stuff							\
};								\
} while (0)
#else
#define TRCA_if_error(m_dummy)
#endif

/*
 * TRC_if_exit ()
 * TRCA_if_exit ()
 * TRCR_if_exit ()
 */
#if TRC_local_FLA & TRC_EXIT
#define TRC_if_exit(m_stuff)			\
do {						\
if (TRC_local_flag & TRC_exit)			\
{						\
  m_stuff					\
};						\
} while (0)
#else
#define TRC_if_exit(m_dummy)
#endif

#if TRC_local_FLAG & TRCR_EXIT
#define TRCR_if_exit(m_stuff)			\
do {						\
if (TRC_local_flag & TRCR_exit)			\
{						\
  m_stuff					\
};						\
} while (0)
#else
#define TRCR_if_exit(m_dummy)
#endif

#if TRC_local_FLAG & (TRC_EXIT|TRCR_EXIT)
#define TRCA_if_exit(m_stuff)			\
do {						\
if (TRC_local_flag & (TRC_exit|TRCR_exit))	\
{						\
  m_stuff					\
};						\
} while (0)
#else
#define TRCA_if_exit(m_dummy)
#endif

/*
 * TRC_if_fatal ()
 * TRCA_if_fatal ()
 * TRCR_if_fatal ()
 */
#if TRC_local_FLAG & TRC_LEVEL_FATAL
#define TRC_if_fatal(m_stuff)			\
do {						\
if (TRC_local_flag & TRC_level_fatal)		\
{						\
  m_stuff					\
};						\
} while (0)
#else
#define TRC_if_fatal(m_dummy)
#endif

#if TRC_local_FLAG & TRCR_LEVEL_FATAL
#define TRCR_if_fatal(m_stuff)			\
do {						\
if (TRC_local_flag & TRCR_level_fatal)		\
{						\
  m_stuff					\
};						\
} while (0)
#else
#define TRCR_if_fatal(m_dummy)
#endif

#if TRC_local_FLAG & (TRC_LEVEL_FATAL|TRCR_LEVEL_FATAL)
#define TRCA_if_fatal(m_stuff)					\
do {								\
if (TRC_local_flag & (TRC_level_fatal|TRCR_level_fatal))	\
{								\
  m_stuff							\
};								\
} while (0)
#else
#define TRCA_if_fatal(m_dummy)
#endif

/*
 * TRC_if_info ()
 * TRCA_if_info ()
 * TRCR_if_info ()
 */
#if TRC_local_FLAG & TRC_LEVEL_INFO
#define TRC_if_info(m_stuff)			\
do {						\
if (TRC_local_flag & TRC_level_info)		\
{						\
  m_stuff					\
};						\
} while (0)
#else
#define TRC_if_info(m_dummy)
#endif

#if TRC_local_FLAG & TRCR_LEVEL_INFO
#define TRCR_if_info(m_stuff)			\
do {						\
if (TRC_local_flag & TRCR_level_info)		\
{						\
  m_stuff					\
};						\
} while (0)
#else
#define TRCR_if_info(m_dummy)
#endif

#if TRC_local_FLAG & (TRC_LEVEL_INFO|TRCR_LEVEL_INFO)
#define TRCA_if_info(m_stuff)				\
do {							\
if (TRC_local_flag & (TRC_level_info|TRCR_level_info))	\
{							\
  m_stuff						\
};							\
} while (0)
#else
#define TRCA_if_info(m_dummy)
#endif

/*
 * TRC_if_span_frus ()
 * TRCA_if_span_frus ()
 * TRCR_if_span_frus ()
 */
#if TRC_local_FLAG & TRC_SPAN_FRUS
#define TRC_if_span_frus(m_stuff)		\
do {						\
if (TRC_local_flag & TRC_span_frus)		\
{						\
  m_stuff					\
};						\
} while (0)
#else
#define TRC_if_span_frus(m_dummy)
#endif

#if TRC_local_FLAG & TRCR_SPAN_FRUS
#define TRCR_if_span_frus(m_stuff)		\
do {						\
if (TRC_local_flag & TRCR_span_frus)		\
{						\
  m_stuff					\
};						\
} while (0)
#else
#define TRCR_if_span_frus(m_dummy)
#endif

#if TRC_local_FLAG & (TRC_SPAN_FRUS|TRCR_SPAN_FRUS)
#define TRCA_if_span_frus(m_stuff) \
do {							\
if (TRC_local_flag & (TRC_span_frus|TRCR_span_frus))	\
{							\
  m_stuff						\
};							\
} while (0)
#else
#define TRCA_if_span_frus(m_dummy)
#endif

/*
 * TRC_if_state ()
 * TRCA_if_state ()
 * TRCR_if_state ()
 */
#if TRC_local_FLAG & TRC_STATE
#define TRC_if_state(m_stuff)			\
do {						\
if (TRC_local_flag & TRC_state)			\
{						\
  m_stuff					\
};						\
} while (0)
#else
#define TRC_if_state(m_dummy)
#endif

#if TRC_local_FLAG & TRCR_STATE
#define TRCR_if_state(m_stuff)			\
do {						\
if (TRC_local_flag & TRCR_state)		\
{						\
  m_stuff					\
};						\
} while (0)
#else
#define TRCR_if_state(m_dummy)
#endif

#if TRC_local_FLAG & (TRC_STATE|TRCR_STATE)
#define TRCA_if_state(m_stuff)			\
do {						\
if (TRC_local_flag & (TRC_state|TRCR_state))	\
{						\
  m_stuff					\
};						\
} while (0)
#else
#define TRCA_if_state(m_dummy)
#endif

/*
 * TRC_if_state2 ()
 * TRCA_if_state2 ()
 * TRCR_if_state2 ()
 */
#if TRC_local_FLAG & TRC_STATE2
#define TRC_if_state2(m_stuff)			\
do {						\
if (TRC_local_flag & TRC_state2)		\
{						\
  m_stuff					\
};						\
} while (0)
#else
#define TRC_if_state2(m_dummy)
#endif

#if TRC_local_FLAG & TRCR_STATE2
#define TRCR_if_state2(m_stuff)			\
do {						\
if (TRC_local_flag & TRCR_state2)		\
{						\
  m_stuff					\
};						\
} while (0)
#else
#define TRCR_if_state2(m_dummy)
#endif

#if TRC_local_FLAG & (TRC_STATE2|TRCR_STATE2)
#define TRCA_if_state2(m_stuff)			\
do {						\
if (TRC_local_flag & (TRC_state2|TRCR_state2))	\
{						\
  m_stuff					\
};						\
} while (0)
#else
#define TRCA_if_state2(m_dummy)
#endif

/*
 * TRC_if_state4 ()
 * TRCA_if_state4 ()
 * TRCR_if_state4 ()
 */
#if TTRC_local_FLAG & TRC_STATE4
#define TRC_if_state4(m_stuff)			\
do {						\
if (TRC_local_flag & TRC_state4)		\
{						\
  m_stuff					\
};						\
} while (0)
#else
#define TRC_if_state4(m_dummy)
#endif

#if TRC_local_FLAG & TRCR_STATE4
#define TRCR_if_state4(m_stuff)			\
do {						\
if (TRC_local_flag & TRCR_state4)		\
{						\
  m_stuff					\
};						\
} while (0)
#else
#define TRCR_if_state4(m_dummy)
#endif

#if TRC_local_FLAG & (TRC_STATE4|TRCR_STATE4)
#define TRCA_if_state4(m_stuff)			\
do {						\
if (TRC_local_flag & (TRC_state4|TRCR_state4))	\
{						\
  m_stuff					\
};						\
} while (0)
#else
#define TRCA_if_state4(m_dummy)
#endif

/*
 * TRC_if_state8 ()
 * TRCA_if_state8 ()
 * TRCR_if_state8 ()
 */
#if TRC_local_FLAG & TRC_STATE8
#define TRC_if_state8(m_stuff)			\
do {						\
if (TRC_local_flag & TRC_state8)		\
{						\
  m_stuff					\
};						\
} while (0)
#else
#define TRC_if_state8(m_dummy)
#endif

#if TRC_local_FLAG & TRCR_STATE8
#define TRCR_if_state8(m_stuff)			\
do {						\
if (TRC_local_flag & TRCR_state8)		\
{						\
  m_stuff					\
};						\
} while (0)
#else
#define TRCR_if_state8(m_dummy)
#endif

#if TRC_local_FLAG & (TRC_STATE8|TRCR_STATE8)
#define TRCA_if_state8(m_stuff)			\
do {						\
if (TRC_local_flag & (TRC_state8|TRCR_state8))	\
{						\
  m_stuff					\
};						\
} while (0)
#else
#define TRCA_if_state8(m_dummy)
#endif

/*
 * TRC_if_stats ()
 * TRCA_if_stats ()
 * TRCR_if_stats ()
 */
#if TRC_local_FLAG & TRC_STATS
#define TRC_if_stats(m_stuff)			\
do {						\
if (TRC_local_flag & TRC_stats)			\
{						\
  m_stuff					\
};						\
} while (0)
#else
#define TRC_if_stats(m_dummy)
#endif

#if TRC_local_FLAG & TRCR_STATS
#define TRCR_if_stats(m_stuff)			\
do {						\
if (TRC_local_flag & TRCR_stats)		\
{						\
  m_stuff					\
};						\
} while (0)
#else
#define TRCR_if_stats(m_dummy)
#endif

#if TRC_local_FLAG & (TRC_STATS|TRCR_STATS)
#define TRCA_if_stats(m_stuff)			\
do {						\
if (TRC_local_flag & (TRC_stats|TRCR_stats))	\
{						\
  m_stuff					\
};						\
} while (0)
#else
#define TRCA_if_stats(m_dummy)
#endif

/*
 * TRC_if_trans ()
 * TRCA_if_trans ()
 * TRCR_if_trans ()
 */
#if TRC_local_FLAG & TRC_trans
#define TRC_if_trans(m_stuff)			\
do {						\
if (TRC_local_flag & TRC_trans)			\
{						\
  m_stuff					\
};						\
} while (0)
#else
#define TRC_if_trans(m_dummy)
#endif

#if TRC_local_FLAG & TRCR_TRANS
#define TRCR_if_trans(m_stuff)			\
do {						\
if (TRC_local_flag & TRCR_trans)		\
{						\
  m_stuff					\
};						\
} while (0)
#else
#define TRCR_if_trans(m_dummy)
#endif

#if TRC_local_FLAG & (TRC_TRANS|TRCR_TRANS)
#define TRCA_if_trans(m_stuff)			\
do {						\
if (TRC_local_flag & (TRC_trans|TRCR_trans))	\
{						\
  m_stuff					\
};						\
} while (0)
#else
#define TRCA_if_trans(m_dummy)
#endif

/*
 * TRC_if_warn ()
 * TRCA_if_warn ()
 * TRCR_if_warn ()
 */
#if TRC_local_FLAG & TRC_LEVEL_WARN
#define TRC_if_warn(m_stuff)			\
do {						\
if (TRC_local_flag & TRC_level_warn)		\
{						\
  m_stuff					\
};						\
} while (0)
#else
#define TRC_if_warn(m_dummy)
#endif

#if TRC_local_FLAG & TRCR_LEVEL_WARN
#define TRCR_if_warn(m_stuff)			\
do {						\
if (TRC_local_flag & TRCR_level_warn)		\
{						\
  m_stuff					\
};						\
} while (0)
#else
#define TRCR_if_warn(m_dummy)
#endif

#if TRC_local_FLAG & (TRC_LEVEL_WARN|TRCR_LEVEL_WARN)
#define TRCA_if_warn(m_stuff)				\
do {							\
if (TRC_local_flag & (TRC_level_warn|TRCR_level_warn))	\
{							\
  m_stuff						\
};							\
} while (0)
#else
#define TRCA_if_warn(m_dummy)
#endif

/*
 * TRC_main()
 * TRCA_main()
 * TRCR_main()
 *
 * DESCRIPTION: trace the name of the main program and the values
 *  of the argc and argv[] arguments.
 *
 *  for TRC_main():
 *  if TRC_ENTRY is cleared in TRC_local_FLAG then
 *    a dummy definition is generated.
 *  if TRC_ENTRY is set in TRC_local_FLAG then
 *    a definition is generated which is a TRC_print() statement and
 *    {{ a call to TRC_main_args_() enclosed in an 
 *      "if (TRC_args & TRC_local_flag)" }}
 *    both enclosed in "if (TRC_entry & TRC_local)flag)"
 *
 *  for TRCR_main():
 *  if TRCR_ENTRY is cleared in TRC_local_FLAG then
 *    a dummy definition is generated.
 *  if TRCR_ENTRY is set in TRC_local_FLAG then
 *    a definition is generated which is a call to TRCR_main_args_()
 *    enclosed in an "if (TRCR_entry & TRC_local_flag)"
 *
 *  for TRCA_func():
 *  if TRC_ENTRY and TRCR_ENTRY are both cleared then
 *    a dummy defiunition is generated
 *  if TRC_ENTRY or TRCR_ENTRY is set in TRC_local_FLAG then
 *    a definition is generated which is TRC_main() and TRCR_main()
 *    inside a do{}while(0) loop
 
 * PARAMETERS:
 *  m_func -- name of the current main program
 *
 * CALLS:
 *  TRC_main_args_()
 *
 * GLOBALS:
 *  argc   -- int,  standard main program first argument
 *  argv[] -- char* standard main program second argument
 *
 * RETURN VALUES: treat as void
 *
 * OUTPUT: 
 */
#if (TRC_local_FLAG &(TRC_ENTRY|TRC_ARGS)) == (TRC_ENTRY|TRC_ARGS)
#define TRC_main(m_func)				\
    TRC_main_args_(m_func, argc, argv, TRC_local_flag)
#else /* TRC_local_FLAG & TRC_ENTRY|TRC_ARGS */
#if TRC_local_FLAG & TRC_ENTRY
#define TRC_main(m_func)					\
do {								\
  if (TRC_local_flag & TRC_entry)				\
  {								\
    TRC_print (TRC_VALUE(TRC_COMP) ":entered:" m_func "\n");	\
  };								\
} while (0)
#else  /* TRC_local_FLAG & TRC_ENTRY */
#define TRC_main(m_dummy)
#endif /* TRC_local_FLAG & TRC_ENTRY */
#endif /* TRC_local_FLAG & TRC_ENTRY|TRC_ARGS */

#if (TRC_local_FLAG &(TRCR_ENTRY|TRCR_ARGS)) == (TRCR_ENTRY|TRCR_ARGS)
#define TRCR_main(m_func)                               \
    TRC_main_args_(m_func, argc, argv, TRC_local_flag)
#else /* TRC_local_FLAG & TRC_ENTRY|TRC_ARGS */
#if TRC_local_FLAG & TRCR_ENTRY
#define TRCR_main(m_func)				\
do {							\
  if (TRC_local_flag & TRCR_entry)			\
  {							\
    KTRACE(TRC_VALUE(TRC_COMP) ":entered:" m_func "\n",	\
           DUMMY_VALUE, DUMMY_VALUE,			\
           DUMMY_VALUE, DUMMY_VALUE);			\
  };							\
} while (0)

#else  /* TRC_local_FLAG & TRCR_ENTRY */
#define TRCR_main(m_dummy)
#endif /* TRC_local_FLAG & TRCR_ENTRY */
#endif /* TRC_local_FLAG & TRCR_ENTRY|TRC_ARGS */

#if (TRC_local_FLAG & (TRC_ENTRY|TRCR_ENTRY))
#define TRCA_main(m_func)			\
do {						\
      TRC_main (m_func);			\
      TRCR_main (m_func);			\
    } while(0)
#else /* (TRC_local_FLAG & (TRC_ENTRY|TRCR_ENTRY)) == (TRC_ENTRY|TRCR_ENTRY))*/
#define TRCA_main(m_dummy)
#endif /*(TRC_local_FLAG & (TRC_ENTRY|TRCR_ENTRY)) == (TRC_ENTRY|TRCR_ENTRY))*/

/*
 * TRC_print()
 * TRCA_print()
 * TRCR_print()
 *
 * DESCRIPTION: Trace the NUL terminated string supplied as the
 *  argument value.  This macro masks the differences between printing
 *  in kernel and user modes.
 *
 *  for TRC_print():
 *    send the output to windbg using OutputDebugString() if in user mode
 *    of DbgPrint() if in kernel mode.
 *
 *  for TRCR_print():
 *    send the output to KTRACE(), passing DUMMY_VALUE as the a0..a3 args
 *
 *  for TRCA_print():
 *    send the output both to windbg and KTRACE()
 *
 * PARAMETERS:
 *  m_arg -- NUL terminated string reference (either a quoted string or
 *           the address of one.
 *
 * CALLS:
 *  DbgPrint()           if in kernel mode
 *  OutputDebugString()  if in user mode
 *  DO_BIST()            if DO_BIST defined
 *  KTRACE()             if TRCR_ or TRCA_
 *
 * GLOBALS: none
 *
 * RETURN VALUES: treat as void
 *
 * GENERATES: none, one, or more of the following:
 *  OutputDebugStringA(<m_arg>)  ( defined(UMODE_ENV)) (ASCII not UNICODE)
 *  DbgPrint          (<m_arg>)  (!defined(UMODE_ENV))
 *  printf            (<m_arg>)  ( defined(DO_BIST))
 *  KTRACE            (<m_arg>)  (TRCR_print())
 * 
 */
#if !defined(DO_BIST)
#if defined(UMODE_ENV) || defined(SIMMODE_ENV)
#define TRC_print(m_arg)                        \
  TRC_OutputDebugString_ (TRC_VALUE(TRC_COMP) ":"  m_arg)
#else /* defined(UMODE_ENV) */
#define TRC_print(m_arg)                        \
  NtDbgPrint     (TRC_VALUE(TRC_COMP) ":"  m_arg)
#endif /* defined(UMODE_ENV) */
#else /* !defined(DO_BIST) */
#define TRC_print(m_arg)                        \
  DO_BIST        (TRC_VALUE(TRC_COMP) ":"  m_arg)
#endif /* !defined(DO_BIST) */

#define TRCR_print(m_arg)					\
  KTRACE (TRC_VALUE(TRC_COMP) ":"  m_arg "\n",			\
          DUMMY_VALUE, DUMMY_VALUE, DUMMY_VALUE, DUMMY_VALUE)

#define TRCRX_print(m_buf, m_arg)				\
  KTRACEX (m_buf, TRC_VALUE(TRC_COMP) ":"  m_arg "\n",		\
          DUMMY_VALUE, DUMMY_VALUE, DUMMY_VALUE, DUMMY_VALUE)

#define TRCA_print(m_arg)			\
do {						\
   TRC_print (m_arg);	\
  TRCR_print (m_arg);	\
   } while(0)

#define TRCAX_print(m_buf, m_arg)			\
do {							\
   TRC_print         (TRC_VALUE(TRC_COMP) ":" m_arg);	\
  TRCRX_print (m_buf, TRC_VALUE(TRC_COMP) ":" m_arg);	\
   } while(0)

#if !defined(DO_BIST)
#if defined(UMODE_ENV) || defined(SIMMODE_ENV)
#define TRCP_print(m_arg)                        \
  TRC_OutputDebugString_ (m_arg)
#else /* defined(UMODE_ENV) */
#define TRCP_print(m_arg)                        \
  NtDbgPrint      (m_arg)
#endif /* defined(UMODE_ENV) */
#else /* !defined(DO_BIST) */
#define TRCP_print(m_arg)                        \
  DO_BIST         (m_arg)
#endif /* !defined(DO_BIST) */

#define TRCRP_print(m_arg)					\
  KTRACE (m_arg "\n",						\
          DUMMY_VALUE, DUMMY_VALUE, DUMMY_VALUE, DUMMY_VALUE)

#define TRCRPX_print(m_buf, m_arg)				\
  KTRACEX (m_buf, m_arg "\n",					\
          DUMMY_VALUE, DUMMY_VALUE, DUMMY_VALUE, DUMMY_VALUE)

#define TRCAP_print(m_arg)			\
do {						\
   TRCP_print (m_arg);				\
  TRCRP_print (m_arg);				\
   } while(0)

#define TRCAPX_print(m_buf, m_arg)		\
do {						\
   TRCP_print         (m_arg);			\
  TRCRPX_print (m_buf, m_arg);			\
   } while(0)

/*
 * TRC_printf()
 *
 * DESCRIPTION: Displays the argument values in the same way as the stdio
 *  printf() function. This macro masks the differences between printing
 *  in kernel and user modes.
 *
 * PARAMETERS:
 *  m_arg -- same a printf
 *
 * NOTE: the argument(s) must be enclosed in doubled parentheses:
 *           TRC_printf (("this is a string, %d, %s\n", number, string));
 *
 * CALLS:
 *  DbgPrint()           if in kernel mode
 *  TRC_sprintf_         if in user mode
 *  DO_BIST()            if DO_BIST defined
 *
 * GLOBALS: none
 *
 * RETURN VALUES: treat as void
 *
 * OUTPUT: one of the following:
 *  trc_sprintf_     (<m_arg>)
 *  DbgPrint         (<m_arg>)
 *  DO_BIST          (<m_arg>)
 * 
 */
#if !defined(DO_BIST)
#if defined(UMODE_ENV) || defined(SIMMODE_ENV)
#define TRC_printf(m_args)                      \
  TRC_sprintf_     m_args/*UNDEF*/
#else /* defined(UMODE_ENV) */
#define TRC_printf(m_args)                      \
  NtDbgPrint       m_args
#endif /* defined(UMODE_ENV) */
#else /* !defined(DO_BIST) */
#define TRC_printf(m_args)                      \
  DO_BIST          m_args
#endif /* !defined(DO_BIST) */

/*
 * TRC_printf1()
 * TRCA_printf1()
 * TRCR_printf1()
 *
 * DESCRIPTION: Displays the argument values in the same way as the stdio
 *  printf() function. It accept exactly one format argument and one data
 *  argument.
 *  This macro masks the differences between printing in kernel and user modes.
 *
 * PARAMETERS:
 *  m_arg  -- same as printf
 *            decomposed to m_arga m_argb
 *
 * NOTE: the argument(s) must be enclosed in doubled parentheses:
 *           TRC_printf ("this is a number, %d\n", number);
 *
 * CALLS:
 *  DbgPrint()           if in kernel mode
 *  TRC_sprintf_         if in user mode
 *  DO_BIST()            if DO_BIST defined
 *  KTRACE()             if TRCR_
 *
 * GLOBALS: none
 *
 * RETURN VALUES: treat as void
 *
 * OUTPUT: one of the following:
 *  trc_sprintf_     (<m_arga>, <m_argb>)
 *  DbgPrint         (<m_arga>, <m_argb>)
 *  DO_BIST          (<m_arga>, <m_argb>)
 *  KTRACE           (<m_arga>, <m_argb>, DUMMY_DATA, DUMMY_DATA, DUMMY_DATA)
 * 
 */
#if !defined(DO_BIST)
#if defined(UMODE_ENV) || defined(SIMMODE_ENV)
#define TRC_printf1(m_arg1, m_arg2)				\
  TRC_sprintf_     (TRC_VALUE(TRC_COMP) ":" m_arg1, m_arg2)
#define TRCP_printf1(m_arg1, m_arg2)		\
  TRC_sprintf_     (m_arg1, m_arg2)
#else /* defined(UMODE_ENV) */
#define TRC_printf1(m_arg1, m_arg2)				\
  NtDbgPrint       (TRC_VALUE(TRC_COMP) ":" m_arg1, m_arg2)
#define TRCP_printf1(m_arg1, m_arg2)		\
  NtDbgPrint       (m_arg1, m_arg2)
#endif /* defined(UMODE_ENV) */
#else /* !defined(DO_BIST) */
#define TRC_printf1(m_arg1, m_arg2)				\
  DO_BIST          (TRC_VALUE(TRC_COMP) ":" m_arg1, m_arg2)
#define TRCP_printf1(m_arg1, m_arg2)		\
  DO_BIST          (m_arg1, m_arg2)
#endif /* !defined(DO_BIST) */

#define TRCR_printf1(m_arg1, m_arg2)			\
  KTRACE (TRC_VALUE(TRC_COMP) ":" m_arg1, m_arg2,	\
          DUMMY_VALUE, DUMMY_VALUE, DUMMY_VALUE)

#define TRCRX_printf1(m_buf, m_arg1, m_arg2)			\
  KTRACEX (m_buf, TRC_VALUE(TRC_COMP) ":" m_arg1, m_arg2,	\
          DUMMY_VALUE, DUMMY_VALUE, DUMMY_VALUE)

#define TRCA_printf1(m_arg1, m_arg2)            \
do {                                            \
         TRC_printf1 (m_arg1, m_arg2);          \
        TRCR_printf1 (m_arg1, m_arg2);          \
   } while(0)

#define TRCAX_printf1(m_buf, m_arg1, m_arg2)	\
do {						\
         TRC_printf1 (m_arg1, m_arg2);		\
        TRCRX_printf1 (m_buf, m_arg1, m_arg2);	\
   } while(0)

#define TRCRP_printf1(m_arg1, m_arg2)			\
  KTRACE (m_arg1, m_arg2,				\
          DUMMY_VALUE, DUMMY_VALUE, DUMMY_VALUE)

#define TRCRPX_printf1(m_buf, m_arg1, m_arg2)		\
  KTRACEX (m_buf, m_arg1, m_arg2,			\
          DUMMY_VALUE, DUMMY_VALUE, DUMMY_VALUE)

#define TRCAP_printf1(m_arg1, m_arg2)		\
do {						\
         TRCP_printf1 (m_arg1, m_arg2);		\
        TRCRP_printf1 (m_arg1, m_arg2);		\
   } while(0)

#define TRCAPX_printf1(m_buf, m_arg1, m_arg2)	\
do {						\
         TRCP_printf1 (m_arg1, m_arg2);		\
        TRCRPX_printf1 (m_buf, m_arg1, m_arg2);	\
   } while(0)

/*
 * TRC_printf2()
 * TRCA_printf2()
 * TRCR_printf2()
 *
 * DESCRIPTION: Displays the argument values in the same way as the stdio
 *  printf() function. It accept exactly one format argument and two data
 *  arguments.
 *  This macro masks the differences between printing in kernel and user modes.
 *
 * PARAMETERS:
 *  m_arg  -- same as printf
 *            decomposed to m_arga m_argb m_argc
 *
 * NOTE: the argument(s) must be enclosed in doubled parentheses:
 *           TRC_printf (("this is a number, %d, %s\n", number, "three"));
 *
 * CALLS:
 *  DbgPrint()           if in kernel mode
 *  TRC_sprintf_         if in user mode
 *  DO_BIST()            if DO_BIST defined
 *  KTRACE()             if TRCR_
 *
 * GLOBALS: none
 *
 * RETURN VALUES: treat as void
 *
 * OUTPUT: one of the following:
 *  trc_sprintf_     (<m_arga>, <m_argb>, <m_argc>)
 *  DbgPrint         (<m_arga>, <m_argb>, <m_argc>)
 *  DO_BIST          (<m_arga>, <m_argb>, <m_argc>)
 *  KTRACE           (<m_arga>, <m_argb>, <m_argc>, DUMMY_DATA, DUMMY_DATA)
 * 
 */
#if !defined(DO_BIST)
#if defined(UMODE_ENV) || defined(SIMMODE_ENV)
#define TRC_printf2(m_arg1, m_arg2, m_arg3)				\
  TRC_sprintf_     (TRC_VALUE(TRC_COMP) ":" m_arg1, m_arg2, m_arg3)
#define TRCP_printf2(m_arg1, m_arg2, m_arg3)     \
  TRC_sprintf_     (m_arg1, m_arg2, m_arg3)
#else /* defined(UMODE_ENV) */
#define TRC_printf2(m_arg1, m_arg2, m_arg3)				\
  NtDbgPrint       (TRC_VALUE(TRC_COMP) ":" m_arg1, m_arg2, m_arg3)
#define TRCP_printf2(m_arg1, m_arg2, m_arg3)     \
  NtDbgPrint       (m_arg1, m_arg2, m_arg3)
#endif /* defined(UMODE_ENV) */
#else /* !defined(DO_BIST) */
#define TRC_printf2(m_arg1, m_arg2, m_arg3)				\
  DO_BIST          (TRC_VALUE(TRC_COMP) ":" m_arg1, m_arg2, m_arg3)
#define TRCP_printf2(m_arg1, m_arg2, m_arg3)     \
  DO_BIST          (m_arg1, m_arg2, m_arg3)
#endif /* !defined(DO_BIST) */

#define TRCR_printf2(m_arg1, m_arg2, m_arg3)				\
  KTRACE            (TRC_VALUE(TRC_COMP) ":" m_arg1, m_arg2, m_arg3,	\
		     DUMMY_VALUE, DUMMY_VALUE)

#define TRCRX_printf2(m_buf, m_arg1, m_arg2, m_arg3)			     \
  KTRACEX            (m_buf, TRC_VALUE(TRC_COMP) ":" m_arg1, m_arg2, m_arg3, \
		     DUMMY_VALUE, DUMMY_VALUE)

#define TRCA_printf2(m_arg1, m_arg2, m_arg3)    \
do {                                            \
         TRC_printf2 (m_arg1, m_arg2, m_arg3);  \
        TRCR_printf2 (m_arg1, m_arg2, m_arg3);  \
   } while(0)

#define TRCAX_printf2(m_buf, m_arg1, m_arg2, m_arg3)	\
do {							\
         TRC_printf2 (m_arg1, m_arg2, m_arg3);		\
        TRCRX_printf2 (m_buf, m_arg1, m_arg2, m_arg3);	\
   } while(0)

#define TRCRP_printf2(m_arg1, m_arg2, m_arg3)			\
  KTRACE (m_arg1, m_arg2, m_arg3, DUMMY_VALUE, DUMMY_VALUE)

#define TRCRPX_printf2(m_buf, m_arg1, m_arg2, m_arg3)			\
  KTRACEX (m_buf, m_arg1, m_arg2, m_arg3, DUMMY_VALUE, DUMMY_VALUE)

#define TRCAP_printf2(m_arg1, m_arg2, m_arg3)	\
do {						\
         TRCP_printf2 (m_arg1, m_arg2, m_arg3);	\
        TRCRP_printf2 (m_arg1, m_arg2, m_arg3);	\
   } while(0)

#define TRCAPX_printf2(m_buf, m_arg1, m_arg2, m_arg3)	\
do {							\
         TRCP_printf2 (m_arg1, m_arg2, m_arg3);		\
        TRCRPX_printf2 (m_buf, m_arg1, m_arg2, m_arg3);	\
   } while(0)

/*
 * TRC_printf3()
 * TRCA_printf3()
 * TRCR_printf3()
 *
 * DESCRIPTION: Displays the argument values in the same way as the stdio
 *  printf() function. It accept exactly one format argument and three data
 *  arguments.
 *  This macro masks the differences between printing in kernel and user modes.
 *
 * PARAMETERS:
 *  m_arg  -- same as printf
 *            decomposed to m_arga m_argb m_argc m_arg_d
 *
 * NOTE: the argument(s) must be enclosed in doubled parentheses:
 *           TRC_printf (("this is a number, %d, %s, '%c'\n",
 *                        number, "three", '3'));
 *
 * CALLS:
 *  DbgPrint()           if in kernel mode
 *  TRC_sprintf_         if in user mode
 *  DO_BIST()            if DO_BIST defined
 *  KTRACE()             if TRCR_
 *
 * GLOBALS: none
 *
 * RETURN VALUES: treat as void
 *
 * OUTPUT: one of the following:
 *  trc_sprintf_     (<m_arga>, <m_argb>, <m_argc>, m_argd>)
 *  DbgPrint         (<m_arga>, <m_argb>, <m_argc>, m_argd>)
 *  DO_BIST          (<m_arga>, <m_argb>, <m_argc>, m_argd>)
 *  KTRACE           (<m_arga>, <m_argb>, <m_argc>, m_argd>, DUMMY_DATA)
 * 
 */
#if !defined(DO_BIST)
#if defined(UMODE_ENV) || defined(SIMMODE_ENV)
#define TRC_printf3(m_arg1, m_arg2, m_arg3, m_arg4)			    \
  TRC_sprintf_     (TRC_VALUE(TRC_COMP) ":" m_arg1, m_arg2, m_arg3, m_arg4)
#define TRCP_printf3(m_arg1, m_arg2, m_arg3, m_arg4)     \
  TRC_sprintf_     (m_arg1, m_arg2, m_arg3, m_arg4)
#else /* defined(UMODE_ENV) */
#define TRC_printf3(m_arg1, m_arg2, m_arg3, m_arg4)			    \
  NtDbgPrint       (TRC_VALUE(TRC_COMP) ":" m_arg1, m_arg2, m_arg3, m_arg4)
#define TRCP_printf3(m_arg1, m_arg2, m_arg3, m_arg4)     \
  NtDbgPrint       (m_arg1, m_arg2, m_arg3, m_arg4)
#endif /* defined(UMODE_ENV) */
#else /* !defined(DO_BIST) */
#define TRC_printf3(m_arg1, m_arg2, m_arg3, m_arg4)			\
  DO_BIST (TRC_VALUE(TRC_COMP) ":" m_arg1, m_arg2, m_arg3, m_arg4)
#define TRCP_printf3(m_arg1, m_arg2, m_arg3, m_arg4)     \
  DO_BIST          (m_arg1, m_arg2, m_arg3, m_arg4)
#endif /* !defined(DO_BIST) */

#define TRCR_printf3(m_arg1, m_arg2, m_arg3, m_arg4)		\
  KTRACE (TRC_VALUE(TRC_COMP)					\
	":" m_arg1, m_arg2, m_arg3, m_arg4, DUMMY_VALUE)

#define TRCRX_printf3(m_buf, m_arg1, m_arg2, m_arg3, m_arg4)		  \
  KTRACEX (m_buf, TRC_VALUE(TRC_COMP) ":" m_arg1, m_arg2, m_arg3, m_arg4, \
	   DUMMY_VALUE)

#define TRCA_printf3(m_arg1, m_arg2, m_arg3, m_arg4)    \
do {                                                    \
         TRC_printf3 (m_arg1, m_arg2, m_arg3, m_arg4);  \
        TRCR_printf3 (m_arg1, m_arg2, m_arg3, m_arg4);  \
   } while(0)

#define TRCAX_printf3(m_buf, m_arg1, m_arg2, m_arg3, m_arg4)	\
do {								\
         TRC_printf3 (m_arg1, m_arg2, m_arg3, m_arg4);		\
        TRCRX_printf3 (m_buf, m_arg1, m_arg2, m_arg3, m_arg4);	\
   } while(0)

#define TRCRP_printf3(m_arg1, m_arg2, m_arg3, m_arg4)	\
  KTRACE (m_arg1, m_arg2, m_arg3, m_arg4, DUMMY_VALUE)

#define TRCRPX_printf3(m_buf, m_arg1, m_arg2, m_arg3, m_arg4)	\
  KTRACEX (m_buf, m_arg1, m_arg2, m_arg3, m_arg4, DUMMY_VALUE)

#define TRCAP_printf3(m_arg1, m_arg2, m_arg3, m_arg4)	\
do {							\
         TRCP_printf3 (m_arg1, m_arg2, m_arg3, m_arg4);	\
        TRCRP_printf3 (m_arg1, m_arg2, m_arg3, m_arg4);	\
   } while(0)

#define TRCAPX_printf3(m_buf, m_arg1, m_arg2, m_arg3, m_arg4)	\
do {								\
         TRCP_printf3         (m_arg1, m_arg2, m_arg3, m_arg4);	\
        TRCRPX_printf3 (m_buf, m_arg1, m_arg2, m_arg3, m_arg4);	\
   } while(0)

/*
 * TRC_printf4()
 * TRCA_printf4()
 * TRCR_printf4()
 *
 * DESCRIPTION: Displays the argument values in the same way as the stdio
 *  printf() function. It accept exactly one format argument and four data
 *  arguments.
 *  This macro masks the differences between printing in kernel and user modes.
 *
 * PARAMETERS:
 *  m_arg  -- same as printf
 *            decomposed to m_arga m_argb m_argc m_argd m_arge
 *
 * NOTE: the argument(s) must be enclosed in doubled parentheses:
 *           TRC_printf (("this is a number, %d, %s, '%c', %#X\n",
 *                        number, "three", '3', '3'));
 *
 * CALLS:
 *  DbgPrint()           if in kernel mode
 *  TRC_sprintf_         if in user mode
 *  DO_BIST()            if DO_BIST defined
 *  KTRACE()             if TRCR_
 *
 * GLOBALS: none
 *
 * RETURN VALUES: treat as void
 *
 * OUTPUT: one of the following:
 *  trc_sprintf_     (<m_arga>, <m_argb>, <m_argc>, <m_argd>, <m_arge>)
 *  DbgPrint         (<m_arga>, <m_argb>, <m_argc>, <m_argd>, <m_arge>)
 *  DO_BIST          (<m_arga>, <m_argb>, <m_argc>, <m_argd>, <m_arge>)
 *  KTRACE           (<m_arga>, <m_argb>, <m_argc>, <m_argd>, <m_arge>)
 * 
 */
#if !defined(DO_BIST)
#if defined(UMODE_ENV) || defined(SIMMODE_ENV)
#define TRC_printf4(m_arg1, m_arg2, m_arg3, m_arg4, m_arg5)	\
  TRC_sprintf_ (TRC_VALUE(TRC_COMP) ":" m_arg1,			\
		m_arg2, m_arg3, m_arg4, m_arg5)
#define TRCP_printf4(m_arg1, m_arg2, m_arg3, m_arg4, m_arg5)	\
  TRC_sprintf_     (m_arg1, m_arg2, m_arg3, m_arg4, m_arg5)
#else /* defined(UMODE_ENV) */
#define TRC_printf4(m_arg1, m_arg2, m_arg3, m_arg4, m_arg5)	\
  NtDbgPrint       (TRC_VALUE(TRC_COMP) ":" m_arg1,		\
		    m_arg2, m_arg3, m_arg4, m_arg5)
#define TRCP_printf4(m_arg1, m_arg2, m_arg3, m_arg4, m_arg5)	\
  NtDbgPrint       (m_arg1, m_arg2, m_arg3, m_arg4, m_arg5)
#endif /* defined(UMODE_ENV) */
#else /* !defined(DO_BIST) */
#define TRC_printf4(m_arg1, m_arg2, m_arg3, m_arg4, m_arg5)		   \
  DO_BIST (TRC_VALUE(TRC_COMP) ":" m_arg1, m_arg2, m_arg3, m_arg4, m_arg5)
#define TRCP_printf4(m_arg1, m_arg2, m_arg3, m_arg4, m_arg5)     \
  DO_BIST          (m_arg1, m_arg2, m_arg3, m_arg4, m_arg5)
#endif /* !defined(DO_BIST) */

#define TRCR_printf4(m_arg1, m_arg2, m_arg3, m_arg4, m_arg5)    \
  KTRACE (TRC_VALUE(TRC_COMP) ":" m_arg1, m_arg2, m_arg3, m_arg4, m_arg5)

#define TRCRX_printf4(m_buf, m_arg1, m_arg2, m_arg3, m_arg4, m_arg5)	\
  KTRACEX (m_buf, TRC_VALUE(TRC_COMP) ":" m_arg1,			\
	   m_arg2, m_arg3, m_arg4, m_arg5)

#define TRCA_printf4(m_arg1, m_arg2, m_arg3, m_arg4, m_arg5)    \
do {                                                            \
         TRC_printf4(m_arg1, m_arg2, m_arg3, m_arg4, m_arg5);   \
        TRCR_printf4(m_arg1, m_arg2, m_arg3, m_arg4, m_arg5);   \
   } while(0)

#define TRCAX_printf4(m_buf, m_arg1, m_arg2, m_arg3, m_arg4, m_arg5)	\
do {									\
         TRC_printf4(m_arg1, m_arg2, m_arg3, m_arg4, m_arg5);		\
        TRCRX_printf4(m_buf, m_arg1, m_arg2, m_arg3, m_arg4, m_arg5);	\
   } while(0)

#define TRCRP_printf4(m_arg1, m_arg2, m_arg3, m_arg4, m_arg5)    \
  KTRACE            (m_arg1, m_arg2, m_arg3, m_arg4, m_arg5)

#define TRCRPX_printf4(m_buf, m_arg1, m_arg2, m_arg3, m_arg4, m_arg5)    \
  KTRACEX (m_buf, m_arg1, m_arg2, m_arg3, m_arg4, m_arg5)

#define TRCAP_printf4(m_arg1, m_arg2, m_arg3, m_arg4, m_arg5)	\
do {								\
         TRCP_printf4(m_arg1, m_arg2, m_arg3, m_arg4, m_arg5);	\
        TRCRP_printf4(m_arg1, m_arg2, m_arg3, m_arg4, m_arg5);	\
   } while(0)

#define TRCAPX_printf4(m_buf, m_arg1, m_arg2, m_arg3, m_arg4, m_arg5)	\
do {									\
         TRCP_printf4(m_arg1, m_arg2, m_arg3, m_arg4, m_arg5);		\
        TRCRPX_printf4(m_buf, m_arg1, m_arg2, m_arg3, m_arg4, m_arg5);	\
   } while(0)
/*
 * TRC_return ()
 *
 * DESCRIPTION: Define macro to support a single "return (val)" point.
 *  This macro stores its argument value in TRC_retval which is declared
 *  by TRC_return_init().  It then performs a "goto TRC_ret".
 *  The TRC_ret label is generated by a TRC_do_return() macro.
 *  If TRC_ARGS is not set in TRC_local_FLAG then "return (<m_val>)"
 *  is generated.
 *
 * PARAMETERS:
 *  m_val -- value to be returned
 *
 * CALLS:
 *  goto TRC_ret;
 *
 * GLOBALS:
 *  TRC_retval -- (w) function scope variable.
 *
 * RETURN_VALUES:
 *  transfers control to TRC_ret in TRC_do_return()
 *
 * OUTPUT:either
 *  do
 *  {
 *    TRC_retval = <m_val>;
 *    goto TRC_ret;
 *  } while (0)
 *
 *         or
 *
 *  return (<m_val);
 */
#if TRC_local_FLAG & (TRC_EXIT | TRCR_EXIT)
#define TRC_return(m_val)                               \
  do {TRC_retval = m_val; goto TRC_ret;} while (0)
#else /* TRC_local_FLAG & (TRC_EXIT | TRCR_EXIT) */
#define TRC_return(m_val)                       \
 return (m_val)/*UNDEF*/
#endif /* TRC_local_FLAG & (TRC_EXIT | TRCR_EXIT) */

/*
 * TRC_return_init ()
 *
 * DESCRIPTION: Define variable to hold return value.
 *
 * This macro defines a variable TRC_retval of type <m_type>.
 * This variable is set by TRC_return() and read by TRC_do_return().
 *
 * PARAMETERS:
 *  m_type -- the function's return "type".
 *
 * CALLS: nothing
 *
 * GLOBALS:
 * TRC_retval -- defined (function scope)
 *
 * RETURN VALUES: none -- data declaration.
 *
 * OUTPUT:
 *  <m_type> TRC_retval
 */
#if TRC_local_FLAG & (TRC_EXIT | TRCR_EXIT)
#define TRC_return_init(m_type)                 \
  m_type TRC_retval
#else /* TRC_local_FLAG & (TRC_EXIT | TRCR_EXIT) */
#define TRC_return_init(m_dummy)/*UNDEF*/
#endif /* TRC_local_FLAG & (TRC_EXIT | TRCR_EXIT) */

/*
 * TRC_return_void ()
 *
 * DESCRIPTION: Define macro to support a single "return" point.
 *
 * This macro performs a "goto TRC_ret_void".
 * The TRC_ret_void label is generated by a TRC_do_return_void() macro.
 * If TRC_EXIT is not set in TRC_local_FLAG, then "return" .
 *
 * PARAMETERS: none
 *
 * CALLS:
 *  goto TRC_ret_void;
 *
 * GLOBALS: none
 *
 * RETURN_VALUES:
 *  transfers control to TRC_ret_void in TRC_do_return_void()
 *
 * OUTPUT:
 *    goto TRC_ret_void
 *            or
 *    return
 */
#if TRC_local_FLAG & (TRC_EXIT | TRCR_EXIT)
#define TRC_return_void()                       \
  goto TRC_ret_void
#else /* TRC_local_FLAG & (TRC_EXIT | TRCR_EXIT) */
#define TRC_return_void()                       \
  return
#endif /* TRC_local_FLAG & (TRC_EXIT | TRCR_EXIT) */

/*
 * TRC_state_any()
 * TRCA_state_any()
 * TRCR_state_any()
 * 
 * DESCRIPTION: Define macro to trace the value of the <m_state>
 *  argument in namelist format, using the <m_spec> printf format
 *  specification, followed by <m_msg> text.
 *  If TRC_state is set in TRC_local_flag display the information.
 *  If TRC_STATE is not set in TRC_local_FLAG then a dummy definition
 *  is generated.
 *
 * PARAMETERS:
 *  m_state -- name of the state variable
 *  m_spec  -- %? specification used to print the state variable
 *  m_msg   -- addition text to display
 *
 * GLOBALS: none.
 *
 * CALLS:
 *  TRC_printf()
 *
 * RETURN_VALUES: treat as void
 *
 * OUTPUT: nothing or
 *  
 * TRC_print((TRC_VALUE(TRC_COMP) ":  state:<m_state>=<m_type> %s\n", <m_state>, <m_msg>))
 */
#if TRC_local_FLAG & TRC_STATE
#define TRC_state_any(m_state, m_spec, m_msg)			\
do {								\
  if (TRC_local_flag & TRC_state)				\
    {								\
       TRC_printf ((TRC_VALUE(TRC_COMP)				\
		    ":  state:" #m_state "=" m_spec " %s\n",	\
		    m_state, m_msg));				\
    };								\
} while (0)
#else /* TRC_local_FLAG & TRC_STATE */
#define TRC_state_any(m_dummy, m_dummy1, m_dummy2)
#endif /* TRC_local_FLAG & TRC_STATE */

#if TRC_local_FLAG & TRCR_STATE
#define TRCR_state_any(m_state, m_spec, m_msg)			\
do {								\
  if (TRC_local_flag & TRCR_state)				\
    {								\
       KTRACE (TRC_VALUE(TRC_COMP)				\
	       ":  state:" #m_state "=" m_spec " %s",		\
                m_state, m_msg, DUMMY_VALUE, DUMMY_VALUE);	\
    };								\
} while (0)

#define TRCRX_state_any(m_buf, m_state, m_spec, m_msg)		\
do {								\
  if (TRC_local_flag & TRCR_state)				\
    {								\
       KTRACEX (m_buf, TRC_VALUE(TRC_COMP)			\
		":  state:" #m_state "=" m_spec " %s",		\
                m_state, m_msg, DUMMY_VALUE, DUMMY_VALUE);	\
    };								\
} while (0)
#else /* TRC_local_FLAG & TRC_STATE */
#define TRCR_state_any(m_dummy, m_dummy1, m_dummy2)
#define TRCRX_state_any(m_dummy0, m_dummy, m_dummy1, m_dummy2)
#endif /* TRC_local_FLAG & TRC_STATE */

#if TRC_local_FLAG & (TRC_STATE|TRCR_STATE)
#define TRCA_state_any(m_state, m_spec, m_msg)  \
do {                                            \
        TRC_state_any (m_state, m_spec, m_msg); \
        TRCR_state_any(m_state, m_spec, m_msg); \
} while (0)

#define TRCAX_state_any(m_buf, m_state, m_spec, m_msg)	\
do {							\
        TRC_state_any (m_state, m_spec, m_msg);		\
        TRCRX_state_any(m_buf, m_state, m_spec, m_msg);	\
} while (0)
#else /* TRC_local_FLAG & (TRC_STATE|TRCR_STATE) */
#define TRCA_state_any(m_dummy, m_dummy1, m_dummy2)
#define TRCAX_state_any(m_dummy0, m_dummy, m_dummy1, m_dummy2)
#endif /* TRC_local_FLAG & (TRC_STATE|TRCR_STATE) */

/*
 * TRC_state2_any()
 * 
 * DESCRIPTION: Define macro to display the value of the <m_state>
 *  argument in namelist format, using the <m_spec> printf format
 *  specification, followed by <m_msg> text.
 *  If TRC_state2 is set in TRC_local_flag display the information.
 *  If TRC_STATE2 is not set in TRC_local_FLAG then a dummy definition
 *  is generated.
 *
 * PARAMETERS:
 *  m_state -- name of the state variable
 *  m_spec  -- %? specification used to print the state variable
 *  m_msg   -- addition text to display
 *
 * GLOBALS: none.
 *
 * CALLS:
 *  TRC_printf()
 *
 * RETURN_VALUES: treat as void
 *
 * OUTPUT: nothing or
 *  
 * TRC_print((TRC_VALUE(TRC_COMP)
 *            ":  state:<m_state>=<m_type> %s\n", <m_state>, <m_msg>))
 */
#if TRC_local_FLAG & TRC_STATE2
#define TRC_state2_any(m_state, m_spec, m_msg)			\
do {								\
  if (TRC_local_flag & TRC_state2)				\
    {								\
       TRC_printf ((TRC_VALUE(TRC_COMP)				\
		    ":  state:" #m_state "=" m_spec " %s\n",	\
		    m_state, m_msg));				\
    };								\
} while (0)
#else /* TRC_local_FLAG & TRC_STATE2 */
#define TRC_state2_any(m_dummy, m_dummy1, m_dummy2)
#endif /* TRC_local_FLAG & TRC_STATE2 */

#if TRC_local_FLAG & TRCR_STATE2
#define TRCR_state2_any(m_state, m_spec, m_msg)			\
do {								\
  if (TRC_local_flag & TRCR_state2)				\
    {								\
       KTRACE (TRC_VALUE(TRC_COMP)				\
	       ":  state:" #m_state "=" m_spec " %s",		\
	       m_state, m_msg, DUMMY_VALUE, DUMMY_VALUE);	\
    };								\
} while (0)

#define TRCRX_state2_any(m_buf, m_state, m_spec, m_msg)		\
do {								\
  if (TRC_local_flag & TRCR_state2)				\
    {								\
       KTRACEX (m_buf, TRC_VALUE(TRC_COMP)			\
		":  state:" #m_state "=" m_spec " %s",		\
                m_state, m_msg, DUMMY_VALUE, DUMMY_VALUE);	\
    };								\
} while (0)
#else /* TRC_local_FLAG & TRC_STATE2 */
#define TRCR_state2_any(m_dummy, m_dummy1, m_dummy2)
#define TRCRX_state2_any(m_dummy0, m_dummy, m_dummy1, m_dummy2)
#endif /* TRC_local_FLAG & TRC_STATE2 */

#if TRC_local_FLAG & (TRC_STATE2|TRCR_STATE2)
#define TRCA_state2_any(m_state, m_spec, m_msg)         \
do {                                                    \
        TRC_state2_any (m_state, m_spec, m_msg);        \
        TRCR_state2_any(m_state, m_spec, m_msg);        \
} while (0)

#define TRCAX_state2_any(m_buf, m_state, m_spec, m_msg)		\
do {								\
        TRC_state2_any (m_state, m_spec, m_msg);		\
        TRCRX_state2_any(m_buf, m_state, m_spec, m_msg);	\
} while (0)
#else /* TRC_local_FLAG & (TRC_STATE2|TRCR_STATE2) */
#define TRCA_state2_any(m_dummy, m_dummy1, m_dummy2)
#define TRCAX_state2_any(m_dummy0, m_dummy, m_dummy1, m_dummy2)
#endif /* TRC_local_FLAG & (TRC_STATE2|TRCR_STATE2) */

/*
 * TRC_state4_any()
 * 
 * DESCRIPTION: Define macro to display the value of the <m_state>
 *  argument in namelist format, using the <m_spec> printf format
 *  specification, followed by <m_msg> text.
 *  If TRC_state4 is set in TRC_local_flag display the information.
 *  If TRC_STATE4 is not set in TRC_local_FLAG then a dummy definition
 *  is generated.
 *
 * PARAMETERS:
 *  m_state -- name of the state variable
 *  m_spec  -- %? specification used to print the state variable
 *  m_msg   -- addition text to display
 *
 * GLOBALS: none.
 *
 * CALLS:
 *  TRC_printf()
 *
 * RETURN_VALUES: treat as void
 *
 * OUTPUT: nothing or
 *  
 * TRC_print((TRC_VALUE(TRC_COMP)
 * ":  state:<m_state>=<m_type> %s\n", <m_state>, <m_msg>))
 */
#if TRC_local_FLAG & TRC_STATE4
#define TRC_state4_any(m_state, m_spec, m_msg)			\
do {								\
  if (TRC_local_flag & TRC_state4)				\
    {								\
       TRC_printf ((TRC_VALUE(TRC_COMP)				\
		    ":  state:" #m_state "=" m_spec " %s\n",	\
		    m_state, m_msg));				\
    };								\
} while (0)
#else /* TRC_local_FLAG & TRC_STATE4 */
#define TRC_state4_any(m_dummy, m_dummy1, m_dummy2)
#endif /* TRC_local_FLAG & TRC_STATE4 */

#if TRC_local_FLAG & TRCR_STATE4
#define TRCR_state4_any(m_state, m_spec, m_msg)			\
do {								\
  if (TRC_local_flag & TRCR_state4)				\
    {								\
       KTRACE (TRC_VALUE(TRC_COMP) ":  state:" #m_state "=" m_spec " %s",	\
                m_state, m_msg, DUMMY_VALUE, DUMMY_VALUE);	\
    };								\
} while (0)

#define TRCRX_state4_any(m_buf, m_state, m_spec, m_msg)			\
do {									\
  if (TRC_local_flag & TRCR_state4)					\
    {									\
       KTRACEX (m_buf, TRC_VALUE(TRC_COMP) ":  state:" #m_state "=" m_spec " %s",	\
                m_state, m_msg, DUMMY_VALUE, DUMMY_VALUE);		\
    };									\
} while (0)
#else /* TRC_local_FLAG & TRC_STATE4 */
#define TRCR_state4_any(m_dummy, m_dummy1, m_dummy2)
#define TRCRX_state4_any(m_dummy0, m_dummy, m_dummy1, m_dummy2)
#endif /* TRC_local_FLAG & TRC_STATE4 */

#if TRC_local_FLAG & (TRC_STATE4|TRCR_STATE4)
#define TRCA_state4_any(m_state, m_spec, m_msg)         \
do {                                                    \
        TRC_state4_any (m_state, m_spec, m_msg);        \
        TRCR_state4_any(m_state, m_spec, m_msg);        \
} while (0)

#define TRCAX_state4_any(m_buf, m_state, m_spec, m_msg)		\
do {								\
        TRC_state4_any (m_state, m_spec, m_msg);		\
        TRCRX_state4_any(m_buf, m_state, m_spec, m_msg);	\
} while (0)
#else /* TRC_local_FLAG & (TRC_STATE4|TRCR_STATE4) */
#define TRCA_state4_any(m_dummy, m_dummy1, m_dummy2)
#define TRCAX_state4_any(m_dummy0, m_dummy, m_dummy1, m_dummy2)
#endif /* TRC_local_FLAG & (TRC_STATE4|TRCR_STATE4) */

/*
 * TRC_state8_any()
 * 
 * DESCRIPTION: Define macro to display the value of the <m_state>
 *  argument in namelist format, using the <m_spec> printf format
 *  specification, followed by <m_msg> text.
 *  If TRC_state8 is set in TRC_local_flag display the information.
 *  If TRC_STATE8 is not set in TRC_local_FLAG then a dummy definition
 *  is generated.
 *
 * PARAMETERS:
 *  m_state -- name of the state variable
 *  m_spec  -- %? specification used to print the state variable
 *  m_msg   -- addition text to display
 *
 * GLOBALS: none.
 *
 * CALLS:
 *  TRC_printf()
 *
 * RETURN_VALUES: treat as void
 *
 * OUTPUT: nothing or
 *  
 * TRC_print((TRC_VALUE(TRC_COMP) ":  state:<m_state>=<m_type> %s\n", <m_state>, <m_msg>))
 */
#if TRC_local_FLAG & TRC_STATE8
#define TRC_state8_any(m_state, m_spec, m_msg)			\
do {								\
  if (TRC_local_flag & TRC_state8)				\
    {								\
       TRC_printf ((TRC_VALUE(TRC_COMP) ":  state:"		\
                                   #m_state "=" m_spec " %s\n",	\
                                    m_state, m_msg));		\
    };								\
} while (0)
#else /* TRC_local_FLAG & TRC_STATE8 */
#define TRC_state8_any(m_dummy, m_dummy1, m_dummy2)
#endif /* TRC_local_FLAG & TRC_STATE8 */

#if TRC_local_FLAG & TRCR_STATE8
#define TRCR_state8_any(m_state, m_spec, m_msg)			\
do {								\
  if (TRC_local_flag & TRCR_state8)				\
    {								\
       KTRACE (TRC_VALUE(TRC_COMP) ":  state:"			\
               #m_state "=" m_spec " %s",			\
                m_state, m_msg, DUMMY_VALUE, DUMMY_VALUE);	\
    };								\
} while (0)

#define TRCRX_state8_any(m_buf, m_state, m_spec, m_msg)		\
do {								\
  if (TRC_local_flag & TRCR_state8)				\
    {								\
       KTRACEX (m_buf, TRC_VALUE(TRC_COMP) ":  state:"	        \
               #m_state "=" m_spec " %s",			\
                m_state, m_msg, DUMMY_VALUE, DUMMY_VALUE);	\
    };								\
} while (0)
#else /* TRC_local_FLAG & TRC_STATE8 */
#define TRCR_state8_any(m_dummy, m_dummy1, m_dummy2)
#define TRCRX_state8_any(m_dummy0, m_dummy, m_dummy1, m_dummy2)
#endif /* TRC_local_FLAG & TRC_STATE8 */

#if TRC_local_FLAG & (TRC_STATE8|TRCR_STATE8)
#define TRCA_state8_any(m_state, m_spec, m_msg)         \
do {                                                    \
        TRC_state8_any (m_state, m_spec, m_msg);        \
        TRCR_state8_any(m_state, m_spec, m_msg);        \
} while (0)

#define TRCAX_state8_any(m_buf, m_state, m_spec, m_msg)		\
do {								\
        TRC_state8_any (m_state, m_spec, m_msg);		\
        TRCRX_state8_any(m_buf, m_state, m_spec, m_msg);	\
} while (0)
#else /* TRC_local_FLAG & (TRC_STATE8|TRCR_STATE8) */
#define TRCA_state8_any(m_dummy, m_dummy1, m_dummy2)
#define TRCAX_state8_any(m_dummy0, m_dummy, m_dummy1, m_dummy2)
#endif /* TRC_local_FLAG & (TRC_STATE8|TRCR_STATE8) */

/*
 * TRC_state_delta()
 * TRCA_state_delta()
 * TRCR_state_delta()
 *
 * DESCRIPTION: Define macro to display the value and previous value of a
 *  state variable WHEN it CHANGES.  It also displays a supplied message.
 *  If TRC_state is set in TRC_local_FLAG display the information.
 *  If TRC_STATE is not set in TRC_local_FLAG then a dummy definition
 *  is generated.
 *
 * PARAMETERS:
 *  m_state -- name of the state variable
 *  m_spec  -- %? specification used to print the state variable
 *  m_msg   -- addition text to display
 *
 * GLOBALS:
 *  TRC_state_old -- previous value of state variable.
 *
 * CALLS:
 *  TRC_printf()
 *
 * RETURN VALUES: treat as void
 *
 * OUTPUT: nothing or
 *
 * TRC_printf ((TRC_VALUE(TRC_COMP) ":  state:prev=<m_spec>, <m_state>=<m_spec>, %s\n",
 *              TRC_state_old, <m_state>, m_msg))
 */
#if TRC_local_FLAG & TRC_STATE
#define TRC_state_delta(m_state, m_spec, m_msg)			\
do {								\
  if ((TRC_local_flag & TRC_state) &&				\
      (m_state != TRC_state_old))				\
  {								\
     TRC_printf ((TRC_VALUE(TRC_COMP)				\
		  ":  state:prev=" m_spec ", " #m_state "="	\
                  m_spec " %s\n",				\
                  TRC_state_old, m_state, m_msg));		\
     TRC_state_old = m_state;					\
  };								\
} while (0)
#else /* TRC_local_FLAG & TRC_STATE */
#define TRC_state_delta(m_dummy, m_dummy1, m_dummy2)
#endif /* TC_local_FLAG & TRC_STATE */

#if TRC_local_FLAG & TRCR_STATE
#define TRCR_state_delta(m_state, m_spec, m_msg)		\
do {								\
  if ((TRC_local_flag & TRCR_state) &&				\
      (m_state != TRC_state_old))				\
  {								\
     KTRACE (TRC_VALUE(TRC_COMP)				\
	     ":  state:prev=" m_spec ", " #m_state "="		\
	     m_spec " %s",					\
	     TRC_state_old, m_state, m_msg, DUMMY_VALUE);	\
     TRC_state_old = m_state;					\
  };								\
} while (0)

#define TRCRX_state_delta(m_buf, m_state, m_spec, m_msg)		\
do {									\
  if ((TRC_local_flag & TRCR_state) &&					\
      (m_state != TRC_state_old))					\
  {									\
     KTRACEX (m_buf, TRC_VALUE(TRC_COMP) ":  state:prev=" m_spec	\
                  ", " #m_state "="					\
                  m_spec " %s",						\
                  TRC_state_old, m_state, m_msg, DUMMY_VALUE);		\
     TRC_state_old = m_state;						\
  };									\
} while (0)
#else /* TRC_local_FLAG & TRCR_STATE */
#define TRCR_state_delta(m_dummy, m_dummy1, m_dummy2)
#define TRCRX_state_delta(m_dummy0, m_dummy, m_dummy1, m_dummy2)
#endif /* TRC_local_FLAG & TRCR_STATE */

#if TRC_local_FLAG & (TRC_STATE|TRCR_STATE)
#define TRCA_state_delta(m_state, m_spec, m_msg)			\
do {									\
  if (m_state != TRC_state_old)						\
    {									\
      if (TRC_local_flag & TRC_state)					\
        {								\
          TRC_printf ((TRC_VALUE(TRC_COMP) ":  state:prev=" m_spec	\
                       ", " #m_state "="				\
                       m_spec " %s\n",					\
                       TRC_state_old, m_state, m_msg));			\
        };								\
      if (TRC_local_flag & TRCR_state)					\
        {								\
          KTRACE (TRC_VALUE(TRC_COMP) ":  state:prev=" m_spec		\
                  ", " #m_state "="					\
                  m_spec " %s",						\
                  TRC_state_old, m_state, m_msg, DUMMY_VALUE);		\
        };								\
      TRC_state_old = m_state;						\
    };									\
   } while (0)

#define TRCAX_state_delta(m_buf, m_state, m_spec, m_msg)		\
do {									\
  if (m_state != TRC_state_old)						\
    {									\
      if (TRC_local_flag & TRC_state)					\
        {								\
          TRC_printf ((TRC_VALUE(TRC_COMP) ":  state:prev=" m_spec	\
                       ", " #m_state "="				\
                       m_spec " %s\n",					\
                       TRC_state_old, m_state, m_msg));			\
        };								\
      if (TRC_local_flag & TRCR_state)					\
        {								\
          KTRACEX (m_buf, TRC_VALUE(TRC_COMP) ":  state:prev=" m_spec	\
                  ", " #m_state "="					\
                  m_spec " %s",						\
                  TRC_state_old, m_state, m_msg, DUMMY_VALUE);		\
        };								\
      TRC_state_old = m_state;						\
    };									\
   } while (0)
#else /* TRC_local_FLAG & (TRC_STATE|TRCR_STATE) */
#define TRCA_state_delta(m_dummy, m_dummy1, m_dummy2)
#define TRCAX_state_delta(m_dummy0, m_dummy, m_dummy1, m_dummy2)
#endif /* TRC_local_FLAG & (TRC_STATE|TRCR_STATE) */

/*
 * TRC_state2_delta()
 *
 * DESCRIPTION: Define macro to display the value and previous value of a
 *  state variable WHEN it CHANGES.  It also displays a supplied message.
 *  If TRC_state2 is set in TRC_local_FLAG display the information.
 *  If TRC_STATE2 is not set in TRC_local_FLAG then a dummy definition
 *  is generated.
 *
 * PARAMETERS:
 *  m_state -- name of the state variable
 *  m_spec  -- %? specification used to print the state variable
 *  m_msg   -- addition text to display
 *
 * GLOBALS:
 *  TRC_state2_old -- previous value of state variable.
 *
 * CALLS:
 *  TRC_printf()
 *
 * RETURN VALUES: treat as void
 *
 * OUTPUT: nothing or
 *
 * TRC_printf ((TRC_VALUE(TRC_COMP)
 *              ":  state:prev=<m_spec>, <m_state>=<m_spec>, %s\n",
 *              TRC_state2_old, <m_state>, m_msg))
 */
#if TRC_local_FLAG & TRC_STATE2
#define TRC_state2_delta(m_state, m_spec, m_msg)		\
do {								\
  if ((TRC_local_flag & TRC_state2) &&				\
      (m_state != TRC_state2_old))				\
  {								\
     TRC_printf ((TRC_VALUE(TRC_COMP)				\
		  ":  state:prev=" m_spec ", " #m_state "="	\
                  m_spec " %s\n",				\
                  TRC_state2_old, m_state, m_msg));		\
     TRC_state2_old = m_state;					\
  };								\
} while (0)
#else /* TRC_local_FLAG & TRC_STATE2 */
#define TRC_state2_delta(m_dummy, m_dummy1, m_dummy2)
#endif /* TC_local_FLAG & TRC_STATE2 */

#if TRC_local_FLAG & TRCR_STATE2
#define TRCR_state2_delta(m_state, m_spec, m_msg)		\
do {								\
  if ((TRC_local_flag & TRCR_state2) &&				\
      (m_state != TRC_state2_old))				\
  {								\
     KTRACE (TRC_VALUE(TRC_COMP) ":  state:prev=" m_spec ", "	\
                 #m_state "="					\
                  m_spec " %s",					\
                  TRC_state2_old, m_state, m_msg, DUMMY_VALUE);	\
     TRC_state2_old = m_state;					\
  };								\
} while (0)

#define TRCRX_state2_delta(m_buf,m_state, m_spec, m_msg)		\
do {									\
  if ((TRC_local_flag & TRCR_state2) &&					\
      (m_state != TRC_state2_old))					\
  {									\
     KTRACEX (m_buf, TRC_VALUE(TRC_COMP) ":  state:prev=" m_spec	\
	              ", " #m_state "="  m_spec " %s",			\
                  TRC_state2_old, m_state, m_msg, DUMMY_VALUE);		\
     TRC_state2_old = m_state;						\
  };									\
} while (0)
#else /* TRC_local_FLAG & TRCR_STATE2 */
#define TRCR_state2_delta(m_dummy, m_dummy1, m_dummy2)
#define TRCRX_state2_delta(m_dummy0, m_dummy, m_dummy1, m_dummy2)
#endif /* TRC_local_FLAG & TRCR_STATE2 */

#if TRC_local_FLAG & (TRC_STATE2|TRCR_STATE2)
#define TRCA_state2_delta(m_state, m_spec, m_msg)			\
do {									\
  if (m_state != TRC_state2_old)					\
    {									\
      if (TRC_local_flag & TRC_state2)					\
        {								\
          TRC_printf ((TRC_VALUE(TRC_COMP) ":  state:prev=" m_spec ", "	\
                      #m_state "="  m_spec " %s\n",			\
                       TRC_state2_old, m_state, m_msg));		\
        };								\
      if (TRC_local_flag & TRCR_state2)					\
        {								\
          KTRACE (TRC_VALUE(TRC_COMP) ":  state:prev=" m_spec ", "	\
                 #m_state "="						\
                  m_spec " %s",						\
                  TRC_state2_old, m_state, m_msg, DUMMY_VALUE);		\
        };								\
      TRC_state2_old = m_state;						\
    };									\
   } while (0)

#define TRCAX_state2_delta(m_buf, m_state, m_spec, m_msg)		\
do {									\
  if (m_state != TRC_state2_old)					\
    {									\
      if (TRC_local_flag & TRC_state2)					\
        {								\
          TRC_printf ((TRC_VALUE(TRC_COMP) ":  state:prev=" m_spec ", "	\
                      #m_state "="  m_spec " %s\n",			\
                       TRC_state2_old, m_state, m_msg));		\
        };								\
      if (TRC_local_flag & TRCR_state2)					\
        {								\
          KTRACEX (m_buf, TRC_VALUE(TRC_COMP) ":  state:prev=" m_spec	\
                   ", " #m_state "=" m_spec " %s",			\
                  TRC_state2_old, m_state, m_msg, DUMMY_VALUE);		\
        };								\
      TRC_state2_old = m_state;						\
    };									\
   } while (0)
#else /* TRC_local_FLAG & (TRC_STATE2|TRCR_STATE2) */
#define TRCA_state2_delta(m_dummy, m_dummy1, m_dummy2)
#define TRCAX_state2_delta(m_dummy0, m_dummy, m_dummy1, m_dummy2)
#endif /* TRC_local_FLAG & (TRC_STATE2|TRCR_STATE2) */

/*
 * TRC_state4_delta()
 *
 * DESCRIPTION: Define macro to display the value and previous value of a
 *  state variable WHEN it CHANGES.  It also displays a supplied message.
 *  If TRC_state4 is set in TRC_local_FLAG display the information.
 *  If TRC_STATE4 is not set in TRC_local_FLAG then a dummy definition
 *  is generated.
 *
 * PARAMETERS:
 *  m_state -- name of the state variable
 *  m_spec  -- %? specification used to print the state variable
 *  m_msg   -- addition text to display
 *
 * GLOBALS:
 *  TRC_state4_old -- previous value of state variable.
 *
 * CALLS:
 *  TRC_printf()
 *
 * RETURN VALUES: treat as void
 *
 * OUTPUT: nothing or
 *
 * TRC_printf ((TRC_VALUE(TRC_COMP) ":  state:prev=<m_spec>,
 *              <m_state>=<m_spec>, %s\n",
 *              TRC_state4_old, <m_state>, m_msg))
 */
#if TRC_local_FLAG & TRC_STATE4
#define TRC_state4_delta(m_state, m_spec, m_msg)	\
do {							\
  if ((TRC_local_flag & TRC_state4) &&			\
      (m_state != TRC_state4_old))			\
  {							\
     TRC_printf ((TRC_VALUE(TRC_COMP) ":  state:prev="	\
                  m_spec ", " #m_state "="		\
                  m_spec " %s\n",			\
                  TRC_state4_old, m_state, m_msg));	\
     TRC_state4_old = m_state;				\
  };							\
} while (0)
#else /* TRC_local_FLAG & TRC_STATE4 */
#define TRC_state4_delta(m_dummy, m_dummy1, m_dummy2)
#endif /* TRC_local_FLAG & TRC_STATE4 */

#if TRC_local_FLAG & TRCR_STATE4
#define TRCR_state4_delta(m_state, m_spec, m_msg)		\
do {								\
  if ((TRC_local_flag & TRCR_state4) &&				\
      (m_state != TRC_state4_old))				\
  {								\
     KTRACE (TRC_VALUE(TRC_COMP) ":  state:prev="		\
                  m_spec ", " #m_state "="			\
                  m_spec " %s",					\
                  TRC_state4_old, m_state, m_msg, DUMMY_VALUE);	\
     TRC_state4_old = m_state;					\
  };								\
} while (0)

#define TRCRX_state4_delta(m_buf, m_state, m_spec, m_msg)	\
do {								\
  if ((TRC_local_flag & TRCR_state4) &&				\
      (m_state != TRC_state4_old))				\
  {								\
     KTRACEX (m_buf, TRC_VALUE(TRC_COMP) ":  state:prev=" m_spec		\
	          ", " #m_state "="				\
                  m_spec " %s",					\
                  TRC_state4_old, m_state, m_msg, DUMMY_VALUE);	\
     TRC_state4_old = m_state;					\
  };								\
} while (0)
#else /* TRC_local_FLAG & TRCR_STATE */
#define TRCR_state4_delta(m_dummy, m_dummy1, m_dummy2)
#define TRCRX_state4_delta(m_dummy0, m_dummy, m_dummy1, m_dummy2)
#endif /* TRC_local_FLAG & TRCR_STATE */

#if TRC_local_FLAG & (TRC_STATE4|TRCR_STATE4)
#define TRCA_state4_delta(m_state, m_spec, m_msg)			\
do {									\
  if (m_state != TRC_state4_old)					\
    {									\
      if (TRC_local_flag & TRC_state4)					\
        {								\
          TRC_printf ((TRC_VALUE(TRC_COMP) ":  state:prev=" m_spec	\
                       ", " #m_state "="				\
                       m_spec " %s\n",					\
                       TRC_state4_old, m_state, m_msg));		\
        };								\
      if (TRC_local_flag & TRCR_state4)					\
        {								\
          KTRACE (TRC_VALUE(TRC_COMP)					\
		  ":  state:prev=" m_spec ", " #m_state "="		\
                  m_spec " %s",						\
                  TRC_state4_old, m_state, m_msg, DUMMY_VALUE);		\
        };								\
      TRC_state4_old = m_state;						\
    };									\
   } while (0)

#define TRCAX_state4_delta(m_buf, m_state, m_spec, m_msg)		 \
do {									 \
  if (m_state != TRC_state4_old)					 \
    {									 \
      if (TRC_local_flag & TRC_state4)					 \
        {								 \
          TRC_printf ((TRC_VALUE(TRC_COMP) ":  state:prev=" m_spec	 \
                       ", " #m_state "="				 \
                       m_spec " %s\n",					 \
                       TRC_state4_old, m_state, m_msg));		 \
        };								 \
      if (TRC_local_flag & TRCR_state4)					 \
        {								 \
          KTRACEX (m_buf, TRC_VALUE(TRC_COMP) ":  state:prev=" m_spec ", \
                  " #m_state "="					 \
                  m_spec " %s",						 \
                  TRC_state4_old, m_state, m_msg, DUMMY_VALUE);		 \
        };								 \
      TRC_state4_old = m_state;						 \
    };									 \
   } while (0)
#else /* TRC_local_FLAG & (TRC_STATE4|TRCR_STATE4) */
#define TRCA_state4_delta(m_dummy, m_dummy1, m_dummy2)
#define TRCAX_state4_delta(m_dummy0, m_dummy, m_dummy1, m_dummy2)
#endif /* TRC_local_FLAG & (TRC_STATE4|TRCR_STATE4) */

/*
 * TRC_state8_delta()
 *
 * DESCRIPTION: Define macro to display the value and previous value of a
 *  state variable WHEN it CHANGES.  It also displays a supplied message.
 *  If TRC_state8 is set in TRC_local_FLAG display the information.
 *  If TRC_STATE8 is not set in TRC_local_FLAG then a dummy definition
 *  is generated.
 *
 * PARAMETERS:
 *  m_state -- name of the state variable
 *  m_spec  -- %? specification used to print the state variable
 *  m_msg   -- addition text to display
 *
 * GLOBALS:
 *  TRC_state8_old -- previous value of state variable.
 *
 * CALLS:
 *  TRC_printf()
 *
 * RETURN VALUES: treat as void
 *
 * OUTPUT: nothing or
 *
 * TRC_printf ((TRC_VALUE(TRC_COMP)
 *              ":  state:prev=<m_spec>, <m_state>=<m_spec>, %s\n",
 *              TRC_state8_old, <m_state>, m_msg))
 */
#if TRC_local_FLAG & TRC_STATE8
#define TRC_state8_delta(m_state, m_spec, m_msg)		\
do {								\
  if ((TRC_local_flag & TRC_state8) &&				\
      (m_state != TRC_state8_old))				\
  {								\
     TRC_printf ((TRC_VALUE(TRC_COMP)				\
		  ":  state:prev=" m_spec ", " #m_state "="	\
                  m_spec " %s\n",				\
                  TRC_state8_old, m_state, m_msg));		\
     TRC_state8_old = m_state;					\
  };								\
} while (0)
#else /* TRC_local_FLAG & TRC_STATE8 */
#define TRC_state8_delta(m_dummy, m_dummy1, m_dummy2)
#endif /* TC_local_FLAG & TRC_STATE8 */

#if TRC_local_FLAG & TRCR_STATE8
#define TRCR_state8_delta(m_state, m_spec, m_msg)		\
do {								\
  if ((TRC_local_flag & TRCR_state8) &&				\
      (m_state != TRC_state8_old))				\
  {								\
     KTRACE (TRC_VALUE(TRC_COMP)				\
             ":  state:prev=" m_spec ", " #m_state "="		\
             m_spec " %s",					\
             TRC_state8_old, m_state, m_msg, DUMMY_VALUE);	\
     TRC_state8_old = m_state;					\
  };								\
} while (0)

#define TRCRX_state8_delta(m_buf, m_state, m_spec, m_msg)		\
do {									\
  if ((TRC_local_flag & TRCR_state8) &&					\
      (m_state != TRC_state8_old))					\
  {									\
     KTRACEX (m_buf, TRC_VALUE(TRC_COMP) ":  state:prev=" m_spec	\
              ", " #m_state "="						\
                  m_spec " %s",						\
                  TRC_state8_old, m_state, m_msg, DUMMY_VALUE);		\
     TRC_state8_old = m_state;						\
  };									\
} while (0)
#else /* TRC_local_FLAG & TRCR_STATE8 */
#define TRCR_state8_delta(m_dummy, m_dummy1, m_dummy2)
#define TRCRX_state8_delta(m_dummy0, m_dummy, m_dummy1, m_dummy2)
#endif /* TRC_local_FLAG & TRCR_STATE8 */

#if TRC_local_FLAG & (TRC_STATE8|TRCR_STATE8)
#define TRCA_state8_delta(m_state, m_spec, m_msg)			\
do {									\
  if (m_state != TRC_state8_old)					\
    {									\
      if (TRC_local_flag & TRC_state8)					\
        {								\
          TRC_printf ((TRC_VALUE(TRC_COMP) ":  state:prev=" m_spec	\
	     ", " #m_state "="						\
                       m_spec " %s\n",					\
                       TRC_state8_old, m_state, m_msg));		\
        };								\
      if (TRC_local_flag & TRCR_state8)					\
        {								\
          KTRACE (TRC_VALUE(TRC_COMP) ":  state:prev=" m_spec		\
	  ", " #m_state "="						\
                  m_spec " %s",						\
                  TRC_state8_old, m_state, m_msg, DUMMY_VALUE);		\
        };								\
      TRC_state8_old = m_state;						\
    };									\
   } while (0)

#define TRCAX_state8_delta(m_buf, m_state, m_spec, m_msg)		\
do {									\
  if (m_state != TRC_state8_old)					\
    {									\
      if (TRC_local_flag & TRC_state8)					\
        {								\
          TRC_printf ((TRC_VALUE(TRC_COMP) ":  state:prev=" m_spec	\
	     ", " #m_state "="						\
                       m_spec " %s\n",					\
                       TRC_state8_old, m_state, m_msg));		\
        };								\
      if (TRC_local_flag & TRCR_state8)					\
        {								\
          KTRACEX (m_buf, TRC_VALUE(TRC_COMP) ":  state:prev=" m_spec	\
	  ", " #m_state "="						\
                  m_spec " %s",						\
                  TRC_state8_old, m_state, m_msg, DUMMY_VALUE);		\
        };								\
      TRC_state8_old = m_state;						\
    };									\
   } while (0)
#else /* TRC_local_FLAG & (TRC_STATE8|TRCR_STATE8) */
#define TRCA_state8_delta(m_dummy, m_dummy1, m_dummy2)
#define TRCAX_state8_delta(m_dummy0, m_dummy, m_dummy1, m_dummy2)
#endif /* TRC_local_FLAG & (TRC_STATE8|TRCR_STATE8) */

/*
 * TRC_state_init()
 *
 * DESCRIPTION: Define macro to declare a variable to contain the
 *  previous value of a state variable.  the variable is initialized
 *  to a supplied value.
 *  If TRC_STATE is not set in TRC_local_FLAG, then a dummy definition
 *  is generated.
 *
 * PARAMETERS:
 *  m_type -- data type of variable
 *  m_val  -- initial value
 *
 * GLOBALS:
 *  TRC_state_old -- name of the variable.
 *
 * CALLS: nothing
 *
 * RETURN VALUES: none (data declaration statement)
 *
 * OUTPUT:
 *   <m_type> TRC_state_old - <m_val>
 */
#if TRC_local_FLAG & (TRC_STATE|TRCR_STATE)
#define TRC_state_init(m_type, m_val)   \
  static m_type TRC_state_old = m_val
#else /* TRC_local_FLAG & (TRC_STATE|TRCR_STATE) */
#define TRC_state_init(m_dummy, m_dummy1)
#endif /* TRC_local_FLAG & (TRC_STATE|TRCR_STATE) */
    
/*
 * TRC_state2_init()
 *
 * DESCRIPTION: Define macro to declare a variable to contain the
 *  previous value of a state variable.  the variable is initialized
 *  to a supplied value.
 *  If TRC_STATE2 is not set in TRC_local_FLAG, then a dummy definition
 *  is generated.
 *
 * PARAMETERS:
 *  m_type -- data type of variable
 *  m_val  -- initial value
 *
 * GLOBALS:
 *  TRC_state2_old -- name of the variable.
 *
 * CALLS: nothing
 *
 * RETURN VALUES: none (data declaration statement)
 *
 * OUTPUT:
 *   <m_type> TRC_state2_old - <m_val>
 */
#if TRC_local_FLAG & (TRC_STATE2|TRCR_STATE2)
#define TRC_state2_init(m_type, m_val)  \
  static m_type TRC_state2_old = m_val
#else /* TRC_local_FLAG & (TRC_STATE2|TRCR_STATE2) */
#define TRC_state2_init(m_dummy, m_dummy1)
#endif /* TRC_local_FLAG & (TRC_STATE2|TRCR_STATE2) */
    
/*
 * TRC_state4_init()
 *
 * DESCRIPTION: Define macro to declare a variable to contain the
 *  previous value of a state variable.  the variable is initialized
 *  to a supplied value.
 *  If TRC_STATE4 is not set in TRC_local_FLAG, then a dummy definition
 *  is generated.
 *
 * PARAMETERS:
 *  m_type -- data type of variable
 *  m_val  -- initial value
 *
 * GLOBALS:
 *  TRC_state4_old -- name of the variable.
 *
 * CALLS: nothing
 *
 * RETURN VALUES: none (data declaration statement)
 *
 * OUTPUT:
 *   <m_type> TRC_state4_old - <m_val>
 */
#if TRC_local_FLAG & (TRC_STATE4|TRCR_STATE4)
#define TRC_state4_init(m_type, m_val)  \
  static m_type TRC_state4_old = m_val
#else /* TRC_local_FLAG & (TRC_STATE4|TRCR_STATE4) */
#define TRC_state4_init(m_dummy, m_dummy1)
#endif /* TRC_local_FLAG & (TRC_STATE4|TRCR_STATE4) */
    
/*
 * TRC_state8_init()
 *
 * DESCRIPTION: Define macro to declare a variable to contain the
 *  previous value of a state variable.  the variable is initialized
 *  to a supplied value.
 *  If TRC_STATE8 is not set in TRC_local_FLAG, then a dummy definition
 *  is generated.
 *
 * PARAMETERS:
 *  m_type -- data type of variable
 *  m_val  -- initial value
 *
 * GLOBALS:
 *  TRC_state8_old -- name of the variable.
 *
 * CALLS: nothing
 *
 * RETURN VALUES: none (data declaration statement)
 *
 * OUTPUT:
 *   <m_type> TRC_state8_old - <m_val>
 */
#if TRC_local_FLAG & (TRC_STATE8|TRCR_STATE8)
#define TRC_state8_init(m_type, m_val)  \
  static m_type TRC_state8_old = m_val
#else /* TRC_local_FLAG & (TRC_STATE8|TRCR_STATE8) */
#define TRC_state8_init(m_dummy, m_dummy1)
#endif /* TRC_local_FLAG & (TRC_STATE8|TRCR_STATE8) */
    
/*
 * TRC_span_fru_init()
 * 
 *
 * DESCRIPTION: Define macro that declares two variables that contain
 *  the minimum and maximum FRU number of interest.  If they have the same
 *  value then one FRU number is of interest.  If min is higher than max
 *  then no FRU numbers are of interest.
 *  If TRC[R]_SPAN_FRUS is not set in TRC_local_FLAGS, then a dummy definition
 *  is generated.
 *
 * NOTE: uses FRU_T typedef which is "temporarily" in this file.  It should
 *  be move to generics.h or some such file.
 *
 * PARAMETERS:
 *  m_min -- default minimum value
 *  m_max -- default maximum value
 *
 * GLOBALS:
 *  TRC_span_fru_min -- name of variable
 *  TRC_span_fru_max -- name of variable
 *
 * CALLS: nothing
 *
 * RETURN VALUES: none (data declaration statement)
 *
 * OUTPUT:
 *  extern FRU_T TRC_span_fru_min = <m_min>;
 *  extern FRU_T TRC_span_fru_max = <m_max>
 *
 */
#if TRC_local_FLAG & (TRC_SPAN_FRUS|TRCR_SPAN_FRUS)
#define TRC_span_fru_init(m_min, m_max)         \
FRU_T TRC_span_fru_min = m_min;          \
FRU_T TRC_span_fru_max = m_max
#else /* TRC_local_FLAG & (TRC_SPAN_FRUS|TRCR_SPAN_FRUS) */
#define TRC_span_fru_init(m_dummy, m_dummy1)
#endif /* TRC_local_FLAG & (TRC_SPAN_FRUS|TRCR_SPAN_FRUS) */
    
/*
 * TRC[R]_span_fru(m_fru, (m_msg, m_a0, m_a1, m_a2, m_a3))
 *
 * DESCRIPTION: Define a macro that places a record in the KTRACE ring
 *  buffer whenever it is called with a FRU number within the inclusive
 *  range from TRC_span_fru_min to TRC_span_fru_max.
 *  If TRCR_span_fru is set in TRC_local_FLAG display the information.  
 *  If TRCR_SPAN_FRU is not set in TRC_local_FLAGS, then a dummy definition
 *  is generated.
 *
 * PARAMETERS:
 *  m_fru -- variable containing fru number
 *  m_msg -- printf format referencing 0 to 4 variables.
 *  m_a0  -- first variable.
 *  m_a1  -- second variable.
 *  m_a2  -- third variable.
 *  m_a3  -- fourth variable.
 *
 * GLOBALS:
 *  TRC_span_fru_min -- min value for fru number of interest
 *  TRC_span_fru_max -- max value for fru number of interest
 *
 * CALLS:
 *  KTRACE()
 *
 * RETURN VALUES: treat as void
 *
 * OUTPUT:
 *  KTRACE (<m_msg>, <m_a0>, <m_a1>, <m_a2>, <m_a3>) 
 */

#if TRC_SPAN_FRUS & TRC_local_FLAG
#define TRC_span_fru(m_fru, m_args)             \
do {                                            \
if ((TRC_span_frus & TRC_local_flag)            \
    && ((m_fru == TRC_SPAN_FRU_ALWAYS)          \
    || ((m_fru <= TRC_span_fru_max)             \
    &&  (m_fru >= TRC_span_fru_min))))          \
{                                               \
   TRC_printf (m_args);                         \
};                                              \
} while (0)
#else /*  TRC_SPAN_FRUS & TRC_local_FLAG */
#define TRC_span_fru(m_dummy,  m_dummy1)
#endif /*  TRC_SPAN_FRUS & TRC_local_FLAG */

#if TRCR_SPAN_FRUS & TRCR_local_FLAG
#define TRCR_span_fru(m_fru, m_args)            \
do {                                            \
if ((TRCR_span_frus & TRC_local_flag)           \
    && ((m_fru == TRC_SPAN_FRU_ALWAYS)          \
    || ((m_fru <= TRC_span_fru_max)             \
    &&  (m_fru >= TRC_span_fru_min))))          \
{                                               \
   KTRACE m_args;                               \
};                                              \
} while (0)

#define TRCRX_span_fru(m_fru,/*m_buf*/  m_args)	\
do {						\
if ((TRCR_span_frus & TRC_local_flag)		\
    && ((m_fru == TRC_SPAN_FRU_ALWAYS)		\
    || ((m_fru <= TRC_span_fru_max)		\
    &&  (m_fru >= TRC_span_fru_min))))		\
{						\
   KTRACEX m_args;				\
};						\
} while (0)
#else /*  TRCR_SPAN_FRUS & TRC_local_FLAG */
#define TRCR_span_fru(m_dummy,  m_dummy1)
#define TRCRX_span_fru(m_dummy0, m_dummy,  m_dummy1)
#endif /*  TRCR_SPAN_FRUS & TRC_local_FLAG */

#if TRC_local_FLAG & (TRC_SPAN_FRUS|TRCR_SPAN_FRUS)
#define TRCA_span_fru(m_fru, m_args)            \
do {                                            \
   TRC_span_fru  (m_fru, m_args);               \
   TRCR_span_fru (m_fru, m_args);               \
   } while (0)

#define TRCAX_span_fru(m_buf, m_fru, m_args)	\
do {						\
   TRC_span_fru  (m_fru, m_args);		\
   TRCRX_span_fru (m_buf, m_fru, m_args);	\
   } while (0)
#else /*  TRC_local_FLAG & (TRC_SPAN_FRUS|TRCR_SPAN_FRUS) */
#define TRCA_span_fru(m_dummy,  m_dummy1)
#define TRCAX_span_fru(m_dummy0, m_dummy,  m_dummy1)
#endif /*  TRC_local_FLAG & (TRC_SPAN_FRUS|TRCR_SPAN_FRUS) */

/*
 * TRC_span_fru_set_max()
 *
 * DESCRIPTION: Define a macro that sets the global variable used
 *  highest interesting fru number.
 *
 * PARAMETERS:
 *  m_val --
 *
 * GLOBALS:
 *  TRC_span_fru_max --
 *
 * CALLS: nothing
 *
 * RETURN VALUES: argument value
 *
 * OUTPUT:
 *      TRC_span_fru_max = <m_val>
 *
 */
#if TRC_local_FLAG & (TRC_SPAN_FRUS | TRCR_SPAN_FRUS)
#define TRC_span_fru_set_max(m_val)        \
TRC_span_fru_max = m_val
#else /* TRC_local_FLAG & (TRC_SPAN_FRUS | TRCR_SPAN_FRUS) */
#define TRC_span_fru_set_max(m_dummy)
#endif /* TRC_local_FLAG & (TRC_SPAN_FRUS | TRCR_SPAN_FRUS) */

/*
 * TRC_span_fru_set_min()
 *
 * DESCRIPTION: Define a macro that sets the global variable used
 *  lowest interesting fru number.
 *
 * PARAMETERS:
 *  m_val --
 *
 * GLOBALS:
 *  TRC_span_fru_min --
 *
 * CALLS: nothing
 *
 * RETURN VALUES: argument value
 *
 * OUTPUT:
 *      TRC_span_fru_min = <m_val>
 *
 */
#if TRC_local_FLAG & (TRC_SPAN_FRUS | TRCR_SPAN_FRUS)
#define TRC_span_fru_set_min(m_val)        \
TRC_span_fru_min = m_val
#else /* TRC_local_FLAG & (TRC_SPAN_FRUS | TRCR_SPAN_FRUS) */
#define TRC_span_fru_set_min(m_dummy)
#endif /* TRC_local_FLAG & (TRC_SPAN_FRUS | TRCR_SPAN_FRUS) */
#endif /* !defined(TRC_UTIL_H) */ /* last line in file */
