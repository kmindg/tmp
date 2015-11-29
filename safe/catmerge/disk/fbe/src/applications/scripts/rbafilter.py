#!/usr/bin/python

import sys
from rbadef import *
import os
from os import path
from rbafile import *

def rba_lun_filter(rba):
    tid = rba[2] & 0xffffffff
    if tid == RBA_traffic_type.KT_LUN_TRAFFIC or tid == RBA_traffic_type.KT_FBE_LUN_TRAFFIC:
        return rba
    return None

def rba_rg_filter(rba):
    tid = rba[2] & 0xffffffff
    if tid == RBA_traffic_type.KT_FBE_RG_TRAFFIC:
        return rba
    return None

def rba_pdo_filter(rba):
    tid = rba[2] & 0xffffffff
    if tid == RBA_traffic_type.KT_FBE_PDO_TRAFFIC:
        return rba
    return None

def rba_load_with_filter(filename, rfilter=None, need_done=False):
    rf = rba_file(filename)
    f = open(filename, "rb")
    rf.read_header(f)
    rf.read_records(f, rfilter=rfilter, need_done=need_done)
    return rf

def rba_load_all(filename, need_done=False):
    return rba_load_with_filter(filename, rfilter=None, need_done=need_done)

def rba_load_lun(filename, need_done=False):
    return rba_load_with_filter(filename, rfilter=rba_lun_filter, need_done=need_done)

def rba_load_rg(filename, need_done=False):
    return rba_load_with_filter(filename, rfilter=rba_rg_filter, need_done=need_done)

def rba_load_pdo(filename, need_done=False):
    return rba_load_with_filter(filename, rfilter=rba_pdo_filter, need_done=need_done)
