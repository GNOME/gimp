require 5.004_04;

use Config;

$|=1;

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
   exit system("./configure",@ARGV)>>8;
} else {
   shift;
   local $do_config_msg = 1;
   do './config.pl'; die $@ if $@;
}

@otherdeps = qw(
      povray imagemagick
);
@gtk_pins =
   qw(
     dataurl colorhtml avi PDB parasite-editor fade-alpha
   ); # miff should be in @pins(!) but detection of non-default arguments is broken
@pins =
   qw(
      windify prep4gif webify tex-to-float ditherize
      xachlego xachshadow roundsel blended2 warp-sharp
      scratches blowinout terral_text xachvision perlcc translogo
      animate_cells image_tile yinyang stamps font_table sethspin
      perlotine randomblends innerbevel fit-text guidegrid
      repdup centerguide stampify goldenmean mirrorsplit
      layerfuncs randomart1 glowing_steel frame_reshuffle frame_filter
      logulator guide_remove guides_to_selection burst
      layerfuncs bricks miff dust
   );
@pdl_pins =
   qw(
      border gouge pixelmap view3d clear_alpha

      fire map_to_gradient
   ); # fire should use normal gradient map!

eval "use Gtk;"; $GTK = $@ eq "";
eval "use Parse::RecDescent;"; $PRD = $@ eq "";

$] >= 5.005 or print <<EOF;

NOTICE:  You are using a version of perl older than 5.005. This will slightly
         reduce the features available to the Gimp-Perl extension.

EOF

$GTK or print <<EOF;

NOTICE:  unable to use the Perl-Gtk interface. Many features (like
         Gimp::Fu) rely on this extension. You can build without it (and
         many scripts will be disabled or run with reduced functionality
         only), but it's better to install it (version 0.6123 or higher is
         required, you can get it from ftp://ftp.gimp.org/pub/gtk/perl/ or
         any CPAN mirror.

EOF

$PRD or print <<EOF;

NOTICE: Parse::RecDescent not found, scm2perl, the Scheme->Perl
        translator, will not work. This is normally not a problem, since
        it is quite unimportant for most people.

EOF

($major,$minor,$patch)=split /[._]/,$Gtk::VERSION;

require ExtUtils::MakeMaker;
import ExtUtils::MakeMaker;

$MICRO = int ($] * 100000 % 100);

if ($MICRO > 49) {
   print <<EOF;

WARNING: You are using perl version $]. This is an *ALPHA* version that is
         very likely to have serious bugs that keep gimp-perl from working
         properly. Please upgrade/downgrade to a stable release.

EOF
}

if ($Config{usethreads} or $Config{use5005threads}) {
   print <<EOF;

WARNING: Your perl has been built with multithreading enabled. The 5.005
         threading model is utterly broken (and about to be removed),
         while the 5.6 threading model is about to change very soon.
         Since I suppose you know how to fix perl since you enabled
         threading, I'll proceed with building gimp-perl.

EOF
}

if ($] < 5.006 && $Config{usethreads}) {
   print <<EOF;

WARNING: You are using the experimental (and very broken) multithread support.
         While gimp-perl might seem to work, it is very likely to not work
         properly (or at all). Please use a *stable* version of perl.
            
EOF
}

push @pins, @pdl_pins if $PDL;
push @pins, @gtk_pins if $GTK;

@INC = grep /^\//,@INC; # comment out as well?
if (0) {		# comment out because it breaks
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
}

sub MY::install {
   my $self=shift;
   package MY;
   my $install = $self->SUPER::install(@_);
   ($install =~ s/\b(un)?install\b/really-$1install/g) or return $install;
   '
install :: install-po
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
	   fi ; \
	done ; \
	$(MAKE) really-install install-plugins'."
	$main::dont_embed || (cd embed && \$(MAKE) install)

install-po:
	cd po && \$(MAKE) install
        
".$install;
}

