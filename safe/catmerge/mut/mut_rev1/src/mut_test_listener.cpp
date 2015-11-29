/***************************************************************************
 * Copyright (C) EMC Corporation 2010-2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                mut_test_listener.cpp
 ***************************************************************************
 *
 * DESCRIPTION: MUT test listener implementation
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *
 **************************************************************************/
# include "mut_test_listener.h"
# include "mut_private.h"
# include "mut_sdk.h"

extern void mut_strip_terminators(char* buf);


static char *mtl_name = "Mut_test_listener";
static char *mtl_client_name = "Mut_test_client_lock";

Mut_test_listener::Mut_test_listener(Mut_test_control *control)
: mControl(control), mListenerRunning(FALSE),
  mSegment(new shmem_segment(mtl_name, sizeof(mtl_data_t))),
  mService(new shmem_service(mSegment, mtl_name, sizeof(mtl_data_t))),
  mClientLock(new shmem_service(mSegment, mtl_client_name)),
  mTestData((mtl_data_t *)mService->get_base()) {

  csx_p_memzero(&mTestThread_handle, sizeof(mTestThread_handle));

    if(!mService->get_flag(mtl_ctl_initialized)) {
        mService->set_flags(3);   // Initialized/enabled

    }
    /*
     * Now drain the semaphore, as they were init to 1
     * Use small timeout, as it may have been drained previously
     */
    mClientLock->wait(15);
  }

Mut_test_listener::~Mut_test_listener() {
    if(mListenerRunning) {
        /*
         * shut down the listener thread
         **/
#if 0
        mClientLock->lock();
#endif /* SAFEBUG - we can deadlock here if the slave SP processes exit while holding this lock */
        mService->set_flag(mtl_ctl_exit, true);
        
        mService->notify();
        mClientLock->wait();
        
        assert_false(mService->get_flag(mtl_ctl_enabled));
        assert_false(mService->get_flag(mtl_ctl_exit));
        assert_true(mService->get_flag(mtl_ctl_done));
#if 0 
        mClientLock->unlock();
#endif /* SAFEBUG - we can deadlock here if the slave SP processes exit while holding this lock */
    }

    delete mService;
    delete mSegment;

}

enum Mut_test_status_e Mut_test_listener::getTestStatus() {
    return mTestData->status;
}

void Mut_test_listener::setTestStatus(enum Mut_test_status_e status) {
    mTestData->status = status;
}

char *Mut_test_listener::getBuffer() {
    return mTestData->buffer;
}

void Mut_test_listener::run() {
    /*
     * Indicate that the Listener thread is running in the current process
     * The destructor is responsible for shutting down the listener,
     * but only from within the process that started it
     */
    mListenerRunning = TRUE;
     while(true) {
        mService->wait();
        //UINT_64 flags = mService->get_flags();
        //mut_printf(MUT_LOG_TEST_STATUS,"server queried flags 0x%x", (UINT_32)flags);

        if(mService->get_flag(mtl_ctl_exit)) {
            mService->set_flag(mtl_ctl_enabled, false);
            mService->set_flag(mtl_ctl_exit, false);
            mService->set_flag(mtl_ctl_done, true);

            //UINT_64 flags = mService->get_flags();
            //mut_printf(MUT_LOG_TEST_STATUS, "server processed exit, flags 0x%x", (UINT_32)flags);

            mClientLock->notify();

            return;
        }

        if(mService->get_flag(mtl_test_txt_msg)) {
            
            char* buf = getBuffer();
            mut_strip_terminators(buf);

            mControl->mut_log->mut_log_console(buf);
            mControl->mut_log->mut_log_text_file(buf);

            mService->set_flag(mtl_test_txt_msg, false);    // reset
            mService->set_flag(mtl_ctl_msg_done, true);     // report

            assert_false(mService->get_flag(mtl_test_txt_msg));
            assert_true(mService->get_flag(mtl_ctl_msg_done));

            //flags = mService->get_flags();
            //mut_printf(MUT_LOG_TEST_STATUS, "server processed text, flags 0x%x", (UINT_32)flags);

            mClientLock->notify();

            continue;
        }

        if(mService->get_flag(mtl_test_xml_msg)) {

            mControl->mut_log->mut_log_xml_file(getBuffer());

            mService->set_flag(mtl_test_xml_msg, false);    // reset
            mService->set_flag(mtl_ctl_msg_done, true);     //report

            //UINT_64 flags = mService->get_flags();
            //mut_printf(MUT_LOG_TEST_STATUS, "server processed xml, flags 0x%x", (UINT_32)flags);

            mClientLock->notify();

            continue;
        }
     }
}

