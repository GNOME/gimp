require ExtUtils::MakeMaker;
import ExtUtils::MakeMaker;
use ExtUtils::Embed;
use Config;

do '../config.pl';

$perl = $Config{perlpath};
$_ccopts = ccopts;
$_ldopts = ldopts "-std", $Config{static_exts};

chomp ($_gccflags = qx<glib-config --cflags glib gmodule>);
chomp ($_gldflags = qx<glib-config --libs glib gmodule>);

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

sub MY::install {
   <<EOF;
install :: all \$(INST_DYNAMIC)
	\$(CP) \$(INST_DYNAMIC) \$(gimpplugindir)/modules/\$(DLBASE).\$(DLEXT)
	\$(CHMOD) 755 \$(gimpplugindir)/modules/\$(DLBASE).\$(DLEXT)

uninstall ::
	\$(RM_F) \$(gimpplugindir)/modules/\$(DLBASE).\$(DLEXT)
EOF
}

sub MY::postamble {
   <<"EOF";
clean ::
	test -f Makefile || mv -f Makefile.old Makefile

perlxsi.c:
	\$(FULLPERL) -MExtUtils::Embed -e xsinit
EOF
}

WriteMakefile(
    NAME	=> 'libperlinterp',
    INSTALLDIRS => 'perl',
    INSTALLARCHLIB => '/tmp',
    OBJECT	=> 'perlmod$(OBJ_EXT) perlxsi$(OBJ_EXT)',
    INC		=> "-I.. $_ccopts $_gccflags $CPPFLAGS $CFLAGS $DEFS".($IN_GIMP ? " -DIN_GIMP" : ""),
    macro	=> \%cfg,
    DEFINE	=> '',
    dynamic_lib	=> { OTHERLDFLAGS => "$LDFLAGS $LIBS $_ldopts $_gldflags" },
    clean	=> { FILES => 'perlmod$(OBJ_EXT) perlxsi.c perlxsi$(OBJ_EXT)' }
);


