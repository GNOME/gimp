#!/usr/bin/env python

#   Interactive Python-GTK Console
#   Copyright (C), 1998 James Henstridge <james@daa.com.au>
#
#   This program is free software; you can redistribute it and/or modify
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
#   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

# This module implements an interactive python session in a GTK window.  To
# start the session, use the gtk_console command.  Its specification is:
#   gtk_console(namespace, title, copyright)
# where namespace is a dictionary representing the namespace of the session,
#       title     is the title on the window and
#       copyright is any additional copyright info to print.
#
# As well as the starting attributes in namespace, the session will also
# have access to the list __history__, which is the command history.

import sys, string, traceback
import pango, gtk, gtk.keysyms

stdout = sys.stdout

if not hasattr(sys, 'ps1'): sys.ps1 = '>>> '
if not hasattr(sys, 'ps2'): sys.ps2 = '... '

# some functions to help recognise breaks between commands
def remQuotStr(s):
    '''Returns s with any quoted strings removed (leaving quote marks)'''
    ret = ''
    in_quote = 0
    quote = ''
    prev = None
    for ch in s:
        if in_quote and (ch != quote or prev == '\\'):
            if ch == '\\' and prev == '\\':
                prev = None
            else:
                prev = ch
            continue
            prev = ch
        if ch in '\'"':
            if in_quote:
                in_quote = 0
            else:
                in_quote = 1
                quote = ch
        ret = ret + ch
    return ret

def bracketsBalanced(s):
    '''Returns true iff the brackets in s are balanced'''
    stack = []
    brackets = {'(':')', '[':']', '{':'}'}
    for ch in s:
        if ch in '([{':
            stack.append(ch)
        elif ch in '}])':
            if len(stack) != 0 and brackets[stack[-1]] == ch:
                del stack[-1]
            else:
                return 0
    return len(stack) == 0

class gtkoutfile:
    '''A fake output file object.  It sends output to a TK test widget,
    and if asked for a file number, returns one set on instance creation'''
    def __init__(self, buffer, fileno, tag):
        self.__fileno = fileno
        self.__buffer = buffer
        self.__tag = tag
    def close(self): pass
    flush = close
    def fileno(self):    return self.__fileno
    def isatty(self):    return 0
    def read(self, a):   return ''
    def readline(self):  return ''
    def readlines(self): return []
    def write(self, string):
        #stdout.write(str(self.__w.get_point()) + '\n')
        iter = self.__buffer.get_end_iter()
        self.__buffer.insert_with_tags_by_name(iter, string, self.__tag)
    def writelines(self, lines):
        iter = self.__buffer.get_end_iter()
        for line in lines:
            self.__buffer.insert_with_tags_by_name(iter, lines, self.__tag)
    def seek(self, a):   raise IOError, (29, 'Illegal seek')
    def tell(self):      raise IOError, (29, 'Illegal seek')
    truncate = tell

