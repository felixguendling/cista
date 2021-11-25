import re

class CistaTuplePrinter:
    def __init__(self, val):
        self.val = val

    def children(self):
        yield '[0]', self.val['head_']

        field_count = 1
        local_val = self.val[self.val.type.fields()[0]]
        while True:
            yield '[' + str(field_count) + ']', local_val['head_']
            field_count += 1
            if (len(local_val.type.fields()) == 1):
                return
            else:
                local_val = local_val[local_val.type.fields()[0]]

    def to_string(self):
        return str(self.val)

def my_pp_func(val):
    regex = re.compile("cista::tuple")
    if regex.match(str(val.type.strip_typedefs().unqualified())):
        return CistaTuplePrinter(val)

gdb.pretty_printers.append(my_pp_func)

