#!/usr/bin/env python

#   Interactive Python-GTK Console
#   Copyright (C), 1998 James Henstridge <james@daa.com.au>
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
#sys.path.append('/home/james/.gimp/plug-ins')
from gtk import *

stdout = sys.stdout

if not hasattr(sys, 'ps1'): sys.ps1 = '>>> '
if not hasattr(sys, 'ps2'): sys.ps2 = '... '

# some functions to help recognise breaks between commands
def remQuotStr(s):
	'''Returns s with any quoted strings removed (leaving quote marks)'''
	r = ''
	inq = 0
	qt = ''
	prev = '_'
	while len(s):
		s0, s = s[0], s[1:]
		if inq and (s0 != qt or prev == '\\'):
			prev = s0
			continue
		prev = s0
		if s0 in '\'"':
			if inq:
				inq = 0
			else:
				inq = 1
				qt = s0
		r = r + s0
	return r

def bracketsBalanced(s):
	'''Returns true iff the brackets in s are balanced'''
	s = filter(lambda x: x in '()[]{}', s)
	stack = []
	brackets = {'(':')', '[':']', '{':'}'}
	while len(s) != 0:
		if s[0] in ")]}":
			if len(stack) != 0 and brackets[stack[-1]] == s[0]:
				del stack[-1]
			else:
				return 0
		else:
			stack.append(s[0])
		s = s[1:]
	return len(stack) == 0

class gtkoutfile:
	'''A fake output file object.  It sends output to a TK test widget,
	and if asked for a file number, returns one set on instance creation'''
	def __init__(self, w, fn, font):
		self.__fn = fn
		self.__w = w
		self.__font = font
	def close(self): pass
	flush = close
	def fileno(self):    return self.__fn
	def isatty(self):    return 0
	def read(self, a):   return ''
	def readline(self):  return ''
	def readlines(self): return []
	def write(self, s):
	        #stdout.write(str(self.__w.get_point()) + '\n')
		self.__w.freeze()
		self.__w.insert(self.__font, self.__w.fg,
				self.__w.bg, s)
		self.__w.thaw()
		self.__w.queue_draw()
	def writelines(self, l):
		self.__w.freeze()
		for s in l: self.__w.insert(self.__font,
					    self.__w.fg, self.__w.bg, s)
		self.__w.thaw()
		self.__w.queue_draw()
	def seek(self, a):   raise IOError, (29, 'Illegal seek')
	def tell(self):      raise IOError, (29, 'Illegal seek')
	truncate = tell

