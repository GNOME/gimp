#!/usr/bin/perl

use Gimp;
use Gimp::Fu;
use Gimp::Util;

sub new_scratchlayer {
    my($image,$length,$gamma,$angle)=@_;
    my $type=$image->layertype(0);
    my($layer)=$image->layer_new ($image->width, $image->height, $image->layertype(0),
                                  "displace layer ($angle)", 100, NORMAL_MODE);
    $layer->add_layer(-1);
    $layer->fill (WHITE_IMAGE_FILL);
    $layer->noisify (0, 1, 1, 1, 0);
    $layer->mblur (0, $length, $angle);
    $layer->levels (VALUE, 120, 255, $gamma, 0, 255);

    $layer;
}

register "scratches",
         "Create a scratch effect",
         "Add scratches to an existing image. Works best on a metallic-like background.",
         "Marc Lehmann",
         "Marc Lehmann <pcg\@goof.com>",
         "19990223",
         "<Image>/Filters/Distorts/Scratches",
         "*",
         [
          [PF_SLIDER	, "angle_x"	, "The horizontal angle"	,  30, [  0, 360]],
          [PF_SLIDER	, "angle_y"	, "The vertical angle"		,  70, [  0, 360]],
          [PF_SLIDER	, "gamma"	, "Scratch map gamma"		, 0.3, [0.1,  10, 0.05]],
          [PF_SPINNER	, "smoothness"	, "The scratch smoothness"	,  15, [  0, 400]],
          [PF_SPINNER	, "length"	, "The scratch length"		,  10, [  0, 400]],
          #[PF_BOOL,	, "bump_map"	, "Use bump map instead of displace", 0],
         ],
         sub {
   my($image,$drawable,$anglex,$angley,$gamma,$length,$width)=@_;

   $image->undo_push_group_start;
   
   my $layer1 = new_scratchlayer ($image, $length, $gamma, $anglex);
   my $layer2 = new_scratchlayer ($image, $length, $gamma, $angley);
   
   $drawable->displace ($width, $width, 1, 1, $layer1, $layer2, WRAP);
   
   $layer1->remove_layer;
   $layer2->remove_layer;

   $image->undo_push_group_end;

   $image;
};

exit main;










