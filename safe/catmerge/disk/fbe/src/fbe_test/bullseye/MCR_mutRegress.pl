#!/usr/bin/perl
#***************************************************************************
#  Copyright (C) EMC Corporation 2014
# Licensed material -- property of EMC Corporation
#***************************************************************************
use strict;
use Cwd;
use Getopt::Long;
use concurrent;
use TestLogger;
use Bullseye;

use File::Path;
use File::Copy;
use File::Spec;
use File::Basename;
use Cwd;
use IO::File;

use VersionControlUtil;
use Net::Ping;
use systeminfo;
use JSON;

# mutRegress.pl is built to handle the old style TEST_LIST format or the new TEST_LIST JSON format.   The decision
# on which is used is based on whether or not TEST_LIST.JSON is found in the configDir directory.  If it is found
# we will use the TEST_LIST.JSON file, if not we will look for the TEST_LIST file.
#
# The TEST_LIST.JSON file describes the tests that will be executed.  The file is assumed to live in the directory 
# which was specified with the -configDir input parameter.  The file has the following format:
#
# {"project": "mcc", "file": "mcc_tests.JSON"}
# {"project": "fct", "file": "fct_tests.JSON"}
#
#  each field is as follows:
#
#  project: the name of the project to test.   This name must match the name in the associated test file.  REQUIRED
#
#  file:  the name of the JSON file containing the tests to be executed.   The file is assumed to live in the
#         directory which was specified with the -configDir input parameter.  REQUIRED.
#
#
# The file specified in the TEST_LIST.JSON file as mentioned above describes the tests that will be run for the
# specified project.  Each line in the file will have the following format:
#
# {"project": "mcc", "framework": "MUT", "testType": ["Regress", "Validation"], "execute": "parallel", "test": "CacheVault07Test.exe", "args":, "arguments for test"}
#
# The arg, testType, and execution options are optional but should be defined.
#
# each field is as follows:
#
# project: the name of the project that this test is part of. the -runproject command line parameter allows the
#          caller the ability to only run tests belonging to a specific project.  REQUIRED
#
# framework: framework the test is based on.  MUT is the only supported value.   REQUIRED
#
# testType: indicates how this test will be used.  We have 3 options, Regress, Validation, Performance. A test
#            can be a member of one or more test types.  Their meaning
#            is as follows:
#
#            Regress - use by MCC developers to ensure that there are no regressions in MCC validates all features in MCC.
#
#            Validation - used by other testers to ensure that MCC still works.  No new features have been added to MCC so this
#              test is just ensuring that they have done nothing in their code to break MCC.
#
#            Performance - used by MCC developers to see if there is any performance degradation in the simulation of MCC
#
#            OPTIONAL, but preferred if not specified, will run always
#
# execute: execute indicates whether the test should be run in parallel with other tests or serially.  The default is parallel.
#          serial tests run one at a time and are executed after the parallel tests.   OPTIONAL but preferred.
#
# test: the name of the test to be run.   This is case sensitive.   The test is assumed to reside in /Targets/xxxx/simulation/exec
#       where xxxx is the tagname from the build, e.g. armada64_checked or armada64_free
#
# args: this contains any command line arguments that the test supports.   mutRegress.pl adds -isolate and -verbosity suite by
#       default to the tests that are run.  OPTIONAL.

###################################################
#                                         Globals                                                #
###################################################
my %argHash;
my $logger = TestLogger->new();
my $bullseye_config = Bullseye->new($logger);
my $starting_time = time;
my $run_time_stamp = time_stamp($starting_time);
my $vcUtil;
my $jobs = 0;
my $job = 0;
my $totalTestsExecuted = 0;
my $totalTestFailures = 0;

my $database;
my $recommended_processors = 8;
my $recommended_memory = 11264; # 11 GB => Keep a buffer, Recommended: 12GB = 12582912 MB

sub usage {
    print "USAGE: $0 [options]\n";
    print " -vcType (version control system type)\n";
    print " -view view (path to or name of view)\n";
    print " -extraCli args   extra cli arguments passed to test programs\n";
    print " -tagname target  (default=armada64_checked)\n";
    print " -worker n (number of worker threads) default = 8\n";  
    print " -iterations n (number of times to execute tests) default 1\n";
    print " -mode mode  (default=simulation)\n";
    print " -verbosity level (default=test)\n";
    print " -build (run build.pl first)\n";
    print " -buildOnly (run build.pl then exit)\n";
    print " -bullseye (perform a Bullseye build and perform Code Coverage analysis)\n";
    print " -text generate text logs during execution\n";
    print " -configDir path (relative path in view)\n";
    print " -list path (test list config file) default = configDir/test_list.JSON\n";
    print " -exclude list (comma separated list of programs to exclude from execution)\n";
    print " -template path (Bullseye template) default = configDir/bullseye_tempalte.cfg\n";
    print " -verbose (turn on additional logging)\n";
    print " -cleanview (undo checkouts/modified. force option is set. May use -update)\n";
    print " -updateview (update view. force option is set. May use -update)\n";
    print " -branch branch_name (branch to which view is based on) \n";
    print " -resultsRootprojects project names (separated by comma) \n";
    print " -suites suite names (separated by comma) \n";
    print " -help (display this message)\n";
    print " -update updateID (updateID to which view should be updated)\n";
    print " -VOB vob (default environment variable VOB, if set, or catmerge)\n";
    print " -select (select tests to be executed based on changed elements and Bullseye DBMS data)\n";
    print " -processors_nocheck (Use this switch to over-ride the recommended processors requirement)\n";
    print " -runproject name (name of project whose tests are to be run, all projects run by default)\n";
    print " -memory_nocheck (Use this switch to over-ride recommended memory requirement)\n";
    print " -testType test type to be run Regress, Validation, Performance.  Regress is the default. \n";
}

###################################################
#                                       Functions                                              #
###################################################

#
# Create new bullseye_template_new.cfg file in
# target directory from input one, and correct
# VOB information inside if necessary.
#
sub generateNewTemplate {
    my ($path, $project) = @_;
    my $newName  = "bullseye_template_$project.cfg";
    my $newCfg   = File::Spec->catdir($argHash{'target'}, "$newName");
    my $template = File::Spec->catdir($argHash{'catmerge'}, $path, $argHash{'template'});

    if(!-e $argHash{'target'}) {
        mkdir $argHash{'target'} or die "Failed to mkdir $argHash{'target'}";
    }

    open(LIST, "$template") or die("Unable to open $template\n");
    # Create new correct config file with project name suffix
    open(CFG, ">$newCfg") or die("unable to open $newCfg\n");


    while(<LIST>) {
        # Replace with correct VOB and change '\' to'/'
        s/ catmerge\\/ $argHash{'VOB'}\\/g;
        s/ catmerge\// $argHash{'VOB'}\//g;
        printf CFG $_;
    }

    close(LIST);
    close(CFG);

    return $newName;
}

#
# Report test schedule
#
sub reportTestSchedule {
    my ($cmdRef, $testtype) = @_;
    my $cmdRef = shift;
    $logger->report("Scheduling $testtype: $_") foreach (@$cmdRef);
}

