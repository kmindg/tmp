####################################################################### 
# 
# gst.py - Generate Stack Trace Utility
# 
####################################################################### 
#   Parse a trace file containing error traces with stacks and/or
#   logger traces.
# 
#   Since these traces contain function addresses, use pdb symbols to
#   get the symbol names for these function addresses and
#   display the formatted stack traces. 
#
####################################################################### 
import sys
import os
import re
from struct import *


class WritableObject:
    def __init__(self):
        self.content = []
    def write(self, string):
        self.content.append(string)

####################################################################### 
# trace_file_object
#   This class represents a trace file where we might want to 
# 
####################################################################### 
class trace_file_object:
    def __init__(self, file_name):
        self._file_name = file_name

    def get_stack_trace_addresses(self):
        """Fetch information about stack traces from a trace file.
           First return is an array where each entry is an array of related stack functions.
           The other return value is an array of error traces."""
        error_trace_array = []
        function_addresses_array = []
        stack_addresses = []
        bt_found = 0
        with open(self._file_name, "r") as f:
            for line in f:

                #print " bt_found " + str(bt_found) + " " + line
                if bt_found == 2:
                    entries = line.split(" ")
                    #print "process backtrace: line: " + "len " + str(len(entries))
                    #print  entries
                    #print  str(len(entries))
                    if "CERR" in line:
                        #print " CERR found " + str(bt_found) + " "
                        #print line
                        bt_found = 0
                        error_trace_array.append(line)
                        if len(stack_addresses) > 0:
                            function_addresses_array.append(stack_addresses)
                            stack_addresses = []
                        continue
                    if "ERR" in line:
                        #print " ERR found " + str(bt_found) + " "
                        #print line
                        bt_found = 0
                        error_trace_array.append(line)
                        if len(stack_addresses) > 0:
                            function_addresses_array.append(stack_addresses)
                            stack_addresses = []
                        continue
                    if len(entries) >= 17 and ":" in line:
                        #print "found bt line: " + line
                        fn_index = len(entries)-1
                        fn_string = entries[fn_index]
                        try:
                            function_addr = int(fn_string[:fn_index], 16)
                            #print "addr found: " + hex(function_addr)
                            stack_addresses.append(function_addr)
                        except ValueError: pass
                            #print "ValueError hit for [" + str(fn_string[:fn_index]) + "] "+ line
                        continue
                if "backtrace" in line:
                    #print " backtrace found " + str(bt_found) + " "
                    bt_found = bt_found + 1
                    if len(stack_addresses) > 0:
                        function_addresses_array.append(stack_addresses)
                        stack_addresses = []
                    continue
            if len(stack_addresses) != 0:
                function_addresses_array.append(stack_addresses)
        return function_addresses_array, error_trace_array

    def get_logger_addresses(self):
        """Parse a trace file and get an array of all logger information. """
        logger_array = []
        stack_addresses = []
        delta_time_array = []
        flag_array = []
        current_time_array = []
        bt_found = 0
        print "Parsing for logger entries...Please be patient ***"
        with open(self._file_name, "r") as f:
            for line in f:

                #print " bt_found " + str(bt_found) + " " + line
                if bt_found == 1:
                    entries = line.split(" ")
                    #print "bt 2 " + line + "len " + str(len(entries))
                    #print  entries
                    #print  str(len(entries))
                    if "logger end" in line:
                        #print " logger finished " + str(bt_found) + " "
                        bt_found = 0
                        continue
                    if len(entries) >= 17 and ":" in line:
                        #print "found bt line: " + line
                        fn_index = len(entries)-3
                        fn_string = entries[fn_index]
                        function_addr = int(fn_string[:fn_index], 16)

                        time_index = fn_index+1
                        time_string = entries[time_index]
                        delta_time = int(time_string[:time_index], 16)

                        flags_index = fn_index+2
                        flags_string = entries[flags_index]
                        flags = int(flags_string[:(len(flags_string)-1)], 16)
                        print "addr found: " + hex(function_addr) + " time: " + hex(delta_time) + " flags: " + hex(flags)
                        delta_time_array.append(delta_time)
                        flag_array.append(flags)
                        stack_addresses.append(function_addr)
                if "logger start" in line:
                    entries = line.split(" ")
                    time_index = len(entries)-1
                    time_string = entries[time_index]
                    current_time = int(time_string[:len(time_string)-1], 16)
                    print "time found " + hex(current_time)
                    bt_found = bt_found + 1
                    if len(stack_addresses) > 0:
                        current_time_array.append(current_time);
                        logger_array.append([stack_addresses, delta_time_array, flag_array, current_time_array])
                    stack_addresses = []
                    continue
            if len(stack_addresses) != 0:
                current_time_array.append(current_time);
                logger_array.append([stack_addresses, delta_time_array, flag_array, current_time_array])
        return logger_array

    def get_driver_entry_addresses_ktrace(self):
        """This function will pull driver entry addresses out of a start ktrace"""
        neit_entry_address = 0
        sep_entry_address = 0
        pp_entry_address = 0
        esp_entry_address = 0
        ppfd_entry_address = 0

        with open(self._file_name, "r") as f:
           for line in f:

               entries = line.split(" ")
               if len(entries) > 0:
                   last_index = len(entries) - 1
               if "SEP driver, starting...EmcpalDriverEntry:" in line:
                   print "found SEP EmcpalDriverEntry " + entries[last_index]
                   sep_entry_address = int(entries[last_index], 16)
                   #print hex(sep_entry_address)
               if "ESP DriverEntry: starting..EmcpalDriverEntry:" in line:
                   entries = line.split(" ")
                   #print entries
                   print "found ESP EmcpalDriverEntry " + entries[last_index]
                   esp_entry_address = int(entries[last_index], 16)
                   #print hex(sep_entry_address)
               if "NEIT package DRIVER, starting..EmcpalDriverEntry:" in line:
                   entries = line.split(" ")
                   #print entries
                   print "found NewNeitPackage EmcpalDriverEntry " + entries[last_index]
                   neit_entry_address = int(entries[last_index], 16)
                   #print hex(sep_entry_address)
               if "physical pkg DRIVER, starting...EmcpalDriverEntry" in line:
                   #print entries
                   print "found PhysicalPackage EmcpalDriverEntry " + entries[last_index]
                   pp_entry_address = int(entries[last_index], 16)
               if "PPFD: DriverEntry" in line and "Boot" not in line:
                   print entries
                   ppfd_entry_address = int(entries[13], 16)
                   print "found PPFD EmcpalDriverEntry " + hex(ppfd_entry_address)

        return pp_entry_address, esp_entry_address, sep_entry_address, ppfd_entry_address, neit_entry_address

    def get_driver_entry_address(self, module_name, fn):
        """Find the offset of the driver from the !show_fbe_driver_info macro. """
        search_string = module_name + "!" + fn
        with open(self._file_name, "r") as f:
           for line in f:
               if search_string in line:
                   entries = line.split(" ")
                   if len(entries) > 0:
                       last_index = len(entries) - 1
                   return int(entries[last_index], 16)
        return 0

    def get_driver_entry_addresses(self, module_list, funct_array):
        """Loops over all modules and gets the driver entry address for each."""
        address_list = []
        index = 0
        for module in module_list:
            address = self.get_driver_entry_address(module, funct_array[index])
            address_list.append(address)
            index = index + 1
        return address_list
        
