# The GIMP -- an image manipulation program
# Copyright (C) 1998 Manish Singh <yosh@gimp.org>

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

*write_file = \&Gimp::CodeGen::util::write_file;

sub generate {
    my @procs = @{(shift)};
    my %out;

    foreach my $name (@procs) {
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
	scalar @outargs and $retarg = $outargs[0] unless $retarg;

	my $rettype; my $retcol = 0;
	if ($retarg) {
	    my ($type) = &arg_parse($retarg->{type});
	    if ($type ne 'color') {
		my $arg = $arg_types{$type};
		$rettype = do {
		    if (exists $arg->{id_func}) { 'gint32 '    }
		    else                        { $arg->{type} }
		};
		chop $rettype unless $rettype =~ /\*$/;
	    }
	    else {
		# Color returns three components in pointers passed in
		$rettype = 'void'; $retcol = 1;
	    }
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
		    if ($id) { 'gint32 '    }
		    else     { $arg->{type} }
		};
		$arglist .= $_->{name};
		$arglist .= '_ID' if $id;
		$arglist .= ', ';
	    }
	    else {
		# A color needs to stick the components into a 3-element array
		$color = "\n" . ' ' x 2 . "guchar $_->{name}\[3];\n\n";
		$color .= ' ' x 2 . "$_->{name}\[0] = red;\n";
		$color .= ' ' x 2 . "$_->{name}\[1] = green;\n";
		$color .= ' ' x 2 . "$_->{name}\[2] = blue;";
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
	my $return_marshal;
	$return_marshal = "gimp_destroy_params (return_vals, nreturn_vals);";

	# We only need to bother with this if we have to return a value
	if ($rettype ne 'void' || $retcol) {
	    my $argc = 1; my $once = 0;
	    my $firstvar;
	    foreach (@outargs) {
		my ($type) = &arg_parse($_->{type});
		my $arg = $arg_types{$type};
		my $id = $arg->{id_ret_func};
		my $var;
		$return_marshal = "" unless $once++;
		if ($type ne 'color') {
		    $return_args .= "\n" . ' ' x 2;
		    $return_args .= do {
		        if ($id) { 'gint32 '    }
		        else     { $arg->{type} }
		    };

		    # The return value variable
		    $var = $_->{name};
		    $var .= '_ID' if $id;
		    $return_args .= $var;

		    # Save the first var to "return" it
		    $firstvar = $var unless defined $firstvar;

		    # Initialize all IDs to -1
		    $return_args .= " = -1" if $id;
		    $return_args .= ";";
		}
		$return_marshal .= <<CODE;
if (return_vals[0].data.d_status == STATUS_SUCCESS)
CODE
		if ($type ne 'color') {
		    $return_marshal .= ' ' x 4 . "$var = ";
		    $return_marshal .= 'g_strdup (' if $type eq 'string';
		    $return_marshal .= "return_vals[$argc].data.d_$type";
		    $return_marshal .= ')' if $type eq 'string';
		    $return_marshal .= ";\n";
		}
		else {
		    # Colors are returned in parts using pointers
		    $arglist .= "guchar \*red, guchar \*green, guchar \*blue";
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
	    $return_marshal .= <<'CODE';

  gimp_destroy_params (return_vals, nreturn_vals);

CODE
	    $return_marshal .= ' ' x 2 . "return $firstvar;" unless $retcol;
	    $return_marshal =~ s/\n\n$//s if $retcol;
	}

	if ($arglist) {
	    # We don't need the last comma in the declaration
	    $arglist =~ s/, $//;
	}

	# Our function prototype for the headers
        push @{$out->{proto}}, "$rettype gimp_$name ($arglist);\n";

	$out->{code} .= <<CODE;

$rettype
gimp_$name ($arglist)
{
  GParam *return_vals;
  int nreturn_vals;$return_args$color

  return_vals = gimp_run_procedure ("gimp_$name",
				    \&nreturn_vals,$argpass
				    PARAM_END);

  $return_marshal
}
CODE
    }

    my $lgpl = <<'LGPL';
/* LIBGIMP - The GIMP Library                                                   
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
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

LGPL

    # We generate two files, a .h file with prototypes for all the functions
    # we make, and a .c file for the actual implementation
    while (my($group, $out) = each %out) {
	my $hfile = "$destdir/gimp${group}.h.tmp.$$";
	my $cfile = "$destdir/gimp${group}.c.tmp.$$";

	my $protos;
	foreach (@{$out->{proto}}) { $protos .= $_ }
	chop $protos;

	open HFILE, "> $hfile" or die "Can't open $cfile: $!\n";
	print HFILE $lgpl;
	my $guard = "__GIMP_@{[uc $group]}_H__";
	print HFILE <<HEADER;
#ifndef $guard
#define $guard


#include <glib.h>
#include <libgimp/gimpprocs.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


$protos


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
	print CFILE $out->{code};
	close CFILE;
	&write_file($cfile);
    }
}
