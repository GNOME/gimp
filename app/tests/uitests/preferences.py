#!/usr/bin/env python3
#
# runs real gimp under a dogtail HermeticSession, closes welcome dialog, opens
# Edit > Preferences, bumps the "Minimal number of undo levels" spinner and
# hits OK, then greps the gimprc in the throwaway config dir for the value.
#
# gimprc only gets written by OK (gimp_rc_save in preferences-dialog.c), so
# the file itself is the proof the dialog was accepted.
#
# args: <gimp-exe> <mutter-exe>  (gimpmain_exe, mutter_prog, see meson)
# exit: 0 pass, 1 fail, 99 hard error (infra bug: mutter/dogtail/session boot)

import os
import sys
import time

from uicommon import close_welcome_dialog, dump_tree, fail, find_first, gimp_session, \
    wait_for

GIMP = sys.argv[1]
MUTTER = sys.argv[2]

SPIN_LABEL = "undo levels"
WANT_UNDO_LEVELS = 7

with gimp_session(GIMP, MUTTER) as (session, app, proc):
    close_welcome_dialog(app)

    # --- step 2: Edit > Preferences -----------------------------------------

    edit = find_first(app, lambda n: n.roleName == "menu" and n.name == "Edit")
    if edit is None:
        fail("Edit menu not found via AT-SPI:\n%s" % dump_tree(app))
    edit.do_action_named("click")

    item = wait_for(
        lambda: find_first(app, lambda n: n.roleName == "menu item"
                           and n.name == "Preferences"),
        10)
    if item is None:
        fail("Preferences menu item not found under Edit")
    item.do_action_named("click")

    prefs = wait_for(
        lambda: find_first(app, lambda n: n.roleName == "dialog"
                           and (n.name or "").strip() == "Preferences"),
        30)
    if prefs is None:
        fail("Preferences dialog not found via AT-SPI:\n%s" % dump_tree(app))

    # --- step 3: bump the undo-levels spin button ---------------------------

    # sticks to the page prefs opens on, System Resources - the notebook only
    # hands out the visible page and picking another row in the left tree over
    # AT-SPI won't move it.
    #
    # spin buttons here have no name and no labelled-by that works, the text is
    # on a plain label right after them in the grid - find that, walk back.
    #
    # the page's widgets can still be filling in when the dialog shows up,
    # so wait for the label.
    label = wait_for(
        lambda: find_first(prefs, lambda n: n.roleName == "label"
                           and SPIN_LABEL in (n.name or ""), max_depth=25),
        15)
    if label is None:
        fail("no %r label in Preferences:\n%s" % (SPIN_LABEL, dump_tree(prefs)))

    spin = None
    siblings = label.parent.children
    for node in reversed(siblings[:siblings.index(label)]):
        if node.roleName == "spin button":
            spin = node
            break
    if spin is None:
        fail("%r spin button not found in Preferences:\n%s"
             % (SPIN_LABEL, dump_tree(prefs)))

    if int(spin.value) == WANT_UNDO_LEVELS:
        fail("undo levels already %d, bump wouldn't prove anything"
             % WANT_UNDO_LEVELS)
    spin.value = WANT_UNDO_LEVELS
    time.sleep(1)
    if int(spin.value) != WANT_UNDO_LEVELS:
        fail("spin button didn't take the new value, reads %r" % spin.value)

    # --- step 4: OK, then read gimprc back ----------------------------------

    ok = find_first(prefs, lambda n: "button" in n.roleName and n.name == "OK",
                    max_depth=10)
    if ok is None:
        fail("Preferences dialog has no OK button:\n%s" % dump_tree(prefs))
    ok.do_action_named("click")

    gimprc = os.path.join(session.config_dir, "gimprc")

    def saved():
        try:
            with open(gimprc) as f:
                text = f.read()
        except OSError:
            return None
        for line in text.splitlines():
            if "undo-levels" in line:
                return line.strip()
        return None

    line = wait_for(saved, 20)
    if line is None:
        fail("gimprc never grew an undo-levels entry after OK (%s)" % gimprc)
    if line != "(undo-levels %d)" % WANT_UNDO_LEVELS:
        fail("expected undo-levels %d in gimprc, got: %r" % (WANT_UNDO_LEVELS, line))
    sys.stderr.write("gimprc says %r, ok\n" % line)

sys.exit(0)
