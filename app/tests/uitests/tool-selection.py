#!/usr/bin/env python3
#
# boots real gimp under hermetic dogtail, closes welcome dialog, new 64x64
# image (white bg), clicks Bucket Fill in the actual toolbox (not the Tools
# menu), fullscreen, single click on canvas, export to png, check the whole
# thing flooded to fg color.
#
# single click not a drag, drags flaked out under headless mutter, plain
# click is fine (already leaned on for warmup clicks elsewhere). bucket fill
# on a flat canvas floods the whole thing in one hit so no need to guess
# where the click landed, corner pixel tells the story either way.
#
# no way to ask gimp "what tool is active right now" - so this only proves
# the toolbox button worked by checking what it did, same trick the rest of
# this suite uses instead of a script-fu socket.
#
# args: <gimp-exe> <mutter-exe>  (gimpmain_exe, mutter_prog, see meson)
# exit: 0 pass, 1 fail, 99 hard error (infra bug: mutter/dogtail/session boot)

import os
import shutil
import struct
import sys
import tempfile
import time
import zlib

# dogtail.rawinput freezes its injection path at import time off
# XDG_SESSION_TYPE - unset (headless CI) means it assumes x11 and routes
# through Atspi.generate_mouse_event, which needs an X server our headless
# --no-x11 mutter doesn't have. force wayland before the import.
os.environ["XDG_SESSION_TYPE"] = "wayland"

from uicommon import dump_tree, fail, find_all, find_first, gimp_session, wait_for

from dogtail.rawinput import click, press_key

GIMP = sys.argv[1]
MUTTER = sys.argv[2]

WANT_W, WANT_H = 64, 64
PNG_MAGIC = b"\x89PNG\r\n\x1a\n"


def fail_with_dump(msg, dump):
    sys.stderr.write("--- AT-SPI tree at failure ---\n%s\n" % dump)
    sys.stderr.write("FAIL: %s\n" % msg)
    sys.exit(1)


