require 5.004;

use Config;

$topdir=".";
$|=1;

@examples =
   qw(windy.pl prep4gif.pl webify.pl PDB alpha2color.pl tex-to-float ditherize.pl
      border.pl view3d.pl feedback.pl xachlego.pl xachshadow.pl parasite-editor
      scratches.pl blowinout.pl terral_text xachvision.pl gimpmagick perlcc
      sethspin.pl animate_cells image_tile yinyang stamps font_table
      perlotine randomblends innerbevel
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
   if(defined $EXTENSIVE_TESTS) {
      print "\nFetched some defaults from an earlier Makefile.PL run.\n";
      print "Run \"make realclean\" if you don't want this\n";
   } else {
      print "
This module usually does only a very short installation check.
Alternatively, it can do more extensive tests, but these tests might fail
even if everything is in fact correct.  And it has only been tested with
gimp-1.1 ;)

If you decide to run these tests (please!), I'd be glad to hear
success stories (and of course any bug-reports ;)

Do you want me to run these tests [y]? ";
      
      print "y\n";
      $EXTENSIVE_TESTS = 1;
      #$EXTENSIVE_TESTS = (<STDIN> !~ /^[nN]/) ? 1 : 0;
   }
   
   print "\n";
   
   $ENV{'EXTENSIVE_TESTS'} = $EXTENSIVE_TESTS;
   $ENV{CC}=$Config{cc};
   $ENV{LD}=$Config{ld};
   $ENV{prefix}=$Config{prefix};
   $ENV{PERL}=$Config{perlpath};
   $ENV{MAKEFILE_PL}=$0;
   $ENV{IN_GIMP}=0;
   exit system("./etc/configure",@ARGV)>>8;
} else {
   do './config.pl'; die $@ if $@;
}

require ExtUtils::MakeMaker;
import ExtUtils::MakeMaker;

@INC = grep /^\//,@INC;
eval { $Gimp::no_SIG=1; require Gimp };
unless($@) {
   $old_prefix = eval { Gimp::_gimp_prefix() };
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
	 overwrite an old but still used installation. Gimp-Perl will
	 therefore be disabled, and not be installed.

EOF
	 not_halt("prefix mismatch");
      }
   }
}

eval "use Gtk;"; $GTK = $@ eq "";
eval "use PDL;"; $PDL = $@ eq "";
eval "use Parse::RecDescent;"; $PRD = $@ eq "";

$] >= 5.005 or print <<EOF;

NOTICE:  you are using a version of perl older than 5.005. While this
         extension should run on older versions (and I try to keep source
         compatibility), some people get spurious errors that go away
         after upgrading to 5.005 (or to gimp-1.1). Also, some features of
         Gimp DO NOT WORK WITH 5.004 or gimp-1.0 and are disabled. Since
         5.005 is much better and has many many bugs fixed, an upgrade
         would come in handy...

EOF

$GTK or print <<EOF;

WARNING: unable to use the Perl-Gtk interface. Most features (like
	 Gimp::Fu) rely on this extension. You can try to build without
	 it (and many scripts won't work), but it's better to install
	 it (version 0.3 or higher is required, you can get it from
	 ftp://ftp.gimp.org/pub/gtk/perl/ or any CPAN mirror.

EOF

$PDL or print <<EOF;

WARNING: unable to use PDL (the perl data language). This means that
	 Gimp::PDL is non-functional. Unless you plan to use Tile/PixelRgn
	 functions together with PDL, this is harmless. Gimp::PDL will
	 be installed, just in case you later install PDL. The plug-ins
	 using PDL, however, will NOT WORK. You can get PDL from any CPAN
	 mirror.

EOF

!$PDL or $PDL::Version::VERSION > 1.99 or print <<EOF;

WARNING: PDL version $PDL::Version::VERSION is installed. Gimp::PDL was only
	 tested with 2.0 and higher.  In case of problems its advisable to
	 upgrade PDL to at least version 2.

EOF

$PRD or print <<EOF;

WARNING: Parse::RecDescent is not installed (correctly) on your system. This
	 means scm2perl (the Scheme->Perl translator) isn't usable. If you
	 don't need this functionality there is nothing to worry about. 
	 Should the need arise you can install Parse::RecDescent later; it
	 is available from any CPAN mirror.

EOF

($major,$minor,$patch)=split /[._]/,$Gtk::VERSION;

unless ($major > 0
	|| ($major == 0 && $minor >  3)
	|| ($major == 0 && $minor == 3 && $patch >= -1)) {
   $GTK && print <<EOF;

WARNING: version 0.3 of Gtk is _required_ for this module to
	 build properly.  You can get the newest version from
	 ftp://ftp.gimp.org/pub/gtk/perl/ or any CPAN mirror.  Older
	 versions may work, but I have warned you!

EOF
}

# wo do no longer do these dirty things
#for(@shebang) {
#   system ($Config{perlpath},"-pi","-e","\$. == 1 and \$_ = '#!$Config{perlpath}\n'",$_);
#}

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
	$(MAKE) UNINST=1 really-install install-plugins

'.$install;
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

clean ::
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

WriteMakefile(
    'dist'	=> {
	            PREOP	=> 'chmod -R u=rwX,go=rX . ;',
	            COMPRESS	=> 'gzip -9v',
	            SUFFIX	=> '.gz',
	           },
    'PREREQ_PM'	=> {
    		    Gtk			=> 0.3,
                    PDL			=> 1.99,
    		    Data::Dumper	=> 2,
                    Parse::RecDescent	=> 1.6,
		   },
    'DIR'	=> ['Gimp'],
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
    		   },
    'LIBS'	=> [''],
    'INC'	=> "$CPPFLAGS $CFLAGS $GIMP_INC_NOUI",
    'DEFINE'	=> ($IN_GIMP ? " -DIN_GIMP " : "")." $DEFS",
    'EXE_FILES'	=> ['scm2perl','scm2scm'],
    'macro'	=> \%cfg,
    'realclean'	=> { FILES => "config.status config.cache config.log config.pl config.h" },
    'clean'	=> { FILES => "Makefile.old stamp-h" },
);

$IN_GIMP or print <<EOF;


Hopefully, Gimp is now correctly configured. you can now enter "make",
"make test" and "make install".

EOF

# write an empty makefile (the last chance to stop)
# and warn the user.
sub not_halt {
   print STDERR "WARNING: *** perl extension will not be built ($_[0]) ***\n\n";
   open MAKEFILE,">Makefile" or die "unable to create Makefile: $!";
   print MAKEFILE <<EOF;
all install check:

clean mostlyclean objclean:

distclean maintainer-clean: clean
	rm -f Makefile config.cache config.pl config.log config.h config.status stamp-h Makefile.old
EOF
   close MAKEFILE;
   exit;
}



