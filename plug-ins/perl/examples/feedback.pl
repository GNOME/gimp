#!/usr/bin/perl

# Revision 1.0: Released it
#          1.1: Marc Lehman added undo capability! <pcg@goof.com>
#          1.2: Added my email, and put it in "Noise" where it belongs 
#                <sjburges@gimp.org>


use Gimp;
use Gimp::Fu;

register "feedback",
         "Take an image and feed it back onto itself multiple times",
         "This plug-in simulates video feedback.  It makes for kinda a neat desktop if you're into that sort of thing",
         "Seth Burgess",
         "Seth Burgess <sjburges\@gimp.org>",
         "2-15-99",
         "<Image>/Filters/Noise/Feedback",
         "RGB, GRAY",
         [
          [PF_SLIDER,	"offset",	"the amount the frames will offset", 3, [0, 255, 1]],
          [PF_SLIDER,	"repeat",	"the number of times to repeat the illusion", 3, [0, 100, 1]],
         ],
         sub {
   my($img,$drawable,$offset,$repeat)=@_;
   
   eval { $img->undo_push_group_start };
   
   for (; $repeat>0; $repeat--) { 
   	 $drawable = $img->flatten;
   	 $copylayer = $drawable->copy(1);
	 $img->add_layer($copylayer,0);
	 $copylayer->scale($img->width - $offset, $img->height - $offset, 0);	
	}
	$img->flatten;
   eval { $img->undo_push_group_end };
	return();
};

exit main;

