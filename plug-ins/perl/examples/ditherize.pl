#!/usr/bin/perl

use strict 'subs';
use Gimp;
use Gimp::Fu;

#
# this is quite convoluted, but I found no other way to do this than:
#
# create a new image & one layer
# copy & paste the layer
# ditherize new image
# copy & paste back
#

#Gimp::set_trace(TRACE_ALL);

my %imagetype2layertype = (
   RGB,		RGB_IMAGE,
   GRAY,	GRAY_IMAGE,
   INDEXED,	INDEXED_IMAGE,
);

register "plug_in_ditherize",
         "dithers current selection",
         "This script takes the current selection and dithers it just like convert to indexed",
         "Marc Lehmann",
         "Marc Lehmann",
         "1.1",
         "<Image>/Filters/Noise/Ditherize",
         "RGB*, GRAY*",
         [
          [PF_SLIDER,		"colours",	"The number of colours to dither to", 10, [0, 256]],
         ],
         sub {
   my($image,$drawable,$colours)=@_;
   
   $drawable->layer or die "this plug-in only works for layers";
   
   # make sure somehting is selected
   $drawable->mask_bounds or $image->selection_all;
   
   my ($x1,$y1,$x2,$y2)=($drawable->mask_bounds)[1..4];
   my ($w,$h)=($x2-$x1,$y2-$y1);
   
   my $sel = $image->selection_save;
   $image->rect_select($x1,$y1,$w,$h,SELECTION_REPLACE,0,0);
   $drawable->edit_copy;
   $sel->selection_load;
   $sel->remove_channel;
   
   my $copy = new Image($w, $h, $image->base_type);
   my $draw = new Layer($copy, $w, $h,
                        $imagetype2layertype{$image->base_type},
                        "temporary layer",100,NORMAL_MODE);
   $copy->add_layer ($draw, 1);
   $draw->edit_paste(0)->anchor;
   $copy->convert_indexed (1, $colours);
   
   $draw->edit_copy;
   $drawable->edit_paste(1)->anchor;
   $copy->delete;
    
   ();
};

exit main;










