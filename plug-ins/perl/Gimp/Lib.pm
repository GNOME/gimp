package Gimp::Lib;

use strict;
use Carp;
use vars qw($VERSION @ISA);

require DynaLoader;

@ISA = qw(DynaLoader);
$VERSION = $Gimp::VERSION;

use subs qw(
	gimp_call_procedure		gimp_main
	_gimp_procedure_available	set_trace
);

sub import {}

bootstrap Gimp::Lib $VERSION;

# functions to "autobless" where the autobless mechanism
# does not work.

sub gimp_list_images {
   map _autobless($_,&Gimp::PARAM_IMAGE),gimp_call_procedure "gimp_list_images";
}

sub gimp_image_get_layers {
   map _autobless($_,&Gimp::PARAM_LAYER),gimp_call_procedure "gimp_image_get_layers",@_;
}

sub gimp_image_get_channels {
   map _autobless($_,&Gimp::PARAM_CHANNEL),gimp_call_procedure "gimp_image_get_channels",@_;
}

sub gimp_gdrawable_width	{ $_[0]->{_width}	}
sub gimp_gdrawable_height	{ $_[0]->{_height}	}
sub gimp_gdrawable_ntile_rows	{ $_[0]->{_ntile_rows}	}
sub gimp_gdrawable_ntile_cols	{ $_[0]->{_ntile_cols}	}
sub gimp_gdrawable_bpp		{ $_[0]->{_bpp}		}
sub gimp_gdrawable_id		{ $_[0]->{_id}		}

sub gimp_pixel_rgn_x		{ $_[0]->{_x}		}
sub gimp_pixel_rgn_y		{ $_[0]->{_y}		}
sub gimp_pixel_rgn_w		{ $_[0]->{_w}		}
sub gimp_pixel_rgn_h		{ $_[0]->{_h}		}
sub gimp_pixel_rgn_rowstride	{ $_[0]->{_rowstride}	}
sub gimp_pixel_rgn_bpp		{ $_[0]->{_bpp}		}
sub gimp_pixel_rgn_dirty	{ $_[0]->{_dirty}	}
sub gimp_pixel_rgn_shadow	{ $_[0]->{_shadow}	}
sub gimp_pixel_rgn_drawable	{ $_[0]->{_drawable}	}

sub gimp_tile_ewidth		{ $_[0]->{_ewidth}	}
sub gimp_tile_eheight		{ $_[0]->{_eheight}	}
sub gimp_tile_bpp		{ $_[0]->{_bpp}		}
sub gimp_tile_shadow		{ $_[0]->{_shadow}	}
sub gimp_tile_gdrawable		{ $_[0]->{_gdrawable}	}

1;
__END__

=head1 NAME

Gimp::Lib - Interface to libgimp (as opposed to Gimp::Net)

=head1 SYNOPSIS

  use Gimp; # internal use only

=head1 DESCRIPTION

This is the package that interfaces to The Gimp via the libgimp interface,
i.e. the normal interface to use with the Gimp. You don't normally use this
module directly, look at the documentation for the package "Gimp".

=head1 AUTHOR

Marc Lehmann <pcg@goof.com>

=head1 SEE ALSO

perl(1), L<Gimp>.

=cut
