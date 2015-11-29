#!/usr/bin/python
from fnmatch import fnmatch

class fbe_drive:
    def __init__(self, info):
        self.vendor = info.get("vendor") or ""
        self.capacity = info.get("capacity") or ""
        self.rpm = info.get("rpm") or ""
        self.product_id = info.get("product_id") or ""
        self.rev = info.get("product_rev") or ""
        self.sn = info.get("sn") or ""
        self.drive_type = info.get("drive_type") or ""
        self.object_id = int(info["object_id"], 0)
        self.pvd_obj_id = int(info["pvd_obj_id"], 0)
        self.pvd_capacity = info.get("pvd_capacity") or ""
        self.pos = info["pos"]
        self.is_zero = False
        self.zero = ""
        self.is_slow = False
        self.speed = { }
        self.tla = info.get("tla") or ""


    def is_match(self, info, flash_only):
        if flash_only and not self.is_flash():
            return False
        pos = "%u_%u_%u" % self.pos
        if fnmatch(pos, info):
            return True
        info = info.lower()
        if info == "all":
            return True
        return fnmatch(self.product_id, info)

    def get_type(self):
        return (self.vendor, self.product_id, self.capacity,
                self.rpm, self.drive_type)

    def is_flash(self):
        temp_drive_type = self.drive_type.lower()
        if "flash" in temp_drive_type:
            return True
        else:
            return False
    def get_tla (self):
        return self.tla

    def set_speed(self, bm, rate):
        self.speed[bm] = rate

    def get_speed(self, bm):
        return self.speed.get(bm)

    def get_speed_desc(self, bm):
        speed = self.get_speed(bm)
        if speed:
            return "%.2f" % speed
        else:
            return "None"

    def set_zero(self, zero):
        self.zero = zero
        if zero == "100%":
            self.is_zero = False
        else:
            self.is_zero = True

    def get_desc(self):
        s =  self.vendor + " " + self.product_id + ", Rev: " + self.rev + ", RPM: " + self.rpm \
             + ", capacity: " + self.capacity + ", type: " + self.drive_type \
             + ", SN: " + self.sn + ", obj: " + hex(self.object_id) \
             + ", pvd: " + hex(self.pvd_obj_id) + ", pvd capacity: " + self.pvd_capacity \
             + ", pos: %u_%u_%u" % self.pos
        return s

    def __str__(self):
        return self.get_desc()

    def __repr__(self):
        return "PDO(%s" % str(self)


class drive_list:
    def __init__(self, index, dtype):
        self.index = index
        self.dtype = dtype
        self.dl = []

    def get_drives(self):
        return self.dl

    def add(self, drive):
        self.dl.append(drive)

    def get_info(self):
        return (self.index, self.dtype, self.dl)

    def get_desc(self, count=None):
        desc = "Group " + str(self.index) + ": " + str(self.dtype)
        if count:
            desc += ", Drive count: " + str(count)
        return desc

    def __str__(self):
        return self.get_desc(len(self.dl))



class drive_pool:
    def __init__(self):
        self.oid_hash = {}
        self.type_hash = {}
        self.type_list = []
        self.drive_list = []

    def is_empty(self):
        if not self.drive_list:
            return True
        return False

    def new_list(self, dtype):
        index = len(self.type_list)
        dl = drive_list(index, dtype)
        self.type_list.append(dl)
        return dl

    def add_drive(self, drive):
        drive_type = drive.get_type()
        dl = self.type_hash.get(drive_type)
        if not dl:
            dl = self.new_list(drive_type)
            print "New type: " + str(drive_type)
            self.type_hash[drive_type] = dl
        dl.add(drive)

        if self.oid_hash.get(drive.object_id):
            print "Warn: duplicate object id:"
            print "    ", self.oid_hash.get(drive.object_id)
            print "    ", drive

        self.oid_hash[drive.object_id] = drive
        self.drive_list.append(drive)

    def find_drive_by_id(self, oid):
        drive = self.oid_hash.get(oid)
        if not drive:
            print "WARN: can't find pdo object 0x%x" % oid
        return drive

    def find_drive_by_pos(self, pos):
        for _, v in self.oid_hash.items():
            if v.pos == pos:
                return v
        print "WARN: can't find bes %s" % str(pos)
        return None

    def find_drive_by_pvd(self, pvd):
        for _, v in self.oid_hash.items():
            if v.pvd_obj_id == pvd:
                return v
        print "WARN: can't find pvd object %s" % str(pvd)
        return None

    def get_lists(self):
        return self.type_list

    def __str__(self):
        drives_count = sum([len(dl) for dl in self.type_list])
        desc = "Drive Types: %u, Drives Count: %u" % (len(self.type_list), drives_count)
        return desc
