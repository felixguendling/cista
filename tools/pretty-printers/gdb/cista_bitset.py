import re


class CistaBitsetPrinter(object):
    """Print a cista::bitset."""

    def __init__(self, val):
        self.val = val
        self.n_words = int(self.val["num_blocks"])
        self.bits_per_word = int(self.val["bits_per_block"])
        if self.n_words == 1:
            self.values = [int(self.val["blocks_"])]
        else:
            self.values = [int(self.val["blocks_"]["el_"][index])
                           for index in range(self.n_words)]

    def to_string(self):
        s = ""
        for word_index in range(self.n_words):
            s += format(self.values[word_index], '064b')
        return s

    def str(self):
        return self.to_string()


def cista_bitset(val):
    regex = re.compile("cista::bitset")
    if regex.match(str(val.type.strip_typedefs().unqualified())):
        return CistaBitsetPrinter(val)


gdb.pretty_printers.append(cista_bitset)
