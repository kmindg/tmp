#!/usr/bin/python

import sys
import os
import time
from struct import *
from rbadef import *

class rba_record:
    def __init__(self, ts, tr, tid, a0, a1, a2, a3):
        self._ts = ts
        self._tr = tr
        self._tid = tid
        self._a0 = a0
        self._a1 = a1
        self._a2 = a2
        self._a3 = a3

        # We need to mask low 32 bits because
        # CBFS mixup traffic_id with dev_id
        self.tid = (self._tid & 0xffffffff)
        self.cpu = tr & 0x7
        self.cmd = a3 & 0xfffe
        self.cmd_is_done = (a3 & 1) != 0
        self.rba_end = False
        self.rt = 0
        self.oid = -1
        self.lba = 0
        self.count = 0
        self.queue_io = 0

    def __hash__(self):
        return (self.tid << 56) + (self._a0 << 32) + (self._a2 << 16) + (self.cmd)

    def __eq__(self, other):
        return (self.tid == other.tid) and (self._a0 == other._a0) \
            and (self._a2 == other._a2) and (self.cmd == other.cmd)

    def __str__(self):
        if self.rba_end:
            end_info = "  END: %u" % self.rt
        else:
            end_info = ""
        return hex(self.tid) + " " + hex(self._a0) + " " + hex(self._a1) + " " + hex(self._a2) + " " + hex(self._a3) + " cpu " + str(self.cpu) + end_info

    def is_done_operation(self):
        return self.cmd_is_done

    def get_object_id(self):
        if self.oid == -1:
            return None
        else:
            return (self.tid, self.oid)
    def get_raw_cmd(self):
        return self._a3 & 0xffff
    def set_completion(self, rba):
        #self._rba_end = rba # completion record
        self.rba_end = True
        self.rt = (rba._ts - self._ts)
        if self.rt < 0:
            print "WARN:", self, rba
            print "   self.ts: %u, rba.ts: %u" % (self._ts, rba._ts)
            self.rt = 0
            self.rba_end = False

    def get_key(self):
        return (self.tid, self._a0, self._a2, self.cmd)

    def get_id_name(self):
        return ""

    def set_queue_io(self, io_num):
        self.queue_io = io_num

class block_record(rba_record):
    def __init__(self, *rba_fields):
        rba_record.__init__(self, *rba_fields)
        self.oid = (self._a2 >> 32) & 0xffff
        self.lba = self._a0
        #print "0x%x 0x%x 0x%x" % (self.cmd & 0xC0, self.cmd & 0x40, self.cmd & 0x80)
        if (self.cmd & 0xC0) == 0xC0:
            self.priority = 3
        elif self.cmd & 0x40:
            self.priority = 1
        elif self.cmd & 0x80:
            #print "cmd: 0x%x " %self.cmd
            self.priority = 2        
        else:
            self.priority = 0
        self.count = self._a2 & 0xffffffff;
        #self.priority = priority_dict['NORMAL'] # Default is normal.

    def get_key(self):
        return (self.tid, self.oid, self.lba, self.count, self.cmd)

    def get_id_name(self):
        return str(self.oid)

    def __hash__(self):
        return (self.tid << 56) + (self.oid << 32) + self.lba + self.count + self.cmd

    def __eq__(self, other):
        return (self.tid == other.tid) and (self.oid == other.oid) and (self.lba == other.lba) \
            and (self.count == other.count) and (self.cmd == other.cmd)

    def __str__(self):
        if self.cmd_is_done:
            info = "(END)"
        else:
            info = "(START)"
        info = get_rba_traffic_cmd_name(self.cmd) + info
        if self.rba_end:
            info = "Done " + str(self.rt) + ", " + info
        name = get_rba_traffic_id_name(self.tid)
        id_name = self.get_id_name()

        return name + " " + id_name + " LBA " + hex(self.lba) + ", " \
            + "BLKS " + hex(self.count) + ", " \
            + "Q " + str(self.queue_io) + ", " + info


