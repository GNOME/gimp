=head1 NAME

 Gimp::Util - some handy routines for Gimp-Perl users

=head1 SYNOPSIS

 use Gimp;
 use Gimp::Util;

=head1 DESCRIPTION

Gimp-Perl is nice, but when you have to write everytime 10 lines just to
get some simple functions done, it very quickly becomes tedious :-/

This module tries to define some functions that aim to automate frequently
used tasks, i.e. its a sort of catch-all-bag for (possibly) useful macro
functions.  If you want to add a function just mail the author of the
Gimp-Perl extension (see below).

In Gimp-Perl (but not in Gimp as seen by the enduser) it is possible to
have layers that are NOT attached to an image. This is, IMHO a bad idea,
you end up with them and the user cannot see them or delete them. So we
always attach our created layers to an image here, too avoid memory leaks
and debugging times.

These functions try to preserve the current settings like colors.

Also: these functions are handled in exactly the same way as
PDB-Functions, i.e. the (hypothetical) function C<gimp_image_xyzzy> can be
called as $image->xyzzy, if the module is available.

=head1 FUNCTIONS

=over 4

=cut

package      Gimp::Util;
require      Exporter;
@ISA       = qw(Exporter);
@EXPORT    = qw(
                layer_create 
                text_draw 
                image_create_text
                layer_add_layer_as_mask
               );
#@EXPORT_OK = qw();

$VERSION=1.001;

use Gimp;

##############################################################################
=pod

=item C<get_state ()>, C<set_state state>

C<get_state> returns a scalar representing most of gimps global state (at the
moment foreground colour and background colour). The state can later be
restored by a call to C<set_state>. This is ideal for library functions such
as the ones used here, at least when it includes more state in the future.

=cut

sub get_state() {
   [Palette->get_foreground,Palette->get_background];
}

sub set_state($) {
   Palette->set_foreground($_->[0]);
   Palette->set_background($_->[1]);
}

##############################################################################
=pod

=item C<layer_create image,name,color,pos>

create a colored layer, insert into image and return layer

=cut

# [12/23/98] v0.0.1 Tels - First version
sub layer_create {
  my ($image,$name,$color,$pos) = @_;
  my $layer;
  my $tcol; # scratch color

  # create a colored layer
  $layer = gimp_layer_new ($image,gimp_image_width($image),
                           gimp_image_height($image),
                           RGB_IMAGE,$name,100,NORMAL_MODE);
  $tcol = gimp_palette_get_background ();
  gimp_palette_set_background ($color);
  gimp_drawable_fill ($layer,BG_IMAGE_FILL);
  gimp_image_add_layer($image, $layer, $pos);
  gimp_palette_set_background ($tcol); # reset
  $layer;  
  }

##############################################################################
=pod

=item C<text_draw image,layer,text,font,size,fgcolor>

Create a colored text, draw over a background, add to img, ret img.

=cut

# [12/23/98] v0.0.1 Tels - First version
sub text_draw {
  my ($image,$layer,$text,$font,$size,$fgcolor) = @_;
  my ($bg_layer,$text_layer);
  my $tcol; # temp. color

  warn ("text string is empty") if ($text eq "");
  warn ("no font specified, using default") if ($font eq "");
  $font = "Helvetica" if ($font eq "");

  $tcol = gimp_palette_get_foreground ();
  gimp_palette_set_foreground ($fgcolor);
  # Create a layer for the text.
  $text_layer = gimp_text($image,-1,0,0,$text,10,1,$size,
			    PIXELS,"*",$font,"*","*","*","*"); 
    
  # Do the fun stuff with the text.
  gimp_layer_set_preserve_trans($text_layer, FALSE);

  if ($resize == 0)
    {
    # Now figure out the size of $image
    $width = gimp_image_width($text_layer);
    $height = gimp_image_height($text_layer);
    # and cut text layer
    }
  else
    {
    }
					  
  # add text to image
  gimp_image_add_layer($image, $text_layer, $pos);
  # merge white and text
  gimp_image_merge_visible_layers ($image,1);
  # cleanup the left over layer (!) 
  gimp_layer_delete($text_layer);
  $layer;
}
    
##############################################################################
=pod

=item C<image_create_text text,font,size,fgcolor,bgcolor>

Create an image, add colored text layer on a background layer, return img.

=cut

# [12/23/98] v0.0.1 Tels - First version
sub image_create_text {
  my ($text,$font,$size,$fgcolor,$bgcolor) = @_;
  my $tcol; # temp. color
  my $text_layer;
  my $bg_layer;
  my $image;

  warn ("text string is empty") if ($text eq "");
  warn ("no font specified, using default") if ($font eq "");
  $font = "Helvetica" if ($font eq "");
  # create an image. We'll just set whatever size here because we want
  # to resize the image when we figure out how big the text is.
  $image = gimp_image_new(64,64,RGB); # don't waste too much  resources ;-/
    
  $tcol = gimp_palette_get_foreground ();
  gimp_palette_set_foreground ($fgcolor);
  # Create a layer for the text.
  $text_layer = gimp_text($image,-1,0,0,$text,10,1,$size,
                          PIXELS,"*",$font,"*","*","*","*"); 
  gimp_palette_set_foreground ($tcol);
    
  gimp_layer_set_preserve_trans($text_layer, FALSE);

  # Resize the image based on size of text.
  gimp_image_resize($image,gimp_drawable_width($text_layer),
                    gimp_drawable_height($text_layer),0,0);

  # create background and merge them
  $bg_layer = layer_create ($image,"text",$bgcolor,1);
  gimp_image_merge_visible_layers ($image,1);

  # return
  $image;
  }

##############################################################################
=pod

=item C<layer_add_layer_as_mask image,layer,layermask>

Take a layer and add it as a mask to another layer, return mask.

=cut

# [12/23/98] v0.0.1 Tels - First version
sub layer_add_layer_as_mask {
  my ($image,$layer,$layer_mask) = @_;
  my $mask;

  gimp_selection_all ($image);  
  $layer_mask->edit_copy ();  
  gimp_layer_add_alpha ($layer); 
  $mask = gimp_layer_create_mask ($layer,0);
  $mask->edit_paste (0);
  gimp_floating_sel_anchor(gimp_image_floating_selection($image));
  gimp_image_add_layer_mask ($image,$layer,$mask);
  $mask;
  }

# EOF, needed for package?

# Sure, consider a package just another kind of function, and
# require/use etc. check for a true return value
1;

##############################################################################
# all functions below are by Marc Lehmann
=pod

=item C<gimp_text_wh $text,$fontname>

returns the width and height of the "$text" of the given font (XLFD format)

=cut
sub gimp_text_wh {
   (Gimp->text_get_extents_fontname($_[0],xlfd_size $_[1],$_[1]))[0,1];
}

=pod

=item C<gimp_image_layertype $alpha>

returns the corresponding layer type for an image, alpha controls wether the layer type
is with alpha or not. Example: imagetype: RGB -> RGB_IMAGE (or RGBA_IMAGE).

=cut
sub gimp_image_layertype {
   my $type = $_[0]->base_type;
   $type == RGB     ? $alpha ? RGBA_IMAGE     : RGB_IMAGE :
   $type == GRAY    ? $alpha ? GRAYA_IMAGE    : GRAY_IMAGE :
   $type == INDEXED ? $alpha ? INDEXEDA_IMAGE : INDEXED_IMAGE :
   die;
}

=pod

=back

=head1 AUTHOR

Various, version 1.000 written mainly by Tels (http://bloodgate.com/). The author
of the Gimp-Perl extension (contact him to include new functions) is Marc
Lehmann <pcg@goof.com>



