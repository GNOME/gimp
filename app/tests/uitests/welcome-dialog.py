#!/usr/bin/env python3
#
# welcome dialog show/hide rules across restarts. one hermetic session, gimp
# quit and relaunched 5 times on the same config dir:
#
# -> 1st start (fresh config counts as an update): Welcome tab, with
#    "You installed GIMP x.y.z"
# -> 2nd start: Create tab
# -> 3rd start with an image on the command line: no dialog
# -> 4th start: Create tab again, uncheck "Show on Start"
# -> 5th start: no dialog
# -> Release Notes tab only exists on stable (even minor) versions
#
# quits through File > Quit, not SIGTERM, so gimprc gets written like a real
# run. the session is one per process (libatspi limit), so relaunch inside it.
#
# args: <gimp-exe> <mutter-exe>  (gimpmain_exe, mutter_prog, see meson)
# exit: 0 pass, 1 fail, 99 hard error (infra bug: mutter/dogtail/session boot)

import os
import subprocess
import sys

from uicommon import dump_tree, fail, find_all, find_first, gimp_session, wait_for

GIMP = sys.argv[1]
MUTTER = sys.argv[2]

IMAGE = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                     "..", "files", "gimp-2-6-file.xcf")

# "GNU Image Manipulation Program version 3.3.1"
VERSION = subprocess.run([GIMP, "--version"], capture_output=True,
                         text=True).stdout.split()[-1]
STABLE = int(VERSION.split(".")[1]) % 2 == 0


def welcome_dialog(app):
    return find_first(app, lambda n: n.roleName == "dialog"
                      and "welcome" in (n.name or "").lower())


def tabs(dlg):
    return find_all(dlg, lambda n: n.roleName == "radio button")


def wait_for_dialog(app, run):
    dlg = wait_for(lambda: welcome_dialog(app), 60)
    if dlg is None:
        fail("%s: welcome dialog not found:\n%s" % (run, dump_tree(app)))
    # dialog can show up in the tree before its children.
    if wait_for(lambda: tabs(dlg) or None, 10) is None:
        fail("%s: welcome dialog has no tabs:\n%s" % (run, dump_tree(dlg)))
    return dlg


def check_tabs(dlg, run, want_selected):
    selected = [t.name for t in tabs(dlg) if t.checked]
    if selected != [want_selected]:
        fail("%s: expected %r tab selected, got %r" % (run, want_selected, selected))

    has_notes = any(t.name == "Release Notes" for t in tabs(dlg))
    if has_notes != STABLE:
        fail("%s: Release Notes tab %s on %s version %s"
             % (run, "present" if has_notes else "missing",
                "stable" if STABLE else "unstable", VERSION))


def close_dialog(app, dlg, run):
    close_btn = find_first(dlg, lambda n: "button" in n.roleName
                           and n.name == "Close")
    if close_btn is None:
        fail("%s: welcome dialog Close button not found:\n%s"
             % (run, dump_tree(dlg)))
    close_btn.do_action_named("click")
    if wait_for(lambda: True if welcome_dialog(app) is None else None, 10) is None:
        fail("%s: welcome dialog still there after Close" % run)


def expect_no_dialog(app, run, frame_name=""):
    win = wait_for(lambda: find_first(app, lambda n: n.roleName == "frame"
                                      and frame_name in (n.name or "")), 60)
    if win is None:
        fail("%s: no gimp window with %r in its name:\n%s"
             % (run, frame_name, dump_tree(app)))
    if wait_for(lambda: welcome_dialog(app), 10) is not None:
        fail("%s: welcome dialog showed up, it shouldn't" % run)


def quit_gimp(app, proc, run):
    import dogtail.tree  # binds root at import, only safe after boot

    item = find_first(app, lambda n: n.roleName == "menu item"
                      and n.name == "Quit", max_depth=20)
    if item is None:
        fail("%s: File > Quit not found:\n%s" % (run, dump_tree(app)))
    item.do_action_named("click")
    try:
        proc.wait(timeout=30)
    except subprocess.TimeoutExpired:
        fail("%s: gimp didn't exit after File > Quit" % run)
    # wait_for_app takes the first match, don't let it grab the dead one.
    gone = wait_for(lambda: None if any("gimp" in (c.name or "").lower()
                                        for c in dogtail.tree.root.children)
                    else True, 10)
    if gone is None:
        fail("%s: gimp still on the a11y bus after quitting" % run)


with gimp_session(GIMP, MUTTER) as (session, app, proc):
    env = {"GIMP3_DIRECTORY": session.config_dir}

    def relaunch(*args):
        proc = session.launch([GIMP, "--new-instance", *args],
                              extra_environment=env)
        return session.wait_for_app("gimp", timeout=120), proc

    run = "1st start"
    dlg = wait_for_dialog(app, run)
    check_tabs(dlg, run, "Welcome")
    installed = "You installed GIMP %s" % VERSION
    if find_first(dlg, lambda n: n.roleName == "label"
                  and n.name == installed, max_depth=40) is None:
        fail("%s: no %r label in the Welcome tab:\n%s"
             % (run, installed, dump_tree(dlg)))
    close_dialog(app, dlg, run)
    quit_gimp(app, proc, run)

    run = "2nd start"
    app, proc = relaunch()
    dlg = wait_for_dialog(app, run)
    check_tabs(dlg, run, "Create")
    close_dialog(app, dlg, run)
    quit_gimp(app, proc, run)

    run = "3rd start (with image)"
    app, proc = relaunch(IMAGE)
    expect_no_dialog(app, run, "gimp-2-6-file")
    quit_gimp(app, proc, run)

    run = "4th start"
    app, proc = relaunch()
    dlg = wait_for_dialog(app, run)
    check_tabs(dlg, run, "Create")
    toggle = find_first(dlg, lambda n: n.roleName == "check box"
                        and (n.name or "").startswith("Show on Start"),
                        max_depth=40)
    if toggle is None:
        fail("%s: Show on Start check box not found:\n%s" % (run, dump_tree(dlg)))
    if not toggle.checked:
        fail("%s: Show on Start should start checked" % run)
    toggle.do_action_named("click")
    if wait_for(lambda: True if not toggle.checked else None, 5) is None:
        fail("%s: Show on Start still checked after click" % run)
    close_dialog(app, dlg, run)
    quit_gimp(app, proc, run)

    run = "5th start"
    app, proc = relaunch()
    expect_no_dialog(app, run)
    quit_gimp(app, proc, run)

sys.exit(0)
