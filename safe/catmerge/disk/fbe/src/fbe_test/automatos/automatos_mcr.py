import os
import sys
import Queue
import time
import getopt
import shutil
import datetime
import subprocess
import threading
import re
import shlex
import time
import multiprocessing
from multiprocessing import Process, Lock
from time import sleep
from distutils.dir_util import mkpath
import xml.etree.ElementTree
import argparse
import traceback


HELP_TEXT = """\n\
Examples:\n\
 To run just one test: \n\
   automatos_mcr.py -u c:\\views\\myworkspace -t paco -b testbed_st327

 To run a set of tests: 
   automatos_mcr.py -u c:\\views\\myworkspace -t verify,paco -b testbed_st327

 To run all tests 10 times in a loop:  automatos_mcr.py -u c:\\views\\myworkspace -t all -b testbed_st327 -i 10

 To run just one test (using alt log directory):  automatos_mcr.py -u c:\\views\\myworkspace -t verify -b testbed_st327 --logs d:\\logs\\

 To list available tests: automatos_mcr.py -l

 To run all tests at functional level: automatos_mcr.py -b JF-H1113 -t all -tl f

 To run all tests at promotion and functional level: automatos_mcr.py -b JF-H1113 -t all -tl all

 Environment variables:
   (required) AUTOMATOSPERL - location of perl install, typically c:\\perl
   (optional) TEST_BED_DIRECTORY - Location of all your .xml test bed files. 
              By default we look under .\\test_beds for test bed files.
   (optional) CTT_PATH - Location of your ctt depot workspace for automatos.
   (optional) UPC_PATH - Location of upc workspace with test files.

"""
perllibs = ["Automatos",
            "Automatos\\Framework\\Dev\\bin",
            "Automatos\\Framework\\Dev\\lib",
            "Automatos\\Tests\\Dev",
            "Automatos\\UtilityLibraries\\Dev",
            "Automatos\\Automatos\\Framework\\Dev",]
test_files_path = "\\safe\\catmerge\\disk\\fbe\\src\\fbe_test\\automatos\\mcr_test\\"
test_bed_path = "\\safe\\catmerge\\disk\\fbe\\src\\fbe_test\\automatos\\test_beds\\"

test_set_parameters = {'utms_identities_rockies': [{"name":"utms_domain", "value":"USD"},
                                           {"name":"utms_project", "value":"ROCKIES"},
                                           {"name":"utms_team_name", "value":"Platform EFT"},
                                           {"name":"utms_address", "value":"utmswebserver01:8082"},],
                       'utms_identities_unity': [{"name":"utms_domain", "value":"USD"},
                                                 {"name":"utms_project", "value":"UNITY"},
                                                 {"name":"utms_team_name", "value":"Platform EFT"},
                                                 {"name":"utms_address", "value":"utmswebserver01:8082"},], 
                       'hooks_rockies': [{"location":"Automatos::Hook::Triage::Vnx::Rdgen",
                                            "parameters" : []},
                                         {"location":"Automatos::Hook::Triage::Vnx::SpCollect",
                                          "parameters" : []},
                                         {"location":"Automatos::Hook::Triage::Vnx::Ktrace",
                                          "parameters" : [{"name":"log_scope", "value":"test_case"},
                                                          {"name":"clean_old_logs", "value":"1"},
                                                          {"name":"only_transfer_on_failure", "value":"1"},]},
                                         ], 
                        'hooks_unity': [{"location":"Automatos::Hook::Triage::Vnxe::SpCollect",
                                          "parameters" : []}],
                       }

alt_cycle_ids = {'salmon_cycle5' : {'zeroing' : "116029",
                                    'paco' : "116022",
                                    'verify' : "116027",
                                    'rebuild' : "116023",
                                    'copyto' : "116026",
                                    'slf' : "116024",
                                    'write_hole' : "116028",
                                    'metadata' : "116021",
                                    'sniff' : "116025",
                                    'io' : "117315",},
                 'salmon_cycle6' : {'zeroing' : "117677",
                                    'io': '117678',
                                    'metadata' : "117679",
                                    'paco' : "117681",
                                    'rebuild' : "117682",
                                    'slf' : "117683",
                                    'sniff' : "117684",
                                    'copyto' : "117685",
                                    'verify' : "117686",
                                    'write_hole' : "117687"},
                 'salmon_cycle7' : {'zeroing' : "118528",
                                    'io': '118529',
                                    'metadata' : "118530",
                                    'paco' : "118531",
                                    'rebuild' : "118532",
                                    'slf' : "118533",
                                    'sniff' : "118534",
                                    'copyto' : "118535",
                                    'verify' : "118536",
                                    'write_hole' : "118537"},
                 'salmon_cycle9' : {'zeroing' : "120256",
                                    'io': '120257',
                                    'metadata' : "120258",
                                    'paco' : "120259",
                                    'rebuild' : "120260",
                                    'slf' : "120261",
                                    'sniff' : "120262",
                                    'copyto' : "120263",
                                    'verify' : "120264",
                                    'write_hole' : "120265",
                                    'encrypt': "124211"},
                 'unity_psi6' : {'zeroing' : "7779",
                                    'io': '7780',
                                    'metadata' : "7781",
                                    'paco' : "7782",
                                    'rebuild' : "7783",
                                    'slf' : "7784",
                                    'sniff' : "7785",
                                    'copyto' : "7786",
                                    'verify' : "7787",
                                    'write_hole' : "7788"},

                 }
