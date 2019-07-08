#   Gimp-Python - allows the writing of Gimp plugins in Python.
#   Copyright (C) 1997  James Henstridge <james@daa.com.au>
#
#    This program is free software: you can redistribute it and/or modify
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

#   plugin.py -- helper for writing gimp plugins
#     Copyright (C) 1997, James Henstridge.
#
# This is a small wrapper that makes plugins look like an object class that
# you can derive to create your plugin.  With this wrapper, you are pretty
# much responsible for doing everything (checking run_mode, gui, etc).  If
# you want to write a quick plugin, you probably want the gimpfu module.
#
# A plugin using this module would look something like this:
#
#   import gimp, gimpplugin
#
#   pdb = gimp.pdb
#
#   class myplugin(gimpplugin.plugin):
#       def query(self):
#           gimp.install_procedure("plug_in_mine", ...)
#
#       def plug_in_mine(self, par1, par2, par3,...):
#           do_something()
#
#   if __name__ == '__main__':
#       myplugin().start()

import gimp

class plugin:
    def start(self):
        # only pass the init()/quit() member functions to gimp.main() if the
        # plug-in overrides them, to avoid the default NOP versions from being
        # called unnecessarily.  in particular, this avoids plug-ins that don't
        # implement init() from being registered as having an init function,
        # causing them to be run at each startup.
        def get_func(name):
            if getattr(self.__class__, name) != getattr(plugin, name):
                return getattr(self, name)
            else:
                return None

        gimp.main(get_func("init"),
                  get_func("quit"),
                  self.query,
                  self._run)

    def init(self):
        pass

    def quit(self):
        pass

    def query(self):
        pass

    def _run(self, name, params):
        import sys
        if "gimpui" in sys.modules.keys():
            sys.modules["gimpui"].gimp_ui_init ()

        if hasattr(self, name):
            return apply(getattr(self, name), params)
        else:
            raise AttributeError, name

if __name__ == '__main__':
    plugin().start()
