package Gimp::Lib;

use strict;
use Carp;
use vars qw($VERSION @ISA);
use base qw(DynaLoader);

require DynaLoader;

$VERSION = $Gimp::VERSION;

use subs qw(
	gimp_call_procedure		gimp_main	gimp_init
	_gimp_procedure_available	set_trace	gimp_end
        initialized
);

sub gimp_init {
   die "gimp_init not implemented for the Lib interface";
}

sub gimp_end {
   die "gimp_end not implemented for in the Lib interface";
}

sub lock {
   # unimplemented, ignored
}

sub unlock {
   # unimplemented, ignored
}

sub import {}

bootstrap Gimp::Lib $VERSION;

# various functions for 1.0 compatibility

sub gimp_progress_init {
   push @_,-1 if @_<2;
   print "proggress_init yeah @_\n";
   eval { gimp_call_procedure "gimp_progress_init",@_ };
   gimp_call_procedure "gimp_progress_init",shift if $@;
}

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
sub gimp_pixel_rgn_shadow	{ $_[0]->{_shadow}	}
sub gimp_pixel_rgn_drawable	{ $_[0]->{_drawable}	}

sub gimp_tile_ewidth		{ $_[0]->{_ewidth}	}
sub gimp_tile_eheight		{ $_[0]->{_eheight}	}
sub gimp_tile_bpp		{ $_[0]->{_bpp}		}
sub gimp_tile_shadow		{ $_[0]->{_shadow}	}
sub gimp_tile_gdrawable		{ $_[0]->{_gdrawable}	}

# "server-side" perl code evaluation
sub server_eval {
   my @res = eval shift;
   die $@ if $@;
   @res;
}

# be careful not to require AUTOLOAD here
sub Gimp::PixelRgn::DESTROY {
   my $self = shift;
   return unless $self =~ /=HASH/;
   gimp_call_procedure "gimp_drawable_update",$self->{_drawable}->{_id},$self->{_x},$self->{_y},$self->{_w},$self->{_h}
      if gimp_pixel_rgn_dirty($self);
};

# this is here to be atomic over the perl-server
sub _gimp_append_data($$) {
   gimp_set_data ($_[0], gimp_get_data ($_[0]) . $_[1]);
}

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
