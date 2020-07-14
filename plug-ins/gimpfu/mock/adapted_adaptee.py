

# testing using a non-mocked adapter
from gimpfu.adapter import Adapter
# from ~/vaggaGimp/gimpfu/adapter import Adapter


class AdaptedAdaptee( Adapter):
    '''
    Class that wraps (adapts) class Adaptee
    E.G. GimpfuImage
    '''
    def __init__(self, adaptee):
        # Adapter remembers the adaptee instance
        super().__init__(adaptee)


    '''
    Define properties of AdaptedAdaptee
    that map to properties of Adaptee.
    All are instance (not class) properties, since implemented
    using the _adaptee attribute of Adaptee instances.

    !!! The names must match those used in Adaptee.
    '''
    DynamicWriteableAdaptedProperties = ('othernameRW', 'instance_nameRW' )
    DynamicReadOnlyAdaptedProperties = ('othernameRO', )


    # A property defined statically
    # i.e. using code explicit about adaptee name

    @property
    def filename(self):
        print("Adapted property accessed")
        return self._adaptee.get_filename()
    @filename.setter
    def filename(self, dummy):
        # TODO:
        raise RuntimeError("not implemented")

    # A callable that Adapted provides
    def adapted_callable():
         pass
