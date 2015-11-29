###########################################################################
# Copyright (C) EMC Corporation 2013
# All rights reserved.
# Licensed material -- property of EMC Corporation
##########################################################################
  
##########################################################################
# rba_to_rdgen.py
# 
# The purpose of this tool is to convert an rba trace file into
# an rdgen file that can be used for rdgen playback.
# 
# This allows us to use rdgen to re-create a host workload on one
# system from an rba trace that we collected on another system.
# 
##########################################################################
import sys
import os
import re
from struct import *
import rbafilter
from rbafilter import *
from rbautils import *
from matplotlib import pyplot as plt
import sys
import argparse

rdgen_filter_dict = {
 'packet' : 1,
 'device' : 3,
}
def get_filter_key_by_value(val):
    return [k for k, v in rdgen_filter_dict.iteritems() if v == val][0]

rdgen_interface_dict = {
 'packet' : 1,
 'dca'    : 2,
 'mj'     : 3,
 'sgl'    : 4,
}
def get_interface_key_by_value(val):
    return [k for k, v in rdgen_interface_dict.iteritems() if v == val][0]

rba_to_rdgen_opcode = {
 0 : 1, # read
 2 : 2, # write
 4 : 6, # zero -> write same
}

class rdgen_config_file:

    def __init__(self, file_name):
        self._file_name = file_name
        self._mapping_dict = {}
        self._interface_dict = {}
        self.source = "UKN"
    def read_file(self):
        
        with open(self._file_name, "r") as f:
            for line in f:
                if "#" in line:
                    # Skip comments
                    continue
                elif "threads=" in line:
                    line = line.rstrip('\n')
                    line = line.rstrip('\r')
                    thread_mapping = line.split("=")
                    if thread_mapping[0] == "threads":
                        self._threads = int(thread_mapping[1])
                        print "threads is " + str(self._threads)
                elif "source=" in line:
                    line = line.rstrip('\n')
                    line = line.rstrip('\r')
                    source_map = line.split("=")
                    if source_map[0] == "source":
                        self.source = source_map[1]
                        print "source is " + self.source
                else:
                    #print "line: " + line
                    line = line.rstrip('\n')
                    line = line.rstrip('\r')
                    entries = line.split(",")
                    #print "split ,"
                    #print entries
                    #entries
                    if len(entries) > 1:
                        interface = entries[0]
                        target = entries[1]
                        mapping = target.split("=")
                        lun = mapping[0]
                        self._interface_dict[lun] = interface
                        if interface == "packet":
                            #print "packet interface: "
                            #print mapping
                            object_id = int(mapping[1], 16)
                            self._mapping_dict[lun] = object_id
                            print str(interface) + " LU " + str(lun) + " = " + hex(object_id) 
                        elif interface == "dca" or interface == 'mj' or interface == 'sgl':
                            device_name = mapping[1]
                            self._mapping_dict[lun] = device_name
                            print str(interface) + " LU " + str(lun) + " = " + str(device_name)
class rdgen_file:
    def __init__(self, file_name):
        self._file_name = file_name

    def write(self, header, filter_list):
        file_name = self._file_name + ".rdg"
        print "Writing file: " + file_name
        with open(file_name, "wb") as f:
            header.write(f)
            print header
            for filter in filter_list:
                filter.write(f)
                print filter
        f.close()
    def write_object(self, header, object_id, record_list):
        file_name = self._file_name + "_" + str(object_id) + ".rdg"
        print "Writing file: " + file_name + " with " + str(len(record_list)) + " records "
        with open( file_name, "wb") as f:
            header.write(f)
            print header
            for rec in record_list:
                rec.write(f)
            f.close()

class rdgen_object_header:
    def __init__(self, num_threads, num_records, filter):
        self._threads = num_threads
        self._records = num_records
        self._filter = filter
        
    def write(self, file):
        # First is a filter for this object, then the #records and threads.
        self._filter.write(file)
        header_format = "LL"
        packed_string = pack(header_format, self._records, self._threads)
        file.write(packed_string)
    def __str__(self):
        return "rdgen object header th: %d records: %d filter: %s" % (self._threads, self._records, self._filter)

