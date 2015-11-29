/*
 * defaulttest.h
 *
 *  Created on: Jan 12, 2012
 *      Author: londhn
 */
 
 #ifndef DEFAULTTEST_H
 #define DEFAULTTEST_H

 #include "mut/abstracttest.h"
 
class DefaultSuite;
 
enum Mut_test_status_e{
    MUT_TEST_PASSED             = 0,
	MUT_TEST_NOT_EXECUTED       = 1,
    MUT_TEST_RESULT_UKNOWN      = 2,
    MUT_TEST_FAILED             = 3,
	MUT_TEST_ERROR              = 4,
};

class DefaultTest:public AbstractTest{
public:
	DefaultTest(){};
    DefaultTest(const char* name, DefaultSuite *suite = 0, unsigned long timeout = 0):
				mTestName(name),
				mTimeout(timeout),
				mShortDescription(""),
				mLongDescription(""){};

    virtual void test(){};

    virtual void startUp(){};

    virtual void tearDown(){};

    int execute(){
    	startUp();
    	test();
    	tearDown();
    	return status; //assert call should set the test status
    }

    const char *   get_short_description(void){return mShortDescription;}

    const char *   get_long_description(void){return mLongDescription;};

    void set_short_description(const char * str){mShortDescription = str;}

    void set_long_description(const char * str){mLongDescription = str;};

	//replaces the getName() method update the code accordingly - note
	//delete note after
	const char *get_test_id(){return mTestName;}

	void set_test_status(enum Mut_test_status_e status){mStatus = status;}

	enum Mut_test_status_e get_test_status(){return mStatus;}

	bool statusFailed(){return !((get_test_status() == MUT_TEST_PASSED ||
		(get_test_status() == MUT_TEST_NOT_EXECUTED) || 
		(get_test_status() == MUT_TEST_RESULT_UKNOWN)));}

	static void reset_global_test_counter(){global_test_counter = 0;}

	static void inc_global_test_counter(){global_test_counter++;}

	static int get_global_test_counter(){return global_test_counter;}

	void set_test_index(int index){mTestIndex = index;}

	int get_test_index(){return mTestIndex;}

	void setTimeout(unsigned long timeout){mTimeout = timeout;}

	unsigned long getTimeout(){return mTimeout;}

private:
    //status of the test, 0 if successful and !0 if failed
    int status;//
	DefaultSuite *mSuite;
    const char *mTestName;
    const char *mShortDescription;
    const char *mLongDescription;
	enum Mut_test_status_e volatile mStatus;	
	static int global_test_counter;
	int mTestIndex;
	unsigned long mTimeout;
};

#endif /* DEFAULT_TEST_H */