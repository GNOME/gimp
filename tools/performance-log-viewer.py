#!/usr/bin/env python3

"""
performance-log-viewer.py -- GIMP performance log viewer
Copyright (C) 2018  Ell

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.


Usage: performance-log-viewer.py < infile
"""

import builtins, sys, os, math, statistics, bisect, functools, enum, re, \
       subprocess

from collections import namedtuple
from xml.etree   import ElementTree

import gi
gi.require_version ("Gdk", "3.0")
gi.require_version ("Gtk", "3.0")
from gi.repository import GLib, GObject, Gio, Gdk, Gtk, Pango

def compose (head = None, *tail):
    return (
        lambda *args, **kwargs: head (compose (*tail) (*args, **kwargs))
    ) if tail else head or (lambda x: x)

def div (x, y):
    return x / y     if y     else \
           +math.inf if x > 0 else \
           -math.inf if x < 0 else \
           None

def format_float (x):
    return "%g" % (round (100 * x) / 100)

def format_percentage (x, digits = 0):
    return "%%.%df%%%%" % digits % (100 * x)

def format_size (size):
    return GLib.format_size_full (size, GLib.FormatSizeFlags.IEC_UNITS)

def format_duration (t):
    return "%02d:%02d:%02d.%02d" % (int (t / 3600),
                                    int (t / 60) % 60,
                                    int (t % 60),
                                    round (100 * t) % 100)

def format_color (color):
    return "#%02x%02x%02x" % tuple (
        map (lambda x: min (max (round (255 * x), 0), 255), color)
    )

def is_bright_color (color):
    return max (tuple (color)[0:3]) > 0.5

def blend_colors (color1, color2, amount):
    color1 = tuple (color1)
    color2 = tuple (color2)

    a1 = color1[-1]
    a2 = color2[-1]

    a = (1 - amount) * a1 + amount * a2

    return tuple (a and ((1 - amount) * a1 * c1 + amount * a2 * c2) / a
                  for c1, c2 in zip (color1[:-1], color2[:-1])) + (a,)

def rounded_rectangle (cr, x, y, width, height, radius):
    radius = min (radius, width / 2, height / 2)

    cr.arc (x + radius, y + radius, radius, -math.pi, -math.pi / 2)
    cr.rel_line_to (width - 2 * radius, 0)

    cr.arc (x + width - radius, y + radius, radius, -math.pi / 2, 0)
    cr.rel_line_to (0, height - 2 * radius)

    cr.arc (x + width - radius, y + height - radius, radius, 0, math.pi / 2)
    cr.rel_line_to (-(width - 2 * radius), 0)

    cr.arc (x + radius, y + height - radius, radius, math.pi / 2, math.pi)
    cr.rel_line_to (0, -(height - 2 * radius))

    cr.close_path ()

def get_basename (path):
    match = re.fullmatch (".*[\\\\/](.+?)[\\\\/]?", path)

    return match[1] if match else path

search_path = list (filter (
    bool,
    os.environ.get ("PERFORMANCE_LOG_VIEWER_PATH", ".").split (":")
))

editor_command  = os.environ.get ("PERFORMANCE_LOG_VIEWER_EDITOR",
                                  "xdg-open {file}")
editor_command += " &"

def find_file (filename):
    def lookup (filename):
        filename = re.sub ("[\\\\/]", GLib.DIR_SEPARATOR_S, filename)

        if GLib.path_is_absolute (filename):
            file = Gio.File.new_for_path (filename)

            if file.query_exists ():
                return file

        for path in search_path:
            rest = filename

            while rest:
                file = Gio.File.new_for_path (GLib.build_filenamev ((path, rest)))

                if file.query_exists ():
                    return file

                sep = rest.find (GLib.DIR_SEPARATOR_S)

                rest = rest[sep + 1:] if sep >= 0 else ""

        return None

    if filename not in find_file.cache:
        find_file.cache[filename] = lookup (filename)

    return find_file.cache[filename]

find_file.cache = {}

def run_editor (file, line):
    subprocess.call (editor_command.format (file = "\"%s\"" % file.get_path (),
                                            line = line),
                     shell = True)

VariableType = namedtuple ("VariableType",
                           ("parse", "format", "format_numeric"))

var_types = {
    "boolean": VariableType (
        parse          = int,
        format         = compose (str, bool),
        format_numeric = format_float
    ),

    "integer": VariableType (
        parse          = int,
        format         = format_float,
        format_numeric = None
    ),

    "size": VariableType (
        parse          = int,
        format         = format_size,
        format_numeric = None
    ),

    "size-ratio": VariableType (
        parse          = lambda x: div (*map (int, x.split ("/"))),
        format         = format_percentage,
        format_numeric = None
    ),

    "int-ratio": VariableType (
        parse          = lambda x: div (*map (int, x.split (":"))),
        format         = lambda x: "%g:%g" % (
                             (0, 0)                     if math.isnan (x) else
                             (1, 0)                     if x == math.inf  else
                             (-1, 0)                    if x == -math.inf else
                             (0, 1)                     if x == 0         else
                             (round (100 * x) / 100, 1) if abs (x) >  1   else
                             (1, round (100 / x) / 100)
                         ),
        format_numeric = None
    ),

    "percentage": VariableType (
        parse          = float,
        format         = format_percentage,
        format_numeric = None
    ),

    "duration": VariableType (
        parse          = float,
        format         = format_duration,
        format_numeric = None
    ),

    "rate-of-change": VariableType (
        parse          = float,
        format         = lambda x: "%s/s" % format_size (x),
        format_numeric = None
    )
}
var_types = {
    type: VariableType (
        parse          = parse,
        format         = lambda x, f = format: \
                             f (x) if x is not None else "N/A",
        format_numeric = lambda x, f = format_numeric or format:
                             f (x) if x is not None else "N/A"
    )
    for type, (parse, format, format_numeric) in var_types.items ()
}

# Read performance log from STDIN
log = ElementTree.fromstring (sys.stdin.buffer.read ())

Variable = namedtuple ("Variable", ("type", "desc", "color"))
Value    = namedtuple ("Value",    ("value", "raw"))

var_colors = [
    (0.8, 0.4, 0.4),
    (0.8, 0.6, 0.4),
    (0.4, 0.8, 0.4),
    (0.8, 0.8, 0.4),
    (0.4, 0.4, 0.8),
    (0.4, 0.8, 0.8),
    (0.8, 0.4, 0.8),
    (0.8, 0.8, 0.8)
]

var_defs = {}

for var in log.find ("var-defs"):
    color = var_colors[len (var_defs) % len (var_colors)]

    var_defs[var.get ("name")] = Variable (var.get ("type"),
                                           var.get ("desc"),
                                           color)

AddressInfo = namedtuple ("AddressInfo", ("id",
                                          "name",
                                          "object",
                                          "symbol",
                                          "offset",
                                          "source",
                                          "line"))

address_map = {}

if log.find ("address-map"):
    for address in log.find ("address-map").iterfind ("address"):
        value = int (address.get ("value"), 0)

        object = address.find ("object").text
        symbol = address.find ("symbol").text
        base   = address.find ("base").text
        source = address.find ("source").text
        line   = address.find ("line").text

        address_map[value] = AddressInfo (
            id     = int (base, 0) if base else value,
            name   = symbol or base or hex (value),
            object = object,
            symbol = symbol,
            offset = value - int (base, 0) if base else None,
            source = source,
            line   = int (line) if line else None
        )

class ThreadState (enum.Enum):
    SUSPENDED = enum.auto ()
    RUNNING   = enum.auto ()

    def __str__ (self):
        return {
            ThreadState.SUSPENDED: "S",
            ThreadState.RUNNING:   "R"
        }[self]

Thread = namedtuple ("Thread", ("id", "name", "state", "frames"))
Frame  = namedtuple ("Frame",  ("id", "address", "info"))

Sample = namedtuple ("Sample", ("t", "vars", "markers", "backtrace"))
Marker = namedtuple ("Marker", ("id", "t", "description"))

samples     = []
markers     = []
last_marker = 0

for element in log.find ("samples"):
    if element.tag == "sample":
        sample = Sample (
            t         = int (element.get ("t")),
            vars      = {},
            markers   = markers[last_marker:],
            backtrace = []
        )

        for var in element.find ("vars"):
            sample.vars[var.tag] = Value (
                value = var_types[var_defs[var.tag].type].parse (var.text) \
                        if var.text else None,
                raw   = var.text.strip () if var.text else None
            )

        if element.find ("backtrace"):
            for thread in element.find ("backtrace").iterfind ("thread"):
                id      = thread.get ("id")
                name    = thread.get ("name")
                running = thread.get ("running")

                t = Thread (
                    id     = int (id),
                    name   = name,
                    state  = ThreadState.RUNNING if running and int (running) \
                             else ThreadState.SUSPENDED,
                    frames = []
                )

                for frame in thread.iterfind ("frame"):
                    address = int (frame.get ("address"), 0)

                    info = address_map.get (address, None)

                    if not info:
                        info = AddressInfo (
                            id     = address,
                            name   = hex (address),
                            object = None,
                            symbol = None,
                            offset = None,
                            source = None,
                            line   = None
                        )

                    t.frames.append (Frame (
                        id      = len (t.frames),
                        address = address,
                        info    = info
                    ))

                sample.backtrace.append (t)

        samples.append (sample)

        last_marker = len (markers)
    elif element.tag == "marker":
        marker = Marker (
            id          = int (element.get ("id")),
            t           = int (element.get ("t")),
            description = element.text.strip () if element.text else None
        )

        markers.append (marker)

if samples:
    samples[-1].markers.extend (markers[last_marker:])

DELTA_SAME = __builtins__.object ()

def delta_encode (dest, src):
    if type (dest) == type (src):
        if dest == src:
            return DELTA_SAME
        elif type (dest) == tuple:
            return tuple (delta_encode (d, s) for d, s in zip (dest, src)) + \
                   dest[len (src):]

    return dest

def delta_decode (dest, src):
    if dest == DELTA_SAME:
        return src
    elif type (dest) == type (src):
        if type (dest) == tuple:
            return tuple (delta_decode (d, s) for d, s in zip (dest, src)) + \
                   dest[len (src):]

    return dest

class History (GObject.GObject):
    Source = namedtuple ("HistorySource", ("get", "set"))

    def __init__ (self):
        GObject.GObject.__init__ (self)

        self.sources = []

        self.state = None

        self.undo_stack = []
        self.redo_stack = []

        self.blocked         = 0
        self.n_groups        = 0
        self.pending_record  = False

    @GObject.Property (type = bool, default = False)
    def can_undo (self):
        return bool (self.undo_stack)

    @GObject.Property (type = bool, default = False)
    def can_redo (self):
        return bool (self.redo_stack)

    def add_source (self, get, set):
        self.sources.append (self.Source (get, set))

    def block (self):
        self.blocked += 1

    def unblock (self):
        self.blocked -= 1

    def is_blocked (self):
        return self.blocked > 0

    def start_group (self):
        self.n_groups += 1

    def end_group (self):
        self.n_groups -= 1

        if self.n_groups == 0 and self.pending_record:
            self.record ()

    def record (self):
        if self.is_blocked ():
            return

        if self.n_groups == 0:
            state = tuple (source.get () for source in self.sources)

            if self.state is None:
                self.state = state
            else:
                self.pending_record = False

                delta = delta_encode (self.state, state)

                if delta == DELTA_SAME:
                    return

                self.undo_stack.append (delta_encode (self.state, state))
                self.redo_stack = []

                self.state = state

                self.notify ("can-undo")
                self.notify ("can-redo")
        else:
            self.pending_record = True

    def update (self):
        if self.is_blocked ():
            return

        if self.n_groups == 0:
            state = tuple (source.get () for source in self.sources)

            for stack in self.undo_stack, self.redo_stack:
                if stack:
                    stack[-1] = delta_encode (delta_decode (stack[-1],
                                                            self.state),
                                              state)

            self.state = state
        else:
            self.pending_record = True

    def move (self, src, dest):
        self.block ()

        state = src.pop ()

        for source, substate, prev_substate in \
            zip (self.sources, self.state, state):
            if prev_substate != DELTA_SAME:
                source.set (delta_decode (prev_substate, substate))

        state = delta_decode (state, self.state)

        dest.append (delta_encode (self.state, state))

        self.state = state

        self.notify ("can-undo")
        self.notify ("can-redo")

        self.unblock ()

    def undo (self):
        self.move (self.undo_stack, self.redo_stack)

    def redo (self):
        self.move (self.redo_stack, self.undo_stack)

