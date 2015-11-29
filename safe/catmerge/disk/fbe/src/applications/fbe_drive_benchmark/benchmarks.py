#!/usr/bin/python

from fnmatch import fnmatch
from fbe_drive import fbe_drive
import re
rdgen_perf_pdo_command = "rdgen -object_id 0x%x -package_id physical -perf "
rdgen_perf_pvd_command = "rdgen -object_id 0x%x -package_id sep -perf "


class Benchmark:
    def __init__(self, name, cmd_line, desc):
        self.name = name
        self.cmd_line = cmd_line
        self.desc = desc

    def get_name(self):
        return self.name

    def get_desc(self):
        return "%-30s%s" % (self.name, self.desc)


    def get_cmd_line(self, drive):
        if re.findall("PVD", self.name) and drive.pvd_obj_id != 0 and re.findall("_rw", self.name):
            cmd = self.cmd_line % (drive.pvd_obj_id, drive.pvd_obj_id)
        elif re.findall("PVD", self.name) and drive.pvd_obj_id != 0:
            cmd = self.cmd_line % drive.pvd_obj_id
        elif re.findall("PVD", self.name) and drive.pvd_obj_id == 0:
            cmd = ""
        else:
            cmd = self.cmd_line % drive.object_id
        a = re.findall("(find_lba_by_.*?\(.*?\))", self.cmd_line)
        if a:
            for s in a:
                replace_s = eval(s)
                cmd = cmd.replace(s, replace_s);
        return cmd

    def __str__(self):
        return self.name

    def __repr__(self):
        return str(self)


FBE_drive_benchmarks = (
    Benchmark("PDO_8K_random_r",
              rdgen_perf_pdo_command + "-constant -align -priority 2 r 1 8 10",
              "Random read with 8K block size, 8 threads using pdo objects"),
    Benchmark("PDO_8K_cache_r",
              rdgen_perf_pdo_command + "-constant -align -priority 2 -start_lba 0 -min_lba 0 -max_lba 0x1000 r 1 8 10",
              "Read from disk internal cache. 8K block size, 8 threads using pdo objects"),
    Benchmark("PDO_band_r",
              rdgen_perf_pdo_command + "-constant -align -seq -priority 2 r 1 1 0x400",
              "Sequential large block read. Block size is 0.5MB using pdo objects"),
    Benchmark("PDO_band_w",
              rdgen_perf_pdo_command + "-constant -align -seq -priority 2 w 1 1 0x400",
              "Sequential large block write. Block size is 0.5MB using pdo objects"),
    Benchmark("PDO_seq_r_random_bs",
              rdgen_perf_pdo_command + "-seq -min_blocks 0x4 -priority 2 r 1 1 0x100",
              "Sequential read with random block size(2K ~ 128K) using pdo objects"),
    Benchmark("PVD_write_same_unmap",
              rdgen_perf_pvd_command + "-seq -constant -align -seq -start_lba 0 u 0 1 80",
              "Sequential write same with unmap (64K ~ 8M) using pvd objects"),
    Benchmark("PVD_8K_random_r",
              rdgen_perf_pvd_command + "-constant -align -priority 2 r 1 8 10",
              "Random read with 8K block size, 8 threads using pvd objects"),
    Benchmark("PVD_8K_random_w",
              rdgen_perf_pvd_command + "-constant -align -priority 2 w 1 8 10",
              "Random write with 8K block size, 8 threads using pvd objects"),
    Benchmark("PVD_band_w",
              rdgen_perf_pvd_command + "-constant -align -seq -priority 2 w 1 1 0x400",
              "Sequential large block write. Block size is 0.5MB using pvd objects"),
    Benchmark("PVD_8K_random_4x1_rw",
              rdgen_perf_pvd_command + "-constant -align -priority 2 r 1 4 10\n" +
              rdgen_perf_pvd_command + "-constant -align -priority 2 w 1 1 10",
              "Random R/W with 8K block size, 4 read threads and 1 write thread using pvd objects"),
    Benchmark("PVD_8K_random_1x4_rw",
              rdgen_perf_pvd_command + "-constant -align -priority 2 r 1 1 10\n" +
              rdgen_perf_pvd_command + "-constant -align -priority 2 w 1 4 10",
              "Random R/W with 8K block size, 1 read thread and 4 write thread using pvd objects"),
    Benchmark("PVD_512_lba_8K_random_1x4_rw",
              rdgen_perf_pvd_command + "-constant -align -priority 2 -min_lba 0 -max_lba find_lba_by_percentage(drive.pvd_capacity, 0.25, False) r 1 1 10\n" +
              rdgen_perf_pvd_command + "-constant -align -priority 2 -min_lba find_lba_by_percentage(drive.pvd_capacity, 0.5, True) " +
              "-max_lba find_lba_by_size(drive.pvd_capacity, 0.5, 107374182400) w 1 4 10",
              "Random R/W with 8K block size, 1 read thread and 4 write thread working inside lbas using pvd objects"),
    Benchmark("PVD_4k_lba_8K_random_1x4_rw",
              rdgen_perf_pvd_command + "-constant -align -priority 2 -min_lba 0 -max_lba find_lba_by_percentage(drive.pvd_capacity, 0.25, False) r 1 1 10\n" +
              rdgen_perf_pvd_command + "-constant -align -priority 2 -min_lba find_lba_by_percentage(drive.pvd_capacity, 0.5, True) " +
              "-max_lba find_lba_by_size(drive.pvd_capacity, 0.5, 107374182400) w 1 4 10",
              "Random R/W with 8K block size, 1 read thread and 4 write thread working inside lbas using pvd objects")
)


def fbe_drive_find_benchmark(benchmark_name):
    benchmark_name = benchmark_name.lower()
    if benchmark_name == "all":
        benchmark_name = "*"
    matched = []
    if benchmark_name == "write_same_unmap":
        data_sizes = [40,80,100,400,800,1000,2000,4000]
        num_threads = [1,2,4,8,10]
        for data_size in data_sizes:
            for num_thread in num_threads:
                new_bm = Benchmark("PVD_ws_unmap_XFER_"+str(data_size)+"_QD_"+str(num_thread),
                                   rdgen_perf_pvd_command + "-seq -constant -align u 0 "+str(num_thread)+" "+str(data_size),
                                   "Sequential write same with unmap (64K ~ 8M) using pvd objects")
                matched.append(new_bm)
        return matched

    for bm in FBE_drive_benchmarks:
        bm_name = bm.name.lower()
        if fnmatch(bm_name, benchmark_name):
            matched.append(bm)
    return matched


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

def get_precondition_bm():
    return Benchmark("PVD_8K_random_w",
                     rdgen_perf_pvd_command + "-constant -align -min_lba 0 -max_lba find_lba_by_percentage(drive.pvd_capacity, 0.25, False) w 1 4 10",
                     "Random w with 8K block size")



