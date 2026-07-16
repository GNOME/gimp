#!/usr/bin/env python3
#
# runs real gimp under a dogtail HermeticSession, closes the welcome dialog,
# opens File > New, sets a weird size (321x123) directly on the width/height
# spin buttons via the AT-SPI Value interface, clicks OK, and checks the new
# image window title carries that same size back.
#
# no keystrokes needed - gtk spin buttons expose AT-SPI's Value interface, so
# dogtail's Node.value setter drives them straight, no click-to-focus/select-all/
# type dance. no script-fu socket either - gimp's default image-title format
# ends in "%wx%h", so the frame title in the a11y tree already spells out the
# dimensions, we just read them off. distinctive 321x123 so a pass means we set
# our value, not some default template size.
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

WANT_W, WANT_H = 321, 123
DIMS = re.compile(r"(\d+)x(\d+)")

with gimp_session(GIMP, MUTTER) as (session, app, proc):
    close_welcome_dialog(app)

    # --- step 2: File > New ------------------------------------------------

    menu = find_first(app, lambda n: n.roleName == "menu" and n.name == "File")
    if menu is None:
        fail("File menu not found via AT-SPI:\n%s" % dump_tree(app))
    menu.do_action_named("click")

    item = wait_for(
        lambda: find_first(app, lambda n: n.roleName == "menu item"
                           and n.name == "New..."),
        10)
    if item is None:
        fail("File > New menu item not found via AT-SPI")
    item.do_action_named("click")

    dlg = wait_for(
        lambda: find_first(app, lambda n: n.roleName == "dialog"
                           and "new image" in (n.name or "").lower()),
        30)
    if dlg is None:
        fail("New Image dialog not found via AT-SPI:\n%s" % dump_tree(app))

    # --- step 3: set width/height straight through the Value interface -----

    spins = find_all(dlg, lambda n: n.roleName == "spin button")
    if len(spins) < 2:
        fail("expected at least 2 spin buttons (width, height) in New dialog, "
             "found %d:\n%s" % (len(spins), dump_tree(dlg)))

    # dialog's tab order lists width first, then height.
    spins[0].value = WANT_W
    spins[1].value = WANT_H
    time.sleep(0.5)

    ok_btn = find_first(dlg, lambda n: "button" in n.roleName and n.name == "OK")
    if ok_btn is None:
        fail("New dialog OK button not found via AT-SPI:\n%s" % dump_tree(dlg))
    ok_btn.do_action_named("click")

    # --- step 4: read the size back off the image window title -------------

    def title_dims():
        frames = find_all(app, lambda n: n.roleName == "frame")
        for f in frames:
            m = None
            for m in DIMS.finditer(f.name or ""):
                pass
            if m:
                return int(m.group(1)), int(m.group(2)), f.name
        return None

    got = wait_for(title_dims, 15)
    if got is None:
        fail("no image window title with WxH found via AT-SPI:\n%s" % dump_tree(app))

    w, h, title = got
    sys.stderr.write("image title %r -> %dx%d\n" % (title, w, h))
    if (w, h) != (WANT_W, WANT_H):
        fail("expected %dx%d, title reports %dx%d" % (WANT_W, WANT_H, w, h))

sys.exit(0)