# In UTMS, (test lab, test x -> Execution Grid  -> Test Instance Details: "Root Test Instance ID -> ax_id
# In UTMS, (test lab, test x -> Details -> "Test set id" -> cycle_id
tests = {'zeroing': [{"name":"zeroing","path":"zeroing","config_file":"zeroing_config.xml",
                     "test_set":"zeroing_test_set.xml",
                     "formal_test_name": "MCR Disk Zeroing",
                     "utms_cycle_id_promotion": "110214",
                     "utms_cycle_id_functional": "114018",
                     "test_params" : [{"name":"is_zeroed", "value":"undef"}],
                     "test_cases": [{"name":"MCR Zeroing Test Case", 
                                     "utms_ax_id":"969375",
                                     "location" : "mcr_test::zeroing::zeroing"}],}],
         'paco': [{"name":"paco","path":"proactive_copy","config_file":"proactive_copy_config.xml",
                     "test_set":"proactive_copy_test_set.xml",
                     "formal_test_name": "MCR Proactive Copy",
                     "utms_cycle_id_promotion": "112923",
                     "utms_cycle_id_functional": "114013",
                     "test_params" : [{"name":"is_zeroed", "value":"undef"},{"name":"timeBetweenCopyCompleteStatusChecks", "value":"60"}],
                     "test_cases": [{"name":"MCR Proactive Copy Test Case", 
                                     "utms_ax_id":"981197",
                                     "location" : "mcr_test::proactive_copy::proactive_copy_tests"}],}],
         'verify': [ {"name":"verify",
                      "path":"verify","config_file":"verify_config.xml",
                      "test_set":"verify_test_set.xml",
                      "formal_test_name": "MCR Verify",
                      "utms_cycle_id_promotion": "113388",
                     "utms_cycle_id_functional": "114016",
                      "test_params" : [{"name":"is_zeroed", "value":"undef"}],
                      "test_cases": [{"name":"MCR Verify Test Case", 
                                      "utms_ax_id":"981417",
                                      "location" : "mcr_test::verify::verify"}],}],
         'rebuild': [ {"name":"rebuild",
                      "path":"rebuild","config_file":"rebuild_config.xml",
                      "test_set":"rebuild_test_set.xml",
                      "formal_test_name": "MCR Rebuild",
                      "utms_cycle_id_promotion": "113389",
                     "utms_cycle_id_functional": "114014",
                      "test_params" : [{"name":"is_zeroed", "value":"undef"}],
                      "test_cases": [{"name":"Rebuild Test Case", 
                                      "utms_ax_id":"981416",
                                      "location" : "mcr_test::rebuild::rebuild"}],}],
         'copyto': [{"name":"user_copy","path":"user_copy","config_file":"user_copy_config.xml",
                     "test_set":"user_copy_test_set.xml",
                     "formal_test_name": "MCR User Copy",
                     "utms_cycle_id_promotion": "112936",
                     "utms_cycle_id_functional": "114015",
                     "test_params" : [{"name":"is_zeroed", "value":"undef"}],
                     "test_cases": [{"name":"User Copy Test Cases", 
                                     "utms_ax_id":"981415",
                                     "location" : "mcr_test::user_copy::user_copy"}],
                     },],
         'slf': [{"name":"slf","path":"single_loop_failure","config_file":"slf_config.xml",
                  "test_set":"slf_test_set.xml",
                  "formal_test_name": "Single Loop Failure",
                  "utms_cycle_id_promotion": "114046",
                  "utms_cycle_id_functional": "114047",
                  "test_params" : [{"name":"is_zeroed", "value":"undef"}],
                  "test_cases": [{"name":"SLF Test Case", 
                                  "utms_ax_id":"988606",
                                  "location" : "mcr_test::single_loop_failure::slf"}],}],
         'write_hole': [{"name":"write hole","path":"write_hole","config_file":"write_hole_config.xml",
                         "test_set":"write_hole_test_set.xml",
                         "formal_test_name": "MCR Write Hole",
                         "utms_cycle_id_promotion": "112938",
                         "utms_cycle_id_functional": "114017",
                         "test_params" : [{"name":"is_zeroed", "value":"undef"}],
                         "test_cases": [{"name":"Write Hole Test Case", 
                                         "utms_ax_id":"984348",
                                         "location" : "mcr_test::write_hole::write_hole"}],}],
         'metadata': [{"name":"paged metadata","path":"metadata","config_file":"metadata_config.xml",
                       "test_set":"metadata_test_set.xml",
                       "formal_test_name": "MCR Metadata",
                       "utms_cycle_id_promotion": "112940",
                       "utms_cycle_id_functional": "114012",
                       "test_params" : [{"name":"is_zeroed", "value":"undef"}],
                       "test_cases": [{"name":"Metadata Test Case", 
                                       "utms_ax_id":"984350",
                                       "location" : "mcr_test::metadata::metadata"}],}],
         'sniff': [{"name":"sniff","path":"sniff","config_file":"sniff_config.xml",
                    "test_set":"sniff_test_set.xml",
                    "formal_test_name": "MCR Sniff Verify",
                    "utms_cycle_id_promotion": "115731",
                    "utms_cycle_id_functional": "115730",
                    "test_params" : [{"name":"is_zeroed", "value":"undef"}],
                    "test_cases": [{"name":"Basic Sniff Verify Test Case", 
                                    "utms_ax_id":"998333",
                                    "location" : "mcr_test::sniff::sniff_basic"}],}],
         'power_savings': [{"name":"power_savings","path":"power_savings","config_file":"power_savings_config.xml",
                    "test_set":"power_savings_test_set.xml",
                    "formal_test_name": "MCR Power Savings",
                    "utms_cycle_id_promotion": "124164",
                    "utms_cycle_id_functional": "124165",
                    "test_params" : [{"name":"is_zeroed", "value":"undef"}],
                    "test_cases": [{"name":"Power Savings Test Case", 
                                    "utms_ax_id":"984350",
                                    "location" : "mcr_test::power_savings::power_savings"}],}],
         'io': [{"name":"io","path":"io","config_file":"io_config.xml",
                    "test_set":"io_test_set.xml",
                    "formal_test_name": "MCR IO Test",
                    "utms_cycle_id_promotion": "117317",
                    "utms_cycle_id_functional": "117316",
                    "test_params" : [{"name":"is_zeroed", "value":"undef"}],
                    "test_cases": [{"name":"IO Test Case", 
                                    "utms_ax_id":"1009193",
                                    "location" : "mcr_test::io::io"}],}],
         'encrypt': [{"name":"encrypt","path":"encryption","config_file":"encryption_config.xml",
                     "test_set":"encryption_test_set.xml",
                     "formal_test_name": "MCR Encryption",
                     "utms_cycle_id_promotion": "112923",
                     "utms_cycle_id_functional": "114013",
                     "test_params" : [{"name":"is_zeroed", "value":"undef"},{"name":"timeBetweenEncryptionCompleteStatusChecks", "value":"60"}],
                     "test_cases": [{"name":"MCR Basic Encryption Test Case", 
                                     "utms_ax_id":"1076286",
                                     "location" : "mcr_test::encryption::encryption_tests"}],}],
         }
