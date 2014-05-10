Doing a Simple Build
====================
To begin with, you need to install jhbuild, mingw-w64, and a few other build-related dependencies.

If you're using debian, install these packages:

	sudo apt-get install build-essential mingw-w64 git jhbuild automake autoconf libtool libgtk2.0-dev ragel intltool bison flex gperf gtk-doc-tools nasm ruby cmake libxml-simple-perl

From there, in theory, you can simply clone this repo, cd into it, and run:

	./build

That will build the development version of the gimp.

If you'd rather build the stable version of the gimp, run this instead:

	MODULE=gimp-stable ./build

If you'd like to build with debigging information, run:

	BUILD_FLAVOUR=dbg ./build

What if it doesn't work out of the box?
=======================================
I've actually never had that work out of the box, so chances are you'll need to adjust things a bit.

If you get an error along the lines of `no: command not found` while building GTK+, then that means gdk-pixbuf-csource can't be found.
You can fix this by installing your distro's GTK+ 2 development package.
(libgtk2.0-dev on debian)

If you get an error that looks like this while building cairo:

	In file included from getline.c:31:0:
	cairo-missing.h:45:17: error: conflicting types for 'ssize_t'
	In file included from /usr/lib/gcc/i486-mingw32/4.7.0/../../../../i486-mingw32/include/stdio.h:534:0,
		             from cairo-missing.h:36,
		             from getline.c:31:
	/usr/lib/gcc/i486-mingw32/4.7.0/../../../../i486-mingw32/include/sys/types.h:118:18: note: previous declaration of 'ssize_t' was here
	In file included from strndup.c:31:0:
	cairo-missing.h:45:17: error: conflicting types for 'ssize_t'
	In file included from /usr/lib/gcc/i486-mingw32/4.7.0/../../../../i486-mingw32/include/stdio.h:534:0,
		             from cairo-missing.h:36,
		             from strndup.c:31:
	/usr/lib/gcc/i486-mingw32/4.7.0/../../../../i486-mingw32/include/sys/types.h:118:18: note: previous declaration of 'ssize_t' was here

Then you need to add `-D_SSIZE_T_DEFINED` to your MINGW_CFLAGS, like this:

	export MINGW_CFLAGS="-D_SSIZE_T_DEFINED"

Other Scripts
=============
There are a few other scripts included in this repo:
 * ./clean will remove all build artifacts (but leave the downloaded tarballs), leaving you with a clean setup.
 * ./mkarchive will create self extracting archives of the gimp.
 * ./split-build will do a special build where it builds both gimp-dev and gimp-stable but the two builds share the same dependencies. The directories then needs to be merged using ./mkarchive.
