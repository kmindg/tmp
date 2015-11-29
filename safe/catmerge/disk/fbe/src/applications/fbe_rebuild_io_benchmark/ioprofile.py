#!/usr/bin/python

from fnmatch import fnmatch
from fbe_rg import fbe_rg
import re
rdgen_perf_lun_command = "rdgen -object_id 0x%(obj)x -package_id sep -perf "


class IOBenchmark:
    def __init__(self, number, name, cmd_line, desc, ratio = 0):
        self.number = number
        self.name = "%(thread)s_" + name
#        print self.name
        self.base_test = name
        self.cmd_line = cmd_line
        self.desc = desc
        self.ratio = ratio

    def get_name(self):
        return "m%d_%s" % (self.multiplier, self.base_test)

    def get_desc(self):
        return "%-30s%s" % (self.base_test, self.desc)

    def get_cmd_line(self, lun):
        cmd_list = []
        items = [item.strip() for item in self.cmd_line.split(":")]
        for item in items:
            if item != "":
                cmd = rdgen_perf_lun_command + item
                cmd = cmd % {'obj': lun.object_id}
                cmd_list.append(cmd)
        return "\n".join(cmd_list)

    def __str__(self):
        return self.name

    def __repr__(self):
        return str(self)
    
    def __eq__(self, other):
        return self.__dict__ == other.__dict__
    
    def __hash__(self):
        return (self.name, self.cmd_line, self.desc).__hash__()


def find_lba_by_percentage(capacity, percentage, ismin):
    lba = int(capacity) * float(percentage)
    if ismin:
        return str(hex(int(lba)).rstrip("L"))
    else:
        return str(hex(int(lba-1)).rstrip("L"))


def find_lba_by_size(capacity, percentage, size):
    offsetlba = find_lba_by_percentage(capacity, percentage, True)
    blocks = size/520
    lba = int(offsetlba, 0) - 1 + blocks
    return str(hex(int(lba-1)).rstrip("L"))


FBE_lun_benchmarks1 = {
    IOBenchmark("LUN_8kb_random_write",
        "-constant -priority 2 w 2 %(thread)s 10:-constant -priority 2 w 2 %(thread)s 10:-constant -priority 2 w 2 %(thread)s 10",
        "Random write 8kb, %(thread)s threads on lun objects",
        1,
        ),
    IOBenchmark("LUN_8K_256k_random_r1_w2low",
        "-priority 1 w 2 %(thread)s 200:-priority 1 w 2 %(thread)s 200:-constant -align -priority 2 r 2 %(thread)s 10",
        "Random read(normal) 8kb and write(low) 256kb 1 by 2 ratio, %(thread)s threads on lun objects",
        2,
        ),
    }
FBE_lun_benchmarks = {
    IOBenchmark(0, "t0_8K_256k_random_r1_w2low",
        "-priority 1 w 2 %(thread)s 200:-priority 1 w 2 %(thread)s 200:-constant -align -priority 2 r 2 %(thread)s 10",
        "Random read(normal) 8kb and write(low) 256kb 1 by 2 ratio, %(thread)s threads on lun objects",
        2,
        ),
    IOBenchmark(1, "t1_8K_256k_random_r1_w2",
        "-priority 2 w 2 %(thread)s 200:-priority 2 w 2 %(thread)s 200:-constant -align -priority 2 r 2 %(thread)s 10",
        "Random read(normal) 8kb and write(low) 256kb 1 by 2 ratio %(thread)s threads on lun objects",
        2,
        ),
    IOBenchmark(2, "t2_8K_256k_random_r2_wlow",
        "-priority 1 w 2 %(thread)s 200:-constant -align -priority 2 r 2 %(thread)s 10:-constant -align -priority 2 r 2 %(thread)s 10",
        "Random read 8kb and write(low) 256kb 2 by 1 ratio, %(thread)s threads on lun objects",
        2,
        ),
    IOBenchmark(3, "t3_0x300_random_wlow",
        "-constant -align -priority 1 w 2 %(thread)s 300:-constant -align -priority 1 w 2 %(thread)s 300:-constant -align -priority 1 w 2 %(thread)s 300",
        "Random write 0x300 (low), %(thread)s threads on lun objects",
        1,
        ),
    IOBenchmark(4, "t4_0x400_random_read",
        "-constant -priority 2 r 2 %(thread)s 400:-constant -priority 2 r 2 %(thread)s 400:-constant -priority 2 r 2 %(thread)s 400",
        "Random read 0x400, %(thread)s threads on lun objects",
        1,
        ),
    IOBenchmark(5, "t5_8kb_random_read",
        "-constant -priority 2 r 2 %(thread)s 10:-constant -priority 2 r 2 %(thread)s 10:-constant -priority 2 r 2 %(thread)s 10",
        "Random read 8kb, %(thread)s threads on lun objects",
        1,
        ),
    IOBenchmark(6, "t6_8kb_random_write",
        "-constant -priority 2 w 2 %(thread)s 10:-constant -priority 2 w 2 %(thread)s 10:-constant -priority 2 w 2 %(thread)s 10",
        "Random write 8kb, %(thread)s threads on lun objects",
        1,
        ),
}