#
# Report test execution results
#
sub reportTestResult {
    my ($exe_parallel, $testListHashRef) = @_;
    my @durations;

    $logger->report("\n\nSimulation tests status");
    my ($noFailures, @results) = $exe_parallel->getStatus(1);
    $logger->report(scalar(@results)." tests executed");
    $totalTestsExecuted += scalar(@results);
    $logger->report("$noFailures failures");
    foreach (@results) {
        # Extract test execution time
        $logger->report($_);
        $_ =~ /elapsed\s+time\s+0*(\d+)/;
        $1 = 0  if ( not defined $1 or $1 eq '""' );
        push @durations, $1;
    }
    $testListHashRef->{durationList} = \@durations;
    $totalTestFailures += $noFailures;
    return $noFailures;
}

#
# Determine if we are using the old list TEST_LIST or new list
# TEST_LIST.JSON
#
# return of 1 means TEST_LIST.JSON found
# return of 0 means TEST_LIST
#
sub determineListType {
    my $pathRef = shift;

    my $configFile = $argHash{'list'};
    
    if(-e $configFile) {
        print "using $configFile\n";
        return 1;
    }
    else {
        $configFile = File::Spec->catdir($argHash{'catmerge'}, @$pathRef, $configFile);
        if(-e $configFile) {
            print "using $configFile\n";
            return 1;
        }
    }

    my $configFile = $argHash{'listold'};
    
    if(-e $configFile) {
        print "using $configFile\n";
        return 0;
    }
    else {
        $configFile = File::Spec->catdir($argHash{'catmerge'}, @$pathRef, $configFile);
        if(-e $configFile) {
        print "using $configFile\n";
            return 0;
        }
    }
    
    print "using $configFile\n";
    return 0;
}

#
# Read project names from test config files specified by input config directories.
# we expect the file to contain JSON objects.  Following the format:
# {"project": "mcc", "file": "mcc_tests.JSON"}
# {"project": "fct", "file": "fct_tests.JSON"}
#
sub getProjects {
    my $pathRef = shift;
    my @projectList;
    
    foreach (@$pathRef) {
        my $configFile = $argHash{'list'};
        
        if(-e $configFile) {
            open(TLIST, $configFile) or die("Unable to open $configFile\n");
        }
        else {
            $configFile = File::Spec->catdir($argHash{'catmerge'}, $_, $configFile);
            open(TLIST, $configFile) or die("Unable to open $configFile\n");
        }
        
        my @contents = <TLIST>;
        my $project = 'default'; # TODO: now we give default name to project

        close(TLIST);

        chomp(@contents);

        foreach my $line (@contents) {
        
            # if the line does is not enclosed in braces e.g. "{....}" then skip it
            if(!($line =~  /^[{](.+)[}]$/)) {
                next;
            }

            # Decode the line 
            $JSON::json_hash = JSON::decode_json($line);
        
            # determine if project is defined in the line.
            if(defined $JSON::json_hash->{project}) {
                
                # Get the project name
                $project = $JSON::json_hash->{project};
                
                #  Validate that a file was defined and if it exists.
                die "cannot find file for $project!" unless defined $JSON::json_hash->{file};
                $JSON::projectfile = $JSON::json_hash->{file};
                $JSON::jsonfile = File::Spec->catdir($argHash{'catmerge'}, $_, $JSON::projectfile);
                if(! -e $JSON::jsonfile)  {
                    die "Unable to find project file $JSON::jsonfile\n";
                }
                
                # Save the project and projectfile.
                push @projectList, $project;
            }
        }
        die "Cannot find project name for $configFile!" unless defined $project;
    }

    return @projectList;
}

#
# Read project names from test config files specified by input config directories.
#
sub getProjectsOld {
    my $pathRef = shift;
    my @projectList;
    
    foreach (@$pathRef) {
        my $configFile = $argHash{'listold'};
        
        if(-e $configFile) {
            open(TLIST, $configFile) or die("Unable to open $configFile\n");
        }
        else {
            $configFile = File::Spec->catdir($argHash{'catmerge'}, $_, $configFile);
            open(TLIST, $configFile) or die("Unable to open $configFile\n");
        }
        
        my @contents = <TLIST>;
        my $project = 'default'; # TODO: now we give default name to project

        close(TLIST);

        chomp(@contents);

        foreach my $line (@contents) {
            # Read the first name wrappered between '<>' or '# project:name' as project name
            if($line =~ /<(\S+)>/) {
                # Read project name
                $project = $1;
                push @projectList, $project;
                last;
            } elsif($line =~ /#\s*project\s*:\s*(\S+)/) {
                # Read project name
                $project = $1;
                push @projectList, $project;
                last;
            }
        }
        die "Cannot find project name for $configFile!" unless defined $project;
    }
    return @projectList;
}

# Look at the input parameters and determine whether or not processing will
# continue.
sub continueProcessing {
    my ($runProjectSettings, $inproject, $fproject) = @_;
    my $continueExecution = 1;
    
    # See if we are doing a project specific run
    if($runProjectSettings == 0) {
        # We are doing a project specific run, see if we are
        # in the project to process yet.
        if($inproject == 0) {
            # We are currently not processing our project, see
            # if the new one we have found is the one we are looking for.
            if($argHash{'runproject'} eq $fproject) {
                # Yippee we have found our project, so mark inproject to
                # 1 indicating that processing is to proceed.
                $inproject = 1;
            }
        }
        else {
            # We were in the project and now we are not, so make inproject
            # to 0 to indicate the project is done.
            $inproject = 0;
        }
    }
    
    return $inproject;
}

