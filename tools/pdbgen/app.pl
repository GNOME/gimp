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

package Gimp::CodeGen::app;

$destdir = "$main::destdir/app";

*arg_types = \%Gimp::CodeGen::pdb::arg_types;
*arg_parse = \&Gimp::CodeGen::pdb::arg_parse;
*arg_ptype = \&Gimp::CodeGen::pdb::arg_ptype;
*arg_vname = \&Gimp::CodeGen::pdb::arg_vname;

*write_file = \&Gimp::CodeGen::util::write_file;

sub declare_args {
    my $proc = shift;
    my $out = shift;
    my $result = "";
    foreach (@_) {
	my @args = @{$proc->{$_}} if exists $proc->{$_};
	foreach (@args) {
	    my $arg = $arg_types{(&arg_parse($_->{type}))[0]};
	    if (not exists $_->{no_declare}) {
		$result .= ' ' x 2;
		$result .= $arg->{type} . &arg_vname($_) . ";\n";
		if (exists $arg->{id_headers}) {
		    foreach (@{$arg->{id_headers}}) {
			$out->{headers}->{$_}++;
		    }
		}
	    }
	}
    }
    $result;
} 

sub make_args {
    my $proc = shift;
    my $result = "";
    my $once;
    foreach (@_) {
	my @args = @{$proc->{$_}} if exists $proc->{$_};
	if (scalar @args) {
	    $result .= "\nstatic ProcArg $proc->{name}_${_}[] =";
	    $result .= "\n{\n";
	    foreach my $arg (@{$proc->{$_}}) {
		my ($type) = &arg_parse($arg->{type});
		$result .= ' ' x 2 . "{\n";
		$result .= ' ' x 4;
		$result .= 'PDB_' . $arg_types{$type}->{name} . ",\n"; 
		$result .= ' ' x 4;
		$result .= qq/"$arg->{name}",\n/; 
		$result .= ' ' x 4;
		$result .= qq/"$arg->{desc}"\n/; 
		$result .= ' ' x 2 . "},\n";
	    }
	    $result =~ s/,\n$/\n/;
	    $result .= "};\n";
	}
    }
    $result;
}

sub marshal_inargs {
    my $proc = shift;
    my $result = "";
    my %decls;
    my $argc = 0;
    my @inargs = @{$proc->{inargs}} if exists $proc->{inargs};
    foreach (@inargs) {
	my($pdbtype, @typeinfo) = &arg_parse($_->{type});
	my $arg = $arg_types{$pdbtype};
	my $type = &arg_ptype($arg);
	my $var = &arg_vname($_);
	$result .= ' ' x 2;
	$result .= "if (success)\n" . ' ' x 4 if $success;
	if (exists $arg->{id_func}) {
	    $decls{$type}++;
	    $result .= "{\n" . ' ' x 6 if $success;
	    $result .= "${type}_value = args[$argc].value.pdb_$type;\n";
	    $result .= ' ' x 4 if $success;
	    $result .= ' ' x 2;
	    $result .= "if (($var = ";
	    $result .= "$arg->{id_func} (${type}_value)) == NULL)\n";
	    $result .= ' ' x 4 unless $success;
	    $result .= "\t" if $success;
	    $result .= "success = FALSE;\n";
	    $result .= ' ' x 4 . "}\n" if $success;
	    $success = 1;
	}
	else {
	    if ($pdbtype eq 'enum') {
		# FIXME: implement this
	    }
	    elsif ($pdbtype eq 'boolean') {
		$result .= "$var = ";
		$result .= "(args[$argc].value.pdb_$type) ? TRUE : FALSE;\n";
	    }
	    elsif (defined $typeinfo[0] || defined $typeinfo[2]) {
		my $tests = 0;
		$result .= "success = (";
		if (defined $typeinfo[0]) {
		    $result .= "$var $typeinfo[1] $typeinfo[0]";
		    $tests++;
		}
		if (defined $typeinfo[2]) {
		    $result .= '|| ' if $tests;
		    $result .= "$var $typeinfo[2] $typeinfo[3]";
		}
		$result .= ");\n";
	    }
	    else {
		my $cast = "";
		$cast = " ($arg->{type})" if $type eq "pointer";
	        $cast = " ($arg->{type})" if $arg->{type} =~ /int(16|8)$/;
		$result .= "$var =$cast args[$argc].value.pdb_$type;\n";
	    }
	}
	$argc++; $result .= "\n";
    }
    chomp $result if !$success && $argc == 1;
    my $decls;
    foreach (keys %decls) { $decls .= ' ' x 2 . "$_ ${_}_value;\n" }
    $result = $decls . "\n" . $result if $decls;
    $result and $result = "\n" . $result unless $decls;
    $result;
}

