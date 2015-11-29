#ifndef CABSTRACTTEST_H
#define CABSTRACTTEST_H

#include "mut.h"

/*! \class Test
 * The root class for any MUT test class
 *
 * \par The Test function implements the basic test functionality.
 * This function shall be overriden for each new test.
 */
class CAbstractTest
{
public:
    //! The main test function.
    /*!
      It specifies the test algoritm
      This function shall be overriden for each new test.
    */
    virtual void Test() = 0;

    //! Optional test function.
    /*!
      This function may be overriden if necessary.
      It specifies the startup logic of the test algorithm.
      Main test function will not be called if startup failed.
    */
    virtual void StartUp()
    {
    };

    //! Optional test function.
    /*!
      This function may be overriden if necessary.
      It specifies the teardown logic of the test algorithm.
      This function will be called even if main test function failed
    */
    virtual void TearDown()
    {
    };

    //! A test destructor
    /*!
      This function may be overriden if necessary
    */
    virtual ~CAbstractTest()
    {
    };

    CAbstractTest(){
        short_description = "<no description provided>";
        long_description = "<no description provided>";
    };

    const char * get_short_description(void) {
        return short_description;
    }
    
    const char * get_long_description(void) {
        return long_description;
    }
    void set_short_description(const char * str) {
        short_description = str;
    }
    
    void set_long_description(const char * str) {
        long_description = str;
    }

private:
    const char *short_description;
    const char *long_description;
    
};


// This class is used as a wrapper for tests that are defined in the 'C' manner, with discrete setup, test, teardown functions.
// In other words, for backward compatibility

class CTest: public CAbstractTest{
public:
    //! The main test function.
    /*!
      It specifies the test algoritm
      This function shall be overriden for each new test.
    */
    // 
    MUT_API CTest(void(*test)(void),  void(*startup)(void), void(*teardown)(void), const char *short_desc, const char *long_desc);
    
    void Test();
    
    //! Optional test function.
    /*!
      This function may be overriden if necessary.
      It specifies the startup logic of the test algorithm.
      Main test function will not be called if startup failed.
    */

    void StartUp();
    
    //! Optional test function.
    /*!
      This function may be overriden if necessary.
      It specifies the teardown logic of the test algorithm.
      This function will be called even if main test function failed
    */
    void TearDown();
    
   
private:
    // pointers to the C routines that the user provides.
    void (*cStartUp)(void);
    void (*cTest)(void);
    void (*cTearDown)(void);
};

#endif
