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

package Gimp::CodeGen::pdb;

%arg_types = (
    int32 => { name => 'INT32', type => 'gint32 ' },
    int16 => { name => 'INT16', type => 'gint16 ' },
    int8  => { name => 'INT8' , type => 'gint8 '  },

    float  => { name => 'FLOAT' , type => 'gdouble ' },
    string => { name => 'STRING', type => 'gchar *'  },

    int32array  => { name  => 'INT32ARRAY' , type  => 'gint32 *' , array => 1 },
    int16array  => { name  => 'INT16ARRAY' , type  => 'gint16 *' , array => 1 },
    int8array   => { name  => 'INT8ARRAY'  , type  => 'gint8 *'  , array => 1 },
    floatarray  => { name  => 'FLOATARRAY' , type  => 'gdouble *', array => 1 },
    stringarray => { name  => 'STRINGARRAY', type  => 'gchar **' , array => 1 },

    color  => { name => 'COLOR' , type => 'guchar *' },

    display   => { name => 'DISPLAY',
		   type => 'GDisplay *',
		   headers => [ qw("gdisplay.h") ],
		   id_func => 'gdisplay_get_ID',
		   id_ret_func => '$var->ID' },
    image     => { name => 'IMAGE',
		   type => 'GimpImage *', 
		   headers => [ qw("procedural_db.h") ],
		   id_func => 'pdb_id_to_image',
		   id_ret_func => 'pdb_image_to_id ($var)' },
    layer     => { name => 'LAYER',
		   type => 'GimpLayer *', 
		   headers => [ qw("drawable.h" "layer.h") ],
		   id_func => 'layer_get_ID',
		   id_ret_func => 'drawable_ID (GIMP_DRAWABLE ($var))' },
    channel   => { name => 'CHANNEL',
		   type => 'Channel *',
		   headers => [ qw("drawable.h" "channel.h") ],
		   id_func => 'channel_get_ID',
		   id_ret_func => 'drawable_ID (GIMP_DRAWABLE ($var))' },
    drawable  => { name => 'DRAWABLE',
		   type => 'GimpDrawable *',
		   headers => [ qw("drawable.h") ],
		   id_func => 'gimp_drawable_get_ID',
		   id_ret_func => 'drawable_ID (GIMP_DRAWABLE ($var))' },
    selection => { name => 'SELECTION',
		   type => 'Channel *',
		   headers => [ qw("drawable.h" "channel.h") ],
		   id_func => 'channel_get_ID',
		   id_ret_func => 'drawable_ID (GIMP_DRAWABLE ($var))' },

    parasite => { name => 'PARASITE', type => 'Parasite *',
		  headers => [ qw("libgimp/parasite.h") ] },

    boundary => { name => 'BOUNDARY', type => 'gpointer ' }, # ??? FIXME
    path     => { name => 'PATH'    , type => 'gpointer ' }, # ??? FIXME
    status   => { name => 'STATUS'  , type => 'gpointer ' }, # ??? FIXME

    # Special cases
    enum    => { name => 'INT32', type => 'gint32 '   },
    boolean => { name => 'INT32', type => 'gboolean ' },

    region => { name => 'REGION', type => 'gpointer ' } # not supported
);

# Split out the parts of an arg constraint
sub arg_parse {
    my %testmap = (
	'<'  => '>',
	'>'  => '<',
	'<=' => '>=',
	'>=' => '<='
    );

    my $arg = shift;

    if ($arg =~ /^enum (\w+)(.*)/) {
	my ($name, $remove) = ($1, $2);
	my @retvals = ('enum', $name);

	if ($remove && $remove =~ m@ \(no @) {
	    chop $remove; ($remove = substr($remove, 5)) =~ s/ $//;
	    push @retvals, split(/,\s*/, $remove);
	}

	return @retvals;
    }
    elsif ($arg =~ /^(?:([+-.\d].*?) \s* (<=|<))?
		     \s* (\w+) \s*
		     (?:(<=|<) \s* ([+-.\d].*?))?
		   /x) {
	return ($3, $1, $2 ? $testmap{$2} : $2, $5, $4 ? $testmap{$4} : $4);
    }
}

# Return the marshaller data type
sub arg_ptype {
    my $arg = shift;
    do {
	if (exists $arg->{id_func})       { 'int'     }
	elsif ($arg->{type} =~ /\*/)      { 'pointer' }
	elsif ($arg->{type} =~ /boolean/) { 'int'     }
	elsif ($arg->{type} =~ /int/)     { 'int'     }
	elsif ($arg->{type} =~ /double/)  { 'float'   }
	else                              { 'pointer' }
    };
}

# Return the alias if defined, otherwise the name
sub arg_vname { exists $_[0]->{alias} ? $_[0]->{alias} : $_[0]->{name} }

sub arg_numtype () { 'gint32 ' }
