#!/usr/bin/perl

#BEGIN {$^W=1};

use Gimp::Feature qw(pdl);
use Gimp;
use Gimp::Fu;
use Gimp::PDL;
use PDL::LiteF;

# Gimp::set_trace(TRACE_ALL);

register "border_average",
         "calculates the average border colour",
         "calulcates the average border colour",
         "Marc Lehmann",
         "Marc Lehmann",
         "0.2.1",
         "<Image>/Filters/Misc/Border Average",
         "RGB",
         [
          [PF_INT32,	"thickness",		"Border size to take in count", 10],
          [PF_INT32,	"bucket_exponent",	"Bits for bucket size (default=4: 16 Levels)", 4],
         ],
         [
          [PF_COLOUR,	"border_colour",	"Average Border Colour"],
         ],
         sub {					# es folgt das eigentliche Skript...
   my($image,$drawable,$thickness,$exponent)=@_;

   ($empty,@bounds)=$drawable->mask_bounds();
   return () if $empty;

   my $rexpo = 8-$exponent;
   my $bucket_num = 1<<$exponent;

   # ideally, we'd use a three-dimensional array, but index3 isn't
   # implemented yet, so we do it flat (Still its nicer than C).
   my $cube = zeroes long,$bucket_num**3;

   my $width  = $drawable->width;
   my $height = $drawable->height;

   $thickness=$width  if $thickness>$width;
   $thickness=$height if $thickness>$height;

   local *add_new_colour = sub($) {
      # linearize and quantize pixels (same as original, slightly wrong)
      my $pixels = $_[0] >> $rexpo;

      # intead of something like
      # $cube->index3d($pixels)++;
      # we have to first flatten the rgb triples into indexes and use index instead

      my $flatten = long([$bucket_num**2,$bucket_num**1,$bucket_num**0]);
      my $subcube = $cube->index(inner($pixels,$flatten)->clump(2));

      $subcube++;
   };

   Gimp->progress_init("Border Average", 0);
   add_new_colour ($drawable->get->pixel_rgn ($bounds[0]           ,$bounds[1]           , $thickness,$height, 0, 0)
                            ->get_rect(0,0, $thickness,$height));
   add_new_colour ($drawable->get->pixel_rgn ($bounds[2]-$thickness,$bounds[1]           , $thickness,$height, 0, 0)
                            ->get_rect(0,0, $thickness,$height));
   add_new_colour ($drawable->get->pixel_rgn ($bounds[0]           ,$bounds[1]           , $width ,$thickness, 0, 0)
                            ->get_rect(0,0, $width, $thickness));
   add_new_colour ($drawable->get->pixel_rgn ($bounds[0]           ,$bounds[3]-$thickness, $width ,$thickness, 0, 0)
                            ->get_rect(0,0, $width, $thickness));

   # now find the colour
   my $max = $cube->maximum_ind;
   my $b = $max                 % $bucket_num << $rexpo;
   my $g = ($max >>= $exponent) % $bucket_num << $rexpo;
   my $r = ($max >>= $exponent) % $bucket_num << $rexpo;

   if ($Gimp::Fu::run_mode != RUN_NONINTERACTIVE)
     {
       my $layer = new Layer ($image, width $image, height $image, RGB_IMAGE, "bordercolour", 100, NORMAL_MODE);
       add_layer $image $layer,0;
       Palette->set_background([$r,$g,$b]);
       $layer->edit_fill;
       Gimp->message("Added layer with border colour ($r,$g,$b) on top");
     }

   [$r,$g,$b];
};

exit main;

