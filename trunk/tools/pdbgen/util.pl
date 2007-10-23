# GIMP - The GNU Image Manipulation Program
# Copyright (C) 1998-2003 Manish Singh <yosh@gimp.org>

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

package Gimp::CodeGen::util;

use File::Copy 'cp';
use File::Compare 'cmp';

$DEBUG_OUTPUT = exists $ENV{PDBGEN_BACKUP} ? $ENV{PDBGEN_BACKUP} : 1;

$FILE_EXT = ".tmp.$$";

sub write_file {
    my $file = shift; my $realfile = $file;
    $realfile =~ s/$FILE_EXT$//;
    if (-e $realfile) {
	if (cmp($realfile, $file)) {
	    cp($realfile, "$realfile~") if $DEBUG_OUTPUT;
	    cp($file, $realfile);
	    print "Wrote $realfile\n";
	}
	else {
	    print "No changes to $realfile\n";
	}
	unlink $file;
    }
    else {
	rename $file, $realfile;
	print "Wrote $realfile\n";
    }
}

1;
