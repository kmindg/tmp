#ifndef MCR_SIM_TEST_H
#define MCR_SIM_TEST_H

#include "CAbstractTest.h"

//  root of all mcr simulation test
class McrSimTest: 
    public  CAbstractTest
{
    public:
        McrSimTest( void(*startup )(void), void(*teardown )(void) )         
        { 
            mstartup = startup;
            mteardown = teardown;
        }
    
        virtual void Test() = 0;
        void StartUp()
        {
            if(mstartup)
            {
                mstartup();
            }
        }
        void Teardown()
        {
            if(mteardown)
            {
                mteardown();
            }
        }
    private:
        void(*mstartup)(void);
        void(*mteardown)(void);
};
	

#endif//end MCR_SIM_TEST_H

