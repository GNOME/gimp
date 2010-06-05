#!/usr/bin/perl -w

# GIMP - The GNU Image Manipulation Program
# Copyright (C) 1998-2003 Manish Singh <yosh@gimp.org>

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

require 5.004;

BEGIN {
    $srcdir  = $ENV{srcdir}  || '.';
    $destdir = $ENV{destdir} || '.';
}

use lib $srcdir;

BEGIN {
    # Some important stuff
    require 'pdb.pl';
    require 'enums.pl';
    require 'util.pl';

    # What to do?
    require 'groups.pl';

    if ($ENV{PDBGEN_GROUPS}) {
	@groups = split(/:/, $ENV{PDBGEN_GROUPS});
    }
}

# Stifle "used only once" warnings
$destdir = $destdir;
%pdb = ();

# The actual parser (in a string so we can eval it in another namespace)
$evalcode = <<'CODE';
{
    my $file = $main::file;
    my $srcdir = $main::srcdir;

    my $copyvars = sub {
	my $dest = shift;

	foreach (@_) {
	    if (eval "defined scalar $_") {
		(my $var = $_) =~ s/^(\W)//;
		 for ($1) {
		    /\$/ && do { $$dest->{$var} =   $$var  ; last; };
		    /\@/ && do { $$dest->{$var} = [ @$var ]; last; };
		    /\%/ && do { $$dest->{$var} = { %$var }; last; };
		}
	    }
	}
    };

    # Variables to evaluate and insert into the PDB structure
    my @procvars = qw($name $group $blurb $help $author $copyright $date $since
		      $deprecated @inargs @outargs %invoke $canonical_name);

    # These are attached to the group structure
    my @groupvars = qw($desc @headers %extra);

    # Hook some variables into the top-level namespace
    *pdb = \%main::pdb;
    *gen = \%main::gen;
    *grp = \%main::grp;

    # Hide our globals
    my $safeeval = sub { local(%pdb, %gen, %grp); eval $_[0]; die $@ if $@ };

    # Some standard shortcuts used by all def files
    &$safeeval("do '$main::srcdir/stddefs.pdb'");

    # Group properties
    foreach (@groupvars) { eval "undef $_" }

    # Load the file in and get the group info
    &$safeeval("require '$main::srcdir/pdb/$file.pdb'");

    # Save these for later
    &$copyvars(\$grp{$file}, @groupvars);

    foreach $proc (@procs) {
	# Reset all our PDB vars so previous defs don't interfere
	foreach (@procvars) { eval "undef $_" }

	# Get the info
	&$safeeval("&$proc");

	# Some derived fields
	$name = $proc;
	$group = $file;

	($canonical_name = $name) =~ s/_/-/g;

	# Load the info into %pdb, making copies of the data instead of refs
	my $entry = {};
	&$copyvars(\$entry, @procvars);
	$pdb{$proc} = $entry;
    }

    # Find out what to do with these entries 
    while (my ($dest, $procs) = each %exports) { push @{$gen{$dest}}, @$procs }
}
CODE

# Slurp in the PDB defs
foreach $file (@groups) {
    print "Processing $srcdir/pdb/$file.pdb...\n";
    eval "package Gimp::CodeGen::Safe::$file; $evalcode;";
    die $@ if $@;
}

# Squash whitespace into just single spaces between words
sub trimspace { for (${$_[0]}) { s/\s+/ /gs; s/^ //; s/ $//; } }

# Trim spaces and escape quotes C-style
sub nicetext {
    my $val = shift;
    if (defined $$val) {
	&trimspace($val);
	$$val =~ s/"/\\"/g;
    }
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
		$arg->{type} = '0 <= int32';
	    }
	    elsif ($arg->{type} !~ /^\s*\d+\s*</) {
		$arg->{type} = '0 <= ' . $arg->{type};
	    }

	    $arg->{void_ret} = 1 if exists $_->{void_ret};

	    $arg->{num} = 1;

	    push @$newargs, $arg;
 	}

	push @$newargs, $_;
    }

    $$args = $newargs;
}

sub canonicalargs {
    my $args = shift;
    foreach $arg (@$args) {
        ($arg->{canonical_name} = $arg->{name}) =~ s/_/-/g;
    }
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
	    &canonicalargs($entry->{$args});
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
