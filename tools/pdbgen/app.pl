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

package Gimp::CodeGen::app;

$destdir = "$main::destdir/app";

*arg_types = \%Gimp::CodeGen::pdb::arg_types;
*arg_parse = \&Gimp::CodeGen::pdb::arg_parse;
*arg_ptype = \&Gimp::CodeGen::pdb::arg_ptype;
*arg_vname = \&Gimp::CodeGen::pdb::arg_vname;

*enums = \%Gimp::CodeGen::enums::enums;

*write_file = \&Gimp::CodeGen::util::write_file;
*FILE_EXT   = \$Gimp::CodeGen::util::FILE_EXT;

use Text::Wrap qw(wrap);

sub quotewrap {
    my ($str, $indent) = @_;
    my $leading = ' ' x $indent . '"';
    $Text::Wrap::columns = 1000;
    $str = wrap($leading, $leading, $str);
    $str =~ s/^\s*//s;
    $str =~ s/(.)$/$1"/mg;
    $str;
}

sub format_code_frag {
    my ($code, $indent) = @_;

    chomp $code;
    $code =~ s/\t/' ' x 8/eg;

    if (!$indent && $code =~ /^\s*{\s*\n.*\n\s*}\s*$/s) {
	$code =~ s/^\s*{\s*\n//s;
	$code =~ s/\n\s*}\s*$//s;
    }
    else {
	$code =~ s/^/' ' x ($indent ? 4 : 2)/meg;
    }
    $code =~ s/^ {8}/\t/mg;
    $code .= "\n";

    $code;
}

sub declare_args {
    my $proc = shift;
    my $out = shift;

    local $result = "";

    foreach (@_) {
	my @args = @{$proc->{$_}} if exists $proc->{$_};

	foreach (@args) {
	    my $arg = $arg_types{(&arg_parse($_->{type}))[0]};

	    if ($arg->{array} && !exists $_->{array}) {
		warn "Array without number of elements param in $proc->{name}";
	    }

	    unless (exists $_->{no_declare}) {
		$result .= ' ' x 2 . $arg->{type} . &arg_vname($_);
		if (!exists $_->{no_init} && exists $_->{init}) {
		    $result .= $arg->{type} =~ /\*$/ ? ' = NULL' : '0'
		}
		$result .= ";\n";

		if (exists $arg->{headers}) {
		    foreach (@{$arg->{headers}}) {
			$out->{headers}->{$_}++;
		    }
		}
	    }
	}
    }

    $result;
} 

sub make_arg_recs {
    my $proc = shift;

    my $result = "";
    my $once;

    foreach (@_) {
	my @args = @{$proc->{$_}} if exists $proc->{$_};

	if (scalar @args) {
	    $result .= "\nstatic ProcArg $proc->{name}_${_}\[] =\n{\n";

	    foreach $arg (@{$proc->{$_}}) {
		my ($type, $name, @remove) = &arg_parse($arg->{type});
		my $desc = $arg->{desc};
		my $info = $arg->{type};

		for ($type) {
		    /array/     && do { 				 last };
		    /boolean/   && do { $info = 'TRUE or FALSE';	 last };
		    /int|float/ && do { $info =~ s/$type/$arg->{name}/e; last };
		    /enum/      && do { $info = $enums{$name}->{info};
					foreach (@remove) {
					    $info =~ s/$_ \(.*?\)(, )?//
					}				 last };
		}

		$desc =~ s/%%desc%%/$info/eg;

		$result .= <<CODE;
  {
    PDB_$arg_types{$type}->{name},
    "$arg->{name}",
    @{[ &quotewrap($desc, 4) ]}
  },
CODE
	    }

	    $result =~ s/,\n$/\n/s;
	    $result .= "};\n";
	}
    }

    $result;
}

