#   Gimp-Python - allows the writing of Gimp plugins in Python.
#   Copyright (C) 2005  Manish Singh  <yosh@gimp.org>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

# gimpenums.py -- constants for use with the gimp module
#
# this file pulls in constants that are useful for use in
# gimp plugins.  Just add 'from gimpenums import *' to the top
# of the script

from _gimpenums import *

# This is from pygtk/gtk/__init__.py
# Copyright (C) 1998-2003  James Henstridge

class _DeprecatedConstant:
    def __init__(self, value, name, suggestion):
        self._v = value
        self._name = name
        self._suggestion = suggestion

    def _deprecated(self, value):
        import warnings
        message = '%s is deprecated, use %s instead' % (self._name,
                                                        self._suggestion)
        warnings.warn(message, DeprecationWarning, 3)
        return value

    __nonzero__ = lambda self: self._deprecated(self._v == True)
    __int__     = lambda self: self._deprecated(int(self._v))
    __str__     = lambda self: self._deprecated(str(self._v))
    __repr__    = lambda self: self._deprecated(repr(self._v))
    __cmp__     = lambda self, other: self._deprecated(cmp(self._v, other))

TRUE = _DeprecatedConstant(True, 'gimpenums.TRUE', 'True')
FALSE = _DeprecatedConstant(False, 'gimpenums.FALSE', 'False')

del _DeprecatedConstant
