/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                mut_sdk.h
 ***************************************************************************
 *
 * DESCRIPTION: MUT Framework SDK
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    11/22/2007 Kirill Timofeev initial version
 *    11/22/2007 Igor Bobrovnikov windows dependency removed from header
 *    01/18/2008 Igor Bobrovnikov mutexes added
 *
 **************************************************************************/

#ifndef MUT_SDK_H
#define MUT_SDK_H

#if ! defined(MUT_REV2)

#include "mut.h" /* For MUT defines */

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
 * MUT THREAD BEGIN
 **************************************************************************/

typedef DWORD MUT_THREAD_STATUS;
#define MUT_THREAD_SUCCESS 0
#define MUT_THREAD_TIMEOUT 1

#define MUT_FALSE 0
#define MUT_TRUE 1

/** \struct mut_thread_s
 *  \details platform independent structure for multithreading support
 */
typedef struct mut_thread_s {
    void *handle;
} mut_thread_t;

/** \struct mut_thread_context_s
 *  \details platform independent structure for multithreading support
 */
typedef struct mut_thread_context_s {
    void (*startroutine)(void *startcontext);
    void *startcontext;
} mut_thread_context_t;

/** \fn MUT_NTSTATUS mut_thread_init(MUT_THREAD thread,
 *      void (*startroutine)(void *startcontext), void *startcontext)
 *  \param thread - the newly created thread
 *  \param startroutine - function we want to start in separate thread
 *  \param startcontext - pointer which would be given as an argument to
 *  the startroutine
 */
MUT_API MUT_THREAD_STATUS mut_thread_init(mut_thread_t *thread,
        void (*startroutine)(void *startcontext), void *startcontext);

/** \fn MUT_NTSTATUS mut_thread_wait(void *thread, DWORD timeout)
 *  \param thread - pointer to the mut_thread_t structure
 *  \param timeout - max time to wait
 *  \return operation status
 */
MUT_API MUT_THREAD_STATUS mut_thread_wait(mut_thread_t *thread,
        unsigned long timeout);

/** \fn void mut_terminate_thread(void *thread)
 *  \param thread - pointer to the thread structure
 */
MUT_API unsigned mut_terminate_thread(mut_thread_t *thread);

MUT_API void mut_terminate_this_thread(void);

/***************************************************************************
 * MUT THREAD END
 **************************************************************************/

/***************************************************************************
 * MUT MUTEX BEGIN
 **************************************************************************/

/** \struct mut_thread_s
 *  \details platform independent structure for multithreading support
 */
typedef struct mut_mutex_s {
    csx_p_mut_t mut;
} mut_mutex_t;

/** \fn void mut_mutex_init(mut_mutex_t *mutex)
 *  \param mutex - pointer to the mutex structure
 *  \details mutex structure initialization
 */
MUT_API void mut_mutex_init(mut_mutex_t *mutex);

/** \fn void mut_mutex_destroy(mut_mutex_t *mutex)
 *  \param mutex - pointer to the mutex structure
 *  \details destroy mutex structure
 */
MUT_API void mut_mutex_destroy(mut_mutex_t *mutex);

/** \fn void mut_mutex_acquire(mut_mutex_t *mutex)
 *  \param mutex - pointer to the mutex structure
 *  \details mutex acquiring (enetering the critical section)
 */
MUT_API void mut_mutex_acquire(mut_mutex_t *mutex);

/** \fn void mut_mutex_release(mut_mutex_t *mutex)
 *  \param mutex - pointer to the mutex structure
 *  \details mutex releasing (leaving the critical section)
 */
MUT_API void mut_mutex_release(mut_mutex_t *mutex);

/***************************************************************************
 * MUT MUTEX END
 **************************************************************************/

/** \fn void mut_sleep(unsigned long ms)
 *  \param ms - time interval in milliseconds
 *  \details suspends the execution of the current thread for the specified time
 */
MUT_API void mut_sleep(unsigned long ms);

/** \fn unsigned long mut_tickcount_get(void)
 *  \return the number of milliseconds elapsed since the system was started
 */
MUT_API unsigned long mut_tickcount_get(void);

/** \fn char *mut_get_host_name(void)
 *  \return user name as a string
 */
MUT_API char *mut_get_host_name(void);

/** \fn char *mut_get_user_name(void)
 *  \return user name as a string
 */
MUT_API char *mut_get_user_name(void);

/** \fn char *mut_register_exit_handler(void (*handler)(void))
 *  \return user name as a string
 */

#ifdef __cplusplus
}
#endif

#endif
	
#endif /* MUT_SDK_H */
