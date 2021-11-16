import re
import gdb.xmethod

def is_cista_hash_storage(gdb_type):
    type_str = str(gdb_type.strip_typedefs())
    return type_str.startswith("cista::hash_storage") and not type_str.endswith("::ctrl_t")

class CistaHashStorage:
    def __init__(self, val):
        self.val = val
        self.entries = self.val['entries_']
        self.capacity = self.val['capacity_']
        self.ctrl = self.val['ctrl_']

        regex = re.compile("cista::offset_ptr")
        if regex.match(str(self.entries.type.strip_typedefs())):
            self.entries = OffsetPointer(self.entries)
            self.ctrl = OffsetPointer(self.ctrl)

    def is_valid_entry(self, idx):
        return (self.ctrl + idx).dereference() & int('0b10000000', 2) == 0

    def __len__(self):
        return int(self.val['size_'])

    def __getitem__(self, key):
        for i in range(self.val['capacity_']):
            if not self.is_valid_entry(i):
                continue

            entry = (self.entries + i).dereference()
            if str(entry['first']) == str(key):
                return entry['second']

class CistaHashStoragePrinter:
    def __init__(self, val):
        self.val = CistaHashStorage(val)

    def children(self):
        current_idx = 0
        for i in range(self.val.capacity):
            if self.val.is_valid_entry(i):
                yield '[' + str(current_idx) + ']', (self.val.entries + i).dereference()
                current_idx += 1

    def to_string(self):
        return str(self.val)

def my_pp_func(val):
    if is_cista_hash_storage(val.type):
        return CistaHashStoragePrinter(val)

### XMethod cista::vector::operator[]

class CistaHashStorageWorker_operator_brackets(gdb.xmethod.XMethodWorker):
    def __init__(self, class_type):
        self.class_type = class_type

    def get_arg_types(self):
        return self.class_type.template_argument(0)

    def get_result_type(self, obj):
        return obj.type.strip_typedefs().template_argument(1)

    def __call__(self, this, key):
        hash_storage = CistaHashStorage(this.dereference())
        return hash_storage[key]

class CistaHashStorageWorker_operator_brackets_char_ptr(gdb.xmethod.XMethodWorker):
    def __init__(self, class_type):
        self.class_type = class_type

    def get_arg_types(self):
        return gdb.lookup_type('const char* const')

    def get_result_type(self, obj):
        return obj.type.strip_typedefs().template_argument(1)

    def __call__(self, this, key):
        hash_storage = CistaHashStorage(this.dereference())
        key = key.cast(gdb.lookup_type("const char* const"))
        return hash_storage[str(key).split()[1]]

class CistaHashStorage_operator_brackets(gdb.xmethod.XMethod):
    def __init__(self):
        gdb.xmethod.XMethod.__init__(self, 'operator[]')

    def get_worker(self, method_name, class_type):
        worker = []
        if method_name == 'operator[]':
            worker.append(CistaHashStorageWorker_operator_brackets(class_type))

        temp_arg = class_type.template_argument(0).name
        is_string = temp_arg.startswith("std::__cxx11::basic_string") \
                    or temp_arg.startswith("cista::basic_string")
        if method_name == 'operator[]' and is_string:
            worker.append(CistaHashStorageWorker_operator_brackets_char_ptr(class_type))

        return worker

### XMethod cista::vector::size

class CistaHashStorageWorker_size(gdb.xmethod.XMethodWorker):
    def get_arg_types(self):
        return None

    def get_result_type(self):
        return gdb.lookup_type('unsigned long int')

    def __call__(self, this):
        hash_storage = CistaHashStorage(this.dereference())
        return len(hash_storage)

class CistaHashStorage_size(gdb.xmethod.XMethod):
    def __init__(self):
        gdb.xmethod.XMethod.__init__(self, 'size')

    def get_worker(self, method_name, _):
        if method_name == 'size':
            return [CistaHashStorageWorker_size()]

class CistaHashStorageMatcher(gdb.xmethod.XMethodMatcher):
    def __init__(self):
        gdb.xmethod.XMethodMatcher.__init__(self, 'CistaHashStorageMatcher')
        # List of methods 'managed' by this matcher
        self.methods = [CistaHashStorage_operator_brackets(), CistaHashStorage_size()]

    def match(self, class_type, method_name):
        if not is_cista_hash_storage(class_type):
            return None

        workers = []
        for method in self.methods:
            if method.enabled:
                worker = method.get_worker(method_name, class_type.template_argument(0))
                if worker:
                    workers.extend(worker)

        return workers

gdb.pretty_printers.append(my_pp_func)
gdb.xmethod.register_xmethod_matcher(None, CistaHashStorageMatcher())
