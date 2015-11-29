import rbafilter
from rbafilter import *
from rbautils import *
from matplotlib import pyplot as plt
import sys
import time

class rg_statics:
    def __init__(self, resp, queue_io, l):
        self.resp = resp
        self.queue_io = queue_io
        self.r = l


def rba_get_rg_statics(lun_list, rg_luns):
    rgs = []
    queue_io = [0] * len(rg_luns)
    for l in lun_list:
        for i in range(len(rg_luns)):
            if l.oid == rg_luns[i]:
                queue_io[i] = l.queue_io
                rg_ios = sum(queue_io)
                resp = l.resp
                rg = rg_statics(resp, rg_ios, l)
                rgs.append(rg)
    return rgs

if __name__ == "__main__":

    def load(load_func, filename, time_offset_ms=0, need_done=False):
        rf = load_func(filename, need_done)
        rec = rf.get_records()
        obj = rf.get_objects()
        rf.calc_record_time(rec)
        for r in rec:
            r.start_xsec += time_offset_ms * 10000.0
        resp = [r for r in rec if r.rba_end == True]
        return {
            "file": rf, "rec": rec, "obj": obj, "resp": resp,
            }

    def load_lun(filename, time_offset_ms=0, need_done=False):
        return load(rba_load_lun, filename, time_offset_ms, need_done)

    def load_pdo(filename, time_offset_ms=0, need_done=False):
        return load(rba_load_pdo, filename, time_offset_ms, need_done)

    # # seagate
    sp = load_pdo("../8_6/seagate_w_zero.ktr", need_done=True)
    # pdo_0_0_18 = [r for r in sp["rec"] if r.oid == 18]
    # show_record_list(pdo_0_0_18, "Seagate W Zero")

    # sp = load_pdo("../8_6/hitachi_w_zero.ktr", need_done=True)
    # pdo_0_3_13 = [r for r in sp["rec"] if r.oid == ((3 << 16) + 13)]
    # show_record_list(pdo_0_3_13, "Hitachi W Zero")

    # sp = load_pdo("../8_6/mix_seagate.ktr", need_done=True)
    # pdo_0_0_18 = [r for r in sp["rec"] if r.oid == 18]
    # show_record_list( pdo_0_0_18, "Seagate Mix")

    # sp = load_pdo("../8_6/mix_hitachi.ktr", need_done=True)
    # pdo_0_3_11 = [r for r in sp["rec"] if r.oid == ((3 << 16) + 11)]
    # show_record_list(pdo_0_3_11, "Hitachi W Zero")

    #spa = load_lun("../rbafile1.ktr", time_offset_ms=8)
    #spb = load_lun("../SPB/rbafile1.ktr")
    # rf_1 = rba_load_lun("../rbafile1.ktr")
    # rf_2 = rba_load_lun("../SPB/rbafile1.ktr")
    # rec_1 = rf_1.get_records()
    # obj_1 = rf_1.get_objects()
    # resp_1 = [ r for r in rec_1 if r.rba_end == True]
    # rba_record_calc_time(resp_1, boot_time, boot_time_stamp, rtc_usec)


    # rg_140_lun = range(16)
    # rg_140_statics = show_rg(rg_140_lun, "140")

    # rg_159_lun = range(16, 32)
    # rg_159_statics = show_rg(rg_159_lun, "159")

    # rg_16f = range(32, 42)
    # rg_16f_statics = show_rg(rg_16f, "16f")
    # #print rg_16f_statics[5902]

    # rg_17f = range(42, 52)
    # rg_17f_statics = show_rg(rg_17f, "17f")
    # #print rg_17f_statics[1068]

    # rg_18f = range(52, 62)
    # rg_18f_statics = show_rg(rg_18f, "18f")
    # #print rg_18f_statics[816]

    # rg_1a0 = range(62, 70)
    # rg_1a0_statics = show_rg(rg_1a0, "1a0")
    # #print rg_1a0_statics[2626]

    # rg_1af = range(70, 78)
    # rg_1af_statics = show_rg(rg_1af, "1af")
    # #print rg_1af_statics[1443]

    # rg_1ba = range(78, 82)
    # rg_1ba_statics = show_rg(rg_1ba, "1ba")
