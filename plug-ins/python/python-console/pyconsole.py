#
#   pyconsole.py
#
#   Copyright (C) 2004-2006 by Yevgen Muntyan <muntyan@math.tamu.edu>
#   Portions of code by Geoffrey French.
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU Lesser General Public version 2.1 as
#   published by the Free Software Foundation.
#
#   See COPYING.lib file that comes with this distribution for full text
#   of the license.
#

# This module 'runs' python interpreter in a TextView widget.
# The main class is Console, usage is:
# Console(locals=None, banner=None, completer=None, use_rlcompleter=True, start_script='') -
# it creates the widget and 'starts' interactive session; see the end
# of this file. If start_script is not empty, it pastes it as it was
# entered from keyboard.
#
# Console has "command" signal which is emitted when code is about to
# be executed. You may connect to it using console.connect or
# console.connect_after to get your callback ran before or after the
# code is executed.
#
# To modify output appearance, set attributes of console.stdout_tag and
# console.stderr_tag.
#
# Console may subclass a type other than gtk.TextView, to allow syntax
# highlighting and stuff,
# e.g.:
#   console_type = pyconsole.ConsoleType(moo.edit.TextView)
#   console = console_type(use_rlcompleter=False, start_script="import moo\nimport gtk\n")
#
# This widget is not a replacement for real terminal with python running
# inside: GtkTextView is not a terminal.
# The use case is: you have a python program, you create this widget,
# and inspect your program interiors.

import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk
gi.require_version('Gdk', '3.0')
from gi.repository import Gdk
from gi.repository import GObject
from gi.repository import Pango
import code
import sys
import keyword
import re

def pango_pixels(value):
    # The PANGO_PIXELS macro is not accessible through GObject
    # Introspection. Just reimplement it:
    # #define PANGO_PIXELS(d) (((int)(d) + 512) >> 10)
    return (value + 512) >> 10

# commonprefix() from posixpath
def _commonprefix(m):
    "Given a list of pathnames, returns the longest common leading component"
    if not m: return ''
    prefix = m[0]
    for item in m:
        for i in range(len(prefix)):
            if prefix[:i+1] != item[:i+1]:
                prefix = prefix[:i]
                if i == 0:
                    return ''
                break
    return prefix

