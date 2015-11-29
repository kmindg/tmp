/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               Thread.h
 ***************************************************************************
 *
 * DESCRIPTION:  Definition of Interface Thread and ThreadRunner,
 *               which contains methods
 *                  start()
 *                  stop()
 *                  getName()
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    10/21/2010  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef _THREAD_
#define _THREAD_
# include "simulation/Runnable.h"

/*
 * Class Thread defines a simple generic Interface for managing a thread
 */
class Thread
{
public:
    virtual ~Thread() {}
    /*
     * \fn const char *getName()
     *
     * \details
     *
     *   return the name that identifies this thread
     */
    virtual const char *getName() = 0;

    /*
     * \fn void start()
     *
     * \details
     *
     *  call start() to launch a threads execution.
     *  A new Thread context is created to execute
     *  any behavior contained in this thread.
     */
    virtual void start() = 0;

    /*
     * \fn void *stop()
     *
     * \details
     *
     *   call stop() to shutdown a threads execution
     *   The 
     */
    virtual void stop()  = 0;
};

class ThreadRunner: public Thread, public Runnable
{
public:
    ThreadRunner(Runnable *task, const char *processName = NULL);
    ~ThreadRunner();

    /*
     * \fn const char *getName()
     *
     * \details
     *
     *   return the name that identifies this ThreadRunner
     */
    const char *getName();

    /*
     * \fn void run()
     *
     * \details
     *
     *  call run() to execute ThreadRunner logic within
     *             the current thread context
     */
    void run();

    /*
     * \fn void start()
     *
     * \details
     *
     *  call start() to launch a ThreadRunners execution
     *  Once the new thread context is launched,
     *  the run() method is called to execute the behavior
     *  containedd in this thread. If/when the run() method
     *  returns, the thread context will be suspended.
     */
    void start();

    /*
     * \fn void *stop()
     *
     * \details
     *
     *   call stop() to shutdown a ThreadRunners execution.
     *   
     */
    void stop();
private:

    /*
     * \fn void launch()
     * \param context -  opaque pointer to a ThreadRunner instance
     *
     * \details
     *
     *  Static method launch() is the generic entrypoint
     *  for launching a thread context.  The supplied context
     *  is an opaque pointer to a ThreadRunner instance.
     *
     *  This method provides a generic API to interface with
     *  different thread management systems. The initial ThreadRunner
     *  implementation is using the EmcPAL thread API.
     */
    static void launch(void *context);

    const char  *mProcessName;
    Runnable    *mTask;
    void        *mThreadData;
};

#endif //_THREAD_
