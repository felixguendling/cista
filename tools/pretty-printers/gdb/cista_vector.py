import re
import gdb.xmethod

def is_cista_vector(gdb_type):
    return str(gdb_type.strip_typedefs()).startswith("cista::basic_vector")

def is_raw_vector(gdb_type):
    return not str(gdb_type.strip_typedefs().template_argument(1)).startswith("cista::offset_ptr")

class CistaVector:
    def __init__(self, val):
        self.val = val
        self.size = val['used_size_']
        self.el = val['el_'] if is_raw_vector(val.type) else OffsetPointer(val['el_'])

    def __len__(self):
        return self.size

    def __getitem__(self, idx):
        return (self.el + idx).dereference()

    def at(self, idx):
        if (self.size < idx):
            print("Accessing vector out of bounds")
            return None

        return self[idx]

class CistaVectorPrinter:
    def __init__(self, val):
        self.val = CistaVector(val)

    def children(self):
        for idx in range(len(self.val)):
            yield '[' + str(idx) + ']', self.val[idx]

    def to_string(self):
        return str(self.val)

def my_pp_func(val):
    if not is_cista_vector(val.type):
        return

    return CistaVectorPrinter(val)

### XMethod cista::vector::at

class CistaVectorWorker_at(gdb.xmethod.XMethodWorker):
    def get_arg_types(self):
        return gdb.lookup_type('int')

    def get_result_type(self, obj):
        return obj.type.strip_typedefs().template_argument(0)

    def __call__(self, this, idx):
        vec = CistaVector(this.dereference())
        return vec.at(idx)

class CistaVector_at(gdb.xmethod.XMethod):
    def __init__(self):
        gdb.xmethod.XMethod.__init__(self, 'at')

    def get_worker(self, method_name):
        if method_name == 'at':
            return CistaVectorWorker_at()

### XMethod cista::vector::operator[]

class CistaVectorWorker_operator_brackets(gdb.xmethod.XMethodWorker):
    def get_arg_types(self):
        return gdb.lookup_type('int')

    def get_result_type(self, obj):
        return obj.type.strip_typedefs().template_argument(0)

    def __call__(self, this, idx):
        vec = CistaVector(this.dereference())
        return vec[idx]

class CistaVector_operator_brackets(gdb.xmethod.XMethod):
    def __init__(self):
        gdb.xmethod.XMethod.__init__(self, 'operator[]')

    def get_worker(self, method_name):
        if method_name == 'operator[]':
            return CistaVectorWorker_operator_brackets()

class CistaVectorMatcher(gdb.xmethod.XMethodMatcher):
    def __init__(self):
        gdb.xmethod.XMethodMatcher.__init__(self, 'CistaVectorMatcher')
        # List of methods 'managed' by this matcher
        self.methods = [CistaVector_at(), CistaVector_operator_brackets()]

    def match(self, class_type, method_name):
        if not is_cista_vector(class_type):
            return None

        workers = []
        for method in self.methods:
            if method.enabled:
                worker = method.get_worker(method_name)
                if worker:
                    workers.append(worker)

        return workers

gdb.pretty_printers.append(my_pp_func)
gdb.xmethod.register_xmethod_matcher(None, CistaVectorMatcher())
