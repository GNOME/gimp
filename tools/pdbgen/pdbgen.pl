#!/usr/bin/perl -w

# The GIMP -- an image manipulation program
# Copyright (C) 1998-1999 Manish Singh <yosh@gimp.org>

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

require 5.002;

BEGIN {
    $srcdir = '.';
    $destdir = '.';
}

use lib $srcdir;

# Stifle "used only once" warnings
$destdir = $destdir;
%pdb = ();
@groups = ();

# The actual parser (in a string so we can eval it in another namespace)
$evalcode = <<'CODE';
{
    my $file = $main::file;
    my $srcdir = $main::srcdir;
    my $proc;
    my($var, $type);
    my($dest, $procs);

    # Variables to evaluate and insert into the PDB structure
    my @procvars = qw($name $group $blurb $help $author $copyright $date
		      @inargs @outargs %invoke);

    # Hook some variables into the top-level namespace
    *pdb = \%main::pdb;
    *gen = \%main::gen;
    *grp = \%main::grp;

    # Hide our globals
    my $safeeval = sub { local(%pdb, %gen, %grp); eval $_[0]; die $@ if $@ };

    # Some standard shortcuts used by all def files
    &$safeeval("do '$main::srcdir/stddefs.pdb'");

    # Group properties
    undef $desc; undef $code;

    # Load the file in and get the group info
    &$safeeval("require '$main::srcdir/pdb/$file.pdb'");

    # Save these for later
    $grp{$file}->{desc} = $desc if defined $desc;
    $grp{$file}->{code} = $code if defined $code;

    foreach $proc (@procs) {
	# Reset all our PDB vars so previous defs don't interfere
	foreach (@procvars) { eval "undef $_" }

	# Get the info
	&$safeeval("&$proc");

	# Some derived fields
	$name = $proc;
	$group = $file;

	# Load the info into %pdb, making copies of the data instead of refs
	my $entry = {};
	foreach (@procvars) {
	    if (eval "defined $_") {
		($var = $_) =~ s/^(\W)//, $type = $1;
		for ($type) {
		    /\$/ && do { $entry->{$var} =   $$var  ; last; };
		    /\@/ && do { $entry->{$var} = [ @$var ]; last; };
		    /\%/ && do { $entry->{$var} = { %$var }; last; };
		}
	    }
	}
	$pdb{$proc} = $entry;
    }

    # Find out what to do with these entries 
    while (($dest, $procs) = each %exports) { push @{$gen{$dest}}, @$procs; }
}
CODE

# What to do?
require 'groups.pl';

# Slurp in the PDB defs
foreach $file (@groups) {
    print "Processing $srcdir/pdb/$file.pdb...\n";
    eval "package Gimp::CodeGen::Safe::$file; $evalcode;";
    die $@ if $@;
}

# Some important stuff
require 'pdb.pl';
require 'enums.pl';
require 'util.pl';

# Squash whitespace into just single spaces between words
sub trimspace { for (${$_[0]}) { s/[\n\s]+/ /g; s/^ //; s/ $//; } }

# Trim spaces and escape quotes C-style
sub nicetext {
    my $val = shift;
    &trimspace($val);
    $$val =~ s/"/\\"/g;
}

# Do the same for all the strings in the args, plus expand constraint text
sub niceargs {
    my $args = shift;
    foreach $arg (@$args) {
	foreach (keys %$arg) {
	    &nicetext(\$arg->{$_});
	}
    }
}

# Trim spaces from all the elements in a list
sub nicelist {
    my $list = shift;
    foreach (@$list) { &trimspace(\$_) }
}

# Add args for array lengths

sub arrayexpand {
    my $args = shift;
    my $newargs;

    foreach (@$$args) {
	if (exists $_->{array}) {
	    my $arg = $_->{array};

	    $arg->{name} = 'num_' . $_->{name} unless exists $arg->{name};

	    # We can't have negative lengths, but let them set a min number
	    unless (exists $arg->{type}) {
		$arg->{type} = '0 < int32';
	    }
	    elsif ($arg->{type} !~ /^\s*\d+\s*</) {
		$arg->{type} = '0 < ' . $arg->{type};
	    }

	    $arg->{num} = 1;

	    push @$newargs, $arg;
 	}

	push @$newargs, $_;
    }

    $$args = $newargs;
}

# Post-process each pdb entry
while ((undef, $entry) = each %pdb) {
    &nicetext(\$entry->{blurb});
    &nicetext(\$entry->{help});
    &nicetext(\$entry->{author});
    &nicetext(\$entry->{copyright});
    &nicetext(\$entry->{date});

    foreach (qw(in out)) {
	my $args = $_ . 'args';
	if (exists $entry->{$args}) {
	    &arrayexpand(\$entry->{$args});
    	    &niceargs($entry->{$args});
	}
    }

    &nicelist($entry->{invoke}{headers}) if exists $entry->{invoke}{headers};
    &nicelist($entry->{globals}) if exists $entry->{globals};

    $entry->{invoke}{success} = 'TRUE' unless exists $entry->{invoke}{success};
}

# Generate code from the modules
my $didstuff;
while (@ARGV) {
    my $type = shift @ARGV;

    print "\nProcessing $type...\n";

    if (exists $gen{$type}) {
	require "$type.pl";
	&{"Gimp::CodeGen::${type}::generate"}($gen{$type});
	print "done.\n";
	$didstuff = 1;
    }
    else {
	print "nothing to do.\n";
    }
}

print "\nNothing done at all.\n" unless $didstuff;
