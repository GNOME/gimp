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

package Gimp::CodeGen::pdb;

%arg_types = (
    int32 => { name => 'INT32', type => 'gint32 ' },
    int16 => { name => 'INT16', type => 'gint16 ' },
    int8  => { name => 'INT8' , type => 'gint8 '  },

    float  => { name => 'FLOAT' , type => 'gdouble ' },
    string => { name => 'STRING', type => 'gchar *'  },

    int32array   => { name => 'INT32ARRAY' , type => 'gint32 *'  },
    int16array   => { name => 'INT16ARRAY' , type => 'gint16 *'  },
    int8array    => { name => 'INT8ARRAY'  , type => 'gint8 *'   },
    floatarray   => { name => 'FLOATARRAY' , type => 'gdouble *' },
    stringarray  => { name => 'STRINGARRAY', type => 'gchar **'  },

    color  => { name => 'COLOR' , type => 'guchar *' },

    display   => {
		     name => 'DISPLAY',
		     type => 'GDisplay *',
		     id_func => 'gdisplay_get_ID',
		     id_ret_func => '$var->ID',
		     id_headers => [ qw("gdisplay.h") ]
		 },
    image     => {
		     name => 'IMAGE',
		     type => 'GImage *', 
		     id_func => 'pdb_id_to_image',
		     id_ret_func => 'pdb_image_to_id ($var)',
		     id_headers => [ qw("procedural_db.h") ]
		 },
    layer     => {
		     name => 'LAYER',
		     type => 'GimpLayer *', 
		     id_func => 'layer_get_ID',
		     id_ret_func => 'drawable_ID (GIMP_DRAWABLE ($var))',
		     id_headers => [ qw("drawable.h" "layer.h") ]
		 },
    channel   => {
		     name => 'CHANNEL',
		     type => 'Channel *',
		     id_func => 'channel_get_ID',
		     id_ret_func => 'drawable_ID (GIMP_DRAWABLE ($var))',
		     id_headers => [ qw("drawable.h" "channel.h") ]
		 },
    drawable  => {
		     name => 'DRAWABLE',
		     type => 'GimpDrawable *',
		     id_func => 'gimp_drawable_get_ID',
		     id_ret_func => 'drawable_ID (GIMP_DRAWABLE ($var))',
		     id_headers => [ qw("drawable.h") ]
		 },
    selection => {
		     name => 'SELECTION',
		     type => 'Channel *',
		     id_func => 'channel_get_ID',
		     id_ret_func => 'drawable_ID (GIMP_DRAWABLE ($var))',
		     id_headers => [ qw("drawable.h" "channel.h") ]
		 },

    boundary => { name => 'BOUNDARY', type => 'gpointer ' }, # ??? FIXME
    path     => { name => 'PATH'    , type => 'gpointer ' }, # ??? FIXME
    status   => { name => 'STATUS'  , type => 'gpointer ' }, # ??? FIXME

    # Special cases
    enum    => { name => 'INT32', type => 'gint32 '   },
    boolean => { name => 'INT32', type => 'gboolean ' },

    region => { name => 'REGION', type => 'gpointer ' }, # not supported
);

# Split out the parts of an arg constraint
sub arg_parse {
    my $arg = shift;
    if ($arg =~ /^enum (\w+)/) {
	return ('enum', $1);
    }
    if ($arg =~ /^([\d\.-].*?)? *(<=|<)? *(\w+) *(<=|<)? *([\d\.-].*?)?/) {
	return ($3, $1, $2, $5, $4);
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
	elsif ($arg->{type} =~ /float/)   { 'float'   }
	else                              { 'pointer' }
    };
}

# Return the alias if defined, otherwise the name
sub arg_vname { exists $_[0]->{alias} ? $_[0]->{alias} : $_[0]->{name} }
