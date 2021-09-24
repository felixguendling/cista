import re

class CistaOffsetPointerPrinter:
    def __init__(self, val):
        self.val = OffsetPointer(val)

    def children(self):
        yield "*", self.val.dereference()

    def to_string(self):
        return str(self.val)

def my_pp_func(val):
    regex = re.compile("cista::offset_ptr")
    if regex.match(str(val.type.strip_typedefs())):
        return CistaOffsetPointerPrinter(val)

gdb.pretty_printers.append(my_pp_func)