def unfilter_row(filter_type, row, prev_row, bpp):
    out = bytearray(row)
    for i in range(len(out)):
        a = out[i - bpp] if i >= bpp else 0
        b = prev_row[i] if prev_row else 0
        c = prev_row[i - bpp] if (prev_row and i >= bpp) else 0
        if filter_type == 0:
            pass
        elif filter_type == 1:
            out[i] = (out[i] + a) & 0xFF
        elif filter_type == 2:
            out[i] = (out[i] + b) & 0xFF
        elif filter_type == 3:
            out[i] = (out[i] + (a + b) // 2) & 0xFF
        elif filter_type == 4:
            p = a + b - c
            pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
            pred = a if (pa <= pb and pa <= pc) else (b if pb <= pc else c)
            out[i] = (out[i] + pred) & 0xFF
        else:
            fail("unknown PNG filter type %d" % filter_type)
    return bytes(out)


def read_png(path):
    """walk chunks for IHDR (dims/color type) and IDAT (concatenated,
    zlib-compressed scanlines), then undo the per-row filter. no
    interlacing/palette support - not needed for a plain RGB(A) export."""
    with open(path, "rb") as f:
        data = f.read()
    if data[:8] != PNG_MAGIC:
        fail("exported file has no PNG magic (got %r)" % data[:8])

    idat = b""
    w = h = color_type = None
    i = 8
    while i < len(data):
        (length,) = struct.unpack(">I", data[i:i + 4])
        ctype = data[i + 4:i + 8]
        chunk = data[i + 8:i + 8 + length]
        if ctype == b"IHDR":
            w, h, _bitdepth, color_type, _comp, _filt, interlace = \
                struct.unpack(">IIBBBBB", chunk)
            if interlace != 0:
                fail("exported PNG is interlaced, this reader doesn't handle that")
        elif ctype == b"IDAT":
            idat += chunk
        elif ctype == b"IEND":
            break
        i += 8 + length + 4

    if w is None:
        fail("exported PNG has no IHDR chunk")

    bpp = {2: 3, 6: 4}.get(color_type)
    if bpp is None:
        fail("exported PNG has unsupported color type %d (want RGB/RGBA)" % color_type)

    raw = zlib.decompress(idat)
    stride = w * bpp
    rows = []
    prev = None
    pos = 0
    for _ in range(h):
        ftype = raw[pos]
        row = unfilter_row(ftype, raw[pos + 1:pos + 1 + stride], prev, bpp)
        rows.append(row)
        prev = row
        pos += 1 + stride

    def pixel(x, y):
        off = x * bpp
        return rows[y][off], rows[y][off + 1], rows[y][off + 2]

    return w, h, pixel


out_dir = tempfile.mkdtemp(prefix="gimp-uitest-out-")
out_file = os.path.join(out_dir, "out.png")

extra_env = {
    "GTK_USE_PORTAL": "0",
}
build_root = os.environ.get("GIMP_GLOBAL_BUILD_ROOT")
if build_root:
    extra_env["GIMP_TESTING_PLUGINDIRS"] = os.path.join(build_root, "plug-ins", "common")

try:
    with gimp_session(GIMP, MUTTER, extra_environment=extra_env, install_input=True) \
            as (session, app, proc):
        # --- step 1: close the welcome dialog -----------------------------

        welcome = wait_for(
            lambda: find_first(app, lambda n: n.roleName == "dialog"
                               and "welcome" in (n.name or "").lower()),
            60)
        if welcome is not None:
            close_btn = wait_for(
                lambda: find_first(welcome, lambda n: "button" in n.roleName
                                   and n.name == "Close"),
                10)
            if close_btn is None:
                fail_with_dump("welcome dialog Close button not found", dump_tree(welcome))
            close_btn.do_action_named("click")
            time.sleep(1)

        def click_menu_item(menu_name, item_name):
            menu = find_first(app, lambda n: n.roleName == "menu" and n.name == menu_name)
            if menu is None:
                fail_with_dump("menu %r not found via AT-SPI" % menu_name, dump_tree(app))
            menu.do_action_named("click")

            item = wait_for(
                lambda: find_first(app, lambda n: "menu item" in n.roleName
                                   and n.name == item_name),
                10)
            if item is None:
                fail("menu item %r not found under %r" % (item_name, menu_name))
            item.do_action_named("click")

        def click_submenu_item(menu_name, submenu_name, item_name):
            menu = find_first(app, lambda n: n.roleName == "menu" and n.name == menu_name)
            if menu is None:
                fail_with_dump("menu %r not found via AT-SPI" % menu_name, dump_tree(app))
            menu.do_action_named("click")

            # some submenu labels bake in live state, e.g. View's "Zoom" shows
            # as "Zoom (100%)" - match by prefix, not exact.
            submenu = wait_for(
                lambda: find_first(app, lambda n: n.roleName == "menu"
                                   and (n.name or "").startswith(submenu_name)),
                10)
            if submenu is None:
                fail_with_dump("%r > %r submenu not found" % (menu_name, submenu_name),
                               dump_tree(app))
            submenu.do_action_named("click")

            item = wait_for(
                lambda: find_first(app, lambda n: "menu item" in n.roleName
                                   and n.name == item_name),
                10)
            if item is None:
                fail_with_dump("%r not found under %r > %r"
                               % (item_name, menu_name, submenu_name), dump_tree(app))
            item.do_action_named("click")

        # --- step 2: File > New, 64x64 -------------------------------------

        click_menu_item("File", "New...")

        dlg = wait_for(
            lambda: find_first(app, lambda n: n.roleName == "dialog"
                               and "new image" in (n.name or "").lower()),
            30)
        if dlg is None:
            fail_with_dump("New Image dialog not found via AT-SPI", dump_tree(app))

        spins = find_all(dlg, lambda n: n.roleName == "spin button")
        if len(spins) < 2:
            fail("expected at least 2 spin buttons (width, height) in New dialog, "
                 "found %d" % len(spins))
        spins[0].value = WANT_W
        spins[1].value = WANT_H
        time.sleep(0.5)

        ok_btn = find_first(dlg, lambda n: "button" in n.roleName and n.name == "OK")
        if ok_btn is None:
            fail_with_dump("New dialog OK button not found via AT-SPI", dump_tree(dlg))
        ok_btn.do_action_named("click")
        time.sleep(2)

        # --- step 3: click Bucket Fill in the toolbox itself --------------------

        # toolbox tool buttons have empty name and empty description - checked
        # every attribute on a live run, nothing usable there. the tooltip text
        # ("Bucket Fill Tool: ...") actually sits on the *panel wrapping* the
        # button, not the button itself. so: find the panel by its description,
        # then grab the one toggle button inside it.
        bucket_fill_panel = wait_for(
            lambda: find_first(app, lambda n: n.roleName == "panel"
                               and "bucket fill" in (n.description or "").lower(),
                               max_depth=30),
            15)
        if bucket_fill_panel is None:
            fail_with_dump("Bucket Fill tool panel not found in toolbox via AT-SPI",
                           dump_tree(app, max_depth=20))
        bucket_fill = find_first(bucket_fill_panel, lambda n: n.roleName == "toggle button")
        if bucket_fill is None:
            fail_with_dump("Bucket Fill panel has no toggle-button child",
                           dump_tree(bucket_fill_panel))
        bucket_fill.do_action_named("click")
        time.sleep(0.3)

        # --- step 4: fullscreen so screen coords are trustworthy ---------------

        frame = session.fullscreen(app)
        if frame is None:
            fail("session.fullscreen() returned no frame")

        # scales the tiny 64x64 canvas up to fill the viewport - without this
        # the click lands on empty canvas background, not the layer.
        click_submenu_item("View", "Zoom", "Fit Image in Window")
        time.sleep(3)  # reflow after this needs real settle time

        fx, fy, fw, fh = frame.extents
        cx, cy = fx + fw // 2, fy + fh // 2
        sys.stderr.write("frame.extents=(%d,%d,%d,%d) click=(%d,%d)\n"
                         % (fx, fy, fw, fh, cx, cy))

        # --- step 5: use the tool - single click, no drag -----------------------

        # first real pointer events after install_input() get dropped for a bit -
        # burn a few warmup clicks on the File menu label first.
        for _ in range(5):
            click(fx + 20, fy + 13)
            time.sleep(0.3)
            press_key("Escape")
            time.sleep(0.3)

        click(cx, cy)
        time.sleep(1)

        # --- step 6: File > Export As a .png, verify the whole canvas flooded --

        click_menu_item("File", "Export As...")

        chooser = wait_for(
            lambda: find_first(app, lambda n: n.roleName == "file chooser"
                               and n.name == "Export Image"),
            30)
        if chooser is None:
            fail_with_dump("Export Image file chooser not found via AT-SPI", dump_tree(app))

        name_field = find_first(chooser, lambda n: n.roleName == "text")
        if name_field is None:
            fail_with_dump("no editable name field in export chooser", dump_tree(chooser))
        name_field.text = out_file

        export_btn = find_first(chooser, lambda n: "button" in n.roleName
                                and n.name == "Export")
        if export_btn is None:
            fail_with_dump("no Export button in export chooser", dump_tree(chooser))
        export_btn.do_action_named("click")

        import dogtail.tree  # noqa: PLC0415 - only safe once session.boot() has set env

        png_app = wait_for(
            lambda: next((c for c in dogtail.tree.root.children if c.name == "file-png"),
                         None),
            30)
        if png_app is None:
            fail("file-png plugin never registered its own AT-SPI application")

        png_dlg = find_first(png_app, lambda n: n.roleName == "dialog")
        if png_dlg is None:
            fail_with_dump("file-png registered but has no dialog", dump_tree(png_app))

        png_export_btn = find_first(png_dlg, lambda n: "button" in n.roleName
                                    and n.name == "Export")
        if png_export_btn is None:
            fail_with_dump("no Export button in PNG options dialog", dump_tree(png_dlg))
        png_export_btn.do_action_named("click")

        def written():
            if os.path.exists(out_file) and os.path.getsize(out_file) > 0:
                return out_file
            return None

        if wait_for(written, 20) is None:
            fail("export produced no file at %s" % out_file)

        w, h, pixel = read_png(out_file)
        if (w, h) != (WANT_W, WANT_H):
            fail("exported png is %dx%d, expected %dx%d" % (w, h, WANT_W, WANT_H))

        # bucket fill on a flat white canvas floods the whole thing to the
        # default foreground (black) - check a corner, no need to guess where
        # the click landed.
        corner = pixel(0, 0)
        if sum(corner) > 300:
            fail("corner pixel (0,0) is %r after Bucket Fill click, expected it "
                 "flooded dark - toolbox button may not have selected the tool"
                 % (corner,))

        sys.stderr.write("bucket fill flooded %dx%d png, corner now %r, ok\n"
                         % (w, h, corner))
finally:
    shutil.rmtree(out_dir, ignore_errors=True)

sys.exit(0)
