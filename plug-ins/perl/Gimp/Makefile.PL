use ExtUtils::MakeMaker;

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

$GIMP_INC_NOUI = "-I../../.. $GIMP_INC_NOUI" if $IN_GIMP;

WriteMakefile(
    'NAME'	=> 'Gimp::Lib',
    'VERSION_FROM' => '../Gimp.pm',
    'INC'	=> "$INC1 $GIMP_INC_NOUI $CPPFLAGS $CFLAGS",
    'DEFINE'	=> "$DEFINE1 $DEFS",
    'macro'	=> { libdir => $libdir, exec_prefix => $exec_prefix, prefix => $prefix },
    dynamic_lib	=> { OTHERLDFLAGS => "$GIMP_LIBS_NOUI $LDFLAGS $LIBS" },
);
