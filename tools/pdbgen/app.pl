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
    while ($code =~ /^\t* {8}/m) { $code =~ s/^(\t*) {8}/$1\t/mg }
    $code .= "\n";

    $code;
}

sub make_arg_test {
    my ($arg, $reverse, $test) = @_;
    my $result = "";

    my $yes = exists $arg->{on_success};
    my $no  = !exists $arg->{no_success} || exists $arg->{on_fail};

    if ($yes || $no) {
	&$reverse(\$test) if $yes;

	if (exists $arg->{cond}) {
	    my $cond = "";
	    foreach (@{$arg->{cond}}) {
		$cond .= '!' if $yes;
		$cond .= /\W/ ? "($_)" : $_;
		$cond .= $yes ? ' !! ' : ' && ';
	    }
	    $test = "$cond($test)";
	}

	$result = ' ' x 2 . "if ($test)\n";

	$result .= &format_code_frag($arg->{on_success}, 1) if $yes;

	if ($no) {
	    $result .= ' ' x 2 . "else\n" if $yes;

	    if (!exists $arg->{no_success}) {
		$success = 1;
		$result .= ' ' x 4 . "success = FALSE;\n";
	    }

	    if (exists $arg->{on_fail}) {
		$result .= &format_code_frag($_->{on_fail}, 1);
	    }
	}
    }

    $result;
}