def format_duration(seconds):
    hours   = int(seconds / 3600)
    mins    = int(seconds % 3600) / 60
    secs    = int(seconds % 60)

    string = "%02d:%02d:%02d" % (hours, mins, secs)

    return string
def get_dir_date_time():
    now = datetime.datetime.now()
    current_date_time = now.strftime('%Y-%m-%d_%H-%M-%S')
    return current_date_time
def get_date_time():
    now = datetime.datetime.now()
    current_date_time = now.strftime('%m/%d/%Y %H:%M:%S')
    return current_date_time

def level_to_string(level):
    if level == 'p':
        level_string = 'Promotion'
    else:
        level_string = 'Functional'
    return level_string

def level_to_int(level):
    if level == 'p':
        return 0
    else:
        return 1

class test_runner:
    def __init__(self):
        self.cur_dir = os.getcwd()
        self.cmdline = ""
        self.cmd_args = []
        self.automatos_path = ""
        self.config_file_path = ""
        self.log_path = "c:\\logs\\"
        self.config_file_directory = "c:\\logs"
        self.test_bed_directory = test_bed_path
        self.test_instance_id = 124
        self.test_set_file = ""
        self.test_bed_file = ""
        self.tests = []
        self.se_debug_path = "C:\\Program Files\\SlickEditV18.0.1 x64\\resource\\tools\\perl5db-0.30"
        self.test_levels = []
        self.test_results = []
        self.iteration_log_path = ""
        self.user_email = ""
        self.clean_config = 1 # By default we always clean the config.
        self.skip_reboot = 0 # By default reboots are allowed.
        self.utms_enabled = 1 # By default we report to utms.
        self.use_raid_debug_flags = 0
        self.rg_debug_flags = 1
        self.rl_debug_flags = 0
        self.show_xml_generated = 0 # Make this one to display xml files at beginning of test.
        self.cycle = None
        self.encryption_enabler = ""
        self.audit_log_path = "c:\\" # @todo This is the HOST path !!
        
    def setup_log_file(self):
        current_date_time = get_dir_date_time()
        self.log_path = self.log_path + "\\" + current_date_time
        self.config_file_directory = self.config_file_directory + "\\" + current_date_time 

        print("Logs path is: %s" % self.log_path)
        os.makedirs(self.log_path)

        self.results_file = self.log_path + "\\\\" + "results.txt"
        print("Logs file is: %s" % self.results_file)
        f = open(self.results_file, "w")
        global log_object
        log_object = writer(f, sys.stdout)
        sys.stdout = log_object
        sys.stderr = log_object

    def parse_cmdline(self):
        parser = argparse.ArgumentParser(description="automatos test runner",add_help=False)
        parser.add_argument('-h','--help', help='Display help info', required=False, action="store_true")
        parser.add_argument('-t','--tests', help='Tests to run', required=False)
        parser.add_argument('-c','--ctt_path',help='ctt workspace path', required=False)
        parser.add_argument('-u','--upc_path',help='upc workspace path', required=False)
        parser.add_argument('-b','--test_bed',help='test bed name.  Example: test_bed_1234', required=False)
        parser.add_argument('-l','--list',help='list test cases', required=False, action="store_true")
        parser.add_argument('-i','--iterations',help='times to run the test', required=False, default=1)
        parser.add_argument('-L','--logs',help='alt log path', required=False)
        parser.add_argument('-tl','--test_level',help='test level - p:promotion(default) f:functional', required=False)
        parser.add_argument('-d','--debugger',help='Launch debugger.', required=False, action="store_true")
        parser.add_argument('-perl_debugger','--perl_debugger',help='Launch raw Perl debugger.', required=False, action="store_true")
        parser.add_argument('-stop','--stop',help='Stop on first failure.', required=False, action="store_true")
        parser.add_argument('-skip_test','--skip_test',help='For debugging this script.', required=False, action="store_true")
        parser.add_argument('-no_clean_config','--no_clean_config',help='If set, we do not clean config.', required=False, action="store_true")
        parser.add_argument('-skip_reboot','--skip_reboot',help='If set, we do not perform reboots.', required=False, action="store_true")
        parser.add_argument('-no_utms','--no_utms',help='If set, skip reporting to UTMS.', required=False, action="store_true")
        parser.add_argument('-rg_debug_flags','--rg_debug_flags',help='Set RAID Group Debug Flags', required=False)
        parser.add_argument('-rl_debug_flags','--rl_debug_flags',help='Set RAID Library Debug Flags', required=False)
        parser.add_argument('-no_auto_test_set','--no_auto_test_set',help='Does not generate test set automatically', required=False, action="store_true")
        parser.add_argument('-test_set_dir','--test_set_dir',help='Directory of test sets to run.', required=False)
        parser.add_argument('-rockies_cycle','--rockies_cycle',help='Use a different set of cycle ids.', required=False)
        parser.add_argument('-unity_cycle','--unity_cycle',help='Use a different set of cycle ids.', required=False)
        parser.add_argument('-encryption_enabler','--encryption_enabler',help='encryption enabler location - Path to security enabler', required=False)

        self.args = parser.parse_args()
        if self.args.help:
            parser.print_help()
            print HELP_TEXT
            exit(0)
        if self.args.logs:
            self.log_path = self.args.logs
            self.config_file_directory = self.args.logs
            print "Using alt log path of %s" % (self.log_path)

        # Need to call this here after the alt log path is setup.
        self.setup_log_file()

        if self.args.iterations:
            self.args.iterations = int(self.args.iterations)
        if self.args.list:
            print "Valid test names:\n"
            test_list = ""
            for t in tests:
                print "%s" % t
                if test_list != "":
                    test_list = test_list + "," + t
                else:
                    test_list = t
            print "to run all tests use -t " + test_list
            exit(0)
        if not self.args.test_bed:
            print "missing test bed name, this is required"
            exit(1)
        if 'AUTOMATOSPERL' not in os.environ:
            print "please set AUTOMATOSPERL. This is required."
            exit(1)
        if 'CTT_PATH' not in os.environ and not self.args.ctt_path:
            print "please set CTT_PATH or provide --ctt_path argument. This is required."
            exit(1)
        if not self.args.ctt_path and 'CTT_PATH' in os.environ:
            self.args.ctt_path = os.environ['CTT_PATH']
            print "CTT_PATH found: %s" %  os.environ['CTT_PATH']
        if not self.args.tests and not self.args.test_set_dir:
            print "please provide -t or --test argument. This is required."
            exit(1)
        if 'UPC_PATH' in os.environ:
            print "UPC_PATH found: %s" %  os.environ['UPC_PATH']
        if not self.args.upc_path and 'UPC_PATH' not in os.environ:
            print "please provide -u or --upc_path argument. This is required."
            exit(1)
        if self.args.encryption_enabler:
            self.encryption_enabler = self.args.encryption_enabler
            print "Using encryption enabler of %s" % (self.args.encryption_enabler)
        print "AUTOMATOSPERL: %s" % os.environ['AUTOMATOSPERL']
        print "ctt_path: %s" % self.args.ctt_path
        print "upc_path: %s" % self.args.upc_path
        print "tests arg: %s" % self.args.tests

        if 'ADE_MAILTO' in os.environ:
            print "ADE_MAILTO found: %s" %  os.environ['ADE_MAILTO']
            self.user_email = os.environ['ADE_MAILTO']
        else:
            print "ADE_MAILTO environment variable not found"
            exit(1)
        if self.args.no_clean_config:
            print "Argument set to not clean configuration"
            self.clean_config = 0
        if self.args.skip_reboot:
            print "Argument set to skip reboots during test"
            self.skip_reboot = 1
        if self.args.no_utms:
            print "Argument set to not report to utms"
            self.utms_enabled = 0
        if self.args.rg_debug_flags:
            print "RG Debug Flags set to %s" % (self.args.rg_debug_flags)
            self.rg_debug_flags = self.args.rg_debug_flags
            self.use_raid_debug_flags = 1
        if self.args.rl_debug_flags:
            print "RL Debug Flags set to %s" % (self.args.rl_debug_flags)
            self.rl_debug_flags = self.args.rl_debug_flags
            self.use_raid_debug_flags = 1
        if self.args.test_level:
            print "test level: %s" % self.args.test_level
            if self.args.test_level == "all":
                print "Running both promotion and functional test levels"
                self.test_levels.append('p')
                self.test_levels.append('f')
            elif self.args.test_level == 'p':
                self.test_levels.append('p')
            elif self.args.test_level == 'f':
                self.test_levels.append('f')
            else:
                print "Error:  unknown test level %s" % self.args.test_level
                exit(1)
        else:
            print "Running promotion test level by default"
            self.test_levels.append('p')
        if self.args.rockies_cycle:
            self.cycle = self.args.rockies_cycle
        if self.args.unity_cycle:
            self.cycle = self.args.unity_cycle
        if self.cycle:
            print "cycle is: %s" % (self.cycle)
            if self.cycle not in alt_cycle_ids:
                print "cycle %s is not valid" % (self.cycle)
                print "valid cycles:"
                for cycle in alt_cycle_ids.iterkeys():
                    print "%s" % (cycle)
                exit(0)
        if self.args.tests:
            self.test_list = self.args.tests.replace(' ','').split(',')
            print "tests to run:"
            if self.args.tests == "all":
                print "Remove `encrypt' from tests"
                del tests['encrypt']
                self.test_list = list(tests.keys())
                print self.test_list
            for t in self.test_list:
                if t not in tests:
                    print "%s is unknown test name.  Use -t to get test list." % t
                    exit(1)
                print t

    def generate_test_config(self):
        config_file_name = self.config_file_path
        
        print "generate config file to: %s" % (config_file_name)
        print "test set: %s" %(self.test_set_file)
        print "test bed: %s" %(self.test_bed_file)

        self.handle = open(config_file_name, 'w')
        self.handle.write("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>\n")
        self.handle.write("<opt doc_type=\"TC_CFG\" version=\"1\"> \n")
        self.handle.write("   <central_base_log_path>%s</central_base_log_path>\n" % (self.iteration_log_path))
        self.handle.write("   <local_base_log_path>%s</local_base_log_path>\n" % (self.iteration_log_path))
        self.handle.write("   <test_server>localhost</test_server>\n")
        test_set = "   <test_set_file>" + self.test_set_file + "</test_set_file>\n"
        #print test_set
        self.handle.write(test_set)
        test_bed = "   <testbed_file>" + self.test_bed_file + "</testbed_file>\n"
        self.handle.write(test_bed)
        self.handle.write("   <tms> <ip>localhost</ip></tms>\n")
        self.handle.write("   \n")
        self.handle.write("   <test_instance_id>%d</test_instance_id>\n" % self.test_instance_id)
        self.handle.write("   <execution_parameters>\n")
        self.handle.write("     <parameter name=\"STOP_ON_ERROR\" value=\"%d\"></parameter>\n" % self.args.stop)
        self.handle.write("     <parameter name=\"LOGGING_LEVEL\" value=\"INFO\"></parameter>\n")
        self.handle.write("   </execution_parameters>\n")
        self.handle.write("   \n")
        self.handle.write("</opt>\n")
        self.handle.close()

    def generate_test_set(self, test_name, test_level):
        self.is_unified = self.is_unified_test_bed();
        test_set_file_name = self.test_set_file
        test_set_file_name = self.new_test_set_file
        print "generate test set file to: %s" % (test_set_file_name)
        test_array = tests[test_name]
        
        self.handle = open(test_set_file_name, 'w')
        self.handle.write("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>")
        self.handle.write("<opt doc_type=\"TC_CFG\" version=\"1\"> \n")
        self.handle.write(" <test_set name=\"%s\">\n" % test_array[0]['formal_test_name'])
        self.handle.write("  <general_test_case_parameters>\n")
        for param in test_array[0]['test_params']:
            self.handle.write("    <param name=\"%s\" value=\"%s\"> </param>\n" % (param['name'], param['value']) )

        self.handle.write("  </general_test_case_parameters> \n")
        global test_set_parameters
        if self.utms_enabled:
            self.handle.write("  <identities>\n");
            if self.args.unity_cycle:
                utms_identities_params = test_set_parameters['utms_identities_unity']
            else:
                utms_identities_params = test_set_parameters['utms_identities_rockies']
            for param in utms_identities_params:
                self.handle.write("    <identity name=\"%s\" value=\"%s\"> </identity>\n" % (param['name'], param['value']) )
            if self.cycle:
                ids_dict = alt_cycle_ids[self.cycle]
                cycle_id = ids_dict[test_name]
                print "test %s cycle %s using cycle id %s" % (test_name, self.cycle, cycle_id)
            elif test_level == "p":
                cycle_id = test_array[0]['utms_cycle_id_promotion']
            else:
                cycle_id = test_array[0]['utms_cycle_id_functional']
            self.handle.write("    <identity name=\"utms_cycle_id\" value=\"%s\"> </identity>\n" % (cycle_id) )
            self.handle.write("    <identity name=\"utms_user_email\" value=\"%s\"> </identity>\n" % (self.user_email))
            self.handle.write("  </identities>\n")
        self.handle.write("  <hooks>\n")
        if self.is_unified:
            hooks = test_set_parameters['hooks_unity']
        else:
            hooks = test_set_parameters['hooks_rockies']
        for hook in hooks:
            self.handle.write("    <hook>\n")
            self.handle.write("      <location>%s</location>\n" % (hook['location']) )
            if len(hook['parameters']):
                self.handle.write("      <parameters>\n");
            for param in hook['parameters']:
                self.handle.write("        <parameter name=\"%s\" value=\"%s\"> </parameter>\n" % (param['name'], param['value']) )
            if len(hook['parameters']):
                self.handle.write("      </parameters>\n");
            self.handle.write("    </hook>\n");
        self.handle.write("  </hooks>\n");
        test_level_int = level_to_int(test_level)
        for test_case in test_array[0]['test_cases']:
            self.handle.write("    <test_case name=\"%s\">\n" % (test_case['name']) );
            self.handle.write("    <location>%s</location>\n" % (test_case['location']) )
            self.handle.write("    <order>10 </order>\n" )
            self.handle.write("    <parameters>\n");
            self.handle.write("    <param name=\"test_level\" value=\"%u\"></param>\n" % test_level_int);
            self.handle.write("    <param name=\"clean_config\" value=\"%u\"></param>\n" % self.clean_config);
            self.handle.write("    <param name=\"skip_reboot\" value=\"%u\"></param>\n" % self.skip_reboot);
            self.handle.write("    <param name=\"set_clear_raid_debug_flags\" value=\"%u\"></param>\n" % self.use_raid_debug_flags);
            self.handle.write("    <param name=\"raid_group_debug_flags\" value=\"%s\"></param>\n" % self.rg_debug_flags);
            self.handle.write("    <param name=\"raid_library_debug_flags\" value=\"%s\"></param>\n" % self.rl_debug_flags);
            self.handle.write("    <param name=\"encryption_enabler\" value=\"%s\"></param>\n" % self.encryption_enabler);
            self.handle.write("    <param name=\"host_audit_log_path\" value=\"%s\"></param>\n" % self.audit_log_path);
            self.handle.write("    </parameters>\n");
            if self.utms_enabled:
                self.handle.write("    <identities>\n");
                self.handle.write("      <identity name=\"ax_id\" value=\"%s\"></identity>\n" % (test_case['utms_ax_id']));
                self.handle.write("    </identities>\n");
            self.handle.write("    </test_case>\n");
        self.handle.write(" </test_set>\n")
        self.handle.write("   \n")
        self.handle.write("</opt>\n")
        self.handle.close()
        if self.show_xml_generated:
            f = open(test_set_file_name, 'r')
            text = f.read()
            print text
            f.close()
            
    def is_unified_test_bed(self):
        f = open(self.test_bed_file, 'r')   
        text = f.read()   
        for matching in re.findall(r"\<unified_devices\>", text):
            f.close();
            return True
        f.close()
        return False
            
    def set_environment(self):
        if 'UPC_PATH' in os.environ:
            self.args.upc_path = os.environ['UPC_PATH']
        new_perl5lib = os.environ['PERL5LIB']
        self.automatos_path = self.args.ctt_path
        self.automatos_cmd = self.args.ctt_path + '\\' + 'Automatos\\Framework\\Dev\\bin\\automatos.pl'
        for p in perllibs:
            lib_path = self.args.ctt_path + '\\' + p 
            new_perl5lib = new_perl5lib + ";" + lib_path
        new_perl5lib = new_perl5lib + ";" + self.args.upc_path + "\\safe\\catmerge\\disk\\fbe\\src\\fbe_test\\automatos"
        print "new perl5lib=%s" % new_perl5lib
        os.environ['perl5lib']=new_perl5lib

        new_path = os.environ['AUTOMATOSPERL'] + ";" + os.environ['path'] + ";" + self.automatos_path

        print "new path=%s" %new_path
        os.environ['path']=new_path
        
        if self.args.debugger:
            print "Configured to launch SlickEdit Debugger"
            os.environ['PERLDB_OPTS'] = "RemotePort=127.0.0.1:52638"; #52030 52637
            os.environ['DBGP_IDEKEY'] ="slickedit";
            os.environ['PERL5DB']="BEGIN { require \'perl5db.pl\'; }";
            if 'SE_DEBUG_PATH' in os.environ:
                self.se_debug_path = os.environ['SE_DEBUG_PATH']
                print "SE_DEBUG_PATH found %s" % (self.se_debug_path)
            else:
                print "using default Slickedit Debugger Path: %s" % (self.se_debug_path)

    def set_paths(self, test_name, test_level):
        if test_name not in tests:
            print "test %s not found" % self.test_name
            exit(1)

        test_array = tests[test_name]
        test_dir = test_array[0]['path']
        test_name = test_array[0]['config_file']
        test_set_name = test_array[0]['test_set']
        if test_level:
            if test_level == "f":
                test_set_name = re.sub(".xml", "_functional.xml", test_set_name);
        #self.config_file_path = self.args.upc_path + test_files_path + test_dir + "\\" + test_name
        if self.args.no_auto_test_set:
            self.test_set_file = self.args.upc_path + test_files_path + test_dir + "\\" + test_set_name
        else:
            self.test_set_file = self.config_file_directory + "\\" + test_set_name
        self.test_bed_file = self.args.upc_path + self.test_bed_directory + self.args.test_bed + ".xml"
        if 'TEST_BED_DIRECTORY' in os.environ:
            self.test_bed_file = os.environ['TEST_BED_DIRECTORY'] + "\\"+ self.args.test_bed + ".xml"
            print "TEST_BED_DIRECTORY: %s" % os.environ['TEST_BED_DIRECTORY']
        self.config_file_path = self.config_file_directory + "\\" + "config_file.xml"
        self.new_test_set_file = self.config_file_directory + "\\" + test_set_name
        #self.config_file_path =  self.args.upc_path + test_files_path + test_dir +"\\" + "config.xml"

    def prepare_cmdline(self):
       if self.args.debugger:
           self.cmd_args.append("perl -d -I\"" + self.se_debug_path + "\" " + self.automatos_cmd + " -configFile " +  self.config_file_path)
           #self.cmd_args.append("-d -I\"" + self.se_debug_path + "\" " + self.automatos_cmd + " -configFile " +  self.config_file_path)
           #self.cmd_args.append("-I\"" + self.se_debug_path + "\"")
           #self.cmd_args.append(self.automatos_cmd)
           #self.cmd_args.append("-configFile")
           #self.cmd_args.append(self.config_file_path)
           self.cmdline = "perl -d -I\"" + self.se_debug_path + "\" " + self.automatos_cmd + " -configFile " +  self.config_file_path
           print self.cmdline
       elif self.args.perl_debugger:
           self.cmdline = "perl -d " + self.automatos_cmd + " -configFile " +  self.config_file_path 
           print self.cmdline
       else:
           self.cmd_args.append("perl")