class rdgen_record: 
    def __init__(self, rba_record, record_data_dict ):
        """Pass in either an rba record to init from or a dictionary."""
        self._interface = rdgen_interface_dict['packet']
        if record_data_dict != None:
            self._opcode = record_data_dict['opcode']
            self._lba = record_data_dict['lba']
            self._blocks = record_data_dict['blocks']
            self._core = record_data_dict['core']
            self._threads = record_data_dict['threads']
            self._ramp_msecs = record_data_dict['ramp_msecs']
            if record_data_dict.get('priority') != None:
                self._priority = record_data(dict['priority'])
            else:
                self._priority = priority_dict['NORMAL'] # Default to normal.
            self.validate_priority()
            return
        cmd = rba_record.cmd & 0xF
        if cmd == 0:
            #read
            self._opcode = 1
        elif cmd == 2:
            #write
            self._opcode = 2
        elif cmd == 4:
            # zero becomes write same
            self._opcode = 6
        else:
            print "rdgen_record: Unknown rba opcode 0x%x\n" % cmd
            exit(0)
        self._lba = rba_record.lba
        self._blocks = rba_record.count
        self._core = rba_record.cpu
        self._threads = rba_record.queue_io
        self._ramp_msecs = 0
        self._priority = rba_record.priority
        self.validate_priority()
    def validate_priority(self):
        if self._priority < priority_dict['LOW'] or self._priority > priority_dict['URGENT']:
            #print "unexpected priority %d\n" % self._priority
            #print self
            self._priority = priority_dict['NORMAL']
    def write(self, file):
        # Filter type, object id, package id, device name
        filter_format = "LLQQLLLLLLL";
        if self._priority < priority_dict['LOW'] or self._priority > priority_dict['URGENT']:
            print "unexpected priority %d\n" % self._priority
            print self
        #print self
        packed_string = pack(filter_format, self._opcode, self._core, self._lba, self._blocks, 0, 
                             self._threads, self._ramp_msecs, self._priority, self._interface, 0, 0)
        file.write(packed_string)
        
    def __str__(self):
        return "Rdgen record:  opcode: 0x%x lba: 0x%x blocks: 0x%x core: 0x%x pri: %d intf: %d ramp_ms: %d" % (self._opcode, self._lba, self._blocks, self._core, self._priority, self._interface, self._ramp_msecs)

class rdgen_delay_record(rdgen_record):
    def __init__(self, delay_msec):
        rdgen_record.__init__(self, None,
                              {'opcode' : 2, 'lba' : 0x0, 'blocks' : 1, 'core' : 0,
                               'flags' : 0, 'threads' : 0, 'ramp_msecs' : delay_msec})
class rdgen_filter:
    def __init__(self, filter_type, object_id, package_id, threads, device_name = "", interface = 'packet'):
        self._filter_type = filter_type
        self._object_id = object_id
        self._interface = rdgen_interface_dict[interface]
        if package_id == "sep":
            self._package_id = 3
        elif package_id == "physical":
            self._package_id = 1
        else:
            self._package_id = 0 # unused
        self._device_name = device_name
        self.convert_device()
        self._threads = threads
        
    def get_object(self):
        return self._object_id
    def convert_device(self):
        if self._filter_type == rdgen_filter_dict['device']:
            self._device_name = '\\Device\\' + self._device_name
    def write(self, file):
        # Filter type, object id, package id, device name
        print self
        filter_format = "LLL80sL";
        packed_string = pack(filter_format, self._filter_type, self._object_id, self._package_id, self._device_name, self._threads)
        file.write(packed_string)
    def __str__ (self):
        return "rdgen filter: type: %d object: 0x%x pkg: %d device: %s threads: %d interface: %d (%s)" % (self._filter_type, self._object_id, self._package_id, self._device_name, self._threads, self._interface, get_interface_key_by_value(self._interface))

class rdgen_object_filter(rdgen_filter):
    def __init__(self, object_id, package_id, threads):
        rdgen_filter.__init__(self,
                              rdgen_filter_dict['packet'],# object filter
                              object_id, package_id, threads)