history = History ()

class SelectionOp (enum.Enum):
    REPLACE   = enum.auto ()
    ADD       = enum.auto ()
    SUBTRACT  = enum.auto ()
    INTERSECT = enum.auto ()
    XOR       = enum.auto ()

class Selection (GObject.GObject):
    __gsignals__ = {
        "changed":           (GObject.SignalFlags.RUN_FIRST, None, ()),
        "change-complete":   (GObject.SignalFlags.RUN_FIRST, None, ()),
        "highlight-changed": (GObject.SignalFlags.RUN_FIRST, None, ())
    }

    def __init__ (self, iter = ()):
        GObject.GObject.__init__ (self)

        self.selection  = set (iter)
        self.highlight  = None
        self.cursor     = None
        self.cursor_dir = 0

        self.pending_change_completion = False

    def __eq__ (self, other):
        return type (self)     == type (other)     and \
               self.selection  == other.selection  and \
               self.cursor     == other.cursor     and \
               self.cursor_dir == other.cursor_dir

    def __str__ (self):
        n_sel = len (self.selection)

        if n_sel == 0 or n_sel == len (samples):
            return "All Samples"
        elif n_sel == 1:
            i, = self.selection

            return "Sample %d" % i
        else:
            sel = list (self.selection)

            sel.sort ()

            if all (sel[i] + 1 == sel[i + 1] for i in range (n_sel - 1)):
                return "Samples %d–%d" % (sel[0], sel[-1])
            else:
                return "%d Samples" % n_sel

    def copy (self):
        selection = Selection ()

        selection.highlight  = self.highlight
        selection.cursor     = self.cursor
        selection.cursor_dir = self.cursor_dir
        selection.selection  = self.selection.copy ()

        return selection

    def get_effective_selection (self):
        if self.selection:
            return self.selection
        else:
            return set (range (len (samples)))

    def select (self, selection, op = SelectionOp.REPLACE):
        if op == SelectionOp.REPLACE:
            self.selection = selection.copy ()
        elif op == SelectionOp.ADD:
            self.selection |= selection
        elif op == SelectionOp.SUBTRACT:
            self.selection -= selection
        elif op == SelectionOp.INTERSECT:
            self.selection &= selection
        elif op == SelectionOp.XOR:
            self.selection.symmetric_difference_update (selection)

        if len (self.selection) == 1:
            (self.cursor,) = self.selection
        else:
            self.cursor = None

        self.cursor_dir = 0

        self.pending_change_completion = True

        self.emit ("changed")

    def select_range (self, first, last, op = SelectionOp.REPLACE):
        if first > last:
            temp  = first
            first = last
            last  = temp

        first = max (first, 0)
        last  = min (last, len (samples) - 1)

        if first <= last:
            self.select (set (range (first, last + 1)), op)
        else:
            self.select (set (), op)

    def clear (self):
        self.select (set ())

    def invert (self):
        self.select_range (0, len (samples), SelectionOp.XOR)

    def change_complete (self):
        if self.pending_change_completion:
            self.pending_change_completion = False

            history.start_group ()

            history.record ()

            self.emit ("change-complete")

            history.end_group ()

    def set_highlight (self, highlight):
        self.highlight = highlight

        self.emit ("highlight-changed")

    def source_get (self):
        return self.copy ()

    def source_set (self, selection):
        self.cursor     = selection.cursor
        self.cursor_dir = selection.cursor_dir
        self.selection  = selection.selection.copy ()

        self.emit ("changed")
        self.emit ("change-complete")

    def add_history_source (self):
        history.add_source (self.source_get, self.source_set)

selection = Selection ()
selection.add_history_source ()

class FindSamplesPopover (Gtk.Popover):
    def __init__ (self, *args, **kwargs):
        Gtk.Popover.__init__ (self, *args, **kwargs)

        vbox = Gtk.Box (orientation  = Gtk.Orientation.VERTICAL,
                        border_width = 20,
                        spacing      = 8)
        self.add (vbox)
        vbox.show ()

        entry = Gtk.Entry (width_chars      = 40,
                           placeholder_text = "Python expression")
        self.entry = entry
        vbox.pack_start (entry, False, False, 0)
        entry.show ()

        entry.connect ("activate", self.find_samples)

        entry.get_buffer ().connect (
            "notify::text",
            lambda *args: self.entry.get_style_context ().remove_class ("error")
        )

        frame = Gtk.Frame (label       = "Selection",
                           shadow_type = Gtk.ShadowType.NONE)
        vbox.pack_start (frame, False, False, 8)
        frame.get_label_widget ().get_style_context ().add_class ("dim-label")
        frame.show ()

        vbox2 = Gtk.Box (orientation  = Gtk.Orientation.VERTICAL,
                         border_width = 8,
                         spacing      = 8)
        frame.add (vbox2)
        vbox2.show ()

        self.radios = []

        radio = Gtk.RadioButton.new_with_mnemonic (None, "_Replace")
        self.radios.append ((radio, SelectionOp.REPLACE))
        vbox2.pack_start (radio, False, False, 0)
        radio.show ()

        radio = Gtk.RadioButton.new_with_mnemonic_from_widget (radio, "_Add")
        self.radios.append ((radio, SelectionOp.ADD))
        vbox2.pack_start (radio, False, False, 0)
        radio.show ()

        radio = Gtk.RadioButton.new_with_mnemonic_from_widget (radio, "_Subtract")
        self.radios.append ((radio, SelectionOp.SUBTRACT))
        vbox2.pack_start (radio, False, False, 0)
        radio.show ()

        radio = Gtk.RadioButton.new_with_mnemonic_from_widget (radio, "_Intersect")
        self.radios.append ((radio, SelectionOp.INTERSECT))
        vbox2.pack_start (radio, False, False, 0)
        radio.show ()

        button = Gtk.Button.new_with_mnemonic ("_Find")
        vbox.pack_start (button, False, False, 0)
        button.set_halign (Gtk.Align.CENTER)
        button.show ()

        button.connect ("clicked", self.find_samples)

    def do_hide (self):
        self.entry.set_text ("")
        self.entry.get_style_context ().remove_class ("error")

        Gtk.Popover.do_hide (self)

    def find_samples (self, *args):
        def var_name (var):
            return var.replace ("-", "_")

        try:
            f = eval ("lambda thread, function, %s: %s" % (
                ", ".join (map (var_name, var_defs)),
                self.entry.get_text ()))
        except:
            self.entry.get_style_context ().add_class ("error")

            return

        sel = set ()

        for i in range (len (samples)):
            try:
                def match_thread (thread, id, state = None):
                    return (id is None or                         \
                            (type (id) == int                 and \
                             id == thread.id)                 or  \
                            (type (id) == str                 and \
                             thread.name                      and \
                             re.fullmatch (id, thread.name))) and \
                           (state is None                     or  \
                            re.fullmatch (state, str (thread.state)))

                def thread (id, state = None):
                    return any (match_thread (thread, id, state)
                                for thread in samples[i].backtrace or [])

                def function (name, id = None, state = None):
                    for thread in samples[i].backtrace or []:
                        if match_thread (thread, id, state):
                            for frame in thread.frames:
                                if re.fullmatch (name, frame.info.name):
                                    return True

                    return False

                if f (thread, function, **{
                    var_name (var): value.value
                    for var, value in samples[i].vars.items ()
                }):
                    sel.add (i)
            except:
                pass

        op = [op for radio, op in self.radios if radio.get_active ()][0]

        selection.select (sel, op)

        selection.change_complete ()

        self.hide ()

class CellRendererColorToggle (Gtk.CellRendererToggle):
    padding = 3

    color = GObject.Property (type = Gdk.RGBA, default = Gdk.RGBA (0, 0, 0))

    def do_render (self, cr, widget, background_area, cell_area, flags):
        state    = widget.get_state_flags ()
        style    = widget.get_style_context ()
        fg_color = style.get_color (state)
        active   = self.get_property ("active")
        size     = max (min (cell_area.width, cell_area.height) -
                        2 * self.padding,
                        0)

        (r, g, b, a) = self.color

        if is_bright_color (fg_color):
            bg = (0.75 * r, 0.75 * g, 0.75 * b)
            fg = (r, g, b)
        else:
            bg = (r, g, b)
            fg = (0.75 * r, 0.75 * g, 0.75 * b)

        x = cell_area.x + (cell_area.width  - size) // 2
        y = cell_area.y + (cell_area.height - size) // 2

        if active:
            cr.rectangle (x, y, size, size)

            cr.set_source_rgba (*bg)
            cr.fill ()
        else:
            style.save ()

            style.set_state (Gtk.StateFlags (state & ~Gtk.StateFlags.SELECTED))
            Gtk.render_background (style, cr, x, y, size, size)

            style.restore ()

        cr.rectangle (x, y, size, size)

        cr.set_source_rgb (*fg)
        cr.set_line_width (2)
        cr.stroke ()

class VariableSet (Gtk.TreeView):
    class Store (Gtk.ListStore):
        NAME   = 0
        DESC   = 1
        COLOR  = 2
        ACTIVE = 3

        def __init__ (self):
            Gtk.ListStore.__init__ (self, str, str, Gdk.RGBA, bool)

            for var, var_def in var_defs.items ():
                i = self.append ((var,
                                  var_def.desc,
                                  Gdk.RGBA (*var_def.color),
                                  False))

    def __init__ (self, *args, **kwargs):
        Gtk.TreeView.__init__ (self, *args, headers_visible = False, **kwargs)

        store = self.Store ()
        self.store = store
        self.set_model (store)
        self.set_tooltip_column (store.DESC)

        col = Gtk.TreeViewColumn ()
        self.append_column (col)

        cell = CellRendererColorToggle ()
        col.pack_start (cell, False)
        col.add_attribute (cell, "active", store.ACTIVE)
        col.add_attribute (cell, "color", store.COLOR)

        cell.connect ("toggled", self.var_toggled)

        cell = Gtk.CellRendererText ()
        col.pack_start (cell, True)
        col.add_attribute (cell, "text", store.NAME)

    def var_toggled (self, cell, path):
        self.store[path][self.store.ACTIVE] = not cell.get_property ("active")

