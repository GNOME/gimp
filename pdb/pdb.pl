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

package Gimp::CodeGen::pdb;

%arg_types = (
    int32       => { name            => 'INT32',
		     gtype           => 'GIMP_TYPE_INT32',
		     type            => 'gint32 ',
		     const_type      => 'gint32 ',
		     init_value      => '0',
		     get_value_func  => '$var = g_value_get_int ($value)',
		     dup_value_func  => '$var = g_value_get_int ($value)',
		     set_value_func  => 'g_value_set_int ($value, $var)',
		     take_value_func => 'g_value_set_int ($value, $var)' },

    int16       => { name            => 'INT16',
		     gtype           => 'GIMP_TYPE_INT16',
		     type            => 'gint16 ',
		     const_type      => 'gint16 ',
		     init_value      => '0',
		     get_value_func  => '$var = g_value_get_int ($value)',
		     dup_value_func  => '$var = g_value_get_int ($value)',
		     set_value_func  => 'g_value_set_int ($value, $var)',
		     take_value_func => 'g_value_set_int ($value, $var)' },

    int8        => { name            => 'INT8',
		     gtype           => 'GIMP_TYPE_INT8',
		     type            => 'guint8 ',
		     const_type      => 'guint8 ',
		     init_value      => '0',
		     get_value_func  => '$var = g_value_get_uint ($value)',
		     dup_value_func  => '$var = g_value_get_uint ($value)',
		     set_value_func  => 'g_value_set_uint ($value, $var)',
		     take_value_func => 'g_value_set_uint ($value, $var)' },

    float       => { name            => 'FLOAT',
		     gtype           => 'G_TYPE_DOUBLE',
		     type            => 'gdouble ',
		     const_type      => 'gdouble ',
		     init_value      => '0.0',
		     get_value_func  => '$var = g_value_get_double ($value)',
		     dup_value_func  => '$var = g_value_get_double ($value)',
		     set_value_func  => 'g_value_set_double ($value, $var)',
		     take_value_func => 'g_value_set_double ($value, $var)' },

    string      => { name            => 'STRING',
		     gtype           => 'G_TYPE_STRING',
		     type            => 'gchar *',
		     const_type      => 'const gchar *',
		     init_value      => 'NULL',
		     get_value_func  => '$var = g_value_get_string ($value)',
		     dup_value_func  => '$var = g_value_dup_string ($value)',
		     set_value_func  => 'g_value_set_string ($value, $var)',
		     take_value_func => 'g_value_take_string ($value, $var)' },

    int32array  => { name            => 'INT32ARRAY',
		     gtype           => 'GIMP_TYPE_INT32_ARRAY',
		     type            => 'gint32 *',
		     const_type      => 'const gint32 *',
		     array           => 1,
		     init_value      => 'NULL',
		     get_value_func  => '$var = gimp_value_get_int32_array ($value)',
		     dup_value_func  => '$var = gimp_value_dup_int32_array ($value)',
		     set_value_func  => 'gimp_value_set_int32_array ($value, $var, $var_len)',
		     take_value_func => 'gimp_value_take_int32_array ($value, $var, $var_len)' },

    int16array  => { name            => 'INT16ARRAY',
		     gtype           => 'GIMP_TYPE_INT16_ARRAY',
		     type            => 'gint16 *',
		     const_type      => 'const gint16 *',
		     array           => 1,
		     init_value      => 'NULL',
		     get_value_func  => '$var = gimp_value_get_int16_array ($value)',
		     dup_value_func  => '$var = gimp_value_dup_int16_array ($value)',
		     set_value_func  => 'gimp_value_set_int16_array ($value, $var, $var_len)',
		     take_value_func => 'gimp_value_take_int16_array ($value, $var, $var_len)' },

    int8array   => { name            => 'INT8ARRAY',
		     gtype           => 'GIMP_TYPE_INT8_ARRAY',
		     type            => 'guint8 *',
		     const_type      => 'const guint8 *',
		     array           => 1,
		     init_value      => 'NULL',
		     get_value_func  => '$var = gimp_value_get_int8_array ($value)',
		     dup_value_func  => '$var = gimp_value_dup_int8_array ($value)',
		     set_value_func  => 'gimp_value_set_int8_array ($value, $var, $var_len)',
		     take_value_func => 'gimp_value_take_int8_array ($value, $var, $var_len)' },

    floatarray  => { name            => 'FLOATARRAY',
		     gtype           => 'GIMP_TYPE_FLOAT_ARRAY',
		     type            => 'gdouble *',
		     const_type      => 'const gdouble *',
		     array           => 1,
		     init_value      => 'NULL',
		     get_value_func  => '$var = gimp_value_get_float_array ($value)',
		     dup_value_func  => '$var = gimp_value_dup_float_array ($value)',
		     set_value_func  => 'gimp_value_set_float_array ($value, $var, $var_len)',
		     take_value_func => 'gimp_value_take_float_array ($value, $var, $var_len)' },

    stringarray => { name            => 'STRINGARRAY',
		     gtype           => 'GIMP_TYPE_STRING_ARRAY',
		     type            => 'gchar **',
		     const_type      => 'const gchar **',
		     array           => 1,
		     init_value      => 'NULL',
		     get_value_func  => '$var = gimp_value_get_string_array ($value)',
		     dup_value_func  => '$var = gimp_value_dup_string_array ($value)',
		     set_value_func  => 'gimp_value_set_string_array ($value, $var, $var_len)',
		     take_value_func => 'gimp_value_take_string_array ($value, $var, $var_len)' },

    colorarray  => { name            => 'COLORARRAY',
		     gtype           => 'GIMP_TYPE_RGB_ARRAY',
		     type            => 'GimpRGB *',
		     const_type      => 'const GimpRGB *',
		     array           => 1,
		     init_value      => 'NULL',
		     get_value_func  => '$var = gimp_value_get_rgb_array ($value)',
		     dup_value_func  => '$var = gimp_value_dup_rgb_array ($value)',
		     set_value_func  => 'gimp_value_set_rgb_array ($value, $var, $var_len)',
		     take_value_func => 'gimp_value_take_rgb_array ($value, $var, $var_len)' },

    color       => { name            => 'COLOR',
		     gtype           => 'GIMP_TYPE_RGB',
		     type            => 'GimpRGB ',
		     const_type      => 'GimpRGB ',
		     struct          => 1,
		     init_value      => '{ 0.0, 0.0, 0.0, 1.0 }',
		     get_value_func  => 'gimp_value_get_rgb ($value, &$var)',
		     dup_value_func  => 'gimp_value_get_rgb ($value, &$var)',
		     set_value_func  => 'gimp_value_set_rgb ($value, $var)',
		     take_value_func => 'gimp_value_set_rgb ($value, &$var)',
		     headers         => [ qw(<cairo.h> "libgimpcolor/gimpcolor.h") ] },

    display     => { name            => 'DISPLAY',
		     gtype           => 'GIMP_TYPE_DISPLAY_ID',
		     type            => 'GimpObject *',
		     const_type      => 'GimpObject *',
		     id              => 1,
		     init_value      => 'NULL',
		     get_value_func  => '$var = gimp_value_get_display ($value, gimp)',
		     dup_value_func  => '$var = gimp_value_get_display_id ($value)',
		     set_value_func  => 'gimp_value_set_display_id ($value, $var)',
		     take_value_func => 'gimp_value_set_display ($value, $var)' },

    image       => { name            => 'IMAGE',
		     gtype           => 'GIMP_TYPE_IMAGE_ID',
		     type            => 'GimpImage *',
		     const_type      => 'GimpImage *',
		     id              => 1,
		     init_value      => 'NULL',
		     get_value_func  => '$var = gimp_value_get_image ($value, gimp)',
		     dup_value_func  => '$var = gimp_value_get_image_id ($value)',
		     set_value_func  => 'gimp_value_set_image_id ($value, $var)',
		     take_value_func => 'gimp_value_set_image ($value, $var)',
		     headers         => [ qw("core/gimpimage.h") ] },

    item        => { name            => 'ITEM',
		     gtype           => 'GIMP_TYPE_ITEM_ID',
		     type            => 'GimpItem *',
		     const_type      => 'GimpItem *',
		     id              => 1,
		     init_value      => 'NULL',
		     get_value_func  => '$var = gimp_value_get_item ($value, gimp)',
		     dup_value_func  => '$var = gimp_value_get_item_id ($value)',
		     set_value_func  => 'gimp_value_set_item_id ($value, $var)',
		     take_value_func => 'gimp_value_set_item ($value, $var)',
		     headers         => [ qw("core/gimpitem.h") ] },

    layer       => { name            => 'LAYER',
		     gtype           => 'GIMP_TYPE_LAYER_ID',
		     type            => 'GimpLayer *',
		     const_type      => 'GimpLayer *',
		     id              => 1,
		     init_value      => 'NULL',
		     get_value_func  => '$var = gimp_value_get_layer ($value, gimp)',
		     dup_value_func  => '$var = gimp_value_get_layer_id ($value)',
		     set_value_func  => 'gimp_value_set_layer_id ($value, $var)',
		     take_value_func => 'gimp_value_set_layer ($value, $var)',
		     headers         => [ qw("core/gimplayer.h") ] },

    channel     => { name            => 'CHANNEL',
		     gtype           => 'GIMP_TYPE_CHANNEL_ID',
		     type            => 'GimpChannel *',
		     const_type      => 'GimpChannel *',
		     id              => 1,
		     init_value      => 'NULL',
		     get_value_func  => '$var = gimp_value_get_channel ($value, gimp)',
		     dup_value_func  => '$var = gimp_value_get_channel_id ($value)',
		     set_value_func  => 'gimp_value_set_channel_id ($value, $var)',
		     take_value_func => 'gimp_value_set_channel ($value, $var)',
		     headers         => [ qw("core/gimpchannel.h") ] },

    drawable    => { name            => 'DRAWABLE',
		     gtype           => 'GIMP_TYPE_DRAWABLE_ID',
		     type            => 'GimpDrawable *',
		     const_type      => 'GimpDrawable *',
		     id              => 1,
		     init_value      => 'NULL',
		     get_value_func  => '$var = gimp_value_get_drawable ($value, gimp)',
		     dup_value_func  => '$var = gimp_value_get_drawable_id ($value)',
		     set_value_func  => 'gimp_value_set_drawable_id ($value, $var)',
		     take_value_func => 'gimp_value_set_drawable ($value, $var)',
		     headers         => [ qw("core/gimpdrawable.h") ] },

    selection   => { name            => 'SELECTION',
		     gtype           => 'GIMP_TYPE_SELECTION_ID',
		     type            => 'GimpSelection *',
		     const_type      => 'GimpSelection *',
		     id              => 1,
		     init_value      => 'NULL',
		     get_value_func  => '$var = gimp_value_get_selection ($value, gimp)',
		     dup_value_func  => '$var = gimp_value_get_selection_id ($value)',
		     set_value_func  => 'gimp_value_set_selection_id ($value, $var)',
		     take_value_func => 'gimp_value_set_selection ($value, $var)',
		     headers         => [ qw("core/gimpselection.h") ] },

    layer_mask  => { name            => 'CHANNEL',
		     gtype           => 'GIMP_TYPE_LAYER_MASK_ID',
		     type            => 'GimpLayerMask *',
		     const_type      => 'GimpLayerMask *',
		     id              => 1,
		     init_value      => 'NULL',
		     get_value_func  => '$var = gimp_value_get_layer_mask ($value, gimp)',
		     dup_value_func  => '$var = gimp_value_get_layer_mask_id ($value)',
		     set_value_func  => 'gimp_value_set_layer_mask_id ($value, $var)',
		     take_value_func => 'gimp_value_set_layer_mask ($value, $var)',
		     headers         => [ qw("core/gimplayermask.h") ] },

    vectors     => { name            => 'VECTORS',
		     gtype           => 'GIMP_TYPE_VECTORS_ID',
		     type            => 'GimpVectors *',
		     const_type      => 'GimpVectors *',
		     id              => 1,
		     init_value      => 'NULL',
		     get_value_func  => '$var = gimp_value_get_vectors ($value, gimp)',
		     dup_value_func  => '$var = gimp_value_get_vectors_id ($value)',
		     set_value_func  => 'gimp_value_set_vectors_id ($value, $var)',
		     take_value_func => 'gimp_value_set_vectors ($value, $var)',
		     headers         => [ qw("vectors/gimpvectors.h") ] },

    parasite    => { name            => 'PARASITE',
		     gtype           => 'GIMP_TYPE_PARASITE',
		     type            => 'GimpParasite *',
		     const_type      => 'const GimpParasite *',
		     init_value      => 'NULL',
		     get_value_func  => '$var = g_value_get_boxed ($value)',
		     dup_value_func  => '$var = g_value_dup_boxed ($value)',
		     set_value_func  => 'g_value_set_boxed ($value, $var)',
		     take_value_func => 'g_value_take_boxed ($value, $var)',
		     headers         => [ qw("libgimpbase/gimpbase.h") ] },

    # Special cases
    enum        => { name            => 'INT32',
		     gtype           => 'G_TYPE_ENUM',
		     type            => 'gint32 ',
		     const_type      => 'gint32 ',
		     init_value      => '0',
		     get_value_func  => '$var = g_value_get_enum ($value)',
		     dup_value_func  => '$var = g_value_get_enum ($value)',
		     set_value_func  => 'g_value_set_enum ($value, $var)',
		     take_value_func => 'g_value_set_enum ($value, $var)' },

    boolean     => { name            => 'INT32',
		     gtype           => 'G_TYPE_BOOLEAN',
		     type            => 'gboolean ',
		     const_type      => 'gboolean ',
		     init_value      => 'FALSE',
		     get_value_func  => '$var = g_value_get_boolean ($value)',
		     dup_value_func  => '$var = g_value_get_boolean ($value)',
		     set_value_func  => 'g_value_set_boolean ($value, $var)',
		     take_value_func => 'g_value_set_boolean ($value, $var)' },

    tattoo      => { name            => 'INT32',
		     gtype           => 'GIMP_TYPE_INT32',
		     type            => 'gint32 ',
		     const_type      => 'gint32 ',
		     init_value      => '0',
		     get_value_func  => '$var = g_value_get_uint ($value)',
		     dup_value_func  => '$var = g_value_get_uint ($value)',
		     set_value_func  => 'g_value_set_uint ($value, $var)',
		     take_value_func => 'g_value_set_uint ($value, $var)' },

    guide       => { name            => 'INT32',
		     gtype           => 'GIMP_TYPE_INT32',
		     type            => 'gint32 ',
		     const_type      => 'gint32 ',
		     id              => 1,
		     init_value      => '0',
		     get_value_func  => '$var = g_value_get_uint ($value)',
		     dup_value_func  => '$var = g_value_get_uint ($value)',
		     set_value_func  => 'g_value_set_uint ($value, $var)',
		     take_value_func => 'g_value_set_uint ($value, $var)' },

   sample_point => { name            => 'INT32',
		     gtype           => 'GIMP_TYPE_INT32',
		     type            => 'gint32 ',
		     const_type      => 'gint32 ',
		     id              => 1,
		     init_value      => '0',
		     get_value_func  => '$var = g_value_get_uint ($value)',
		     dup_value_func  => '$var = g_value_get_uint ($value)',
		     set_value_func  => 'g_value_set_uint ($value, $var)',
		     take_value_func => 'g_value_set_uint ($value, $var)' },

    unit        => { name            => 'INT32',
		     gtype           => 'GIMP_TYPE_UNIT',
		     type            => 'GimpUnit ',
		     const_type      => 'GimpUnit ',
		     init_value      => '0',
		     get_value_func  => '$var = g_value_get_int ($value)',
		     dup_value_func  => '$var = g_value_get_int ($value)',
		     set_value_func  => 'g_value_set_int ($value, $var)',
		     take_value_func => 'g_value_set_int ($value, $var)' }
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