sub MY::postamble {
   my $self=shift;
   my $install_program = $cfg{INSTALL};
   $install_program =~ s{^\.\./}{../../};
   my $GT = $IN_GIMP ? $install_program : "$GIMPTOOL --install-admin-bin";
   my $UT = $IN_GIMP ? '$(RM_F)' : "$GIMPTOOL --uninstall-admin-bin";
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
	\$(CP) ".join(' ',map("'../examples/$_'",sort @pins))." ../Perl-Server . ; \\
	\$(CHMOD) 755 * ; \\
	\$(MY_FIXIN) * ; \\
	for plugin in * ; do \\
	   $GT \"\$\$plugin\" $GT2 ; \\
	done
	\$(RM_RF) inst-temp

uninstall :: force_uninstall_from_\$(INSTALLDIRS)dirs
	for plugin in Perl-Server @pins; do \\
           $UT $GT2\$\$plugin; \\
        done

force_uninstall_from_perldirs ::
	\$(PERL) -MExtUtils::Install -e 'uninstall(\$\$ARGV[0],1,0)' ".$self->catfile('$(PERL_ARCHLIB)','auto','$(FULLEXT)','.packlist')."

force_uninstall_from_sitedirs ::
	\$(PERL) -MExtUtils::Install -e 'uninstall(\$\$ARGV[0],1,0)' ".$self->catfile('$(SITEARCHEXP)','auto','$(FULLEXT)','.packlist')."

# generate tags file
tags: .
	ctags --lang=c `find . -name '*.xs' -print`

# run autoconf (&c)
autoconf:
	aclocal; autoheader; autoconf
        
# set \$VERSION in all modules
setver:
	\$(PERL) -pi -e 's/^(\\s*\\\$\$VERSION\\s*=\\s*).*\$\$/\$\${1}\$(VERSION);/' *.pm Gimp/*.pm UI/*.pm Net/*.pm

";

   $postamble;
}

$GIMP_INC_NOUI = "-I../../ $GIMP_INC_NOUI" if $IN_GIMP;
@DIRS= qw/Gimp Net UI/;

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

@EXE_FILES = qw(gimpdoc xcftopnm embedxpm);
push @EXE_FILES, qw(scm2perl scm2scm) if $PRD;

WriteMakefile(
    'dist'	=> {
	            PREOP	=> 'make setver && chmod -R u=rwX,go=rX . ;',
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
    'INSTALLBIN'=> $bindir,
    'PM'	=> {
    		    'Gimp.pm'		=> '$(INST_LIBDIR)/Gimp.pm',
    		    'Gimp/Data.pm'	=> '$(INST_LIBDIR)/Gimp/Data.pm',
    		    'Gimp/Fu.pm'	=> '$(INST_LIBDIR)/Gimp/Fu.pm',
    		    'Gimp/Lib.pm'	=> '$(INST_LIBDIR)/Gimp/Lib.pm',
    		    'Gimp/PDL.pm'	=> '$(INST_LIBDIR)/Gimp/PDL.pm',
    		    'Gimp/Util.pm'	=> '$(INST_LIBDIR)/Gimp/Util.pm',
    		    'Gimp/Feature.pm'	=> '$(INST_LIBDIR)/Gimp/Feature.pm',
    		    'Gimp/Module.pm'	=> '$(INST_LIBDIR)/Gimp/Module.pm',
    		    'Gimp/Config.pm'	=> '$(INST_LIBDIR)/Gimp/Config.pm',
    		   },
    'LDFROM'	=> expand("\$(OBJECT) $LIBS $INTLLIBS"),
    'INC'	=> "$INC1 $GIMP_INC_NOUI $CPPFLAGS $CFLAGS",
    'DEFINE'	=> "$DEFINE1 $DEFS",
    'EXE_FILES'	=> \@EXE_FILES,
    'macro'	=> \%cfg,
    'realclean'	=> { FILES => "config.status config.cache config.pl config.log config.h Gimp/Config.pm Makefile" },
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



