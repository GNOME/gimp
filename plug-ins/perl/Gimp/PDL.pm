package Gimp::PDL;

use Carp;
use Gimp ();
use PDL;

sub Gimp::Tile::set_data($) {
   (my $p = byte $_[1])->make_physical;
   Gimp::Tile::_set_data($_[0],${$p->get_dataref});
};

sub Gimp::Tile::get_data($) {
   my($tile)=@_;
   my($pdl)=new_from_specification PDL (byte,width(),height(),
                                        $tile->bpp > 1 ? $tile->bpp : ());
   ${$pdl->get_dataref} = Gimp::Tile::_get_data(@_);
   $pdl->upd_data;
   return $pdl;
};

sub Gimp::PixelRgn::get_pixel {
   my($rgn)=@_;
   my($pdl)=new_from_specification PDL (byte,$_[0]->bpp);
   ${$pdl->get_dataref} = Gimp::PixelRgn::_get_pixel(@_);
   $pdl->upd_data;
   return $pdl;
};

sub Gimp::PixelRgn::get_col {
   my($rgn)=@_;
   my($pdl)=new_from_specification PDL (byte,$_[0]->bpp,$_[3]);
   ${$pdl->get_dataref} = Gimp::PixelRgn::__get_col(@_);
   $pdl->upd_data;
   return $pdl;
};

sub Gimp::PixelRgn::get_row {
   my($rgn)=@_;
   my($pdl)=new_from_specification PDL (byte,$_[0]->bpp,$_[3]);
   ${$pdl->get_dataref} = Gimp::PixelRgn::_get_row(@_);
   $pdl->upd_data;
   return $pdl;
};

sub Gimp::PixelRgn::get_rect {
   my($pdl)=new_from_specification PDL (byte,$_[0]->bpp,$_[3],$_[4]);
   ${$pdl->get_dataref} = Gimp::PixelRgn::_get_rect(@_);
   $pdl->upd_data;
   return $pdl;
};

sub Gimp::PixelRgn::set_pixel {
   (my $p = byte $_[1])->make_physical;
   Gimp::PixelRgn::_set_pixel($_[0],${$p->get_dataref},$_[2],$_[3]);
};

sub Gimp::PixelRgn::set_col {
   (my $p = byte $_[1])->make_physical;
   Gimp::PixelRgn::_set_col($_[0],${$p->get_dataref},$_[2],$_[3]);
};

sub Gimp::PixelRgn::set_row {
   (my $p = byte $_[1])->make_physical;
   Gimp::PixelRgn::_set_row($_[0],${$p->get_dataref},$_[2],$_[3]);
};

sub Gimp::PixelRgn::set_rect {
   (my $p = byte $_[1])->make_physical;
   Gimp::PixelRgn::_set_rect($_[0],${$p->get_dataref},$_[2],$_[3],($_[1]->dims)[1]);
};

1;
__END__

=head1 NAME

Gimp::PDL - Overwrite Tile/Region functions to work with piddles.

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

 $region = $drawable->pixel_rgn (0,0, 100,100, 1,0);
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

perl(1), Gimp(1).

=cut
