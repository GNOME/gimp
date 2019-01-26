#   Gimp-Python - allows the writing of Gimp plugins in Python.
#   Copyright (C) 1997  James Henstridge <james@daa.com.au>
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <https://www.gnu.org/licenses/>.

# gimpshelf.py -- a simple module to help gimp modules written in Python
#                 store persistent data.
#
# Copyright (C) 1997, James Henstridge
#
# The gimp module provides a basic method for storing information that persists
# for a whole gimp session, but only allows for the storage of strings.  This
# is because other Python types usually have pointers to other Python objects,
# making it difficult to work out what to save.  This module gives an interface
# to the gimp module's primitive interface, which resembles the shelve module.

# use cPickle and cStringIO if available

try:
    import cPickle as pickle
except ImportError:
    import pickle

try:
    import cStringIO as StringIO
except ImportError:
    import StringIO

import gimp

import copy_reg

def _image_id(obj):
    return gimp._id2image, (obj.ID,)

def _drawable_id(obj):
    return gimp._id2drawable, (obj.ID,)

def _display_id(obj):
    return gimp._id2display, (obj.ID,)

def _vectors_id(obj):
    return gimp._id2vectors, (int(obj.ID),)

copy_reg.pickle(gimp.Image,      _image_id,    gimp._id2image)
copy_reg.pickle(gimp.Layer,      _drawable_id, gimp._id2drawable)
copy_reg.pickle(gimp.GroupLayer, _drawable_id, gimp._id2drawable)
copy_reg.pickle(gimp.Channel,    _drawable_id, gimp._id2drawable)
copy_reg.pickle(gimp.Display,    _display_id,  gimp._id2display)
copy_reg.pickle(gimp.Vectors,    _vectors_id,  gimp._id2vectors)

del copy_reg, _image_id, _drawable_id, _display_id, _vectors_id

class Gimpshelf:
    def has_key(self, key):
        try:
            s = gimp.get_data(key)
            return 1
        except gimp.error:
            return 0

    def __getitem__(self, key):
        try:
            s = gimp.get_data(key)
        except gimp.error:
            raise KeyError, key

        f = StringIO.StringIO(s)
        return pickle.Unpickler(f).load()

    def __setitem__(self, key, value):
        f = StringIO.StringIO()
        p = pickle.Pickler(f)
        p.dump(value)
        gimp.set_data(key, f.getvalue())

    def __delitem__(self, key):
        gimp.set_data(key, '')

shelf = Gimpshelf()
del Gimpshelf
