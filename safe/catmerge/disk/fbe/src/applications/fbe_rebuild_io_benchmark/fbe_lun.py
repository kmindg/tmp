#!/usr/bin/python
from fnmatch import fnmatch

class fbe_lun:
    def __init__(self, info):
        self.object_id = int(info["object_id"], 0)
        self.has_rate = False
        self.io_count = 0
        self.threads = 0
        self.errors = 0
        self.rate = 0
        self.avg_response = 0
        self.read_rate = 0
        self.read_avg_response = 0
        self.write_rate = 0
        self.write_avg_response = 0
    def reset(self):
        
        self.has_rate = False
        self.io_count = 0
        self.threads = 0
        self.errors = 0
        self.rate = 0
        self.avg_response = 0
        self.read_rate = 0
        self.read_avg_response = 0
        self.write_rate = 0
        self.write_avg_response = 0

    def get_desc(self):
        s =  "LUN obj: " + hex(self.object_id)
        return s

    def __str__(self):
        s = self.get_desc()
        if self.has_rate:
            s += ", Rate: %.2f" % (self.rate)
        return s

    def get_stats(self):
        st = "%40s %10d %10d %10d %20.2f" % (self.object_id, self.threads, self.io_count, self.errors, self.rate)
        return st

    def __repr__(self):
        return "LUN(%s)" % str(self)
