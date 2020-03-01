
from collections.abc import Mapping


from message.deprecation import Deprecation


'''
Knows backward compatibility for Gimp changes.
Gimp versions infrequently:
    rename functions  (most often)
    condense many functions into one
    migrate functions from one class to another

And other hacky workarounds of limitations in Gimp
???
'''



# See SO How would I implement a dict with Abstract Base Classes in Python?

class GimpFuMap(Mapping):
    '''
    A dictionary that returns key itself,
    on an access that would otherwise engender KeyError.
    Read-only, since Mapping is.
    Dictionary syntax:  new_name = gimpFuMap[name]

    Subclasses may do additional alterations to name.
    '''

    def __init__(self, map):
        # replace wrapped dict
        self.__dict__ = map

    '''
    Implement abstract methods of Mapping
    '''
    def __getitem__(self, key):
        '''
        CRUX:
        If the key is not in the wrapped dictionary, return unmapped key
        If key is in dictionary, return mapped and tell author of deprecated.
        '''
        # TODO implement with except KeyError: would be faster?
        if key in self.__dict__.keys():
            Deprecation.say(f"Deprecated name: {key}")
            return self.__dict__[key]
        else:
            return key



    def __iter__(self):
        raise NotImplementedError("Compat iterator.")
    def __len__(self):
        raise NotImplementedError("Compat len()")

    '''
    Not required by ABC.
    '''
    def __repr__(self):
        return 'GimpFuMap'
        """
        TODO use this snippet
        '''echoes class, id, & reproducible representation in the REPL'''
        return '{}, D({})'.format(super(D, self).__repr__(),
                                  self.__dict__)
        """


class GimpFuPDBMap(GimpFuMap):
    '''
    Specialize GimpFuMap for the Gimp PDB.

    Alter all mapped names: transliterate
    The names in the PDB use hyphens, which Python does not allow in symbols.
    Super() will then map deprecated names.
    '''
    def __getitem__(self, key):
        hyphenized_key = key.replace( '_' , '-')
        # !!! The map must use hyphenated strings
        return super().__getitem__(hyphenized_key)




'''
Maps of renamings

This excludes renamings that were accompanied by signature changes.
For those, we adapt the method elsewhere.
TODO can we adapt signature also
'''

# see Gimp commit  233ac80d "script-fu: port all scripts to the new gimp-drawable-edit functions "
# 'gimp-threshold' : 'gimp-drawable-threshold',  needs param2 channel, and values in range [0.0, 1.0]

# !!! both left and right sides hyphenated, not underbar
pdb_renaming = {
    "gimp-edit-fill"        : "gimp-drawable-edit-fill",
    "gimp-edit-clear"        : "gimp-drawable-edit-clear",
    "gimp-histogram"        : "gimp-drawable-histogram",
    # Rest are guesses, not tested
    "gimp-edit-bucket-fill" : "gimp-drawable-edit-bucket-fill", # TODO # (fill_type, x, y):
    "gimp-ellipse-select"   : "gimp-image-select-ellipse",

}
"""
Tested not renamed.
gimp-selection-none


TODO
in scheme
(gimp-edit-blend bl-mask BLEND-FG-BG-RGB LAYER-MODE-NORMAL
                                     GRADIENT-LINEAR 100 20 REPEAT-NONE FALSE
                                     FALSE 0 0 TRUE
                                     (+ bl-x-off bl-width) 0 bl-x-off 0)
=>
		    (gimp-context-set-gradient-fg-bg-rgb)
                    (gimp-drawable_edit-gradient-fill bl-mask
						      GRADIENT-LINEAR 20
						      FALSE 0 0
						      TRUE
						      (+ bl-x-off bl-width) 0
						      bl-x-off 0)
"""



gimp_renaming = {
    # TODO which Gimp commit changed this?  Get the rest of them.
    "set_foreground" : "context_set_foreground",
    # signature change also: "set_background" : "context_set_background",
}

# TODO maybe these become undo_renaming i.e. calls to Gimp.Undo.disable?
image_renaming = {
    'disable_undo' : "undo_disable",
    'enable_undo' : "undo_enable",
}
layer_renaming = {
}


'''
One map for each Gimp object that GimpFu wraps
'''
pdb_name_map = GimpFuPDBMap(pdb_renaming);
gimp_name_map = GimpFuMap(gimp_renaming);
image_name_map = GimpFuMap(image_renaming);
layer_name_map = GimpFuMap(layer_renaming);
display_name_map = GimpFuMap( {} ); # no renaming anticipated
# todo channel, etc

# maps string to GimpFuMap
# TODO other adapted objects e.g. Channel?
map_map = {
    'Image' : image_name_map,
    'Layer' : layer_name_map,
    'Display' : display_name_map,
}

def get_name_map_for_adaptee_class(class_name):
    ''' Return GimpFuMap for class_name. '''
    '''
    Throws KeyError if class_name is not adapted.
    When you develop an adapter, add class_name to map_map
    '''
    return map_map[class_name]
