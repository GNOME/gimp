require 5.004_04;

use Config;

$|=1;

@examples =
   qw(windy.pl prep4gif.pl webify.pl PDB alpha2color.pl tex-to-float ditherize.pl
      border.pl view3d.pl feedback.pl xachlego.pl xachshadow.pl parasite-editor
      scratches.pl blowinout.pl terral_text xachvision.pl gimpmagick perlcc
      sethspin.pl animate_cells image_tile yinyang stamps font_table
      perlotine randomblends innerbevel fit-text guidegrid roundrectsel
      repdup centerguide stampify goldenmean triangle billboard mirrorsplit
      oneliners randomart1 pixelmap glowing_steel
   );
@shebang = (map("examples/$_",@examples),
   qw(Perl-Server examples/example-net.pl examples/homepage-logo.pl
      examples/example-fu.pl examples/example-oo.pl
   ));

if ($ARGV[0] ne "--writemakefile") {
   for(@ARGV) {
      s/^prefix=/--prefix=/i;
   }
   
   do './config.pl';
   $EXTENSIVE_TESTS = 1;
   
   $ENV{'EXTENSIVE_TESTS'} = $EXTENSIVE_TESTS;
   $ENV{CC}=$Config{cc};
   $ENV{LD}=$Config{ld};
   $ENV{prefix}=$Config{prefix};
   $ENV{PERL}=$Config{perlpath};
   $ENV{MAKEFILE_PL}=$0;
   $ENV{IN_GIMP}=0;
   exit system("./etc/configure",@ARGV)>>8;
} else {
   shift;
   local $do_config_msg = 1;
   do './config.pl'; die $@ if $@;
}

require ExtUtils::MakeMaker;
import ExtUtils::MakeMaker;

@INC = grep /^\//,@INC;
eval { $Gimp::no_SIG=1; require Gimp };
unless($@) {
   eval { require Gimp::Config };
   $old_prefix = eval { $Gimp::Config{GIMP_PREFIX} };
   $old_prefix = eval { Gimp::_gimp_prefix() } unless $old_prefix;
   if ($@) {
      print <<EOF;

WARNING: I've detected an old version of Gimp-Perl installed
	 already. Since I cannot detect the prefix used to install
	 it I will just overwrite it.  If you happen to use two
	 different and incompatible versions of the Gimp with differing
	 prefixes you should call configure with the --disable-perl
	 switch to disable the perl extension, or consider installing
	 the perl module elsewhere, using the environment variables
	 PERL5LIB=/my/module/dir and PERL_MM_OPTS="PREFIX=\$PERL5LIB" to
	 overwrite the installation directory (PERL_MM_OPTS) and run the
	 Gimp (PERL5LIB). See "perldoc ExtUtils::MakeMaker" for a full
	 discussion of your options.

EOF
   } else {
      if ($GIMP_PREFIX ne $old_prefix) {
	 print <<EOF;

WARNING: I've detected another installation of the Gimp-Perl extension.
	    
	 This version uses the prefix '$GIMP_PREFIX'.
	 The already installed version uses the prefix '$old_prefix'.

	 They don't match, which indicates that installing Gimp-Perl might
	 overwrite an old but still used installation. The Gimp-Perl
         installation _might_ therefore be broken.

EOF
      }
   }
}

eval "use Gtk;"; $GTK = $@ eq "";
eval "use Parse::RecDescent;"; $PRD = $@ eq "";

$] >= 5.005 or print <<EOF;

NOTICE:  You are using a version of perl older than 5.005. This will reduce
         the features available to the Gimp-Perl extension.

EOF

$GTK or print <<EOF;

NOTICE:  unable to use the Perl-Gtk interface. Many features (like
         Gimp::Fu) rely on this extension. You can build without it (and
         many scripts will be disabled or run with reduced functionality
         only), but it's better to install it (version 0.5120 or higher is
         required, you can get it from ftp://ftp.gimp.org/pub/gtk/perl/ or
         any CPAN mirror.

EOF

$PRD or print <<EOF;

WARNING: Parse::RecDescent not found, scm2perl, the Scheme->Perl
         translator, will not work. Should the need arise you can install
         Parse::RecDescent later; it is available from any CPAN mirror.

EOF

($major,$minor,$patch)=split /[._]/,$Gtk::VERSION;

sub MY::install {
   package MY;
   my $install = shift->SUPER::install(@_);
   ($install =~ s/^install\b/really-install/m) or return $install;
   '
install ::
	@for dir in \
	   $(INSTALLPRIVLIB) \
	   $(INSTALLARCHLIB) \
	   $(INSTALLBIN) \
	   $(INSTALLSCRIPT) \
	   $(INSTALLMAN1DIR) \
	   $(INSTALLMAN3DIR) ; \
	do \
	   $(MKPATH) "$$dir"; \
	   if test -d "$$dir" && test -w "$$dir" ; then : ; else \
	      echo ; \
	      echo "ERROR: installation directory \"$$dir\"" ; \
	      echo "       is not writable, not installing gimp-perl" ; \
	      echo ; \
	      exit ; \
	   fi \
	done ; \
	$(MAKE) UNINST=1 really-install install-plugins'."
	$main::dont_embed || (cd embed && \$(MAKE) install)

".$install;
}