#
# Read one or more test config locations, parse each and return test information which
# has been filtered by user exclusion specified by '-exclude'
#
sub getTestList {
    my $configRef = shift;
    my $test;
    my @configs = @$configRef; # Relative test config dir path to 'catmerge/'
    
    # Read user exclusions
    my %excluded;
    if(defined $argHash{'exclude'}) {
        # turn comma separate string list into a hash
        my @excludes = split(",", $argHash{'exclude'});
        foreach (@excludes) {
            $test = lc($_);
            # Remove extension
            if($test =~ /(\S+)\./) {
                $excluded{$1} = $1;
            } else {
                $excluded{$test} = $test;
            }
        }
    }
    
    my (@parallelList, @parallelIdxList, @serialList, @serialIdxList, @suiteList, @testNameList, @testNumList, @projectList, @configList);
    my $parallelExecution = 1;
    my $serialExecution = 0;
    my $testIdx = 0;
    
    # For each config location found we will open its JSON file and process it 
    foreach my $num (0..$#configs) {
        my $configFile = $argHash{'list'};
        if(-e $configFile) {
            open(TLIST, $configFile) or die("Unable to open $configFile\n");
        }
        else {
            $configFile = "$argHash{'catmerge'}/$configs[$num]/$configFile";
            open(TLIST, $configFile) or die("Unable to open $configFile\n");
        }

        my @contents = <TLIST>;
        my $project = 'default'; # TODO: now we give default name to project

        close(TLIST);

        chomp(@contents);

        # $runProjectSettings definition
        #    0 run specific project found in file.
        #    1 run all projects found in file.
        my $runProjectSettings = 1; 
        my $inproject = 1; # indicates we are processing a project
        
        # see if we are running all projects, which is the default.  If not we 
        # are looking for a particular project to run.
        if(defined $argHash{'runproject'} && $argHash{'runproject'} ne 'all') {
            # we are not running all projects found in the
            # file we are running a specified project.
            $runProjectSettings = 0;
            $inproject = 0;
        }
        
        # We have the configuration file open.   We must now look at each line
        # to see what we will process.
        foreach my $line (@contents) {

            # if the line does is not enclosed in braces e.g. "{....}" then skip it
            if(!($line =~  /^[{](.+)[}]$/)) {
                next;
            }
                    
            # Decode the line 
            $JSON::json_hash = JSON::decode_json($line);
        
            # determine if project is defined in the line.
            if(defined $JSON::json_hash->{project}) {
                
                $inproject = continueProcessing($runProjectSettings, $inproject, $JSON::json_hash->{project});
                if($inproject == 0) {
                    # Not processing line, skip to next
                    next;
                }

                # Get the project name
                $project = $JSON::json_hash->{project};

                #  Validate that a file was defined and if it exists.
                die "cannot find file for $project!" unless defined $JSON::json_hash->{file};
                
                $JSON::projectfile = $JSON::json_hash->{file};
                $JSON::jsonfile = "$argHash{'catmerge'}/$configs[$num]/$JSON::projectfile";
                
                if(! -e $JSON::jsonfile)  {
                    die "Unable to find project file\n";
                }

                # We now have the file open for one of the tests we are going to process.
                open(TLIST, $JSON::jsonfile ) or die("Unable to open $JSON::jsonfile\n");

                my @testfilecontents = <TLIST>;

                close(TLIST);

                chomp(@testfilecontents);
                
                # we have opened up the file that contains the list of JSON objects that
                # describe tests to be executed.   We need to go through the file one line
                # at a time and process each line.
                foreach my $testfileline (@testfilecontents) {
                    # initialize some defaults
                    $serialExecution = 0;
                    $parallelExecution = 0;

                    # if the line does is not enclosed in braces e.g. "{....}" then skip it
                    if(!($testfileline =~  /^[{](.+)[}]$/)) {
                        next;
                    }
                    
                    
                    # Decode the line 
                    $JSON::testfile_line_hash = JSON::decode_json($testfileline);
                
                    # determine if project is defined in the line.
                    if(! defined $JSON::testfile_line_hash->{project} ||
                       ! defined $JSON::testfile_line_hash->{framework} ||
                       ! defined $JSON::testfile_line_hash->{execute} ||
                       ! defined $JSON::testfile_line_hash->{test}) {
                       
                       die("necessary fields missing in $testfileline file $JSON::jsonfile\n");
                    }
                    
                    # make sure that this line is of the project we are processing.   If not report
                    # error.
                    if($JSON::testfile_line_hash->{project} ne $JSON::json_hash->{project}) {
                        die("$JSON::testfile_line_hash->{project} != $JSON::json_hash->{project} in $JSON::projectfile\n");
                    }
                    
                    if($JSON::testfile_line_hash->{framework} ne "MUT") {
                        die("$JSON::testfile_line_hash->{framework} != MUT in $JSON::projectfile\n");
                    }

                    # Make sure that the test exists.
                    my $test = $JSON::testfile_line_hash->{test};

                    if ($test =~ /^\-(\S+)\W+(.*)/) {
                        # Pass
                    } elsif (! -e $test)  {
                        print "Unable to find executable $JSON::testfile_line_hash->{test} in $JSON::projectfile\n";
                        next;
                    }
                    
                    # Exclude executable, specified in CLI
                    if(defined $excluded{lc($JSON::testfile_line_hash->{test})}) {
                        my $exclude_name = $JSON::testfile_line_hash->{test};
                        $logger->report("Excluding test $exclude_name from regression for user exclusion.");
                    }
                    
                    # See if test type was defined for the test.   If not we execute everything.
                    if(defined $JSON::testfile_line_hash->{testType}) {
                         my $testTypeFound = 0;
                         # Go thru the list of defined tests and see if this testtype
                         # is for this test.
                         my $test = $JSON::testfile_line_hash->{testType};
                         foreach my $testentry (@$test) {
                            if($testentry eq $argHash{'testType'}) {
                                # The user wants to execute this test
                                $testTypeFound = 1;
                                last;
                            }
                         }
                         if($testTypeFound eq 0) {
                             next;
                         }
                    }

                    # determine what type of execution is required
                    if($JSON::testfile_line_hash->{execute} eq "parallel") {
                        # This is a test that can be run in parallel with other tests.
                        $serialExecution = 0;
                        $parallelExecution = 1;
                    } elsif($JSON::testfile_line_hash->{execute} eq "serial") {
                        # This is a test that must be run serially
                        $serialExecution = 1;
                        $parallelExecution = 0;
                    } else {
                        # We don't know what this is, that is bad...
                        die("$JSON::testfile_line_hash->{test} not valid in $JSON::projectfile\n");
                    }
                    
                    my $testtorun = $JSON::testfile_line_hash->{test};

                    # see if this object has any arguments.
                    if(defined $JSON::testfile_line_hash->{args}) {
                        $testtorun = join "", $JSON::testfile_line_hash->{test}, " " , $JSON::testfile_line_hash->{args};
                    }
                    
                    # Remove extension from the name for the suite and testname lists.
                    my $execName;
                    if($testtorun =~ /(\S+)\./) {
                        $execName = $1;
                    } else {
                        $execName = $testtorun;
                    }

                    # Here are passed tests
                    if($parallelExecution) {
                        push @parallelList, $testtorun;
                        push @parallelIdxList, $testIdx;
                    } 
                    if($serialExecution) {
                        push @serialList, $testtorun;
                        push @serialIdxList, $testIdx;
                    }
                    # The test index keeps track of the tests order in
                    # the list of processed tests.   We need this because we
                    # move tests into the serial or parallel test list and
                    # we need to relate the tests in those lists back to
                    # the base test list.
                    $testIdx++;
                    
                    push @suiteList, $execName;
                    push @testNameList, $execName;
                    push @testNumList, -1;
                    push @projectList, $project;
                    push @configList, $configs[$num];
                }
            }
        }
    }

    return {parallelList => \@parallelList,
            serialList => \@serialList,
            suiteList => \@suiteList,
            testNameList => \@testNameList,
            testNumList => \@testNumList,
            projectList => \@projectList,
            configList => \@configList,
            parallelIdxLst => \@parallelIdxList,
            serialIdxLst => \@serialIdxList,
        };
}