class SampleGraph (Gtk.DrawingArea):
    def __init__ (self, model = None, *args, **kwargs):
        Gtk.DrawingArea.__init__ (self, *args, can_focus = True, **kwargs)

        self.style_widget = Gtk.Entry ()

        self.model = model

        if model:
            model.connect ("row-changed", lambda *args: self.update ())

            self.update ()

        self.selection = None
        self.sel       = None

        selection.connect ("changed", self.selection_changed)
        selection.connect ("highlight-changed",
                           lambda selection: self.queue_draw ())

        self.add_events (Gdk.EventMask.BUTTON_PRESS_MASK   |
                         Gdk.EventMask.BUTTON_RELEASE_MASK |
                         Gdk.EventMask.KEY_PRESS_MASK      |
                         Gdk.EventMask.KEY_RELEASE_MASK)

        self.selection_changed (selection)

    def sample_to_x (self, i):
        if not samples:
            return None

        width     = self.get_allocated_width ()
        n_samples = max (len (samples), 2)

        return 1 + (width - 3) * i / (n_samples - 1)

    def sample_to_range (self, i):
        if not samples:
            return None

        width     = self.get_allocated_width ()
        n_samples = max (len (samples), 2)

        return (1 + math.floor ((width - 3) * (i - 0.5) / (n_samples - 1)),
                1 + math.ceil  ((width - 3) * (i + 0.5) / (n_samples - 1)))

    def x_to_sample (self, x):
        if not samples:
            return None

        width     = max (self.get_allocated_width (), 4)
        n_samples = len (samples)

        return round ((n_samples - 1) * (x - 1) / (width - 3))

    def update (self):
        if not samples or not self.model:
            return

        self.max_value = 1

        for row in self.model:
            var_name   = row[self.model.NAME]
            var_active = row[self.model.ACTIVE]

            if var_active:
                values = (sample.vars[var_name].value for sample in samples)
                values = filter (lambda x: x is not None, values)
                values = filter (math.isfinite,           values)

                try:
                    self.max_value = max (self.max_value, max (values))
                except:
                    pass

        self.queue_draw ()

    def selection_changed (self, selection):
        if selection.selection:
            self.sel = list (selection.selection)
            self.sel.sort ()
        else:
            self.sel = None

        self.queue_draw ()

    def do_get_preferred_width (self):
        return (300, 300)

    def do_get_preferred_height (self):
        if self.model:
            return (32, 256)
        else:
            return (16, 16)

    def update_selection (self):
        sel = self.selection.copy ()

        i0 = self.selection_i0
        i1 = self.selection_i1

        if self.selection_range:
            swap = i0 > i1

            if swap:
                temp = i0
                i0   = i1
                i1   = temp

            n_samples = len (samples)

            while i0 > 0             and not samples[i0 - 1].markers: i0 -= 1
            while i1 < n_samples - 1 and not samples[i1 + 1].markers: i1 += 1

            if swap:
                temp = i0
                i0   = i1
                i1   = temp

        sel.select_range (i0, i1, self.selection_op)

        selection.select (sel.selection)

        selection.cursor     = i1
        selection.cursor_dir = i1 - i0

    def do_button_press_event (self, event):
        state = event.state & Gdk.ModifierType.MODIFIER_MASK

        self.grab_focus ()

        if event.button == 1:
            i = self.x_to_sample (event.x)

            if i is None:
                return False

            self.selection       = selection.copy ()
            self.selection_i0    = i
            self.selection_i1    = i
            self.selection_op    = SelectionOp.REPLACE
            self.selection_range = event.type != Gdk.EventType.BUTTON_PRESS

            if state == Gdk.ModifierType.SHIFT_MASK:
                self.selection_op = SelectionOp.ADD
            elif state == Gdk.ModifierType.CONTROL_MASK:
                self.selection_op = SelectionOp.SUBTRACT
            elif state == (Gdk.ModifierType.SHIFT_MASK |
                           Gdk.ModifierType.CONTROL_MASK):
                self.selection_op = SelectionOp.INTERSECT

            self.update_selection ()

            self.grab_add ()
        elif event.button == 3:
            if state == 0:
                selection.clear ()
            elif state == Gdk.ModifierType.CONTROL_MASK:
                selection.invert ()

            self.grab_add ()

        return True

    def do_button_release_event (self, event):
        if event.button == 1 or event.button == 3:
            self.selection = None

            selection.change_complete ()

            self.grab_remove ()

            return True

        return False

    def do_motion_notify_event (self, event):
        i = self.x_to_sample (event.x)

        selection.set_highlight (i)

        if self.selection and i is not None:
            self.selection_i1 = i

            self.update_selection ()

            return True

        return False

    def do_leave_notify_event (self, event):
        selection.set_highlight (None)

        return False

    def do_key_press_event (self, event):
        if event.keyval == Gdk.KEY_Left    or \
           event.keyval == Gdk.KEY_Right   or \
           event.keyval == Gdk.KEY_Home    or \
           event.keyval == Gdk.KEY_KP_Home or \
           event.keyval == Gdk.KEY_End     or \
           event.keyval == Gdk.KEY_KP_End:
            n_samples = len (samples)

            state = event.state & Gdk.ModifierType.MODIFIER_MASK

            op = SelectionOp.REPLACE

            if state == Gdk.ModifierType.SHIFT_MASK:
                op = SelectionOp.XOR

            cursor     = selection.cursor
            cursor_dir = selection.cursor_dir

            if event.keyval == Gdk.KEY_Left or \
               event.keyval == Gdk.KEY_Home or \
               event.keyval == Gdk.KEY_KP_Home:
                if selection.cursor is not None:
                    if cursor_dir <= 0 or op == SelectionOp.REPLACE:
                        cursor -= 1
                else:
                    cursor = n_samples - 1

                cursor_dir = -1
            elif event.keyval == Gdk.KEY_Right or \
                 event.keyval == Gdk.KEY_End   or \
                 event.keyval == Gdk.KEY_KP_End:
                if cursor is not None:
                    if cursor_dir >= 0 or op == SelectionOp.REPLACE:
                        cursor += 1
                else:
                    cursor = 0

                cursor_dir = +1

            if cursor < 0 or cursor >= n_samples:
                cursor = min (max (cursor, 0), n_samples - 1)

                selection.cursor     = cursor
                selection.cursor_dir = cursor_dir

                if op != SelectionOp.REPLACE:
                    return True

            i0 = cursor

            if event.keyval == Gdk.KEY_Home or \
               event.keyval == Gdk.KEY_KP_Home:
                cursor = 0
            elif event.keyval == Gdk.KEY_End or \
                 event.keyval == Gdk.KEY_KP_End:
                cursor = n_samples - 1

            if op == SelectionOp.REPLACE:
                i0 = cursor

            selection.select_range (i0, cursor, op)

            if len (selection.selection) > 1:
                selection.cursor     = cursor
                selection.cursor_dir = cursor_dir

            return True
        elif event.keyval == Gdk.KEY_Escape:
            selection.select (set ())

            return True

        return False

    def do_key_release_event (self, event):
        selection.change_complete ()

        return False

    def do_draw (self, cr):
        state           = self.get_state_flags ()
        style           = (self.style_widget if self.model else
                           self).get_style_context ()
        (width, height) = (self.get_allocated_width  (),
                           self.get_allocated_height ())

        fg_color        = tuple (style.get_color (state))
        grid_color      = (*fg_color[:3], 0.25 * fg_color[3])
        highlight_color = grid_color
        selection_color = (*fg_color[:3], 0.15 * fg_color[3])

        Gtk.render_background (style, cr, 0, 0, width, height)

        if self.model:
            max_value = self.max_value
            vscale    = (height - 4) / max_value

            cr.save ()

            cr.translate (0, height - 2)
            cr.scale (1, -1)

            first_sample = True
            has_infinite = False

            for row in self.model:
                var_name   = row[self.model.NAME]
                var_active = row[self.model.ACTIVE]

                if var_active:
                    is_boolean    = var_defs[var_name].type == "boolean"
                    is_continuous = not is_boolean

                    for i in range (len (samples)):
                        value = samples[i].vars[var_name].value

                        if value is not None:
                            first_sample = False

                            if math.isinf (value):
                                first_sample = True
                                has_infinite = True

                                value = max_value
                            elif is_boolean:
                                value *= max_value

                            y = value * vscale

                            if is_continuous:
                                x = self.sample_to_x (i)

                                if first_sample:
                                    cr.move_to (x, y)
                                else:
                                    cr.line_to (x, y)
                            else:
                                (x0, x1) = self.sample_to_range (i)

                                if first_sample:
                                    cr.move_to (x0, y)
                                else:
                                    cr.line_to (x0, y)

                                cr.line_to (x1, y)
                        else:
                            first_sample = True

                    (r, g, b) = var_defs[var_name].color

                    cr.set_source_rgb (r, g, b)
                    cr.set_line_width (2)
                    cr.stroke ()

                    if has_infinite:
                        cr.save ()

                        for i in range (len (samples)):
                            value = samples[i].vars[var_name].value

                            if value is not None and math.isinf (value):
                                first_sample = False

                                y = max_value * vscale

                                if is_continuous:
                                    x = self.sample_to_x (i)

                                    if first_sample:
                                        cr.move_to (x, y)
                                    else:
                                        cr.line_to (x, y)
                                else:
                                    (x0, x1) = self.sample_to_range (i)

                                    if first_sample:
                                        cr.move_to (x0, y)
                                    else:
                                        cr.line_to (x0, y)

                                    cr.line_to (x1, y)
                            else:
                                first_sample = True

                        cr.set_dash ([6, 6], 0)
                        cr.stroke ()

                        cr.restore ()

            cr.restore ()

            cr.set_line_width (1)

            cr.set_source_rgba (*grid_color)

            n_hgrid_lines = 4
            n_vgrid_lines = 1

            for i in range (n_hgrid_lines + 1):
                cr.move_to (0, round (i * (height - 1) / n_hgrid_lines) + 0.5)
                cr.rel_line_to (width, 0)

                cr.stroke ()

            for i in range (n_vgrid_lines + 1):
                cr.move_to (round (i * (width - 1) / n_vgrid_lines) + 0.5, 0)
                cr.rel_line_to (0, height)

                cr.stroke ()
        else:
            for i in range (len (samples)):
                if samples[i].markers:
                    (x0, x1) = self.sample_to_range (i)

                    cr.rectangle (x0, 0, x1 - x0, height)

            cr.set_source_rgba (*fg_color)
            cr.fill ()

        if selection.highlight is not None:
            (x0, x1) = self.sample_to_range (selection.highlight)

            cr.rectangle (x0, 0, x1 - x0, height)

            (r, g, b, a) = style.get_color (state)

            cr.set_source_rgba (*highlight_color)
            cr.fill ()

        if self.sel:
            def draw_selection ():
                x0 = self.sample_to_range (i0)[0]
                x1 = self.sample_to_range (i1)[1]

                cr.rectangle (x0, 0, x1 - x0, height)

                (r, g, b, a) = style.get_color (state)

                cr.set_source_rgba (*selection_color)
                cr.fill ()

            i0 = None

            for i in self.sel:
                if i0 is None:
                    i0 = i
                    i1 = i
                elif i == i1 + 1:
                    i1 = i
                else:
                    draw_selection ()

                    i0 = i
                    i1 = i

            if i0 is not None:
                draw_selection ()

