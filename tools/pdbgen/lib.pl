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

package Gimp::CodeGen::lib;

# Generates all the libgimp C wrappers (used by plugins)
$destdir = "$main::destdir/libgimp";

*arg_types = \%Gimp::CodeGen::pdb::arg_types;
*arg_parse = \&Gimp::CodeGen::pdb::arg_parse;

*enums = \%Gimp::CodeGen::enums::enums;

*write_file = \&Gimp::CodeGen::util::write_file;
*FILE_EXT   = \$Gimp::CodeGen::util::FILE_EXT;

use Text::Wrap qw(wrap);

sub desc_wrap {
    my ($str) = @_;
    my $leading = ' * ';
    $str =~ s/&/&amp\;/g;
    $str =~ s/\</&lt\;/g;
    $str =~ s/\>/&gt\;/g;
    $Text::Wrap::columns = 72;
    wrap($leading, $leading, $str);
}

sub generate {
    my @procs = @{(shift)};
    my %out;

    sub libtype {
	my $arg = shift;
	my $outarg = shift;
	my ($type, $name) = &arg_parse($arg->{type});
	my $argtype = $arg_types{$type};
	my $rettype = '';
	
	if (exists $argtype->{id}) {
	    return 'gint32 ';
	}

	if ($type eq 'enum') {
	    return "$name ";
	}

	if ($outarg) {
	    $rettype .= $argtype->{type};
	}
	else {
	    if (exists $argtype->{struct}) {
		$rettype .= 'const ';
	    }
	    $rettype .= $argtype->{const_type};
	}
	
	$rettype =~ s/int32/int/ unless exists $arg->{keep_size};
	$rettype .= '*' if exists $argtype->{struct};
	return $rettype;
    }

    foreach $name (@procs) {
	my $proc = $main::pdb{$name};
	my $out = \%{$out{$proc->{group}}};

	my @inargs = @{$proc->{inargs}} if exists $proc->{inargs};
	my @outargs = @{$proc->{outargs}} if exists $proc->{outargs};

	my $funcname = "gimp_$name"; my $wrapped = "";
	my %usednames;
	my $retdesc = "";

	if ($proc->{deprecated} && !$out->{deprecated}) {
	    push @{$out->{protos}}, "#ifndef GIMP_DISABLE_DEPRECATED\n";
	    $out->{deprecated} = 1;
	    $out->{seen_deprecated} = 1;
	}
	elsif (!$proc->{deprecated} && $out->{deprecated}) {
	    push @{$out->{protos}}, "#endif /* GIMP_DISABLE_DEPRECATED */\n";
	    $out->{deprecated} = 0;
	}

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

	my $rettype;
	if ($retarg) {
	    $rettype = &libtype($retarg, 1);
	    chop $rettype unless $rettype =~ /\*$/;

	    $retarg->{retval} = 1;

	    $retdesc = exists $retarg->{desc} ? $retarg->{desc} : "";
	}
	else {
	    # No return values
	    $rettype = 'void';
	}

	# The parameters to the function
	my $arglist = ""; my $argpass = "";
	my $argdesc = ""; my $sincedesc = "";
	foreach (@inargs) {
	    my ($type) = &arg_parse($_->{type});
	    my $desc = exists $_->{desc} ? $_->{desc} : "";
	    my $arg = $arg_types{$type};

	    $wrapped = "_" if exists $_->{wrap};

	    $usednames{$_->{name}}++;

	    $arglist .= &libtype($_, 0);
	    $arglist .= $_->{name};
	    $arglist .= '_ID' if $arg->{id};
	    $arglist .= ', ';

	    $argdesc .= " * \@$_->{name}";
	    $argdesc .= '_ID' if $arg->{id};
	    $argdesc .= ": $desc";

	    # This is what's passed into gimp_run_procedure
	    $argpass .= "\n" . ' ' x 36;
	    $argpass .= "GIMP_PDB_$arg->{name}, ";

	    $argpass .= "$_->{name}";
	    $argpass .= '_ID' if $arg->{id};

	    $argpass .= ',';

            unless ($argdesc =~ /[\.\!\?]$/) { $argdesc .= '.' }
            $argdesc .= "\n";
	}

	# This marshals the return value(s)
	my $return_args = "";
	my $return_marshal = "gimp_destroy_params (return_vals, nreturn_vals);";

	# return success/failure boolean if we don't have anything else
	if ($rettype eq 'void') {
	    $return_args .= "\n" . ' ' x 2 . "gboolean success = TRUE;";
	    $retdesc = "TRUE on success.";
	}

	# We only need to bother with this if we have to return a value
	if ($rettype ne 'void' || $retvoid) {
	    my $once = 0;
	    my $firstvar;
	    my @initnums;

	    foreach (@outargs) {
		my ($type) = &arg_parse($_->{type});
		my $arg = $arg_types{$type};
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
		elsif (exists $_->{retval}) {
		    $return_args .= "\n" . ' ' x 2;
		    $return_args .= &libtype($_, 1);

		    # The return value variable
		    $var = $_->{libname};
		    $var .= '_ID' if $arg->{id};
		    $return_args .= $var;

		    # Save the first var to "return" it
		    $firstvar = $var unless defined $firstvar;

		    if ($arg->{id}) {
			# Initialize all IDs to -1
			$return_args .= " = -1";
		    }
		    elsif ($_->{libdef}) {
			$return_args .= " = $_->{libdef}";
		    }
		    else {
			$return_args .= " = $arg->{init_value}";
		    }

		    $return_args .= ";";

		    if (exists $_->{array} && exists $_->{array}->{no_lib}) {
			$return_args .= "\n" . ' ' x 2 . "gint num_$var;";
		    }
		}
		elsif ($retvoid) {
		    push @initnums, $_ unless exists $arg->{struct};
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

	    if ($rettype eq 'void') {
		$return_marshal .= <<CODE;
success = return_vals[0].data.d_status == GIMP_PDB_SUCCESS;

  if (success)
CODE
	    }
	    else {
		$return_marshal .= <<CODE;
if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
CODE
	    }

	    $return_marshal .= ' ' x 4 . "{\n" if $#outargs;

	    my $argc = 1; my ($numpos, $numtype);
	    foreach (@outargs) {
		my ($type) = &arg_parse($_->{type});
                my $desc = exists $_->{desc} ? $_->{desc} : "";
		my $arg = $arg_types{$type};
		my $var;
	    
		my $ch = ""; my $cf = "";
		if ($type =~ /^string(array)?/) {
		    $ch = 'g_strdup (';
		    $cf = ')';
		}
		elsif ($type =~ /parasite/) {
		    $ch = 'gimp_parasite_copy (&';
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
			$argdesc .= " * \@$_->{libname}: $desc";
		    }
		}
		elsif (exists $_->{array}) {
		    my $datatype = $arg->{type};
		    chop $datatype;
		    $datatype =~ s/ *$//;

		    my $var = $_->{libname}; my $dh = ""; my $df = "";
		    unless (exists $_->{retval}) {
			$var = "*$var"; $dh = "(*"; $df = ")";
			if ($type eq 'stringarray') {
			    $arglist .= "$datatype**$_->{libname}, ";
			}
			else {
			    $arglist .= "$datatype **$_->{libname}, ";
			}
			$argdesc .= " * \@$_->{libname}: $desc";
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
      memcpy ($var,
              return_vals[$argc].data.d_$type,
              $numvar * sizeof ($datatype));
CP2
		    $out->{headers} = "#include <string.h>\n" unless ($ch || $cf);
                }
		else {
		    # The return value variable
		    $var = "";

		    unless (exists $_->{retval}) {
			$var .= '*';
			$arglist .= &libtype($_, 1);
			$arglist .= '*' unless exists $arg->{struct};
			$arglist .= "$_->{libname}";
			$arglist .= '_ID' if $arg->{id};
			$arglist .= ', ';

			$argdesc .= " * \@$_->{libname}";
			$argdesc .= '_ID' if $arg->{id};
			$argdesc .= ": $desc";
		    }

		    $var = exists $_->{retval} ? "" : '*';
		    $var .= $_->{libname};
		    $var .= '_ID' if $arg->{id};

		    $return_marshal .= ' ' x 2 if $#outargs;
		    $return_marshal .= <<CODE
    $var = ${ch}return_vals[$argc].data.d_$type${cf};
CODE
		}

                if ($argdesc) {
                    unless ($argdesc =~ /[\.\!\?]$/) { $argdesc .= '.' }
                    unless ($argdesc =~ /\n$/)       { $argdesc .= "\n" }
		}
		$argc++;
	    }

	    $return_marshal .= ' ' x 4 . "}\n" if $#outargs;

	    $return_marshal .= <<'CODE';

  gimp_destroy_params (return_vals, nreturn_vals);

CODE
	    unless ($retvoid) {
		$return_marshal .= ' ' x 2 . "return $firstvar;";
	    }
	    else {
		$return_marshal .= ' ' x 2 . "return success;";
	    }
	}
	else {
	    $return_marshal = <<CODE;
success = return_vals[0].data.d_status == GIMP_PDB_SUCCESS;

  $return_marshal

  return success;
CODE

	    chop $return_marshal;
	}

	if ($arglist) {
	    my @arglist = split(/, /, $arglist);
	    my $longest = 0; my $seen = 0;
	    foreach (@arglist) {
		/(const \w+) \S+/ || /(\w+) \S+/;
		my $len = length($1);
		my $num = scalar @{[ /\*/g ]};
		$seen = $num if $seen < $num;
		$longest = $len if $longest < $len;
	    }

	    $longest += $seen;

	    my $once = 0; $arglist = "";
	    foreach (@arglist) {
		my $space = rindex($_, ' ');
		my $len = $longest - $space + 1;
		$len -= scalar @{[ /\*/g ]};
		substr($_, $space, 1) = ' ' x $len if $space != -1 && $len > 1;
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
	
	$rettype = 'gboolean' if $rettype eq 'void';

	# Our function prototype for the headers
	(my $hrettype = $rettype) =~ s/ //g;

	my $proto = "$hrettype $wrapped$funcname ($arglist);\n";
	$proto =~ s/ +/ /g;

        push @{$out->{protos}}, $proto;

	my $clist = $arglist;
	my $padlen = length($wrapped) + length($funcname) + 2;
	my $padding = ' ' x $padlen;
	$clist =~ s/\t/$padding/eg;

        unless ($retdesc =~ /[\.\!\?]$/) { $retdesc .= '.' }

        if ($proc->{since}) {
	    $sincedesc = "\n *\n * Since: GIMP $proc->{since}";
	}

	if ($proc->{deprecated}) {
            if ($proc->{deprecated} eq 'NONE') {
		$procdesc = &desc_wrap("Deprecated: There is no replacement " .
                                       "for this procedure.");
	    }
	    else {
		my $underscores = $proc->{deprecated};
		$underscores =~ s/-/_/g;

		$procdesc = &desc_wrap("Deprecated: " .
				       "Use $underscores() instead.");
	    }
	}
	else {
	    $procdesc = &desc_wrap($proc->{blurb}) . "\n *\n" .
			&desc_wrap($proc->{help});
	}

	$out->{code} .= <<CODE;

/**
 * $wrapped$funcname:
$argdesc *
$procdesc
 *
 * Returns: $retdesc$sincedesc
 */
$rettype
$wrapped$funcname ($clist)
{
  GimpParam *return_vals;
  gint nreturn_vals;$return_args

  return_vals = gimp_run_procedure ("gimp-$proc->{canonical_name}",
                                    \&nreturn_vals,$argpass
                                    GIMP_PDB_END);

  $return_marshal
}
CODE
    }

    if ($out->{deprecated}) {
        push @{$out->{protos}}, "#endif /* GIMP_DISABLE_DEPRECATED */\n";
	$out->{deprecated} = 0;
    }

    my $lgpl_top = <<'LGPL';
/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
LGPL

    my $lgpl_bottom = <<'LGPL';
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/* NOTE: This file is auto-generated by pdbgen.pl */

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
	    my $arglist;

	    if (!/^#/) {
		my $len;

		$arglist = [ split(' ', $_, 3) ];

		if ($arglist->[1] =~ /^_/) {
		    $arglist->[0] = "G_GNUC_INTERNAL ".$arglist->[0];
		}

		for (0..1) {
		    $len = length($arglist->[$_]);
		    $longest[$_] = $len if $longest[$_] < $len;
		}

		foreach (split(/,/, $arglist->[2])) {
		    next unless /(const \w+) \S+/ || /(\w+) \S+/;
		    $len = length($1) + 1;
		    my $num = scalar @{[ /\*/g ]};
		    $seen = $num if $seen < $num;
		    $longest[2] = $len if $longest[2] < $len;
		}
	    }
	    else {
		$arglist = $_;
	    }

	    push @arglist, $arglist;
	}

	$longest[2] += $seen;

	@{$out->{protos}} = ();
	foreach (@arglist) {
	    my $arg;

	    if (ref) {
		my ($type, $func, $arglist) = @$_;

		my @args = split(/,/, $arglist); $arglist = "";

		foreach (@args) {
		    $space = rindex($_, ' ');
                    if ($space > 0 && substr($_, $space - 1, 1) eq ')') {
                        $space = rindex($_, ' ', $space - 1)
                    }
		    my $len = $longest[2] - $space + 1;

		    $len -= scalar @{[ /\*/g ]};
		    $len++ if /\t/;

		    if ($space != -1 && $len > 1) {
			substr($_, $space, 1) = ' ' x $len;
		    }

		    $arglist .= $_;
		    $arglist .= "," if !/;\n$/;
		}

		$arg = $type;
		$arg .= ' ' x ($longest[0] - length($type) + 1) . $func;
		$arg .= ' ' x ($longest[1] - length($func) + 1) . $arglist;
		$arg =~ s/\t/' ' x ($longest[0] + $longest[1] + 3)/eg;
	    }
	    else {
		$arg = $_;
	    }

	    push @{$out->{protos}}, $arg;
	}

	my $body;
	$body = $extra->{decls} if exists $extra->{decls};
	foreach (@{$out->{protos}}) { $body .= $_ }
        if ($out->{deprecated}) {
	    $body .= "#endif /* GIMP_DISABLE_DEPRECATED */\n";
	}
	chomp $body;

	open HFILE, "> $hfile" or die "Can't open $hfile: $!\n";
        print HFILE $lgpl_top;
        print HFILE " * $hname\n";
        print HFILE $lgpl_bottom;
 	my $guard = "__GIMP_\U$group\E_PDB_H__";
	print HFILE <<HEADER;
#ifndef $guard
#define $guard

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


$body


G_END_DECLS

#endif /* $guard */
HEADER
	close HFILE;
	&write_file($hfile);

	open CFILE, "> $cfile" or die "Can't open $cfile: $!\n";
        print CFILE $lgpl_top;
        print CFILE " * $cname\n";
        print CFILE $lgpl_bottom;
        print CFILE qq/#include "config.h"\n\n/;
	print CFILE $out->{headers}, "\n" if exists $out->{headers};
	print CFILE qq/#include "gimp.h"\n/;
	if ($out->{seen_deprecated}) {
	    print CFILE "#undef GIMP_DISABLE_DEPRECATED\n";
	    print CFILE "#undef __GIMP_\U$group\E_PDB_H__\n";
	    print CFILE qq/#include "${hname}"\n/;
	}
	$long_desc = &desc_wrap($main::grp{$group}->{doc_long_desc});
	print CFILE <<SECTION_DOCS;


/**
 * SECTION: $main::grp{$group}->{doc_title}
 * \@title: $main::grp{$group}->{doc_title}
 * \@short_description: $main::grp{$group}->{doc_short_desc}
 *
${long_desc}
 **/

SECTION_DOCS
	print CFILE "\n", $extra->{code} if exists $extra->{code};
	print CFILE $out->{code};
	close CFILE;
	&write_file($cfile);
    }

    if (! $ENV{PDBGEN_GROUPS}) {
        my $gimp_pdb = "$destdir/gimp_pdb.h$FILE_EXT";
	open PFILE, "> $gimp_pdb" or die "Can't open $gimp_pdb: $!\n";
        print PFILE $lgpl_top;
        print PFILE " * gimp_pdb.h\n";
        print PFILE $lgpl_bottom;
	my $guard = "__GIMP_PDB_H__";
	print PFILE <<HEADER;
#ifndef $guard
#define $guard

HEADER
	my @groups;
	foreach $group (keys %out) {
	    my $hname = "gimp${group}pdb.h";
	    $hname =~ s/_//g; $hname =~ s/pdb\./_pdb./;
	    push @groups, $hname;
	}
	foreach $group (sort @groups) {
	    print PFILE "#include <libgimp/$group>\n";
	}
	print PFILE <<HEADER;

#endif /* $guard */
HEADER
	close PFILE;
	&write_file($gimp_pdb);
    }
}
