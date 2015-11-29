Main.Automatos Functional Testing

#######################################
Setting Up an Automatos Environment
#######################################


Stream
------

The official stream for Automatos Framework promotion is ctt-Apollo-int in the ctt depot. A user collaboration stream ctt-hengs1-mcrtest-usr has been created off of ctt-Apollo-int for MCR functional test development. 


Environment
-----------

Install the required version of Perl. Please see the Automatos Software Requirements. 
https://usd.wiki.emc.com/xwiki/bin/view/Automatos-Dev/Software%20Requirements

This version of perl needs to coexist with the version of perl used for our build tools. Install Perl to a different directory, i.e. C:\perl516

When executing the automatos script it is required that the environment of the command window is using the automatos perl version. This can be done by setting the path such as "set PATH=C:\perl516\bin" in the command window

Set up ‘PERL5LIB’ paths

    Note: Change paths appropriately for Automatos workspace

D:\Automatos\Automatos

D:\Automatos\Automatos\Framework\Dev\bin

D:\Automatos\Automatos\Framework\Dev\lib

D:\Automatos\Automatos\Tests\Dev

D:\Automatos\Automatos\UtilityLibraries\Dev

D:\Automatos\Automatos\Framework\Dev

D:\views\starburst\safe\catmerge\disk\fbe\src\fbe_test\automatos 

The location of the tests in the upc stream also needs to be included in the perl5lib path. 

If the workspace is likely to change, set the path in the command prompt

i.e. "set PERL5LIB=%PERL5LIB%;D:\views\starburst\safe\catmerge\disk\fbe\src\fbe_test\automatos" 



Run ppminstall.pl in the Automatos/Framework/Dev/bin folder to set up additional modules required for automatos. 

The location of the tests in the upc stream also needs to be included in the perl5lib path. 

If the workspace is likely to change, set the path in the command prompt

i.e. "set PERL5LIB=%PERL5LIB%;D:\views\starburst\safe\catmerge\disk\fbe\src\fbe_test\automatos" 



Alternative: automatos_mcr.pl
-----------------------------

This script will set the perl path, add the workspace to the PERL5LIB environment, variable. 

1. create an environment variable called AUTOMATOSPERL and set it to the perl bin directory required for automatos

2. create an environment variable called AUTOMATOS and set it to the Automatos bin directory. 

i.e. D:\Automatos\Automatos;D:\Automatos\Automatos\Framework\Dev\bin

3. add only the automatos directories above to the PERL5LIB environment variable

4. execute automatos through automatos_mcr.pl from the catmerge\disk\fbe\src\fbe_test\automatos directory. 


Location of MCR Functional Tests
--------------------------------

MCR functional Tests are located in catmerge\disk\fbe\src\fbe_test\automatos\MCR_Test\


#######################################
Executing an Automatos Test Set
#######################################

<Path to Automatos executable> --configFile <Path to config file>

Example:

C:\>perl C:\Automatos\Automatos\Framework\Dev\bin\automatos.pl --configFile C:\Automatos\Automatos\Tests\Dev\MCR_Test\Promotion\DZ\DZ_ConfigFile.xml

In the works...

It may be beneficial to be able to use specify a path or use an environment variable in the config file. This is currently under code review by the automatos framework team. 

replace %WORKSPACE% in configfile.xml with either --workspace option or WORKSPACE environment variable.

#######################################
UTMS
#######################################

Test case results are tracked in UTMS. The location of current MCR test cases will be located in the USD-> Rockies Project. The test sets are located under Test Lab -> MCR_Functional_Regression

UTMS Automatic Reporting through Automatos
------------------------------------------

The results of the test cases can be placed into UTMS. Please see the requirements in UTMS Reporting. 
https://usd.wiki.emc.com/xwiki/bin/view/Automatos-X/UTMS%20Reporting

The following utms xml nodes must be placed in the Test Set File under <test_set name="required"> 

        <identities>

          <identity name="utms_domain" value="USD" /> 

          <identity name="utms_project" value="ROCKIES" /> 

          <identity name="utms_team_name" value="Platform EFT" /> 

          <identity name="utms_cycle_id" value="110214" /> 

          <identity name="utms_user_email" value="socheavy.heng@emc.com" /> 

          <identity name="utms_address" value="utmswebserver01:8082" /> 

      </identities>

Example Test Set File
---------------------
\catmerge\disk\fbe\src\fbe_test\automatos\MCR_Test\Promotion\DZ\DZ_TSInfo.xml

#######################################
Helpful Links 
#######################################

https://usd.wiki.emc.com/xwiki/bin/view/Automatos/Contributing
https://usd.wiki.emc.com/xwiki/bin/view/Automatos-Dev/OnlineTraining
https://usd.wiki.emc.com/xwiki/bin/view/Automatos/
https://usd.wiki.emc.com/xwiki/bin/view/Automatos/XML%20RPC