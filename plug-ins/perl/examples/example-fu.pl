#!/usr/bin/perl

use Gimp;
use Gimp::Fu;

register "my_first_gimp_fu",				# fill in name
         "My very first Gimp::Fu script",		# and a small description,
         "Just a starting point to derive new scripts",	# a help text
         "My name",					# don't forget your name (author)
         "My name (my copyright)",			# and your copyright!
         "19981006",					# the date this script was written
         "<Toolbox>/Xtns/MY Very First",		# the menu path
         "*",						# which image types do I accept (all)
         [
          [PF_SLIDER	, "width"	, "The image width"		, 360, [300, 500]],
          [PF_SPINNER	, "height"	, "The image height"		, 100, [100, 200]],
          [PF_STRING	, "text"	, "The Message"			, "example text"],
          [PF_FONT	, "font"	, "The Font Family"		],
          [PF_COLOR	, "text_colour"	, "The (foreground) text colour", [10,10,10]],
          [PF_COLOR	, "bg_colour"	, "The background colour"	, "#ff8000"],
          [PF_TOGGLE	, "ignore_cols" , "Ignore colours"		, 0],
          [PF_BRUSH	, "a_brush"	, "An unused brush"		],
          [PF_PATTERN	, "a_pattern"	, "An unused pattern"		],
          [PF_GRADIENT	, "a_gradients"	, "An unused gradients"		],
         ],
         sub {
   
   # now do sth. useful with the garbage we got ;)
   my($width,$height,$text,$font,$fg,$bg,$ignore,$brush,$pattern,$gradient)=@_;
   
   Gimp::set_trace(TRACE_ALL);

   my $img=new Image($width,$height,RGB);
   
   my $l=new Layer($img,$width,$height,RGB,"Background",100,NORMAL_MODE);
   $l->add_layer(-1);
   
   Palette->set_foreground($fg) unless $ignore;
   Palette->set_background($bg) unless $ignore;
   
   fill $l BG_IMAGE_FILL;
   $text_layer=$img->text_fontname(-1,10,10,$text,5,1,xlfd_size($font),$font);
   gimp_palette_set_foreground("green");
   
   $img;	# return the image, or undef
};

exit main;