class tcd_record(block_record):
    def __init__(self, *rba_fields):
        block_record.__init__(self, *rba_fields)
        self.oid = self._a3 >> 16
        self.lba = self._a1
        self.count = self._a2


class psm_record(block_record):
    def __init__(self, *rba_fields):
        block_record.__init__(self, *rba_fields)
        self.oid = self._a3 >> 16
        self.lba = (self._a0 << 32) + self._a1
        self.count = self._a2

# LUN share with FBE_LUN
# FRU is unused, share with RG_FRU
# DBE is unused, return rba_record by default
# SFE is unused

class dml_record(block_record):
    def __init__(self, *rba_fields):
        block_record.__init__(self, *rba_fields)
        self.oid = self._a3 >> 16
        self.lba = (self._a0 << 32) + self._a1
        self.count = self._a2

class mvs_record(block_record):
    def __init__(self, *rba_fields):
        block_record.__init__(self, *rba_fields)
        self.oid = self._a3 >> 16
        self.lba = (self._a0 << 32) + self._a1
        self.count = self._a2

# MVA is unused

class mlu_record(block_record):
    def __init__(self, *rba_fields):
        block_record.__init__(self, *rba_fields)
        self.oid = self._a3 >> 16
        self.lba = (self._a0 << 32) + self._a1
        self.count = self._a2


class snap_record(block_record):
    def __init__(self, *rba_fields):
        block_record.__init__(self, *rba_fields)
        self.oid = self._a3 >> 16
        self.lba = (self._a0 << 32) + self._a1
        self.count = self._a2


class agg_record(block_record):
    def __init__(self, *rba_fields):
        block_record.__init__(self, *rba_fields)
        self.oid = self._a3 >> 16
        self.lba = (self._a0 << 32) + self._a1
        self.count = self._a2


class mig_record(block_record):
    def __init__(self, *rba_fields):
        block_record.__init__(self, *rba_fields)
        self.oid = self._a3 >> 16
        self.lba = (self._a0 << 32) + self._a1
        self.count = self._a2


class clones_record(block_record):
    def __init__(self, *rba_fields):
        block_record.__init__(self, *rba_fields)
        self.lba = (self._a0 << 32) + self._a1
        self.count = self._a2
        self.cmd = self._a3 & 0xffe # only low 12 bits is used as command
        self.oid = self._a3 >> 12

    def get_id_name(self):
        flu = self.oid >> 16
        index = (self.oid >> 12) & 0xf
        return "%u_%u" % (flu, index)


class cpm_record(block_record):
    def __init__(self, *rba_fields):
        block_record.__init__(self, *rba_fields)
        self.lba = (self._a0 << 32) + self._a1
        self.count = self._a2
        self.cmd = self._a3 & 0xffe # only low 12 bits is used as command
        self.oid = self._a3 >> 12

    def get_id_name(self):
        did = self.oid >> 17
        iodid = (self.oid >> 12) & 0x1f
        return "%u_%u" % (did, iodid)


class fedisk_record(block_record):
    def __init__(self, *rba_fields):
        block_record.__init__(self, *rba_fields)
        self.lba = (self._a0 << 32) + self._a1
        self.count = self._a2
        self.cmd = self._a3 & 0xffe # only low 12 bits is used as command
        self.oid = self._a3 >> 12


class compress_record(block_record):
    def __init__(self, *rba_fields):
        block_record.__init__(self, *rba_fields)
        self.lba = (self._a0 << 32) + self._a1
        self.count = self._a2
        self.oid = self._a3 >> 16

# FBE is unused

class fct_record(block_record):
    def __init__(self, *rba_fields):
        block_record.__init__(self, *rba_fields)
        self.lba = (self._a0 << 32) + self._a1
        # WARNING: fct count is untrustable
        self.count = self._a2 & 0xffff
        flu = (self._a3 >> 16)
        self.oid = flu


class fbe_lun_record(block_record):
    pass


