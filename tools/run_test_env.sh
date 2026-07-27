#!/bin/sh
#
# Wrapper script to use for the Meson test setup.
#
# Define the "UI_TEST" for all tests that should run headless

if [ -n "${UI_TEST}" ]; then
  # Use Xvfb to simulate a graphical session; note that this needs
  # a new enough version which has the -d option.
  #
  # Also use dbus-run-session to make sure parallel tests aren't failing
  # as they simultaneously try to own the "org.gimp.GIMP.UI" D-Bus name

  # This is weird but basically on a Debian testing/bookworm, apparently
  # the --auto-display (neither the short version -d) option does not
  # exist and ends up in error:
  # > xvfb-run: unrecognized option '--auto-display'
  # There only --auto-servernum works fine.
  #
  # On a recent Fedora (33 in my case), the later exists but a few of
  # the tests fail with some weirder:
  # > /usr/bin/xvfb-run: line 186: kill: (53539) - No such process
  # There using --auto-display instead (supposed to deprecate
  # --auto-servernum) works instead, but only in its short form (-d).
  # The long form --auto-display also results in the "unrecognized
  # option" error even though the help output lists it.
  # Yep it's a huge mess.
  xvfb-run 2>&1|grep --quiet auto-display
  HAS_AUTO_DISPLAY="$?"
  if [ "$HAS_AUTO_DISPLAY" -eq 0 ]; then
    OPT="-d"
  else
    OPT="--auto-servernum"
  fi
  xvfb-run $OPT --server-args="-screen 0 1280x1024x24" \
    dbus-run-session -- "$@"

else
  # Run the executable directly,
  # i.e. no need to run Xvfb (which will have a timeout)

  "$@"
fi