class SampleGraphList (Gtk.Box):
    Item = namedtuple (
        "SampleGraphListGraph", ("widget",
                                 "model",
                                 "remove_button",
                                 "move_up_button",
                                 "move_down_button")
    )

    def __init__ (self, *args, **kwargs):
        Gtk.Box.__init__ (self,
                          *args,
                          orientation = Gtk.Orientation.VERTICAL,
                          **kwargs)

        self.items = []

        self.vset_size_group = Gtk.SizeGroup (
            mode = Gtk.SizeGroupMode.HORIZONTAL
        )

        hbox = Gtk.Box (orientation = Gtk.Orientation.HORIZONTAL)
        self.pack_start (hbox, False, False, 0)
        hbox.show ()

        empty = Gtk.DrawingArea ()
        hbox.pack_start (empty, False, True, 0)
        self.vset_size_group.add_widget (empty)
        empty.show ()

        graph = SampleGraph (has_tooltip = True)
        hbox.pack_start (graph, True, True, 0)
        graph.show ()

        graph.connect ("query-tooltip", self.graph_query_tooltip)

        separator = Gtk.Separator (orientation = Gtk.Orientation.HORIZONTAL)
        self.pack_start (separator, False, False, 0)
        separator.show ()

        vbox = Gtk.Box (orientation = Gtk.Orientation.VERTICAL)
        self.items_vbox = vbox
        self.pack_start (vbox, False, False, 0)
        vbox.show ()

        self.add_item (0)

    def update_items (self):
        for widget in self.items_vbox.get_children ():
            self.items_vbox.remove (widget)

        i = 0

        for item in self.items:
            if i > 0:
                separator = Gtk.Separator (
                    orientation = Gtk.Orientation.HORIZONTAL
                )
                self.items_vbox.pack_start (separator, False, False, 0)
                separator.show ()

            self.items_vbox.pack_start (item.widget, False, False, 0)

            item.remove_button.set_sensitive    (len (self.items) > 1)
            item.move_up_button.set_sensitive   (i > 0)
            item.move_down_button.set_sensitive (i < len (self.items) - 1)

            i += 1

    def add_item (self, i):
        hbox = Gtk.Box (orientation = Gtk.Orientation.HORIZONTAL)
        hbox.show ()

        vbox = Gtk.Box (orientation = Gtk.Orientation.VERTICAL)
        hbox.pack_start (vbox, False, True, 0)
        self.vset_size_group.add_widget (vbox)
        vbox.show ()

        scroll = Gtk.ScrolledWindow (
            hscrollbar_policy = Gtk.PolicyType.NEVER,
            vscrollbar_policy = Gtk.PolicyType.AUTOMATIC
        )
        vbox.pack_start (scroll, True, True, 0)
        scroll.show ()

        vset = VariableSet ()
        scroll.add (vset)
        vset.show ()

        buttons = Gtk.ButtonBox (orientation = Gtk.Orientation.HORIZONTAL)
        vbox.pack_start (buttons, False, False, 0)
        buttons.set_layout (Gtk.ButtonBoxStyle.EXPAND)
        buttons.show ()

        button = Gtk.Button.new_from_icon_name ("list-add-symbolic",
                                                Gtk.IconSize.BUTTON)
        add_button = button
        buttons.add (button)
        button.show ()

        button = Gtk.Button.new_from_icon_name ("list-remove-symbolic",
                                                Gtk.IconSize.BUTTON)
        remove_button = button
        buttons.add (button)
        button.show ()

        button = Gtk.Button.new_from_icon_name ("go-up-symbolic",
                                                Gtk.IconSize.BUTTON)
        move_up_button = button
        buttons.add (button)
        button.show ()

        button = Gtk.Button.new_from_icon_name ("go-down-symbolic",
                                                Gtk.IconSize.BUTTON)
        move_down_button = button
        buttons.add (button)
        button.show ()

        graph = SampleGraph (vset.get_model (), has_tooltip = True)
        hbox.pack_start (graph, True, True, 0)
        graph.show ()

        graph.connect ("query-tooltip", self.graph_query_tooltip)

        item = self.Item (
            widget           = hbox,
            model            = vset.get_model (),
            remove_button    = remove_button,
            move_up_button   = move_up_button,
            move_down_button = move_down_button
        )
        self.items.insert (i, item)

        add_button.connect       ("clicked",
                                  lambda *args: self.add_item (
                                                    self.items.index (item) + 1
                                                ))
        remove_button.connect    ("clicked",
                                  lambda *args: self.remove_item (
                                                    self.items.index (item)
                                                ))
        move_up_button.connect   ("clicked",
                                  lambda *args: self.move_item (
                                                    self.items.index (item),
                                                    -1
                                                ))
        move_down_button.connect ("clicked",
                                  lambda *args: self.move_item (
                                                    self.items.index (item),
                                                    +1
                                                ))

        self.update_items ()

    def remove_item (self, i):
        del self.items[i]

        self.update_items ()

    def move_item (self, i, offset):
        item = self.items[i]
        del self.items[i]
        self.items.insert (i + offset, item)

        self.update_items ()

    def graph_query_tooltip (self, graph, x, y, keyboard_mode, tooltip):
        if keyboard_mode:
            return False

        i = graph.x_to_sample (x)

        if i is None or i < 0 or i >= len (samples):
            return False

        grid = Gtk.Grid (column_spacing = 4)
        tooltip.set_custom (grid)
        grid.show ()

        row = 0

        label = Gtk.Label ()
        grid.attach (label, 0, row, 2, 1)
        label.set_markup ("<b>Sample %d</b>" % i)
        label.show ()

        row += 1

        label = Gtk.Label ()
        grid.attach (label, 0, row, 2, 1)
        label.set_markup ("<sub>%s</sub>" %
                          format_duration (samples[i].t / 1000000))
        label.get_style_context ().add_class ("dim-label")
        label.show ()

        row += 1

        for item in self.items:
            model = item.model

            vars = tuple (var[model.NAME] for var in model if var[model.ACTIVE])

            if not vars:
                continue

            separator = Gtk.Separator (orientation = Gtk.Orientation.HORIZONTAL)
            grid.attach (separator, 0, row, 2, 1)
            separator.show ()

            row += 1

            for var in vars:
                color = format_color (var_defs[var].color)

                label = Gtk.Label (halign = Gtk.Align.START)
                grid.attach (label, 0, row, 1, 1)
                label.set_markup (
                    "<span color=\"%s\"><b>%s</b></span>" % (color, var)
                )
                label.show ()

                value = samples[i].vars[var].value
                text  = var_types[var_defs[var].type].format (value) \
                        if value is not None else "N/A"

                label = Gtk.Label (label = text, halign = Gtk.Align.END)
                grid.attach (label, 1, row, 1, 1)
                label.show ()

                row += 1

        markers = samples[i].markers

        if markers:
            separator = Gtk.Separator (orientation = Gtk.Orientation.HORIZONTAL)
            grid.attach (separator, 0, row, 2, 1)
            separator.show ()

            row += 1

            for marker in markers:
                label = Gtk.Label (halign = Gtk.Align.START)
                grid.attach (label, 0, row, 1, 1)
                label.set_markup ("<b>Marker %d</b>" % (marker.id))
                label.show ()

                if marker.description:
                    label = Gtk.Label (marker.description,
                                       halign = Gtk.Align.END)
                    grid.attach (label, 1, row, 1, 1)
                    label.show ()

                row += 1

        return True

class InformationViewer (Gtk.ScrolledWindow):
    class Store (Gtk.ListStore):
        NAME  = 0
        VALUE = 1

        def __init__ (self):
            Gtk.ListStore.__init__ (self, str, str)

    def __init__ (self, *args, **kwargs):
        Gtk.ScrolledWindow.__init__ (
            self,
            *args,
            hscrollbar_policy = Gtk.PolicyType.AUTOMATIC,
            vscrollbar_policy = Gtk.PolicyType.AUTOMATIC,
            **kwargs
        )

        vbox = Gtk.Box (orientation  = Gtk.Orientation.VERTICAL,
                        border_width = 32,
                        margin_left  = 64,
                        margin_right = 64,
                        spacing      = 32)
        self.add (vbox)
        vbox.show ()

        def add_element (element):
            name    = {
                "params":       "Log Parameters",
                "gimp-version": "GIMP Version",
                "env":          "Environment",
                "gegl-config":  "GEGL Config"
            }.get (element.tag, element.tag)
            text    = element.text.strip ()
            n_items = len (element)

            if not text and n_items == 0:
                return

            vbox2 = Gtk.Box (orientation = Gtk.Orientation.VERTICAL,
                             spacing     = 16)
            vbox.pack_start (vbox2, False, False, 0)
            vbox2.show ()

            label = Gtk.Label (xalign = 0)
            vbox2.pack_start (label, False, False, 0)
            label.set_markup ("<b>%s</b>" % name)
            label.show ()

            frame = Gtk.Frame (shadow_type = Gtk.ShadowType.IN)
            vbox2.pack_start (frame, False, False, 0)
            frame.show ()

            if text:
                scrolled = Gtk.ScrolledWindow (
                    hscrollbar_policy = Gtk.PolicyType.AUTOMATIC,
                    vscrollbar_policy = Gtk.PolicyType.AUTOMATIC,
                    height_request    = 400
                )
                frame.add (scrolled)
                scrolled.show ()

                text = Gtk.TextView (editable      = False,
                                     monospace     = True,
                                     wrap_mode     = Gtk.WrapMode.WORD,
                                     left_margin   = 16,
                                     right_margin  = 16,
                                     top_margin    = 16,
                                     bottom_margin = 16)
                scrolled.add (text)
                text.get_buffer ().set_text (element.text.strip (), -1)
                text.show ()
            else:
                scrolled = Gtk.ScrolledWindow (
                    hscrollbar_policy = Gtk.PolicyType.AUTOMATIC,
                    vscrollbar_policy = Gtk.PolicyType.NEVER
                )
                frame.add (scrolled)
                scrolled.show ()

                store = self.Store ()

                for item in element:
                    store.append ((item.tag, item.text.strip ()))

                tree = Gtk.TreeView (model = store)
                scrolled.add (tree)
                tree.show ()

                col = Gtk.TreeViewColumn (title = "Name")
                tree.append_column (col)

                cell = Gtk.CellRendererText ()
                col.pack_start (cell, False)
                col.add_attribute (cell, "text", store.NAME)

                col = Gtk.TreeViewColumn (title = "Value")
                tree.append_column (col)
                col.set_alignment (0.5)

                cell = Gtk.CellRendererText (xalign = 1)
                col.pack_start (cell, False)
                col.add_attribute (cell, "text", store.VALUE)

        params = log.find ("params")

        if params:
            add_element (params)

        info = log.find ("info")

        if info:
            for element in info:
                add_element (element)

class MarkersViewer (Gtk.ScrolledWindow):
    class Store (Gtk.ListStore):
        ID   = 0
        TIME = 1
        DESC = 2

        def __init__ (self):
            Gtk.ListStore.__init__ (self, int, GObject.TYPE_INT64, str)

            for marker in markers:
                self.append ((marker.id, marker.t, marker.description))

    def __init__ (self, *args, **kwargs):
        Gtk.Box.__init__ (self,
                          *args,
                          hscrollbar_policy = Gtk.PolicyType.AUTOMATIC,
                          vscrollbar_policy = Gtk.PolicyType.AUTOMATIC,
                          **kwargs)

        self.needs_update = True

        store = self.Store ()
        self.store = store

        tree = Gtk.TreeView (model = store)
        self.tree = tree
        self.add (tree)
        tree.show ()

        tree.get_selection ().set_mode (Gtk.SelectionMode.MULTIPLE)

        self.tree_selection_changed_handler = tree.get_selection ().connect (
            "changed", self.tree_selection_changed
        )

        col = Gtk.TreeViewColumn (title = "#")
        tree.append_column (col)
        col.set_resizable (True)

        cell = Gtk.CellRendererText (xalign = 1)
        col.pack_start (cell, False)
        col.add_attribute (cell, "text", store.ID)

        def format_time_col (tree_col, cell, model, iter, col):
            time = model[iter][col]

            cell.set_property ("text", format_duration (time / 1000000))

        col = Gtk.TreeViewColumn (title = "Time")
        tree.append_column (col)
        col.set_resizable (True)
        col.set_alignment (0.5)

        cell = Gtk.CellRendererText (xalign = 1)
        col.pack_start (cell, False)
        col.set_cell_data_func (cell, format_time_col, store.TIME)

        col = Gtk.TreeViewColumn (title = "Description")
        tree.append_column (col)
        col.set_resizable (True)
        col.set_alignment (0.5)

        cell = Gtk.CellRendererText ()
        col.pack_start (cell, False)
        col.add_attribute (cell, "text", store.DESC)

        col = Gtk.TreeViewColumn ()
        tree.append_column (col)

        selection.connect ("change-complete", self.selection_change_complete)

    def update (self):
        markers = set ()

        if not self.needs_update:
            return

        self.needs_update = False

        for i in selection.selection:
            markers.update (marker.id for marker in samples[i].markers)

        tree_sel = self.tree.get_selection ()

        GObject.signal_handler_block (tree_sel,
                                      self.tree_selection_changed_handler)

        tree_sel.unselect_all ()

        for row in self.store:
            if row[self.store.ID] in markers:
                tree_sel.select_iter (row.iter)

        GObject.signal_handler_unblock (tree_sel,
                                        self.tree_selection_changed_handler)

    def do_map (self):
        self.update ()

        Gtk.ScrolledWindow.do_map (self)

    def selection_change_complete (self, selection):
        self.needs_update = True

        if self.get_mapped ():
            self.update ()

    def tree_selection_changed (self, tree_sel):
        sel = set ()

        for row in self.store:
            if tree_sel.iter_is_selected (row.iter):
                id = row[self.store.ID]

                for i in range (len (samples)):
                    if any (marker.id == id for marker in samples[i].markers):
                        sel.add (i)

        selection.select (sel)

        selection.change_complete ()

