#!/usr/bin/perl
#[Xach] start off with an image, then pixelize it
#[Xach] then add alpha->add layer mask                                   [20:21]
#[Xach] render a checkerboard into the layer mask
#[Xach] duplicate the image. fill the original with black, then blur the layer 
#           mask (i used 30% of pixelize size) and offset it by some value (i 
#           chose 20% of the pixelize size)
#[Xach] duplicate the duplicate, remove the layer mask, move it below everything
#[Xach] then add a new white layer on top, set the mode to multiply, and render 
#           a grid into it at pixelize size
#[Xach] that's a bit roundabout, but it's also in the xcf
#
# Because the way xach does it is a bit ackward, I'm switching it around a bit
# and working from the bottom up..

# Revision 1.1: Marc Lehman <pcg@goof.com> added undo capability
# Revision 1.2: Marc Lehman <pcg@goof.com>, changed function name

# Here's the boring start of every script...

use Gimp;
use Gimp::Fu;

register "xach_shadows",
         "Xach's Shadows o' Fun",
         "Screen of 50% of your drawing into a dropshadowed layer.",
         "Seth Burgess",
         "Seth Burgess",
         "1.2",
         "<Image>/Filters/Misc/Xach Shadows",
         "RGB*, GRAY*",
         [
          [PF_SLIDER,	"Block size",	"The size of the blocks...", 10, [0, 255, 1]],
         ],
         sub {
   my($img,$drawable,$blocksize) =@_;

        eval { $img->undo_push_group_start };
 #	$selection_flag = 0;
	if (!$drawable->has_alpha) {
		$drawable->add_alpha;
		}; 
# This only can be applied to an entire image right now..	
#	$selection = $img->selection_save;
    $img->selection_all;
	$oldbackground = gimp_palette_get_background();
# Now the fun begins :) 
	$drawable->plug_in_pixelize($blocksize); 
	$shadowlayer = $drawable->layer_copy(0);
	$img->add_layer($shadowlayer,0);
	$checkmask = $shadowlayer->create_mask(WHITE_MASK);
	$img->add_layer_mask($shadowlayer, $checkmask);
	plug_in_checkerboard ($img, $checkmask, 0, $blocksize);

	$frontlayer = $shadowlayer->layer_copy(0);
	$img->add_layer($frontlayer,0);
	gimp_palette_set_background([0,0,0]);
	$shadowlayer->fill(BG_IMAGE_FILL);	
	$checkmask->plug_in_gauss_iir(0.3*$blocksize, 1, 1);
	gimp_channel_ops_offset ($checkmask, 1, 0, 0.2*$blocksize, 0.2*$blocksize);


	$gridlayer = $img->layer_new($img->width, $img->height, RGBA_IMAGE, "Grid 1", 100, 0);
	$img->add_layer($gridlayer,0);
	$img->selection_all;
	gimp_edit_clear($gridlayer);
	gimp_palette_set_background([255,255,255]);
	gimp_edit_fill($gridlayer);
	$gridlayer->plug_in_grid($blocksize, $blocksize, 0, 0);

	gimp_layer_set_mode($gridlayer, 3);
# Clean up stuff
	gimp_palette_set_background($oldbackground);
        eval { $img->undo_push_group_end };
	gimp_displays_flush();
	return();
};
exit main;

