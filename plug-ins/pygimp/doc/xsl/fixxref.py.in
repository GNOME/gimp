#!/usr/bin/env python
# -*- Mode: Python; py-indent-offset: 4 -*-

import getopt
import os
import re
import sys

anchors = {}
anchor_pat = re.compile(r'''^\s*<ANCHOR\s+id\s*=\s*"([^"]*)"\s+
                            href\s*=\s*"([^"]*)"\s*>''',
                        re.MULTILINE | re.VERBOSE)
link_pat = re.compile(r'''<PYGTKDOCLINK\s+HREF="([^"]*)"\s*>(.*?)
                          </PYGTKDOCLINK\s*>''', re.DOTALL | re.VERBOSE)
def scan_index_dir(idir):
    for root, dirs, files in os.walk(idir):
        if 'index.sgml' in files:
            scan_index_file(os.path.join(root, 'index.sgml'))
    return

def scan_index_file(ifile):
    buf = open(ifile).read()
    for id, href in anchor_pat.findall(buf):
        anchors[id] = href

def fix_xrefs(hdir):
    for f in os.listdir(hdir):
        if os.path.splitext(f)[1] == '.html':
            fix_html_file(os.path.join(hdir, f))

def link_subst(m):
    id, text = m.groups()
    if anchors.has_key(id):
        return '<a\nhref="../' + anchors[id] + '"\n>' + text + '</a>'
    return text

def fix_html_file(hfile):
    buf = open(hfile).read()
    buf = link_pat.sub(link_subst, buf)
    open(hfile, 'w').write(buf)

def usage(e=None):
    if e:
        sys.stderr.write('fixxref.py: %s\n' % e)
    sys.stderr.write('usage: fixxref.py [-i index-dir] html-dir\n')
    sys.exit(1)

if __name__ == '__main__':
    try:
        opts, args = getopt.getopt(sys.argv[1:], "i:h:",
                                   ["index-dir=", "html-dir="])
    except getopt.error, e:
        usage(e)

    index_dirs = []
    for opt, arg in opts:
        if opt in ('-i', '--index-dir'):
            index_dirs.append(arg)

    if len(args) != 1:
        usage()

    for idir in index_dirs:
        scan_index_dir(idir)

    html_dir = args[0]
    fix_xrefs(html_dir)
