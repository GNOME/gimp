

import sys

# Insure all warnings (deprecation and user) will be printed
if not sys.warnoptions:
    import warnings
    warnings.simplefilter("default") # Change the filter in this process




class Deprecation():
    '''
    Knows how to tell Author about deprecations.

    Many occur at registration time.
    FUTURE save them, and print them at run-time also

    FUTURE set procedure name in state, to be prepended to messages
    '''

    log = []

    @classmethod
    def say(cls, message):
        ''' Tell user about a deprecation '''

        print("************")

        Deprecation.log.append(message)

        ''' wrapper of warnings.warn() that fixpoints the parameters. '''
        # stacklevel=2 means print two lines, including caller's info
        warnings.warn(message, DeprecationWarning, stacklevel=2)

    @classmethod
    def summarize(cls):
        if not Deprecation.log:
            result = False
        else:
            print("=================================")
            print("GimpFu's summary of deprecations.")
            print("=================================")
            for line in Deprecation.log:
                print(line)
            print("")
            result = True
        return result
