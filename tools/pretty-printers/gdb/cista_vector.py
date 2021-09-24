import re

class CistaRawVectorPrinter:
    def __init__(self, val):
        self.val = val

    def children(self):
        curr_idx = 0
        while curr_idx < self.val['used_size_']:
            yield '[' + str(curr_idx) + ']', (self.val['el_'] + curr_idx).dereference()
            curr_idx += 1

    def to_string(self):
        return str(self.val)

class CistaOffsetVectorPrinter:
    def __init__(self, val):
        self.val = val

    def children(self):
        curr_idx = 0
        el = OffsetPointer(self.val['el_'])
        while curr_idx < self.val['used_size_']:
            yield '[' + str(curr_idx) + ']', el.add(curr_idx).dereference()
            curr_idx += 1

    def to_string(self):
        return str(self.val)

def my_pp_func(val):
    regex = re.compile("cista::basic_vector")
    stripped = val.type.strip_typedefs()

    if regex.match(str(stripped)):
        offset_regex = re.compile("cista::offset_ptr")
        if offset_regex.match(str(stripped.template_argument(1))):
            return CistaOffsetVectorPrinter(val)
        else:
            return CistaRawVectorPrinter(val)

gdb.pretty_printers.append(my_pp_func)

