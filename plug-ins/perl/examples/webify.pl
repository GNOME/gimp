#!/usr/bin/perl

use Gimp;
use Gimp::Fu;

#Gimp::set_trace(TRACE_ALL);

register "webify",
         "Make an image suitable for the web",
         "This plug-in converts the image to indexed, with some extra options",
         "Marc Lehmann",
         "Marc Lehmann",
         "1.0",
         __"<Image>/Filters/Misc/Webify",
         "RGB*, GRAY*",
         [
          [PF_BOOL,	"new",		"create a new image?", 1],
          [PF_BOOL,	"transparent",	"make transparent?", 1],
          [PF_COLOUR,	"bg_color",	"the background colour to use for transparency", "white"],
          [PF_SLIDER,	"threshold",	"the threshold to use for background detection", 3, [0, 255, 1]],
          [PF_INT32,	"colors",	"how many colours to use (0 = don't convert to indexed)", 32],
          [PF_BOOL,	"autocrop",	"autocrop at end?", 1],
         ],
         sub {
   my($img,$drawable,$new,$alpha,$bg,$thresh,$colours,$autocrop)=@_;

   $img = $img->channel_ops_duplicate if $new;

   eval { $img->undo_group_start };

   $drawable = $img->flatten;

   if ($alpha) {
      $drawable->add_alpha;
      $drawable->by_color_select($bg,$thresh,REPLACE,1,0,0,0);
      $drawable->edit_cut if $img->selection_bounds;
   }
   Plugin->autocrop($drawable) if $autocrop;
   $img->convert_indexed (1, $colours) if $colours;

   eval { $img->undo_group_end };

   $new ? ($img->clean_all, $img) : ();
};

exit main;