class VariablesViewer (Gtk.ScrolledWindow):
    class Store (Gtk.ListStore):
        NAME        =  0
        DESC        =  1
        COLOR       =  2
        VALUE       =  3
        RAW         =  4
        MIN         =  5
        MAX         =  6
        MEDIAN      =  7
        MEAN        =  8
        STDEV       =  9
        LAST_COLUMN = 10

        def __init__ (self):
            n_stats = self.LAST_COLUMN - self.COLOR

            Gtk.ListStore.__init__ (self,
                                    *((str, str, Gdk.RGBA) + n_stats * (str,)))

            for var, var_def in var_defs.items ():
                 self.append (((var,
                                var_def.desc,
                                Gdk.RGBA (*var_def.color)) +
                               n_stats * ("",)))

    def __init__ (self, *args, **kwargs):
        Gtk.Box.__init__ (self,
                          *args,
                          hscrollbar_policy = Gtk.PolicyType.AUTOMATIC,
                          vscrollbar_policy = Gtk.PolicyType.AUTOMATIC,
                          **kwargs)

        self.needs_update = True

        store = self.Store ()
        self.store = store

        tree = Gtk.TreeView (model = store)
        self.add (tree)
        tree.set_tooltip_column (store.DESC)
        tree.show ()

        self.single_sample_cols = []
        self.multi_sample_cols  = []

        col = Gtk.TreeViewColumn (title = "Variable")
        tree.append_column (col)
        col.set_resizable (True)

        cell = CellRendererColorToggle (active = True)
        col.pack_start (cell, False)
        col.add_attribute (cell, "color", store.COLOR)

        cell = Gtk.CellRendererText ()
        col.pack_start (cell, False)
        col.add_attribute (cell, "text", store.NAME)

        def add_value_column (title, column, single_sample):
            col = Gtk.TreeViewColumn (title = title)
            tree.append_column (col)
            col.set_resizable (True)
            col.set_alignment (0.5)

            cell = Gtk.CellRendererText (xalign = 1)
            col.pack_start (cell, False)
            col.add_attribute (cell, "text", column)

            if single_sample:
                self.single_sample_cols.append (col)
            else:
                self.multi_sample_cols.append (col)

        add_value_column ("Value",     store.VALUE,  True)
        add_value_column ("Raw",       store.RAW,    True)
        add_value_column ("Min",       store.MIN,    False)
        add_value_column ("Max",       store.MAX,    False)
        add_value_column ("Median",    store.MEDIAN, False)
        add_value_column ("Mean",      store.MEAN,   False)
        add_value_column ("Std. Dev.", store.STDEV,  False)

        col = Gtk.TreeViewColumn ()
        tree.append_column (col)

        selection.connect ("change-complete", self.selection_change_complete)

    def update (self):
        if not self.needs_update:
            return

        self.needs_update = False

        sel   = selection.get_effective_selection ()
        n_sel = len (sel)

        if n_sel == 1:
            i, = sel

            for row in self.store:
                var_name = row[self.store.NAME]

                var      = samples[i].vars[var_name]
                var_type = var_types[var_defs[var_name].type]

                row[self.store.VALUE] = var_type.format (var.value)
                row[self.store.RAW]   = var.raw if var.raw is not None \
                                        else "N/A"
        else:
            for row in self.store:
                var_name = row[self.store.NAME]

                var_type = var_types[var_defs[var_name].type]

                vals = (samples[i].vars[var_name].value for i in sel)
                vals = tuple (val for val in vals if val is not None)

                if vals:
                    min_val = min               (vals)
                    max_val = max               (vals)
                    median  = statistics.median (vals)
                    mean    = statistics.mean   (vals)
                    stdev   = statistics.pstdev (vals, mean)

                    row[self.store.MIN]    = var_type.format         (min_val)
                    row[self.store.MAX]    = var_type.format         (max_val)
                    row[self.store.MEDIAN] = var_type.format         (median)
                    row[self.store.MEAN]   = var_type.format_numeric (mean)
                    row[self.store.STDEV]  = var_type.format_numeric (stdev)
                else:
                    row[self.store.MIN]    = \
                    row[self.store.MAX]    = \
                    row[self.store.MEDIAN] = \
                    row[self.store.MEAN]   = \
                    row[self.store.STDEV]  = var_type.format (None)

        for col in self.single_sample_cols: col.set_visible (n_sel == 1)
        for col in self.multi_sample_cols:  col.set_visible (n_sel > 1)

    def do_map (self):
        self.update ()

        Gtk.ScrolledWindow.do_map (self)

    def selection_change_complete (self, selection):
        self.needs_update = True

        if self.get_mapped ():
            self.update ()

class BacktraceViewer (Gtk.Box):
    class ThreadStore (Gtk.ListStore):
        INDEX = 0
        ID    = 1
        NAME  = 2
        STATE = 3

        def __init__ (self):
            Gtk.ListStore.__init__ (self, int, int, str, str)

    class FrameStore (Gtk.ListStore):
        ID       = 0
        ADDRESS  = 1
        OBJECT   = 2
        FUNCTION = 3
        OFFSET   = 4
        SOURCE   = 5
        LINE     = 6

        def __init__ (self):
            Gtk.ListStore.__init__ (self, int, str, str, str, str, str, str)

    class CellRendererViewSource (Gtk.CellRendererPixbuf):
        file = GObject.Property (type = Gio.File, default = None)
        line = GObject.Property (type = int,      default = 0)

        def __init__ (self, *args, **kwargs):
            Gtk.CellRendererPixbuf.__init__ (
                self,
                *args,
                icon_name = "text-x-generic-symbolic",
                mode      = Gtk.CellRendererMode.ACTIVATABLE,
                **kwargs)

            self.connect ("notify::file",
                          lambda *args:
                              self.set_property ("visible", bool (self.file)))

        def do_activate (self, event, widget, path, *args):
            if self.file:
                run_editor (self.file, self.line)

                return True

            return False

    def __init__ (self, *args, **kwargs):
        Gtk.Box.__init__ (self,
                          *args,
                          orientation = Gtk.Orientation.HORIZONTAL,
                          **kwargs)

        self.needs_update = True

        vbox = Gtk.Box (orientation = Gtk.Orientation.VERTICAL)
        self.pack_start (vbox, False, False, 0)
        vbox.show ()

        header = Gtk.HeaderBar (title = "Threads", has_subtitle = False)
        vbox.pack_start (header, False, False, 0)
        header.show ()

        scrolled = Gtk.ScrolledWindow (
            hscrollbar_policy = Gtk.PolicyType.NEVER,
            vscrollbar_policy = Gtk.PolicyType.AUTOMATIC
        )
        vbox.pack_start (scrolled, True, True, 0)
        scrolled.show ()

        store = self.ThreadStore ()
        self.thread_store = store
        store.set_sort_column_id (store.ID, Gtk.SortType.ASCENDING)

        tree = Gtk.TreeView (model = store)
        self.thread_tree = tree
        scrolled.add (tree)
        tree.set_search_column (store.NAME)
        tree.show ()

        tree.connect ("row-activated", self.threads_row_activated)

        tree.get_selection ().connect ("changed",
                                       self.threads_selection_changed)

        col = Gtk.TreeViewColumn (title = "ID")
        tree.append_column (col)
        col.set_resizable (True)

        cell = Gtk.CellRendererText (xalign = 1)
        col.pack_start (cell, False)
        col.add_attribute (cell, "text", self.ThreadStore.ID)

        col = Gtk.TreeViewColumn (title = "Name")
        tree.append_column (col)
        col.set_resizable (True)

        cell = Gtk.CellRendererText ()
        col.pack_start (cell, False)
        col.add_attribute (cell, "text", self.ThreadStore.NAME)

        col = Gtk.TreeViewColumn (title = "State")
        tree.append_column (col)
        col.set_resizable (True)

        cell = Gtk.CellRendererText ()
        col.pack_start (cell, False)
        col.add_attribute (cell, "text", self.ThreadStore.STATE)

        separator = Gtk.Separator (orientation = Gtk.Orientation.VERTICAL)
        self.pack_start (separator, False, False, 0)
        separator.show ()

        vbox = Gtk.Box (orientation = Gtk.Orientation.VERTICAL)
        self.pack_start (vbox, True, True, 0)
        vbox.show ()

        header = Gtk.HeaderBar (title = "Stack", has_subtitle = False)
        vbox.pack_start (header, False, False, 0)
        header.show ()

        scrolled = Gtk.ScrolledWindow (
            hscrollbar_policy = Gtk.PolicyType.AUTOMATIC,
            vscrollbar_policy = Gtk.PolicyType.AUTOMATIC
        )
        vbox.pack_start (scrolled, True, True, 0)
        scrolled.show ()

        store = self.FrameStore ()
        self.frame_store = store

        tree = Gtk.TreeView (model = store, has_tooltip = True)
        scrolled.add (tree)
        tree.set_search_column (store.FUNCTION)
        tree.show ()

        tree.connect ("row-activated", self.frames_row_activated)
        tree.connect ("query-tooltip", self.frames_query_tooltip)

        def format_filename_col (tree_col, cell, model, iter, col):
            object = model[iter][col]

            cell.set_property ("text", get_basename (object) if object else "")

        self.tooltip_columns = {}

        col = Gtk.TreeViewColumn (title = "#")
        tree.append_column (col)
        col.set_resizable (True)

        cell = Gtk.CellRendererText (xalign = 1)
        col.pack_start (cell, False)
        col.add_attribute (cell, "text", self.FrameStore.ID)

        col = Gtk.TreeViewColumn (title = "Address")
        tree.append_column (col)
        col.set_resizable (True)

        cell = Gtk.CellRendererText (xalign = 1)
        col.pack_start (cell, False)
        col.add_attribute (cell, "text", self.FrameStore.ADDRESS)

        col = Gtk.TreeViewColumn (title = "Object")
        self.tooltip_columns[col] = store.OBJECT
        tree.append_column (col)
        col.set_resizable (True)

        cell = Gtk.CellRendererText ()
        col.pack_start (cell, False)
        col.set_cell_data_func (cell, format_filename_col, store.OBJECT)

        col = Gtk.TreeViewColumn (title = "Function")
        tree.append_column (col)
        col.set_resizable (True)

        cell = Gtk.CellRendererText ()
        col.pack_start (cell, False)
        col.add_attribute (cell, "text", self.FrameStore.FUNCTION)

        col = Gtk.TreeViewColumn (title = "Offset")
        tree.append_column (col)
        col.set_resizable (True)

        cell = Gtk.CellRendererText (xalign = 1)
        col.pack_start (cell, False)
        col.add_attribute (cell, "text", self.FrameStore.OFFSET)

        col = Gtk.TreeViewColumn (title = "Source")
        self.tooltip_columns[col] = store.SOURCE
        tree.append_column (col)
        col.set_resizable (True)

        cell = Gtk.CellRendererText ()
        col.pack_start (cell, False)
        col.set_cell_data_func (cell, format_filename_col, store.SOURCE)

        col = Gtk.TreeViewColumn (title = "Line")
        tree.append_column (col)
        col.set_resizable (True)

        cell = Gtk.CellRendererText (xalign = 1)
        col.pack_start (cell, False)
        col.add_attribute (cell, "text", self.FrameStore.LINE)

        def format_view_source_col (tree_col, cell, model, iter, cols):
            filename = model[iter][cols[0]] or None
            line     = model[iter][cols[1]] or "0"

            cell.set_property ("file", filename and find_file (filename))
            cell.set_property ("line", int (line))

        def format_view_source_tooltip (row):
            filename = row[store.SOURCE]

            if filename:
                file = find_file (filename)

                if file:
                    return file.get_path ()

            return None

        col = Gtk.TreeViewColumn ()
        self.tooltip_columns[col] = format_view_source_tooltip
        tree.append_column (col)

        cell = self.CellRendererViewSource (xalign = 0)
        col.pack_start (cell, False)
        col.set_cell_data_func (cell, format_view_source_col, (store.SOURCE,
                                                               store.LINE))

        selection.connect ("change-complete", self.selection_change_complete)

    @GObject.Property (type = bool, default = False)
    def available (self):
        sel = selection.get_effective_selection ()

        if len (sel) == 1:
            i, = sel

            return bool (samples[i].backtrace)

        return False

    def update (self):
        if not self.needs_update or not self.available:
            return

        self.needs_update = False

        tid = None

        sel_rows = self.thread_tree.get_selection ().get_selected_rows ()[1]

        if sel_rows:
            tid = self.thread_store[sel_rows[0]][self.ThreadStore.ID]

        i, = selection.get_effective_selection ()

        self.thread_store.clear ()

        for t in range (len (samples[i].backtrace)):
            thread = samples[i].backtrace[t]

            iter = self.thread_store.append (
                (t, thread.id, thread.name, str (thread.state))
            )

            if thread.id == tid:
                self.thread_tree.get_selection ().select_iter (iter)

    def do_map (self):
        self.update ()

        Gtk.Box.do_map (self)

    def selection_change_complete (self, selection):
        self.needs_update = True

        if self.get_mapped ():
            self.update ()

        self.notify ("available")

    def threads_row_activated (self, tree, path, col):
        iter = self.thread_store.get_iter (path)

        tid = self.thread_store[iter][self.ThreadStore.ID]

        sel = set ()

        for i in range (len (samples)):
            threads = filter (lambda thread:
                                thread.id    == tid and
                                thread.state == ThreadState.RUNNING,
                              samples[i].backtrace or [])

            if list (threads):
                sel.add (i)

        selection.select (sel)

        selection.change_complete ()

    def threads_selection_changed (self, tree_sel):
        self.frame_store.clear ()

        (store, rows) = tree_sel.get_selected_rows ()

        if not rows:
            return

        i, = selection.get_effective_selection ()

        try:
            frames = samples[i].backtrace[store[rows[0]][store.INDEX]].frames

            for frame in frames:
                info = frame.info

                self.frame_store.append ((
                    frame.id, hex (frame.address), info.object, info.symbol,
                    hex (info.offset) if info.offset is not None else None,
                    info.source, str (info.line) if info.line else None
                ))
        except:
            pass

    def frames_row_activated (self, tree, path, col):
        iter = self.frame_store.get_iter (path)

        address = int (self.frame_store[iter][self.FrameStore.ADDRESS], 0)
        info    = address_map.get (address, None)

        if not info:
            return

        id = info.id

        if not id:
            return

        sel = set ()

        def has_frame (sample, id):
            for thread in sample.backtrace or []:
                for frame in thread.frames:
                    if frame.info.id == id:
                        return True

            return False

        for i in range (len (samples)):
            if has_frame (samples[i], id):
                sel.add (i)

        selection.select (sel)

        selection.change_complete ()

    def frames_query_tooltip (self, tree, x, y, keyboard_mode, tooltip):
        hit, x, y, model, path, iter =  tree.get_tooltip_context (x, y,
                                                                  keyboard_mode)

        if hit:
            column = None

            if keyboard_mode:
                cursor_path, cursor_col = tree.get_cursor ()

                if path.compare (cursor_path) == 0:
                    column = self.tooltip_columns[cursor_col]
            else:
                for col in self.tooltip_columns:
                    area = tree.get_cell_area (path, col)

                    if x >= area.x and x < area.x + area.width and \
                       y >= area.y and y < area.y + area.height:
                        column = self.tooltip_columns[col]

                        break

            if column is not None:
                value = None

                if type (column) == int:
                    value = model[iter][column]
                else:
                    value = column (model[iter])

                if value:
                    tooltip.set_text (str (value))

                    return True

        return False

