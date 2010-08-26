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

package Gimp::CodeGen::pdb;

%arg_types = (
    int32       => { name           => 'INT32',
		     type           => 'gint32 ',
		     const_type     => 'gint32 ',
		     init_value     => '0',
		     get_value_func => '$var = g_value_get_int ($value)',
		     set_value_func => 'g_value_set_int ($value, $var)' },

    int16       => { name           => 'INT16',
		     type           => 'gint16 ',
		     const_type     => 'gint16 ',
		     init_value     => '0',
		     get_value_func => '$var = g_value_get_int ($value)',
		     set_value_func => 'g_value_set_int ($value, $var)' },

    int8        => { name           => 'INT8' ,
		     type           => 'guint8 ',
		     const_type     => 'guint8 ',
		     init_value     => '0',
		     get_value_func => '$var = g_value_get_uint ($value)',
		     set_value_func => 'g_value_set_uint ($value, $var)' },

    float       => { name           => 'FLOAT',
		     type           => 'gdouble ',
		     const_type     => 'gdouble ',
		     init_value     => '0.0',
		     get_value_func => '$var = g_value_get_double ($value)',
		     set_value_func => 'g_value_set_double ($value, $var)' },

    string      => { name           => 'STRING',
		     type           => 'gchar *',
		     const_type     => 'const gchar *',
		     init_value     => 'NULL',
		     get_value_func => '$var = g_value_get_string ($value)',
		     set_value_func => 'g_value_take_string ($value, $var)' },

    int32array  => { name           => 'INT32ARRAY',
		     type           => 'gint32 *',
		     const_type     => 'const gint32 *',
		     array          => 1,
		     init_value     => 'NULL',
		     get_value_func => '$var = gimp_value_get_int32array ($value)',
		     set_value_func => 'gimp_value_take_int32array ($value, $var, $var_len)' },

    int16array  => { name           => 'INT16ARRAY',
		     type           => 'gint16 *',
		     const_type     => 'const gint16 *',
		     array          => 1,
		     init_value     => 'NULL',
		     get_value_func => '$var = gimp_value_get_int16array ($value)',
		     set_value_func => 'gimp_value_take_int16array ($value, $var, $var_len)' },

    int8array   => { name           => 'INT8ARRAY',
		     type           => 'guint8 *',
		     const_type     => 'const guint8 *',
		     array          => 1,
		     init_value     => 'NULL',
		     get_value_func => '$var = gimp_value_get_int8array ($value)',
		     set_value_func => 'gimp_value_take_int8array ($value, $var, $var_len)' },

    floatarray  => { name           => 'FLOATARRAY',
		     type           => 'gdouble *',
		     const_type     => 'const gdouble *',
		     array          => 1,
		     init_value     => 'NULL',
		     get_value_func => '$var = gimp_value_get_floatarray ($value)',
		     set_value_func => 'gimp_value_take_floatarray ($value, $var, $var_len)' },

    stringarray => { name           => 'STRINGARRAY',
		     type           => 'gchar **',
		     const_type     => 'const gchar **',
		     array          => 1,
		     init_value     => 'NULL',
		     get_value_func => '$var = gimp_value_get_stringarray ($value)',
		     set_value_func => 'gimp_value_take_stringarray ($value, $var, $var_len)' },

    colorarray  => { name           => 'COLORARRAY',
		     type           => 'GimpRGB *',
		     const_type     => 'const GimpRGB *',
		     array          => 1,
		     init_value     => 'NULL',
		     get_value_func => '$var = gimp_value_get_colorarray ($value)',
		     set_value_func => 'gimp_value_take_colorarray ($value, $var, $var_len)' },

    color       => { name           => 'COLOR' , 
		     type           => 'GimpRGB ',
		     const_type     => 'GimpRGB ',
		     struct         => 1,
		     init_value     => '{ 0.0, 0.0, 0.0, 1.0 }',
		     get_value_func => 'gimp_value_get_rgb ($value, &$var)',
		     set_value_func => 'gimp_value_set_rgb ($value, &$var)',
		     headers        => [ qw("libgimpcolor/gimpcolor.h") ] },

    display     => { name           => 'DISPLAY',
		     type           => 'GimpObject *',
		     const_type     => 'GimpObject *',
		     id             => 1,
		     init_value     => 'NULL',
		     get_value_func => '$var = gimp_value_get_display ($value, gimp)',
		     set_value_func => 'gimp_value_set_display ($value, $var)' },

    image       => { name           => 'IMAGE',
		     type           => 'GimpImage *', 
		     const_type     => 'GimpImage *', 
		     id             => 1,
		     init_value     => 'NULL',
		     get_value_func => '$var = gimp_value_get_image ($value, gimp)',
		     set_value_func => 'gimp_value_set_image ($value, $var)',
		     headers        => [ qw("core/gimpimage.h") ] },

    item        => { name           => 'ITEM',
		     type           => 'GimpItem *', 
		     const_type     => 'GimpItem *', 
		     id             => 1,
		     init_value     => 'NULL',
		     get_value_func => '$var = gimp_value_get_item ($value, gimp)',
		     set_value_func => 'gimp_value_set_item ($value, $var)',
		     headers        => [ qw("core/gimpitem.h") ] },

    layer       => { name           => 'LAYER',
		     type           => 'GimpLayer *', 
		     const_type     => 'GimpLayer *', 
		     id             => 1,
		     init_value     => 'NULL',
		     get_value_func => '$var = gimp_value_get_layer ($value, gimp)',
		     set_value_func => 'gimp_value_set_layer ($value, $var)',
		     headers        => [ qw("core/gimplayer.h") ] },

    channel     => { name           => 'CHANNEL',
		     type           => 'GimpChannel *',
		     const_type     => 'GimpChannel *',
		     id             => 1,
		     init_value     => 'NULL',
		     get_value_func => '$var = gimp_value_get_channel ($value, gimp)',
		     set_value_func => 'gimp_value_set_channel ($value, $var)',
		     headers        => [ qw("core/gimpchannel.h") ] },

    drawable    => { name           => 'DRAWABLE',
		     type           => 'GimpDrawable *',
		     const_type     => 'GimpDrawable *',
		     id             => 1,
		     init_value     => 'NULL',
		     get_value_func => '$var = gimp_value_get_drawable ($value, gimp)',
		     set_value_func => 'gimp_value_set_drawable ($value, $var)',
		     headers        => [ qw("core/gimpdrawable.h") ] },

    selection   => { name           => 'SELECTION',
		     type           => 'GimpSelection *',
		     const_type     => 'GimpSelection *',
		     id             => 1,
		     init_value     => 'NULL',
		     get_value_func => '$var = gimp_value_get_selection ($value, gimp)',
		     set_value_func => 'gimp_value_set_selection ($value, $var)',
		     headers        => [ qw("core/gimpselection.h") ] },

    layer_mask  => { name           => 'CHANNEL',
		     type           => 'GimpLayerMask *', 
		     const_type     => 'GimpLayerMask *', 
		     id             => 1,
		     init_value     => 'NULL',
		     get_value_func => '$var = gimp_value_get_layer_mask ($value, gimp)',
		     set_value_func => 'gimp_value_set_layer_mask ($value, $var)',
		     headers        => [ qw("core/gimplayermask.h") ] },

    vectors     => { name           => 'VECTORS',
		     type           => 'GimpVectors *', 
		     const_type     => 'GimpVectors *', 
		     id             => 1,
		     init_value     => 'NULL',
		     get_value_func => '$var = gimp_value_get_vectors ($value, gimp)',
		     set_value_func => 'gimp_value_set_vectors ($value, $var)',
		     headers        => [ qw("vectors/gimpvectors.h") ] },

    parasite    => { name           => 'PARASITE',
		     type           => 'GimpParasite *',
		     const_type     => 'const GimpParasite *',
		     init_value     => 'NULL',
		     get_value_func => '$var = g_value_get_boxed ($value)',
		     set_value_func => 'g_value_take_boxed ($value, $var)',
		     headers => [ qw("libgimpbase/gimpbase.h") ] },

    # Special cases
    enum        => { name           => 'INT32',
		     type           => 'gint32 ',
		     const_type     => 'gint32 ',
		     init_value     => '0',
		     get_value_func => '$var = g_value_get_enum ($value)',
		     set_value_func => 'g_value_set_enum ($value, $var)' },

    boolean     => { name           => 'INT32',
		     type           => 'gboolean ',
		     const_type     => 'gboolean ',
		     init_value     => 'FALSE',
		     get_value_func => '$var = g_value_get_boolean ($value)',
		     set_value_func => 'g_value_set_boolean ($value, $var)' },

    tattoo      => { name           => 'INT32',
		     type           => 'gint32 ',
		     const_type     => 'gint32 ',
		     init_value     => '0',
		     get_value_func => '$var = g_value_get_uint ($value)',
		     set_value_func => 'g_value_set_uint ($value, $var)' },

    guide       => { name           => 'INT32',
		     type           => 'gint32 ',
		     const_type     => 'gint32 ',
		     id             => 1,
		     init_value     => '0',
		     get_value_func => '$var = g_value_get_uint ($value)',
		     set_value_func => 'g_value_set_uint ($value, $var)' },

    unit        => { name           => 'INT32',
		     type           => 'GimpUnit ',
		     const_type     => 'GimpUnit ',
		     init_value     => '0',
		     get_value_func => '$var = g_value_get_int ($value)',
		     set_value_func => 'g_value_set_int ($value, $var)' }
);

# Split out the parts of an arg constraint
sub arg_parse {
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
    elsif ($arg =~ /^unit(?: \(min (.*?)\))?/) {
	my @retvals = ('unit');
	push @retvals, $1 if $1;
	return @retvals;
    }
    elsif ($arg =~ /^(?:([+-.\dA-Z_][^\s]*) \s* (<=|<))?
		     \s* (\w+) \s*
		     (?:(<=|<) \s* ([+-.\dA-Z_][^\s]*))?
		   /x) {
	return ($3, $1, $2, $5, $4);
    }
}

1;