class _ReadLine(object):

    class Output(object):
        def __init__(self, console, tag_name):
            object.__init__(self)
            self.buffer = console.get_buffer()
            self.tag_name = tag_name
        def write(self, text):
            pos = self.buffer.get_iter_at_mark(self.buffer.get_insert())
            self.buffer.insert_with_tags_by_name(pos, text, self.tag_name)
        def flush(self):
            # The output is just a GtkTextBuffer inside a GtkTexView so I
            # believe it should be always in-sync without needing to be flushed.
            # Nevertheless someone might be just copy-pasting plug-in code to
            # test it, for instance. So let's just add a no-op flush() method to
            # get a similar interface as the real sys.stdout which we overrode.
            # It avoids useless and unexpected code failure.
            pass

    class History(object):
        def __init__(self):
            object.__init__(self)
            self.items = ['']
            self.ptr = 0
            self.edited = {}

        def commit(self, text):
            if text and self.items[-1] != text:
                self.items.append(text)
            self.ptr = 0
            self.edited = {}

        def get(self, dir, text):
            if len(self.items) == 1:
                return None

            if text != self.items[self.ptr]:
                self.edited[self.ptr] = text
            elif self.ptr in self.edited:
                del self.edited[self.ptr]

            self.ptr = self.ptr + dir
            if self.ptr >= len(self.items):
                self.ptr = 0
            elif self.ptr < 0:
                self.ptr = len(self.items) - 1

            try:
                return self.edited[self.ptr]
            except KeyError:
                return self.items[self.ptr]

    def __init__(self, quit_func=None):
        object.__init__(self)

        self.quit_func = quit_func

        self.set_wrap_mode(Gtk.WrapMode.CHAR)
        self.modify_font(Pango.FontDescription("Monospace"))

        self.buffer = self.get_buffer()
        self.buffer.connect("insert-text", self.on_buf_insert)
        self.buffer.connect("delete-range", self.on_buf_delete)
        self.buffer.connect("mark-set", self.on_buf_mark_set)
        self.do_insert = False
        self.do_delete = False

        #Use the theme's color scheme for the text color
        font_color = self.get_style_context().get_property('color', Gtk.StateFlags.NORMAL)
        r = int (font_color.red * 255)
        g = int (font_color.green * 255)
        b = int (font_color.blue * 255)
        hex_font_color = "#" + '{r:02x}{g:02x}{b:02x}'.format (r = r, g = g, b = b)

        self.stdout_tag = self.buffer.create_tag("stdout", foreground=hex_font_color)
        self.stderr_tag = self.buffer.create_tag("stderr", foreground="#B00000")
        self._stdout = _ReadLine.Output(self, "stdout")
        self._stderr = _ReadLine.Output(self, "stderr")

        self.cursor = self.buffer.create_mark("cursor",
                                              self.buffer.get_start_iter(),
                                              False)
        insert = self.buffer.get_insert()
        self.cursor.set_visible(True)
        insert.set_visible(False)

        self.ps = ''
        self.in_raw_input = False
        self.in_modal_raw_input = False
        self.run_on_raw_input = None
        self.tab_pressed = 0
        self.history = _ReadLine.History()
        self.nonword_re = re.compile(r"[^\w\._]")

    def freeze_undo(self):
        try: self.begin_not_undoable_action()
        except: pass

    def thaw_undo(self):
        try: self.end_not_undoable_action()
        except: pass

    def raw_input(self, ps=None):
        '''Show prompt 'ps' and enter input mode until the current input
        is committed.'''

        if ps:
            self.ps = ps
        else:
            self.ps = ''

        iter = self.buffer.get_iter_at_mark(self.buffer.get_insert())

        if ps:
            self.freeze_undo()
            self.buffer.insert(iter, self.ps)
            self.thaw_undo()

        self.__move_cursor_to(iter)
        self.scroll_to_mark(self.cursor, 0.2, False, 0.0, 0.0)

        self.in_raw_input = True

        if self.run_on_raw_input:
            run_now = self.run_on_raw_input
            self.run_on_raw_input = None
            self.buffer.insert_at_cursor(run_now + '\n')

    def modal_raw_input(self, text):
        '''Starts raw input in modal mode. The event loop is spinned until
        the input is committed. Returns the text entered after the prompt.'''
        orig_ps = self.ps

        self.raw_input(text)
        self.in_modal_raw_input = True

        while self.in_modal_raw_input:
            Gtk.main_iteration()

        self.ps = orig_ps
        self.in_modal_raw_input = False
        self.in_raw_input = False

        return self.modal_raw_input_result

    def modal_input(self, text):
        return eval(self.modal_raw_input(text))

    # Each time the insert mark is modified, move the cursor to it.
    def on_buf_mark_set(self, buffer, iter, mark):
        if mark is not buffer.get_insert():
            return
        start = self.__get_start()
        end = self.__get_end()
        if iter.compare(self.__get_start()) >= 0 and \
           iter.compare(self.__get_end()) <= 0:
                buffer.move_mark_by_name("cursor", iter)
                self.scroll_to_mark(self.cursor, 0.2, False, 0.0, 0.0)

    def __insert(self, iter, text):
        self.do_insert = True
        self.buffer.insert(iter, text)
        self.do_insert = False

    # Make sure that text insertions while in text input mode are properly
    # committed to the history.
    def on_buf_insert(self, buf, iter, text, len):
        # Bail out if not in input mode.
        if not self.in_raw_input or self.do_insert or not len:
            return

        buf.stop_emission("insert-text")
        lines = text.splitlines()
        need_eol = False
        for l in lines:
            if need_eol:
                self._commit()
                iter = self.__get_cursor()
            else:
                cursor = self.__get_cursor()
                if iter.compare(self.__get_start()) < 0:
                    iter = cursor
                elif iter.compare(self.__get_end()) > 0:
                    iter = cursor
                else:
                    self.__move_cursor_to(iter)
            need_eol = True
            self.__insert(iter, l)
        self.__move_cursor(0)

    def __delete(self, start, end):
        self.do_delete = True
        self.buffer.delete(start, end)
        self.do_delete = False

    def on_buf_delete(self, buf, start, end):
        if not self.in_raw_input or self.do_delete:
            return

        buf.stop_emission("delete-range")

        start.order(end)
        line_start = self.__get_start()
        line_end = self.__get_end()

        if start.compare(line_end) > 0:
            return
        if end.compare(line_start) < 0:
            return

        self.__move_cursor(0)

        if start.compare(line_start) < 0:
            start = line_start
        if end.compare(line_end) > 0:
            end = line_end
        self.__delete(start, end)

    # We overload the key press event handler to handle "special keys"
    # when in input mode to make history browsing, completions, etc. work.
    def do_key_press_event(self, event):
        if not self.in_raw_input:
            return Gtk.TextView.do_key_press_event(self, event)

        tab_pressed = self.tab_pressed
        self.tab_pressed = 0
        handled = True

        state = event.state & (Gdk.ModifierType.SHIFT_MASK |
                               Gdk.ModifierType.CONTROL_MASK |
                               Gdk.ModifierType.MOD1_MASK)
        keyval = event.keyval

        if not state:
            if keyval == Gdk.KEY_Escape:
                pass
            elif keyval == Gdk.KEY_Return:
                self._commit()
            elif keyval == Gdk.KEY_Up:
                self.__history(-1)
            elif keyval == Gdk.KEY_Down:
                self.__history(1)
            elif keyval == Gdk.KEY_Left:
                self.__move_cursor(-1)
            elif keyval == Gdk.KEY_Right:
                self.__move_cursor(1)
            elif keyval == Gdk.KEY_Home:
                self.__move_cursor(-10000)
            elif keyval == Gdk.KEY_End:
                self.__move_cursor(10000)
            elif keyval == Gdk.KEY_Tab:
                cursor = self.__get_cursor()
                if cursor.starts_line():
                    handled = False
                else:
                    cursor.backward_char()
                    if cursor.get_char().isspace():
                        handled = False
                    else:
                        self.tab_pressed = tab_pressed + 1
                        self.__complete()
            else:
                handled = False
        elif state == Gdk.ModifierType.CONTROL_MASK:
            if keyval == Gdk.KEY_u:
                start = self.__get_start()
                end = self.__get_cursor()
                self.__delete(start, end)
            elif keyval == Gdk.KEY_d:
                if self.quit_func:
                    self.quit_func()
            else:
                handled = False
        else:
            handled = False

        # Handle ordinary keys
        if not handled:
            return Gtk.TextView.do_key_press_event(self, event)
        else:
            return True

    def __history(self, dir):
        text = self._get_line()
        new_text = self.history.get(dir, text)
        if not new_text is None:
            self.__replace_line(new_text)
        self.__move_cursor(0)
        self.scroll_to_mark(self.cursor, 0.2, False, 0.0, 0.0)

    def __get_cursor(self):
        '''Returns an iterator at the current cursor position.'''
        return self.buffer.get_iter_at_mark(self.cursor)

    def __get_start(self):
        '''Returns an iterator at the start of the input on the current
        cursor line.'''

        iter = self.__get_cursor()
        iter.set_line(iter.get_line())
        iter.forward_chars(len(self.ps))
        return iter

    def __get_end(self):
        '''Returns an iterator at the end of the cursor line.'''
        iter = self.__get_cursor()
        if not iter.ends_line():
            iter.forward_to_line_end()
        return iter

    def __get_text(self, start, end):
        '''Get text between 'start' and 'end' markers.'''
        return self.buffer.get_text(start, end, False)

    def __move_cursor_to(self, iter):
        self.buffer.place_cursor(iter)
        self.buffer.move_mark_by_name("cursor", iter)

    def __move_cursor(self, howmany):
        iter = self.__get_cursor()
        end = self.__get_cursor()
        if not end.ends_line():
            end.forward_to_line_end()
        line_len = end.get_line_offset()
        move_to = iter.get_line_offset() + howmany
        move_to = min(max(move_to, len(self.ps)), line_len)
        iter.set_line_offset(move_to)
        self.__move_cursor_to(iter)

    def __delete_at_cursor(self, howmany):
        iter = self.__get_cursor()
        end = self.__get_cursor()
        if not end.ends_line():
            end.forward_to_line_end()
        line_len = end.get_line_offset()
        erase_to = iter.get_line_offset() + howmany
        if erase_to > line_len:
            erase_to = line_len
        elif erase_to < len(self.ps):
            erase_to = len(self.ps)
        end.set_line_offset(erase_to)
        self.__delete(iter, end)

    def __get_width(self):
        '''Estimate the number of characters that will fit in the area
        currently allocated to this widget.'''

        if not self.get_realized():
            return 80

        context = self.get_pango_context()
        metrics = context.get_metrics(context.get_font_description(),
                                      context.get_language())
        pix_width = metrics.get_approximate_char_width()
        allocation = Gtk.Widget.get_allocation(self)
        return allocation.width * Pango.SCALE / pix_width

    def __print_completions(self, completions):
        line_start = self.__get_text(self.__get_start(), self.__get_cursor())
        line_end = self.__get_text(self.__get_cursor(), self.__get_end())
        iter = self.buffer.get_end_iter()
        self.__move_cursor_to(iter)
        self.__insert(iter, "\n")

        width = max(self.__get_width(), 4)
        max_width = max(len(s) for s in completions)
        n_columns = max(int(width / (max_width + 1)), 1)
        col_width = int(width / n_columns)
        total = len(completions)
        col_length = total / n_columns
        if total % n_columns:
            col_length = col_length + 1
        col_length = max(col_length, 1)

        if col_length == 1:
            n_columns = total
            col_width = width / total

        for i in range(int(col_length)):
            for j in range(n_columns):
                ind = i + j*col_length
                if ind < total:
                    if j == n_columns - 1:
                        n_spaces = 0
                    else:
                        n_spaces = int(col_width - len(completions[int(ind)]))
                    self.__insert(iter, completions[int(ind)] + " " * n_spaces)
            self.__insert(iter, "\n")

        self.__insert(iter, "%s%s%s" % (self.ps, line_start, line_end))
        iter.set_line_offset(len(self.ps) + len(line_start))
        self.__move_cursor_to(iter)
        self.scroll_to_mark(self.cursor, 0.2, False, 0.0, 0.0)

    def __complete(self):
        text = self.__get_text(self.__get_start(), self.__get_cursor())
        start = ''
        word = text
        nonwords = self.nonword_re.findall(text)
        if nonwords:
            last = text.rfind(nonwords[-1]) + len(nonwords[-1])
            start = text[:last]
            word = text[last:]

        completions = self.complete(word)

        if completions:
            prefix = _commonprefix(completions)
            if prefix != word:
                start_iter = self.__get_start()
                start_iter.forward_chars(len(start))
                end_iter = start_iter.copy()
                end_iter.forward_chars(len(word))
                self.__delete(start_iter, end_iter)
                self.__insert(end_iter, prefix)
            elif self.tab_pressed > 1:
                self.freeze_undo()
                self.__print_completions(completions)
                self.thaw_undo()
                self.tab_pressed = 0

    def complete(self, text):
        return None

    def _get_line(self):
        '''Return the current input behind the prompt.'''
        start = self.__get_start()
        end = self.__get_end()
        return self.buffer.get_text(start, end, False)

    def __replace_line(self, new_text):
        '''Replace the current input with 'new_text' '''
        start = self.__get_start()
        end = self.__get_end()
        self.__delete(start, end)
        self.__insert(end, new_text)

    def _commit(self):
        '''Commit the input entered on the current line.'''

        # Find iterator and end of cursor line.
        end = self.__get_cursor()
        if not end.ends_line():
            end.forward_to_line_end()

        # Get text at current line.
        text = self._get_line()

        # Move cursor to the end of the line, insert new line.
        self.__move_cursor_to(end)
        self.freeze_undo()
        self.__insert(end, "\n")

        self.history.commit(text)
        if self.in_modal_raw_input:
            self.in_modal_raw_input = False
            self.modal_raw_input_result = text
        else:
            self.in_raw_input = False
            self.do_raw_input(text)

        self.thaw_undo()

    def do_raw_input(self, text):
        pass


