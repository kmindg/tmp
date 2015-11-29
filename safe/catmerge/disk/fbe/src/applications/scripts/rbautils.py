#!/usr/bin/python
#utils for ipython command line
import sys
from rbadef import *
import os
from os import path
from rbafile import *
from rbadef import *
from matplotlib import pyplot as plt

def rba_record_calc_time(rl, boot_time, boot_time_stamp, rtc_usec):
    for r in rl:
        r.resp = r.rt / rtc_usec / 1000
        utc = stamp_to_utc(r._ts, boot_time, boot_time_stamp, rtc_usec)
        gm = utc[0]
        ms = utc[1]
        r.start_time = "%02u:%02u:%02u.%03u" % (gm.tm_hour, gm.tm_min, gm.tm_sec, int(ms))


def record_merge(rl1, rl2):
    r1_len = len(rl1)
    r2_len = len(rl2)

    i1 = 0
    i2 = 0
    l = []
    while i1 < r1_len and i2 < r2_len:
        r1 = rl1[i1]
        r2 = rl2[i2]
        if r1.start_xsec <= r2.start_xsec:
            l.append(r1)
            i1 += 1
        else:
            l.append(r2)
            i2 += 1

    if i1 < r1_len:
        l.extend(rl1[i1:])
    if i2 < r2_len:
        l.extend(rl2[i2:])

    return l


def map_record_iops(rl, rt):
    io_t = [0]
    iops = [0]
    ios = 0
    st = 0
    step_time = 0.2
    for i in range(len(rl)):
        r = rl[i]
        if not r.cmd_is_done:
            t = rt[i]
            if t > st + step_time:
                io_t.append(st + step_time)
                iops.append(ios)
                st += step_time
                ios = 0
                while (st + step_time < t):
                    io_t.append(st + step_time)
                    iops.append(0)
                    st += step_time

            ios += 1
    return { "st": io_t, "iops": iops }

def map_record_lba(rl, st):
    rt = []
    wt = []
    rlba = []
    wlba = []
    for i in range(len(rl)):
        r = rl[i]
        cmd = r.cmd & 0xe
        if not r.cmd_is_done:
            t = st[i]
            lba = r.lba
            if cmd == 0x00:
                rt.append(t)
                rlba.append(lba)
            elif cmd == 0x02:
                wt.append(t)
                wlba.append(lba)
            else:
                print "Skip cmd: 0x%02x" % cmd
    return {"rt":rt, "rlba":rlba, "wt":wt, "wlba":wlba}

def map_record_resp_time(rl, st):
    wt = []
    wrt = []
    rt = []
    rrt = []

    for i in range(len(rl)):
        r = rl[i]
        if not r.cmd_is_done:
            if r.cmd == 0:
                rt.append(st[i])
                rrt.append(r.resp)
            elif r.cmd == 2:
                wt.append(st[i])
                wrt.append(r.resp)
    return { "rt": rt, "rrt": rrt, "wt": wt, "wrt": wrt }

def map_record_name(rl):
    r = rl[0]
    traffic_name = get_rba_traffic_id_name(r.tid)
    id_name = r.get_id_name()
    return "%s %s" % (traffic_name, id_name)

def show_record_list(rl, info=""):
    def to_ms(xsec, base):
        return (xsec - base) / 10000.0 / 1000.0

    name = map_record_name(rl)
    base_xsec = rl[0].start_xsec
    st = [ to_ms(r.start_xsec, base_xsec) for r in rl ]

    wr_lba = map_record_lba(rl, st)

    plt.figure()
    plt.title(info)
    plt.subplot(411)
    plt.grid()
    plt.title("LBA range - %s" % name)
    plt.plot(wr_lba["rt"], wr_lba["rlba"], "gh")
    plt.plot(wr_lba["wt"], wr_lba["wlba"], "rH")

    plt.subplot(412)
    plt.grid()
    plt.title("Incoming IOPS - %s" % name)
    iops = map_record_iops(rl, st)
    plt.plot(iops["st"], iops["iops"])

    plt.subplot(413)
    plt.grid()
    plt.title("Queue IO - %s" % name)
    queue_io = [ r.queue_io for r in rl ]
    plt.plot(st, queue_io)

    plt.subplot(414)
    plt.grid()
    plt.title("Resp time - %s" %name)
    wr_rt = map_record_resp_time(rl, st)
    plt.plot(wr_rt["rt"], wr_rt["rrt"], "gh", wr_rt["wt"], wr_rt["wrt"], "rh")
    plt.show()


def get_record_resp_time(rl):
    rrt = []
    wrt = []
    for r in rl:
        if r.cmd_is_done:
            pass
        elif r.cmd == 0:
            rrt.append(r.resp)
        elif r.cmd == 2:
            wrt.append(r.resp)

    return { "rrt": rrt, "wrt": wrt }

def get_avg_resp_time(rl):
    rt = get_record_resp_time(rl)
    wrt = rt["wrt"]
    rrt = rt["rrt"]
    avg_wrt = 0.0
    avg_rrt = 0.0
    avg_rt = 0.0

    if len(wrt) != 0:
        avg_wrt = sum(wrt) / len(wrt)
    if len(rrt) != 0:
        avg_rrt = sum(rrt) / len(rrt)
    if len(wrt) + len(rrt) != 0:
        avg_rt = (sum(wrt) + sum(rrt)) / (len(wrt) + len(rrt))
    return { "avg_wrt": avg_wrt, "avg_rrt": avg_rrt, "avg_rt": avg_rt }

