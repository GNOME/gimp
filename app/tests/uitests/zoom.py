#!/usr/bin/env python3
#
# runs real gimp under a dogtail HermeticSession, closes the welcome dialog,
# creates a new image (File > New > OK, default size), reads the current
# zoom percentage off the View > Zoom submenu's own name (it carries the
# live value, e.g. "Zoom (100%)"), fires View > Zoom > Zoom In, and checks
# that name now reports a different percentage.
#
# large default-template images don't open at 100% (gimp fits them to the
# window), so this doesn't assume a starting value - just that Zoom In
# changes it. no script-fu socket needed, no statusbar widget-digging either.
#
# args: <gimp-exe> <mutter-exe>  (gimpmain_exe, mutter_prog, see meson)
# exit: 0 pass, 1 fail, 99 hard error (infra bug: mutter/dogtail/session boot)

import re
import sys
import time

from uicommon import close_welcome_dialog, dump_tree, fail, find_first, gimp_session, \
    wait_for

GIMP = sys.argv[1]
MUTTER = sys.argv[2]

ZOOM_MENU_RE = re.compile(r"^Zoom \(([\d.]+)%\)$")

with gimp_session(GIMP, MUTTER) as (session, app, proc):
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

    # --- step 3: File > New > OK (default size) -----------------------

    click_menu_path("File", "New...")

    dlg = wait_for(
        lambda: find_first(app, lambda n: n.roleName == "dialog"
                           and "new image" in (n.name or "").lower()),
        30)
    if dlg is None:
        fail("New Image dialog not found via AT-SPI:\n%s" % dump_tree(app))

    ok_btn = find_first(dlg, lambda n: "button" in n.roleName and n.name == "OK")
    if ok_btn is None:
        fail("New dialog OK button not found via AT-SPI:\n%s" % dump_tree(dlg))
    ok_btn.do_action_named("click")
    time.sleep(1)

    # --- step 4: read the zoom submenu's own name --------------------------

    def find_zoom_menu():
        return find_first(app, lambda n: n.roleName == "menu"
                          and ZOOM_MENU_RE.match(n.name or ""))

    zoom_menu = wait_for(find_zoom_menu, 15)
    if zoom_menu is None:
        fail("no 'Zoom (NN%%)' menu found via AT-SPI:\n%s" % dump_tree(app))

    before = ZOOM_MENU_RE.match(zoom_menu.name).group(1)

    # --- step 5: View > Zoom > Zoom In -------------------------------------

    click_menu_path("View", zoom_menu.name, "Zoom In")

    # --- step 6: zoom submenu name should now report a different value -----

    def after_zoom():
        node = find_zoom_menu()
        if node is None:
            return None
        m = ZOOM_MENU_RE.match(node.name)
        return m.group(1) if m and m.group(1) != before else None

    after = wait_for(after_zoom, 15)
    if after is None:
        fail("expected zoom to change from %s%% after Zoom In" % before)

    sys.stderr.write("zoom %s%% -> %s%% after Zoom In, ok\n" % (before, after))

sys.exit(0)
