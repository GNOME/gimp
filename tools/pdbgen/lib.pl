# The GIMP -- an image manipulation program
# Copyright (C) 1998-2000 Manish Singh <yosh@gimp.org>

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
	my $arg = shift;
	my ($type, $name) = &arg_parse($arg->{type});
	my $argtype = $arg_types{$type};
	
	if (exists $argtype->{id_func} || $arg->{type} =~ /guide/) {
	    return 'gint32 ';
	}

	if ($type eq 'enum') {
	    $name = "Gimp$name" if $name !~ /^Gimp/;
	    return "$name ";
	}

	my $rettype = $argtype->{type};
	$rettype =~ s/int32/int/ unless exists $arg->{keep_size};
	return $rettype;
    }

    foreach $name (@procs) {
	my $proc = $main::pdb{$name};
	my $out = \%{$out{$proc->{group}}};

	my @inargs = @{$proc->{inargs}} if exists $proc->{inargs};
	my @outargs = @{$proc->{outargs}} if exists $proc->{outargs};

	my $funcname = "gimp_$name"; my $wrapped = ""; my %usednames;

	# The 'color' argument is special cased to accept and return the
	# individual color components. This is to maintain backwards
	# compatibility, but it certainly won't fly for other color models
	# It also makes the code a bit messier.

	# Find the return argument (defaults to the first arg if not
	# explicity set
	my $retarg  = undef; $retvoid = 0;
	foreach (@outargs) { $retarg = $_, last if exists $_->{retval} }
	unless ($retarg) {
	    if (scalar @outargs) {
		if (exists $outargs[0]->{void_ret}) {
		    $retvoid = 1;
		}
		else {
		    $retarg = exists $outargs[0]->{num} ? $outargs[1]
							: $outargs[0];
		}
	    }
	}

	my $rettype; my $retcol = 0;
	if ($retarg) {
	    my ($type) = &arg_parse($retarg->{type});
	    if ($type ne 'color') {
		$rettype = &libtype($retarg);
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
	my $arglist = ""; my $argpass = ""; my $color = ""; my $privatevars = 0;
	foreach (@inargs) {
	    my ($type) = &arg_parse($_->{type});
	    my $arg = $arg_types{$type};
	    my $id = exists $arg->{id_func} || $_->{type} =~ /guide/;

	    $wrapped = "_" if exists $_->{wrap};

	    $usednames{$_->{name}}++;

	    if (exists $_->{implicit_fill}) {
		$privatevars++;
	    }
	    elsif ($type ne 'color') {
		$arglist .= &libtype($_);
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
	    $argpass .= "PARAM_$arg->{name}, ";

	    if (exists $_->{implicit_fill}) {
		$argpass .= $_->{implicit_fill};
	    }
	    else {
		$argpass .= "$_->{name}";
		$argpass .= '_ID' if $id;
	    }

	    $argpass .= ',';
	}

	# This marshals the return value(s)
	my $return_args = "";
	my $return_marshal = "gimp_destroy_params (return_vals, nreturn_vals);";

	# We only need to bother with this if we have to return a value
	if ($rettype ne 'void' || $retcol || $retvoid) {
	    my $once = 0;
	    my $firstvar;
	    my @initnums;

	    foreach (@outargs) {
		my ($type) = &arg_parse($_->{type});
		my $arg = $arg_types{$type};
		my $id = $arg->{id_ret_func} || $_->{type} =~ /guide/;
		my $var;

		$return_marshal = "" unless $once++;

		$wrapped = "_" if exists $_->{wrap};

		$_->{libname} = exists $usednames{$_->{name}} ? "ret_$_->{name}"
							      : $_->{name};

	        if (exists $_->{num}) {
		    if (!exists $_->{no_lib}) {
			push @initnums, $_;
		    }
		}
		elsif (exists $_->{retval} && $type ne 'color') {
		    $return_args .= "\n" . ' ' x 2;
		    $return_args .= &libtype($_);

		    # The return value variable
		    $var = $_->{libname};
		    $var .= '_ID' if $id;
		    $return_args .= $var;

		    # Save the first var to "return" it
		    $firstvar = $var unless defined $firstvar;

		    if (exists $_->{libdef}) {
			$return_args .= " = $_->{libdef}";
		    }
		    elsif ($id) {
			# Initialize all IDs to -1
			$return_args .= " = -1";
		    }
		    elsif ($arg->{type} =~ /\*/) {
			# Initialize pointers to NULL
			$return_args .= " = NULL";
		    }
		    elsif ($arg->{type} =~ /boolean/) {
			$return_args .= " = FALSE";
		    }
		    else {
			# Default to 0
			$return_args .= " = 0";
		    }

		    $return_args .= ";";

		    if (exists $_->{array} && exists $_->{array}->{no_lib}) {
			$return_args .= "\n" . ' ' x 2 . "gint num_$var;";
		    }
		}
		elsif ($retvoid) {
		    push @initnums, $_;
		}
	    }

	    if (scalar(@initnums)) {
		foreach (@initnums) {
		    $return_marshal .= "\*$_->{libname} = ";
		    my ($type) = &arg_parse($_->{type});
		    for ($arg_types{$type}->{type}) {
			/\*$/     && do { $return_marshal .= "NULL";  last };
			/boolean/ && do { $return_marshal .= "FALSE"; last };
			/double/  && do { $return_marshal .= "0.0";   last };
					  $return_marshal .= "0";
		    }
		    $return_marshal .= ";\n  ";
		}
		$return_marshal =~ s/\n  $/\n\n  /s;
	    }

	    $return_marshal .= <<CODE;
if (return_vals[0].data.d_status == STATUS_SUCCESS)
CODE

	    $return_marshal .= ' ' x 4 . "{\n" if $#outargs;

	    my $argc = 1; my ($numpos, $numtype);
	    foreach (@outargs) {
		my ($type) = &arg_parse($_->{type});
		my $arg = $arg_types{$type};
		my $id = $arg->{id_ret_func} || $_->{type} =~ /guide/;
		my $var;

		my $ch = ""; my $cf = "";
		if ($type =~ /^string(array)?/) {
		    $ch = 'g_strdup (';
		    $cf = ')';
		}
		elsif ($type =~ /parasite/) {
		    $ch = 'parasite_copy (&';
		    $cf = ')';
		}
		elsif ($type =~ /boolean|enum|guide/) {
		    $type = 'int32';
		}

		if (exists $_->{num}) {
		    $numpos = $argc;
		    $numtype = $type;
		    if (!exists $_->{no_lib}) {
			$arglist .= "gint \*$_->{libname}, ";
		    }
		}
		elsif (exists $_->{array}) {
		    my $datatype = $arg->{type};
		    chop $datatype;
		    $datatype =~ s/ *$//;

		    my $var = $_->{libname}; my $dh = ""; my $df = "";
		    unless (exists $_->{retval}) {
			$var = "*$var"; $dh = "(*"; $df = ")";
			$arglist .= "$datatype **$_->{libname}";
		    }

		    if ($ch || $cf) {
			$return_args .= "\n" . ' ' x 2 . "gint i;";
		    }

		    my $numvar = '*' . $_->{array}->{name};
		    $numvar = "num_$_->{name}" if exists $_->{array}->{no_lib};

		    $return_marshal .= <<NEW . (($ch || $cf) ? <<CP1 : <<CP2);
      $numvar = return_vals[$numpos].data.d_$numtype;
      $var = g_new ($datatype, $numvar);
NEW
      for (i = 0; i < $numvar; i++)
	$dh$_->{name}$df\[i] = ${ch}return_vals[$argc].data.d_$type\[i]${cf};
CP1
      memcpy ($var, return_vals[$argc].data.d_$type,
	      $numvar * sizeof ($datatype));
CP2
		}
		elsif ($type ne 'color') {
		    # The return value variable
		    $var = "";

		    unless (exists $_->{retval}) {
			$var .= '*';
			$arglist .= &libtype($_);
			$arglist .= "*$_->{libname}";
			$arglist .= '_ID' if $id;
			$arglist .= ', ';
		    }

		    $var = exists $_->{retval} ? "" : '*';
		    $var .= $_->{libname};
		    $var .= '_ID' if $id;

		    $return_marshal .= ' ' x 2 if $#outargs;
		    $return_marshal .= <<CODE
    $var = ${ch}return_vals[$argc].data.d_$type${cf};
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
	    unless ($retcol || $retvoid) {
		$return_marshal .= ' ' x 2 . "return $firstvar;";
	    }
	    else {
		$return_marshal =~ s/\n\n$//s;
	    }
	}

	if ($arglist) {
	    my @arglist = split(/, /, $arglist);
	    my $longest = 0; my $seen = 0;
	    foreach (@arglist) {
		/(\w+) \S+/;
		my $len = length($1);
		my $num = scalar @{[ /\*/g ]};
		$seen = $num if $seen < $num;
		$longest = $len if $longest < $len;
	    }

	    $longest += $seen;

	    my $once = 0; $arglist = "";
	    foreach (@arglist) {
		my $len = $longest - index($_, ' ') + 1;
		$len -= scalar @{[ /\*/g ]};
		s/ /' ' x $len/e if $len > 1;
		$arglist .= "\t" if $once;
                $arglist .= $_;
                $arglist .= ",\n";
		$once++;
	    }
	    $arglist =~ s/,\n$//;
	}
	else {
	    $arglist = "void";
	}

	# Our function prototype for the headers
	(my $hrettype = $rettype) =~ s/ //g;

	my $proto = "$hrettype $wrapped$funcname ($arglist);\n";
	$proto =~ s/ +/ /g;
        push @{$out->{protos}}, $proto;

	my $clist = $arglist;
	my $padlen = length($wrapped) + length($funcname) + 2;
	my $padtab = $padlen / 8; my $padspace = $padlen % 8;
	my $padding = "\t" x $padtab . ' ' x $padspace;
	$clist =~ s/\t/$padding/eg;
	
	$out->{code} .= <<CODE;

$rettype
$wrapped$funcname ($clist)
{
  GParam *return_vals;
  gint nreturn_vals;$return_args$color

  return_vals = gimp_run_procedure ("$funcname",
				    \&nreturn_vals,$argpass
				    PARAM_END);

  $return_marshal
}
CODE
    }

    my $lgpl_top = <<'LGPL';
/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
LGPL

    my $lgpl_bottom = <<'LGPL';
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* NOTE: This file is autogenerated by pdbgen.pl */

LGPL

    # We generate two files, a _pdb.h file with prototypes for all
    # the functions we make, and a _pdb.c file for the actual implementation
    while (my($group, $out) = each %out) {
        my $hname = "gimp${group}pdb.h"; 
        my $cname = "gimp${group}pdb.c"; 
        $hname =~ s/_//g; $hname =~ s/pdb\./_pdb./;
        $cname =~ s/_//g; $cname =~ s/pdb\./_pdb./;
	my $hfile = "$destdir/$hname$FILE_EXT";
	my $cfile = "$destdir/$cname$FILE_EXT";

	my $extra = {};
	if (exists $main::grp{$group}->{extra}->{lib}) {
	    $extra = $main::grp{$group}->{extra}->{lib}
	}

	if (exists $extra->{protos}) {
	    my $proto = "";
	    foreach (split(/\n/, $extra->{protos})) {
		next if /^\s*$/;

		if (/^\t/ && length($proto)) {
		    s/\s+/ /g; s/ $//; s/^ /\t/;
		    $proto .= $_ . "\n";
		}
		else {
		    push @{$out->{protos}}, $proto if length($proto);

		    s/\s+/ /g; s/^ //; s/ $//;
		    $proto = $_ . "\n";
		}
	    }
	}

	my @longest = (0, 0, 0); my @arglist = (); my $seen = 0;
	foreach (@{$out->{protos}}) {
	    my $len; my $arglist = [ split(' ', $_, 3) ];

	    for (0..1) {
		$len = length($arglist->[$_]);
		$longest[$_] = $len if $longest[$_] < $len;
	    }

	    foreach (split(/,/, $arglist->[2])) {
		/(\w+) \S+/;
		$len = length($1) + 1;
		my $num = scalar @{[ /\*/g ]};
		$seen = $num if $seen < $num;
		$longest[2] = $len if $longest[2] < $len;
	    }

	    push @arglist, $arglist;
	}

	$longest[2] += $seen;

	@{$out->{protos}} = ();
	foreach (@arglist) {
	    my ($type, $func, $arglist) = @$_;

	    my @args = split(/,/, $arglist); $arglist = "";
	    foreach (@args) {
		my $len = $longest[2] - index($_, ' ') + 1;
		$len -= scalar @{[ /\*/g ]};
		$len++ if /\t/;
		s/ /' ' x $len/e if $len > 1;
		$arglist .= $_;
		$arglist .= "," if !/;\n$/;
	    }
	    my $arg = $type;
	    $arg .= ' ' x ($longest[0] - length($type) + 1) . $func;
	    $arg .= ' ' x ($longest[1] - length($func) + 1) . $arglist;
	    $arg =~ s/\t/' ' x ($longest[0] + $longest[1] + 3)/eg;
	    while ($arg =~ /^\t* {8}/m) { $arg =~ s/^(\t*) {8}/$1\t/mg }
	    push @{$out->{protos}}, $arg;
	}

	my $body;
	$body = $extra->{decls} if exists $extra->{decls};
	foreach (@{$out->{protos}}) { $body .= $_ }
	chomp $body;

	open HFILE, "> $hfile" or die "Can't open $hfile: $!\n";
        print HFILE $lgpl_top;
        print HFILE " * $hname\n";
        print HFILE $lgpl_bottom;
 	my $guard = "__GIMP_\U$group\E_PDB_H__";
	print HFILE <<HEADER;
#ifndef $guard
#define $guard

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* For information look into the C source or the html documentation */


$body


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* $guard */
HEADER
	close HFILE;
	&write_file($hfile);

	open CFILE, "> $cfile" or die "Can't open $cfile: $!\n";
        print CFILE $lgpl_top;
        print CFILE " * $cname\n";
        print CFILE $lgpl_bottom;
	print CFILE qq/#include "gimp.h"\n/;
	print CFILE qq/#include "gimpprivate.h"\n/ if $privatevars;
	print CFILE "\n", $extra->{code} if exists $extra->{code};
	print CFILE $out->{code};
	close CFILE;
	&write_file($cfile);
    }
}