class rg_fru_record(block_record):
    def __init__(self, *rba_fields):
        block_record.__init__(self, *rba_fields)
        rg_number = (self._a2) >> 16
        fru_pos = self._a1 & 0xffff
        self.oid = (rg_number << 16) + fru_pos

    def get_id_name(self):
        rg_number = self.oid >> 16
        fru_pos = self.oid & 0xffff
        return "%u_%u" % (rg_number, fru_pos)


class raid_record(block_record):
    pass


# LDO is unused

class pdo_record(block_record):
    def __init__(self, *rba_fields):
        block_record.__init__(self, *rba_fields)
        self.oid = (self._a3 >> 16)
        self.lba = (self._a0 & 0xffffffff00000000) + \
                   (self._a1 & 0x00000000ffffffff)
        self.count = self._a2

    def get_id_name(self):
        fru = self.oid
        slot = fru & 0xffff
        enc = (fru >> 16) & 0xffff
        port = (fru >> 32) & 0xffff
        return "%u_%u_%u" % (port, enc, slot)


# Port is unused

class vd_record(block_record):
    pass


class pvd_record(block_record):
    def __init__(self, *rba_fields):
        block_record.__init__(self, *rba_fields)
        self.oid = (self._a3 >> 16)
        self.lba = self._a0
        self.count = self._a2

    def get_id_name(self):
        fru = self.oid
        slot = fru & 0xffff
        enc = (fru >> 16) & 0xffff
        port = (fru >> 32) & 0xffff
        return "%u_%u_%u" % (port, enc, slot)


class dds_record(block_record):
    def __init__(self, *rba_fields):
        block_record.__init__(self, *rba_fields)
        self.lba = self._a0
        self.count = self._a2 & 0xffffffff
        flu = (self._a2 >> 32) & 0xffff
        pool_id = (self._a2 >> 48)
        self.oid = (pool_id << 16) + flu

    def get_id_name(self):
        flu = self.oid & 0xffff
        pool_id = self.oid >> 16
        return "%u_%u" % (pool_id, flu)


class spc_record(block_record):
    def __init__(self, *rba_fields):
        block_record.__init__(self, *rba_fields)
        self.lba = (self._a1 << 64) + self._a0
        self.count = self._a2 & 0xffffffff;

        src_lun = (self._a2 >> 32) & 0xffff
        dst_lun = (self._a2 >> 48) & 0xffff
        self.oid = (src_lun << 16) + dst_lun

    def get_id_name(self):
        src_lun = self.oid >> 16
        dst_lun = self.oid & 0xffff
        return "%u_%u" % (src_lun, dst_lun)


class cbfs_record(block_record):
    def __init__(self, *rba_fields):
        block_record.__init__(self, *rba_fields)
        self.lba = self._a0
        self.count = self._a2 & 0xffffffff

        devid = self._tid >> 32
        inum = self._a2 >> 32
        self.oid = (devid << 32) | inum

    def get_id_name(self):
        devid = self.oid >> 32
        inum = self.oid & 0xffffffff
        return "%u_%u" % (devid, inum)


class ccb_record(block_record):
    def __init__(self, *rba_fields):
        block_record.__init__(self, *rba_fields)
        self.lba = (self._a0 << 32) + self._a1
        self.count = self._a2
        self.oid = self._a3 >> 16


class rba_object:
    def __init__(self, object_id, record = None):
        self.oid = object_id
        self.queue_depth = 0
        self.in_io = 0
        self.out_io = 0
        if record:
            self.name = record.get_id_name()
        else:
            self.name = str(oid[1])

    def __str__(self):
        name = get_rba_traffic_id_name(self.oid[0])
        return name + "-" + self.name + ", " + "IO: " \
            + str(self.queue_depth) + "(%u %u)" % (self.in_io, self.out_io)

    def queue_io(self, rba_record, is_inc):
        if is_inc:
            self.queue_depth += 1
            self.in_io += 1
        else:
            if self.queue_depth <= 0:
                print "WARN: internal bug"
            else:
                self.queue_depth -= 1
                self.out_io += 1
        rba_record.set_queue_io(self.queue_depth)
        #print "IO: ", self