class CellRendererPercentage (Gtk.CellRendererText):
    padding = 0

    def __init__ (self, *args, **kwargs):
        Gtk.CellRendererText.__init__ (self, *args, xalign = 1, **kwargs)

        self.value = 0

    @GObject.Property (type = float)
    def value (self):
        return self.value_property

    @value.setter
    def value (self, value):
        self.value_property = value

        self.set_property ("text", format_percentage (value, 2))

    def do_render (self, cr, widget, background_area, cell_area, flags):
        full_width  = cell_area.width  - 2 * self.padding
        full_height = cell_area.height - 2 * self.padding

        if full_width <= 0 or full_height <= 0:
            return

        state    = widget.get_state_flags()
        style    = widget.get_style_context ()
        fg_color = style.get_color (state)

        rounded_rectangle (cr,
                           cell_area.x + self.padding,
                           cell_area.y + self.padding,
                           full_width,
                           full_height,
                           1)

        cr.clip ()

        cr.set_source_rgba (*blend_colors ((0, 0, 0, 0), fg_color, 0.2))
        cr.paint ()

        Gtk.CellRendererText.do_render (self,
                                        cr, widget,
                                        background_area, cell_area,
                                        flags)

        value  = min (max (self.value, 0), 1)
        width  = round (full_width * value)
        height = full_height

        if width > 0 and height > 0:
            state = Gtk.StateFlags        (state |
                                           Gtk.StateFlags.SELECTED)
            flags = Gtk.CellRendererState (flags |
                                           Gtk.CellRendererState.SELECTED)

            style.save ()
            style.set_state (state)

            x = round ((full_width - width) * self.get_property ("xalign"))

            cr.rectangle (cell_area.x + self.padding + x,
                          cell_area.y + self.padding,
                          width,
                          height)

            cr.clip ()

            Gtk.render_background (style, cr,
                                   cell_area.x,     cell_area.y,
                                   cell_area.width, cell_area.height)

            cr.set_source_rgba (0, 0, 0, 0.25)
            cr.paint ()

            Gtk.CellRendererText.do_render (self,
                                            cr, widget,
                                            background_area, cell_area,
                                            flags)

            style.restore ()

