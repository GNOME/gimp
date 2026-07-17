#!/usr/bin/env python3
#
# runs real gimp under a dogtail HermeticSession, closes the welcome dialog,
# opens Windows > Dockable Dialogs > Colors (the FG/BG dockable - always
# shows a live hex triplet for whichever of FG/BG is active, FG by default),
# reads it (expect 000000, gimp's default fg is black), fires Tools > Swap
# Colors, checks the same hex entry now reads ffffff.
#
# toolbox's own FG/BG swatch is a custom-drawn widget, no AT-SPI children to
# click - so we leave it alone. the Colors dockable's hex entry is a plain
# GtkEntry that tracks context fg/bg-changed signals, so a menu-driven swap
# shows up there for free.
#
# args: <gimp-exe> <mutter-exe>  (gimpmain_exe, mutter_prog, see meson)
# exit: 0 pass, 1 fail, 99 hard error (infra bug: mutter/dogtail/session boot)

import re
import sys
import time

from uicommon import close_welcome_dialog, dump_tree, fail, find_all, find_first, \
    gimp_session, wait_for

GIMP = sys.argv[1]
MUTTER = sys.argv[2]

HEX_RE = re.compile(r"^[0-9a-fA-F]{6}$")

with gimp_session(GIMP, MUTTER) as (session, app, proc):
    close_welcome_dialog(app)

    # --- step 2: menu helpers ------------------------------------------

    def click_menu_path(*names):
        # scope each hop to the node we just clicked, not the whole app -
        # there's a top-level "Colors" menu (image adjustments) before
        # "Windows" in the bar, a from-root search grabs that one first.
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
        fail("expected default foreground 000000 before swap, got %r" % before)

    # --- step 5: Tools > Swap Colors ---------------------------------------

    click_menu_path("Tools", "Swap Colors")

    # --- step 6: hex entry should now read ffffff -----------------------

    def after_swap():
        try:
            txt = hex_entry.text.strip().lower()
        except Exception:
            return None
        return txt if txt == "ffffff" else None

    after = wait_for(after_swap, 15)
    if after is None:
        fail("expected foreground hex to become ffffff after swap, still %r"
             % hex_entry.text)

    sys.stderr.write("fg hex %r -> %r after swap, ok\n" % (before, after))

sys.exit(0)
