%define ver      0.99.29
%define rel      SNAP

Summary: The GNU Image Manipulation Program
Name: gimp
Version: %ver
Release: %rel
Copyright: GPL, LGPL
Group: X11/Applications/Graphics
URL: http://www.gimp.org/
Source: ftp://ftp.gimp.org/pub/gimp/v0.99/v%{PACKAGE_VERSION}/gimp-%{PACKAGE_VERSION}.tar.gz
BuildRoot: /tmp/gimp-root
Obsoletes: gimp-data-min
Requires: gtk+ >= 1.0.1


%changelog
* Mon Apr 20 1998 Marc Ewing <marc@redhat.com>
- include *.xpm and .wmconfig in CVS source
- removed explicit glibc require

* Thu Apr 16 1998 Marc Ewing <marc@redhat.com>
- Handle builds using autogen.sh
- SMP builds
- put in CVS, and tweak for automatic CVS builds

* Sun Apr 12 1998 Trond Eivind Glomsrød <teg@pvv.ntnu.no>
- Upgraded to 0.99.26

* Sat Apr 11 1998 Trond Eivind Glomsrød <teg@pvv.ntnu.no>
- Upgraded to 0.99.25

* Wed Apr 08 1998 Trond Eivind Glomsrød <teg@pvv.ntnu.no>
- Upgraded to version 0.99.24

* Sun Apr 05 1998 Trond Eivind Glomsrød <teg@pvv.ntnu.no>
- Stop building the docs - they require emacs and
  (even worse), you must run X.

* Fri Mar 27 1998 Trond Eivind Glomsrød <teg@pvv.ntnu.no>
- upgraded to 0.99.23

* Sat Mar 21 1998 Trond Eivind Glomsrød <teg@pvv.ntnu.no>
- No longer requires xdelta, that was a bug on my part
- spec cleanup, changed libgimp copyright, can now be
  built by non-root users, removed some lines in the description

* Fri Mar 20 1998 Trond Eivind Glomsrød <teg@pvv.ntnu.no>
- upgraded to 0.99.22

* Sun Mar 15 1998 Trond Eivind Glomsrød <teg@pvv.ntnu.no>
- upgraded to 0.99.21

* Thu Mar 12 1998 Trond Eivind Glomsrød <teg@pvv.ntnu.no>
- Upgraded to 0.99.20

* Mon Mar 09 1998 Trond Eivind Glomsrød <teg@pvv.ntnu.no>
- Recompiled with gtk+ 0.99.5
- Now requires gtk+ >= 0.99.5 instead of gtk+ 0.99.4

%description
The GIMP is an image manipulation program suitable for photo retouching,
image composition and image authoring.  Many people find it extremely useful
in creating logos and other graphics for web pages.  The GIMP has many of the
tools and filters you would expect to find in similar commercial offerings,
and some interesting extras as well.

The GIMP provides a large image manipulation toolbox, including channel
operations and layers, effects, sub-pixel imaging and anti-aliasing,
and conversions, all with multi-level undo.

This version of The GIMP includes a scripting facility, but many of the
included scripts rely on fonts that we cannot distribute.  The GIMP ftp
site has a package of fonts that you can install by yourself, which
includes all the fonts needed to run the included scripts.  Some of the
fonts have unusual licensing requirements; all the licenses are documented
in the package.  Get ftp://ftp.gimp.org/pub/gimp/fonts/freefonts-0.10.tar.gz
and ftp://ftp.gimp.org/pub/gimp/fonts/sharefonts-0.10.tar.gz if you are so
inclined.  Alternatively, choose fonts which exist on your system before
running the scripts.

%package devel
Summary: GIMP plugin and extension development kit
Group: X11/Applications/Graphics
Requires: gtk+-devel
Prereq: /sbin/install-info
%description devel
Static libraries and header files for writing GIMP plugins and extensions.

%package libgimp
Summary: GIMP libraries
Group: X11/Applications/Graphics
Copyright: LGPL
%description libgimp
Libraries used to communicate between The GIMP and other programs which
may function as "GIMP plugins".

%prep
%setup

%build
if [ ! -f configure ]; then
  CFLAGS="$RPM_OPT_FLAGS" ./autogen.sh --prefix=/usr
else
  CFLAGS="$RPM_OPT_FLAGS" ./configure --prefix=/usr
fi

if [ "$SMP" != "" ]; then
  (make "MAKE=make -k -j $SMP"; exit 0)
  make
else
  make
fi

#cd docs
#make
#cd ..

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/info $RPM_BUILD_ROOT/usr/include \
	$RPM_BUILD_ROOT/usr/lib $RPM_BUILD_ROOT/usr/bin \
	$RPM_BUILD_ROOT/etc/X11/wmconfig \
	$RPM_BUILD_ROOT/usr/share/icons/mini
make prefix=$RPM_BUILD_ROOT/usr install
#cd docs
#make prefix=$RPM_BUILD_ROOT/usr install
#gzip -9 $RPM_BUILD_ROOT/usr/info/*
strip $RPM_BUILD_ROOT/usr/bin/gimp
install RPM/gimp.wmconfig $RPM_BUILD_ROOT/etc/X11/wmconfig/gimp
install RPM/wilbur.xpm $RPM_BUILD_ROOT/usr/share/icons/
install RPM/mini-wilbur.xpm $RPM_BUILD_ROOT/usr/share/icons/mini/

%clean
rm -rf $RPM_BUILD_ROOT

%post devel
#/sbin/install-info --section="GIMP" --entry="* PDB: (pdb).          The GIMP procedural database." /usr/info/pdb.info.gz /usr/info/dir

%preun devel
#if [ $1 = 0 ]; then
#   /sbin/install-info --delete --section="GIMP" --entry="* PDB: (pdb).          The GIMP procedural database." /usr/info/pdb.info.gz /usr/info/dir
#fi

%post libgimp -p /sbin/ldconfig
%postun libgimp -p /sbin/ldconfig

%files 
%attr(644, root, root) %config /etc/X11/wmconfig/gimp
%attr(-, root, root) %doc AUTHORS COPYING ChangeLog INSTALL NEWS NOTES README TODO
%attr(-, root, root) %doc docs/*.txt docs/*.eps
%attr(-, root, root) /usr/share/*
%attr(-, root, root) /usr/bin/gimp
%attr(-, root, root) /usr/lib/gimp/*
#%attr(-, root, root) /usr/man

%files devel
%attr(-, root, root) /usr/lib/lib*a
%attr(-, root, root) /usr/include/*
#%attr(-, root, root) /usr/info/pdb.info*

%files libgimp
%attr(-, root, root) /usr/lib/lib*.so
%attr(-, root, root) /usr/lib/lib*.so.*