class ProfileViewer (Gtk.ScrolledWindow):
    class ThreadFilter (Gtk.TreeView):
        class Store (Gtk.ListStore):
            VISIBLE = 0
            ID      = 1
            NAME    = 2
            STATE   = {list (ThreadState)[i]: 3 + i
                       for i in range (len (ThreadState))}

            def __init__ (self):
                Gtk.ListStore.__init__ (self,
                                        bool, int, str,
                                        *(len (self.STATE) * (bool,)))

                threads = list ({thread.id
                                 for sample in samples
                                 for thread in sample.backtrace or ()})
                threads.sort ()

                states = [state == ThreadState.RUNNING for state in self.STATE]

                for id in threads:
                    self.append ((False, id, None, *states))

            def get_filter (self):
                return {row[self.ID]: {state
                                       for state, column in self.STATE.items ()
                                       if row[column]}
                        for row in self}

            def set_filter (self, filter):
                for row in self:
                    states = filter[row[self.ID]]

                    for state, column in self.STATE.items ():
                        row[column] = state in states

        def __init__ (self, *args, **kwargs):
            Gtk.TreeView.__init__ (self, *args, **kwargs)

            self.needs_update = True

            store = self.Store ()
            self.store = store

            filter = Gtk.TreeModelFilter (child_model = store)
            filter.set_visible_column (store.VISIBLE)
            self.set_model (filter)
            self.set_search_column (store.NAME)

            col = Gtk.TreeViewColumn (title = "ID")
            self.append_column (col)

            cell = Gtk.CellRendererText (xalign = 1)
            col.pack_start (cell, False)
            col.add_attribute (cell, "text", store.ID)

            col = Gtk.TreeViewColumn (title = "Name")
            self.append_column (col)

            cell = Gtk.CellRendererText ()
            col.pack_start (cell, False)
            col.add_attribute (cell, "text", store.NAME)

            for state in store.STATE:
                col = Gtk.TreeViewColumn (title = str (state))
                col.column = store.STATE[state]
                self.append_column (col)
                col.set_alignment (0.5)
                col.set_clickable (True)

                def col_clicked (col):
                    active = not all (row[col.column] for row in filter)

                    for row in filter:
                        row[col.column] = active

                col.connect ("clicked", col_clicked)

                cell = Gtk.CellRendererToggle ()
                cell.column = store.STATE[state]
                col.pack_start (cell, False)
                col.add_attribute (cell, "active", store.STATE[state])

                def cell_toggled (cell, path):
                    filter[path][cell.column] = not cell.get_property ("active")

                cell.connect ("toggled", cell_toggled)

            selection.connect ("change-complete",
                               self.selection_change_complete)

        def update (self):
            if not self.needs_update:
                return

            self.needs_update = False

            sel = selection.get_effective_selection ()

            threads = {thread.id: thread.name
                       for i in sel
                       for thread in samples[i].backtrace or ()}

            for row in self.store:
                id = row[self.store.ID]

                if id in threads:
                    row[self.store.VISIBLE] = True
                    row[self.store.NAME]    = threads[id]
                else:
                    row[self.store.VISIBLE] = False
                    row[self.store.NAME]    = None

        def do_map (self):
            self.update ()

            Gtk.TreeView.do_map (self)

        def selection_change_complete (self, selection):
            self.needs_update = True

            if self.get_mapped ():
                self.update ()

    class ThreadPopover (Gtk.Popover):
        def __init__ (self, *args, **kwargs):
            Gtk.Popover.__init__ (self, *args, border_width = 4, **kwargs)

            frame = Gtk.Frame (shadow_type = Gtk.ShadowType.IN)
            self.add (frame)
            frame.show ()

            scrolled = Gtk.ScrolledWindow (
                hscrollbar_policy        = Gtk.PolicyType.NEVER,
                vscrollbar_policy        = Gtk.PolicyType.AUTOMATIC,
                propagate_natural_height = True,
                max_content_height       = 400
            )
            frame.add (scrolled)
            scrolled.show ()

            thread_filter = ProfileViewer.ThreadFilter ()
            self.thread_filter = thread_filter
            scrolled.add (thread_filter)
            thread_filter.show ()

    class Profile (Gtk.Box):
        ProfileFrame = namedtuple ("ProfileFrame", ("sample", "stack", "i"))

        class Direction (enum.Enum):
            CALLEES = enum.auto ()
            CALLERS = enum.auto ()

        class Store (Gtk.ListStore):
            ID        = 0
            FUNCTION  = 1
            EXCLUSIVE = 2
            INCLUSIVE = 3

            def __init__ (self):
                Gtk.ListStore.__init__ (self,
                                        GObject.TYPE_UINT64, str, float, float)

        __gsignals__ = {
            "needs-update":       (GObject.SignalFlags.RUN_FIRST,
                                   None, (bool,)),
            "subprofile-added":   (GObject.SignalFlags.RUN_FIRST,
                                   None, (Gtk.Widget,)),
            "subprofile-removed": (GObject.SignalFlags.RUN_FIRST,
                                   None, (Gtk.Widget,)),
            "path-changed":       (GObject.SignalFlags.RUN_FIRST,
                                   None, ())
        }

        def __init__ (self,
                      root      = None,
                      id        = None,
                      title     = None,
                      frames    = None,
                      direction = Direction.CALLEES,
                      sort      = (Store.INCLUSIVE, Gtk.SortType.DESCENDING),
                      *args,
                      **kwargs):
            Gtk.Box.__init__ (self,
                              *args,
                              orientation = Gtk.Orientation.HORIZONTAL,
                              **kwargs)

            self.root      = root or self
            self.id        = id
            self.frames    = frames
            self.direction = direction

            self.subprofile = None

            vbox = Gtk.Box (orientation = Gtk.Orientation.VERTICAL)
            self.pack_start (vbox, False, False, 0)
            vbox.show ()

            header = Gtk.HeaderBar (title = title or "All Functions")
            self.header = header
            vbox.pack_start (header, False, False, 0)
            header.show ()

            if not id:
                popover = ProfileViewer.ThreadPopover ()

                thread_filter_store = popover.thread_filter.store

                self.thread_filter_store = thread_filter_store
                self.thread_filter       = thread_filter_store.get_filter ()

                history.add_source (self.thread_filter_source_get,
                                    self.thread_filter_source_set)

                button = Gtk.MenuButton (popover = popover)
                header.pack_end (button)
                button.show ()

                button.connect ("toggled", self.thread_filter_button_toggled)

                hbox = Gtk.Box (orientation = Gtk.Orientation.HORIZONTAL,
                                spacing     = 4)
                button.add (hbox)
                hbox.show ()

                label = Gtk.Label (label = "Threads")
                hbox.pack_start (label, False, False, 0)
                label.show ()

                image = Gtk.Image.new_from_icon_name ("pan-down-symbolic",
                                                      Gtk.IconSize.BUTTON)
                hbox.pack_start (image, False, False, 0)
                image.show ()

                history.add_source (self.direction_source_get,
                                    self.direction_source_set)

                button = Gtk.Button (tooltip_text = "Call-graph direction")
                header.pack_end (button)
                button.show ()

                button.connect ("clicked", self.direction_button_clicked)

                image = Gtk.Image ()
                self.direction_image = image
                button.add (image)
                image.show ()
            else:
                button = Gtk.Button.new_from_icon_name (
                    "edit-select-symbolic",
                    Gtk.IconSize.BUTTON
                )
                header.pack_end (button)
                button.set_tooltip_text (
                    str (Selection (frame.sample for frame in frames))
                )
                button.show ()

                button.connect ("clicked", self.select_samples_clicked)

            scrolled = Gtk.ScrolledWindow (
                hscrollbar_policy = Gtk.PolicyType.NEVER,
                vscrollbar_policy = Gtk.PolicyType.AUTOMATIC
            )
            vbox.pack_start (scrolled, True, True, 0)
            scrolled.show ()

            store = self.Store ()
            self.store = store
            store.set_sort_column_id (*sort)

            tree = Gtk.TreeView (model = store)
            self.tree = tree
            scrolled.add (tree)
            tree.set_search_column (store.FUNCTION)
            tree.show ()

            tree.get_selection ().connect ("changed",
                                           self.tree_selection_changed)

            tree.connect ("row-activated",   self.tree_row_activated)
            tree.connect ("key-press-event", self.tree_key_press_event)

            col = Gtk.TreeViewColumn (title = "Function")
            tree.append_column (col)
            col.set_resizable (True)
            col.set_sort_column_id (store.FUNCTION)

            cell = Gtk.CellRendererText (ellipsize = Pango.EllipsizeMode.END)
            col.pack_start (cell, True)
            col.add_attribute (cell, "text", store.FUNCTION)
            cell.set_property ("width-chars", 40)

            col = Gtk.TreeViewColumn (title = "Self")
            tree.append_column (col)
            col.set_alignment (0.5)
            col.set_sort_column_id (store.EXCLUSIVE)

            cell = CellRendererPercentage ()
            col.pack_start (cell, False)
            col.add_attribute (cell, "value", store.EXCLUSIVE)

            col = Gtk.TreeViewColumn (title = "All")
            tree.append_column (col)
            col.set_alignment (0.5)
            col.set_sort_column_id (store.INCLUSIVE)

            cell = CellRendererPercentage ()
            col.pack_start (cell, False)
            col.add_attribute (cell, "value", store.INCLUSIVE)

            if id:
                self.update ()

        def update (self):
            self.remove_subprofile ()

            if not self.id:
                self.update_frames ()

            self.update_store ()

            self.update_ui ()

        def update_frames (self):
            self.frames = []

            for i in selection.get_effective_selection ():
                for thread in samples[i].backtrace or []:
                    if thread.state in self.thread_filter[thread.id]:
                        thread_frames = thread.frames

                        if self.direction == self.Direction.CALLERS:
                            thread_frames = reversed (thread_frames)

                        stack   = []
                        prev_id = 0

                        for frame in thread_frames:
                            id = frame.info.id

                            if id == prev_id:
                                continue

                            self.frames.append (self.ProfileFrame (
                                sample = i,
                                stack  = stack,
                                i      = len (stack)
                            ))

                            stack.append (frame)

                            prev_id = id

        def update_store (self):
            stacks  = {}
            symbols = {}

            sort = self.store.get_sort_column_id ()

            self.store = self.Store ()

            for frame in self.frames:
                info      = frame.stack[frame.i].info
                symbol_id = info.id
                stack_id  = builtins.id (frame.stack)

                symbol = symbols.get (symbol_id, None)

                if not symbol:
                    symbol             = [info, 0, 0]
                    symbols[symbol_id] = symbol

                stack = stacks.get (stack_id, None)

                if not stack:
                    stack            = set ()
                    stacks[stack_id] = stack

                if frame.i == 0:
                    symbol[1] += 1

                if symbol_id not in stack:
                    stack.add (symbol_id)

                    symbol[2] += 1

            n_stacks = len (stacks)

            for symbol in symbols.values ():
                id   = symbol[0].id
                name = symbol[0].name if id != self.id else "[Self]"

                self.store.append ((id,
                                    name,
                                    symbol[1] / n_stacks,
                                    symbol[2] / n_stacks))

            self.store.set_sort_column_id (*sort)

            self.tree.set_model (self.store)
            self.tree.set_search_column (self.store.FUNCTION)

        def update_ui (self):
            if not self.id:
                if self.direction == self.Direction.CALLEES:
                    icon_name = "format-indent-more-symbolic"
                else:
                    icon_name = "format-indent-less-symbolic"

                self.direction_image.set_from_icon_name (icon_name,
                                                         Gtk.IconSize.BUTTON)
            else:
                if self.direction == self.Direction.CALLEES:
                    subtitle = "Callees"
                else:
                    subtitle = "Callers"

                self.header.set_subtitle (subtitle)

        def select (self, id):
            if id is not None:
                for row in self.store:
                    if row[self.store.ID] == id:
                        iter = row.iter
                        path = self.store.get_path (iter)

                        self.tree.get_selection ().select_iter (iter)

                        self.tree.scroll_to_cell (path, None, True, 0.5, 0)

                        break
            else:
                self.tree.get_selection ().unselect_all ()

        def add_subprofile (self, subprofile):
            self.remove_subprofile ()

            box = Gtk.Box (orientation = Gtk.Orientation.HORIZONTAL)
            self.subprofile_box = box
            self.pack_start (box, True, True, 0)
            box.show ()

            separator = Gtk.Separator (orientation = Gtk.Orientation.VERTICAL)
            box.pack_start (separator, False, False, 0)
            separator.show ()

            self.subprofile = subprofile
            box.pack_start (subprofile, True, True, 0)
            subprofile.show ()

            subprofile.connect ("subprofile-added",
                                lambda profile, subprofile:
                                    self.emit ("subprofile-added",
                                               subprofile))
            subprofile.connect ("subprofile-removed",
                                lambda profile, subprofile:
                                    self.emit ("subprofile-removed",
                                               subprofile))
            subprofile.connect ("path-changed",
                                lambda profile: self.emit ("path-changed"))

            self.emit ("subprofile-added", subprofile)

        def remove_subprofile (self):
            if self.subprofile:
                subprofile = self.subprofile

                self.remove (self.subprofile_box)

                self.subprofile     = None
                self.subprofile_box = None

                self.emit ("subprofile-removed", subprofile)

        def get_path (self):
            tree_sel = self.tree.get_selection ()

            sel_rows = tree_sel.get_selected_rows ()[1]

            if not sel_rows:
                return ()

            id = self.store[sel_rows[0]][self.store.ID]

            if self.subprofile:
                return (id,) + self.subprofile.get_path ()
            else:
                return (id,)

        def set_path (self, path):
            self.select (path[0] if path else None)

            if self.subprofile:
                self.subprofile.set_path (path[1:])

        def thread_filter_source_get (self):
            return self.thread_filter_store.get_filter ()

        def thread_filter_source_set (self, thread_filter):
            self.thread_filter = thread_filter

            self.thread_filter_store.set_filter (thread_filter)

            self.emit ("needs-update", False)

        def thread_filter_button_toggled (self, button):
            if not button.get_active ():
                thread_filter = self.thread_filter_store.get_filter ()

                if thread_filter != self.thread_filter:
                    self.thread_filter = thread_filter

                    history.start_group ()

                    history.record ()

                    self.emit ("needs-update", True)

                    history.end_group ()

        def direction_source_get (self):
            return self.direction

        def direction_source_set (self, direction):
            self.direction = direction

            self.emit ("needs-update", False)

        def direction_button_clicked (self, button):
            if self.direction == self.Direction.CALLEES:
                self.direction = self.Direction.CALLERS
            else:
                self.direction = self.Direction.CALLEES

            history.start_group ()

            history.record ()

            self.emit ("needs-update", True)

            history.end_group ()

        def select_samples_clicked (self, button):
            selection.select ({frame.sample for frame in self.frames})

            selection.change_complete ()

        def tree_selection_changed (self, tree_sel):
            self.remove_subprofile ()

            sel_rows = tree_sel.get_selected_rows ()[1]

            if not sel_rows:
                self.emit ("path-changed")

                return

            id    = self.store[sel_rows[0]][self.store.ID]
            title = self.store[sel_rows[0]][self.store.FUNCTION]

            frames = []

            for frame in self.frames:
                if frame.stack[frame.i].info.id == id:
                    frames.append (frame)

                    if frame.i > 0 and id != self.id:
                        frames.append (self.ProfileFrame (sample = frame.sample,
                                                          stack  = frame.stack,
                                                          i      = frame.i - 1))

            if id != self.id:
                self.add_subprofile (ProfileViewer.Profile (
                    self.root,
                    id,
                    title,
                    frames,
                    self.direction,
                    self.store.get_sort_column_id ()
                ))
            else:
                filenames = {frame.stack[frame.i].info.source
                             for frame in frames}
                filenames = list (filter (bool, filenames))

                if len (filenames) == 1:
                    file = find_file (filenames[0])

                    if file:
                        self.add_subprofile (ProfileViewer.SourceProfile (
                            file,
                            frames[0].stack[frames[0].i].info.name,
                            frames
                        ))

            self.emit ("path-changed")

        def tree_row_activated (self, tree, path, col):
            if self.root != self:
                self.root.select (self.store[path][self.store.ID])

        def tree_key_press_event (self, tree, event):
            if event.keyval == Gdk.KEY_Escape:
                self.select (None)

                if self.root is not self:
                    self.get_parent ().get_ancestor (
                        ProfileViewer.Profile
                    ).tree.grab_focus ()

                return True

            return False

    class SourceProfile (Gtk.Box):
        class Store (Gtk.ListStore):
            LINE       = 0
            HAS_FRAMES = 1
            EXCLUSIVE  = 2
            INCLUSIVE  = 3
            TEXT       = 4

            def __init__ (self):
                Gtk.ListStore.__init__ (self, int, bool, float, float, str)

        __gsignals__ = {
            "subprofile-added":   (GObject.SignalFlags.RUN_FIRST,
                                   None, (Gtk.Widget,)),
            "subprofile-removed": (GObject.SignalFlags.RUN_FIRST,
                                   None, (Gtk.Widget,)),
            "path-changed":       (GObject.SignalFlags.RUN_FIRST,
                                   None, ())
        }

        def __init__ (self,
                      file,
                      function,
                      frames,
                      *args,
                      **kwargs):
            Gtk.Box.__init__ (self,
                              *args,
                              orientation = Gtk.Orientation.VERTICAL,
                              **kwargs)

            self.file   = file
            self.frames = frames

            header = Gtk.HeaderBar (title    = file.get_basename (),
                                    subtitle = function)
            self.header = header
            self.pack_start (header, False, False, 0)
            header.show ()

            box = Gtk.Box (orientation = Gtk.Orientation.HORIZONTAL)
            header.pack_start (box)
            box.get_style_context ().add_class ("linked")
            box.get_style_context ().add_class ("raised")
            box.show ()

            button = Gtk.Button.new_from_icon_name ("go-up-symbolic",
                                                    Gtk.IconSize.BUTTON)
            self.prev_button = button
            box.pack_start (button, False, True, 0)
            button.show ()

            button.connect ("clicked", lambda *args: self.move (-1))

            button = Gtk.Button.new_from_icon_name ("go-down-symbolic",
                                                    Gtk.IconSize.BUTTON)
            self.next_button = button
            box.pack_end (button, False, True, 0)
            button.show ()

            button.connect ("clicked", lambda *args: self.move (+1))

            button = Gtk.Button.new_from_icon_name ("edit-select-symbolic",
                                                    Gtk.IconSize.BUTTON)
            self.select_samples_button = button
            header.pack_end (button)
            button.show ()

            button.connect ("clicked", self.select_samples_clicked)

            button = Gtk.Button.new_from_icon_name ("text-x-generic-symbolic",
                                                    Gtk.IconSize.BUTTON)
            header.pack_end (button)
            button.set_tooltip_text (file.get_path ())
            button.show ()

            button.connect ("clicked", self.view_source_clicked)

            scrolled = Gtk.ScrolledWindow (
                hscrollbar_policy = Gtk.PolicyType.NEVER,
                vscrollbar_policy = Gtk.PolicyType.AUTOMATIC
            )
            self.pack_start (scrolled, True, True, 0)
            scrolled.show ()

            store = self.Store ()
            self.store = store

            tree = Gtk.TreeView (model = store)
            self.tree = tree
            scrolled.add (tree)
            tree.set_search_column (store.LINE)
            tree.show ()

            tree.get_selection ().connect ("changed",
                                           self.tree_selection_changed)

            scale = 0.85

            col = Gtk.TreeViewColumn (title = "Self")
            tree.append_column (col)
            col.set_alignment (0.5)
            col.set_sort_column_id (store.EXCLUSIVE)

            cell = CellRendererPercentage (scale = scale)
            col.pack_start (cell, False)
            col.add_attribute (cell, "visible", store.HAS_FRAMES)
            col.add_attribute (cell, "value",   store.EXCLUSIVE)

            col = Gtk.TreeViewColumn (title = "All")
            tree.append_column (col)
            col.set_alignment (0.5)
            col.set_sort_column_id (store.INCLUSIVE)

            cell = CellRendererPercentage (scale = scale)
            col.pack_start (cell, False)
            col.add_attribute (cell, "visible", store.HAS_FRAMES)
            col.add_attribute (cell, "value",   store.INCLUSIVE)

            col = Gtk.TreeViewColumn ()
            tree.append_column (col)

            cell = Gtk.CellRendererText (xalign = 1,
                                         xpad   = 8,
                                         family = "Monospace",
                                         weight = Pango.Weight.BOLD,
                                         scale  = scale)
            col.pack_start (cell, False)
            col.add_attribute (cell, "text", store.LINE)

            cell = Gtk.CellRendererText (family = "Monospace",
                                         scale  = scale)
            col.pack_start (cell, True)
            col.add_attribute (cell, "text", store.TEXT)

            self.update ()

        def get_samples (self):
            sel_rows = self.tree.get_selection ().get_selected_rows ()[1]

            if sel_rows:
                line = self.store[sel_rows[0]][self.store.LINE]

                sel = {frame.sample for frame in self.frames
                       if frame.stack[frame.i].info.line == line}

                return sel
            else:
                return {}

        def update (self):
            self.update_store ()
            self.update_ui ()

        def update_store (self):
            stacks = {}
            lines  = {}

            for frame in self.frames:
                info     = frame.stack[frame.i].info
                line_id  = info.line
                stack_id = builtins.id (frame.stack)

                line = lines.get (line_id, None)

                if not line:
                    line           = [0, 0]
                    lines[line_id] = line

                stack = stacks.get (stack_id, None)

                if not stack:
                    stack            = set ()
                    stacks[stack_id] = stack

                if frame.i == 0:
                    line[0] += 1

                if line_id not in stack:
                    stack.add (line_id)

                    line[1] += 1

            self.lines = list (lines.keys ())
            self.lines.sort ()

            n_stacks = len (stacks)

            self.store.clear ()

            i = 1

            for text in open (self.file.get_path (), "r"):
                text = text.rstrip ("\n")

                line = lines.get (i, None)

                if line:
                    self.store.append ((i,
                                        True,
                                        line[0] / n_stacks,
                                        line[1] / n_stacks,
                                        text))
                else:
                    self.store.append ((i,
                                        False,
                                        0,
                                        0,
                                        text))

                i += 1

            self.select (max (lines.items (), key = lambda line: line[1][1])[0])

        def update_ui (self):
            sel_rows = self.tree.get_selection ().get_selected_rows ()[1]

            if sel_rows:
                line = self.store[sel_rows[0]][self.store.LINE]

                i = bisect.bisect_left (self.lines, line)

                self.prev_button.set_sensitive (i > 0)

                if i < len (self.lines) and self.lines[i] == line:
                    i += 1

                self.next_button.set_sensitive (i < len (self.lines))
            else:
                self.prev_button.set_sensitive (False)
                self.next_button.set_sensitive (False)

            samples = self.get_samples ()

            if samples:
                self.select_samples_button.set_sensitive (True)
                self.select_samples_button.set_tooltip_text (
                    str (Selection (samples))
                )
            else:
                self.select_samples_button.set_sensitive (False)
                self.select_samples_button.set_tooltip_text (None)

        def select (self, line):
            if line is not None:
                for row in self.store:
                    if row[self.store.LINE] == line:
                        iter = row.iter
                        path = self.store.get_path (iter)

                        self.tree.get_selection ().select_iter (iter)

                        self.tree.scroll_to_cell (path, None, True, 0.5, 0)

                        break
            else:
                self.tree.get_selection ().unselect_all ()


        def move (self, dir):
            if dir == 0:
                return

            sel_rows = self.tree.get_selection ().get_selected_rows ()[1]

            if sel_rows:
                line = self.store[sel_rows[0]][self.store.LINE]

                i = bisect.bisect_left (self.lines, line)

                if dir < 0:
                    i -= 1
                elif i < len (self.lines) and self.lines[i] == line:
                    i += 1

                if i >= 0 and i < len (self.lines):
                    self.select (self.lines[i])
                else:
                    self.select (None)

        def select_samples_clicked (self, button):
            selection.select (self.get_samples ())

            selection.change_complete ()

        def view_source_clicked (self, button):
            line = 0

            sel_rows = self.tree.get_selection ().get_selected_rows ()[1]

            if sel_rows:
                line = self.store[sel_rows[0]][self.store.LINE]

            run_editor (self.file, line)

        def get_path (self):
            tree_sel = self.tree.get_selection ()

            sel_rows = tree_sel.get_selected_rows ()[1]

            if not sel_rows:
                return ()

            line = self.store[sel_rows[0]][self.store.LINE]

            return (line,)

        def set_path (self, path):
            self.select (path[0] if path else None)

        def tree_selection_changed (self, tree_sel):
            self.update_ui ()

            self.emit ("path-changed")

    def __init__ (self, *args, **kwargs):
        Gtk.ScrolledWindow.__init__ (
            self,
            *args,
            hscrollbar_policy = Gtk.PolicyType.AUTOMATIC,
            vscrollbar_policy = Gtk.PolicyType.NEVER,
            **kwargs
        )

        self.adjustment_changed_handler = None
        self.needs_update               = True
        self.path                       = ()

        profile = self.Profile ()
        self.root_profile = profile
        self.add (profile)
        profile.show ()

        selection.connect ("change-complete", self.selection_change_complete)

        profile.connect ("needs-update",       self.profile_needs_update)
        profile.connect ("subprofile-added",   self.profile_subprofile_added)
        profile.connect ("subprofile-removed", self.profile_subprofile_removed)
        profile.connect ("path-changed",       self.profile_path_changed)

        history.add_source (self.source_get, self.source_set)

    @GObject.Property (type = bool, default = False)
    def available (self):
        sel = selection.get_effective_selection ()

        if len (sel) > 1:
            return any (samples[i].backtrace for i in sel)

        return False

    def update (self):
        if not self.available:
            return

        history.block ()

        if self.needs_update:
            self.root_profile.update ()

            self.needs_update = False

        self.root_profile.set_path (self.path)

        history.unblock ()

    def queue_update (self, now = False):
        self.needs_update = True

        if now or self.get_mapped ():
            self.update ()

    def do_map (self):
        self.update ()

        Gtk.ScrolledWindow.do_map (self)

    def selection_change_complete (self, selection):
        self.queue_update ()

        self.notify ("available")

    def profile_needs_update (self, profile, now):
        self.queue_update (now)

    def profile_subprofile_added (self, profile, subprofile):
        if not history.is_blocked ():
            self.path = profile.get_path ()

        history.record ()

        if not self.adjustment_changed_handler:
            adjustment = self.get_hadjustment ()

            def adjustment_changed (adjustment):
                GObject.signal_handler_disconnect (
                    adjustment,
                    self.adjustment_changed_handler
                )
                self.adjustment_changed_handler = None

                adjustment.set_value (adjustment.get_upper ())

            self.adjustment_changed_handler = adjustment.connect (
                "changed",
                adjustment_changed
            )

    def profile_subprofile_removed (self, profile, subprofile):
        if not history.is_blocked ():
            self.path = profile.get_path ()

        history.record ()


    def profile_path_changed (self, profile):
        if not history.is_blocked ():
            self.path = profile.get_path ()

        history.update ()

    def source_get (self):
        return self.path

    def source_set (self, path):
        self.path = path

        if self.get_mapped ():
            self.root_profile.set_path (path)