####################################################################### 
# map_file_object
#   This object is used to parse a .map file for function addresses.
# 
####################################################################### 
class map_file_object:
    def __init__(self, map_file_name):
        self._map_file_name = map_file_name  + ".map"
        #print "map file is: " + self._map_file_name
    def find_offset(self, search_function_offset):
        b_found_header = 0
        found_fn_addr = 0
        found_fn_offset = 0
        found_fn_name = ""
        max_offset = 0
        max_offset_fn = ""
        try:
            with open(self._map_file_name, "r") as map_file:

                for line in map_file:
                    if "Publics by Value" in line:
                        b_found_header = 1
                    if b_found_header == 0:
                        continue
                    entries = line.split(" ")
                    if len(entries) < 4:
                        continue
                    #print entries
                    offset_entries = entries[1].split(":")
                    if len(offset_entries) < 2:
                        continue
                    #print offset_entries
                    #print "current_fn_addr: " + hex(current_fn_addr)
                    current_fn_offset = int(offset_entries[1], 16)
                    current_fn_name = entries[8]
                    if search_function_offset == current_fn_offset:
                        found_fn_name = current_fn_name
                        found_fn_offset = current_fn_offset
                    elif search_function_offset > current_fn_offset:
                        found_offset = search_function_offset - found_fn_offset
                        current_offset = search_function_offset - current_fn_offset
                        if current_offset < found_offset:
                            found_fn_name = current_fn_name
                            found_fn_offset = current_fn_offset
                    if current_fn_offset > max_offset:
                        max_offset = current_fn_offset
                        max_offset_fn = current_fn_name
            #print self._map_file_name + " max offset: " + hex(max_offset) + " symbol name: " + max_offset_fn 
            map_file.close
        except IOError as e: pass
        return (found_fn_name, found_fn_offset)

    def find_max_offset(self):
       b_found_header = 0
       found_fn_addr = 0
       found_fn_offset = 0
       found_fn_name = ""
       max_offset = 0
       max_offset_fn = ""
       try:
         with open(self._map_file_name, "r") as map_file:
             for line in map_file:
               if "Publics by Value" in line:
                   b_found_header = 1
               if b_found_header == 0:
                   continue
               entries = line.split(" ")
               if len(entries) < 4:
                   continue
               #print entries
               offset_entries = entries[1].split(":")
               if len(offset_entries) < 2:
                   continue
               #print offset_entries
               #print "current_fn_addr: " + hex(current_fn_addr)
               current_fn_offset = int(offset_entries[1], 16)
               current_fn_name = entries[8]
               if current_fn_offset > max_offset:
                   max_offset = current_fn_offset
                   max_offset_fn = current_fn_name
             #print self._map_file_name + " max offset: " + hex(max_offset) + " symbol name: " + max_offset_fn 
             map_file.close()
       except IOError as e: pass
       return max_offset

    def find_function_offset(self, fn_name):
       try:
         with open(self._map_file_name, "r") as map_file:
          for line in map_file:
              if fn_name in line:
                  entries = line.split(" ")
                  offset_entries = entries[1].split(":")
                  offset = int(offset_entries[1], 16)
                  print self._map_file_name + " EmcpalDriverEntry offset: " + hex(offset)
                  map_file.close()
                  return offset
          map_file.close()     
       except IOError as e: pass       
      
       return 0 # no offset found


    def find_module_load_base_address(self, fn_name, fn_address):
        offset = self.find_function_offset(fn_name)

        module_offset = fn_address - offset
        print "module_offset " + hex(module_offset) + "\n\n"
        return module_offset

