# The GIMP -- an image manipulation program
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

package Gimp::CodeGen::pdb;

%arg_types = (
    int32 => { name => 'INT32', type => 'gint32 ' },
    int16 => { name => 'INT16', type => 'gint16 ' },
    int8  => { name => 'INT8' , type => 'guint8 ' },

    float  => { name => 'FLOAT' , type => 'gdouble ' },
    string => { name => 'STRING', type => 'gchar *'  },

    int32array  => { name  => 'INT32ARRAY' , type  => 'gint32 *' , array => 1 },
    int16array  => { name  => 'INT16ARRAY' , type  => 'gint16 *' , array => 1 },
    int8array   => { name  => 'INT8ARRAY'  , type  => 'guint8 *' , array => 1 },
    floatarray  => { name  => 'FLOATARRAY' , type  => 'gdouble *', array => 1 },
    stringarray => { name  => 'STRINGARRAY', type  => 'gchar **' , array => 1 },

    color      => { name => 'COLOR' , 
		    type => 'GimpRGB ',
		    headers => [ qw("libgimpcolor/gimpcolor.h") ],
		    struct => 1 },
    display    => { name => 'DISPLAY',
		    type => 'GimpObject *',
		    id_func => 'gimp_get_display_by_ID',
		    id_ret_func => 'gimp_get_display_ID (gimp, $var)',
	            check_func => 'GIMP_IS_OBJECT ($var)' },
    image      => { name => 'IMAGE',
		    type => 'GimpImage *', 
		    headers => [ qw("core/gimpimage.h") ],
		    id_func => 'gimp_image_get_by_ID',
		    id_ret_func => 'gimp_image_get_ID ($var)',
		    check_func => 'GIMP_IS_IMAGE ($var)' },
    layer      => { name => 'LAYER',
		    type => 'GimpLayer *', 
		    headers => [ qw("core/gimplayer.h") ],
		    id_func => '(GimpLayer *) gimp_item_get_by_ID',
		    id_ret_func => 'gimp_item_get_ID (GIMP_ITEM ($var))',
		    check_func => '(GIMP_IS_LAYER ($var) && ! gimp_item_is_removed (GIMP_ITEM ($var)))' },
    channel    => { name => 'CHANNEL',
		    type => 'GimpChannel *',
		    headers => [ qw("core/gimpchannel.h") ],
		    id_func => '(GimpChannel *) gimp_item_get_by_ID',
		    id_ret_func => 'gimp_item_get_ID (GIMP_ITEM ($var))',
		    check_func => '(GIMP_IS_CHANNEL ($var) && ! gimp_item_is_removed (GIMP_ITEM ($var)))' },
    drawable   => { name => 'DRAWABLE',
		    type => 'GimpDrawable *',
		    headers => [ qw("core/gimpdrawable.h") ],
		    id_func => '(GimpDrawable *) gimp_item_get_by_ID',
		    id_ret_func => 'gimp_item_get_ID (GIMP_ITEM ($var))',
		    check_func => '(GIMP_IS_DRAWABLE ($var) && ! gimp_item_is_removed (GIMP_ITEM ($var)))' },
    selection  => { name => 'SELECTION',
		    type => 'GimpChannel *',
		    headers => [ qw("core/gimpchannel.h") ],
		    id_func => '(GimpChannel *) gimp_item_get_by_ID',
		    id_ret_func => 'gimp_item_get_ID (GIMP_ITEM ($var))',
		    check_func => '(GIMP_IS_CHANNEL ($var) && ! gimp_item_is_removed (GIMP_ITEM ($var)))' },
    layer_mask => { name => 'CHANNEL',
		    type => 'GimpLayerMask *', 
		    headers => [ qw("core/gimplayermask.h") ],
		    id_func => '(GimpLayerMask *) gimp_item_get_by_ID',
		    id_ret_func => 'gimp_item_get_ID (GIMP_ITEM ($var))',
		    check_func => '(GIMP_IS_LAYER_MASK ($var) && ! gimp_item_is_removed (GIMP_ITEM ($var)))' },
    vectors    => { name => 'PATH',
		    type => 'GimpVectors *', 
		    headers => [ qw("vectors/gimpvectors.h") ],
		    id_func => '(GimpVectors *) gimp_item_get_by_ID',
		    id_ret_func => 'gimp_item_get_ID (GIMP_ITEM ($var))',
		    check_func => '(GIMP_IS_VECTORS ($var) && ! gimp_item_is_removed (GIMP_ITEM ($var)))' },
    parasite   => { name => 'PARASITE',
		    type => 'GimpParasite *',
		    headers => [ qw("libgimpbase/gimpbase.h") ] },

    boundary => { name => 'BOUNDARY', type => 'gpointer ' }, # ??? FIXME
    status   => { name => 'STATUS'  , type => 'gpointer ' }, # ??? FIXME

    # Special cases
    enum    => { name => 'INT32', type => 'gint32 '   },
    boolean => { name => 'INT32', type => 'gboolean ' },
    tattoo  => { name => 'INT32', type => 'gint32 '   },
    guide   => { name => 'INT32', type => 'gint32 '   },
    unit    => { name => 'INT32', type => 'GimpUnit ' },

    region => { name => 'REGION', type => 'gpointer ' } # not supported
);

# Split out the parts of an arg constraint
sub arg_parse {
    my %premap = (
	'<'  => '<=',
	'>'  => '>=',
	'<=' => '<',
	'>=' => '>'
    );

    my %postmap = (
	'<'  => '>=',
	'>'  => '<=',
	'<=' => '>',
	'>=' => '<'
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
    elsif ($arg =~ /^unit(?: \(min (.*?)\))?/) {
	my @retvals = ('unit');
	push @retvals, $1 if $1;
	return @retvals;
    }
    elsif ($arg =~ /^(?:([+-.\d][^\s]*) \s* (<=|<))?
		     \s* (\w+) \s*
		     (?:(<=|<) \s* ([+-.\d][^\s]*))?
		   /x) {
	return ($3, $1, $2 ? $premap{$2} : $2, $5, $4 ? $postmap{$4} : $4);
    }
}

# Return the marshaller data type
sub arg_ptype {
    my $arg = shift;
    do {
	if (exists $arg->{id_func})        { 'int'     }
	elsif ($arg->{type} =~ /\*/)       { 'pointer' }
	elsif ($arg->{type} =~ /boolean/)  { 'int'     }
	elsif ($arg->{type} =~ /GimpUnit/) { 'int'     }
	elsif ($arg->{type} =~ /int/)      { 'int'     }
	elsif ($arg->{type} =~ /double/)   { 'float'   }
	elsif ($arg->{type} =~ /GimpRGB/)  { 'color'   }
	else                               { 'pointer' }
    };
}

# Return the alias if defined, otherwise the name
sub arg_vname { exists $_[0]->{alias} ? $_[0]->{alias} : $_[0]->{name} }

sub arg_numtype () { 'gint32 ' }

1;
