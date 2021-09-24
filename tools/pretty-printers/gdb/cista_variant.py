import re

class CistaVariantPrinter:
  def __init__(self, val):
    self.val = val

  def get_actual_type(self):
      return self.val.type.template_argument(self.val['idx_'])

  def children(self):
      for field in self.val.type.fields():
          if (field.name == "storage_"): 
              yield field.name, self.val[field.name].cast(self.get_actual_type()) 
          else: 
              yield field.name, self.val[field.name].cast(field.type)

  def to_string(self):
      return str(self.val)


def my_pp_func(val):
    regex = re.compile("cista::variant")
    if regex.match(str(val.type.strip_typedefs())):
        return CistaVariantPrinter(val)

gdb.pretty_printers.append(my_pp_func)