######################################################################
# pdb_file_object
#  This object is used to parse a pdb file in order to find function
#  offsets.
# 
####################################################################### 
class pdb_file_object:
    def __init__(self, map_file_name):
        self._map_file_name = map_file_name + ".info"
        
    def find_offset(self, search_function_offset):
        """Find the best match offset and name of the input function offset.
           This allows us to map a function address to an actual symbol. """
        b_found_header = 0
        found_fn_addr = 0
        found_fn_offset = 0
        found_fn_name = ""
        max_offset = 0
        max_offset_fn = ""
        try:
            with open(self._map_file_name, "r") as map_file:

                for line in map_file:
                    if "** *pUBLICS" in line:
                        b_found_header = 1
                    if b_found_header == 0:
                        continue
                    # Find the offset and name for this line.
                    current_fn_offset,current_fn_name = self.get_offset(line)

                    if current_fn_offset == -1:
                        # Not a symbol.
                        continue
                    # See if this function is an exact match.
                    if search_function_offset == current_fn_offset:
                        found_fn_name = current_fn_name
                        found_fn_offset = current_fn_offset
                    # Check if we are above this offset.
                    # If we are above the function start offset we might be within this function.
                    elif search_function_offset > current_fn_offset:
                        found_offset = search_function_offset - found_fn_offset
                        current_offset = search_function_offset - current_fn_offset
                        # If this is the closest match so far, save the name and offset.
                        if current_offset < found_offset:
                            found_fn_name = current_fn_name
                            found_fn_offset = current_fn_offset
                    if current_fn_offset > max_offset:
                        max_offset = current_fn_offset
                        max_offset_fn = current_fn_name
            #print self._map_file_name + " max offset: " + hex(max_offset) + " symbol name: " + max_offset_fn 
            map_file.close
        except IOError as e: pass
        return (found_fn_name, found_fn_offset)

    def find_max_offset(self):
       """Finds the largest offset in this module.  This will allow us to know if a given
          function address lies within this module."""
       b_found_header = 0
       found_fn_addr = 0
       found_fn_offset = 0
       found_fn_name = ""
       max_offset = 0
       max_offset_fn = ""
       try:
         with open(self._map_file_name, "r") as map_file:
             for line in map_file:
               if "** *pUBLICS" in line:
                   b_found_header = 1
               if b_found_header == 0:
                   continue
               current_fn_offset,current_fn_name = self.get_offset(line)
               if current_fn_offset == -1:
                   continue
               if current_fn_offset > max_offset:
                   max_offset = current_fn_offset
                   max_offset_fn = current_fn_name
             #print self._map_file_name + " max offset: " + hex(max_offset) + " symbol name: " + max_offset_fn 
             map_file.close()
       except IOError as e: pass
       return max_offset

    def get_offset(self, line):
        """Extract a function name and offset from a pdb info file line. 
           If the line does not contain a symbol we return -1 for the offset. """
        #print line
        if "Function: [" in line:
            line = line.replace("["," ")
            line = line.replace("]"," ")
            line = line.replace(":"," ")
            line = line.replace("\w+"," ")
            line = line.replace("\W+"," ")
            entries = line.split(" ")
            #print entries
            #offset_entries = entries[1].split(":")
            if len(entries) < 9:
                return -1, ""
            offset = int(entries[6], 16)
            fn = str(entries[8])
            #print offset_entries
            return offset, fn
        return -1, ""

    def match_fn(self, function, line):
        """Extract a function name and offset from a pdb info file line. 
           If the function name matches then we return a valid offset and function name,
           othewise we return -1"""
        if "Function: [" in line and function in line:
            line = line.replace("["," ")
            line = line.replace("]"," ")
            line = line.replace(":"," ")
            line = line.replace("\w+"," ")
            line = line.replace("\W+"," ")
            entries = line.split(" ")
            #print entries
            offset_entries = entries[1].split(":")
            if len(entries) < 9:
                return -1, ""
            offset = int(entries[6], 16)
            fn = str(entries[8])
            #print offset_entries
            return offset, fn
        return -1, ""

    def find_function_offset(self, fn_name):
       """Find the offset of a given function name in the pdb file."""
       try:
         #print "find function offset " + fn_name
         with open(self._map_file_name, "r") as map_file:
          for line in map_file:
              offset, funct_name = self.match_fn(fn_name, line)
              if offset != -1:
                  #print self._map_file_name + " fn:" + fn_name + " offset: " + hex(offset) + " name: " + funct_name
                  map_file.close()
                  return offset
            
          map_file.close()
       # If we get any I/O error, we allow ourselves to continue.
       except IOError as e: pass       
      
       return 0 # no offset found


    def find_module_load_base_address(self, fn_name, fn_address):
        """ Find the load base address of this module based on having one input address and symbol name. """
        offset = self.find_function_offset(fn_name)

        #print "fn_addr " + hex(fn_address) + " offset: " + hex(offset) + "\n"
        module_offset = fn_address - offset
        #print "module_offset " + hex(module_offset) + "\n"
        return module_offset

