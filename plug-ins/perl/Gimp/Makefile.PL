use ExtUtils::MakeMaker;

$topdir="..";
do '../config.pl';

sub MY::const_loadlibs {
   my $self = shift;
   if ($IN_GIMP) {
      $self->{LD_RUN_PATH} = join (":",
         $libdir,
         split /:/,$self->{LD_RUN_PATH}
      );
   }
   package MY;
   $self->SUPER::const_loadlibs(@_);
}

sub MY::postamble {
   <<"EOF";
clean ::
	test -f Makefile || mv -f Makefile.old Makefile
EOF
}

WriteMakefile(
    'NAME'	=> 'Gimp::Lib',
    'VERSION_FROM' => '../Gimp.pm',
    'INC'	=> "$CPPFLAGS -I.. $CFLAGS $GIMP_INC_NOUI $DEFS".($IN_GIMP ? " -DIN_GIMP" : ""),
    'macro'	=> { libdir => $libdir, exec_prefix => $exec_prefix, prefix => $prefix },
    'LDLOADLIBS'=> "$LDFLAGS $LIBS $GIMP_LIBS_NOUI",
    'DEFINE'	=> '',
    dynamic_lib	=> { OTHERLDFLAGS => "$LDFLAGS $LIBS $GIMP_LIBS_NOUI" },
);