sub marshal_inargs {
    my ($proc, $argc) = @_;

    my $result = "";
    my %decls;

    my @inargs = @{$proc->{inargs}} if exists $proc->{inargs};

    foreach (@inargs) {
	my($pdbtype, @typeinfo) = &arg_parse($_->{type});
	my $arg = $arg_types{$pdbtype};
	my $type = &arg_ptype($arg);
	my $var = &arg_vname($_);
	
	if (exists $arg->{id_func}) {
	    $result .= <<CODE;
  $var = $arg->{id_func} (args[$argc].value.pdb_$type);
CODE

	    if (!exists $_->{no_success}) {
		$result .= ' ' x 2 . "if ($var ";
		$result .= exists $_->{on_success} ? '!=' : '==';
		$result .= " NULL)\n";

		if (exists $_->{on_success}) {
		    $result .= &format_code_frag($_->{on_success}, 1);
		    $result .= ' ' x 2 . "else\n";
		}

		$result .= ' ' x 4 . "success = FALSE;\n";

		if (exists $_->{on_fail}) {
		    $result .= &format_code_frag($_->{on_fail}, 1);
		}

		$success = 1;
	    }
	}
	else {
	    my $code = ' ' x 2 . "$var =";

	    my $cast = "";
	    if ($type eq 'pointer' || $arg->{type} =~ /int(16|8)$/) {
		$cast = " ($arg->{type})";
	    }
	    $code .= "$cast args[$argc].value.pdb_$type";
	    $code .= ' ? TRUE : FALSE' if $pdbtype eq 'boolean';
	    $code .= ";\n";

	    if (!exists $_->{no_success}) {
		if ($pdbtype eq 'string') {
		    $code .= ' ' x 2 . "success = $var != NULL;\n";
		}
		elsif ($pdbtype eq 'enum' && !$enums{$typeinfo[0]}->{contig}) {
		    my %vals; my $symbols = $enums{pop @typeinfo}->{symbols};
		    @vals{@$symbols}++; delete @vals{@typeinfo};
		
		    $code .= <<CODE;
  switch ($var)
    {
CODE

		    foreach (@$symbols) {
			$code .= ' ' x 4 . "case $_:\n" if exists $vals{$_};
		    }

		    $code .= <<CODE;
      break;

    default:
      success = FALSE;
      break;
    }
CODE
		}
		elsif (defined $typeinfo[0] || defined $typeinfo[2]) {
		    my $tests = 0; my $extra = "";

		    if ($pdbtype eq 'enum') {
			my $symbols = $enums{pop @typeinfo}->{symbols};

			foreach (@typeinfo) { $extra .= " && $var != $_" }

			$typeinfo[0] = $symbols->[0];
			$typeinfo[1] = '>=';
			$typeinfo[2] = $symbols->[$#$symbols];
			$typeinfo[3] = '<=';
		    }

		    $code .= ' ' x 2 . "success = ";

		    if (defined $typeinfo[0]) {
			$code .= "$var $typeinfo[1] $typeinfo[0]";
			$tests++;
		    }

		    if (defined $typeinfo[2]) {
			$code .= ' && ' if $tests;
			$code .= "$var $typeinfo[3] $typeinfo[2]";
		    }

		    $code .= "$extra;\n";
		}

		if ($code =~ /success/) {
		    my $tests = 0;

		    if (exists $_->{on_success}) {
			$code .= ' ' x 2 . "if (success)\n";
			$code .= &format_code_frag($_->{on_success}, 1);
			$tests++;
		    }

		    if (exists $_->{on_fail}) {
			$code .= ' ' x 2;
			$code .= $tests ? "else\n" : "if (success)\n"; 
			$code .= &format_code_frag($_->{on_fail}, 1);
		    }

		    if ($success) {
			$code =~ s/^/' ' x 4/meg;
			$code =~ s/^ {8}/\t/mg;

			$code .= ' '  x 4 . "}\n";
			$result .= ' ' x 2 . "if (success)\n" . ' ' x 4 . "{\n";
		    }
		    else {
			$success_init = 0;
		    }

		    $success = 1;
		}
	    }

	    $result .= $code;
	}

	$argc++; $result .= "\n";
    }

    $result = "\n" . $result if $result;
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
		$var = eval qq/"$arg->{id_ret_func}"/;
	    }

	    $outargs .= "return_args[$argc].value.pdb_$type = $var;\n";
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

    foreach $name (@procs) {
	my $proc = $main::pdb{$name};
	my $out = \%{$out{$proc->{group}}};

	my @inargs = @{$proc->{inargs}} if exists $proc->{inargs};
	my @outargs = @{$proc->{outargs}} if exists $proc->{outargs};
	
	local $success = 0;
	local $success_init = 1;

	$out->{pcount}++; $total++;

	$out->{procs} .= "static ProcRecord ${name}_proc;\n";

	$out->{register} .= <<CODE;
  procedural_db_register (\&${name}_proc);
CODE

	if (exists $proc->{invoke}->{headers}) {
	    foreach $header (@{$proc->{invoke}->{headers}}) {
		$out->{headers}->{$header}++;
	    }
	}
	$out->{headers}->{q/"procedural_db.h"/}++;

	$out->{code} .= "\nstatic Argument *\n";
	$out->{code} .= "${name}_invoker (Argument *args)\n{\n";

	my $code = "";

	if (exists $proc->{invoke}->{pass_through}) {
	    my $invoke = $proc->{invoke};

	    my $argc = 0;
	    $argc += @{$invoke->{pass_args}} if exists $invoke->{pass_args};
	    $argc += @{$invoke->{make_args}} if exists $invoke->{make_args};

	    my %pass; my @passgroup;
	    my $before = 0; my $contig = 0; my $pos = -1;
	    if (exists $invoke->{pass_args}) {
		foreach (@{$invoke->{pass_args}}) {
		    $pass{$_}++;
		    $_ - 1 == $before ? $contig = 1 : $pos++;
		    push @{$passgroup[$pos]}, $_;
		    $before = $_;
		}
	    } 
	    $code .= ' ' x 2 . "int i;\n" if $contig;

	    $code .= ' ' x 2 . "Argument argv[$argc];\n";

	    my $tempproc; $pos = 0;
	    foreach (@{$proc->{inargs}}) {
		$_->{argpos} = $pos++;
		push @{$tempproc->{inargs}}, $_ if !exists $pass{$_->{argpos}};
	    }

	    $code .= &declare_args($tempproc, $out, qw(inargs)) . "\n";

	    my $marshal = "";
	    foreach (@{$tempproc->{inargs}}) {
		my $argproc; $argproc->{inargs} = [ $_ ];
		$marshal .= &marshal_inargs($argproc, $_->{argpos});
	    }

	    if ($success) {
		$marshal .= <<CODE;
  if (!success)
    return procedural_db_return_args (\&${name}_proc, FALSE);

CODE
	    }

	    $marshal = substr($marshal, 1) if $marshal;
	    $code .= $marshal;

	    foreach (@passgroup) {
		$code .= ($#$_ ? <<LOOP : <<CODE) . "\n";
  for (i = $_->[0]; i < @{[ $_->[$#$_] + 1 ]}; i++)
    argv[i] = args[i];
LOOP
  argv[$_->[0]] = args[$_->[0]];
CODE
	    }

	    if (exists $invoke->{make_args}) {
		$pos = 0;
		foreach (@{$invoke->{make_args}}) {
		    while (exists $pass{$pos}) { $pos++ }
		    
		    my $arg = $arg_types{(&arg_parse($_->{type}))[0]};
		    my $type = &arg_ptype($arg);

		    $code .= <<CODE;
  argv[$pos].arg_type = PDB_$arg->{name};
CODE

		    my $frag = $_->{code};
		    $frag =~ s/%%arg%%/"argv[$pos].value.pdb_$type"/e;
		    $code .= &format_code_frag($frag, 0);

		    $pos++;
		}
		$code .= "\n";
	    }

	    $code .= <<CODE;
  return $invoke->{pass_through}_invoker (argv);
}
CODE
	}
	else {
	    my $invoker = "";
	
	    $invoker .= ' ' x 2 . "Argument *return_args;\n" if scalar @outargs;
	    $invoker .= &declare_args($proc, $out, qw(inargs outargs));

	    if (exists $proc->{invoke}->{vars}) {
		foreach (@{$proc->{invoke}->{vars}}) {
		   $invoker .= ' ' x 2 . $_ . ";\n";
		}
	    }

	    $invoker .= &marshal_inargs($proc, 0);

	    $invoker .= "\n" if $invoker && $invoker !~ /\n\n/s;

	    my $frag = &format_code_frag($proc->{invoke}->{code}, $success);

	    $frag = ' ' x 2 . "if (success)\n" . $frag if $success;
	    $success = ($frag =~ /success =/) unless $success;

	    $code .= $invoker . $frag;
	    $code .= "\n" if $frag =~ /\n\n/s || $invoker;
	    $code .= &marshal_outargs($proc) . "}\n";
	}

	if ($success) {
	    $success_init = 0 if $proc->{invoke}->{success} eq 'NONE';

	    my $header = ' ' x 2 . "gboolean success";
	    $header .= " = $proc->{invoke}->{success}" if $success_init;
	    $header .= ";\n";

	    $out->{code} .= $header;
	}

        $out->{code} .= $code;

	$out->{code} .= &make_arg_recs($proc, qw(inargs outargs));

	$out->{code} .= <<CODE;

static ProcRecord ${name}_proc =
{
  "gimp_$name",
  @{[ &quotewrap($proc->{blurb}, 2) ]},
  @{[ &quotewrap($proc->{help},  2) ]},
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
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

/* NOTE: This file is autogenerated by pdbgen.pl. */

GPL

    my $internal = "$destdir/internal_procs.h$FILE_EXT";
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

	my $cfile = "$destdir/${group}_cmds.c$FILE_EXT";
	open CFILE, "> $cfile" or die "Can't open $cmdfile: $!\n";
	print CFILE $gpl;
	foreach $header (sort keys %{$out->{headers}}) {
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

    $internal = "$destdir/internal_procs.c$FILE_EXT";
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