sub marshal_outargs {
    my $proc = shift;
    my $result = <<CODE;
  return_args = procedural_db_return_args (\&$proc->{name}_proc, success);
CODE
    my $argc = 0;
    my @outargs = @{$proc->{outargs}} if exists $proc->{outargs};
    if (scalar @outargs) {
	my $outargs = "";
	foreach (@{$proc->{outargs}}) {
	    my ($pdbtype) = &arg_parse($_->{type});
	    my $arg = $arg_types{$pdbtype};
	    my $type = &arg_ptype($arg);
	    my $var = &arg_vname($_);
	    $argc++; $outargs .= ' ' x 2;
            if (exists $arg->{id_ret_func}) {
		$outargs .= "return_args[$argc].value.pdb_$type = ";
		$outargs .= eval qq/"$arg->{id_ret_func}"/;
		$outargs .= ";\n";
	    }
	    else {
		$outargs .= "return_args[$argc].value.pdb_$type = $var;\n";
	    }
	}
	$outargs =~ s/^/' ' x 2/meg if $success;
	$outargs =~ s/^/' ' x 2/meg if $success && $argc > 1;
	$result .= "\n" if $success || $argc > 1;
	$result .= ' ' x 2 . "if (success)\n" if $success;
	$result .= ' ' x 4 . "{\n" if $success && $argc > 1;
	$result .= $outargs;
	$result .= ' ' x 4 . "}\n" if $success && $argc > 1;
        $result .= "\n" . ' ' x 2 . "return return_args;\n";
    }
    else {
	$result =~ s/_args =//;
    }
    $result =~ s/, success\);$/, TRUE);/m unless $success;
    $result;
}

