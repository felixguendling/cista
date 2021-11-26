import re


# Based on
# libcxx/trunk/utils/gdb/libcxx/printers.py
class CistaBitsetPrinter(object):
    """Print a cista::bitset."""

    def __init__(self, val):
        self.val = val
        self.n_words = int(self.val["num_blocks"])
        self.bits_per_word = int(self.val["bits_per_block"])
        if self.n_words == 1:
            self.values = [int(self.val["blocks_"])]
        else:
            self.values = [int(self.val["blocks_"][index])
                           for index in range(self.n_words)]

    def to_string(self):
        return str(self.val)

    def _byte_it(self, value):
        index = -1
        while value:
            index += 1
            will_yield = value % 2
            value /= 2
            if will_yield:
                yield index

    def _list_it(self):
        for word_index in range(self.n_words):
            current = self.values[word_index]
            if current:
                for n in self._byte_it(current):
                    yield ("[%d]" % (word_index * self.bits_per_word + n), 1)

    def __iter__(self):
        return self._list_it()

    def children(self):
        return self


def my_pp_func(val):
    regex = re.compile("cista::bitset")
    if regex.match(str(val.type.strip_typedefs())):
        return CistaBitsetPrinter(val)


gdb.pretty_printers.append(my_pp_func)
