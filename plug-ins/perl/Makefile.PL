require 5.004_04;

use ExtUtils::MakeMaker;
use Config;

$topdir=".";

if ($ARGV[0] ne "--writemakefile") {
   for(@ARGV) {
      s/^prefix=/--prefix=/i;
   }
   
   $|=1;
   do './config.pl';
   if(defined $EXTENSIVE_TESTS) {
      print "\nFetched some defaults from an earlier Makefile.PL run.\n";
      print "Run \"make clean\" if you don't want this\n";
   } else {
      print "
This module usually does only a very short installation check.
Alternatively, it can do more extensive tests, but these tests might fail
even if everything is in fact correct.  And it has only been tested with
gimp-1.1 ;)

If you decide to run these tests (please!), I'd be glad to hear
success stories (and of course any bug-reports ;)

Do you want me to make these tests [n]? ";
      
      $EXTENSIVE_TESTS = (<STDIN> =~ /^[yYjJ]/) ? 1 : 0;
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
   shift @ARGV;
   do './config.pl';
}

eval "use Gtk;"; $GTK = $@ eq "";
eval "use PDL;"; $PDL = $@ eq "";
eval "use Parse::RecDescent;"; $PRD = $@ eq "";

$] >= 5.005 or print <<EOF;

WARNING: you are using a version of perl older than 5.005. While this
         extension should run on older versions (and I try to keep source
         compatibility), some people get spurious errors that go away after
         upgrading to 5.005. Since 5.005 is much better and has many many
         bugs fixed, an upgrade wpuld come in handy...

EOF

$GTK or print <<EOF;

WARNING: unable to use the Perl-Gtk interface.  Some features (like
         Gimp::Fu) rely on this extension.  You can try to build without it,
         but it's better to install it (version 0.3 or higher is required,
         you can get it from ftp://ftp.gimp.org/pub/gtk/perl/ or any
         CPAN mirror.

EOF

$PDL or print <<EOF;

WARNING: unable to use PDL (the perl data language).  This means that
         Gimp::PDL is non-functional.  Unless you plan to use Tile/PixelRgn
         functions together with PDL, this is harmless.  Gimp::PDL will be
         installed, just in case you later install PDL.  You can get PDL
         from any CPAN mirror.

EOF

!$PDL or $PDL::Version::VERSION > 1.99 or print <<EOF;

WARNING: PDL version $PDL::Version::VERSION is installed. Gimp::PDL was only
         tested with 1.99 and higher.  In case of problems its advisable to
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
   print <<EOF;

WARNING: version 0.3 of Gtk is _required_ for this module to build
         properly.  You can get the newest version from
         ftp://ftp.gimp.org/pub/gtk/perl/ or any CPAN mirror.  Older
         versions may work, but I have warned you!

EOF
}

sub MY::postamble {
   (my $GIMPTOOL2 = $GIMPTOOL) =~ s/^\.\./..\/../;
   <<"EOF";

maintainer-clean :: realclean
objclean :: realclean
distclean :: realclean
check :: test

install :: install-plugins

install-plugins:
	$GIMPTOOL --install-admin-bin Perl-Server
	-cd examples && $GIMPTOOL2 --install-admin-bin windy.pl
	-cd examples && $GIMPTOOL2 --install-admin-bin prep4gif.pl
	-cd examples && $GIMPTOOL2 --install-admin-bin webify.pl
	-cd examples && $GIMPTOOL2 --install-admin-bin PDB
	-cd examples && $GIMPTOOL2 --install-admin-bin alpha2color.pl
	-cd examples && $GIMPTOOL2 --install-admin-bin tex-to-float
#	-cd examples && $GIMPTOOL2 --install-admin-bin border.pl
EOF
}

# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.
WriteMakefile(
    'dist'	=> {
                    'PREOP'     => 'chmod -R u=rwX,go=rX . ;',
                    'COMPRESS'	=> 'gzip -9v',
                    'SUFFIX'	=> '.gz',
                   },
    'PREREQ_PM'	=> {
    		    "Gtk"		=> 0.3,
    		    "Data::Dumper"	=> 2,
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
    		   },
    'LIBS'	=> [''],
    'INC'	=> "$CPPFLAGS $CFLAGS $GIMP_INC_NOUI $DEFS",
    'DEFINE'	=> ($IN_GIMP ? " -DIN_GIMP" : ""),
    'realclean'	=> { FILES => "config.status config.cache config.log config.pl config.h" },
    'clean'	=> { FILES => "Makefile.old config.pl" },
);

$IN_GIMP or print <<EOF;


Hopefully, Gimp is now correctly configured. you can now enter "make", "make
test" and "make install".

EOF