sub generate {
    my @procs = @{(shift)};
    my %out;
    my $total = 0.0;

    foreach my $name (@procs) {
	my $proc = $main::pdb{$name};
	my $out = \%{$out{$proc->{group}}};

	my @inargs = @{$proc->{inargs}} if exists $proc->{inargs};
	my @outargs = @{$proc->{outargs}} if exists $proc->{outargs};
	
	local $success = 0;

	$out->{pcount}++; $total++;

	$out->{procs} .= "static ProcRecord ${name}_proc;\n";

	$out->{register} .= <<CODE;
  procedural_db_register (\&${name}_proc);
CODE

	if (exists $proc->{invoke}->{headers}) {
	    foreach my $header (@{$proc->{invoke}->{headers}}) {
		$out->{headers}->{$header}++;
	    }
	}
	$out->{headers}->{q/"procedural_db.h"/}++;

	$out->{code} .= "\nstatic Argument *\n";
	$out->{code} .= "${name}_invoker (Argument *args)\n{\n";

	my $invoker = "";
	$invoker .= ' ' x 2 . "Argument *return_args;\n" if scalar @outargs;
	$invoker .= &declare_args($proc, $out, qw(inargs outargs));

	if (exists $proc->{invoke}->{vars}) {
	    foreach (@{$proc->{invoke}->{vars}}) {
		$invoker .= ' ' x 2 . $_ . ";\n";
	    }
	}

	$invoker.= &marshal_inargs($proc);

	$invoker .= "\n" if $invoker && $invoker !~ /\n\n/s;

	my $code = $proc->{invoke}->{code};
	chomp $code;
	$code =~ s/\t/' ' x 8/eg;
	if ($code =~ /^\s*\{\s*\n.*\n\s*\}\s*$/s && !$success) {
	    $code =~ s/^\s*\{\s*\n//s;
	    $code =~ s/\n\s*}\s*$//s;
	}
	else {
	    $code =~ s/^/' ' x 2/meg;
	    $code =~ s/^/' ' x 2/meg if $success;
	}
	$code =~ s/^ {8}/\t/mg;

	$code = ' ' x 2 . "if (success)\n" . $code if $success;
	$success = ($code =~ /success =/) unless $success;

	$out->{code} .= ' ' x 2 . "int success = TRUE;\n" if $success;
	$out->{code} .= $invoker . $code . "\n";
	$out->{code} .= "\n" if $code =~ /\n/s || $invoker;
	$out->{code} .= &marshal_outargs($proc) . "}\n";

	$out->{code} .= &make_args($proc, qw(inargs outargs));

	$out->{code} .= <<CODE;

static ProcRecord ${name}_proc =
{
  "gimp_$name",
  "$proc->{blurb}",
  "$proc->{help}",
  "$proc->{author}",
  "$proc->{copyright}",
  "$proc->{date}",
  PDB_INTERNAL,
  @{[scalar @inargs or '0']},
  @{[scalar @inargs ? "${name}_inargs" : 'NULL']},
  @{[scalar @outargs or '0']},
  @{[scalar @outargs ? "${name}_outargs" : 'NULL']},
  { { ${name}_invoker } }
};
CODE
    }

    my $gpl = <<'GPL';
/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

GPL

    my $internal = "$destdir/internal_procs.h.tmp.$$";
    open INTERNAL, "> $internal" or die "Can't open $cmdfile: $!\n";
    print INTERNAL $gpl;
    my $guard = "__INTERNAL_PROCS_H__";
    print INTERNAL <<HEADER;
#ifndef $guard
#define $guard

void internal_procs_init (void);

#endif /* $guard */
HEADER
    close INTERNAL;
    &write_file($internal);

    my $group_procs = ""; my $longest = 0;
    my $once = 0; my $pcount = 0.0;
    foreach $group (@main::groups) {
	my $out = $out{$group};

	my $cfile = "$destdir/${group}_cmds.c.tmp.$$";
	open CFILE, "> $cfile" or die "Can't open $cmdfile: $!\n";
	print CFILE $gpl;
	foreach my $header (sort keys %{$out->{headers}}) {
	    print CFILE "#include $header\n";
	}
	print CFILE "\n";
	if (exists $main::grp{$group}->{code}) {
	    print CFILE "$main::grp{$group}->{code}\n";
	}
	print CFILE $out->{procs};
	print CFILE "\nvoid\nregister_${group}_procs (void)\n";
	print CFILE "{\n$out->{register}}\n";
	print CFILE $out->{code};
	close CFILE;
	&write_file($cfile);

	my $decl = "register_${group}_procs";
	push @group_decls, $decl;
	$longest = length $decl if $longest < length $decl;

	$group_procs .= ' ' x 2 . "app_init_update_status (";
	$group_procs .= q/"Internal Procedures"/ unless $once;
	$group_procs .= 'NULL' if $once++;
	$group_procs .= qq/, "$main::grp{$group}->{desc}", /;
       ($group_procs .= sprintf "%.3f", $pcount / $total) =~ s/\.?0*$//s;
	$group_procs .= ($group_procs !~ /\.\d+$/s ? ".0" : "") . ");\n";
	$group_procs .=  ' ' x 2 . "register_${group}_procs ();\n\n";
	$pcount += $out->{pcount};
    }

    $internal = "$destdir/internal_procs.c.tmp.$$";
    open INTERNAL, "> $internal" or die "Can't open $cmdfile: $!\n";
    print INTERNAL $gpl;
    print INTERNAL qq/#include "app_procs.h"\n\n/;
    print INTERNAL "/* Forward declarations for registering PDB procs */\n\n";
    foreach (@group_decls) {
	print INTERNAL "void $_" . ' ' x ($longest - length $_) . " (void);\n";
    }
    chop $group_procs;
    print INTERNAL "\n/* $total total procedures registered total */\n\n";
    print INTERNAL "void\ninternal_procs_init (void)\n{\n$group_procs}\n";
    close INTERNAL;
    &write_file($internal);
}

1;