class Console(gtk.VBox):
    def __init__(self, namespace={}, quit_cb=None):
        gtk.VBox.__init__(self, spacing=2)
        self.set_border_width(2)

        self.quit_cb = quit_cb

        swin = gtk.ScrolledWindow()
        swin.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        self.pack_start(swin)
        swin.show()

        self.vadj = swin.get_vadjustment()

        self.textview = gtk.TextView()
        self.textview.set_editable(gtk.FALSE)
        self.textview.set_cursor_visible(gtk.FALSE)
        self.textview.set_wrap_mode(gtk.WRAP_WORD)
        self.buffer = self.textview.get_buffer()
        swin.add(self.textview)
        self.textview.show()

        # tags to use when inserting text ...
        tag = self.buffer.create_tag('normal')
        tag = self.buffer.create_tag('command')
        tag.set_property('weight', pango.WEIGHT_BOLD)
        tag = self.buffer.create_tag('error')
        tag.set_property('style', pango.STYLE_ITALIC)

        self.inputbox = gtk.HBox(spacing=2)
        self.pack_end(self.inputbox, expand=gtk.FALSE)
        self.inputbox.show()

        self.prompt = gtk.Label(sys.ps1)
        self.prompt.set_padding(2, 0)
        self.inputbox.pack_start(self.prompt, expand=gtk.FALSE)
        self.prompt.show()

        self.closer = gtk.Button("Close")
        self.closer.connect("clicked", self.quit)
        self.inputbox.pack_end(self.closer, expand=gtk.FALSE)
        self.closer.show()

        self.line = gtk.Entry()
        self.line.connect("key_press_event", self.key_function)
        self.inputbox.pack_start(self.line, padding=2)
        self.line.show()

        self.namespace = namespace

        self.cmd = ''
        self.cmd2 = ''

        # set up hooks for standard output.
        self.stdout = gtkoutfile(self.buffer, sys.stdout.fileno(), 'normal')
        self.stderr = gtkoutfile(self.buffer, sys.stderr.fileno(), 'error')

        # set up command history
        self.history = ['']
        self.histpos = 0
        self.namespace['__history__'] = self.history

    def init(self):
        message = 'Python %s\n\n' % (sys.version,)
        iter = self.buffer.get_end_iter()
        self.buffer.insert_with_tags_by_name(iter, message, 'command') 
        self.line.grab_focus()

    def quit(self, *args):
        self.hide()
        self.destroy()
        if self.quit_cb: self.quit_cb()

    def key_function(self, entry, event):
        if event.keyval == gtk.keysyms.Return:
            self.line.emit_stop_by_name("key_press_event")
            self.eval()
        if event.keyval == gtk.keysyms.Tab:
            self.line.emit_stop_by_name("key_press_event")
            self.line.append_text('\t')
            gtk.idle_add(self.focus_text)
        elif event.keyval in (gtk.keysyms.KP_Up, gtk.keysyms.Up):
            self.line.emit_stop_by_name("key_press_event")
            self.historyUp()
            gtk.idle_add(self.focus_text)
        elif event.keyval in (gtk.keysyms.KP_Down, gtk.keysyms.Down):
            self.line.emit_stop_by_name("key_press_event")
            self.historyDown()
            gtk.idle_add(self.focus_text)
        elif event.keyval in (gtk.keysyms.D, gtk.keysyms.d) and \
                 event.state & gtk.gdk.CONTROL_MASK:
            self.line.emit_stop_by_name("key_press_event")
            self.ctrld()

    def focus_text(self):
        self.line.grab_focus()
        return gtk.FALSE  # don't requeue this handler
    def scroll_bottom(self):
        self.vadj.set_value(self.vadj.upper - self.vadj.page_size)
        return gtk.FALSE

    def ctrld(self):
        #self.quit()
        pass

    def historyUp(self):
        if self.histpos > 0:
            l = self.line.get_text()
            if len(l) > 0 and l[0] == '\n': l = l[1:]
            if len(l) > 0 and l[-1] == '\n': l = l[:-1]
            self.history[self.histpos] = l
            self.histpos = self.histpos - 1
            self.line.set_text(self.history[self.histpos])

    def historyDown(self):
        if self.histpos < len(self.history) - 1:
            l = self.line.get_text()
            if len(l) > 0 and l[0] == '\n': l = l[1:]
            if len(l) > 0 and l[-1] == '\n': l = l[:-1]
            self.history[self.histpos] = l
            self.histpos = self.histpos + 1
            self.line.set_text(self.history[self.histpos])
            self.line.set_position(-1)

    def eval(self):
        l = self.line.get_text() + '\n'
        if len(l) > 1 and l[0] == '\n': l = l[1:]
        self.histpos = len(self.history) - 1
        if len(l) > 0 and l[-1] == '\n':
            self.history[self.histpos] = l[:-1]
        else:
            self.history[self.histpos] = l
        self.line.set_text('')
        iter = self.buffer.get_end_iter()
        self.buffer.insert_with_tags_by_name(iter, self.prompt.get_text() + l,
                                             'command')
        if l == '\n':
            self.run(self.cmd)
            self.cmd = ''
            self.cmd2 = ''
            return
        self.histpos = self.histpos + 1
        self.history.append('')
        self.cmd = self.cmd + l
        self.cmd2 = self.cmd2 + remQuotStr(l)
        l = string.rstrip(l)
        if not bracketsBalanced(self.cmd2) or l[-1] == ':' or \
                l[-1] == '\\' or l[0] in ' \11':
            self.prompt.set_text(sys.ps2)
            self.prompt.queue_draw()
            return
        self.run(self.cmd)
        self.cmd = ''
        self.cmd2 = ''

    def run(self, cmd):
        sys.stdout, self.stdout = self.stdout, sys.stdout
        sys.stderr, self.stderr = self.stderr, sys.stderr
        try:
            try:
                r = eval(cmd, self.namespace, self.namespace)
                self.namespace['_'] = r
                if r is not None:
                    print `r`
            except SyntaxError:
                exec cmd in self.namespace
        except:
            if hasattr(sys, 'last_type') and \
                    sys.last_type == SystemExit:
                self.quit()
            else:
                traceback.print_exc()
        self.prompt.set_text(sys.ps1)
        self.prompt.queue_draw()
        gtk.idle_add(self.scroll_bottom)
        sys.stdout, self.stdout = self.stdout, sys.stdout
        sys.stderr, self.stderr = self.stderr, sys.stderr

def gtk_console(ns, title='Python', menu=None):
    win = gtk.Window()
    win.set_default_size(475, 300)
    win.connect("destroy", gtk.mainquit)
    win.connect("delete_event", gtk.mainquit)
    win.set_title(title)
    cons = Console(namespace=ns, quit_cb=gtk.mainquit)
    if menu:
        box = gtk.VBox()
        win.add(box)
        box.show()
        box.pack_start(menu, expand=gtk.FALSE)
        menu.show()
        box.pack_start(cons)
    else:
        win.add(cons)
    cons.show()
    win.show()
    cons.init()
    gtk.main()

if __name__ == '__main__':
    if len(sys.argv) < 2 or sys.argv[1] != '-gimp':
        gtk_console({'__builtins__': __builtins__, '__name__': '__main__',
                     '__doc__': None})
