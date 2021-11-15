import re

class CistaHashStoragePrinter:
    def __init__(self, val):
        self.val = val
        self.entries = self.val['entries_']
        self.ctrl = self.val['ctrl_']

        regex = re.compile("cista::offset_ptr")
        if regex.match(str(self.entries.type.strip_typedefs())):
            self.entries = OffsetPointer(self.entries)
            self.ctrl = OffsetPointer(self.ctrl)

    def children(self):
        current_idx = 0
        for i in range(self.val['capacity_']):
            if (self.ctrl + i).dereference() & int('0b10000000', 2) == 0:
                yield '[' + str(current_idx) + ']', (self.entries + i).dereference()
                current_idx += 1

    def to_string(self):
        return str(self.val['size_'])

def my_pp_func(val):
    type_str = str(val.type.strip_typedefs())
    if type_str.startswith("cista::hash_storage") and not type_str.endswith("::ctrl_t"):
        return CistaHashStoragePrinter(val)

gdb.pretty_printers.append(my_pp_func)

