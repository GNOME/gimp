#!/usr/bin/perl -w

open MK, "> Makefile.am";

require 'plugin-defs.pl';

$bins = ""; $opts = "";

foreach (sort keys %plugins) {
    $bins .= "\t";
    if (exists $plugins{$_}->{optional}) {
	$bins .= "\$(\U$_\E)";
	$opts .= "\t$_\t\t\\\n";
    }
    else {
	$bins .= $_;
    }
    $bins .= "\t\t\\\n";
}

foreach ($bins, $opts) { s/\t\t\\\n$//s }

print MK <<EOT;
pluginlibdir = \$(gimpplugindir)/plug-ins

AM_CPPFLAGS = \\
	-DLOCALEDIR=\\""\$(localedir)"\\"

INCLUDES = \\
	-I\$(top_srcdir)		\\
	\$(GTK_CFLAGS)		\\
	-I\$(includedir)

pluginlib_PROGRAMS = \\
$bins

EXTRA_PROGRAMS = \\
$opts
EOT

foreach (sort keys %plugins) {
    my $libgimp = "";

    $libgimp .= "\$(top_builddir)/libgimp/libgimp.la";
    if (exists $plugins{$_}->{ui}) {
	$libgimp .= "\t\\\n\t$libgimp";
	$libgimp =~ s/gimp\./gimpui./;
    }

    my $optlib = ""; 
    if (exists $plugins{$_}->{optional}) {
	$optlib = "\n\t\$(LIB\U$_\E_LIB)\t\\";
    }

    if (exists $plugins{$_}->{libsupp}) {
	my @lib = split(/:/, $plugins{$_}->{libsupp});
	foreach $lib (@lib) {
	    $libgimp = "\$(top_builddir)/plug-ins/$lib/lib$lib.a\t\\\n\t$libgimp";
	}
	$libgimp =~ s@gck/libgck\.a@libgck/gck/libgck.la@;
    }

    print MK <<EOT;

${_}_SOURCES = \\
	$_.c

${_}_LDADD = \\
	$libgimp	\\$optlib
	\$(\U$plugins{$_}->{libdep}\E_LIBS)				\\
	\$(INTLLIBS)
EOT
}

close MK;