void Mut_test_listener::send_text_msg(const char *buffer) {
    if(buffer == NULL || *buffer == '\0') {
        return;
    }

    MUT_ASSERT_TRUE_MSG((strlen(buffer) < MTL_BUFFER_LEN),
                        "buffer too large");

    if(mService->get_flag(mtl_ctl_enabled)) {
        /*
         * Serialize access to the Msg mailbox
         */
        /* this gets called by simulation data path code that may do logging with a spinlock held.  
         * disable CSX xlevel validation so CSX won't complain that we're sleeping while holding a lock */
        csx_p_thr_disable_xlevel_validation();
        mClientLock->lock();
        
        assert_false(mService->get_flag(mtl_test_txt_msg));
        
        strcpy(getBuffer(), buffer);
        mService->set_flag(mtl_test_txt_msg, true);   // report
        
        mService->notify();
        mClientLock->wait();
        
        assert_false(mService->get_flag(mtl_test_txt_msg));
        assert_true(mService->get_flag(mtl_ctl_msg_done));
        
        mService->set_flag(mtl_ctl_msg_done, false);  //reset
        
        mClientLock->unlock();
        csx_p_thr_enable_xlevel_validation();
    }
}

void Mut_test_listener::send_xml_msg(const char *buffer) {
    if(buffer == NULL || *buffer == '\0') {
        return;
    }

    MUT_ASSERT_TRUE_MSG((strlen(buffer) < MTL_BUFFER_LEN),
                        "buffer too large");

    if(mService->get_flag(mtl_ctl_enabled)) {
        /* this gets called by simulation data path code that may do logging with a spinlock held.  
         * disable CSX xlevel validation so CSX won't complain that we're sleeping while holding a lock */
        csx_p_thr_disable_xlevel_validation();
        /*
         * Serialize access to the Msg mailbox
         */
        mClientLock->lock();
        
        assert_false(mService->get_flag(mtl_test_xml_msg));
        
        strcpy(getBuffer(), buffer);
        mService->set_flag(mtl_test_xml_msg, true);   // report
        
        mService->notify();
        mClientLock->wait();
        
        assert_false(mService->get_flag(mtl_test_xml_msg));
        assert_true(mService->get_flag(mtl_ctl_msg_done));
        
        mService->set_flag(mtl_ctl_msg_done, false);  // reset
        
        mClientLock->unlock();
        csx_p_thr_enable_xlevel_validation();
    }
}

void Mut_test_listener::start() {
    EMCPAL_STATUS status;
    status = EmcpalThreadCreate(EmcpalDriverGetCurrentClientObject(), &mTestThread_handle, "mutTestListenerThreadStart",&Mut_test_listener::threadStart,(void *) this);
    FF_ASSERT(status == EMCPAL_STATUS_SUCCESS);
}

void Mut_test_listener::threadStart(EMCPAL_TIMER_CONTEXT context) {
    Mut_test_listener *listener = (Mut_test_listener *)context;
    listener->run();
}


void Mut_test_listener::assert_true(BOOL b) {
    if(!b) {
        EmcpalDebugBreakPoint();
    }
}

void Mut_test_listener::assert_false(BOOL b) {
    if(b) {
        EmcpalDebugBreakPoint();
    }
}
