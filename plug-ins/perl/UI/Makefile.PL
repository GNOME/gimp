use ExtUtils::MakeMaker;

do '../config.pl';

sub MY::const_config {
   my $self = shift;
   $self->{LDDLFLAGS}="$GIMP_LIBS_NOUI $LDFLAGS $LIBS $self->{LDDLFLAGS}";
   package MY;
   $self->SUPER::const_config(@_);
}

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

$GIMP_INC = "-I../../.. $GIMP_INC" if $IN_GIMP;

WriteMakefile(
    'NAME'	=> 'Gimp::UI',
    'VERSION_FROM' => '../Gimp.pm',
    'INC'	=> "$INC1 $GIMP_INC $CPPFLAGS $pdl_inc $CFLAGS",
    'LIBS'	=> "$GIMP_LIBS",
    'DEFINE'	=> "$DEFINE1 $DEFS",
    'macro'	=> { libdir => $libdir, exec_prefix => $exec_prefix, prefix => $prefix },
    'TYPEMAPS'	=> ["$topdir/typemap",@pdl_typemaps],
);
