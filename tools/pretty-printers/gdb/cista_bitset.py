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
        s = ""
        for word_index in range(self.n_words):
            current = self.values[word_index]
            index = -1
            while current:
                index += 1
                will_yield = current % 2
                current /= 2
                if will_yield:
                    s += '1'
                else:
                    s += '0'
        return s

    def str(self):
        return self.to_string()

    def _byte_it(self, value):
        index = -1
        while value:
            index += 1
            will_yield = value % 2
            value /= 2
            if will_yield:
                yield index

    def children(self):
        for word_index in range(self.n_words):
            current = self.values[word_index]
            if current:
                for n in self._byte_it(current):
                    yield ("[%d]" % (word_index * self.bits_per_word + n)), 1


def cista_bitset(val):
    regex = re.compile("cista::bitset")
    if regex.match(str(val.type.strip_typedefs())):
        return CistaBitsetPrinter(val)


gdb.pretty_printers.append(cista_bitset)
