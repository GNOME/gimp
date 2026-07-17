#!/usr/bin/env python3
#
# runs real gimp under a dogtail HermeticSession, closes the welcome dialog,
# makes a fresh image via File > New, adds a layer via Layer > New Layer...
# (title goes "1 layer" -> "2 layers"), then Edit > Undo New layer (back to
# "1 layer") and Edit > Redo New layer (back to "2 layers"). the undo/redo
# menu item label is built from the undo stack entry's name (gimp_image_undo
# _group_start's desc arg, "New layer"/"New layers" via ngettext), not the
# action that triggered it - see edit-actions.c edit_actions_update.
#
# same title-encodes-layer-count readback as layer-add.py, no script-fu
# socket needed.
#
# args: <gimp-exe> <mutter-exe>  (gimpmain_exe, mutter_prog, see meson)
# exit: 0 pass, 1 fail, 99 hard error (infra bug: mutter/dogtail/session boot)

import sys

from uicommon import close_welcome_dialog, dump_tree, fail, find_first, gimp_session, \
    wait_for

GIMP = sys.argv[1]
MUTTER = sys.argv[2]

with gimp_session(GIMP, MUTTER) as (session, app, proc):
    close_welcome_dialog(app)

    # --- step 2: menu + dialog helpers ---------------------------------

    def click_menu_item(menu_name, item_name):
        menu = find_first(app, lambda n: n.roleName == "menu" and n.name == menu_name)
        if menu is None:
            fail("menu %r not found via AT-SPI:\n%s" % (menu_name, dump_tree(app)))
        menu.do_action_named("click")

        item = wait_for(
            lambda: find_first(app, lambda n: n.roleName == "menu item"
                               and n.name == item_name),
            10)
        if item is None:
            fail("menu item %r not found under %r" % (item_name, menu_name))
        item.do_action_named("click")

    def confirm_dialog(name_substr, button_name="OK"):
        dlg = wait_for(
            lambda: find_first(app, lambda n: n.roleName == "dialog"
                               and name_substr in (n.name or "").lower()),
            30)
        if dlg is None:
            fail("%r dialog not found via AT-SPI:\n%s" % (name_substr, dump_tree(app)))
        btn = find_first(dlg, lambda n: "button" in n.roleName and n.name == button_name)
        if btn is None:
            fail("%r dialog has no %r button:\n%s"
                 % (name_substr, button_name, dump_tree(dlg)))
        btn.do_action_named("click")

    def image_window():
        return find_first(app, lambda n: n.roleName == "frame"
                          and "layer" in (n.name or "").lower())

    def wait_for_title(substr, timeout=15):
        def check():
            win = image_window()
            if win is not None and substr in (win.name or ""):
                return win
            return None
        return wait_for(check, timeout)

    # --- step 3: File > New, expect "1 layer" -------------------------------

    click_menu_item("File", "New...")
    confirm_dialog("new image")

    win = wait_for_title("1 layer")
    if win is None:
        fail("expected a fresh image to report '1 layer', got:\n%s" % dump_tree(app))

    # --- step 4: Layer > New Layer..., expect "2 layers" --------------------

    click_menu_item("Layer", "New Layer...")
    confirm_dialog("new layer")

    win = wait_for_title("2 layers")
    if win is None:
        fail("expected '2 layers' in image window title after adding a layer:\n%s"
             % dump_tree(app))

    # --- step 5: Edit > Undo, expect back to "1 layer" -----------------------

    click_menu_item("Edit", "Undo New layer")

    win = wait_for_title("1 layer")
    if win is None:
        fail("expected '1 layer' in image window title after undo:\n%s"
             % dump_tree(app))

    # --- step 6: Edit > Redo, expect "2 layers" again -------------------------

    click_menu_item("Edit", "Redo New layer")

    win = wait_for_title("2 layers")
    if win is None:
        fail("expected '2 layers' in image window title after redo:\n%s"
             % dump_tree(app))

    sys.stderr.write("image title now %r, ok\n" % win.name)

sys.exit(0)
