#!/usr/bin/perl

# this test-plugin will create a simple button, and does automatically
# save it as an indexed gif in /tmp/x.gif

# it works as plug-in as well as standalone!
# this script is old (its the first script ever written for gimp-perl)
# and I had no time to fix it yet.

use Gimp;

$blend1 = [0, 150, 255];
$blend2 = [0, 255, 208];
$black  = "#000000";
$font   = "Engraver";

# enable example mode... if disabled, it will write out some logos, and not
# wont' display anything.
$example = 1;

# set trace level to watch functions as they are executed
Gimp::set_trace(TRACE_NAME) if $example;

sub set_fg ($) { gimp_palette_set_foreground ($_[0]) };
sub set_bg ($) { gimp_palette_set_background ($_[0]) };
sub get_fg ($) { gimp_palette_get_foreground () };
sub get_bg ($) { gimp_palette_get_background () };

# shorthand function for drawing text
sub text($$$$$) {
  my($img,$text,$border,$font,$size)=@_;
  my $layer=gimp_text($img,-1,0,0,$text,$border,1,$size,PIXELS,"*",$font,"*","*","*","*");
  if (wantarray()) {
    ($layer,gimp_text_get_extents($text,$size,PIXELS,"*",$font,"*","*","*","*"));
  } else {
    $layer;
  }
}

# convert image to indexed
# and automatically save it as interlaced gif.
sub index_and_save($$) {
  my($img,$path)=@_;
  gimp_image_flatten($img);
  gimp_convert_indexed_palette($img,1,0,32,"");
  file_gif_save(RUN_NONINTERACTIVE,$img,-1,$path,$path,1,0,0,0) unless $example;
}

sub write_logo {
  my($string,$active,$w,$h,$uc)=@_;

  # create a new image
  my $img=gimp_image_new($w,$h,RGB);

  # and a layer for it
  my $bg=gimp_layer_new($img,$w,$h,RGB_IMAGE,"Background",100,NORMAL_MODE);

  gimp_image_add_layer($img,$bg,1);

  set_fg($blend1);
  set_bg($blend2);

  # blend the background
  gimp_blend($bg,FG_BG_HSV,NORMAL_MODE,LINEAR,100,0,
             REPEAT_NONE,0,0,0,
             0,0,$w*0.9,$h);
  gimp_rect_select ($img,$w*0.92,0,$w,$h,REPLACE, 0, 0);
  gimp_blend($bg,FG_BG_HSV,NORMAL_MODE,LINEAR,100,0,
             REPEAT_NONE,0,0,0,
             $w,0,$w*0.92,0);
  gimp_selection_all($img);

  set_fg($black);

  my ($text,$tw,$th,$ta,$td) = text ($img, $string, 1, $font, $active ? $h*0.7 : $h*0.5);

  gimp_layer_translate ($text,($w-$tw)/2,($h-$th+$td)/2);

  my ($shadow) = gimp_layer_copy ($text, 0);

  plug_in_gauss_rle ($text, 1, 1, 1) unless $active;

  gimp_image_add_layer ($img,$shadow,1);

  gimp_shear ($shadow,1,ORIENTATION_HORIZONTAL,-$th);
  gimp_layer_scale ($shadow, $tw, $th*0.3, 1);
  gimp_layer_translate ($shadow, $th*0.1, $th*0.3);
  plug_in_gauss_rle ($shadow, 1, 1, 1);

  gimp_hue_saturation($bg, ALL_HUES, 0, 0, $active ? 10 : -40);

  plug_in_nova ($bg, $h*0.4, $h*0.5, '#f0a020', 5, 50) if $active;
  plug_in_nova ($bg, $w-$h*0.4, $h*0.5, '#f0a020', 5, 50) if $active;

  # add an under construction sign
  if ($uc) {
    set_fg($active ? "#a00000" : "#000000");
    my ($uc,$tw,$th,$ta,$td) = text ($img, "u/c", 1, $font, $h*0.4);
    gimp_rotate ($uc,1,0.2);
    gimp_layer_translate ($uc,$w*0.84,($h-$th+$td)/2);
  }

  index_and_save ($img, "/root/www/src/marc/images/${string}_".($active ? "on" : "off").".gif");

  gimp_display_new ($img) if $example;
  gimp_image_delete($img) unless $example;
}

# the extension that's called.
sub extension_homepage_logo {
  # if in example mode just draw one example logo.
  if($example) {
    push(@logos,[ "-Projects", 0, 480, 60 ]);
#    push(@logos,[ "-Projects", 1, 480, 60 ]);
  } else {
    for $active (0, 1) {
      for $string (qw(Projects -Background -Main)) {
        push(@logos,[$string,$active,240,30]);
      }
    }
    for $active (0, 1) {
      for $string (qw(PGCC Judge -FreeISDN -Gimp -Destripe -Links -EGCS)) {
        push(@logos,[$string,$active,240,20]);
      }
    }
  }
  gimp_progress_init ("rendering buttons...");
  $numlogos = $#logos+1;
  while($#logos>=0) {
     gimp_progress_update (1-($#logos+1)/$numlogos);
     my($string,$active,$w,$h)=@{pop(@logos)};
     $uc=$string=~s/^-//;
     write_logo($string,$active,$w,$h,$uc);
  }
}

sub query {
  gimp_install_procedure("extension_homepage_logo", "a test extension in perl",
                        "try it out", "Marc Lehmann", "Marc Lehmann", "1997-02-06",
                        "<Toolbox>/Xtns/Homepage-Logo", "*", PROC_EXTENSION,
                        [[PARAM_INT32, "run_mode", "Interactive, [non-interactive]"]], []);
}

sub net {
  extension_homepage_logo;
}

exit main;

