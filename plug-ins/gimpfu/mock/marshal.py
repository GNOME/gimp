


'''
Knows how to create AdaptedAdaptees from AdaptedAdaptee
and vice versa

TODO ABC
'''
class Marshal():

    @classmethod
    def wrap(cls, adaptee):
        ''' Return Adapter for adaptee '''
        # more generally, dispatch on type of adaptee

        wrapper = Adapted(adaptee)
        return wrapper
