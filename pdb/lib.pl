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
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

package Gimp::CodeGen::lib;

# Generates all the libgimp C wrappers (used by plugins)
$destdir  = "$main::destdir/libgimp";
$builddir = "$main::builddir/libgimp";

*arg_types = \%Gimp::CodeGen::pdb::arg_types;
*arg_parse = \&Gimp::CodeGen::pdb::arg_parse;

*enums = \%Gimp::CodeGen::enums::enums;

*write_file = \&Gimp::CodeGen::util::write_file;
*FILE_EXT   = \$Gimp::CodeGen::util::FILE_EXT;

use Text::Wrap qw(wrap);

sub desc_wrap {
    my ($str) = @_;
    my $leading = ' * ';
    my $wrapped;

    $str =~ s/\s+$//; # trim trailing whitespace
    $str =~ s/&/&amp\;/g;
    $str =~ s/\</&lt\;/g;
    $str =~ s/\>/&gt\;/g;
    $Text::Wrap::columns = 72;
    $wrapped = wrap($leading, $leading, $str);
    $wrapped =~ s/[ \t]+\n/\n/g;
    return $wrapped;
}

sub generate_fun {
    my ($proc, $out) = @_;
    my @inargs = @{$proc->{inargs}} if (defined $proc->{inargs});
    my @outargs = @{$proc->{outargs}} if (defined $proc->{outargs});

    sub libtype {
	my $arg = shift;
	my $outarg = shift;
	my ($type, $name) = &arg_parse($arg->{type});
	my $argtype = $arg_types{$type};
	my $rettype = '';

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

    my $funcname = "gimp_$name";
    my $wrapped = "";
    my $new_funcname = $funcname;
    my %usednames;
    my $retdesc = " * Returns:";
    my $func_annotations = "";

    if ($proc->{lib_private}) {
        $wrapped = '_';
    }

    $skip_gi = '';
    if ($proc->{skip_gi}) {
        $skip_gi = ' (skip)';
    }

    if ($proc->{deprecated}) {
        if ($proc->{deprecated} eq 'NONE') {
            push @{$out->{protos}}, "GIMP_DEPRECATED\n";
        }
        else {
            my $underscores = $proc->{deprecated};
            $underscores =~ s/-/_/g;

            push @{$out->{protos}}, "GIMP_DEPRECATED_FOR($underscores)\n";
        }
    }

    # Find the return argument (defaults to the first arg if not
    # explicitly set
    my $retarg  = undef;
    $retvoid = 0;
    foreach (@outargs) {
        $retarg = $_, last if exists $_->{retval};
    }

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
        my ($type) = &arg_parse($retarg->{type});
        my $argtype = $arg_types{$type};
        my $annotate = "";
        $rettype = &libtype($retarg, 1);
        chop $rettype unless $rettype =~ /\*$/;

        $retarg->{retval} = 1;

        if (exists $argtype->{array}) {
            $annotate = " (array length=$retarg->{array}->{name})";
        }
        if (exists $retarg->{none_ok}) {
            $annotate .= " (nullable)";
        }

        if (exists $argtype->{out_annotate}) {
            $annotate .= " $argtype->{out_annotate}";
        }

        if ($annotate eq "") {
            $retdesc .= " $retarg->{desc}";
        }
        else {
            if (exists $retarg->{desc}) {
                if ((length ($annotate) +
                     length ($retarg->{desc})) > 65) {
                    $retdesc .= $annotate . ":\n *          " . $retarg->{desc};
                }
                else {
                    $retdesc .= $annotate . ": " . $retarg->{desc};
                }
            }
        }

        unless ($retdesc =~ /[\.\!\?]$/) { $retdesc .= '.' }

        if ($retarg->{type} eq 'string') {
            $retdesc .= "\n *          The returned value must be freed with g_free().";
        }
        elsif ($retarg->{type} eq 'strv') {
            $retdesc .= "\n *          The returned value must be freed with g_strfreev().";
        }
        elsif ($retarg->{type} eq 'param') {
            $retdesc .= "\n *          The returned value must be freed with g_param_spec_unref().";
        }
        elsif (exists $argtype->{array}) {
            $retdesc .= "\n *          The returned value must be freed with g_free().";
        }
    }
    else {
        # No return values
        $rettype = 'void';
    }

    # The parameters to the function
    my $arglist = "";
    my $argdesc = "";
    my $sincedesc = "";
    my $value_array = "";
    my $arg_array = "";
    my $argc = 0;
    foreach (@inargs) {
        my ($type, @typeinfo) = &arg_parse($_->{type});
        my $arg = $arg_types{$type};
        my $var = $_->{name};
        my $desc = exists $_->{desc} ? $_->{desc} : "";
        my $var_len;
        my $value;

        # This gets passed to gimp_value_array_new_with_types()
        if ($type eq 'enum') {
            $enum_type = $typeinfo[0];
            $enum_type =~ s/([a-z])([A-Z])/$1_$2/g;
            $enum_type =~ s/([A-Z]+)([A-Z])/$1_$2/g;
            $enum_type =~ tr/[a-z]/[A-Z]/;
            $enum_type =~ s/^GIMP/GIMP_TYPE/;
            $enum_type =~ s/^GEGL/GEGL_TYPE/;

            $value_array .= "$enum_type, ";
        }
        else {
            $value_array .= "$arg->{gtype}, ";
        }

        if (exists $_->{array}) {
            $value_array .= "NULL";
        }
        else {
            $value_array .= "$var";
        }

        $value_array .= ",\n" . " " x 42;

        if (exists $_->{array}) {
            my $arrayarg = $_->{array};

            $value = "gimp_value_array_index (args, $argc)";

            if (exists $arrayarg->{name}) {
                $var_len = $arrayarg->{name};
            }
            else {
                $var_len = 'num_' . $_->{name};
            }

            # This is the list of g_value_set_foo_array
            $arg_array .= eval qq/"  $arg->{set_value_func};\n"/;
        }

        $usednames{$_->{name}}++;

        $arglist .= &libtype($_, 0);
        $arglist .= $_->{name};
        $arglist .= ', ';

        $argdesc .= " * \@$_->{name}";
        $argdesc .= ":";

        if (exists $arg->{array}) {
            $argdesc .= " (array length=$inargs[$argc - 1]->{name})";
        }

        if (exists $arg->{in_annotate}) {
            $argdesc .= " $arg->{in_annotate}";
        }
        if (exists $_->{none_ok}) {
            $argdesc .= " (nullable)";
        }

        if (exists $arg->{array} || exists $_->{none_ok} || exists $arg->{in_annotate}) {
            $argdesc .= ":";
        }

        $argdesc .= " $desc";

        unless ($argdesc =~ /[\.\!\?]$/) { $argdesc .= '.' }
        $argdesc .= "\n";

        $argc++;
    }

    # This marshals the return value(s)
    my $return_args = "";
    my $return_marshal = "gimp_value_array_unref (return_vals);";

    # return success/failure boolean if we don't have anything else
    if ($rettype eq 'void') {
        $return_args .= "\n" . ' ' x 2 . "gboolean success = TRUE;";
        $retdesc .= " TRUE on success.";
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
                $return_args .= $var;

                # Save the first var to "return" it
                $firstvar = $var unless defined $firstvar;

                if ($_->{libdef}) {
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
success = GIMP_VALUES_GET_ENUM (return_vals, 0) == GIMP_PDB_SUCCESS;

  if (success)
CODE
        }
        else {
            $return_marshal .= <<CODE;
if (GIMP_VALUES_GET_ENUM (return_vals, 0) == GIMP_PDB_SUCCESS)
CODE
        }

        $return_marshal .= ' ' x 4 . "{\n" if $#outargs;

        my $argc = 1; my ($numpos, $numtype);
        foreach (@outargs) {
            my ($type) = &arg_parse($_->{type});
            my $desc = exists $_->{desc} ? $_->{desc} : "";
            my $arg = $arg_types{$type};
            my $var;

            # The return value variable
            $var = "";

            unless (exists $_->{retval}) {
                $var .= '*';
                $arglist .= &libtype($_, 1);
                $arglist .= '*' unless exists $arg->{struct};
                $arglist .= "$_->{libname}";
                $arglist .= ', ';

                $argdesc .= " * \@$_->{libname}";

                if ($arg->{name} eq 'COLOR') {
                    $argdesc .= ": (out caller-allocates)";
                }
                else {
                    $argdesc .= ": (out)";
                }

                if (exists $arg->{array}) {
                    $argdesc .= " (array length=$outargs[$argc - 2]->{name})";
                }

                if (exists $arg->{out_annotate}) {
                    $argdesc .= " $arg->{out_annotate}";
                }

                $argdesc .= ": $desc";
            }

            $var = exists $_->{retval} ? "" : '*';
            $var .= $_->{libname};

            $value = "return_vals, $argc";

            $return_marshal .= ' ' x 2 if $#outargs;
            $return_marshal .= eval qq/"    $arg->{dup_value_func};\n"/;

            if ($argdesc) {
                unless ($argdesc =~ /[\.\!\?]$/) { $argdesc .= '.' }
                unless ($argdesc =~ /\n$/)       { $argdesc .= "\n" }
            }

            $argc++;
        }

        $return_marshal .= ' ' x 4 . "}\n" if $#outargs;

        $return_marshal .= <<'CODE';

  gimp_value_array_unref (return_vals);

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
success = GIMP_VALUES_GET_ENUM (return_vals, 0) == GIMP_PDB_SUCCESS;

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

    if ($proc->{since}) {
        $sincedesc = "\n *\n * Since: $proc->{since}";
    }

    my $procdesc = '';

    if ($proc->{deprecated}) {
        if ($proc->{deprecated} eq 'NONE') {
            if ($proc->{blurb}) {
                $procdesc = &desc_wrap($proc->{blurb}) . "\n *\n";
            }
            if ($proc->{help}) {
                $procdesc .= &desc_wrap($proc->{help}) . "\n *\n";
            }
            $procdesc .= &desc_wrap("Deprecated: There is no replacement " .
                                    "for this procedure.");
        }
        else {
            my $underscores = $proc->{deprecated};
            $underscores =~ s/-/_/g;

            if ($proc->{blurb}) {
                $procdesc = &desc_wrap($proc->{blurb}) . "\n *\n";
            }
            if ($proc->{help}) {
                $procdesc .= &desc_wrap($proc->{help}) . "\n *\n";
            }
            $procdesc .= &desc_wrap("Deprecated: " .
                                    "Use $underscores() instead.");
        }
    }
    else {
        $procdesc = &desc_wrap($proc->{blurb}) . "\n *\n" .
                    &desc_wrap($proc->{help});
    }
    return <<CODE;

/**
 * $wrapped$funcname:$func_annotations$skip_gi
$argdesc *
$procdesc
 *
$retdesc$sincedesc
 **/
$rettype
$wrapped$funcname ($clist)
{
  GimpValueArray *args;
  GimpValueArray *return_vals;$return_args

  args = gimp_value_array_new_from_types (NULL,
                                          ${value_array}G_TYPE_NONE);
$arg_array
  return_vals = _gimp_pdb_run_procedure_array (gimp_get_pdb (),
                                               "gimp-$proc->{canonical_name}",
                                               args);
  gimp_value_array_unref (args);

  $return_marshal
}
CODE
}

sub generate_hbody {
    my ($out, $extra) = @_;

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

        if (!/^GIMP_DEPRECATED/) {
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

    my $body = '';
    $body = $extra->{decls} if exists $extra->{decls};
    foreach (@{$out->{protos}}) { $body .= $_ }
    if ($out->{deprecated}) {
        $body .= "#endif /* GIMP_DISABLE_DEPRECATED */\n";
    }
    chomp $body;

    return $body;
}


sub generate {
    my @procs = @{(shift)};
    my %out;

    foreach $name (@procs) {
	my $proc = $main::pdb{$name};
	my $out = \%{$out{$proc->{group}}};

	my @inargs = @{$proc->{inargs}} if (defined $proc->{inargs});
	my @outargs = @{$proc->{outargs}} if (defined $proc->{outargs});

        $out->{code} .= generate_fun($proc, $out);
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
 * <https://www.gnu.org/licenses/>.
 */

/* NOTE: This file is auto-generated by pdbgen.pl */

LGPL

    # We generate two files, a _pdb.h file with prototypes for all
    # the functions we make, and a _pdb.c file for the actual implementation
    while (my($group, $out) = each %out) {
        my $hname = "${group}pdb.h"; 
        my $cname = "${group}pdb.c";
        if ($group ne 'gimp') {
	    $hname = "gimp${hname}"; 
	    $cname = "gimp${cname}";
        }
        $hname =~ s/_//g; $hname =~ s/pdb\./_pdb./;
        $cname =~ s/_//g; $cname =~ s/pdb\./_pdb./;
	my $hfile = "$builddir/$hname$FILE_EXT";
	my $cfile = "$builddir/$cname$FILE_EXT";
        my $body;

	my $extra = {};
	if (exists $main::grp{$group}->{extra}->{lib}) {
	    $extra = $main::grp{$group}->{extra}->{lib}
	}

        $body = generate_hbody($out, $extra, "protos");

	open HFILE, "> $hfile" or die "Can't open $hfile: $!\n";
        print HFILE $lgpl_top;
        print HFILE " * $hname\n";
        print HFILE $lgpl_bottom;
 	my $guard = "__GIMP_\U$group\E_PDB_H__";
	print HFILE <<HEADER;
#if !defined (__GIMP_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimp.h> can be included directly."
#endif

#ifndef $guard
#define $guard

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


$body


G_END_DECLS

#endif /* $guard */
HEADER
	close HFILE;
	&write_file($hfile, $destdir);

	open CFILE, "> $cfile" or die "Can't open $cfile: $!\n";
        print CFILE $lgpl_top;
        print CFILE " * $cname\n";
        print CFILE $lgpl_bottom;
        print CFILE qq/#include "config.h"\n\n/;
	print CFILE qq/#include "stamp-pdbgen.h"\n\n/;
	print CFILE $out->{headers}, "\n" if exists $out->{headers};
	print CFILE qq/#include "gimp.h"\n/;

	if (exists $main::grp{$group}->{lib_private}) {
	    print CFILE qq/#include "$hname"\n/;
	}

	if (exists $main::grp{$group}->{doc_title}) {
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
}

	print CFILE "\n", $extra->{code} if exists $extra->{code};
	print CFILE $out->{code};
	close CFILE;
	&write_file($cfile, $destdir);
    }

    if (! $ENV{PDBGEN_GROUPS}) {
        my $gimp_pdb_headers = "$builddir/gimp_pdb_headers.h$FILE_EXT";
	open PFILE, "> $gimp_pdb_headers" or die "Can't open $gimp_pdb_headers: $!\n";
        print PFILE $lgpl_top;
        print PFILE " * gimp_pdb_headers.h\n";
        print PFILE $lgpl_bottom;
	my $guard = "__GIMP_PDB_HEADERS_H__";
	print PFILE <<HEADER;
#if !defined (__GIMP_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimp.h> can be included directly."
#endif

#ifndef $guard
#define $guard

HEADER
	my @groups;
	foreach $group (keys %out) {
	    my $hname = "${group}pdb.h";
	    if ($group ne 'gimp') {
		$hname = "gimp${hname}";
	    }
	    $hname =~ s/_//g; $hname =~ s/pdb\./_pdb./;
	    if (! exists $main::grp{$group}->{lib_private}) {
		push @groups, $hname;
	    }
	}
	foreach $group (sort @groups) {
	    print PFILE "#include <libgimp/$group>\n";
	}
	print PFILE <<HEADER;

#endif /* $guard */
HEADER
	close PFILE;
	&write_file($gimp_pdb_headers, $destdir);
    }
}
