use ExtUtils::MakeMaker;

do '../config.pl';

sub MY::postamble {
   <<"EOF";
clean ::
	test -f Makefile || mv -f Makefile.old Makefile
EOF
}

$GIMP_INC_NOUI = "-I../../.. $GIMP_INC_NOUI" if $IN_GIMP;

WriteMakefile(
    'NAME'	=> 'Gimp::Net',
    'VERSION_FROM' => '../Gimp.pm',
    'INC'	=> "$INC1 $CPPFLAGS $pdl_inc $CFLAGS",
    'DEFINE'	=> "$DEFINE1 $DEFS",
    'TYPEMAPS'	=> ["$topdir/typemap",@pdl_typemaps],
);
