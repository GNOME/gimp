## Portability defines that help when running aclocal+autoconf without
## glib-2.0.m4 and pkg.m4 which are required when building on Win32.

ifdef([AM_GLIB_GNU_GETTEXT],[],[
  define([AM_GLIB_GNU_GETTEXT],
         [# This code is in the Win32 branch of an if statement.
	  # If you get here, you are building for Win32 but
	  # don't have the glib-2.0.m4 file from GLib-1.3.10 or later
	  # properly installed.
	  echo "You don't have glib-2.0.m4 properly installed" >&2
	  exit 1
	 ])
])
ifdef([PKG_CHECK_MODULES],[],[
  define([PKG_CHECK_MODULES],
         [# This code is in the Win32 branch of an if statement.
	  # If you get here, you are building for Win32 but
	  # don't have the pkg.m4 file from pkg-config
	  # properly installed.
	  echo "You don't have pkg.m4 properly installed" >&2
	  exit 1
	 ])
])
