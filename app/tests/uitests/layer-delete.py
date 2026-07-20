#!/usr/bin/env python3
#
# runs real gimp under a dogtail HermeticSession, closes the welcome dialog,
# makes a fresh image via File > New (1 layer), adds a layer via Layer > New
# Layer... (2 layers), then drives Layer > Delete Layers and checks the
# title drops back to "1 layer".
#
# no script-fu socket needed - gimp's title format spells out layer count in
# words (see gimpdisplayshell-title.c, %L), so the frame's AT-SPI name is
# the readback for free.
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

    def wait_for_layer_count(text, timeout):
        def check():
            win = image_window()
            if win is not None and text in (win.name or ""):
                return win
            return None
        return wait_for(check, timeout)

    # --- step 3: File > New, expect "1 layer" -------------------------------

    click_menu_item("File", "New...")
    confirm_dialog("new image")

    win = wait_for(image_window, 15)
    if win is None:
        fail("image window not found via AT-SPI:\n%s" % dump_tree(app))
    if "1 layer" not in win.name:
        fail("expected a fresh image to report '1 layer', got: %r" % win.name)

    # --- step 4: Layer > New Layer..., expect "2 layers" --------------------

    click_menu_item("Layer", "New Layer...")
    confirm_dialog("new layer")

    win = wait_for_layer_count("2 layers", 15)
    if win is None:
        fail("expected '2 layers' in image window title after adding a layer:\n%s"
             % dump_tree(app))

    # --- step 5: Layer > Delete Layers, expect "1 layer" ---------------------

    click_menu_item("Layer", "Delete Layers")

    win = wait_for_layer_count("1 layer", 15)
    if win is None:
        fail("expected '1 layer' in image window title after deleting a layer:\n%s"
             % dump_tree(app))

    sys.stderr.write("image title now %r, ok\n" % win.name)

sys.exit(0)