####################################################################### 
# test_control_object
#   This object ties together the various objects and coordinates the
#   program operation.
# 
####################################################################### 
class test_control_object:
    def __init__(self):
        self._path_name = ""
        self._stack_trace_file_name = ""
        self._module_offsets_file_name = ""
        self._module_load_address_array = []
        self._module_max_offset_array = []
        self._module_array = ["PhysicalPackage", "espkg", "sep", "ppfd", "NtMirror", 
                              "NewNeitPackage", "DRAMCache", "psm", "EmcPAL", "interSPlock", "Fct"]
        self._entry_function_array = ["EmcpalDriverEntry", "EmcpalDriverEntry", "EmcpalDriverEntry", "EmcpalDriverEntry",
                                "EmcpalDriverEntry", "EmcpalDriverEntry", "EmcpalDriverEntry", "EmcpalDriverEntry", 
                                "DriverEntry", "EmcpalDriverEntry", "EmcpalDriverEntry"]

        for index in range(len(self._module_array)):
            self._module_max_offset_array.append(0)
            self._module_load_address_array.append(0)
    
    def usage(self, script_name):
        print ''
        print ''
        print 'Generate Stack Trace Utility'
        print ''
        print ''
        print 'Usage:    gst.py [symbol files path][trace file][show driver info file]'
        print ''
        print '    This script will decode function addresses from within a trace file.'
        print ''
        print 'Options:'
        print ''
        print '    symbol files path - is the path where the .pdb files reside'
        print ''
        print '    trace file        - is the file that contains either stack traces or logger traces.'
        print ''
        print '    show driver info file - is the file that contains the !show_fbe_driver_info text'
        print '                            This can either be the SPx_sep_info.txt file'
        print '                            or a file with !show_fbe_driver_info (from ppdbg) that'
        print '                                 was run on a live system via ktcons or on a dump via windbg'
        print 'Examples:'
        print '    print help message:'
        print '        python gst.py'
        print ''
        print '    Decode stacks from a ktrace for a spcollect:'
        print '        python gst.py .\symbols SPB_kt_std.txt SPB_sep_info.txt'
        print ''
    def process_args(self):
        if len(sys.argv) < 2:
            self.usage(sys.argv[0])
            print "%s expects a path name" % "gst.py"
            return 0

        if len(sys.argv) < 4:
            self.usage(sys.argv[0])
            print "%s expects a ktrace file" % "gst.py"
            return 0

        #if len(sys.argv) < 4:
            #print "trying to use just one file %s for both backtraces and !show_fbe_driver_info" % "gst.py"
            #print "%s [map file path][backtrace file][module name file]" % sys.argv[0]
            #print "%s expects a module file" % sys.argv[0]

        self._path_name = sys.argv[1]
        self._stack_trace_file_name = sys.argv[2]
        self._module_offsets_file_name = sys.argv[3]
        print("input path: " + self._path_name)
        print("trace file path: " + self._stack_trace_file_name)
        print("module offsets file path: " + self._module_offsets_file_name)
        return 1

    def generate_pdb_info(self):
        """Generate an intermediate representation of the pdb file, which we can easily
           parse as text. 
           The files are named: "module_name.info.  
           And we only create the file if the file does not already exist since it takes a while to generate it. """
           
        for module in self._module_array:
            info_file_name = self._path_name + "\\" + module + ".info"
            pdb_file_name = self._path_name + "\\" + module + ".pdb"
            string = "Dia2Dump -all " + pdb_file_name + " > " + info_file_name
            if not os.path.exists(pdb_file_name):
                print "file: " + pdb_file_name + " does not exist!  Please check your symbol path."
                exit(0)
            if not os.path.exists(info_file_name):
                #print string
                print "Parsing " + pdb_file_name 
                print "Please be patient, this may take a while. "
                os.system(string)
            else:
                print "file exists, not generating " + info_file_name
    def get_module_load_addresses(self):
        """Get driver entry addresses for the modules in the module_offsets file."""
        driver_entry_address_file = trace_file_object(self._module_offsets_file_name)
        self._module_driver_entry_address_array = driver_entry_address_file.get_driver_entry_addresses(self._module_array,
                                                                                                       self._entry_function_array)
        
    def get_stack_function_addresses(self):
        """Get stack function pointers from the stack trace file."""
        trace_file = trace_file_object(self._stack_trace_file_name)
        (self._function_addresses, self._error_traces) = trace_file.get_stack_trace_addresses()

    def get_logger_function_addresses(self):
        """Get logger function pointers from the stack trace file."""

        trace_file = trace_file_object(self._stack_trace_file_name)

        self._logger_info = trace_file.get_logger_addresses()


    def get_modules_load_addresses(self):
        """Find actual offset of the driver in memory"""
        module_index = 0
        for module_name in self._module_array:
            # A zero address means we didn't find the trace in the start buffer.
            if self._module_driver_entry_address_array[module_index] != 0:
                map_file_name = self._path_name + "\\" + module_name
                #print "file is: " + map_file_name
                mf = pdb_file_object(map_file_name)
                print (module_name + "!" + self._entry_function_array[module_index] +  " = " + 
                       hex(self._module_driver_entry_address_array[module_index]) )
                self._module_load_address_array[module_index] = mf.find_module_load_base_address(self._entry_function_array[module_index], 
                                                                                                 self._module_driver_entry_address_array[module_index])
                print "module: " + module_name + " load base address: " + hex(self._module_load_address_array[module_index])
                self._module_max_offset_array[module_index] = mf.find_max_offset()
            module_index = module_index + 1
    def display_stack_trace(self):
        """Map all the stack trace addresses to actual symbols (if possible) and
           display the formatted stack traces. """
        stack_index = 0
        # This dictionary is to cache stack trace addresses we have already mapped.
        # This helps speed up the parsing when we have many similar stack traces.
        matches_dict = {}
        print ""
        print str(len(self._function_addresses)) + " total stacks found"
        print ""

        # Loop over each entry in the function_addresses.
        # Note that each entry is an array of addresses associated with one stack.
        for stack in self._function_addresses:
            #print "[" + str(stack_index) + "]"
            #print stack
            if len(stack) > 0:
                print "start stack[" + str(stack_index) + "]"

            # Display the error trace associated with this stack trace.
            if stack_index < len(self._error_traces) and len(stack) > 0:
                print self._error_traces[stack_index]

            # Now iterate over each of the addresses in this stack,
            # map the address to a symbol and then display the formatted entry.
            for search_function_addr in stack:
                found_fn_name = 0
                module_index = 0
                for module_name in self._module_array:
                    # A zero address means we didn't find the trace in the start buffer.
                    if self._module_driver_entry_address_array[module_index] != 0:
                        map_file_name = self._path_name + "\\" + module_name
                        #mf = pdb_file_object(map_file_name)
                        possible_offset = search_function_addr - self._module_load_address_array[module_index]
                        #print "module: " + module_name + " fn_addr: " + hex(search_function_addr) + " module_load_addr: " + hex(self._module_load_address_array[module_index]) + " driverentry:  " + hex(self._module_driver_entry_address_array[module_index]) + " index " + str(module_index) + " possible off: " + hex(possible_offset) + " module max offset:" + hex(self._module_max_offset_array[module_index])

                        # If the address is within the range of this module, then search for it.
                        if search_function_addr > self._module_load_address_array[module_index] and possible_offset < self._module_max_offset_array[module_index]:
                            
                            search_function_offset = search_function_addr - self._module_load_address_array[module_index]
                            #print "func addr: " + hex(search_function_addr)
                            b_found_header = 0
                            found_fn_addr = 0
                            found_fn_offset = 0

                            #print str(search_function_offset)

                            # We cache all the previously found address, function combination
                            # in matches_dict, where the key is the function address.
                            # If we find the match, no need to look it up in the file.
                            if search_function_addr in matches_dict:
                                #print "match found " + hex(search_function_addr)
                                (found_fn_name, found_fn_offset) = matches_dict[search_function_addr] 
                            else:
                                #print "no match found " + hex(search_function_addr)
                                mf = pdb_file_object(map_file_name)
                                (found_fn_name, found_fn_offset) = mf.find_offset(search_function_offset)

                            found_fn_addr = self._module_load_address_array[module_index] + found_fn_offset
                            #print "found function name " + str(found_fn_name) + " offset " + hex(found_fn_addr) + " search: " + hex(search_function_addr)
                            #print str(found_fn_name) + " " + hex(found_fn_addr) + " " + hex(search_function_addr)
                            if found_fn_name != "":
                                # Save the match in our cache.
                                matches_dict[search_function_addr] = (found_fn_name, found_fn_offset)

                                found_fn_name = found_fn_name.replace("\W+"," ")
                                found_fn_name = found_fn_name.replace("\n"," ")
                                found_fn_name = found_fn_name.replace("\r"," ")
                                print module_name + "!" + str(found_fn_name)
                                break;
                    module_index = module_index + 1
                if found_fn_name == 0:
                    print "unknown!" + hex(search_function_addr)
            if len(stack) > 0:
                print "end stack[" + str(stack_index) + "]"
                print ""
            stack_index = stack_index + 1

    def display_logger_trace(self):
        """Display the formatted output of all logger entries.
           This will map the logger function addresses to symbols and then display each one."""

        stack_index = 0
        for logger_record in self._logger_info:
            #print logger_record
            #print logger_record[0]
            #print logger_record[1]
            #print logger_record[2]
            current_times = logger_record[1]
            current_flags = logger_record[2]
            address_array = logger_record[0]
            if len(address_array) > 0:
                print "start logger[" + str(stack_index) + "]"

            logger_index = 0
            for search_function_addr in address_array:

                current_time = logger_record[3][0]
                found_fn_name = 0
                module_index = 0
                for module_name in self._module_array:
                    # A zero address means we didn't find the trace in the start buffer.
                    if self._module_driver_entry_address_array[module_index] != 0:
                        map_file_name = self._path_name + "\\" + module_name
                        mf = pdb_file_object(map_file_name)
                        possible_offset = search_function_addr - self._module_load_address_array[module_index]
                        #print "module: " + module_name + " fn_addr: " + hex(search_function_addr) + " module_load_addr: " + hex(self._module_load_address_array[module_index]) + " driverentry:  " + hex(self._module_driver_entry_address_array[module_index]) + " index " + str(module_index) + " possible off: " + hex(possible_offset) + " module max offset:" + hex(self._module_max_offset_array[module_index])
                        if search_function_addr > self._module_load_address_array[module_index] and possible_offset < self._module_max_offset_array[module_index]:
                            search_function_offset = search_function_addr - self._module_load_address_array[module_index]
                            #print "func addr: " + hex(search_function_addr)
                            b_found_header = 0
                            found_fn_addr = 0
                            found_fn_offset = 0

                            mf = pdb_file_object(map_file_name)
                            (found_fn_name, found_fn_offset) = mf.find_offset(search_function_offset)
                            found_fn_addr = self._module_load_address_array[module_index] + found_fn_offset
                            #print "found function name " + str(found_fn_name) + " offset " + hex(found_fn_addr) + " search: " + hex(search_function_addr)
                            #print str(found_fn_name) + " " + hex(found_fn_addr) + " " + hex(search_function_addr)
                            if found_fn_name != "":
                                flags = current_flags[logger_index]
                                logger_time = current_times[logger_index]
                                if flags == 0x8:
                                    seconds = (current_time - logger_time)/1000
                                if flags == 0x1:
                                    seconds = (current_time - logger_time)/1000
                                if flags == 0x7:
                                    seconds = (logger_time)/1000
                                print " time: " + hex(current_time) + " fl: " + hex(flags) + " sec: " + str(seconds) + " " + module_name + "!" + str(found_fn_name)
                                break;
                    module_index = module_index + 1
                if found_fn_name == 0:
                    print "unknown!" + hex(search_function_addr) + " time: " + hex(current_times[logger_index]) + " fl: " + hex(current_flags[logger_index])
                logger_index = logger_index + 1
            if len(address_array) > 0:
                print "end logger[" + str(stack_index) + "]"
                print ""
            stack_index = stack_index + 1
  
test_control_obj = test_control_object()

# Parse our arguments and bail if an error was hit.
if not test_control_obj.process_args():
    exit(0)

print "starting"

# Generate a text representation of the PDB, which can be easily parsed.
test_control_obj.generate_pdb_info()

# Parse the driver entry offset from the !show_fbe_driver_info
test_control_obj.get_module_load_addresses()

# Parse the stack traces.
test_control_obj.get_stack_function_addresses()

# Fetch the actual address of driver entry for each module and then
# determine the actual module load address.
# We'll use this later when displaying the stack trace to map a 
# stack trace address to an actual symbol.
test_control_obj.get_modules_load_addresses()

# Now put it all together.
# Map the stack trace addresses to actual symbols and display them.
test_control_obj.display_stack_trace()

# Parse the logger entries we found in the trace file.
test_control_obj.get_logger_function_addresses()

# Map the logger addresses to actual symbols and display the formatted output
# For the logger entries we found.
test_control_obj.display_logger_trace()




