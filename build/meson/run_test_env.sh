#!/bin/sh
#
# Wrapper script to use for the Meson test setup.
#
# Define the "UI_TEST" for all tests that should run headless

if [[ -n "${UI_TEST}" ]]; then
  # Use Xvfb to simulate a graphical session; note that this needs
  # a new enough version which has the -d option.
  #
  # Also use dbus-run-session to make sure parallel tests aren't failing
  # as they simultaneously try to own the "org.gimp.GIMP.UI" D-Bus name

  xvfb-run -d --server-args="-screen 0 1280x1024x24" \
    dbus-run-session -- "$@"

else
  # Run the executable directly,
  # i.e. no need to run Xvfb (which will have a timeout)

  "$@"
fi

