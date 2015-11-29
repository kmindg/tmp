/***************************************************************************
 * Copyright (C) EMC Corporation 2015
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
***************************************************************************/
#include "mut_cpp.h" 

class exampleSuite: public Mut_testsuite {
public:
    void suiteStartUp () {
        mut_printf(MUT_LOG_KTRACE, "suitestartup");
    }
    void suiteTearDown() {
        mut_printf(MUT_LOG_KTRACE, "suiteteardown");

    }
    exampleSuite(const char * n) : Mut_testsuite(n){};
    
};

class Test_subtraction: public CAbstractTest {
public:
    //! The main test function.
    /*!
      It specifies the test algoritm
      This function shall be overriden for each new test.
    */
    Test_subtraction(){
        set_short_description("subtraction");
        set_long_description("sub long description");
    }
    void Test()
    {
        MUT_ASSERT_INT_EQUAL(5-3 , 2);
    }
};

class Test_addition: public CAbstractTest {
public:  
     

    //! The main test function.
    /*!
      It specifies the test algoritm
      This function shall be overriden for each new test.
    */
    
    int *ma;
    int *mb;
    int *msum;

    Test_addition () : ma(NULL), mb(NULL), msum(NULL) {
        set_short_description("My short test_addition description");
        set_long_description( "My longer test addition description\n \t just a little longer");
    }
    void Test()
    {
       int i, j, k = 0;

       for(i = 0; i < 3; i++)
       {
          for(j = 0; j < 3; j++)
          {
            MUT_ASSERT_INT_EQUAL(ma[j] + mb[i], msum[k])
            k++;
          }
       } 
    
    }


    void StartUp()                                   
    {                                                
        // set things up, please pardon the verbosity
        //   for the sake of illustration              
                                                   
                                                     
        ma = new int[3];                             
        ma[0] = -1; ma[1] = 0; ma[2] = 1;            
                                                     
        mb = new int[3];                             
        mb[0] = -2; mb[1] = 0; mb[2] = 2;            
                                                     
        msum = new int[9];                           
        msum[0] = -3; msum[1] = -2; msum[2] = -1;    
        msum[3] = -1; msum[4] = 0 ; msum[5] = 1;     
        msum[6] = 1 ; msum[7] = 2 ; msum[8] = 3;     
    }                                                



    void TearDown()            
    {                          
        //take things apart 
        delete [] ma;          
        delete [] mb;          
        delete [] msum;        
    }                          
    
    

};

void mul_test(void){
    MUT_ASSERT_INT_NOT_EQUAL(2*3, 7);
}

int numerator;
int denominator;

void div_setup(void){
    numerator = 8;
    denominator = 4;
}

void div_test(void){
    MUT_ASSERT_INT_EQUAL(numerator/denominator, 2);
}

int main(int argc , char ** argv)
{
    
    mut_testsuite_t * add_suite;
    mut_testsuite_t * sub_suite;
    Mut_testsuite *cppsuite = (Mut_testsuite *)new exampleSuite("cpp");
    mut_init(argc, argv); 
    
    add_suite = mut_create_testsuite("additiontestsuite");
    sub_suite = mut_create_testsuite("subtestsuite");
    MUT_ADD_TEST_CLASS(add_suite, Test_addition);
    MUT_ADD_TEST_CLASS(sub_suite, Test_subtraction);
    MUT_ADD_TEST_WITH_DESCRIPTION(add_suite, mul_test, NULL, NULL, "mul_test_short desc", "mul_test_longer........desc");
    MUT_ADD_TEST(sub_suite, div_test, div_setup, NULL);
    MUT_ADD_SUITE_CLASS(cppsuite);

    MUT_ADD_TEST_CLASS(cppsuite, Test_addition);
    MUT_ADD_TEST_CLASS(cppsuite, Test_subtraction);
    MUT_ADD_TEST(cppsuite, div_test, div_setup, NULL);
    MUT_ADD_TEST_WITH_DESCRIPTION(cppsuite, mul_test, NULL, NULL, "mul_test_short desc in cppsuite", "mul_test_longer cppsuite........desc");


    MUT_RUN_TESTSUITE(add_suite);
    MUT_RUN_TESTSUITE(sub_suite);
    MUT_RUN_TESTSUITE(cppsuite);
    return 0;
}
