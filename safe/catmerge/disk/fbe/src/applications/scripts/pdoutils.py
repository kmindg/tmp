#!/usr/bin/python

from rbafile import *
from rbautils import *
from rbafilter import *

def pdo_load(file_name, need_done=False):
    rf = rba_load_pdo(file_name, need_done)
    rec = rf.get_records()
    obj = rf.get_objects()
    rf.calc_record_time(rec)
    print "PDOs load done. Dividing for each PDO"
    pdo = []
    for _, v in obj.items():
        # WARN: this may be very slow as it need to loop rec list
        # for multiple times.
        v.rl = [ r for r in rec if r.get_object_id() == v.oid ]
        pdo.append(v)

    return pdo


def pdo_get_avg_resp_time(pdo):
    rt = get_avg_resp_time(pdo.rl)
    return rt

def pdo_sort(pdo_list, measurement=pdo_get_avg_resp_time, sort_func=None):
    if measurement != None:
        for pdo in pdo_list:
            pdo.measurement = measurement(pdo)

    pdo_list.sort(cmp=sort_func)
    return pdo_list

def pdo_sort_by_avg_write_resp_time(pdo_list):
    def sort_by_wrt(a, b):
        m0 = a.measurement
        m1 = b.measurement
        return -cmp(m0["avg_wrt"], m1["avg_wrt"])

    return pdo_sort(pdo_list, pdo_get_avg_resp_time, sort_by_wrt)

def pdo_sort_by_avg_read_resp_time(pdo_list):
    def sort_by_rrt(a, b):
        m0 = a.measurement
        m1 = b.measurement
        return -cmp(m0["avg_rrt"], m1["avg_rrt"])

    return pdo_sort(pdo_list, pdo_get_avg_resp_time, sort_by_rrt)

def pdo_sort_by_avg_resp_time(pdo_list):
    def sort_by_rt(a, b):
        m0 = a.measurement
        m1 = b.measurement
        return -cmp(m0["avg_rt"], m1["avg_rt"])

    return pdo_sort(pdo_list, pdo_get_avg_resp_time, sort_by_rt)

if __name__ == "__main__":
    import sys
    import time

    st = time.clock()
    pdo_list = pdo_load(sys.argv[1])
    end = time.clock()
    print "Used: %.3fs" % (end - st)

    print "\nSort by avg write resp time:"
    _ = pdo_sort_by_avg_write_resp_time(pdo_list)
    for i in pdo_list:
        print i, "%.3fms" % i.measurement["avg_wrt"]

    print "\nSort by avg read resp time:"
    _ = pdo_sort_by_avg_read_resp_time(pdo_list)
    for i in pdo_list:
        print i, "%.3fms" % i.measurement["avg_rrt"]

    print "\nSort by avg resp time:"
    _ = pdo_sort_by_avg_resp_time(pdo_list)
    for i in pdo_list:
        print i, "%.3fms" % i.measurement["avg_rt"]

