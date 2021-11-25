import re

class CistaString:
    def __init__(self, val):
        self.val = val
        self.GDB_CHAR_PTR = gdb.lookup_type("char*")

    def is_short(self):
        return self.val['s_']['is_short_']

    def data(self):
        return self.val['s_']['s_'] if self.is_short() else self.val['h_']['ptr_']

    def str(self):
        if self.is_short():
            return self.data().address.cast(self.GDB_CHAR_PTR)

        if is_offset_ptr(self.data().type):
            return OffsetPointer(self.data()).as_raw_ptr()

        return self.data()

class CistaStringPrinter:
    def __init__(self, val):
        self.val = CistaString(val)

    def to_string(self):
        return self.val.str()

def my_pp_func(val):
    regex = re.compile("cista::basic_string")
    if regex.match(str(val.type.strip_typedefs().unqualified())):
        return CistaStringPrinter(val)

gdb.pretty_printers.append(my_pp_func)