class record_list():
    def __init__(self, rtc_usec=2.343798):
        self.clear()
        self.rtc_usec = rtc_usec

    def clear(self):
        self.rlist = []
        # use hash to speedup recording finding
        self.record_hash = { }
        self.objects = { }
        self.warn_unsupport = { }
        self.rec_none = 0
        self.rec_start = 0
        self.rec_pending = 0
        self.rec_done = 0
        self.rec_ignore = 0
        self.cur_stamp = 0
        self.rec_ignore = 0
        self.in_clear = 0

    def clear_hash(self):
        self.record_hash = { }

    def warn_once(self, record):
        tid = record.tid
        is_warn = self.warn_unsupport.get(tid)
        if not is_warn:
            self.warn_unsupport[tid] = 1
            print "WARN: unsupport type 0x%x" % tid

    def add_record_to_hash(self, record):
        key = record.get_key()
        r = self.record_hash.get(key)
        if r:
            r.append(record)
        else:
            self.record_hash[key] = [record]

    def remove_record_from_hash(self, record):
        key = record.get_key()
        r = self.record_hash.get(key)
        if r:
            start_record = r.pop(0)
            if r == []:
                del self.record_hash[key]
        else:
            start_record = None
        return start_record

    def find_object(self, record, is_start):
        object_id = record.get_object_id()
        if object_id == None:
            self.warn_once(record)
            return None

        obj = self.objects.get(object_id)
        if obj == None and is_start:
            # Auto create the object if we found a start io
            obj = rba_object(object_id, record)
            print "INFO: add new object:", obj
            self.objects[object_id] = obj
        return obj

    def queue_io(self, record, is_start):
        obj = self.find_object(record, is_start)
        if obj:
            obj.queue_io(record, is_start)

    def finish_record(self, done_record, need_done):
        start_record = self.remove_record_from_hash(done_record)
        if start_record:
            start_record.set_completion(done_record)
            self.queue_io(done_record, False)
            self.rec_pending -= 1
            self.rec_done += 1
        else:
            self.rec_ignore += 1

        if need_done:
            self.rlist.append(done_record)

    def add_record(self, record):
        self.rlist.append(record)
        self.add_record_to_hash(record)
        self.queue_io(record, True)
        self.rec_start += 1
        self.rec_pending += 1

    def push(self, record, need_done = False):
        if not record:
            self.rec_none += 1
            return

        if record._ts < self.cur_stamp:
            if not self.in_clear:
                time_stamp = self.cur_stamp - record._ts
                sec = time_stamp / self.rtc_usec / 1000000.0
                print "WARN: found rba before %.3fs, reset hash" % (sec)
                self.clear_hash()
                self.in_clear = 1
            return
        else:
            self.in_clear = 0

        if self.cur_stamp == 0:
            self.cur_stamp = record._ts

        is_done = record.is_done_operation()
        if is_done:
            self.finish_record(record, need_done)
        else:
            self.add_record(record)

    def get_info(self):
        return (self.rlist, self.objects)

    def get_statics(self):
        rec_dict = {
            "total": self.rec_none + self.rec_start + self.rec_done + self.rec_ignore,
            "none": self.rec_none, "start": self.rec_start, "done": self.rec_done,
            "pending": self.rec_pending, "ignore": self.rec_ignore,
            "clear_count": self.in_clear,
            }
        return rec_dict


