#!/usr/bin/perl
# This is (hopefully) a demonstration of how pathetically easy it is to script
# a neato effect you've come up with.  This lil' effect was created by xach,
# and translated by sjburges (me).  You can consider it released under the GPL
# or Artistic liscence, whichever makes you happier.
#
# <Xach> sjburges: 1. pixelize the photo  2. in a new white layer, render a grid
#           at the same resolution as the pixelize, then blur it. threshold the 
#           grid until you get a roundish blob in the center of each square (you
#           may need to repeat a few times).
# <Xach> sjburges: meanwhile, back at the pixelized image, bumpmap it with 
#           itself and a depth of about 5. do this twice. then bumpmap it with 
#           the round blobby layer.
# <Xach> then create a new, clean grid, and bumpmap the pixelized layer with it
#

# (To get a decent blobby grid)
# <Xach> <Xach> render a grid at 10x10, gaussian blur at 7, then set levels to 
#           196 1.00 234                                                    

# Revision - 1.1:  	added a gimp_displays_flush() for 1.0.x users
#            		stopped deleting the layers after removal - it was
#                   causing bad things to happen with refcounts.  I hope
#                   gimp is cleaning up this memory on its own...
#            1.2:   Fixed buggy selection handling - oops ;)  
#            1.3:   Added undo capability by Marc Lehman <pcg@goof.com>
#            1.4:   Marc Lehman <pcg@goof.com>, changed function name
#            1.5:   Seth Burgess <sjburges@gimp.org> added my email, put it
#                   in a directory more suitable than the lame "Misc"
# Here's the boring start of every script...

use Gimp qw(:auto __ N_);
use Gimp::Fu;

register "xach_blocks",
         "Xach's Blocks o' Fun",
         "Turn your picture into something that resembles a certain trademarked
          building block creation",
         "Seth Burgess",
         "Seth Burgess <sjburges\@gimp.org>",
         "2-15-98",
         N_"<Image>/Filters/Map/Xach Blocks",
         "*",
         [
          [PF_SLIDER,	"block_size",	"The size of the blocks...", 10, [0, 255, 1]],
          [PF_SLIDER,	"knob_factor",	"The size of your knob...", 67, [0, 100, 5]],
         ],
         sub {
   my($img,$drawable,$blocksize, $knobfactor)=@_;
 	$selection_flag = 0;
        eval { $img->undo_push_group_start };
	if (!$drawable->has_alpha) {
		$drawable->add_alpha;
		}; 
	if ($img->selection_is_empty) {
		$img->selection_all;
		$selection_flag = 1;
		}
	$oldbackground = gimp_palette_get_background();
# Now the fun begins :) 

	$selection = $img->selection_save;
  
#1. Pixelize the photo
	$drawable->plug_in_pixelize($blocksize); 
# 2. in a new white layer, render a grid
#           at the same resolution as the pixelize, then blur it.
	$gridlayer = $img->layer_new($img->width, $img->height, RGBA_IMAGE, "Grid 1", 100, 0);
	$img->add_layer($gridlayer,0);
	$img->selection_all;
	gimp_edit_clear($gridlayer);
	gimp_palette_set_background([255,255,255]);
	gimp_edit_fill($gridlayer);
	$gridlayer->plug_in_grid($blocksize, $blocksize, 0, 0);
	$gridlayer->plug_in_gauss_iir(0.7*$blocksize, 1, 1);

#	threshold the 
#   grid until you get a roundish blob in the center of each square (you
#   may need to repeat a few times).	

	$gridlayer->levels(0, 196, 234, $knobfactor/100.0 , 0, 255);

# <Xach> sjburges: meanwhile, back at the pixelized image, bumpmap it with 
#           itself and a depth of about 5. do this twice.  
	gimp_selection_load($selection);
	$drawable->plug_in_bump_map($drawable, 135, 45, 5, 0, 0, 0, 0, 1, 0, 0);
	$drawable->plug_in_bump_map($drawable, 135, 45, 5, 0, 0, 0, 0, 1, 0, 0);
	$drawable->plug_in_bump_map($gridlayer, 135, 45, 5, 0, 0, 0, 0, 1, 0, 0);
 
# <Xach> then create a new, clean grid, and bumpmap the pixelized layer with it
	$img->selection_all;	
	$cleangrid = $img->layer_new($img->width, $img->height, 
                                 RGBA_IMAGE, "Grid 2", 100, 0);
	$img->add_layer($cleangrid,0);
	gimp_edit_fill($cleangrid);
	$cleangrid->plug_in_grid($blocksize, $blocksize, 0, 0);
	gimp_selection_load($selection);
	$drawable->plug_in_bump_map($cleangrid, 135, 45, 3, 0, 0, 0, 0, 1, 0, 0);
	$img->selection_all;	

# Clean up stuff

	$img->remove_layer($cleangrid);
#	$cleangrid->delete;  # Deleting these layers after removal seems to cause
                         # strange problems (I think gimp handles this 
                         # automatically now)
	$img->remove_layer($gridlayer);
#	$gridlayer->delete;
	gimp_selection_load($selection);
	gimp_palette_set_background($oldbackground);
	if ($selection_flag ==1) {
		$img->selection_none;
		}
        eval { $img->undo_push_group_end };
	return ();
};

exit main;

