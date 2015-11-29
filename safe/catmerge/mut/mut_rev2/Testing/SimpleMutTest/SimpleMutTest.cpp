/* SimpleMutTest.cpp
 *
 *  Created on: Sep 21, 2011
 *      Author: londhn
 */

#include "mut_cpp.h"
#include "mut.h"
#include "mut_assert.h"
#include <stdio.h>
#include <iostream>

#include "mut/defaulttest.h"
#include "mut/defaultsuite.h"
#include "simulation/arguments.h"
#include "EmcPAL_Misc.h"
#include "EmcUTIL_CsxShell_Interface.h"

#include "windows.h"

using namespace std;

static char *arguments[] = 
{
	"-argumentString",
	"valueArg1",
	"-argumentStringCsv",
	"string1,string2,string3",
	"-booleanArgument",
	"-argumentInt",
	"600",
	"",
	NULL
};

class TestSubtraction: public DefaultTest {
public:
	TestSubtraction(const char* name):DefaultTest(name){};

    void test()
    {
		MUT_ASSERT_EQUAL(5-3 , 2);
    }
};

class TestAddition: public DefaultTest {
public:

    int *ma;
    int *mb;
    int *msum;

	TestAddition(const char* name):DefaultTest(name){};

    void test()
    {
       int i, j, k = 0;

       for(i = 0; i < 3; i++){
    	   for(j = 0; j < 3; j++){
    		   MUT_ASSERT_EQUAL(ma[j] + mb[i], msum[k]);
    		   k++;
    	   }
       }

    }

    void startUp()
    {
		ma = new int[3];
        ma[0] = -1; ma[1] = 0; ma[2] = 1;

        mb = new int[3];
        mb[0] = -2; mb[1] = 0; mb[2] = 2;

        msum = new int[9];
        msum[0] = -3; msum[1] = -2; msum[2] = -1;
        msum[3] = -1; msum[4] = 0 ; msum[5] = 1;
        msum[6] = 1 ; msum[7] = 2 ; msum[8] = 3;
    }

    void tearDown()
    {
        //take things apart
        delete [] ma;
        delete [] mb;
        delete [] msum;
    }

};

class ArgumentsTestCase: public DefaultTest
{
public:
	int argumentCount;
	char **argumentValues;
	char **temp; 

	ArgumentsTestCase(const char* name):DefaultTest(name){};

	void startUp(){
		argumentCount = Arguments::getInstance()->getArgc();
		argumentValues = Arguments::getInstance()->getArgv();
		temp = (char **)malloc(2*sizeof(char *));
	}

	void test(){
		Arguments::getInstance()->getCli("-abort_policy",temp);
		
		MUT_ASSERT_NOT_NULL(temp);
			
		temp++;

		MUT_ASSERT_EQUAL(*temp,"debug");
	}

	void tearDown(){
		argumentCount = 0;
		argumentValues = NULL;
	}
};

class TestTimeout:public DefaultTest{
public:
	TestTimeout(const char *name):DefaultTest(name){};

	void test(){
		mut_printf("Control sleeping for 6 seconds");
		Sleep(6000);
	}
};

class UnitTestsSuite:public DefaultSuite
{
public:
	UnitTestsSuite(const char* name):DefaultSuite(name){};
	
	// care should be taken to add the timeout to every test if required
	// otherwise default value of 600 will be used
	void populate(){
		addTest(new TestAddition("Test_Addition"));
		addTest(new TestSubtraction("Test_Subtraction"));
		addTest(new ArgumentsTestCase("Test_Arguments"));
		addTest(new TestTimeout("Test_Timeout"),3);
	}
};

int __cdecl main(int argc , char ** argv)
{
	Arguments::init(argc,argv);

	Emcutil_Shell_MainApp theApp;

	mut_init(argc,argv);
	DefaultSuite *unitTestsSuite = new UnitTestsSuite("UnitTests_Suite");
	return unitTestsSuite->execute();
}