class _Console(_ReadLine, code.InteractiveInterpreter):
    def __init__(self, locals=None, banner=None,
                 completer=None, use_rlcompleter=True,
                 start_script=None, quit_func=None):
        _ReadLine.__init__(self, quit_func)

        code.InteractiveInterpreter.__init__(self, locals)
        self.locals["__console__"] = self

        # The builtin raw_input function reads from stdin, we don't want
        # this. Therefore, replace this function with our own modal raw
        # input function.
        exec ("import builtins", self.locals)
        #self.locals['builtins'].__dict__['raw_input'] = lambda text='': self.modal_raw_input(text)
        self.locals['builtins'].__dict__['input'] = lambda text='': self.modal_input(text)

        self.start_script = start_script
        self.completer = completer
        self.banner = banner

        if not self.completer and use_rlcompleter:
            try:
                import rlcompleter
                self.completer = rlcompleter.Completer()
            except ImportError:
                pass

        self.ps1 = ">>> "
        self.ps2 = "... "
        self.__start()
        self.run_on_raw_input = start_script
        self.raw_input(self.ps1)

    def __start(self):
        self.cmd_buffer = ""

        self.freeze_undo()
        self.thaw_undo()

        self.do_delete = True
        self.buffer.set_text("")
        self.do_delete = False

        if self.banner:
            iter = self.buffer.get_start_iter()
            self.buffer.insert_with_tags_by_name(iter, self.banner, "stdout")
            if not iter.starts_line():
                self.buffer.insert(iter, "\n")

    def clear(self, start_script=None):
        if start_script is None:
            start_script = self.start_script
        else:
            self.start_script = start_script

        self.__start()
        self.run_on_raw_input = start_script
        self.raw_input(self.ps1)

    def do_raw_input(self, text):
        if self.cmd_buffer:
            cmd = self.cmd_buffer + "\n" + text
        else:
            cmd = text

        saved_stdout, saved_stderr = sys.stdout, sys.stderr
        sys.stdout, sys.stderr = self._stdout, self._stderr

        if self.runsource(cmd):
            self.cmd_buffer = cmd
            ps = self.ps2
        else:
            self.cmd_buffer = ''
            ps = self.ps1

        sys.stdout, sys.stderr = saved_stdout, saved_stderr
        self.raw_input(ps)

    def do_command(self, code):
        try:
            eval(code, self.locals)
        except SystemExit:
            if self.quit_func:
                self.quit_func()
            else:
                raise
        except:
            self.showtraceback()

    def runcode(self, code):
        #if gtk.pygtk_version[1] < 8:
        self.do_command(code)
        #else:
            #self.emit("command", code)

    def complete_attr(self, start, end):
        try:
            obj = eval(start, self.locals)
            strings = dir(obj)

            if end:
                completions = {}
                for s in strings:
                    if s.startswith(end):
                        completions[s] = None
                completions = completions.keys()
            else:
                completions = strings

            completions.sort()
            return [start + "." + s for s in completions]
        except:
            return None

    def complete(self, text):
        if self.completer:
            completions = []
            i = 0
            try:
                while 1:
                    s = self.completer.complete(text, i)
                    if s:
                        completions.append(s)
                        i += 1
                    else:
                        completions.sort()
                        return completions
            except NameError:
                return None

        dot = text.rfind(".")
        if dot >= 0:
            return self.complete_attr(text[:dot], text[dot+1:])

        completions = {}
        strings = keyword.kwlist

        if self.locals:
            strings.extend(self.locals.keys())

        try: strings.extend(eval("globals()", self.locals).keys())
        except: pass

        try:
            exec("import __builtin__", self.locals)
            strings.extend(eval("dir(__builtin__)", self.locals))
        except:
            pass

        for s in strings:
            if s.startswith(text):
                completions[s] = None
        completions = completions.keys()
        completions.sort()
        return completions


