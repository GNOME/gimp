use ExtUtils::MakeMaker;

chomp ($_gccflags = qx<glib-config --cflags glib gmodule>);
chomp ($_gldflags = qx<glib-config --libs glib gmodule>);

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
    'INC'	=> "$INC1 $GIMP_INC_NOUI $_gccflags $CPPFLAGS $CFLAGS",
    'macro'	=> \%cfg,
    'DEFINE'	=> "$DEFINE1 $DEFS",
    dynamic_lib	=> { OTHERLDFLAGS => "$LDFLAGS $LIBS $_gldflags" },
);