#
# Parse input test configuration file line for test information and CLI for concurrent execution.
#
sub processOldLine {
    my $originLine = shift;
    
    # Extract executable name without extension
    if($originLine =~ /(\S+)\./) {
        return {cmdList => [$originLine],
            suiteList => [$1],
            testNameList => [$1],
            testNumList => [-1],
            exeList => [$1]};
    }
    
}
# Read one or more test config locations, parse each and return test information which
# has been filtered by user exclusion specified by '-exclude'
#
sub getTestListOld {
    my $configRef = shift;
    my $test;
    my @configs = @$configRef; # Relative test config dir path to 'catmerge/'
    
    # Read user exclusions
    my %excluded;
    if(defined $argHash{'exclude'}) {
        # turn comma separate string list into a hash
        my @excludes = split(",", $argHash{'exclude'});
        foreach (@excludes) {
            $test = lc($_);
            # Remove extension
            if($test =~ /(\S+)\./) {
                $excluded{$1} = $1;
            } else {
                $excluded{$test} = $test;
            }
        }
    }
    
    my (@parallelList, @parallelIdxList, @serialList, @serialIdxList, @suiteList, @testNameList, @testNumList, @projectList, @configList);
    my $parallelExecution = 1;
    my $serialExecution = 0;
    my $testIdx = 0;
    
    # Each config location
    foreach my $num (0..$#configs) {
        my $configFile = $argHash{'listold'};
        if(-e $configFile) {
            open(TLIST, $configFile) or die("Unable to open $configFile\n");
        }
        else {
            $configFile = "$argHash{'catmerge'}/$configs[$num]/$configFile";
            open(TLIST, $configFile) or die("Unable to open $configFile\n");
        }

        my @contents = <TLIST>;
        my $project = 'default'; # TODO: now we give default name to project

        close(TLIST);

        chomp(@contents);

        # $runProjectSettings definition
        #    0 run specific project found in file.
        #    1 run all projects found in file.
        my $runProjectSettings = 1; 
        my $inproject = 1; # indicates we are processing a project
        
        # see if we are running all projects
        if(defined $argHash{'runproject'} && $argHash{'runproject'} ne 'all') {
            # we are not running all projects found in the
            # file we are running a specified project.
            $runProjectSettings = 0;
            $inproject = 0;
        }
        
        foreach my $line (@contents) {
        
            if($line =~ /<(\S+)>/) {
                # See if we are doing a project specific run
                $inproject = continueProcessing($runProjectSettings, $inproject, $1);
                if($inproject == 0) {
                    # Not processing line, skip to next
                    next;
                }
                # Read project name
                $project = $1;
                $argHash{'project'} = $project;
                next;
            } elsif($line =~ /#\s*project\s*:\s*(\S+)/) {
                # See if we are doing a project specific run
                $inproject = continueProcessing($runProjectSettings, $inproject, $1);
                if($inproject == 0) {
                    # Not processing line, skip to next
                    next;
                }
                # Read project name
                $project = $1;
                $argHash{'project'} = $project;
                next;
            } elsif($line =~ /\[ParallelExecution\]/) {
                # Check to see if we are parsing a project.  If not, we'll skip this line.
                if($inproject == 0) {
                    # Not processing line, skip to next
                    next;
                }
                die "Cannot find project name!" unless defined $project;
                $serialExecution = 0;
                $parallelExecution = 1;
                next;
            } elsif($line =~ /\[SerialExecution\]/) {
                # Check to see if we are parsing a project.  If not, we'll skip this line.
                if($inproject == 0) {
                    # Not processing line, skip to next
                    next;
                }
                die "Cannot find project name!" unless defined $project;
                $serialExecution = 1;
                $parallelExecution = 0;
                next;
            } elsif(!($line =~ m/^#/) &&  !($line =~ m/^\s*$/)) {
                # Check to see if we are parsing a project.  If not, we'll skip this line.
                if($inproject == 0) {
                    # Not processing line, skip to next
                    next;
                }
                die "Cannot find project name!" unless defined $project;
                # Process to get test information line by line.
                # Break down CLI if the executable contains multiple tests.
                my $testInfoHash = processOldLine($line);
                foreach (0..scalar(@{$testInfoHash->{cmdList}}) - 1) {
                    # Exclude executable, suite, test exe::test or suite::test specified in CLI
                    if(defined $excluded{lc($testInfoHash->{exeList}->[$_])} || defined $excluded{lc($testInfoHash->{suiteList}->[$_]).'::'.lc($testInfoHash->{testNameList}->[$_])} 
                    || defined $excluded{lc($testInfoHash->{suiteList}->[$_])} || defined $excluded{lc($testInfoHash->{testNameList}->[$_])}
                    || defined $excluded{lc($testInfoHash->{exeList}->[$_]).'::'.lc($testInfoHash->{testNameList}->[$_])}) {
                        my $exclude_name = $testInfoHash->{suiteList}->[$_];
                        $logger->report("Excluding test $exclude_name from regression for user exclusion.");
                    } else {
                        my $execName;
                        # Remove extension
                        if($testInfoHash->{cmdList}->[$_] =~ /(\S+)/) {
                            $execName = $1;
                        } else {
                            $execName = $testInfoHash->{cmdList}->[$_];
                        }
                        # Exclude executables do not exist    
                        if(! -e $execName) {
                            $logger->report("Test $execName does not exist.");
                        } else {
                            # Here are passed tests
                            if($parallelExecution) {
                                push @parallelList, $testInfoHash->{cmdList}->[$_];
                                push @parallelIdxList, $testIdx;
                            } 
                            if($serialExecution) {
                                push @serialList, $testInfoHash->{cmdList}->[$_];
                                push @serialIdxList, $testIdx;
                            }
                            # The test index keeps track of the tests order in
                            # the list of processed tests.   We need this because we
                            # move tests into the serial or parallel test list and
                            # we need to relate the tests in those lists back to
                            # the base test list.
                            $testIdx++;
                            
                            push @suiteList, $testInfoHash->{suiteList}->[$_];
                            push @testNameList, $testInfoHash->{testNameList}->[$_];
                            push @testNumList, $testInfoHash->{testNumList}->[$_];
                            push @projectList, $project;
                            push @configList, $configs[$num];
                        }
                    }
                }        
            }
        }
    }

    return {parallelList => \@parallelList,
                serialList => \@serialList,
                suiteList => \@suiteList,
                testNameList => \@testNameList,
                testNumList => \@testNumList,
                projectList => \@projectList,
                configList => \@configList,
                parallelIdxLst => \@parallelIdxList,
                serialIdxLst => \@serialIdxList};
}

sub time_stamp {
    my @ltime = localtime($starting_time);

    my $stamp = sprintf("%04d_%02d%02d_%02d%02d%02d",
        $ltime[5]+1900, $ltime[4]+1, $ltime[3], $ltime[2],
        $ltime[1], $ltime[0]);
    return($stamp);
}

#
# Post process for Bullseye related stuff in working directories.
#
sub processTempDirs {
    my ($testListHashRef, $resultRoot) = @_;
    
    if(defined $argHash{'bullseye'}) {
        my $resultsRootCovFile = File::Spec->catdir($resultRoot, "bullseye.cov");
        my $projectRoot = File::Spec->catdir($resultRoot, $argHash{'project'});
        my $projectCovFile = File::Spec->catdir($projectRoot, "bullseye.cov");
        #
        # First Create a Bullseye results directory
        #    and save the bullseye files to it
        #
        $bullseye_config->moveBullseyeConfig($argHash{'target'}, $resultRoot);

        #
        # Second Create the Bullseye results "project" directory
        #
        $bullseye_config->moveBullseyeConfig($argHash{'target'}, $projectRoot);

        #
        # Third Create Bullseye results "project"/"suite" directory
        #
        foreach (0..scalar(@{$testListHashRef->{parallelworkingDirList}}) - 1) {
            my $resultsDir = $testListHashRef->{parallelworkingDirList}->[$_];
            my $curJobIndex = $testListHashRef->{parallelIdxLst}->[$_];
            my $suite = File::Spec->catdir($projectRoot, $testListHashRef->{suiteList}->[$curJobIndex]);
            if(! -e $suite) {
                # Just copy the cov files from the test execution directory to the suite results dir
                $bullseye_config->moveBullseyeConfig($resultsDir, $suite);
            }
            else {
                # 
                # Directory already exists, 
                #  This means that this suite was executed more than once.
                #  merge these cov data into this suites cov data
                #
                my $resultsCoveFile = File::Spec->catdir($resultsDir, "bullseye.cov");
                chdir($suite); 
                #$logger->report("Third");
                $logger->execute_command("covmerge --no-banner -fbullseye.cov bullseye.cov $resultsCoveFile");
            }
        }

        foreach (0..scalar(@{$testListHashRef->{serialworkingDirList}}) - 1) {
            my $resultsDir = $testListHashRef->{serialworkingDirList}->[$_];
            my $curJobIndex = $testListHashRef->{serialIdxLst}->[$_];
            my $suite = File::Spec->catdir($projectRoot, $testListHashRef->{suiteList}->[$curJobIndex]);
            if(! -e $suite) {
                # Just copy the cov files from the test execution directory to the suite results dir
                $bullseye_config->moveBullseyeConfig($resultsDir, $suite);
            }
            else {
                # 
                # Directory already exists, 
                #  This means that this suite was executed more than once.
                #  merge these cov data into this suites cov data
                #
                my $resultsCoveFile = File::Spec->catdir($resultsDir, "bullseye.cov");
                chdir($suite); 
                #$logger->report("Third");
                $logger->execute_command("covmerge --no-banner -fbullseye.cov bullseye.cov $resultsCoveFile");
            }
        }

        #
        # Fourth
        #
        # Now we have .cov files for each test. Use 'covmerge' command to generate .cov files for suite, project and branch.
        # Note that branch and suite .cov creation code is commented becasue it costs too much time and space.
        $logger->report("Start merging coverage data...");
        # Each test
        foreach (0..scalar(@{$testListHashRef->{parallelworkingDirList}}) - 1) {
            my $resultsDir = $testListHashRef->{parallelworkingDirList}->[$_];
            my $curJobIndex = $testListHashRef->{parallelIdxLst}->[$_];
            if(-e $resultsDir) {
                my $resultsCovFile = File::Spec->catdir($resultsDir, "bullseye.cov");

                
                # Merge current .cov to project
                chdir($projectRoot); 
                #$logger->report("Fourth.1  chdir($projectRoot)");
                $logger->execute_command("covmerge --no-banner -fbullseye.cov bullseye.cov $resultsCovFile");

                # Copy The tests bullseye files to test results directory
                my $suite = File::Spec->catdir($projectRoot, $testListHashRef->{suiteList}->[$curJobIndex]);
                my $testName = $testListHashRef->{testNameList}->[$curJobIndex];
                # Bug testName is an empty string.  THis needs to be fixed???
                my $testResult = File::Spec->catdir($suite,  $testListHashRef->{testNameList}->[$curJobIndex]);
                if(! -e $testResult) {
                    $bullseye_config->moveBullseyeConfig($resultsDir, $testResult);
                }
                else {
                    # Merge .cov into  test result 
                    chdir($testResult);
                    #$logger->report("Fourth.2 testName($testName) chdir($testResult)");
                    $logger->execute_command("covmerge --no-banner -fbullseye.cov bullseye.cov $resultsCovFile");
                }
            }
        }

        foreach (0..scalar(@{$testListHashRef->{serialworkingDirList}}) - 1) {
            my $resultsDir = $testListHashRef->{serialworkingDirList}->[$_];
            my $curJobIndex = $testListHashRef->{serialIdxLst}->[$_];
            if(-e $resultsDir) {
                my $resultsCovFile = File::Spec->catdir($resultsDir, "bullseye.cov");

                
                # Merge current .cov to project
                chdir($projectRoot); 
                #$logger->report("Fourth.1  chdir($projectRoot)");
                $logger->execute_command("covmerge --no-banner -fbullseye.cov bullseye.cov $resultsCovFile");

                # Copy The tests bullseye files to test results directory
                my $suite = File::Spec->catdir($projectRoot, $testListHashRef->{suiteList}->[$curJobIndex]);
                my $testName = $testListHashRef->{testNameList}->[$curJobIndex];
                # Bug testName is an empty string.  THis needs to be fixed???
                my $testResult = File::Spec->catdir($suite,  $testListHashRef->{testNameList}->[$curJobIndex]);
                if(! -e $testResult) {
                    $bullseye_config->moveBullseyeConfig($resultsDir, $testResult);
                }
                else {
                    # Merge .cov into  test result 
                    chdir($testResult);
                    #$logger->report("Fourth.2 testName($testName) chdir($testResult)");
                    $logger->execute_command("covmerge --no-banner -fbullseye.cov bullseye.cov $resultsCovFile");
                }
            }
        }

        #
        # Fifth
        # Now merge the project coverage file into the top coverage file in the resultsdir
        #
        chdir($resultRoot);
        #$logger->report("Fifth");
        $logger->execute_command("covmerge --no-banner -fbullseye.cov bullseye.cov $projectCovFile");

        #
        # Sixth process cov files into cvs files and upload to Coverity, when specified
        #        
        $bullseye_config->ApplyExclusions($projectRoot);
        $bullseye_config->ApplyExclusions($resultRoot);
        
        $logger->report("End merging coverage data.");

        if (defined $argHash{'coverity'}) {
           if(-e $argHash{'coverity_intermediate_dir'}) {
            $logger->report("............................................");
            $logger->report("Start uploading coverage data to Coverity...");
            $logger->report("............................................");
            
            my ($second, $minute, $hour, $dayofMonth, $month, $yearOffset, $dayOfWeek, $dayOfYear, $daylightSavings) = localtime();
            my $year = 1900 + $yearOffset;
            my $testStart = "$year-$month-$dayofMonth $hour:$minute:$second";

            my $workingDir = $argHash{'coverity_intermediate_dir'};
            foreach (0..scalar(@{$testListHashRef->{workingDirList}}) - 1) {
                my $resultsDir = $testListHashRef->{workingDirList}->[$_];
                if(-e $resultsDir) {
                    my $srcCovFile = File::Spec->catdir($testListHashRef->{workingDirList}->[$_], "bullseye.cov");
                    my $suite = File::Spec->catdir($projectRoot, $testListHashRef->{suiteList}->[$_]);
                    my $testResult = File::Spec->catdir($suite, $testListHashRef->{testNameList}->[$_]);
                    chdir($testResult);

                    if ( ! -e "bullseye.cvs" ) {
                        $logger->execute_command("covbr --csv --file bullseye.cov --output bullseye.cvs");
                        $logger->execute_command("cov-manage-emit --dir $workingDir add-coverage --bullseye-csv bullseye.csv --suitename $testListHashRef->{suiteList}->[$_] --testname $testListHashRef->{testNameList}->[$_] --teststart \"$testStart\" --teststatus pass");
                    }
                }
            }
            my $viewPath = $argHash{'viewPath'};
            my $priority = $argHash{'coverity-ta-priority-policy'};
            my $policy = $argHash{'coverity-ta-policy'};
            my $results = File::Spec->catdir($resultRoot, "coverity_recommendation.cvs");
            my $priority_flags = "--disable-default --compute-test-priority --test-priority-policy \$$priority\" --test-priority-output \"$results\" --strip-path \"$viewPath\"";
            my $upload_flags = "--disable-default --test-advisor --test-advisor-policy $policy --strip-path \"$viewPath\" --enable-test-metrics";
            $logger->execute_command("cov-analyze --dir $workingDir $upload_flags");


            #Push defects up to the coverity server
            # Todo:  make server access configurable on CLI
            $logger->execute_command("cov-commit-defects --dir $workingDir --host 10.244.52.53 --port 8080 --user admin --password coverity --stream TA2");

            $logger->report("...............................................");
            $logger->report("Finished uploading coverage data to Coverity...");
            $logger->report("...............................................");
          }
          else {
            my $workingDir = $argHash{'coverity_intermediate_dir'};
            $logger->report("...............................................");
            $logger->report("The Coverity working directory $workingDir doesn't exist.");
            $logger->report("...............................................");
          }
        }
    }
    
}

#
# Delete working directory tree.
#
sub cleanup{
    my $root = shift;
    $logger->report("Deleting $root...");
    rmtree($root, 0, 1);
}

#
# Get and process parameters 
#
sub generateArgv {
    $argHash{'tagname'}     = 'armada64_checked';
    $argHash{'mode'}        = 'simulation';
    $argHash{'verbosity'}   = 'test';
    $argHash{'list'}        = "Test_List.JSON";
    $argHash{'listold'}        = "Test_List";
    $argHash{'template'}    = "bullseye_template.cfg";
    $argHash{'iterations'}  = "1";
    $argHash{'project'}     = 'all';
    $argHash{'testType'}    = 'Regress';

    &Getopt::Long::config('require_order');
    unless (GetOptions(\%argHash,
            'vcType=s',
            'view=s',
            'tagname=s',
            'mode=s',
            'verbosity=s',
            'worker=s', 
            'extraCli=s',
            'branch=s',
            'projects=s',
            'suites=s',
            'build', 
            'buildOnly',
            'text',
            'cleanview',
            'updateview',
            'bullseye',
            'coverity',
            "iterations=s",
            "configDir=s",
            "list=s",
            "exclude=s",
            "template=s",
            'VOB=s',
            'update=s',
            'verbose',
            'parallelTests',
            'help',
            'processors_nocheck',
            'runproject=s',
            'memory_nocheck',
            'testType=s'
    )) {
        &usage();
        exit(-1);
    }

    if(defined $argHash{'help'}) {
        &usage();
        exit(-1);
    }
    
    # If '-view' is not specified, try current path
    if(!defined $argHash{'view'}) {
        $argHash{'view'} = cwd();
    }

    if(!defined $argHash{'configDir'}) {
        print "Please specify the directory contains configuration files\n";
        &usage();
        exit(-1);
    }
    
    #as per Martin set num workers to 8 by default and print correct messages.
    if (!defined $argHash{'worker'}) {
        $argHash{'worker'} = 8;
        $logger->report("Default number of workers (8) being assigned");
    } else {
        $logger->report("Number of workers assigned is $argHash{'worker'}");
    }

    if(defined $argHash{'coverity'}) {
        $argHash{'bullseye'} = 'bullseye';
    }

    if(defined $argHash{'bullseye'}) {
        #
        # Now force the TARGET so that the appropriate bits will be generated
        #
        $argHash{'tagname'} = 'bseye_armada64_checked';
    }

    if($argHash{'tagname'} eq 'bseye_armada64_checked') {
        $argHash{'bullseye'} = 'bullseye';
    }
    
    # Version control related logic is put in VersionControlUtil module
    $vcUtil = VersionControlUtil->new(argv => \%argHash,
                                                    logger => $logger);                                               
                                                    
    # Find actual view path
    $vcUtil->findView();

    if($argHash{'viewPath'} eq '') {
        die "$argHash{'view'} does not exist";
    }

    if(! -e $argHash{'viewPath'}) {
        die "view $argHash{'viewPath'} does not exist";
    }

    if(! -e $argHash{'catmerge'}) {
        die "view catmerge, $argHash{'catmerge'} does not exist";
    }

    #
    # set build when buildOnly set
    #
    if(defined $argHash{'buildOnly'}) {
        $argHash{'build'} = 'build';
    }
    
    # The following CLI indicate that the user intends that the view be built before running regression
    # Performing a regression after updating a view without building generates inconsistent results
    if(!defined $argHash{'build'}) {
        if(defined $argHash{'update'} || defined $argHash{'updateview'} || defined $argHash{'cleanview'}) {
            $argHash{'build'} = "";
        }
    }

    # If '-branch' is not set, read branch name from version control system
    $argHash{'branch'} = $vcUtil->getBranch() unless defined $argHash{'branch'};

    # Read multiple config dir separated by comma
    my @inputConfigDirs = split(/,/, $argHash{'configDir'}); # Relative config path to 'catmerge/'

    # Read multiple suites separated by comma
    my @inputSuites = split(/,/, $argHash{'suites'}); 
    
    return (\@inputConfigDirs, \@inputSuites);
}

#
# Initialize logger
#
sub initializeLogger {
    my $logFile = $argHash{'viewPath'} . "/regress_" . $run_time_stamp . ".txt";

    print "calling logger open($logFile)\n";
    $logger->open($logFile);
    
    $logger->report( "Starting " . localtime($starting_time));
    $logger->report("");
}

#
# Build necessary environment for concurrent test execution
#
sub buildConcurrentContext {
    my ($workingDirRoot, $testListHashRef) = @_;

    $testListHashRef->{parallelworkingDirList} = [];
    # Set the environment path
    if ($^O ne 'MSWin32') {
        $ENV{PATH} .= ":$argHash{'execDir'}";        
        $ENV{LD_LIBRARY_PATH} = $argHash{'execDir'};
    } else {
        $ENV{PATH} .= ";$argHash{'execDir'}";        
    }

    my $exe_parallel = concurrent->new();
    my $requestedIterations = $argHash{'iterations'};
    my $iterations = $requestedIterations + 1;  # increase by 1 to simplify for bounds check
    if($iterations != 2) {
     $logger->report("Executing all tests $requestedIterations times");
    }

    my $curJobIndex;
    
    $jobs += scalar(@{$testListHashRef->{parallelList}});

    for(my $pass = 1; $pass < $iterations; $pass++) {
      $curJobIndex = 0;
      foreach (0..scalar(@{$testListHashRef->{parallelList}}) - 1) {
        my $cmd   = $testListHashRef->{parallelList}->[$_];
        $curJobIndex = $testListHashRef->{parallelIdxLst}->[$_];
        my $suite = $testListHashRef->{suiteList}->[$curJobIndex]; 
        my $test  = $testListHashRef->{testNameList}->[$curJobIndex];
        my $num   = $testListHashRef->{testNumList}->[$curJobIndex];
        
        $curJobIndex++;
      
        $job++;  # index within this iteration

        # Remove the file extension
        my $execName;
        if($suite =~ /(\S+)\./) {
            $execName = $1;
        } else {
            $execName = $suite;
        }

        # Setup a temp directory to run each simulation test in a different directory. 
        # Some tests (like some cache tests) have very long test name which exceeds the max length of dir.
        my $tempdir;
        if($num > 0) {
            $tempdir = File::Spec->catdir($workingDirRoot, "pass$pass", "job${job}_[$execName]_$num");
        } else {
            $tempdir = File::Spec->catdir($workingDirRoot, "pass$pass", "job${job}_[$execName]");
        }
        
        mkpath($tempdir, 0, 0777);
        # add this temporary directory to the list for workingDirList entries
        push(@{$testListHashRef->{parallelworkingDirList}}, $tempdir);
        
        # Copy .cov generated during building to each work directory
        if($argHash{'tagname'} eq 'bseye_armada64_checked' && defined $argHash{'bullseye'}) {
            $bullseye_config->moveBullseyeConfig($argHash{'target'}, $tempdir);
        }

        my @mCmd : shared;
        # Break CLI and push them into concurrent module
        if($cmd =~ /(\S+) ([\S| ]+)/) {
            @mCmd = (File::Spec->catdir($argHash{'execDir'},"$1"), split(/ /, $2), '-verbosity', $argHash{'verbosity'});
        }
        else {
            @mCmd = (File::Spec->catdir($argHash{'execDir'},$cmd), '-verbosity', $argHash{'verbosity'});
        }
        
        if(defined $argHash{'text'}) {
            push(@mCmd, "-text");
        }

        if (defined $argHash{'extraCli'}) {
            foreach (split(/ /, $argHash{'extraCli'})) {
                push(@mCmd, $_);
            }
        }

        my $logfile = "$argHash{'execDir'}/pass${pass}_job${job}_[$execName]";
        
#  num not being used, so it is skipped.  Jenkins parser does not look for it.
#        $logfile .= "_$num" if $num >= 0;
#
        $logfile .= ".log";
        $exe_parallel->addCommand(\@mCmd, $logfile, $tempdir, $pass);
        }
    }

    $exe_parallel->setMaxTasks($argHash{'worker'});
    
    return $exe_parallel;
}

#
# Build necessary environment for serial test execution
#
sub buildSerialContext {
    my ($workingDirRoot, $testListHashRef) = @_;

    $testListHashRef->{serialworkingDirList} = [];
    # Set the environment path
    if ($^O ne 'MSWin32') {
        $ENV{PATH} .= ":$argHash{'execDir'}";        
        $ENV{LD_LIBRARY_PATH} = $argHash{'execDir'};
    } else {
        $ENV{PATH} .= ";$argHash{'execDir'}";        
    }

    my $exe_serial = concurrent->new();
    my $requestedIterations = $argHash{'iterations'};
    my $iterations = $requestedIterations + 1;  # increase by 1 to simplify for bounds check
    if($iterations != 2) {
     $logger->report("Executing all tests $requestedIterations times");
    }

    my $curJobIndex;
    
    $jobs += scalar(@{$testListHashRef->{serialList}});
    
    for(my $pass = 1; $pass < $iterations; $pass++) {
      $curJobIndex = 0;    
      foreach (0..scalar(@{$testListHashRef->{serialList}}) - 1) {
        my $cmd   = $testListHashRef->{serialList}->[$_];
        $curJobIndex = $testListHashRef->{serialIdxLst}->[$_];
        my $suite = $testListHashRef->{suiteList}->[$curJobIndex]; 
        my $test  = $testListHashRef->{testNameList}->[$curJobIndex];
        my $num   = $testListHashRef->{testNumList}->[$curJobIndex];
      
        $job++;  # index within this iteration

        # Remove the file extension from the name.
        my $execName;
        if($suite =~ /(\S+)\./) {
            $execName = $1;
        } else {
            $execName = $suite;
        }
        
        # Setup a temp directory to run each simulation test in a different directory. 
        # Some tests (like some cache tests) have very long test name which exceeds the max length of dir.
        my $tempdir;
        if($num > 0) {
            $tempdir = File::Spec->catdir($workingDirRoot, "pass$pass", "job${job}_[$execName]_$num");
        } else {
            $tempdir = File::Spec->catdir($workingDirRoot, "pass$pass", "job${job}_[$execName]");
        }
        mkpath($tempdir, 0, 0777);
        
        # add this temporary directory to the list for workingDirList entries
        push(@{$testListHashRef->{serialworkingDirList}}, $tempdir);
        
        # Copy .cov generated during building to each work directory
        if($argHash{'tagname'} eq 'bseye_armada64_checked' && defined $argHash{'bullseye'}) {
            $bullseye_config->moveBullseyeConfig($argHash{'target'}, $tempdir);
        }

        my @mCmd : shared;
        # Break CLI and push them into concurrent module
        if ($cmd =~ /^\-(\S+)\W+([\S| ]+)/) {
            my @args = split(/ /, $2);
            my $script = File::Spec->catdir($argHash{'execDir'}, shift(@args));
            @mCmd = ("$1", $script, @args);
        } elsif($cmd =~ /(\S+) ([\S| ]+)/) {
            @mCmd = (File::Spec->catdir($argHash{'execDir'},"$1"), split(/ /, $2), '-verbosity', $argHash{'verbosity'});
        }
        else {
            @mCmd = (File::Spec->catdir($argHash{'execDir'},$cmd), '-verbosity', $argHash{'verbosity'});
        }

        if(defined $argHash{'text'}) {
            push(@mCmd, "-text");
        }

        if (defined $argHash{'extraCli'}) {
            foreach (split(/ /, $argHash{'extraCli'})) {
                push(@mCmd, $_);
            }
        }

        my $logfile = "$argHash{'execDir'}/pass${pass}_job${job}_[$execName]";

#  num not being used, so it is skipped.  Jenkins parser does not look for it.
#        $logfile .= "_$num" if $num >= 0;
#

        $logfile .= ".log";
        $exe_serial->addCommand(\@mCmd, $logfile, $tempdir, $pass);
        }
    }

    $exe_serial->setMaxTasks(1);
    
    return $exe_serial;
}


sub isMemorySufficient {
    my $memory = shift;
    $logger->report("Memory: [$memory MB], Recommended: [$recommended_memory MB] (~12 GB)");
    return ($memory >= $recommended_memory) ? 1 : 0;
}

sub isProcessorSufficient {
    my $processors = shift;
    $logger->report("Processors: [$processors], Recommended: [$recommended_processors]");
    return ($processors >= $recommended_processors) ? 1 : 0;
}

sub checkPlatformSupport {
    if(defined($argHash{'processors_nocheck'}) && defined($argHash{'memory_nocheck'})) {
        $logger->report("Skipping core and memory requirement check\n");
        return;
    }
    
    $logger->report("\nChecking platform support...");
    my $supported = 1;
    my $system = systeminfo->new();
    
    if(defined($argHash{'processors_nocheck'}) and !defined($argHash{'memory_nocheck'})) {
        $supported = isMemorySufficient($system->{'memory'});
    }
    elsif(defined($argHash{'memory_nocheck'}) and !defined($argHash{'processors_nocheck'})) {
        $supported = isProcessorSufficient($system->{'processors'});
    }
    else {
        $supported = isMemorySufficient($system->{'memory'}) && isProcessorSufficient($system->{'processors'});
    }
    
    if (!$supported) {
        $logger->report("System requirements NOT met. See help to over-ride the requirements.\n");
        $logger->report("Ending " . localtime(time));
        exit(-1);
    }
    $logger->report("System requirements met.\n");
}

###################################################
#                                     Workflow Starts                                        #
###################################################

# Get and initialise argument hash. Parse -configDir and -suites arguments for value lists.
my ($inputConfigDirsRef, $inputSuitesRef) = generateArgv();

initializeLogger();
# Bail out if recommended configuration is not present, unless over-ridden.
checkPlatformSupport();

my $listType = determineListType($inputConfigDirsRef);

# Read projects names from config files.
my @configProjects;
if($listType == 1) {
    @configProjects = getProjects($inputConfigDirsRef);
} else {
    @configProjects = getProjectsOld($inputConfigDirsRef);
}

if(defined $argHash{'bullseye'} || $argHash{'tagname'} eq "bseye_armada64_checked") {
    #
    # Perform Bullseye initialization and build of appropriate directories
    #
    # Note that bullseye_template.cfg may not consider about VOB difference in different sites,
    # so we will always create new .cfg file which contains correct VOB information
    #
    foreach (0..scalar(@$inputConfigDirsRef) - 1) {
        # Gather inclusion and exclusion information of all projects for one build
        $bullseye_config->ProcessTemplate($argHash{'target'}, generateNewTemplate($inputConfigDirsRef->[$_], $configProjects[$_]));
    }
    $bullseye_config->GenerateConfig($argHash{'viewPath'}, $argHash{'target'});
    $bullseye_config->GenerateExclusions($argHash{'viewPath'}, $argHash{'target'});
}

if(defined $argHash{'coverity'}) {
   #
   # Set coverity intermediary directory so that it can be used across all phases of the coverity workflow
   #
   $argHash{'coverity_intermediate_dir'} = File::Spec->catdir($argHash{'viewPath'}, "coverity_intermediate_dir");
   
   my $policy = File::Spec->catdir($argHash{'catmerge'}, $argHash{'configDir'}, "coverity-ta-policy.json");
   if(-e "$policy") {
      $argHash{'coverity-ta-policy'} = $policy;
   }
   else {
   }
   my $priority_policy = File::Spec->catdir($argHash{'catmerge'}, $argHash{'configDir'}, "coverity-ta-priority-policy.json");
   if(-e "$priority_policy") {
      $argHash{'coverity-ta-priority-policy'} = $priority_policy;
   }
   else {
   }
}


# Update, clean and build if necessary
if( $argHash{'build'}) {
  if(defined $argHash{'bullseye'} || $argHash{'tagname'} eq "bseye_armada64_checked") {
    #
    # Set covccfg environment variable, which causes the build process to use these
    # config during the build to generate the bullseye.cov
    #
    $bullseye_config->setCovEnv($argHash{'target'},);

    if(defined $argHash{'coverity'}) {
        $logger->report("Performing Coverity Test Advisor build");
        
        # Mark all code as not un-tested
        # TOD:  Figure out where this belongs
        #my $viewPath = $argHash{'viewPath'};
        #$logger->execute_command("cov-manage-emit --dir $viewPath compute-coverability");
    }
  }
  $vcUtil->buildView();

  if(defined $argHash{'coverity'}) {
     my $dir = $argHash{'coverity_intermediate_dir'};
     chdir($argHash{'catmerge'}); 

     # Pull in the SCM annotations from AccuRev
     $logger->execute_command("cov-import-scm --dir $dir --scm accurev --filename-regex safe");
  }
}

if (defined $argHash{'buildOnly'}) {
    exit(0);
}

my $resultDir = $argHash{'execDir'};

$logger->report("execDir is $argHash{'execDir'}");

if(defined $argHash{'verbose'}) {
    $logger->report("chdir $resultDir");
}
chdir($resultDir) or die("Unable to cd to $resultDir\n");

# Analysis test config files to get test candidates information
my $testListHashRef;
if($listType == 1) {
    $testListHashRef = getTestList($inputConfigDirsRef);
} else {
    $testListHashRef = getTestListOld($inputConfigDirsRef);
}

if($testListHashRef->{parallelList}) {
    reportTestSchedule($testListHashRef->{parallelList}, "parallel");
}
if($testListHashRef->{serialList}) {
    reportTestSchedule($testListHashRef->{serialList}, "serial");
}

my $workingDirRoot =  File::Spec->catdir($argHash{'execDir'}, "Results_$run_time_stamp");

if(defined $argHash{'bullseye'} || $argHash{'tagname'} eq "bseye_armada64_checked") {
    $bullseye_config->setCovEnv();
}

# set up the build contexts for both the Serial and Parallel tests. 
my $exe_parallel = buildConcurrentContext($workingDirRoot, $testListHashRef);
my $exe_serial = buildSerialContext($workingDirRoot, $testListHashRef);


local %SIG;
$SIG{INT} = sub {
  $logger->report("Caught CTRL-C\n");
  $logger->report("Simulation Test will stopped...\n\n");

  $exe_parallel->stopAll();
  $exe_serial->stopAll();

  cleanup($workingDirRoot);
};


# Kick everything off, wait for it to finish, complain if something breaks

# Start with the parallel tests of present
if($testListHashRef->{parallelList}->[0]) {
    $exe_parallel->runAll();
}

# Now we will do the serial list if present.
if($testListHashRef->{serialList}->[0]) {
    # Next run the serial tests
    $exe_serial->runAll();
}

# Report all test results.
my $noFailures;
if($testListHashRef->{parallelList}->[0]) {
    $noFailures = reportTestResult($exe_parallel, $testListHashRef);
    $logger->report("Parallel simulation tasks completed\n");
}
my $noFailuresSerial;
if($testListHashRef->{serialList}->[0]) {
    $noFailuresSerial = reportTestResult($exe_serial, $testListHashRef);
    $logger->report("Serial simulation tasks completed\n");
}
$logger->report("All simulation tasks completed, final status");
$logger->report($totalTestsExecuted." total tests executed");
$logger->report($totalTestFailures." total test failures\n\n");

if(defined $argHash{'build'} || defined $argHash{'buildOnly'}) {
  #
  # only update the bullseye files in the target directory if a build is being perform 
  # Otherwise, leave the old builseye files generated during the last build
  #
  if(defined $argHash{'bullseye'} || $argHash{'tagname'} eq "bseye_armada64_checked") {
    $bullseye_config->SetupResultDir($argHash{'target'}, $argHash{'viewPath'}, $run_time_stamp);
  }
}

# change the working directory to current directory
if (!chdir($argHash{'execDir'})) {
    $logger->report("Could not chdir to $argHash{'execDir'}");
}

# Process Bullseye results in working directories
processTempDirs($testListHashRef, File::Spec->catdir($argHash{'viewPath'}, "Bullseye_results_$run_time_stamp"));

cleanup($workingDirRoot);

$logger->report( "Ending " . localtime(time));

# a non-zero exit status indicate failure
exit($noFailures || $noFailuresSerial);
