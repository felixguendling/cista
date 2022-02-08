import re

def get_offset(gdb_type, idx):
    current_offset = 0

    for i in range(0, idx):
        current_offset += gdb_type.template_argument(i).sizeof

        next_type = gdb_type.template_argument(i + 1)
        align_of = next_type.alignof

        misalign = current_offset % align_of
        if misalign != 0:
            current_offset += (align_of - misalign) % align_of

    return current_offset

class CistaTuplePrinter:
    def __init__(self, val):
        self.val = val

    def this_as_char_ptr(self):
        char_ptr = gdb.lookup_type("char*")
        return self.val.address.reinterpret_cast(char_ptr)

    def children(self):
        current_idx = 0

        while True:
            try:
                template_type = self.val.type.template_argument(current_idx)
            except:
                return

            member_address = self.this_as_char_ptr() + get_offset(self.val.type, current_idx)
            casted_member = member_address.reinterpret_cast(template_type.pointer()).dereference()

            yield '[' + str(current_idx) + ']', casted_member

            current_idx += 1

    def to_string(self):
        return str(self.val)

def my_pp_func(val):
    regex = re.compile("cista::tuple")
    if regex.match(str(val.type.strip_typedefs().unqualified())):
        return CistaTuplePrinter(val)

gdb.pretty_printers.append(my_pp_func)