class Console(GtkVBox):
	def __init__(self, namespace={}, copyright='', quit_cb=None):
		GtkVBox.__init__(self, spacing=2)
		self.set_border_width(2)
		self.copyright = copyright
		#self.set_usize(475, 300)

		self.quit_cb = quit_cb

		#load the fonts we will use
		self.normal = load_font(
			"-*-helvetica-medium-r-normal-*-*-100-*-*-*-*-*-*")
		self.title = load_font(
			"-*-helvetica-bold-r-normal-*-*-100-*-*-*-*-*-*")
		self.error = load_font(
			"-*-helvetica-medium-o-normal-*-12-100-*-*-*-*-*-*")
		self.command = load_font(
			"-*-helvetica-bold-r-normal-*-*-100-*-*-*-*-*-*")

		self.inp = GtkHBox()
		self.pack_start(self.inp)
		self.inp.show()

		self.text = GtkText()
		self.text.set_editable(FALSE)
		self.text.set_word_wrap(TRUE)
		self.text.set_usize(500, 400)
		self.inp.pack_start(self.text, padding=1)
		self.text.show()

		self.vscroll = GtkVScrollbar(self.text.get_vadjustment())
		self.vscroll.set_update_policy(POLICY_AUTOMATIC)
		self.inp.pack_end(self.vscroll, expand=FALSE)
		self.vscroll.show()

		self.inputbox = GtkHBox(spacing=2)
		self.pack_end(self.inputbox, expand=FALSE)
		self.inputbox.show()

		self.prompt = GtkLabel(sys.ps1)
		self.prompt.set_padding(xp=2, yp=0)
		self.prompt.set_usize(26, -1)
		self.inputbox.pack_start(self.prompt, fill=FALSE, expand=FALSE)
		self.prompt.show()

		self.closer = GtkButton("Close")
		self.closer.connect("clicked", self.quit)
		self.inputbox.pack_end(self.closer, fill=FALSE, expand=FALSE)
		self.closer.show()

		self.line = GtkEntry()
		self.line.set_usize(400,-1)
		self.line.connect("key_press_event", self.key_function)
		self.inputbox.pack_start(self.line, padding=2)
		self.line.show()

		# now let the text box be resized
		self.text.set_usize(0, 0)
		self.line.set_usize(0, -1)

		self.namespace = namespace

		self.cmd = ''
		self.cmd2 = ''

		# set up hooks for standard output.
		self.stdout = gtkoutfile(self.text, sys.stdout.fileno(),
					self.normal)
		self.stderr = gtkoutfile(self.text, sys.stderr.fileno(),
					self.error)

		# set up command history
		self.history = ['']
		self.histpos = 0
		self.namespace['__history__'] = self.history

	def init(self):
		self.text.realize()
		self.text.style = self.text.get_style()
		self.text.fg = self.text.style.fg[STATE_NORMAL]
		self.text.bg = self.text.style.white

		self.text.insert(self.title, self.text.fg,
				 self.text.bg, 'Python %s\n%s\n\n' %
				 (sys.version, sys.copyright) +
				 'Interactive Python-GTK Console - ' +
				 'Copyright (C) 1998 James Henstridge\n\n' +
				 self.copyright + '\n')
		self.line.grab_focus()

	def quit(self, *args):
		self.hide()
		self.destroy()
		if self.quit_cb: self.quit_cb()

	def key_function(self, entry, event):
		if event.keyval == GDK.Return:
			self.line.emit_stop_by_name("key_press_event")
			self.eval()
		if event.keyval == GDK.Tab:
		        self.line.emit_stop_by_name("key_press_event")
			self.line.append_text('\t')
			idle_add(self.focus_text)
		elif event.keyval in (GDK.KP_Up, GDK.Up):
			self.line.emit_stop_by_name("key_press_event")
			self.historyUp()
			idle_add(self.focus_text)
		elif event.keyval in (GDK.KP_Down, GDK.Down):
			self.line.emit_stop_by_name("key_press_event")
			self.historyDown()
			idle_add(self.focus_text)
		elif event.keyval in (GDK.D, GDK.d) and \
		     event.state & GDK.CONTROL_MASK:
			self.line.emit_stop_by_name("key_press_event")
			self.ctrld()

	def focus_text(self):
		self.line.grab_focus()
		return FALSE  # don't requeue this handler

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

	def eval(self):
		l = self.line.get_text() + '\n'
		if len(l) > 1 and l[0] == '\n': l = l[1:]
		self.histpos = len(self.history) - 1
		if len(l) > 0 and l[-1] == '\n':
			self.history[self.histpos] = l[:-1]
		else:
			self.history[self.histpos] = l
		self.line.set_text('')
		self.text.freeze()
		self.text.insert(self.command, self.text.fg, self.text.bg,
			self.prompt.get() + l)
		self.text.thaw()
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
		adj = self.text.get_vadjustment()
		adj.set_value(adj.upper - adj.page_size)
		sys.stdout, self.stdout = self.stdout, sys.stdout
		sys.stderr, self.stderr = self.stderr, sys.stderr

def gtk_console(ns, title='Python', copyright='', menu=None):
	win = GtkWindow()
	win.set_usize(475, 300)
	win.connect("destroy", mainquit)
	win.connect("delete_event", mainquit)
	win.set_title(title)
	cons = Console(namespace=ns, copyright=copyright, quit_cb=mainquit)
	if menu:
		box = GtkVBox()
		win.add(box)
		box.show()
		box.pack_start(menu, expand=FALSE)
		menu.show()
		box.pack_start(cons)
	else:
		win.add(cons)
	cons.show()
	win.show()
	win.set_usize(0,0)
	cons.init()
	mainloop()

if __name__ == '__main__':
	gtk_console({'__builtins__': __builtins__, '__name__': '__main__',
		'__doc__': None})

