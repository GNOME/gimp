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

package Gimp::CodeGen::lib;

# Generates all the libgimp C wrappers (used by plugins)
$destdir = "$main::destdir/libgimp";

*arg_types = \%Gimp::CodeGen::pdb::arg_types;
*arg_parse = \&Gimp::CodeGen::pdb::arg_parse;

*enums = \%Gimp::CodeGen::enums::enums;

*write_file = \&Gimp::CodeGen::util::write_file;
*FILE_EXT   = \$Gimp::CodeGen::util::FILE_EXT;

sub generate {
    my @procs = @{(shift)};
    my %out;

    sub libtype {
	my ($arg, $type) = @_;
	$type =~ s/\d+// unless exists $arg->{keep_size};
	$type;
    }

    foreach $name (@procs) {
	my $proc = $main::pdb{$name};
	my $out = \%{$out{$proc->{group}}};

	my @inargs = @{$proc->{inargs}} if exists $proc->{inargs};
	my @outargs = @{$proc->{outargs}} if exists $proc->{outargs};

	# The 'color' argument is special cased to accept and return the
	# individual color components. This is to maintain backwards
	# compatibility, but it certainly won't fly for other color models
	# It also makes the code a bit messier.

	# Find the return argument (defaults to the first arg if not
	# explicity set
	my $retarg;
	foreach (@outargs) { $retarg = $_, last if exists $_->{retval} }
	unless ($retarg) {
	    if (scalar @outargs) {
		$retarg = exists $outargs[0]->{num} ? $outargs[1] : $outargs[0];
	    }
	}

	my $rettype; my $retcol = 0;
	if ($retarg) {
	    my ($type) = &arg_parse($retarg->{type});
	    if ($type ne 'color') {
		my $arg = $arg_types{$type};
		$rettype = do {
		    if (exists $arg->{id_func}) { 'gint32 '                  }
		    else                        { &libtype($_, $arg->{type}) }
		};
		chop $rettype unless $rettype =~ /\*$/;
	    }
	    else {
		# Color returns three components in pointers passed in
		$rettype = 'void'; $retcol = 1;
	    }

	    $retarg->{retval} = 1;
	}
	else {
	    # No return values
	    $rettype = 'void';
	}

	# The parameters to the function
	my $arglist = ""; my $argpass = ""; my $color = "";
	foreach (@inargs) {
	    my ($type) = &arg_parse($_->{type});
	    my $arg = $arg_types{$type};
	    my $id = exists $arg->{id_func};

	    if ($type ne 'color') {
		$arglist .= do {
		    if ($id) { 'gint32 '                  }
		    else     { &libtype($_, $arg->{type}) }
		};

		$arglist .= $_->{name};
		$arglist .= '_ID' if $id;
		$arglist .= ', ';
	    }
	    else {
		# A color needs to stick the components into a 3-element array
		chop ($color = <<CODE);

  guchar $_->{name}\[3];

  $_->{name}\[0] = red;
  $_->{name}\[1] = green;
  $_->{name}\[2] = blue;
CODE

		$arglist .= "guchar red, guchar green, guchar blue, ";
	    }

	    # This is what's passed into gimp_run_procedure
	    $argpass .= "\n\t\t\t\t" . ' ' x 4;
	    $argpass .= "PARAM_$arg->{name}, $_->{name}";
	    $argpass .= '_ID' if $id;
	    $argpass .= ',';
	}

	# This marshals the return value(s)
	my $return_args = "";
	my $return_marshal = "gimp_destroy_params (return_vals, nreturn_vals);";

	# We only need to bother with this if we have to return a value
	if ($rettype ne 'void' || $retcol) {
	    my $once = 0;
	    my $firstvar;
	    my @arraynums;

	    foreach (@outargs) {
		my ($type) = &arg_parse($_->{type});
		my $arg = $arg_types{$type};
		my $id = $arg->{id_ret_func};
		my $var;

		$return_marshal = "" unless $once++;

	        if (exists $_->{num}) {
		    if (!exists $_->{no_lib}) {
			$arglist .= "gint \*$_->{name}, ";
			push @arraynums, $_;
		    }
		}
		elsif ($type ne 'color') {
		    $return_args .= "\n" . ' ' x 2;
		    $return_args .= do {
			if ($id) { 'gint32 '    }
			else     { &libtype($_, $arg->{type}) }
		    };

		    # The return value variable
		    $var = $_->{name};
		    $var .= '_ID' if $id;
		    $return_args .= $var;

		    # Save the first var to "return" it
		    $firstvar = $var unless defined $firstvar;

		    # Initialize all IDs to -1
		    $return_args .= " = -1" if $id;

		    # Initialize pointers to NULL
		    $return_args .= " = NULL" if !$id && ($arg->{type} =~ /\*/);

		    $return_args .= ";";

		    if (exists $_->{array} && exists $_->{array}->{no_lib}) {
			$return_args .= "\n" . ' ' x 2 . "gint num_$var;";
		    }
		}
	    }

	    foreach (@arraynums) { $return_marshal .= "\*$_->{name} = 0;\n  " }
	    $return_marshal =~ s/\n  $/\n\n  /s if scalar(@arraynums);

	    $return_marshal .= <<CODE;
if (return_vals[0].data.d_status == STATUS_SUCCESS)
CODE

	    $return_marshal .= ' ' x 4 . "{\n" if $#outargs;

	    my $argc = 1; my ($numpos, $numtype);
	    foreach (@outargs) {
		my ($type) = &arg_parse($_->{type});
		my $arg = $arg_types{$type};
		my $id = $arg->{id_ret_func};
		my $var;

		my $head = ""; my $foot = "";
		if ($type =~ /^string(array)?/) {
		    $head = 'g_strdup (';
		    $foot = ')';
		}

	        if (exists $_->{num}) {
		    $numpos = $argc;
		    $numtype = $type;
		}
		elsif (exists $_->{array}) {
		    my $datatype = $arg->{type};
		    chop $datatype;
		    $datatype =~ s/ *$//;

		    $return_args .= "\n" . ' ' x 2 . "gint i;";

		    my $numvar = '*' . $_->{array}->{name};
		    $numvar = "num_$_->{name}" if exists $_->{array}->{no_lib};

		    $return_marshal .= <<CODE;
      $numvar = return_vals[$numpos].data.d_$numtype;
      $_->{name} = g_new ($datatype, $numvar);
      for (i = 0; i < $numvar; i++)
	$_->{name}\[i] = ${head}return_vals[$argc].data.d_$type\[i]${foot};
CODE
		}
		elsif ($type ne 'color') {
		    # The return value variable
		    $var = $_->{name};
		    $var .= '_ID' if $id;

		    $return_marshal .= ' ' x 2 if $#outargs;
		    $return_marshal .= <<CODE
    $var = ${head}return_vals[$argc].data.d_$type${foot};
CODE
		}
		else {
		    # Colors are returned in parts using pointers
		    $arglist .= "guchar \*red, guchar \*green, guchar \*blue, ";
		    $return_marshal .= <<CODE
    {
      \*red = return_vals[$argc].data.d_color.red;
      \*green = return_vals[$argc].data.d_color.green;
      \*blue = return_vals[$argc].data.d_color.blue;
    }
CODE
		}

		$argc++;
	    }

	    $return_marshal .= ' ' x 4 . "}\n" if $#outargs;

	    $return_marshal .= <<'CODE';

  gimp_destroy_params (return_vals, nreturn_vals);

CODE
	    $return_marshal .= ' ' x 2 . "return $firstvar;" unless $retcol;
	    $return_marshal =~ s/\n\n$//s if $retcol;
	}

	if ($arglist) {
	    my @arglist = ();
	    my $longest = 0; my $indirect = 0;
	    foreach (split(/, /, $arglist)) {
		my ($type, $var) = /(\w+) ((?:\w|\*)+)/;
		my $num = scalar @{[ $var =~ /\*/g ]};

		push @arglist, [ $type, $var, $num ];

		$longest = length $type if $longest < length $type;
		$indirect = $num if $indirect < $num;
	    }

	    $longest += $indirect + 1;

	    my $once = 0; $arglist = "";
	    foreach (@arglist) {
		my ($type, $var, $num) = @$_;
		my $space = $longest - length($type) - $num;
		$arglist .= ",\n\t" if $once++;
		$arglist .= $type . ' ' x $space . $var;
	    }

	    $arglist =~ s/ +/ / if !$#arglist;
	}
	else {
	    $arglist = "void";
	}

	my $funcname = "gimp_$name";

	# Our function prototype for the headers
	(my $hrettype = $rettype) =~ s/ //g;

	my $proto = "$hrettype gimp_$name ($arglist);\n";
	$proto =~ s/ +/ /g;
        push @{$out->{proto}}, $proto;

	my $clist = $arglist;
	$clist =~ s/\t/' ' x (length("gimp_$name") + 2)/eg;
	$clist =~ s/ {8}/\t/g;

	$out->{code} .= <<CODE;

$rettype
gimp_$name ($clist)
{
  GParam *return_vals;
  gint nreturn_vals;$return_args$color

  return_vals = gimp_run_procedure ("gimp_$name",
				    \&nreturn_vals,$argpass
				    PARAM_END);

  $return_marshal
}
CODE
    }

    my $lgpl = <<'LGPL';
/* LIBGIMP - The GIMP Library                                                   
 * Copyright (C) 1995-1999 Peter Mattis and Spencer Kimball                
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.             
 *                                                                              
 * This library is distributed in the hope that it will be useful,              
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU            
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */                                                                             

/* NOTE: This file is autogenerated by pdbgen.pl */

LGPL

    # We generate two files, a .h file with prototypes for all the functions
    # we make, and a .c file for the actual implementation
    while (my($group, $out) = each %out) {
	my $hfile = "$destdir/gimp${group}.h$FILE_EXT";
	my $cfile = "$destdir/gimp${group}.c$FILE_EXT";

	my $extra = {};
	if (exists $main::grp{$group}->{extra}->{lib}) {
	    $extra = $main::grp{$group}->{extra}->{lib}
	}

	push @{$out->{proto}}, $extra->{protos} if exists $extra->{protos};

	my @longest = (0, 0, 0); my @arglist = (); my $seen = 0;
	foreach (@{$out->{proto}}) {
	    my $len; my $arglist = [ split(' ', $_, 3) ];

	    for (0..1) {
		$len = length($arglist->[$_]);
		$longest[$_] = $len if $longest[$_] < $len;
	    }

	    my @arg = split(/,/, $arglist->[2]);
	    foreach (@arg) {
		/(\w+) \S+/;
		$len = length($1) + 1;
		$seen++ if /\*/;
		$longest[2] = $len if $longest[2] < $len;
	    }

	    push @arglist, $arglist;
	}

	$longest[2]++ if $seen;

	@{$out->{proto}} = ();
	foreach (@arglist) {
	    my ($type, $func, $arglist) = @$_;

	    my @args = split(/,/, $arglist); $arglist = "";
	    foreach (@args) {
		my $len = $longest[2] - index($_, ' ') + 1;
		$len-- if /\*/;
		s/ /' ' x $len/e if $len > 1;
		$arglist .= $_;
		$arglist .= "," if !/;\n$/;
	    }
	    my $arg = $type;
	    $arg .= ' ' x ($longest[0] - length($type) + 1) . $func;
	    $arg .= ' ' x ($longest[1] - length($func) + 1) . $arglist;
	    $arg =~ s/\t/' ' x ($longest[0] + $longest[1] + 3)/eg;
	    while ($arg =~ /^\t* {8}/m) { $arg =~ s/^(\t*) {8}/$1\t/mg }
	    push @{$out->{proto}}, $arg;
	}

	my $body;
	$body = $extra->{decls} if exists $extra->{decls};
	foreach (@{$out->{proto}}) { $body .= $_ }
	$body .= $extra->{protos} if exists $extra->{protos};
	chomp $body;

	open HFILE, "> $hfile" or die "Can't open $cfile: $!\n";
	print HFILE $lgpl;
	my $guard = "__GIMP_\U$group\E_H__";
	print HFILE <<HEADER;
#ifndef $guard
#define $guard


#include <glib.h>
#include <libgimp/gimpprocs.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


$body


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* $guard */
HEADER
	close HFILE;
	&write_file($hfile);

	open CFILE, "> $cfile" or die "Can't open $cfile: $!\n";
	print CFILE $lgpl;
	print CFILE qq/#include "gimp${group}.h"\n/;
	print CFILE "\n", $extra->{code} if exists $extra->{code};
	print CFILE $out->{code};
	close CFILE;
	&write_file($cfile);
    }

    my $hfile = "$destdir/gimpenums.h$FILE_EXT";
    open HFILE, "> $hfile" or die "Can't open $cfile: $!\n";
    print HFILE $lgpl;
    my $guard = "__GIMP_ENUMS_H__";
    print HFILE <<HEADER;
#ifndef $guard
#define $guard


HEADER

    foreach (sort keys %enums) {
	print HFILE "typedef enum\n(\n";

	my $enum = $enums{$_}; my $body = "";
	foreach $symbol (@{$enum->{symbols}}) {
	    $body .= "  $symbol";
	    $body .= " = $enum->{mapping}->{$symbol}" if !$enum->{contig};
	    $body .= ",\n";
	}

	$body =~ s/,\n$//s;
	$body .= "\n} Gimp$_;\n\n";
	print HFILE $body
    }

    print HFILE <<HEADER;

#endif /* $guard */
HEADER
    close HFILE;
    &write_file($hfile);
}
