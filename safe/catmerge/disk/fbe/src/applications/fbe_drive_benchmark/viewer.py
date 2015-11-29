#!/usr/bin/python

import sys
from utils import time_to_filename, time_to_string

viewer_item_div = "===============================================================\n"

class BaseViewer:
    def __init__(self, result):
        self.name = "base"
        self.cfg = result["config"]
        self.start_time, self.end_time = result["test_time"]
        self.test_drive_pool = result["test_drives"]
        self.test_benchmark = result["benchmarks"]


    def get_log_file_name(self):
        return "result_" + self.name + "_" + time_to_filename(self.end_time) + ".txt"


    def log_config(self, f):
        desc = str(self.cfg)
        desc += "Start Time: " + time_to_string(self.start_time) + "\n"
        desc += "End Time:   " + time_to_string(self.end_time) + "\n"
        f.write(desc)


    def log_benchmark_info(self, f):
        f.write("BENCHMARKS(%u):\n" % len(self.test_benchmark))
        for bm in self.test_benchmark:
            f.write("    " + str(bm) + "\n")


    def log_groups(self, f):
        lists = self.test_drive_pool.get_lists()
        f.write("GROUPS(%u):\n" % len(lists))
        for dl in lists:
            desc = str(dl)
            f.write("    " + desc + "\n")


    def sort_drive_list(self, drive_list, bm):
        drives = sorted(drive_list, key=lambda drive: drive.get_speed(bm))
        drives = sorted(drives, key=lambda drive: drive.rev)
        return drives


    def log_benchmark_by_drive_type(self, bm, drive_type, f):
        desc = str(drive_type)
        f.write(desc + "\n")
        drives = drive_type.get_drives()
        drives = self.sort_drive_list(drives, bm)
        for drive in drives:
            desc = str(drive)
            speed_desc = drive.get_speed_desc(bm)
            desc += ", Rate: " + speed_desc
            f.write("    " + desc + "\n")
        f.write("\n")


    def log_benchmark(self, bm, f):
        div = "***************************************************************************************************\n"
        f.write(div)
        f.write(bm.get_desc() + "\n")
        f.write(div)
        for drive_type in self.test_drive_pool.get_lists():
            self.log_benchmark_by_drive_type(bm, drive_type, f)

        f.write("\n\n")


    def log_result(self, f=sys.stdout):
        div = "================================================================================================================\n"
        self.log_config(f)
        f.write("\n\n")

        f.write(div)
        self.log_benchmark_info(f)
        f.write("\n\n")

        f.write(div)
        self.log_groups(f)
        f.write("\n\n")

        for bm in self.test_benchmark:
            self.log_benchmark(bm, f)


    def log(self):
        file_name = self.get_log_file_name()
        with open(file_name, "w") as f:
            self.log_result(f)


class SortViewer(BaseViewer):
    def __init__(self, result):
        BaseViewer.__init__(self, result)
        self.name = "sort"


    def log_benchmark(self, bm, f):
        div = "***************************************************************************************************\n"
        f.write(div)
        f.write(bm.get_desc() + "\n")
        f.write(div)
        drives = []
        for drive_type in self.test_drive_pool.get_lists():
            drives.extend(drive_type.get_drives())
        drives = sorted(drives, key=lambda drive: drive.get_speed(bm))
        for drive in drives:
            desc = str(drive)
            speed_desc = drive.get_speed_desc(bm)
            desc += ", Rate: " + speed_desc
            f.write("    " + desc + "\n")
        f.write("\n\n")

class CSVViewer(BaseViewer):
    def __init__(self, result):
        BaseViewer.__init__(self, result)
        self.name = "csv"

    def get_log_file_name(self):
        return "result_" + self.name + "_" + time_to_filename(self.end_time) + ".csv"

    def log_result(self, f=sys.stdout):
        drives = []
        f.write("tla,location,")
        for bm in self.test_benchmark:
            f.write(bm.get_name()+",")
        f.write("\n")

        for drive_type in self.test_drive_pool.get_lists():
            drives.extend(drive_type.get_drives())
        drives = sorted(drives, key=lambda drive: drive.get_tla())
        for drive in drives:
            f.write(drive.get_tla()+",")
            f.write(drive.pos+",")
            for bm in self.test_benchmark:
                speed_desc = drive.get_speed_desc(bm)
            f.write(desc + ",")
            f.write("\n")

fbe_viewer_list = (
    BaseViewer,
    SortViewer,
    CSVViewer,
)


def fbe_drive_benchmark_get_viewers():
    return fbe_viewer_list