class rdgen_device_filter(rdgen_filter):
    def __init__(self, device_name, tag, threads, interface):
        rdgen_filter.__init__(self,
                              rdgen_filter_dict['device'],# object filter
                              tag, 0, # obj/package
                              threads, device_name, interface)

class rdgen_header:
    def __init__(self, threads, objects):
        self._threads = threads
        self._objects = objects
        
    def write(self, file):
        # targets, threads, extra
        header_format = "LL"
        packed_string = pack(header_format, self._objects, self._threads)
        file.write(packed_string)

    def __str__(self):
        return "rdgen root header th: %d objects: %d" % (self._threads, self._objects)

def write_rdgen_root_file(config_file, output_file, filter_dict):
    root_file = rdgen_file(output_file)
    filter_list = []
    threads = config_file._threads

    for lun in config_file._mapping_dict.keys():
        #object_id = config_file._mapping_dict[lun]
        filter_list.append(filter_dict[lun])
    header = rdgen_header(threads, len(filter_list)) #objects 
    root_file.write(header, filter_list)
    total_chunks = 5

def write_rdgen_object_file(config_file, rdgen_filter, rec_list, output_file):
    root_file = rdgen_file(output_file)
    index = 0
    object_header = rdgen_object_header(rdgen_filter._threads, len(rec_list), rdgen_filter)
    root_file.write_object(object_header, rdgen_filter._object_id, rec_list)

class app_control:
    def __init__(self):
        self.file_name = ""
        self.output_file_name = ""
        self.init_arg_parser()
        self.verbose = False
        self.allow_delay = False
        
    def usage(self, script_name):
        print 'rba_to_rdgen.py rba_trace_file.ktr ouput_file_prefix config_file_name.txt'
    def init_arg_parser(self):
        desc = 'Convert an rba trace file into an rdgen playback script.'

        self._parser = argparse.ArgumentParser(description=desc)
        self._parser.add_argument("-f", "--file", dest="filename",
                                  help="rba trace file", metavar="rba_trace_file.ktr", required=True)
        self._parser.add_argument("-c", "--config", dest="config_file",
                                  help="rdgen configuration file", metavar="rdgen_config_file.txt", required=True)
        self._parser.add_argument("-p", "--prefix", dest="prefix",
                                  help="output files prefix", metavar="prefix", required=True)
        self._parser.add_argument("-v", "--verbose", dest="verbose",
                                  action="store_true", default=False,
                                  help="show formatted output of all rba traces")
        self._parser.add_argument("-nd", "--no_delay", dest="no_delay",
                                  action="store_true", default=False,
                                  help="Do not mimic any delays in issuing requests.")
        self._parser.add_argument('--version', action='version', version='%(prog)s 0.1')

        #self._parser.add_argument("-q", "--quiet",
        #                    action="store_false", dest="verbose", default=True,
        #                    help="don't print status messages to stdout")
        #parser.add_argument("-o", "--object_id", dest="object_id", type=int,
        #                    help="filter by object id")
        

    def process_args(self):
        args = self._parser.parse_args()
        if args.filename == None:
            print "--file option is required"
            parser.print_help()
            return False
        self.file_name = args.filename

        if args.config_file == None:
            print "--config option is required"
            parser.print_help()
            return False
        self.config_file_name = args.config_file

        if args.prefix == None:
            print "--prefix option is required"
            parser.print_help()
            return False
        self.output_file_name = args.config_file
        self.verbose = args.verbose
        self.allow_delay = not args.no_delay
        return True

def calc_delta_msec(r1, r2, rtc_usec):
    delta_xsec = r1._ts - (r2._ts + r2.rt) 
    delta_msec = stamp_to_ms(delta_xsec, rtc_usec)
    return delta_msec

def calc_start_delta_msec(r1, r2, rtc_usec):
    delta_xsec = r1._ts - r2._ts 
    delta_msec = stamp_to_ms(delta_xsec, rtc_usec)
    return delta_msec