sub declare_args {
    my $proc = shift;
    my $out = shift;

    local $result = "";

    foreach (@_) {
	my @args = @{$proc->{$_}} if exists $proc->{$_};

	foreach (@args) {
	    my ($type, $name) = &arg_parse($_->{type});
	    my $arg = $arg_types{$type};

	    if ($type eq 'enum') {
		$out->{headers}->{qq/"$enums{$name}->{header}"/}++
	    }

	    if ($arg->{array} && !exists $_->{array}) {
		warn "Array without number of elements param in $proc->{name}";
	    }

	    unless (exists $_->{no_declare}) {
		$result .= ' ' x 2 . $arg->{type} . &arg_vname($_);
		if (!exists $_->{no_init} && exists $_->{init}) {
		    $result .= $arg->{type} =~ /\*$/ ? ' = NULL' : ' = 0'
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

sub declare_vars {
    my $proc = shift;
    my $code = "";
    if (exists $proc->{invoke}->{vars}) {
	foreach (@{$proc->{invoke}->{vars}}) {
	   $code .= ' ' x 2 . $_ . ";\n";
	}
    }
    $code;
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
		    /enum/      && do { my $enum = $enums{$name};
					$info = $enum->{info};
					foreach (@remove) {
					    if (exists $enum->{nicks}->{$_}) {
						$nick = $enum->{nicks}->{$_};
					    }
					    else {
						$nick = $_;
					    }
					    $info =~ s/$nick \(.*?\)(, )?//
					}				 
					$info =~ s/, $//;
					if (!$#{[$info =~ /,/g]} &&
					     $desc !~ /{ %%desc%% }/) {
					    $info =~ s/,/ or/
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
	    $result .= &make_arg_test($_, sub { ${$_[0]} =~ s/==/!=/ },
				      "$var == NULL");
	}
	else {
	    $result .= ' ' x 2 . "$var =";

	    my $cast = "";
	    if ($type eq 'pointer' || $arg->{type} =~ /int(16|8)$/) {
		$cast = " ($arg->{type})";
	    }
	    $result .= "$cast args[$argc].value.pdb_$type";
	    $result .= ' ? TRUE : FALSE' if $pdbtype eq 'boolean';
	    $result .= ";\n";

	    if ($pdbtype eq 'string' || $pdbtype eq 'parasite') {
		$result .= &make_arg_test($_, sub { ${$_[0]} =~ s/==/!=/ },
					  "$var == NULL");
	    }
	    elsif ($pdbtype eq 'tattoo') {
		$result .= &make_arg_test($_, sub { ${$_[0]} =~ s/==/!=/ },
					  '$var == 0');
	    }
	    elsif ($pdbtype eq 'unit') {
		$result .= &make_arg_test($_, sub { ${$_[0]} = "!(${$_[0]})" },
					  "$var < UNIT_PIXEL || $var >= " .
					  'gimp_unit_get_number_of_units ()');
	    }
	    elsif ($pdbtype eq 'enum' && !$enums{$typeinfo[0]}->{contig}) {
		if (!exists $_->{no_success} || exists $_->{on_success} ||
		     exists $_->{on_fail}) {
		    my %vals; my $symbols = $enums{pop @typeinfo}->{symbols};
		    @vals{@$symbols}++; delete @vals{@typeinfo};

		    my $okvals = ""; my $failvals = "";

		    my $once = 0;
		    foreach (@$symbols) {
			if (exists $vals{$_}) {
			    $okvals .= ' ' x 4 if $once++;
			    $okvals .= "case $_:\n";
			}
		    }

		    sub format_switch_frag {
			my ($arg, $key) = @_;
			my $frag = "";
			if (exists $arg->{$key}) {
			    $frag = &format_code_frag($arg->{$key}, 1);
			    $frag =~ s/\t/' ' x 8/eg;
			    $frag =~ s/^/' ' x 2/meg;
			    $frag =~ s/^ {8}/\t/mg;
			}
			$frag;
		    }

		    $okvals .= &format_switch_frag($_, 'on_success');
		    chomp $okvals;

		    $failvals .= "default:\n";
		    if (!exists $_->{no_success}) {
			$success = 1;
			$failvals .= ' ' x 6 . "success = FALSE;\n"
		    }
		    $failvals .=  &format_switch_frag($_, 'on_fail');
		    chomp $failvals;

		    $result .= <<CODE;
  switch ($var)
    {
    $okvals
      break;

    $failvals
      break;
    }
CODE
		}
	    }
	    elsif (defined $typeinfo[0] || defined $typeinfo[2]) {
		my $code = ""; my $tests = 0; my $extra = "";

		if ($pdbtype eq 'enum') {
		    my $symbols = $enums{shift @typeinfo}->{symbols};

		    my ($start, $end) = (0, $#$symbols);

		    my $syms = "@$symbols "; my $test = $syms;
		    foreach (@typeinfo) { $test =~ s/$_ // }

		    if ($syms =~ /$test/g) {
			if (pos $syms  == length $syms) {
			    $start = @typeinfo;
			}
			else {
			    $end -= @typeinfo;
			}
		    }
		    else {
			foreach (@typeinfo) {
			    $extra .= " || $var == $_";
			}
		    }

		    $typeinfo[0] = $symbols->[$start];
		    if ($start != $end) {
			$typeinfo[1] = '<';
			$typeinfo[2] = $symbols->[$end];
			$typeinfo[3] = '>';
		    }
		    else {
			$typeinfo[1] = '!=';
			undef @typeinf[2..3];
		    }
		}
		elsif ($pdbtype eq 'float') {
		    foreach (@typeinfo[0, 2]) {
			$_ .= '.0' if defined $_ && !/\./
		    }
		}

		if (defined $typeinfo[0]) {
		    $code .= "$var $typeinfo[1] $typeinfo[0]";
		    $code .= '.0' if $pdbtype eq 'float' && $typeinfo[0] !~ /\./;
		    $tests++;
		}

		if (defined $typeinfo[2]) {
		    $code .= ' || ' if $tests;
		    $code .= "$var $typeinfo[3] $typeinfo[2]";
		}

		$code .= $extra;

		$result .= &make_arg_test($_, sub { ${$_[0]} = "!(${$_[0]})" },
					  $code);
	    }
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
	    $code .= &declare_vars($proc);

	    my $marshal = "";
	    foreach (@{$tempproc->{inargs}}) {
		my $argproc; $argproc->{inargs} = [ $_ ];
		$marshal .= &marshal_inargs($argproc, $_->{argpos});
		chop $marshal;
	    }
	    $marshal .= "\n" if $marshal;

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
	    $invoker .= &declare_vars($proc);

	    $invoker .= &marshal_inargs($proc, 0);
	    $invoker .= "\n" if $invoker && $invoker !~ /\n\n/s;

	    my $frag = "";

	    if (exists $proc->{invoke}->{code}) {
		$frag = &format_code_frag($proc->{invoke}->{code}, $success);
		$frag = ' ' x 2 . "if (success)\n" . $frag if $success;
		$success = ($frag =~ /success =/) unless $success;
	    }

	    chomp $invoker if !$frag;
	    $code .= $invoker . $frag;
	    $code .= "\n" if $frag =~ /\n\n/s || $invoker;
	    $code .= &marshal_outargs($proc) . "}\n";
	}

	if ($success) {
	    $out->{code} .= ' ' x 2 . "gboolean success";
	    unless ($proc->{invoke}->{success} eq 'NONE') {
		$out->{code} .= " = $proc->{invoke}->{success}";
	    }
	    $out->{code} .= ";\n";
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
  @{[scalar @inargs]},
  @{[scalar @inargs ? "${name}_inargs" : 'NULL']},
  @{[scalar @outargs]},
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

    my $group_procs = ""; my $longest = 0;
    my $once = 0; my $pcount = 0.0;
    foreach $group (@main::groups) {
	my $out = $out{$group};

	foreach (@{$main::grp{$group}->{headers}}) { $out->{headers}->{$_}++ }
	delete $out->{headers}->{q/"procedural_db.h"/};

	my $headers = "";
	foreach (sort map { s/^</!/; $_ } keys %{$out->{headers}}) {
	    s/^\!/</; $headers .= "#include $_\n";
	}

	my $extra = {};
	if (exists $main::grp{$group}->{extra}->{app}) {
	    $extra = $main::grp{$group}->{extra}->{app}
	}

	my $cfile = "$destdir/${group}_cmds.c$FILE_EXT";
	open CFILE, "> $cfile" or die "Can't open $cmdfile: $!\n";
	print CFILE $gpl;
	print CFILE qq/#include "procedural_db.h"\n\n/;
	print CFILE $headers, "\n";
	print CFILE $extra->{decls}, "\n" if exists $extra->{decls};
	print CFILE $out->{procs};
	print CFILE "\nvoid\nregister_${group}_procs (void)\n";
	print CFILE "{\n$out->{register}}\n";
	print CFILE "\n", $extra->{code} if exists $extra->{code};
	print CFILE $out->{code};
	close CFILE;
	&write_file($cfile);

	my $decl = "register_${group}_procs";
	push @group_decls, $decl;
	$longest = length $decl if $longest < length $decl;

	$group_procs .= ' ' x 2 . "app_init_update_status (";
	$group_procs .= q/_("Internal Procedures")/ unless $once;
	$group_procs .= 'NULL' if $once++;
	$group_procs .= qq/, _("$main::grp{$group}->{desc}"), /;
       ($group_procs .= sprintf "%.3f", $pcount / $total) =~ s/\.?0*$//s;
	$group_procs .= ($group_procs !~ /\.\d+$/s ? ".0" : "") . ");\n";
	$group_procs .=  ' ' x 2 . "register_${group}_procs ();\n\n";
	$pcount += $out->{pcount};
    }

    if (!exists $ENV{PDBGEN_GROUPS}) {
	my $internal = "$destdir/internal_procs.h$FILE_EXT";
	open IFILE, "> $internal" or die "Can't open $cmdfile: $!\n";
	print IFILE $gpl;
	my $guard = "__INTERNAL_PROCS_H__";
	print IFILE <<HEADER;
#ifndef $guard
#define $guard

void internal_procs_init (void);

#endif /* $guard */
HEADER
	close IFILE;
	&write_file($internal);

	$internal = "$destdir/internal_procs.c$FILE_EXT";
	open IFILE, "> $internal" or die "Can't open $cmdfile: $!\n";
	print IFILE $gpl;
	print IFILE qq@#include "config.h"\n\n@;
	print IFILE qq@#include "app_procs.h"\n\n@;
	print IFILE qq@#include "libgimp/gimpintl.h"\n\n@;
	print IFILE "/* Forward declarations for registering PDB procs */\n\n";
	foreach (@group_decls) {
	    print IFILE "void $_" . ' ' x ($longest - length $_) . " (void);\n";
	}
	chop $group_procs;
	print IFILE "\n/* $total total procedures registered total */\n\n";
	print IFILE "void\ninternal_procs_init (void)\n{\n$group_procs}\n";
	close IFILE;
	&write_file($internal);
    }
}

1;
