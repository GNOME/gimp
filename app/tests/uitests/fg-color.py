#!/usr/bin/env python3
#
# runs real gimp under a dogtail HermeticSession, closes the welcome dialog,
# opens Windows > Dockable Dialogs > Colors, focuses the live hex-triplet
# entry (default 000000, same widget color-swap.py reads), selects all and
# types a new hex value, hits Return, and checks the entry now reads that
# value back.
#
# the toolbox's own FG/BG swatch is a custom-drawn GimpColorArea with no
# AT-SPI children to click (see color-swap.py), so we drive the Colors
# dockable's hex entry directly instead of popping the "Change Foreground
# Color" dialog - same context->foreground GObject property either way.
# GimpColorHexEntry only commits on a real key-release/Return event
# (libgimpwidgets/gimpcolorhexentry.c), so setting .text through the
# EditableText interface alone wouldn't trigger the color change - real
# keystrokes are needed here, unlike the Value-interface spin buttons in
# new-image.py.
#
# args: <gimp-exe> <mutter-exe>  (gimpmain_exe, mutter_prog, see meson)
# exit: 0 pass, 1 fail, 99 hard error (infra bug: mutter/dogtail/session boot)

import os
import re
import sys
import time

# rawinput freezes its injection backend at import time off this var - must
# be set before dogtail.rawinput (imported transitively by hermetic.session's
# key_combo/type_text helpers) is loaded. unset means it assumes x11 and
# keyboard synthesis goes nowhere under our headless wayland mutter.
os.environ.setdefault("XDG_SESSION_TYPE", "wayland")

from uicommon import close_welcome_dialog, dump_tree, fail, find_all, find_first, \
    gimp_session, wait_for

from dogtail.rawinput import key_combo, press_key, type_text

GIMP = sys.argv[1]
MUTTER = sys.argv[2]

HEX_RE = re.compile(r"^[0-9a-fA-F]{6}$")
NEW_COLOR = "3366cc"

with gimp_session(GIMP, MUTTER, install_input=True) as (session, app, proc):
    close_welcome_dialog(app)

    # --- step 2: menu helper -------------------------------------------

    def click_menu_path(*names):
        node = app
        for i, name in enumerate(names):
            if i == 0:
                found = find_first(node, lambda n: n.roleName == "menu" and n.name == name)
            else:
                found = wait_for(
                    lambda: find_first(node, lambda n: n.roleName in ("menu item", "menu")
                                       and n.name == name),
                    10)
            if found is None:
                fail("menu path %r: %r not found via AT-SPI:\n%s"
                     % (names, name, dump_tree(app)))
            found.do_action_named("click")
            node = found
        time.sleep(0.3)

    # --- step 3: Windows > Dockable Dialogs > Colors -----------------------

    click_menu_path("Windows", "Dockable Dialogs", "Colors")

    # --- step 4: find the hex entry -----------------------------------

    def find_hex_entry():
        for node in find_all(app, lambda n: n.roleName == "text"):
            try:
                txt = (node.text or "").strip()
            except Exception:
                continue
            if HEX_RE.match(txt):
                return node
        return None

    hex_entry = wait_for(find_hex_entry, 15)
    if hex_entry is None:
        fail("no hex-triplet text entry found via AT-SPI:\n%s" % dump_tree(app))

    before = hex_entry.text.strip().lower()
    if before != "000000":
        fail("expected default foreground 000000 before edit, got %r" % before)

    # --- step 5: focus the entry, select all, type new hex, commit --------

    hex_entry.grab_focus()
    time.sleep(0.3)
    key_combo("<Control>a")
    type_text(NEW_COLOR)
    press_key("Return")

    # --- step 6: entry should now read the new hex value -------------------

    def after_edit():
        try:
            txt = hex_entry.text.strip().lower()
        except Exception:
            return None
        return txt if txt == NEW_COLOR else None

    after = wait_for(after_edit, 15)
    if after is None:
        fail("expected foreground hex to become %r, still %r"
             % (NEW_COLOR, hex_entry.text))

    sys.stderr.write("fg hex %r -> %r after edit, ok\n" % (before, after))

sys.exit(0)
