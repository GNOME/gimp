package Gimp::PDL;

use Carp;
use Gimp;
use PDL;

require Exporter;
require DynaLoader;
require AutoLoader;

@ISA = qw(Exporter);
@EXPORT = ();

$old_w = $^W; $^W = 0;

*old_set_data = \&Gimp::Tile::set_data;
*Gimp::Tile::set_data = sub {
   (my $p = byte $_[1])->make_physical;
   old_set_data($_[0],${$p->get_dataref});
};

*old_get_data = \&Gimp::Tile::get_data;
*Gimp::Tile::get_data = sub {
   my($tile)=@_;
   my($pdl)=new_from_specification PDL (byte,width(),height(),
                                        $tile->bpp > 1 ? $tile->bpp : ());
   ${$pdl->get_dataref} = old_get_data($tile);
   $pdl->upd_data;
   return $pdl;
};

# this tries to overwrite a function with another one. this is quite tricky
# (almost impossible in general), we only overwrite Gimp::<iface>::function
# and hope no other references are around.
sub rep ($&) {
   my($name,$sub)=@_;
   *{"old_$name"}=\&{"${Gimp::interface_pkg}::gimp_pixel_rgn_$name"};
   undef *{"${Gimp::interface_pkg}::gimp_pixel_rgn_$name"};
   *{"${Gimp::interface_pkg}::gimp_pixel_rgn_$name"}=$sub;
}

rep "get_pixel", sub($$$) {
   my($rgn)=@_;
   my($pdl)=new_from_specification PDL (byte,$_[0]->bpp);
   ${$pdl->get_dataref} = &old_get_pixel;
   $pdl->upd_data;
   return $pdl;
};

rep "get_col", sub($$$$) {
   my($rgn)=@_;
   my($pdl)=new_from_specification PDL (byte,$_[0]->bpp,$_[3]);
   ${$pdl->get_dataref} = &old_get_col;
   $pdl->upd_data;
   return $pdl;
};

rep "get_row", sub($$$$) {
   my($rgn)=@_;
   my($pdl)=new_from_specification PDL (byte,$_[0]->bpp,$_[3]);
   ${$pdl->get_dataref} = &old_get_row;
   $pdl->upd_data;
   return $pdl;
};

rep "get_rect", sub($$$$$) {
   my($pdl)=new_from_specification PDL (byte,$_[0]->bpp,$_[3],$_[4]);
   ${$pdl->get_dataref} = &old_get_rect;
   $pdl->upd_data;
   return $pdl;
};

rep "set_pixel", sub($$$$) {
   (my $p = byte $_[1])->make_physical;
   old_set_pixel($_[0],${$p->get_dataref},$_[2],$_[3]);
};

rep "set_col", sub($$$$) {
   (my $p = byte $_[1])->make_physical;
   old_set_col($_[0],${$p->get_dataref},$_[2],$_[3]);
};

rep "set_row", sub($$$$) {
   (my $p = byte $_[1])->make_physical;
   old_set_row($_[0],${$p->get_dataref},$_[2],$_[3]);
};

rep "set_rect", sub($$$$) {
   (my $p = byte $_[1])->make_physical;
   old_set_rect($_[0],${$p->get_dataref},$_[2],$_[3],($_[1]->dims)[1]);
};

$^W = $old_w; undef $old_w;

1;
__END__

=head1 NAME

Gimp::PDL - Overwrite Tile/Region functions to work with piddles.

=head1 SYNOPSIS

  use Gimp;
  use Gimp::PDL;	# must be use'd _after_ Gimp!
  use PDL;

=head1 DESCRIPTION

This module overwrites all methods of Gimp::Tile and Gimp::PixelRgn. The new
functions return and accept piddles instead of strings for pixel values. The
last argument (height) of C<gimp_pixel_rgn_set_rect> is calculated from the
piddle.

Some exmaples:

 $region = $drawable->pixel_rgn (0,0, 100,100, 1,0);
 $pixel = $region->get_pixel (5,7);	# fetches the pixel from (5|7)
 print $pixel;
 -> [255, 127, 0]			# RGB format ;)
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