rba_type_class = (
    None,                 # KT_NULL
    None,                 # KT_TIME
    None,                 # KT_LOST
    None,                 # KT_FILE_OPEN_WRITE
    None,                 # KT_FILE_OPEN_READ
    None,                 # KT_FILE_CLOSE
    None,                 # KT_FILE_CLOSED
    tcd_record,           # KT_TCD_TRAFFIC
    psm_record,           # KT_PSM_TRAFFIC
    fbe_lun_record,       # KT_LUN_TRAFFIC
    rba_record,           # KT_FRU_TRAFFIC
    rba_record,           # KT_DBE_TRAFFIC
    rba_record,           # KT_SFE_TRAFFIC
    dml_record,           # KT_DML_TRAFFIC
    mvs_record,           # KT_MVS_TRAFFIC
    rba_record,           # KT_MVA_TRAFFIC
    mlu_record,           # KT_MLU_TRAFFIC
    snap_record,          # KT_SNAP_TRAFFIC
    agg_record,           # KT_AGG_TRAFFIC
    mig_record,           # KT_MIG_TRAFFIC
    clones_record,        # KT_CLONES_TRAFFIC
    cpm_record,           # KT_CPM_TRAFFIC
    fedisk_record,        # KT_FEDISK_TRAFFIC
    compress_record,      # KT_COMPRESSION_TRAFFIC
    rba_record,           # KT_FBE_TRAFFIC
    fct_record,           # KT_FCT_TRAFFIC
    fbe_lun_record,       # KT_FBE_LUN_TRAFFIC
    rg_fru_record,        # KT_FBE_RG_FRU_TRAFFIC
    raid_record,          # KT_FBE_RG_TRAFFIC
    None,                 # 0x1d
    None,                 # 0x1e
    None,                 # 0x1f
    rba_record,           # KT_FBE_LDO_TRAFFIC
    pdo_record,           # KT_FBE_PDO_TRAFFIC
    rba_record,           # KT_FBE_PORT_TRAFFIC
    vd_record,            # KT_FBE_VD_TRAFFIC
    pvd_record,           # KT_FBE_PVD_TRAFFIC
    dds_record,           # KT_DDS_TRAFFIC
    spc_record,           # KT_SPC_TRAFFIC
    cbfs_record,          # KT_CBFS_TRAFFIC
    ccb_record,           # KT_CCB_TRAFFIC
)

def __make_record(raw_record):
    # format: (ts, tr, tid, a0, a1, a2, a3 )
    tid = raw_record[2] & 0xffffffff
    if tid >= len(rba_type_class):
        return None
    cls = rba_type_class[tid]
    if cls == None:
        return None
    else:
        return cls(*raw_record)

def make_record(rba):
    raw_record = unpack(record_format, rba)
    return __make_record(raw_record)

def filter_record(rba, rba_filter = None):
    raw_record = unpack(record_format, rba)
    if rba_filter:
        raw_record = rba_filter(raw_record)
    if raw_record:
        return __make_record(raw_record)
    else:
        return None

if __name__ == "__main__":
    import sys

    def test_classes():
        rba = (0, 1, 2, 3, 4, 5, 6)
        for i in rba_type_class:
            if i:
                t = i(*rba)
                print t

    def test_filter(rba):
        return rba
        tid = rba[2]
        name = get_rba_traffic_id_name(tid)
        if name == "PDO":
            return rba
        else:
            return None

    if len(rba_type_class) != len(rba_id_array):
        print "Len unmatch: %u %u" % (len(rba_type_class), len(rba_id_array))
        sys.exit(1)

    print "Self test"
    test_classes()
    count = 0
    f = open(sys.argv[1], "rb")
    rl = record_list()

    f.read(512)
    record_size = calcsize(record_format)
    record_data = f.read(record_size)
    while record_data:
        if len(record_data) != record_size:
            print "Error: len: %u" % len(record_data)
            break
        record = filter_record(record_data, test_filter)
        rl.push(record, True)
        count -= 1
        if count == 0:
            break
        record_data = f.read(record_size)
    f.close()

    records, objects = rl.get_info()
    for k, v in objects.items():
        print v
    for i in range(99, 99 + 10):
        print records[i]

    print rl.get_statics()
    #del rl
    #del records
    #del objects