class LogViewer (Gtk.Window):
    def __init__ (self, *args, **kwargs):
        Gtk.Window.__init__ (
            self,
            *args,
            default_width   = 1024,
            default_height  = 768,
            window_position = Gtk.WindowPosition.CENTER,
            **kwargs)

        header = Gtk.HeaderBar (
            title             = "GIMP Performance Log Viewer",
            show_close_button = True
        )
        self.header = header
        self.set_titlebar (header)
        header.show ()

        box = Gtk.Box (orientation = Gtk.Orientation.HORIZONTAL)
        header.pack_start (box)
        box.get_style_context ().add_class ("linked")
        box.get_style_context ().add_class ("raised")
        box.show ()

        button = Gtk.Button.new_from_icon_name ("go-previous-symbolic",
                                                Gtk.IconSize.BUTTON)
        box.pack_start (button, False, True, 0)
        button.show ()

        history.bind_property ("can-undo",
                               button, "sensitive",
                               GObject.BindingFlags.SYNC_CREATE)

        button.connect ("clicked", lambda *args: history.undo ())

        button = Gtk.Button.new_from_icon_name ("go-next-symbolic",
                                                Gtk.IconSize.BUTTON)
        box.pack_end (button, False, True, 0)
        button.show ()

        history.bind_property ("can-redo",
                               button, "sensitive",
                               GObject.BindingFlags.SYNC_CREATE)

        button.connect ("clicked", lambda *args: history.redo ())

        button = Gtk.MenuButton ()
        header.pack_end (button)
        button.set_tooltip_text ("Find samples")
        button.show ()

        image = Gtk.Image.new_from_icon_name ("edit-find-symbolic",
                                              Gtk.IconSize.BUTTON)
        button.add (image)
        image.show ()

        popover = FindSamplesPopover (relative_to = button)
        self.find_popover = popover
        button.set_popover (popover)

        def selection_action (action):
            def f (*args):
                action (selection)
                selection.change_complete ()

            return f

        button = Gtk.Button.new_from_icon_name (
            "object-flip-horizontal-symbolic",
            Gtk.IconSize.BUTTON
        )
        header.pack_end (button)
        button.set_tooltip_text ("Invert selection")
        button.show ()

        button.connect ("clicked", selection_action (Selection.invert))

        button = Gtk.Button.new_from_icon_name (
            "edit-clear-symbolic",
            Gtk.IconSize.BUTTON
        )
        self.clear_selection_button = button
        header.pack_end (button)
        button.set_tooltip_text ("Clear selection")
        button.show ()

        button.connect ("clicked", selection_action (Selection.clear))

        paned = Gtk.Paned (orientation = Gtk.Orientation.VERTICAL)
        self.paned = paned
        self.add (paned)
        paned.set_position (144)
        paned.show ()

        graphs = SampleGraphList ()
        paned.add1 (graphs)
        paned.child_set (graphs, shrink = False)
        graphs.show ()

        hbox = Gtk.Box (orientation = Gtk.Orientation.HORIZONTAL)
        paned.add2 (hbox)
        hbox.show ()

        sidebar = Gtk.StackSidebar ()
        hbox.pack_start (sidebar, False, False, 0)
        sidebar.show ()

        stack = Gtk.Stack (transition_type = Gtk.StackTransitionType.CROSSFADE)
        self.stack = stack
        hbox.pack_start (stack, True, True, 0)
        stack.show ()

        sidebar.set_stack (stack)

        info_viewer = InformationViewer ()
        stack.add_titled (info_viewer, "information", "Information")
        info_viewer.show ()

        if markers:
            markers_viewer = MarkersViewer ()
            stack.add_titled (markers_viewer, "markers", "Markers")
            markers_viewer.show ()

        vars_viewer = VariablesViewer ()
        stack.add_titled (vars_viewer, "variables", "Variables")
        vars_viewer.show ()

        box = Gtk.Box (orientation = Gtk.Orientation.VERTICAL)
        self.cflow_box = box
        stack.add_named (box, "cflow")

        backtrace_viewer = BacktraceViewer ()
        self.backtrace_viewer = backtrace_viewer
        box.pack_start (backtrace_viewer, True, True, 0)

        backtrace_viewer.bind_property ("available",
                                        backtrace_viewer, "visible",
                                        GObject.BindingFlags.SYNC_CREATE)

        backtrace_viewer.connect ("notify::available",
                                  self.cflow_notify_available)

        profile_viewer = ProfileViewer ()
        self.profile_viewer = profile_viewer
        box.pack_start (profile_viewer, True, True, 0)

        profile_viewer.bind_property ("available",
                                      profile_viewer, "visible",
                                      GObject.BindingFlags.SYNC_CREATE)

        profile_viewer.connect ("notify::available",
                                self.cflow_notify_available)

        self.cflow_notify_available (self)

        selection.connect ("change-complete", self.selection_change_complete)

        self.selection_change_complete (selection)

    def selection_change_complete (self, selection):
        self.header.set_subtitle (str (selection))
        self.clear_selection_button.set_sensitive (selection.selection)

    def cflow_notify_available (self, *args):
        if self.backtrace_viewer.available:
            self.stack.child_set (self.cflow_box, title = "Backtrace")
            self.cflow_box.show ()
        elif self.profile_viewer.available:
            self.stack.child_set (self.cflow_box, title = "Profile")
            self.cflow_box.show ()
        else:
            self.cflow_box.hide ()

Gtk.Settings.get_default ().set_property ("gtk-application-prefer-dark-theme",
                                          True)

window = LogViewer ()
window.show ()

window.connect ("destroy", Gtk.main_quit)

history.record ()

Gtk.main ()
