# Building from git and contributing

While packagers are expected to compile GIMP from tarballs (same for
personal usage), developers who wish to contribute patches should
compile from the git repository.

The procedure is basically the same (such as described in the `INSTALL`
file, which you will notice doesn't exist in the repository; you can
find [INSTALL.in](/INSTALL.in) instead, whose base text is the same
before substitution by the build configuration step). Yet it has a few
extra steps.

## Git

GIMP is available from GNOME Git. You can use the following commands
to get GIMP from the the git server:

    $ git clone https://gitlab.gnome.org/GNOME/gimp.git

You can read more on using GNOME's git service at these URLs:

    https://wiki.gnome.org/Git
    https://www.kernel.org/pub/software/scm/git/docs/


The development branches of GIMP closely follow the development branches
of `babl` and `GEGL` which are considered part of the project. Therefore
you will also have to clone and build these repositories:

    $ git clone https://gitlab.gnome.org/GNOME/babl.git
    $ git clone https://gitlab.gnome.org/GNOME/gegl.git


As for other requirements as listed in the `INSTALL` file, if you use
a development-oriented Linux distribution (Debian Testing or Fedora for
instance), you will often be able to install all of them from your
package manager. On Windows, the MSYS2 project is great for keeping up
with libraries too. On macOS, it is unfortunately much more of a hurdle
and you should probably look at instructions in our [gimp-macos-build
repository](https://gitlab.gnome.org/Infrastructure/gimp-macos-build)
which is how we build GIMP for macOS.

We also know that GIMP is built on various \*BSD, proprietary Unixes,
even on GNU-Hurd and less known systems such as Haiku OS but we don't
have much details to help you there. Yet we still welcome patches to
improve situation on any platform.

In any case, if you use a system providing too old packages, you might
be forced to build from source (tarballs or repositories) the various
dependencies list in `INSTALL`.

## Additional Requirements

Our autotools build system requires the following packages (or newer
versions) installed when building from git (unlike building from
tarball):

    * GNU autoconf 2.54 or over
        - ftp://ftp.gnu.org/gnu/autoconf/
    * GNU automake 1.13 or over
        - ftp://ftp.gnu.org/gnu/automake/
    * GNU libtool 1.5 or over
        - ftp://ftp.gnu.org/gnu/libtool/

Alternatively a build with meson 0.53.0 or over is possible but it is
not complete yet, hence not usable for packaging (yet usable for
development).

For some specific build rules, you will also need:

    * xsltproc
        - ftp://ftp.gnome.org/pub/GNOME/sources/libxslt/1.1/

In any cases, you will require this tool for dependency retrieval:

    * pkg-config 0.16.0 (or preferably a newer version)
        - https://www.freedesktop.org/software/pkgconfig/

These are only the additional requirements if you want to compile from
the git repository. The file `INSTALL` lists the various libraries we
depend on.


## Additional Compilation Steps

If you are accessing GIMP via git, then you will need to take more
steps to get it to compile.  You can do all these steps at once by
running:

    gimp/trunk$ ./autogen.sh

Basically this does the following for you:

    gimp/trunk$ aclocal-1.9; libtoolize; automake-1.9 -a;
    gimp/trunk$ autoconf;

The above commands create the "configure" script.  Now you can run the
configure script in gimp/trunk to create all the Makefiles.

Before running autogen.sh or configure, make sure you have libtool in
your path. Also make sure glib-2.0.m4 glib-gettext.m4, gtk-3.0.m4 and
pkg.m4 are either installed in the same $prefix/share/aclocal relative to your
automake/aclocal installation or call autogen.sh as follows:

    $ ACLOCAL_FLAGS="-I $prefix/share/aclocal" ./autogen.sh

Note that autogen.sh runs configure for you.  If you wish to pass
options like --prefix=/usr to configure you can give those options to
autogen.sh and they will be passed on to configure.

If AUTOGEN_CONFIGURE_ARGS is set, these options will also be passed to
the configure script. If for example you want to enable the build of
the GIMP API reference manuals, you can set AUTOGEN_CONFIGURE_ARGS to
"--enable-gi-docgen".

If you want to use libraries from a non-standard prefix, you should set
PKG_CONFIG_PATH appropriately. Some libraries do not use pkgconfig, see
the output of ./configure --help for information on what environment
variables to set to point the compiler and linker to the correct path.
Note that you need to do this even if you are installing GIMP itself
into the same prefix as the library.


## Patches

The best way to submit patches is to provide files created with
git-format-patch. The recommended command for a patch against the
`master` branch is:

  $ git format-patch origin/master

It is recommended that you file a bug report in our
[tracker](https://gitlab.gnome.org/GNOME/gimp) and either create a merge
request or attach your patch to the report as a plain text file, not
compressed.

Please follow the guidelines for coding style and other requirements
listed in [CODING_STYLE.md](https://developer.gimp.org/core/coding_style/). When
you will contribute your first patches, you will notice that we care very much
about clean and tidy code, which helps for understanding. Hence we care about
coding style and consistency!


## Auto-generated Files

Please notice that some files in the source are generated from other
sources. All those files have a short notice about being generated
somewhere at the top. Among them are the files ending in `pdb.[ch]` in
the `libgimp/` directory and the files ending in `cmds.c` in the
`app/pdb/` subdirectory. Those are generated from the respective `.pdb`
files in `pdb/groups`.

Other files are:

* `AUTHORS` from `authors.xml`
