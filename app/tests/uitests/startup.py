#!/usr/bin/env python3
#
# runs real gimp under a dogtail HermeticSession (private dbus + at-spi2-registryd
# + headless mutter, see dogtail.hermetic) and checks it comes up clean: it
# registers on the a11y bus, gets past the splash to its main window, and the
# process didn't die on the way.
#
# no clicking here - this is the "does it even start" smoke test the rest of the
# ui suite leans on. if this fails the others are noise.
#
# args: <gimp-exe> <mutter-exe>  (gimpmain_exe, mutter_prog, see meson)
# exit: 0 pass, 1 fail, 99 hard error (infra bug: mutter/dogtail/session boot)

import sys

from uicommon import dump_tree, fail, find_first, gimp_session, wait_for

GIMP = sys.argv[1]
MUTTER = sys.argv[2]

with gimp_session(GIMP, MUTTER) as (session, app, proc):
    # at registration only the splash is up and gimp is still loading. a
    # SIGTERM mid-load can deadlock its signal handler, so wait for the main
    # window, which only appears once loading is done.
    main_window = wait_for(
        lambda: find_first(app, lambda n: n.roleName == "frame"
                           and n.name == "GNU Image Manipulation Program",
                           max_depth=1),
        60)
    if main_window is None:
        fail("main window never showed up after the splash:\n%s"
             % dump_tree(app))

    sys.stderr.write("startup ok: %r registered, main window up\n" % app.name)

    # a window that appears then the process crashes is still a failed startup
    if proc.poll() is not None:
        fail("gimp exited (rc=%s) right after its main window came up"
             % proc.returncode)

sys.exit(0)