if __name__ == "__main__":
    app = app_control()

    if (app.process_args() == False):
        exit(0)
    
    print("input file is: " + app.file_name)
    print("output file is: " + app.output_file_name)
    print("config file is: " + app.config_file_name)
    if (app.verbose):
        print("verbose mode:   " + str(app.verbose))

    config_file = rdgen_config_file(app.config_file_name)
    config_file.read_file()

    if config_file.source == "LUN":
        rf_1 = rba_load_lun(app.file_name)
        package_id = "sep"
    elif config_file.source == "PDO":
        rf_1 = rba_load_pdo(app.file_name)
        package_id = "physical"
    else:
        print("Unknown source " + config_file.source)
        exit(0)
    boot_time = rf_1.boot_time
    boot_time_stamp = rf_1.boot_time_stamp
    rtc_usec = rf_1.rtc_usec

    rec_1 = rf_1.get_records()
    obj_1 = rf_1.get_objects()

    print "config file keys: "
    print config_file._mapping_dict.keys()

    first_rec = rec_1[0]
    first_ts = rec_1[0]._ts
    filter_dict = {}
    for tag in config_file._mapping_dict.keys():
        max_threads = 0

        # Filter out by tag.
        rec_list = [ r for r in rec_1 if r.get_id_name() == tag]
        rec_list = [ r for r in rec_list if r.count != 0]

        rdgen_rec_list = []

        # If there is a delay for starting any I/O on this object,
        # then add a delay record.
        if app.allow_delay and (rec_list[0]._ts > first_ts):
            delta_msec = calc_start_delta_msec(rec_list[0], first_rec, rf_1.rtc_usec)
            rdgen_rec_list.append(rdgen_delay_record(delta_msec))
            if app.verbose:
                print "Add delay to start of %d msec" % (delta_msec)
        i=0
        for r in rec_list:
            # If there is a point where we completed I/O and waited before
            # issuing the next I/O, then add a delay record.
            if app.allow_delay and i != 0 and r.queue_io == 1 and rec_list[i - 1].queue_io == 1:
                delta_xsec = r._ts - (rec_list[i - 1]._ts + rec_list[i - 1].rt) 
                delta_msec = stamp_to_ms(delta_xsec, rf_1.rtc_usec)
                # Insert a new entry to track the delay with 0 threads.
                if delta_msec > 1:
                    rdgen_rec_list.append(rdgen_delay_record(delta_msec))
                    if app.verbose:
                        print "Add delay between records of %d msec" % (delta_msec)
                    #print "added delay record %d ms first: %d" % (delta_msec, first_ts)
            if r.queue_io > max_threads:
                max_threads = r.queue_io

            # With rdgen we need to kick off the next thread now so our max threads need to be set
            # to the next record's
            if i < len(rec_list) - 1:
                if rec_list[i + 1].queue_io > r.queue_io:
                    r.queue_io = rec_list[i + 1].queue_io

            description = rf_1.get_record_desc(r)
            if r.cmd == 2:
                # Make writes low priority
                r.priority = priority_dict['LOW']
            elif r.cmd == 1:
                r.priority = priority_dict['NORMAL']
            
            new_rec = rdgen_record(r, None)
            new_rec._interface = rdgen_interface_dict[config_file._interface_dict[tag]]
            #print new_rec._interface
            rdgen_rec_list.append(new_rec)
            i = i + 1
            if app.verbose:
                print description #+ " " + str(r.get_id_name()) 

        if config_file._interface_dict[tag] == "packet":
            print "Object id: %d Max threads is: %d " % (config_file._mapping_dict[tag], max_threads)
            filter_dict[tag] = rdgen_object_filter(config_file._mapping_dict[tag], package_id, max_threads)
        else:
            print "device: %s intf: %s Max threads is: %d " % (config_file._mapping_dict[tag], config_file._interface_dict[tag], max_threads)
            filter_dict[tag] = rdgen_device_filter(config_file._mapping_dict[tag], int(tag), max_threads, config_file._interface_dict[tag])
        write_rdgen_object_file(config_file, filter_dict[tag], rdgen_rec_list, app.output_file_name)

    write_rdgen_root_file(config_file, app.output_file_name, filter_dict)