def ReadLineType(t=Gtk.TextView):
    class readline(t, _ReadLine):
        def __init__(self, *args, **kwargs):
            t.__init__(self)
            _ReadLine.__init__(self, *args, **kwargs)
        def do_key_press_event(self, event):
            return _ReadLine.do_key_press_event(self, event)
    GObject.type_register(readline)
    return readline

def ConsoleType(t=Gtk.TextView):
    class console_type(t, _Console):
        __gsignals__ = {
            'command' : (GObject.SIGNAL_RUN_LAST, GObject.TYPE_NONE, (object,)),
          }

        def __init__(self, *args, **kwargs):
            #if gtk.pygtk_version[1] < 8:
            GObject.GObject.__init__(self)
            #else:
                #t.__init__(self)
            _Console.__init__(self, *args, **kwargs)

        def do_command(self, code):
            return _Console.do_command(self, code)

        def do_key_press_event(self, event):
            return _Console.do_key_press_event(self, event)

        def get_default_size(self):
            context = self.get_pango_context()
            metrics = context.get_metrics(context.get_font_description(),
                                          context.get_language())
            width = metrics.get_approximate_char_width()
            height = metrics.get_ascent() + metrics.get_descent()

            # Default to a 80x40 console
            width = pango_pixels(int(width * 80 * 1.05))
            height = pango_pixels(height * 40)

            return width, height

    #if gtk.pygtk_version[1] < 8:
    GObject.type_register(console_type)

    return console_type

ReadLine = ReadLineType()
Console = ConsoleType()

def _make_window():
    window = Gtk.Window()
    window.set_title("pyconsole.py")
    swin = Gtk.ScrolledWindow()
    swin.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.ALWAYS)
    window.add(swin)
    console = Console(banner="Hello there!",
                      use_rlcompleter=False,
                      start_script="gi.require_version('Gimp', '3.0')\nfrom gi.repository import Gimp\n")
    swin.add(console)

    width, height = console.get_default_size()
    sb_width, sb_height = swin.get_vscrollbar().size_request()

    window.set_default_size(width + sb_width, height)
    window.show_all()

    if not Gtk.main_level():
        window.connect("destroy", Gtk.main_quit)
        Gtk.main()

    return console

if __name__ == '__main__':
    if len(sys.argv) < 2 or sys.argv[1] != '-gimp':
        _make_window()