#           self.cmd_args.append("-d ")
           self.cmd_args.append(self.automatos_cmd)
           self.cmd_args.append("-configFile")
           self.cmd_args.append(self.config_file_path)
           self.cmdline = "perl " + self.automatos_cmd + " -configFile " +  self.config_file_path 
           print self.cmdline

    def run_debugger_cmd(self):
       print("Run command: %s" % self.cmdline)
       status = subprocess.call(self.cmdline, shell=True)
       return status
    def run_cmd(self, cmd):
       print("Run command: %s" % cmd)
       status = subprocess.call(cmd, shell=True)
       return status
    def run_cmdline(self):
        #self.cmdline = "python c:\\temp\\
        #args = self.cmdline.split(' ')

        #print self.cmd_args
        child = subprocess.Popen(self.cmdline, stdout = subprocess.PIPE, stderr= subprocess.PIPE)
        #output, status = child.communicate()

        while child.poll() is None:
            out = child.stdout.read(1)
            sys.stdout.write(out)
            sys.stdout.flush()
        status = child.returncode 
        print 'status is: %s ' % status
        return int(status)
    def print_results (self):
        header_printed = False

        last_level = ""
        total_level_seconds = 0
        total_seconds = 0
        this_level_count = 0
        total_levels = 1
        for r in self.test_results:
            if header_printed == False:
                print "%20s %20s %10s %10s %10s %10s" % ("Date/Time", "Name", "Time", "Status", "Iteration", "Level")
                print "----------------------------------------------------------------------------------------------------------"
                header_printed = True
            if last_level != "" and last_level != r['level']:
                if this_level_count > 1:
                    print "%52s" % ("Total:   " + format_duration(total_level_seconds))
                    print ""
                this_level_count = 0
                total_levels = total_levels + 1
                total_level_seconds = 0
                
            date = r['date_time']
            print "%20s %20s %10s %10s %10u %10s" % (date, r['test'][:20], format_duration(r['seconds']), r['status_string'], r['iteration'], r['level'])
            total_seconds = total_seconds + r['seconds']
            total_level_seconds = total_level_seconds + r['seconds']
            last_level = r['level']
            this_level_count = this_level_count + 1
        
        print "%52s" % ("Total:   " + format_duration(total_level_seconds))
        print ""
        if total_levels != 1:
            print "%52s" % ("Overall Total:   " + format_duration(total_seconds))
            print ""
    def run_test(self, t, l, iteration):
        self.set_paths(t, l)
        self.prepare_cmdline()
        self.generate_test_config()

        # Only auto generate test set if this is enabled.
        if not self.args.no_auto_test_set:
            self.generate_test_set(t, l)

        start_time = time.time()
        date_time = get_date_time()

        print "test %s starting at %s" % (t, date_time)
        
        if self.args.skip_test:
            status = 0
            #time.sleep(1)
        else:
            if self.args.perl_debugger:# or self.args.debugger:
                status = self.run_debugger_cmd()
            else:
                status = self.run_cmdline()
        elapsed = (time.time()-start_time)

        if status == 0:
            status_string = 'SUCCESS'
        else:
            status_string = 'FAILED'

        result = {'status' : status, 
                  'status_string' : status_string, 
                  'test': t, 
                  'level' : level_to_string(l), 
                  'seconds': elapsed, 
                  'iteration' : iteration,
                  'date_time' : date_time}
        self.test_results.append(result)
        print "test %s completed in %u seconds with status %s (%d)" % (t, elapsed, status_string, status)
        self.print_results()

        if status != 0 and self.args.stop:
            print "stop on failure is set.  Exit test"
            exit(1)
    def run_test_set(self, file, iteration):
        file_path = os.path.join(self.args.test_set_dir, file)
        self.test_set_file = file_path
        self.test_bed_file = self.args.upc_path + self.test_bed_directory + self.args.test_bed + ".xml"
        if 'TEST_BED_DIRECTORY' in os.environ:
            self.test_bed_file = os.environ['TEST_BED_DIRECTORY'] + "\\"+ self.args.test_bed + ".xml"
            print "TEST_BED_DIRECTORY: %s" % os.environ['TEST_BED_DIRECTORY']
        self.config_file_path = self.config_file_directory + "\\" + "config_file.xml"
        self.prepare_cmdline()
        self.generate_test_config()

        start_time = time.time()
        date_time = get_date_time()

        print "test %s starting at %s" % (file, date_time)
        
        if self.args.skip_test:
            status = 0
            #time.sleep(1)
        else:
            if self.args.perl_debugger:# or self.args.debugger:
                status = self.run_debugger_cmd()
            else:
                status = self.run_cmdline()
        elapsed = (time.time()-start_time)

        if status == 0:
            status_string = 'SUCCESS'
        else:
            status_string = 'FAILED'

        result = {'status' : status, 
                  'status_string' : status_string, 
                  'test': file, 
                  'level' : "unknown", 
                  'seconds': elapsed, 
                  'iteration' : iteration,
                  'date_time' : date_time}
        self.test_results.append(result)
        print "test %s completed in %u seconds with status %s (%d)" % (file, elapsed, status_string, status)
        self.print_results()

        if status != 0 and self.args.stop:
            print "stop on failure is set.  Exit test"
            exit(1)
    def get_xml_files(self, directory):
        files = []
        for directory_file in os.listdir(directory):
           if (os.path.isfile(os.path.join(directory, directory_file))) and ".xml" in directory_file:
               print "found file: %s" % directory_file
               files.append(directory_file)
        return files

    def run(self):
        self.parse_cmdline()
        self.set_environment()
        #self.run_cmd("perl -v")
        for iteration in range(0, self.args.iterations):
            self.iteration_log_path = self.log_path + "\\iteration_" + str(iteration)
            os.makedirs(self.iteration_log_path)
            print "Iteration path is %s" % (self.iteration_log_path)
            if self.args.test_set_dir:
                files = self.get_xml_files(self.args.test_set_dir)
                #print files
                for test in files:
                    self.run_test_set(test, iteration)
            else:
                for l in self.test_levels:
                    print "\nstarting iteration (%u) of (%u) level: %s\n" % (iteration + 1, self.args.iterations, level_to_string(l)) 
                    for t in self.test_list:
                        self.run_test(t, l, iteration)
      
class writer(object):
    def __init__(self, *files):
        self.files = list(files)
        #print self.files

    def add(self, f):
        self.files.append(f)

    def write(self, txt):
        for fp in self.files:
            fp.write(txt)
            fp.flush()
    def flush(self):
        for fp in self.files:
            fp.flush()


if __name__ == "__main__":
    
    orig_out = sys.stdout
    orig_err = sys.stderr

    try:
        test_runner().run()
    except KeyboardInterrupt:
        print "*****************************"
        print "** Got CTRL-C; Shutting down"
        print "*****************************"
        exit(1)
    except Exception as e:
        traceback.print_exc()
        print e
    finally:
        sys.stdout = orig_out
        sys.stderr = orig_err