sub MY::postamble {
   my $GT = $IN_GIMP ? '$(INSTALL_PROGRAM)' : "$GIMPTOOL --install-admin-bin";
   my $GT2 = $IN_GIMP ? '$(gimpplugindir)/plug-ins/' : '';
   my $postamble="

objclean :: clean
maintainer-clean :: realclean
distclean :: realclean
check :: test

MY_FIXIN = \$(PERL) -I\$(PERL_ARCHLIB) -I\$(PERL_LIB) -MExtUtils::MakeMaker -e 'MY->fixin(\@ARGV)'

all ::
	$main::dont_embed || (cd embed && \$(MAKE) all)

clean ::
	$main::dont_embed || (cd embed && \$(MAKE) clean)
	test -f Makefile || mv -f Makefile.old Makefile
	\$(RM_RF) inst-temp

install-plugins ::
	\$(RM_RF) inst-temp
	\$(MKPATH) inst-temp
	cd inst-temp ; \\
	\$(UMASK_NULL) ; \\
	\$(CP) ".join(' ',map("'../examples/$_'",@examples))." ../Perl-Server . ; \\
	\$(CHMOD) 755 * ; \\
	\$(MY_FIXIN) * ; \\
	for plugin in * ; do \\
	   $GT \"\$\$plugin\" $GT2 ; \\
	done
	\$(RM_RF) inst-temp
";
}

$GIMP_INC_NOUI = "-I../../ $GIMP_INC_NOUI" if $IN_GIMP;
@DIRS= qw/Gimp Net/;

$build_module = $IN_GIMP || $ENV{GIMP_PERL_MODULE_INC};
# temporarily disabled because of politics
$build_module = $ENV{GIMP_PERL_MODULE_INC};

print "building embedded perl module... ";
if ($build_module) {
   print "yes\n";
   $dont_embed = "false";
   push(@DIRS,'Module');
   print "configuring in embed/Makefile...\n";
   system("cd embed && perl Makefile.PL");
} else {
   print "no\n";
   $dont_embed = "true";
}

print "writing Gimp/Config.pm... ";
{
   sub conf_eval {
      my $v = expand($cfg{$_[0]});
      $v =~ s/([\\\]])/\\$1/g;
      $v;
   }
   local $/,*FH;
   open FH,"<Gimp/Config.pm.in" or die "Gimp/Config.pm.in: $!\n";
   my $cfg = <FH>;
   $cfg =~ s/#CFG#/join "",
                        map sprintf("   %-20s => q[%s],\n",$_,conf_eval $_),
                            keys %cfg/e;
   open FH,">Gimp/Config.pm" or die "Gimp/Config.pm: $!\n";
   print(FH $cfg)>0 or die "Gimp/Config.pm: $!\n";
}
print "ok\n";

WriteMakefile(
    'dist'	=> {
	            PREOP	=> 'chmod -R u=rwX,go=rX . ;',
	            COMPRESS	=> 'gzip -9v',
	            SUFFIX	=> '.gz',
	           },
    'PREREQ_PM'	=> {
    		    Gtk			=> 0.5,
                    PDL			=> 1.99,
    		    Data::Dumper	=> 2.0,
                    Parse::RecDescent	=> 1.6,
		   },
    'DIR'	=> [@DIRS],
    'NAME'	=> 'Gimp',
    'VERSION_FROM' => 'Gimp.pm',
    'PM'	=> {
    		    'Gimp.pm'		=> '$(INST_LIBDIR)/Gimp.pm',
    		    'Gimp/Data.pm'	=> '$(INST_LIBDIR)/Gimp/Data.pm',
    		    'Gimp/Fu.pm'	=> '$(INST_LIBDIR)/Gimp/Fu.pm',
    		    'Gimp/Lib.pm'	=> '$(INST_LIBDIR)/Gimp/Lib.pm',
    		    'Gimp/UI.pm'	=> '$(INST_LIBDIR)/Gimp/UI.pm',
    		    'Gimp/Net.pm'	=> '$(INST_LIBDIR)/Gimp/Net.pm',
    		    'Gimp/PDL.pm'	=> '$(INST_LIBDIR)/Gimp/PDL.pm',
    		    'Gimp/Util.pm'	=> '$(INST_LIBDIR)/Gimp/Util.pm',
    		    'Gimp/Feature.pm'	=> '$(INST_LIBDIR)/Gimp/Feature.pm',
    		    'Gimp/Module.pm'	=> '$(INST_LIBDIR)/Gimp/Module.pm',
    		    'Gimp/Config.pm'	=> '$(INST_LIBDIR)/Gimp/Config.pm',
    		   },
    'LIBS'	=> [''],
    'INC'	=> "$INC1 $GIMP_INC_NOUI $CPPFLAGS $CFLAGS",
    'DEFINE'	=> "$DEFINE1 $DEFS",
    'EXE_FILES'	=> ['scm2perl','scm2scm','gimpdoc'],
    'macro'	=> \%cfg,
    'realclean'	=> { FILES => "config.status config.cache config.pl config.log config.h Gimp/Config.pm" },
    'clean'	=> { FILES => "Makefile.old stamp-h" },
);

$IN_GIMP or print <<EOF;


Hopefully, Gimp is now correctly configured. you can now enter "make",
"make test" and "make install".

EOF

__END__
# write an empty makefile (the last chance to stop)
# and warn the user.
sub not_halt {
   print STDERR "WARNING: *** perl extension will not be built ($_[0]) ***\n\n";
   open MAKEFILE,">Makefile" or die "unable to create Makefile: $!";
   print MAKEFILE <<EOF;
all install check:

clean mostlyclean objclean:

distclean maintainer-clean realclean clobber: clean
	\$(RM_F) Makefile config.cache config.pl config.log
        \$(RM_F) config.h config.status stamp-h Makefile.old Gimp/Config.pm
        \$(RM_RF) test-dir inst-temp
EOF
   close MAKEFILE;
   exit;
}



