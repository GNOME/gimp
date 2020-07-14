
class GimpfuFormalParam():
    def __init__(self, pf_type, label, desc, default_value, extras = [] ):
        self.PF_TYPE= pf_type
        self.LABEL= label
        self.DESC= desc
        self.DEFAULT_VALUE= default_value
        self.EXTRAS = extras
        # EXTRAS defaults to empty list or ???  [None]  Python 3.7
