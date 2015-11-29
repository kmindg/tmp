#!/usr/bin/python
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
import platform
from multiprocessing import Process, Lock

script_env = {}
script_env.update(os.environ)
is_running_in_x = True
def is_in_windows():
   sys_info = platform.system().lower()
   if sys_info == "windows":
      return True
   return False

def setup_environment():
   is_in_win = is_in_windows()
   if is_in_win:
      return

   global is_running_in_x
   script_dir = os.path.dirname(os.path.abspath(__file__))
   path = os.environ.get('PATH') or ""
   ld_path = os.environ.get('LD_LIBRARY_PATH') or ""
   os.environ['PATH'] = script_dir + ":" + path
   os.environ['LD_LIBRARY_PATH'] = script_dir + ":" + ld_path

   if os.path.exists("oldenv.txt"):
      with open('oldenv.txt', 'r') as f:
         line = f.readline()
         while line:
            env_pair=re.split('=', line)
            os.environ[env_pair[0]] = env_pair[1].strip('\n') 
            line = f.readline()
   script_env.update(os.environ)

   if sys.platform == "linux2":
      if not os.path.isdir('/corefiles') :
         os.mkdir('/corefiles')
      os.system('ulimit -c unlimited')
      os.system('echo 1 > /proc/sys/kernel/core_uses_pid')
      os.system('chmod 777 /corefiles')
      os.system('echo /corefiles/core-%e-%p-%t > /proc/sys/kernel/core_pattern')
      # we have to change the maximum socket buffer size on Linux, 
      # otherwise if we use SO_RCVBUF or SO_SNDBUF to set a big buffer, it will not take effect
      os.system('echo 5242880 > /proc/sys/net/core/rmem_max')
      os.system('echo 5242880 > /proc/sys/net/core/wmem_max')
      # rm the temp files under /dev/shm/ to release memory
      os.system('rm -f /dev/shm/*')

   try:
      p = subprocess.Popen(["xset", "-q"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
      p.communicate()
      if p.returncode != 0:
         print "Running in console, disable X now"
         is_running_in_x = False
   except Exception as e:
      is_running_in_x = False


class test_control:
   def __init__( self ):
      self._thread_num = 1         #default fbe_test number to start
      self._port_base = 13000      #default port_base
      self._iteration = 2          #default interations after failure
      self._rerun_flag = 'FAILED'  #default rerun_flag 
      self._last_test_num = 0 
      self._fbe_script_cmds = []  
      self._fbe_test_cmds = []  
      self._test_num_list = []
      self._fbe_test_list_content = ''
      self._all_tests_iteration = 1   # Number of times to iterate over all tests.
      self._debugger_flag = '-nodebugger'; 
      self._results_path = ""
      self._nogui_flag = False
      
   def set_thread_num ( self, thread_num ):
      self._thread_num = thread_num
   def get_thread_num( self ):
      return self._thread_num    
   def get_results_path( self ):
      return self._results_path    
   def set_results_path ( self, results_path):
      self._results_path = results_path
   def set_port_base ( self, port_base):
      self._port_base = port_base
   def get_port_base( self ):
      return self._port_base
   def set_all_tests_iteration ( self, all_tests_iteration ):
      self._all_tests_iteration = all_tests_iteration
   def get_all_tests_iteration( self ):
      return self._all_tests_iteration
   def set_iteration ( self, iteration ):
      self._iteration = iteration
   def get_iteration( self ):
      return self._iteration
   def set_rerun_flag ( self, rerun_flag):
      self._rerun_flag = rerun_flag
   def get_rerun_flag( self ):
      return self._rerun_flag
   def set_last_test_num ( self, last_test_num ):
      self._last_test_num = last_test_num
   def get_last_test_num( self ):
      return self._last_test_num
   def set_fbe_script_cmds ( self, fbe_script_cmds ):
      self._fbe_script_cmds = fbe_script_cmds
   def get_fbe_script_cmds( self ):
      return self._fbe_script_cmds
   def set_fbe_test_cmds ( self, fbe_test_cmds):
      self._fbe_test_cmds = fbe_test_cmds
   def get_fbe_test_cmds( self ):
      return self._fbe_test_cmds
   def set_test_num_list ( self, test_num_list ):
      self._test_num_list = test_num_list
   def get_test_num_list( self ):
      return self._test_num_list
   def set_fbe_test_list_content ( self, fbe_test_list_content):
      self._fbe_test_list_content = fbe_test_list_content
   def get_fbe_test_list_content( self ):
      return self._fbe_test_list_content
   def set_debugger_flag ( self ):
      self._debugger_flag = ' '    #  there is no debugger flag, just remove -nodebugger
   def get_debugger_flag( self ):
      return self._debugger_flag
   def set_nogui_flag ( self ):
      self._nogui_flag = True    #  not work on GUI mode
   def get_nogui_flag ( self ):
      return self._nogui_flag 
class test_result_summary:
     def __init__( self, date_time ):
         self._test_result = [] #a list holds test number, pass_no, iteraiton, and test status
         self._lock = Lock()
         self._log_dir = get_test_base_directory(date_time)
     def set_test_result ( self, test_result ):
         self._lock.acquire()
         self._test_result.append(test_result)
         self._lock.release()
         if test_result[3] != 'PASSED':
             self.write_summary_to_file(-1)
     def get_test_name_by_test_num ( self, test_num ):
         test_list_content = current_test_control.get_fbe_test_list_content()
         index = test_list_content.find('('+ str(test_num) + ')')
         next_right_bracket_index = test_list_content[index:].find(']')
         next_colon_index = test_list_content[(index+next_right_bracket_index+1):].find(':')
         return test_list_content[(index + next_right_bracket_index + 1): (index + next_right_bracket_index+1+ next_colon_index)]
     def write_summary_to_file (self, seconds):
         output_object = WritableObject()
         self._lock.acquire()
         sys.stdout = output_object
         if seconds != -1:
             print 'Elapsed Time: %2.0f seconds.' % seconds
         self.print_summary(1)
         sys.stdout = sys.__stdout__
         #print output_object.content
         file_name = self._log_dir + "summary.txt"

         fo = open(file_name, "w+")
         fo.seek(0,0)
         for line in output_object.content:
             fo.write( line )
         fo.close()
         self._lock.release()

     def print_summary( self, b_write_succes_summary ):
         last_test_result_dict = {} #a dictionary holds max (pass_no + interation) and test status
         passed_test = [] #a list holds test_num of passed tests
         failed_test = [] #a list holds test_num of failed tests
         self._test_result.sort()
          
         #get the last test stauts of the same test for different pass and iteration
         for test_num, pass_no, iteration, test_status, execution_time in self._test_result:

             # Populate dict for both success and failures.
             if not last_test_result_dict.has_key(test_num):
                 last_test_result_dict[test_num] = 0,0

             if (test_status == 'FAILED'):
                 num_failures,num_success = last_test_result_dict[test_num]
                 last_test_result_dict[test_num] = num_failures + 1, num_success
             else:
                 num_failures,num_success = last_test_result_dict[test_num]
                 last_test_result_dict[test_num] = num_failures, num_success + 1

         failed_test_num = 0
         passed_test_num = 0
         consistent_failures = 0
         inconsistent_failures = 0
         for test_num in last_test_result_dict:
             num_success = 0
             num_failed = 0
             test_status = 'PASSED'

             if last_test_result_dict.has_key(test_num):
                 num_failures, num_success = last_test_result_dict.get(test_num)
                 failed_test.append((test_num, num_failures, num_success))

                 if num_failures > 0:
                     failed_test_num = failed_test_num + 1 
                     if num_success > 0:
                         inconsistent_failures = inconsistent_failures + 1
                     else:
                         consistent_failures = consistent_failures + 1
                 else:
                     passed_test_num = passed_test_num + 1
                        
         print 'Summary:  TOTAL(' + str(failed_test_num + passed_test_num) + ')  PASSED(' + str(passed_test_num) + ')  FAILED(' + str(failed_test_num) + ')  CONSISTENT FAILURES(' + str(consistent_failures) + ')  INCONSISTENT FAILURES(' + str(inconsistent_failures) +')'
         
         if failed_test_num >0 :
             print 'Failed tests:'
             for failed_test_num,failed_passes,success_passes in failed_test:
                 total_passes = success_passes + failed_passes
                 if failed_passes > 0:
                     print '(' + str(failed_test_num) + ')' + self.get_test_name_by_test_num(failed_test_num) + ' failed (' + str(failed_passes) + '/' + str(total_passes) + ')'

         # Only conditionally write the full summary
         if b_write_succes_summary == 1:
             print("\n\n")
             print("All test results and times:")
             print("--------------------------------------------------------------------------------------------------")
             for test_num, pass_no, iteration, test_status, execution_time in self._test_result:
                 elapsed = "%2.2f s" % execution_time
                 name = " %55s " % self.get_test_name_by_test_num(test_num)
                 print '(' + str(test_num) + ')' + str(name) + " " + str(test_status) + " pass " + str(pass_no) + " " + str(elapsed)

class test_detailed_results:

   def __init__( self, results_directory, script_directory ):
       self._path = results_directory

       # We make the assumption that the script directory and the symbols are in the same place.
       self._script_directory = script_directory
       self._symbol_path = script_directory

   def print_test_status(self, file_name, file_path):
    
       file = open(file_path)
       window_lines = 10
       text = file.read()
       lines = text.splitlines()
       line_num = 0
       max_line = 0
    
       if ("AssertFailed " not in text and "MUT_LOG" not in text and "CSX RT" not in text):
           #print "Test saw no errors"
           return 0
    
       print "\n\n***********************************"
       print "Test Errors: " + file_path
       print "***********************************"

       for line in lines:
           if ((line.find("AssertFailed") >= 0) or (line.find("MUT_LOG") >= 0) or (line.find("CSX RT") >= 0)):
               if (line_num >= max_line):
                   for print_line in lines[line_num - window_lines:line_num + window_lines]:
                       print print_line
                       max_line = line_num + window_lines
               elif ((line_num + window_lines) > max_line):
                   for print_line in lines[max_line:line_num + window_lines]:
                       print print_line
                       max_line = line_num + window_lines
                        
           line_num = line_num + 1
       return 1 # we saw errors

   def print_error_traces(self, file_name, file_path):
    
       file = open(file_path)
       window_lines = 10
       text = file.read()
       lines = text.splitlines()
       line_num = 0
       max_line = 0
       max_errs = 15
    
       if ("ERR " not in text):
           #print "No error traces found"
           return 0
    
       print "\n\n***********************************"
       print "SP Error Traces: " + file_path
       print "***********************************"
    
       err_cnt = 0
       for line in lines:
           if (line.find("ERR ") >= 0):
               if (line_num >= max_line):
                   for print_line in lines[line_num - window_lines:line_num + window_lines]:
                       print print_line
                       max_line = line_num + window_lines
                   err_cnt = err_cnt + 1
               elif ((line_num + window_lines) > max_line):
                   for print_line in lines[max_line:line_num + window_lines]:
                       print print_line
                       max_line = line_num + window_lines
                   err_cnt = err_cnt + 1
           if err_cnt > max_errs:
               return
           line_num = line_num + 1    
       return 1 # we saw errors    

   def get_failed_tests(self):

       path = self._path

       number_list = []
       directory_list = []
       directory_sorted_list = []
       #print "path: " + path
       for directory_file in os.listdir(path):
           if (not os.path.isfile(os.path.join(path, directory_file))) and "FAILED" in directory_file:

               split_name = re.split("_", directory_file)
               #print " [" + directory_file + "] "
               #print split_name
               if (split_name[0] == "iteration"):
                   #print "found iteration for test " + str(split_name[1])
                   number_list.append(int(split_name[1]))
                   #print number_list
               else:
                   number_list.append(int(split_name[0]))

               directory_list.append(directory_file)
       
       # remove duplicates first.
       number_set = set(number_list)
       number_list = list(number_set)
  
       number_list.sort()
       #print number_list

       # Generate a list of directories
       for number in number_list:
           for directory_file in directory_list:
               split_name = re.split("_", directory_file)
               if split_name[0] == str(number):
                   #print "found " + directory_file
                   directory_sorted_list.append(directory_file)
               elif (split_name[0] == "iteration") and (split_name[1] == str(number)):
                   directory_sorted_list.append(directory_file)
       return directory_sorted_list

   def print_dump_information(self, dump_file, dump_path):

       # For now we will skip cracking the dump on linux, since this is platform specific.
       if sys.platform == "linux2":
           return

       # Open the dump and run a series of commands to get a stack trace.
       command = 'cdb -z ' + dump_path + ' -logo dump1.txt -c ".reload /i ; .ecxr ; kc; q" -y ' + self._symbol_path

       # Redirect output to the dev null and call the command.  The below is cross platform compatible.
       dev_null = open(os.devnull, 'w')
       env = script_env.copy()
       error = subprocess.call(command, shell=True, stdout=dev_null, env=env)

       if error != 0:
           print "\n\n***********************************"
           print "Error in fbe_test.py \n"
           print "error " + str(error) + " Unable to open dump " + dump_path + "\n"
           print "Please confirm that CDB is in your path\n"
           print "***********************************"
           return
       print "\n\n***********************************"
       print "SP Dump Information: " + dump_path
       print "***********************************"

       fo = open("dump1.txt", "r+")
       fo.seek(0,0)
       b_display_lines = 0
       for line in fo:
           if b_display_lines == 1 or "Call Site" in line:
               if line != "\n" and line != "\r\n" and line != "\n\r":
                   print line
                   b_display_lines = 1

       fo.close()

   def process_failed_tests(self, directory_sorted_list):
       path = self._path
       # Process the directories.
       for directory_file in directory_sorted_list:
           #print directory_file
           if (not os.path.isfile(os.path.join(path, directory_file))) and "FAILED" in directory_file:
        
               failed_directory = os.path.join(path, directory_file)
               for filename in os.listdir(failed_directory):
                   if ".txt" in filename:
                       self.print_test_status(filename, os.path.join(failed_directory, filename))
        
               for filename in os.listdir(failed_directory):
                   # Only show error traces if the test failed.
                   if "_spa.dbt" in filename:
                       self.print_error_traces(filename, os.path.join(failed_directory, filename))
            
                   if "_spb.dbt" in filename:
                       self.print_error_traces(filename, os.path.join(failed_directory, filename))

                   if "_spa.dmp" in filename:
                       self.print_dump_information(filename, os.path.join(failed_directory, filename))

                   if "_spb.dmp" in filename:
                       self.print_dump_information(filename, os.path.join(failed_directory, filename))

   def write_detailed_summary(self):
       file_name = self._path + "\\results_detailed.txt"
       print("Generating detailed results to: " + file_name)

       output_object = WritableObject()
       sys.stdout = output_object

       sorted_test_list = []
       sorted_test_list = self.get_failed_tests()
       #print "sorted_test_list"
       #print sorted_test_list
       self.process_failed_tests(sorted_test_list)


       fo = open(file_name, "w+")
       fo.seek(0,0)
       for line in output_object.content:
           fo.write( line )
       fo.close()

       sys.stdout = sys.__stdout__

def usage():
    print 'Usage:    python fbe_test.py [options] argument'
    print 'Options:'
    print '    -h, -help          show this help message and exit'
    print '    -n, -num           set the number of fbe_test.exe to run'
    print '    -p, -port_base     set the port base of SP and CMI'
    print '    -t, -tests         set the testes to run'
    print '                       Specify tests by test number,name and test sutite name'
    print '                       Example: -t "1,10-20,suitename1,testname1,testname2"'
    print '    -a, -all_tests_iterations    set number of interations to run all tests'
    print '    -i, -iterations    set interation to run a single test'
    print '                       combine with rerun flag to rerun tests'
    print '    -r, -rerun_flag    set rerun_flag to run'
    print '                       valid arguments are PASSED or FAILED'
    print '    -[fbe cmd options] pass options to fbe_test'
    print '                       valid arguments are any fbe_test cmd arguments'
    print '                       Example: -rdgen_peer_option PEER_ONLY'
    print '    -debugger          launch into the debugger when a test faults'
    print '    -detailed_results [path] Just generate detailed results for a given test folder'
    print ''
    print 'Examples:'
    print '    print help message:'
    print '        python fbe_test.py -h or python fbe_test.py -help'
    print '    start 3 fbe_test:'
    print '        python fbe_test.py -n 3  or python fbe_test.py -num 3'
    print '    specify tests to run by test number, name and test suite name:'
    print '        python fbe_test.py  -t "13,27-29,sobo_4,fbe_cli_test_suite"'
    print '    specify 2 fbe_test options raid_test_level, and raid0_only: '
    print '        python fbe_test.py -raid_test_level 2 -raid0_only'
    print '    Rerun fbe_tests for 2 iterations while test is passed: '
    print '        python fbe_test.py -i 2 -r PASSED'
    print '                       -t "sep_test_suite" will run all sep test suites including the dualsp tests'
    print '                       -t "sep_dualsp_test_suite" will simply run the sep dualsp test suites'
    print ''


def get_fbe_test_list_content():
    if sys.platform == "linux2":
        if (os.path.isfile("run_simulation.sh")):
            list_cmd = ['run_simulation.sh', 'fbe_test.exe', '-list']
        else:
            list_cmd = ['fbe_test.exe', '-list']

        env = script_env.copy()
        proc = subprocess.Popen(list_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env)
        fbe_test_list_content, stderrdata = proc.communicate()
        # print("stdout = %s" % fbe_test_list_content)
        # print("stderr = %s" % stderrdata)
    else:
        list_cmd = ['fbe_test.exe' , '-list']
        #print list_cmd
        proc = subprocess.Popen(list_cmd, shell=True, stdout=subprocess.PIPE)
        fbe_test_list_content = proc.communicate()[0]

    
    #print fbe_test_list_content
    last_left_bracket = fbe_test_list_content.rfind('(')
    last_right_bracket = fbe_test_list_content.rfind(')')
    last_test_num_string = fbe_test_list_content[last_left_bracket+1:last_right_bracket]
    #print last_test_num_string
    last_test_num = int(last_test_num_string)
    current_test_control.set_last_test_num(last_test_num)
    current_test_control.set_fbe_test_list_content(fbe_test_list_content)
    return fbe_test_list_content


def get_test_num_of_suites(fbe_test_list_content):
    test_num_of_suites = {}
    test_suite_names = ['fbe_cli_test_suite','physical_package_test_suite','sep_test_suite','esp_test_suite','system_test_suite']

    # Find the list of all the test suites.
    test_suite_names = re.findall('\w+test_suite', fbe_test_list_content)

    start_number = 0
    end_number = 0
    for i in range(0, len(test_suite_names)):
      
        index = fbe_test_list_content.find(test_suite_names[i])
        last_left_bracket = fbe_test_list_content[index:].find('(')
        last_right_bracket = fbe_test_list_content[index:].find(')')
        start_number = fbe_test_list_content[index+last_left_bracket+1:index+last_right_bracket]
        if( test_suite_names[i] == 'system_test_suite'):
             test_num_of_suites[test_suite_names[i]] = str(start_number) + '-' + str(current_test_control.get_last_test_num())
             break
     
        index = fbe_test_list_content.find(test_suite_names[i+1])
        last_left_bracket = fbe_test_list_content[index:].find('(')
        last_right_bracket = fbe_test_list_content[index:].find(')')
        end_number = fbe_test_list_content[index + last_left_bracket+1:index+last_right_bracket]

        test_num_of_suites[test_suite_names[i]] = str(start_number) + '-' + str(int(end_number)-1)

    return  test_num_of_suites


def set_tests(test_cmds_value, test_list_content):
     test_num_list = []
     test_num_of_suites = get_test_num_of_suites(test_list_content)
     test_num_set = set()
     values = test_cmds_value.replace(' ','').split(',')
     for value in values:
        if(value.isdigit()):
               test_num_set.add(int(value))
        elif(value[0].isdigit() and value[len(value)-1].isdigit() and (value.find('-') != -1)):
              index = value.find('-')
              min_value = int(value[0:index])
              max_value = int (value[index+1:])
              for i in range(min_value,max_value+1):
                  test_num_set.add(i)
        elif(value.find('sep_test_suite') != -1 ):
              # Find the list of all the sep test suites.
              test_suite_names = re.findall('sep_\w+test_suite', test_list_content)
              #print test_suite_names 
              for suite_name in test_suite_names:
                  #handle 2 scenarios: test suite name is valid or not
                  if suite_name in test_num_of_suites.keys():
                      index = test_num_of_suites[suite_name].find('-')
                      min_value = int(test_num_of_suites[suite_name][0:index])
                      max_value = int (test_num_of_suites[suite_name][index+1:])
                      for i in range(min_value,max_value+1):
                        test_num_set.add(i)
                  else:
                      test_num_set.add(-1)
        elif(value.find('sep_dualsp_test_suite') != -1 ):
              # Find the list of all the sep dual sp test suites.
              test_suite_names = re.findall('sep_dualsp\w+test_suite', test_list_content)
              #print test_suite_names 
              for suite_name in test_suite_names:
                  #handle 2 scenarios: test suite name is valid or not
                  if suite_name in test_num_of_suites.keys():
                      index = test_num_of_suites[suite_name].find('-')
                      min_value = int(test_num_of_suites[suite_name][0:index])
                      max_value = int (test_num_of_suites[suite_name][index+1:])
                      for i in range(min_value,max_value+1):
                        test_num_set.add(i)
                  else:
                      test_num_set.add(-1)
        elif(value.find('sep_singlesp_test_suite') != -1 ):
              print "found -t sep_singlesp_test_suite"
              # Find the list of all the sep dual sp test suites.
              dualsp_test_suite_names = re.findall('sep_dualsp\w+test_suite', test_list_content)
              #print dualsp_test_suite_names
              singlesp_test_suite_names = re.findall('sep_\w+test_suite', test_list_content)
              #print singlesp_test_suite_names 
              for suite_name in singlesp_test_suite_names:
                  #handle 2 scenarios: test suite name is valid or not
                  #print suite_name
                  if suite_name in test_num_of_suites.keys():
                      if suite_name not in dualsp_test_suite_names:
                          index = test_num_of_suites[suite_name].find('-')
                          min_value = int(test_num_of_suites[suite_name][0:index])
                          max_value = int (test_num_of_suites[suite_name][index+1:])
                          #print suite_name + " add min" + str(min_value) + "max " + str(max_value)
                          for i in range(min_value,max_value+1):
                              test_num_set.add(i)
                  #else:
                      #test_num_set.add(-1)
              #exit(0)
        elif(value.find('_test_suite') != -1 ):
              print("Running: " + str(value))
              #hanlde 2 scenarios: test suite name is valid or not
              if value in test_num_of_suites.keys():
                 index = test_num_of_suites[value].find('-')
                 min_value = int(test_num_of_suites[value][0:index])
                 max_value = int (test_num_of_suites[value][index+1:])
                 for i in range(min_value,max_value+1):
                     test_num_set.add(i)
              else:
                  print(value + " not in num_of_suites")
                  print test_num_of_suites.keys()
                  test_num_set.add(-1)

        else:
              index = test_list_content.find(value)
              #hanlde 2 scenarios: test name is valid or not
              if index != -1:
                  last_left_bracket = test_list_content[0:index].rfind('(')
                  last_right_bracket = test_list_content[0:index].rfind(')')
                  last_test_num_string = test_list_content[last_left_bracket+1:last_right_bracket]
                  last_test_num = int(last_test_num_string)
                  test_num_set.add(last_test_num)
              else:
                  test_num_set.add(-1)

     test_num_list = list(test_num_set)
     test_num_list.sort()
     current_test_control.set_test_num_list(test_num_list)

def show_help_message():
    usage()
    sys.exit(2)
    
def set_thread_num(thread_num_value):
     if thread_num_value.isdigit():
        current_test_control.set_thread_num(int(thread_num_value))
     else:
        print thread_num_value + ' is not a valid argument of n/num'
        sys.exit(2)

def set_results_path(results_path):
    current_test_control.set_results_path(str(results_path))

def set_port_base(port_base_value):
    if port_base_value.isdigit():
        current_test_control.set_port_base(int(port_base_value))
    else:
        print port_base_value + ' is not a valid argument of p/port_base'
        sys.exit(2)

def set_tests_by_cmd_values(test_cmds_value):
    set_tests(test_cmds_value,current_test_control.get_fbe_test_list_content())

    
def set_iteration(test_cmds_value):
    if test_cmds_value.isdigit():
        current_test_control.set_iteration(int(test_cmds_value))
    else:
         print test_cmds_value + ' is not a valid argument of interations'
         sys.exit(2)
         
def set_all_tests_iteration(test_cmds_value):
    if test_cmds_value.isdigit():
        current_test_control.set_all_tests_iteration(int(test_cmds_value))
    else:
         print test_cmds_value + ' is not a valid argument of all_tests_iteration'
         sys.exit(2)
    
def set_rerun_flag(test_cmds_value):
    current_test_control.set_rerun_flag(test_cmds_value)

def set_debugger_flag():
    current_test_control.set_debugger_flag()

def set_nogui_flag():
    current_test_control.set_nogui_flag()

def parse_cmd_options():
     fbe_script_cmds = []
     fbe_test_cmds = []
      
     script_cmd_actions = {
     '-h': show_help_message,
     '-help': show_help_message,
     '-n': set_thread_num,
     '-num': set_thread_num,
     '-num_threads': set_thread_num,
     '-p': set_port_base,
     '-port_base': set_port_base,
     '-t': set_tests_by_cmd_values,
     '-tests': set_tests_by_cmd_values,
     '-detailed_results': set_results_path,
     '-i': set_iteration,
     '-iterations': set_iteration,
     '-a': set_all_tests_iteration,
     '-all_tests_iterations': set_all_tests_iteration,
     '-r': set_rerun_flag,
     '-rerun_flag': set_rerun_flag,
     '-debugger': set_debugger_flag,
     '-nogui': set_nogui_flag}

     nameList = sys.argv[1:]

     for nameIndex in range(len(nameList)):
          #skip cmd value
          #print 'nameIndex is ' + nameIndex
          if not nameList[nameIndex].startswith('-'):
              continue;
          if nameList[nameIndex] in ('-h','-help','-n','-num', '-num_threads', '-p','-port_base','-t', '-tests','-a','-all_tests_iterations','-i','-iterations','-r','-rerun_flag', '-debugger', '-detailed_results', '-nogui'):
              #parse cmd options of this script 
              if nameIndex+1 >= len(nameList) or nameList[nameIndex+1].startswith('-'):
                  fbe_script_cmds.append( nameList[nameIndex])
                  script_cmd_actions.get(nameList[nameIndex])()
              else:
                  fbe_script_cmds.append( nameList[nameIndex] + ' ' + nameList[nameIndex+1])
                  script_cmd_actions.get(nameList[nameIndex])(nameList[nameIndex+1])
          else:
              #parse fbe_test cmd options
              if nameIndex+1 >= len(nameList) or nameList[nameIndex+1].startswith('-'):
                  fbe_test_cmds.append(nameList[nameIndex])
              else:
                  fbe_test_cmds.append( nameList[nameIndex] + ' ' + nameList[nameIndex+1])
        
     current_test_control.set_fbe_script_cmds(fbe_script_cmds)
     current_test_control.set_fbe_test_cmds(fbe_test_cmds)


def create_test_result_directory():
    if not os.path.isdir('fbe_test_results') :
        os.mkdir('fbe_test_results')
    if sys.platform == "linux2":
        os.mkdir('fbe_test_results/' + currentDateTime)
        os.mkdir('fbe_test_results/' + currentDateTime + "/details")
    else:
        os.mkdir('fbe_test_results\\' + currentDateTime)
        os.mkdir('fbe_test_results\\' + currentDateTime + "\\details")

class TestThread(threading.Thread):
    """Test thread to start fbe_test.exe"""
    def __init__(self, queue,port_base, thread_num, lock, test_summary):
        threading.Thread.__init__(self)
        self.queue = queue
        self._port_base = port_base
        self._thread_num = thread_num
        self._lock = lock
        self._test_summary = test_summary
        
    def completion_msg(self, msg_string, start_time, test_num, execution_time ):
        self._lock.acquire()

        now = datetime.datetime.now()
        date_time_string = now.strftime("%m/%d/%Y %H-%M-%S")
        elapsed = " TestTime: %2.2f s ,  Total Elapsed Time: %2.0f" % (execution_time,(time.time() - start_time))
        print date_time_string + " Thread: " + str(self._thread_num) + " " + msg_string + "(" + str(test_num) + ")" + self._test_summary.get_test_name_by_test_num(test_num)+ elapsed + "s"

        self._lock.release()

    def run(self):
        start_time = time.time()
        
        while True:
                testNumber, repeat_index = current_thread_control.get_next(self._thread_num)
                #testNumber, repeat_index = self.queue.get()
                
                if testNumber == -1:
                     #print "thread " + str(self._thread_num) + " complete."
                     return
                self._lock.acquire()
                now = datetime.datetime.now()
                current_date_time = now.strftime("%m/%d/%Y %H-%M-%S")
                if repeat_index > 0:
                    print current_date_time + " Thread: " + str(self._thread_num) + " Starting " + "(" + str(testNumber) + ")" + self._test_summary.get_test_name_by_test_num(testNumber) + " iteration: " + str(repeat_index)
                else:
                    print current_date_time + " Thread: " + str(self._thread_num) + " Starting " + "(" + str(testNumber) + ")" + self._test_summary.get_test_name_by_test_num(testNumber)
                self._lock.release()

                test_status,execution_time = run_fbe_test(testNumber,self._test_summary.get_test_name_by_test_num(testNumber),self._port_base,1, repeat_index, self._test_summary)

                self._test_summary.set_test_result([testNumber,1,repeat_index,test_status,execution_time])
                self.completion_msg((test_status + "   "), start_time, testNumber, execution_time)
                
                #rerun fbe test for specified iternrations while rerun flag is failed or passed
                if test_status == current_test_control.get_rerun_flag().upper():
                     for i in range(1,current_test_control.get_iteration()):

                         self._lock.acquire()
                         print current_date_time + " Thread: " + str(self._thread_num) + " Re-Running " + "(" + str(testNumber) + ")" +  self._test_summary.get_test_name_by_test_num(testNumber) 
                         self._lock.release()
                         
                         test_status,execution_time = run_fbe_test(testNumber,self._test_summary.get_test_name_by_test_num(testNumber),self._port_base,i+1, repeat_index, self._test_summary)
                         self.completion_msg((test_status + "   "), start_time, testNumber, execution_time)
                         self._test_summary.set_test_result([testNumber,i+1,repeat_index,test_status, execution_time])

                current_thread_control.test_finished(testNumber);

                self.queue.task_done()

def run_fbe_test2(index, name, port_base, pass_no, iteration, summary):
    """This is for testing fbe test failures"""
    if (index != 0) and (index % 3) == 0:
        return 'FAILED', 10
    if (index % 2) or (pass_no > 1):
        return 'PASSED', 5
    return 'FAILED', 10

def get_test_base_directory(date):
    path_separator = '\\'
    if sys.platform == "linux2":
        path_separator = '/'
    
    base_dir = 'fbe_test_results' + path_separator + date  + path_separator
    return base_dir

def run_fbe_test(index, name, port_base, pass_no, iteration, summary):
    path_separator = '\\'
    if sys.platform == "linux2":
        path_separator = '/'
    
    test_name = summary.get_test_name_by_test_num(index)
    test_name = test_name.lstrip()
    test_name = '_' + test_name

    base_log_dir = 'fbe_test_results' + path_separator + currentDateTime + path_separator
    nodebugger_string = current_test_control.get_debugger_flag()


    # The repeat string is only used when we have an iteration
    if (iteration == 0):
        repeat_string = ""
    else:
        repeat_string = "iteration_" + str(iteration) + "_test_"
    
    current_log_dir = base_log_dir + 'details'+ path_separator + str(index) + path_separator + repeat_string + 'Pass' + str(pass_no) + test_name
    #fbe_test specifies it's own timeout.  set the null string here in case it needs to be overridden by -rtl 1 
    timeout = ''     
    # Check if we need a longer timeout.
    for fbe_test_option in current_test_control.get_fbe_test_cmds():
        
        if ((fbe_test_option.find('-rtl') == 0) or (fbe_test_option.find('-raid_test_level') == 0)):
             #print "raid test level found, using timeout of 3600"
             timeout = '-timeout 3600'
             #there was a 'break' here, but I removed it.  Why skip processing the rest of the arguments?
             #also, this timeout will be overriden by a user supplied '-timeout' because the user value will hit mut arg processing second.

    #add cmd options
   
    if sys.platform == "linux2":
        current_log_dir = current_log_dir.replace('\\', '/')
        if (os.path.isfile("run_simulation.sh")):
            fbe_cmd = ['run_simulation.sh', 'fbe_text.exe']
        else:
            fbe_cmd = ['fbe_test.exe']
        raw_cmd = [fbe_cmd, str(index),
                   '-port_base', str(port_base),
                   '-logDir', current_log_dir,
                   '-text',
                   '-failontimeout',
                  timeout,
                   '-sp_log', 'both']

        if is_running_in_x and not current_test_control.get_nogui_flag():
           cmd = ['xterm -iconic -sb -sl 50000 -e'] + raw_cmd
        else:
           cmd = raw_cmd + ['-nogui']
    else:
        cmd = ['start',
           "\""+name+"\"", 
           '/MIN /WAIT fbe_test.exe' , \
            str(index), \
            '-port_base', str(port_base), \
            '-logDir', current_log_dir, \
            '-text', \
            '-failontimeout', \
            timeout, \
            '-sp_log', 'both']
    

    cmd.append(nodebugger_string);
    for option in current_test_control.get_fbe_test_cmds():
        for value in option.split(' '):
             cmd.append(value)

    cmd_string = ""
    for cmd_str in cmd:
        cmd_string += ''.join(cmd_str)
        cmd_string += " "
    if not is_running_in_x or current_test_control.get_nogui_flag():
        cmd_string += " &> /dev/null"

    if sys.platform == "linux2":
        base_log_dir = base_log_dir.replace('\\', '/')
    
    if not os.path.isdir(base_log_dir + 'details') :
        os.makedirs(base_log_dir + 'details')
    
    if not os.path.isdir(current_log_dir) :
        os.makedirs(current_log_dir)

    #execute fbe_test.exe
    #subprocess.call(cmd)
    elapsed = time.time()
    #print cmd_string
    os.system(cmd_string)
    elapsed = (time.time()-elapsed)


    test_status = 'PASSED'
    log_files = os.listdir(current_log_dir)
    duration = ""
    if log_files == []:
       print "Please verify the input options, at least one of them is invalid"
       test_status = 'FAILED'
    for f in log_files :
       s = os.path.getsize(current_log_dir + path_separator + f)
       if s == 0 :
           os.remove(current_log_dir + path_separator + f)
           continue
       else :
           if f.find('.txt') >= 0:
               infile = open(current_log_dir + path_separator + f,'r')
               text = infile.read()
               infile.close()
               if (text.find('AssertFailed') >= 0) or (text.find('TestFailed') >= 0):
                   test_status = 'FAILED'
               if (text.find("Duration:") == 0):
                   test_status = 'FAILED'
               break
       
    #Take care of log files
    log_files = os.listdir(current_log_dir + path_separator)
    if pass_no == 1:        
        if test_status == 'FAILED':
            os.mkdir(base_log_dir + repeat_string + str(index) + "_" + test_status + '_' + 'Pass' + str(pass_no) + test_name) 
            for f in log_files :
                shutil.copyfile(current_log_dir + path_separator + f, base_log_dir + repeat_string + str(index) + "_" + test_status + '_' + 'Pass' + str(pass_no) + test_name + path_separator + f)
        else:
            if not os.path.isdir(base_log_dir + repeat_string + str(index) + "_" + test_status):
                os.mkdir(base_log_dir + repeat_string + str(index) + "_" + test_status + test_name)
    else:
        if not os.path.isdir(base_log_dir + repeat_string + str(index) + "_" + test_status+ '_' + 'Pass' + str(pass_no)) :
            os.mkdir(base_log_dir + repeat_string + str(index) + "_" + test_status + '_' + 'Pass' + str(pass_no) + test_name) 
        if test_status == 'FAILED':
            for f in log_files :
               shutil.copyfile(current_log_dir + path_separator + f, base_log_dir + repeat_string + str(index) + "_"+ test_status + '_' + 'Pass' + str(pass_no) + test_name + path_separator + f)
    return test_status, elapsed

class thread_control:
   def __init__( self, test_control, queue ):
      self._lock = Lock()
      self._test_suite_threads = {}
      self._test_suite_threads[7] = ['sep_degraded_io_test_suite']
      self._test_suite_threads[7] += ['sep_normal_io_test_suite', 'sep_disk_errors_test_suite']
      self._test_suite_threads[5] = ['sep_dualsp_normal_io_test_suite', 'sep_dualsp_disk_errors_test_suite']
      self._test_suite_threads[3] = ['sep_dualsp_disk_errors_test_suite', 'sep_dualsp_degraded_io_test_suite']
      if not is_in_windows():
            # not enough memory on KH VM
            self._test_suite_threads[3] += ['sep_dualsp_encryption_test_suite']
      self._test_suite_threads[9]= ['sep_configuration_test_suite', 'sep_services_test_suite', 'sep_rebuild_test_suite', 'sep_sparing_test_suite', 'sep_verify_test_suite', 'sep_verify_test_suite', 'sep_zeroing_test_suite', 'sep_sniff_test_suite', 'sep_power_savings_test_suite', 'sep_special_ops_test_suite']
      self._test_suite_threads[7] += ['sep_dualsp_configuration_test_suite', 'sep_dualsp_services_test_suite', 'sep_dualsp_rebuild_test_suite', 'sep_dualsp_sparing_test_suite', 'sep_dualsp_verify_test_suite', 'sep_dualsp_verify_test_suite', 'sep_dualsp_zeroing_test_suite', 'sep_dualsp_sniff_test_suite', 'sep_dualsp_power_savings_test_suite', 'sep_dualsp_special_ops_test_suite']
      self._num_running_threads = 0
      self._max_num_running_threads = 1
      self._test_control = test_control
      self._queue = queue
      self._default_num_threads = 1
      self._test_counts = []
      self._block = 0 # Set to 1 to block all other threads.
      self._dbg_enabled = 0 # Set to 1 to enable more tracing.
   def set_thread_num ( self, thread_num ):
      self._thread_num = thread_num
   def get_thread_num( self ):
      return self._thread_num    
   def set_port_base ( self, port_base):
      self._port_base = port_base 
   def get_current_threads(self):
      return self._num_running_threads
   def set_max_threads(self, max_threads):
       self._max_num_running_threads = max_threads
   def set_running_threads(self, threads):
       self._num_running_threads = threads
   def set_default_num_threads(self, threads):
       self._default_num_threads = threads
       for i in range(0, threads):
           self._test_counts += [0]
   def dec_current_threads(self):
       self._num_running_threads = self._num_running_threads - 1 
   def inc_current_threads(self):
       self._num_running_threads = self._num_running_threads + 1 
   def get_threads_for_test(self, test_num):
       for threads in self._test_suite_threads.keys():
           thread_info = self._test_suite_threads[threads]
           for suite_name in thread_info:
               if self.is_in_suite(test_num, suite_name):
                   if threads > self._default_num_threads:
                       threads = self._default_num_threads
                   self.dbgprint("found threads " + str(threads) + " for " + suite_name)
                   return threads
       return self._default_num_threads
   def dbgprint (self, string):
       """Set the trace level to enable debug printing."""
       if self._dbg_enabled == 1:
           print string
       return
   def is_in_suite(self, test_num, suite_name):
       test_num_of_suites = get_test_num_of_suites(self._test_control.get_fbe_test_list_content())
       # Find the list of all the sep dual sp test suites.
       if suite_name in test_num_of_suites.keys():
           index = test_num_of_suites[suite_name].find('-')
           min_value = int(test_num_of_suites[suite_name][0:index])
           max_value = int (test_num_of_suites[suite_name][index+1:])
           #print suite_name + " add min" + str(min_value) + "max " + str(max_value)
           if (test_num >= min_value) and (test_num <= max_value):
               self.dbgprint("found test " + str(test_num) + " in " + str(suite_name))
               return 1
           #else:
               #self.dbgprint("did not find test_num " + str(test_num) + " in " + str(suite_name) + " " + str(min_value) + " to " + str(max_value))
       return 0

   def wait_for_run_allowed_and_for_start(self, thread_num):
       """Wait for the correct number of threads.  Also wait for other threads to start which is indicated by block."""
       while (self._num_running_threads + 1 > self._max_num_running_threads) or (self._block != 0):
           self.dbgprint("thread " + str(thread_num) + " waiting current: " + str(self._num_running_threads) + " max:" + str(self._max_num_running_threads))
           self._lock.release()
           time.sleep(1)
           self._lock.acquire()
       self.dbgprint("thread " + str(thread_num) + " starting current: " + str(self._num_running_threads) + " max:" + str(self._max_num_running_threads))

   def wait_until_run_allowed(self, thread_num):
       """Wait until the threads are the correct number.  Do not wait for block since we set it."""
       while (self._num_running_threads + 1 > self._max_num_running_threads):
           self.dbgprint("thread " + str(thread_num) + " waiting current: " + str(self._num_running_threads) + " max:" + str(self._max_num_running_threads))
           self._lock.release()
           time.sleep(1)
           self._lock.acquire()
   def refresh_max_running_threads(self):
       # Find the new max running threads which will be the first
       # thread we find with the lowest max allowed threads.
       for i in range(0, self._default_num_threads):
           if (self._test_counts[i] != 0):
               if (i != self._max_num_running_threads - 1):
                   self._max_num_running_threads = i + 1
                   if self._num_running_threads != 0:
                      print "set max threads to " + str(i + 1) + " current: " + str(self._num_running_threads)
               return

   def test_finished(self, test_number):
       self._lock.acquire()
       max_threads = self.get_threads_for_test(test_number) - 1
       self.dbgprint("test " + str(test_number) + " finished " + str(max_threads))
       self._test_counts[max_threads] = self._test_counts[max_threads] - 1
       #print "current running threads: " + str(self._test_counts)

       self.refresh_max_running_threads()
       self._lock.release()

   def test_started(self, test_number):
       """Assumes lock is held. """
       #print "test_started " + str(test_number)
       max_threads = self.get_threads_for_test(test_number) - 1
       self.dbgprint("test " + str(test_number) + " started " + str(max_threads + 1))
       self._test_counts[max_threads] = self._test_counts[max_threads] + 1
       self.refresh_max_running_threads()
   def get_next(self, thread_num):
       self._lock.acquire()
       self.dec_current_threads()

       # Wait until there are the right number of threads running.
       self.wait_for_run_allowed_and_for_start(thread_num)

       if self._queue.empty():
           self._lock.release()
           return -1, -1
       # Prevent other threads are running until we get the item off the queue and start running it.
       # This helps us in the case where we pull something off the queue but we can't run it yet.  
       # This prevents other threads from starting items out of order.
       # This helps us in the case where we are going from a high number of threads to a lower number of threads.
       self._block = 1

       # Allow us to proceed
       # Get the next item to process.
       testNumber, repeat_index = self._queue.get()
       self.test_started(testNumber)
       max_threads = self.get_threads_for_test(testNumber)

       self.dbgprint("thread " + str(thread_num) + " test: " + str(testNumber) + " test max: " + str(max_threads) + " current: " + str(self._num_running_threads) + " max:" + str(self._max_num_running_threads))
       # Wait until there are the right number of threads running.
       self.wait_until_run_allowed(thread_num)


       self.inc_current_threads()
       self._block = 0
       self._lock.release()

       #time.sleep(1)
       self.dbgprint( "get next done " + str(testNumber) + " " + str(repeat_index))
       return testNumber, repeat_index

def get_non_exception_suites(suite_threads_list):
    test_num_of_suites = get_test_num_of_suites(current_test_control.get_fbe_test_list_content())
    new_list = set()
    new_list_index = 0
    ne_suites = [val for val in test_num_of_suites.keys() if val not in suite_threads_list]
    print ne_suites
    for suite_name in test_num_of_suites.keys():
        for suite_threads in suite_threads_list:
            if suite_threads[0] != suite_name:
                index = test_num_of_suites[suite_name].find('-')
                min_value = int(test_num_of_suites[suite_name][0:index])
                max_value = int (test_num_of_suites[suite_name][index+1:])
                print "adding " + str(min_value) + " " + str(max_value) + " " + suite_threads[0]
                for i in range(min_value,max_value):
                    new_list.add(i)
            else:
                print "skipping " + suite_threads[0]
    print new_list
    test_num_list = list(new_list)
    test_num_list.sort()
    return test_num_list

class WritableObject:
    def __init__(self):
        self.content = []
    def write(self, string):
        self.content.append(string)

def main():
    test_script_directory = os.path.dirname(__file__) 
    print "Test script directory is: " + test_script_directory

    get_fbe_test_list_content()
    parse_cmd_options()

    if (current_test_control.get_results_path() != ""):
        detailed_results = test_detailed_results(current_test_control.get_results_path(), test_script_directory)
        detailed_results.write_detailed_summary()
        return
    start = time.time()
    lock = Lock() #Used to serialize output

#   if sys.platform == "linux2":
#       if (os.path.isfile("run_simulation.sh")):
#           run_csx_helper_cmd = ['run_simulation.sh', 'sim_run_csx_helper.pl']
#       else:
#           run_csx_helper_cmd = ['sim_run_csx_helper.pl']
#       proc = subprocess.Popen(run_csx_helper_cmd , stdout=subprocess.PIPE, stderr=subprocess.PIPE)
#       stdoutdata, stderrdata = proc.communicate()
#       print("%s" % stdoutdata)
#       print("%s" % stderrdata) 

    create_test_result_directory()
    
    port_base =  current_test_control.get_port_base()

    test_summary = test_result_summary(currentDateTime);
    base_directory = get_test_base_directory(currentDateTime)
    detailed_results = test_detailed_results(base_directory, test_script_directory)

    current_thread_control.set_running_threads(current_test_control.get_thread_num())
    current_thread_control.set_default_num_threads(current_test_control.get_thread_num())

    #Repeat the range of tests some number of times.
    for repeat_index in range(0, (current_test_control.get_all_tests_iteration())):
       if len(current_test_control.get_test_num_list()) > 0:
          #adding tests by parsing '-t' or '-tests'
          for i in current_test_control.get_test_num_list():
             #test name or test suite name is valid
             if i != -1:
                 queue.put((i, repeat_index))
       else:
          #adding all tests if '-t' or '-tests' are not specified
          for i in range(0,current_test_control.get_last_test_num() +1 ):
             queue.put((i, repeat_index))

    #start one fbe_test.exe for each thread
    for i in range(current_test_control.get_thread_num()):
        port_base = port_base + 23 * i;
        t = TestThread(queue,port_base, i, lock,test_summary)
        t.setDaemon(True)
        t.start()
    
    #wait on the queue until everything has been processed
    queue.join()
    print 'Elapsed Time: %2.0f seconds.' % (time.time() - start)
    test_summary.print_summary(0)
    test_summary.write_summary_to_file(time.time() - start)
    detailed_results.write_detailed_summary()

queue = Queue.Queue()
now = datetime.datetime.now()
currentDateTime = now.strftime('%Y-%m-%d_%H-%M-%S')
current_test_control = test_control()
current_thread_control = thread_control(current_test_control, queue)
setup_environment()
main()
