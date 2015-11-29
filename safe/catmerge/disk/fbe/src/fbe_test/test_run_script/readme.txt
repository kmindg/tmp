fbe_test.py is a python script that does the following.
- It runs all fbe_test one by one.
- Support running multiple fbe_test.exe in parallel by specify command line options 
- Option to specify the tet number range by test number, name and test suite name
- Pass cmd options to fbe_test
- Options to run multiple interations of the same test with a test pass or fail.  



This script supports below options:

Usage:    python fbe_test.py [options] argument
Options:
    -h, -help          show this help message and exit
    -n, -num           set the number of fbe_test.exe to run
    -p, -port_base     set the port base of SP and CMI.
                       default port is 13000
    -t, -tests         set the testes to run
                       Specify tests by test number,name and test sutite name
                       Example: -t "1,10-20,suitename1,testname1,testname2"
    -i, -iterations    set interation to run
                       combine with rerun flag to rerun tests
                       Default value is 2.
    -r, -rerun_flag    set rerun_flag to run
                       valid arguments are PASSED or FAILED
                       Default value is FAILED
    -[fbe cmd options] pass options to fbe_test
                       valid arguments are any fbe_test cmd arguments
                       Example: -rdgen_peer_option PEER_ONLY
    -debugger          launch into the debugger when a test faults

Examples:
    print help message:
        python fbe_test.py -h or python fbe_test.py -help
    start 3 fbe_test:
        python fbe_test.py -n 3  or python fbe_test.py -num 3
    specify tests to run by test number, name and test suite name:
        python fbe_test.py  -t "13,27-29,sobo_4,fbe_cli_test_suite"
    specify 2 fbe_test options raid_test_level, and raid0_only:
        python fbe_test.py -raid_test_level 2 -raid0_only
    Rerun fbe_tests for 2 iterations while test is passed:
        python fbe_test.py -i 2 -r PASSED


System setup
	1. Install Python 2.6 or 2.7, DON'T install Python 3.x (One time operation)
	   dowoload software: http://www.python.org/download/
           Python 3.0 is a new version of the language that is incompatible with the 2.x line of releases,
           so don't install Python 3.x, see details: http://www.python.org/download/releases/3.0/
	2. Add python directory to the "path" environment variable of your computer
        3. Copy fbe_test.py to the directory where fbe_test.exe is located


Future enhancement(Not implemented):
	1.When SP hit an exception, generate a dump of fbe_test and peer SP. 
