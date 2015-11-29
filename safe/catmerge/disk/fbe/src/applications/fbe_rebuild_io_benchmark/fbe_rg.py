#!/usr/bin/python
from fnmatch import fnmatch

class fbe_rg:
    def __init__(self, object_id):
        self.object_id = int(object_id, 10)
        self.rebuild_checkpoint = ""
        self.pos = ""
        self.checkpoint_time = ""
        self.lun_object_list = []
        self.results = {}
        self.capacity = ""
        self.raid_type = ""
        self.width = ""
        self.swap_disk = ""

    def update(self, info):
        self.pos = info["pos"]
        self.rebuild_checkpoint = info["rebuild_checkpoint"]
        self.checkpoint_time = info["checkpoint_time"]
        self.lun_list = info["lun_list"]
        self.disks = info["disks"]
        self.capacity = info["capacity"]
        self.raid_type = info["raid_type"]
        self.width = info["width"]


    def get_data_disks(self):
        if self.raid_type == "FBE_RAID_GROUP_TYPE_RAID5" or self.raid_type == "FBE_RAID_GROUP_TYPE_RAID3":
            return self.width - 1
        if self.raid_type == "FBE_RAID_GROUP_TYPE_RAID6":
            return self.width - 2
        if (self.raid_type == "FBE_RAID_GROUP_TYPE_RAID1") or (self.raid_type == "FBE_RAID_GROUP_TYPE_RAID10") or (self.raid_type == "FBE_RAID_GROUP_TYPE_MIRROR_UNDER_STRIPER"):  
            return self.width/2
    def is_rebuild_started(self):
        return (self.rebuild_checkpoint != "0x0") and (self.rebuild_checkpoint != "") and (self.rebuild_checkpoint != "0xFFFFFFFFFFFFFFFF")
    def get_rb_percent(self):
        if self.rebuild_checkpoint == "0xFFFFFFFFFFFFFFFF" or self.rebuild_checkpoint == "":
            return 100
        else:
            disk_capacity = self.capacity / self.get_data_disks()
            chkpt = int(self.rebuild_checkpoint, 16)
            percentage = (chkpt * 100) / disk_capacity
            return percentage

    def is_match(self, info):
        pos = "%u_%u_%u" % self.pos
        if fnmatch(pos, info):
            return True
        info = info.lower()
        if info == "all":
            return True
        return fnmatch(self.object_id, info)

    def get_desc(self):
        s =  "Raid group obj: " + hex(self.object_id) \
             + ", pos: %s" % self.pos
        return s

    def __str__(self):
        return self.get_desc()

    def __repr__(self):
        return "RG(%s)" % str(self)
