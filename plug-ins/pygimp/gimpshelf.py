#   Gimp-Python - allows the writing of Gimp plugins in Python.
#   Copyright (C) 1997  James Henstridge <james@daa.com.au>
#
#    This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

# gimshelf.py -- a simple module to help gimp modules written in Python
#                store persistent data.
#
# Copyright (C) 1997, James Henstridge
#
# The gimp module provides a basic method for storing information that persists
# for a whole gimp session, but only allows for the storage of strings.  This
# is because other Python types usually have pointers to other Python objects,
# making it dificult to work out what to save.  This module gives an interface
# to the gimp module's primitive interface, which resembles the shelve module.

try:
	# use cPickle instead of pickle if it is available.
	import cPickle
	pickle = cPickle
	del cPickle
except ImportError:
	import pickle
import StringIO
import gimp

try:
	# this will fail with python 1.4.  All we lose is that the values
	# for a plugin which takes extra image/drawables/etc will not be
	# saved between invocations.
	import copy_reg
	def _image_id(obj):
		return gimp._id2image, (obj.ID,)
	def _drawable_id(obj):
		return gimp._id2drawable, (obj.ID,)
	def _display_id(obj):
		return gimp._id2display, int(obj)
	copy_reg.pickle(gimp.ImageType, _image_id, gimp._id2image)
	copy_reg.pickle(gimp.LayerType, _drawable_id, gimp._id2drawable)
	copy_reg.pickle(gimp.ChannelType, _drawable_id, gimp._id2drawable)
	copy_reg.pickle(gimp.DisplayType, _display_id, gimp._id2display)
	del copy_reg, _image_id, _drawable_id, _display_id
except ImportError:
	pass

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

