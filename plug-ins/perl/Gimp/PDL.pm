package Gimp::PDL;

use Carp;
use Gimp ();

warn "use'ing Gimp::PDL is no longer necessary, please remove it\n";

1;
__END__

=head1 NAME

Gimp::PDL - Overwrite Tile/Region functions to work with piddles.
This module is obsolete, please remove any references to it.

=head1 SYNOPSIS

  use Gimp;
  use Gimp::PDL;
  use PDL;

=head1 DESCRIPTION

This module overwrites some methods of Gimp::Tile and Gimp::PixelRgn. The
new functions return and accept piddles. The last argument (height) of
C<gimp_pixel_rgn_set_rect> is calculated from the piddle. There is no
other way to access the raw pixeldata in Gimp.

Some exmaples:

 $region = $drawable->get->pixel_rgn (0,0, 100,100, 1,0);
 $pixel = $region->get_pixel (5,7);	# fetches the pixel from (5|7)
 print $pixel;				# outputs something like
 					# [255, 127, 0], i.e. in
 					# RGB format ;)
 $region->set_pixel ($pixel * 0.5, 5, 7);# darken the pixel
 $rect = $region->get_rect (3,3,70,20);	# get a horizontal stripe
 $rect = $rect->hclip(255/5)*5;		# clip and multiply by 5
 $region->set_rect($rect);		# and draw it!
 undef $region;				# and update it!

=head1 AUTHOR

Marc Lehmann <pcg@goof.com>

=head1 SEE ALSO

L<Gimp::Pixel>, perl(1), Gimp(1).

=cut
