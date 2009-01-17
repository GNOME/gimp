#!/usr/bin/env python
# Copyright (C) 2003  Helvetix Victorinox, a pseudonym, <helvetix@gimp.org>
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
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# -*- mode: python py-indent-offset: 2; -*-
#
# Look at object files and figure things about the namespaces they
# require and provide.
#
# It is very useful when working on libraries where you really should
# be hygenic about the namespace you occupy and not clutter it with
# conflicting and extraneous names.
#

import os
import re
import sys
import string
import pprint

pp = pprint.PrettyPrinter(indent=2)

#
# for each object file, we keep two lists: exported names and imported names.
#
# nm -A [files...]
#
class nmx:
    def __init__(self, objfile=None):
        self.objects = dict()
        self.filename = None

        if objfile != None:
            self.update(objfile)
            pass

        return (None)

    def split_(self, line):
      tmp=string.split(line)[0:2]
      tmp.reverse()
      return tmp

    def update(self, objfile):
        self.filename = objfile

        (sysname, nodename, release, version, machine) = os.uname()
        if sysname == "Linux":
            fp = os.popen("nm -P " + objfile, "r")
            symbols = map(self.split_, fp.readlines())
        elif sysname == "SunOS":
            fp = os.popen("nm -p " + objfile, "r")
            symbols = map(lambda l: string.split(l[12:]), fp.readlines())
            pass
        elif sysname == "IRIX":
            fp = os.popen("nm -B " + objfile, "r")
            symbols = map(lambda l: string.split(l[8:]), fp.readlines())
            pass

        object = objfile

        for (type, symbol) in symbols:
            if not self.objects.has_key(object):
                self.objects.update({ object : dict({ "exports" : dict(), "imports" : dict() }) })
                pass

            if type == "U":
                self.objects[object]["imports"].update({ symbol : dict() })
            elif type in ["C", "D", "T"]:
                self.objects[object]["exports"].update({ symbol : dict() })
                pass
            pass

        fp.close()
        return (None)

    def exports(self, name):
        for o in self.objects.keys():
            if self.objects[o]["exports"].has_key(name):
                return (1)
            pass
        return (0)

    def exports_re(self, name):
        regex = re.compile(name)

        for o in self.objects.keys():
            for p in self.objects[o]["exports"].keys():
                if regex.match(p):
                    return (p)
                pass
            pass
        return (None)

    pass


def nm(nmfile):
    objects = dict()

    fp = open(nmfile, "r")
    for line in fp.readlines():
        (object, type, symbol) = string.split(line)
        object = object[:string.rfind(object, ':')]

        if not objects.has_key(object):
            objects.update({ object : dict({"exports" : dict(), "imports" : dict()})})
            pass

        if type == "U":
            objects[object]["imports"].update({symbol : dict()})
        elif type in ["C", "D", "T"]:
            objects[object]["exports"].update({symbol : dict()})
        pass

    fp.close()
    return (objects)

def resolve_(objects, obj):

    for object in objects.keys():
        if object != obj:
            for imported in objects[obj]["imports"].keys():
                if objects[object]["exports"].has_key(imported):
                    objects[obj]["imports"][imported] = object
                    pass
                pass

            for exported in objects[obj]["exports"].keys():
                if objects[object]["imports"].has_key(exported):
                    objects[obj]["exports"][exported] = object
                    pass
                pass
            pass
        pass

    return

def resolve(objects):

    for object in objects.keys():
        resolve_(objects, object)

    return (objects)

def report_unreferenced(objects):
    for object in objects.keys():
        for symbol in objects[object]["exports"].keys():
            if len(objects[object]["exports"][symbol]) == 0:
                print object + ":" + symbol, "unreferenced"
                pass
            pass
        pass
    return

def report_referenced(objects):
    for object in objects.keys():
        for symbol in objects[object]["imports"].keys():
            if len(objects[object]["imports"][symbol]) > 0:
                print objects[object]["imports"][symbol] + ":" + symbol, object, "referenced"
                pass
            pass
        pass
    return

def make_depend(objects):
    for object in objects.keys():
        for symbol in objects[object]["imports"].keys():
            if len(objects[object]["imports"][symbol]) > 0:
                print object + ":" + symbol, "referenced", objects[object]["imports"][symbol]
                pass
            pass
        pass
    return


def main(argv):
    ns = nm(argv[0])

    resolve(ns)

    report_referenced(ns)
    report_unreferenced(ns)
    pass

if __name__ == "__main__":
    main(sys.argv[1:])
