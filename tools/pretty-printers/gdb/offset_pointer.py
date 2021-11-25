class OffsetPointer:
    def __init__(self, val):
        self.val = val
        self.offset = val['offset_']
        self.pointer_type = val.type.template_argument(0).pointer()

    def this_as_intptr_t(self):
        intptr_t = gdb.lookup_type("intptr_t")
        return self.val.address.reinterpret_cast(intptr_t)

    def as_raw_ptr(self):
        return (self.this_as_intptr_t() + self.offset).reinterpret_cast(self.pointer_type)

    def add(self, offset):
        return self.as_raw_ptr() + offset

    def __add__(self, o):
        return self.add(o)

    def dereference(self):
        return self.as_raw_ptr().dereference()

def is_offset_ptr(type):
    return str(type.strip_typedefs().unqualified()).startswith("cista::offset_ptr")