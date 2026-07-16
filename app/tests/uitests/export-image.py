#!/usr/bin/env python3
#
# runs real gimp under a dogtail HermeticSession, closes the welcome dialog,
# makes a fresh 321x123 image via File > New, then drives File > Export As:
# sets the file chooser's name field straight through the AT-SPI EditableText
# interface (no keystrokes) and clicks Export. Checks the file actually landed
# on disk, starts with the PNG magic bytes, and carries the size we made.
#
# the hard case here: gimp's export plugins run as their own process, so the
# PNG options dialog registers as its own AT-SPI application ("file-png"), not
# nested under "gimp". do_action doesn't care - it's just another application
# on the same a11y bus, found and clicked the same way. no window/process
# tracking, no coordinate math, no cross-process transient-parent hacks.
#
# args: <gimp-exe> <mutter-exe>  (gimpmain_exe, mutter_prog, see meson)
# exit: 0 pass, 1 fail, 99 hard error (infra bug: mutter/dogtail/session boot)

import os
import shutil
import sys
import tempfile
import time

from uicommon import close_welcome_dialog, dump_tree, fail, find_all, find_first, \
    gimp_session, wait_for

GIMP = sys.argv[1]
MUTTER = sys.argv[2]

WANT_W, WANT_H = 321, 123
PNG_MAGIC = b"\x89PNG\r\n\x1a\n"

out_dir = tempfile.mkdtemp(prefix="gimp-uitest-out-")
out_file = os.path.join(out_dir, "out.png")

extra_env = {
    # GtkFileChooser's "Recent"/"Other locations" backends want
    # xdg-desktop-portal, which isn't running headless. force the
    # in-process chooser instead.
    "GTK_USE_PORTAL": "0",
}
# we launch the gui binary straight, not the in-build wrapper, so
# GIMP_TESTING_PLUG_INS (which only the wrapper reads) does nothing here -
# point gimp at the freshly built plug-ins/common (where file-png lives)
# via GIMP_TESTING_PLUGINDIRS, the var gimp core actually reads.
build_root = os.environ.get("GIMP_GLOBAL_BUILD_ROOT")
if build_root:
    extra_env["GIMP_TESTING_PLUGINDIRS"] = os.path.join(build_root, "plug-ins", "common")

try:
    with gimp_session(GIMP, MUTTER, extra_environment=extra_env) as (session, app, proc):
        close_welcome_dialog(app)

        # --- step 2: File > New, distinctive size ---------------------------

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

        click_menu_item("File", "New...")

        dlg = wait_for(
            lambda: find_first(app, lambda n: n.roleName == "dialog"
                               and "new image" in (n.name or "").lower()),
            30)
        if dlg is None:
            fail("New Image dialog not found via AT-SPI:\n%s" % dump_tree(app))

        spins = find_all(dlg, lambda n: n.roleName == "spin button")
        if len(spins) < 2:
            fail("expected at least 2 spin buttons (width, height) in New dialog, "
                 "found %d:\n%s" % (len(spins), dump_tree(dlg)))
        spins[0].value = WANT_W
        spins[1].value = WANT_H
        time.sleep(0.5)

        ok_btn = find_first(dlg, lambda n: "button" in n.roleName and n.name == "OK")
        if ok_btn is None:
            fail("New dialog OK button not found via AT-SPI:\n%s" % dump_tree(dlg))
        ok_btn.do_action_named("click")
        time.sleep(2)

        # --- step 3: File > Export As, type the path, Export -----------------

        click_menu_item("File", "Export As...")

        chooser = wait_for(
            lambda: find_first(app, lambda n: n.roleName == "file chooser"
                               and n.name == "Export Image"),
            30)
        if chooser is None:
            fail("Export Image file chooser not found via AT-SPI:\n%s" % dump_tree(app))

        name_field = find_first(chooser, lambda n: n.roleName == "text")
        if name_field is None:
            fail("no editable name field in export chooser:\n%s" % dump_tree(chooser))
        name_field.text = out_file

        export_btn = find_first(chooser, lambda n: "button" in n.roleName
                                and n.name == "Export")
        if export_btn is None:
            fail("no Export button in export chooser:\n%s" % dump_tree(chooser))
        export_btn.do_action_named("click")

        # --- step 4: PNG options dialog - separate at-spi app "file-png" -----

        import dogtail.tree  # noqa: PLC0415 - only safe once session.boot() has set env

        png_app = wait_for(
            lambda: next((c for c in dogtail.tree.root.children if c.name == "file-png"),
                         None),
            30)
        if png_app is None:
            fail("file-png plugin never registered its own AT-SPI application - "
                 "either the plugin didn't run, or this build doesn't split "
                 "exporters into a separate process")

        png_dlg = find_first(png_app, lambda n: n.roleName == "dialog")
        if png_dlg is None:
            fail("file-png registered but has no dialog:\n%s" % dump_tree(png_app))

        png_export_btn = find_first(png_dlg, lambda n: "button" in n.roleName
                                    and n.name == "Export")
        if png_export_btn is None:
            fail("no Export button in PNG options dialog:\n%s" % dump_tree(png_dlg))
        png_export_btn.do_action_named("click")

        # --- step 5: the file must be on disk and be a real PNG --------------

        def written():
            if os.path.exists(out_file) and os.path.getsize(out_file) > 0:
                return out_file
            return None

        if wait_for(written, 20) is None:
            fail("export produced no file at %s" % out_file)

        with open(out_file, "rb") as f:
            head = f.read(24)
        if head[:8] != PNG_MAGIC:
            fail("file at %s is not a PNG (header %r)" % (out_file, head[:8]))

        # png IHDR is the first chunk: 8 magic, 4 length, 4 "IHDR", then width,
        # height as big-endian uint32 - read them off and check they're the size
        # we typed, so this is our image, not a default template or stray file.
        if head[12:16] != b"IHDR":
            fail("png at %s has no IHDR where expected (%r)" % (out_file, head[12:16]))
        w = int.from_bytes(head[16:20], "big")
        h = int.from_bytes(head[20:24], "big")
        if (w, h) != (WANT_W, WANT_H):
            fail("exported png is %dx%d, expected %dx%d" % (w, h, WANT_W, WANT_H))

        sys.stderr.write("exported %s (%d bytes), %dx%d png, ok\n"
                         % (out_file, os.path.getsize(out_file), w, h))
finally:
    shutil.rmtree(out_dir, ignore_errors=True)

sys.exit(0)
