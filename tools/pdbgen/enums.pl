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

package Gimp::CodeGen::enums;

%enums = (
    ConvertPaletteType =>
	{ name => 'ConvertPaletteType', base => 0,
	  symbols => [ qw(MAKE_PALETTE REUSE_PALETTE WEB_PALETTE MONO_PALETTE
			  CUSTOM_PALETTE) ] }

);

foreach $enum (values %enums) {
    $enum->{start} = $enum->{symbols}->[0];
    $enum->{end}   = $enum->{symbols}->[$#{$enum->{symbols}}];

    my $pos = 0; $enum->{info} = "";
    foreach (@{$enum->{symbols}}) { $enum->{info} .= "$_ ($pos), "; $pos++ }
    $enum->{info} =~ s/, $//;
}

1;
