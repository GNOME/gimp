#!/usr/bin/perl

use Gimp;
use Gimp::Fu;

register "gimp_fu_example_script",			# fill in a function name
         "A non-working example of Gimp::Fu usage",	# and a short description,
         "Just a starting point to derive new ".        # a (possibly multiline) help text
            "scripts. Always remember to put a long".
            "help message here!",
         "Marc Lehmann",				# don't forget your name (author)
         "(c) 1998, 1999 Marc Lehmann",			# and your copyright!
         "19990316",					# the date this script was written
         "<Toolbox>/Xtns/Gimp::Fu Example",		# the menu path
         "RGB*, GRAYA",					# image types to accept (RGB, RGAB amnd GRAYA)
         [
         # argument type, switch name	, a short description		, default value, extra arguments
          [PF_SLIDER	, "width"	, "The image width"		, 360, [300, 500]],
          [PF_SPINNER	, "height"	, "The image height"		, 100, [100, 200]],
          [PF_STRING	, "text"	, "The Message"			, "example text"],
          [PF_INT	, "bordersize"	, "The bordersize"		, 10],
          [PF_FLOAT	, "borderwidth"	, "The borderwidth"		, 1/5],
          [PF_FONT	, "font"	, "The Font Family"		],
          [PF_COLOUR	, "text_colour"	, "The (foreground) text colour", [10,10,10]],
          [PF_COLOUR	, "bg_colour"	, "The background colour"	, "#ff8000"],
          [PF_TOGGLE	, "ignore_cols" , "Ignore colours"		, 0],
          [PF_IMAGE	, "extra_image"	, "An additonal picture to ignore"],
          [PF_DRAWABLE	, "extra_draw"	, "Somehting to ignroe as well"	],
          [PF_RADIO	, "type"	, "The effect type"		, 0, [small => 0, large => 1]],
          [PF_BRUSH	, "a_brush"	, "An unused brush"		],
          [PF_PATTERN	, "a_pattern"	, "An unused pattern"		],
          [PF_GRADIENT	, "a_gradients"	, "An unused gradients"		],
         ],
         sub {
   
   # now do sth. useful with the garbage we got ;)
   my($width,$height,$text,$font,$fg,$bg,$ignore,$brush,$pattern,$gradient)=@_;
   
   # set tracing
   Gimp::set_trace(TRACE_ALL);

   my $img=new Image($width,$height,RGB);
   
   # put an undo group around any modifications, so that
   # they can be undone in one step. The eval shields against
   # gimp-1.0, which does not have this function.
   eval { $img->undo_push_group_start };
   
   my $l=new Layer($img,$width,$height,RGB,"Background",100,NORMAL_MODE);
   $l->add_layer(0);

   # now a few syntax examples
   
   Palette->set_foreground($fg) unless $ignore;
   Palette->set_background($bg) unless $ignore;
   
   fill $l BG_IMAGE_FILL;

   # the next function only works in gimp-1.1
   $text_layer=$img->text_fontname(-1,10,10,$text,5,1,xlfd_size($font),$font);

   gimp_palette_set_foreground("green");
   
   # close the undo push group
   eval { $img->undo_push_group_end };

   $img;	# return the image, or an empty list, i.e. ()
};

exit main;

