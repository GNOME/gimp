use ExtUtils::MakeMaker;

chomp ($_gccflags = qx<glib-config --cflags glib gmodule>);
chomp ($_gldflags = qx<glib-config --libs glib gmodule>);

$topdir="..";
do '../config.pl';

sub MY::postamble {
   <<"EOF";
clean ::
	test -f Makefile || mv -f Makefile.old Makefile
EOF
}

WriteMakefile(
    'NAME'	=> 'Gimp::Module',
    'VERSION_FROM' => '../Gimp.pm',
    'INC'	=> "$_gccflags $CPPFLAGS -I.. $CFLAGS $DEFS".($IN_GIMP ? " -DIN_GIMP" : ""),
    'macro'	=> \%cfg,
    'DEFINE'	=> '',
    dynamic_lib	=> { OTHERLDFLAGS => "$LDFLAGS $LIBS $_gldflags" },
);
