#!/usr/bin/env python

"""
module-dependencies.py -- GIMP library and core module dependency constructor
Copyright (C) 2010  Martin Nordholts <martinn@src.gnome.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.



This program uses graphviz (you need PyGraphViz) to construct a graph
with dependencies between GIMP library and core modules. Run it from
the source root. Note that you'll either need the very latest
PyGraphViz binding or use this hack in agraph.py:

--- agraph.py.orig	2010-01-04 16:07:46.000000000 +0100
+++ agraph.py	2010-01-04 16:13:54.000000000 +0100
@@ -1154,7 +1154,8 @@ class AGraph(object):
             raise IOError("".join(errors))
 
         if len(errors)>0:
-            raise IOError("".join(errors))
+            # Workaround exception throwing due to warning about cycles
+            pass
         return "".join(data)
 
 
"""


from __future__ import with_statement
from sets import Set
import os, re, pygraphviz



# First make a sanity check
if not os.path.exists("app") or not os.path.exists("libgimp"):
    print("Must be run in source root!")
    exit(-1);


# This file lives in libgimp and is used by many low-level
# libs. Exclude it in the calculations so the graph become nicer
ignored_interface_files = [
    "libgimp/libgimp-intl.h",
    ]

# List of library modules
libmodules = [
    "libgimp",
    "libgimpbase",
    "libgimpcolor",
    "libgimpconfig",
    "libgimpmath",
    "libgimpmodule",
    "libgimpthumb",
    "libgimpwidgets",
    ]

# List of app modules
# XXX: Maybe group some of these together to simplify graph?
appmodules = [
    "actions",
    "base",
    "composite",
    "config",
    "core",
    "dialogs",
    "display",
    "file",
    "gegl",
    "gui",
    "menus",
    "paint",
    "paint-funcs",
    "pdb",
    "plug-in",
    "tests",
    "text",
    "tools",
    "vectors",
    "widgets",
    "xcf",
    ]

# Bootstrap modules, i.e. modules we assume exist even though we don't
# have the code for them
boostrap_modules = [
    [ "GLib", ["glib.h"] ],
    [ "GTK+", ["gtk/gtk.h"] ],
    [ "GEGL", ["gegl.h"] ],
    [ "Pango", ["pango/pango.h"] ],
    [ "Cairo", ["cairo.h"] ],
    ]

##
# Function to determine if a filename is for an interface file
def is_interface_file(filename):
    return re.search("\.h$", filename)

##
# Function to determine if a filename is for an implementation file,
# i.e. a file that contains references to interface files
def is_implementation_file(filename):
    return re.search("\.c$", filename)

##
# Represents a software module. Think of it as a node in the
# dependency graph
class Module:
    def __init__(self, name, color, interface_files=[]):
        self.name = name
        self.color = color
        self.interface_files = Set(interface_files.__iter__())
        self.interface_file_dependencies = Set()
        self.dependencies = Set()

    def __repr__(self):
        return self.name

    def get_color(self):
        return self.color

    def get_interface_files(self):
        return self.interface_files

    def get_interface_file_dependencies(self):
        return self.interface_file_dependencies

    def get_dependencies(self):
        return self.dependencies

    def add_module_dependency(self, module):
        if self != module:
            self.dependencies.add(module)


# Represents a software module constructed from actual source code
class CodeModule(Module):
    def __init__(self, path, color):
        Module.__init__(self, path, color)

        all_files = os.listdir(path)

        # Collect interfaces this module provides
        for interface_file in filter(is_interface_file, all_files):
            self.interface_files.add(os.path.join(path, interface_file))

        # Collect dependencies to interfaces
        for filename in filter(is_implementation_file, all_files):
            with open(os.path.join(path, filename), 'r') as f:
                for line in f:
                    m = re.search("#include +[\"<](.*)[\">]", line)
                    if m:
                        interface_file = m.group(1)
                        # If the interface file appears to come from a core
                        # module, prepend with "app/"
                        m = re.search ("(.*)/.*", interface_file)
                        if m:
                            dirname = m.group(1)
                            if appmodules.__contains__(dirname):
                                interface_file = "app/" + interface_file

                        self.interface_file_dependencies.add(interface_file)

        for ignored_interface_file in ignored_interface_files:
            self.interface_file_dependencies.discard(ignored_interface_file)



# Initialize the modules to use for the dependency analysis
modules = Set()
for bootstrap_module in boostrap_modules:
    modules.add(Module(bootstrap_module[0], 'lightblue', bootstrap_module[1]))
for module_path in libmodules:
    modules.add(CodeModule(module_path, 'coral1'))
for module_path in appmodules:
    modules.add(CodeModule("app/" + module_path, 'lawngreen'))


# Map the interface files in the modules to the module that hosts them
interface_file_to_module = {}
for module in modules:
    for interface_file in module.get_interface_files():
        interface_file_to_module[interface_file] = module

# Figure out dependencies between modules
unknown_interface_files = Set()
for module in modules:
    interface_files = filter (is_interface_file, module.get_interface_file_dependencies())
    for interface_file in interface_files:
        if interface_file_to_module.has_key(interface_file):
            module.add_module_dependency(interface_file_to_module[interface_file])
        else:
            unknown_interface_files.add(interface_file)
if False:
    print "Unknown interface files:", unknown_interface_files

# Construct a GraphViz graph from the modules
dependency_graph = pygraphviz.AGraph(directed=True)
for module in modules:
    dependency_graph.add_node(module, fillcolor=module.get_color(), style='filled')
for module in modules:
    for depends_on in module.get_dependencies():
        dependency_graph.add_edge(module, depends_on)

# If module A depends on module B, and module B depends on module C, A
# gets C implicitly. Perform a transitive reduction on the graph to
# reflect this
dependency_graph.tred()

# Write result
if True:
    dependency_graph.draw("devel-docs/gimp-module-dependencies.svg", prog="dot")
else:
    dependency_graph.write("devel-docs/gimp-module-dependencies.dot")
