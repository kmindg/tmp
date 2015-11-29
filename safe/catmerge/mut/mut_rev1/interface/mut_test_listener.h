/***************************************************************************
 *                                mut_test_listener.cpp
 ***************************************************************************
 *
 * DESCRIPTION: MUT test listener definition
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *
 **************************************************************************/
#ifndef __MUTPROCESSTESTLISTNER__
#define __MUTPROCESSTESTLISTNER__

# include "EmcPAL.h"
# include "EmcPAL_DriverShell.h"
# include "EmcPAL_Misc.h"
# include "K10Assert.h" 
# include "simulation/shmem_class.h"
# include "generic_types.h"
# include "mut_test_entry.h"
# include "mut_test_control.h"
# include "mut_abstract_log_listener.h"
# include "mut_sdk.h"

/*
 * Flag bits for control/status
 */
typedef enum mtl_flags_e {
    mtl_ctl_initialized = 0,
    mtl_ctl_enabled     = 1,
    mtl_ctl_exit        = 2,
    mtl_ctl_done        = 3,
    mtl_test_started    = 4,
    mtl_test_done       = 5,
    mtl_test_txt_msg    = 6,
    mtl_test_xml_msg    = 7,
    mtl_ctl_msg_done    = 8,
} mtl_flags_t;

#define MTL_BUFFER_LEN 1024
typedef struct mtl_data_s {
    enum Mut_test_status_e status;
    char buffer[MTL_BUFFER_LEN];
} mtl_data_t;

class Mut_test_control;

class Mut_test_listener: public Mut_abstract_log_listener {
public:
    Mut_test_listener(Mut_test_control *control);
    virtual ~Mut_test_listener();

    enum Mut_test_status_e getTestStatus();
    void    setTestStatus(enum Mut_test_status_e status);

    char *getBuffer();

    void run();
    void start();

    void send_text_msg(const char *buffer);
    void send_xml_msg(const char *buffer);

private:

    void assert_true(BOOL b);
    void assert_false(BOOL b);

    static void threadStart(EMCPAL_TIMER_CONTEXT context);

    Mut_test_control    *mControl;
    shmem_segment       *mSegment;
    shmem_service       *mService;    // Data Service & Consumer thread waits on this services lock
    shmem_service       *mClientLock; // producers wait on mClientLock
    mtl_data_t          *mTestData;
    BOOL                mListenerRunning;
    EMCPAL_THREAD       mTestThread_handle; // Emcpal thread handle
};
#endif
